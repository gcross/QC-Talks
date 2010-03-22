#include <windows.h>
#include <dshow.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "sl_stage.h"
#include "sl_video.h"

#define VSTATE_STOP   0
#define VSTATE_RUN    1
#define VSTATE_PAUSE  2

#define VRATE_NORM    2
#define VRATE_MAX     4
double rates[VRATE_MAX+1] = { 0.25, 0.5, 1.0, 2.0, 4.0 };

typedef struct
{
    int state;
    int frames;
    int rate;
    int loop;
    IGraphBuilder* pGraph;
    IMediaControl* pMediaControl;
    IMediaEvent* pEvent;
    IMediaEventEx* pEventEx;
    IBasicVideo* pBasic;
    IVideoWindow* pVidWin;
    IMediaSeeking* pMediaSeeking;
} video_object;

extern "C" void free_video_object( void* vovo );
static void fit_video_frame( double x1, double y1, double x2, double y2, int fit,
			     int vwidth, int vheight,
			     int* ix, int* iy, int* iw, int* ih );

void video_initialize( void )
{
    printf( "initializing COM...\n" );
    CoInitialize( NULL );
}

void video_uninitialize( void )
{
    printf( "uninitializing COM...\n" );
    CoUninitialize();
}
	
    
PyObject* video_create( PyObject* self, PyObject* args )
{
    video_object* vo = NULL;
    
    Py_UNICODE* filename = NULL;
    PyObject* soggy;
    PyObject* hwndstring;
    HWND hwnd;
    HRESULT hr;
    int loop;

    LONGLONG duration;

    if ( !PyArg_ParseTuple( args, "Oui", &soggy, &filename, &loop ) )
	return NULL;

    hwndstring = PyObject_CallMethod( soggy, "gethwnd", NULL );
    if ( hwndstring == NULL )
	return NULL;
    hwnd = (HWND)(strtoul( PyString_AsString( hwndstring ), NULL, 16 ));
    Py_DECREF( hwndstring );

    vo = (video_object*)malloc( sizeof( video_object ) );
    
    // Create the filter graph manager and query for interfaces.
    hr = CoCreateInstance( 
	CLSID_FilterGraph, 
	NULL, 
	CLSCTX_INPROC_SERVER, 
	IID_IGraphBuilder, 
	(void **)&(vo->pGraph) );

    if ( hr != S_OK )
    {
	PyErr_SetString( PyExc_RuntimeError, "CoCreateInstance failed:  DirectX runtime installed?" );
	return NULL;
    }

    vo->pGraph->QueryInterface(IID_IVideoWindow, (void **)&(vo->pVidWin));
    vo->pGraph->QueryInterface(IID_IMediaControl, (void **)&(vo->pMediaControl));
    vo->pGraph->QueryInterface(IID_IMediaEvent, (void **)&(vo->pEvent));
    vo->pGraph->QueryInterface(IID_IMediaEventEx, (void **)&(vo->pEventEx));
    vo->pGraph->QueryInterface(IID_IBasicVideo, (void**)&(vo->pBasic));
    vo->pGraph->QueryInterface(IID_IMediaSeeking, (void**)&(vo->pMediaSeeking));

    vo->pEventEx->SetNotifyWindow( (OAHWND)hwnd, WM_KEYDOWN, 0 );
    
    vo->pGraph->RenderFile( filename, NULL );

    vo->pMediaSeeking->SetTimeFormat( &TIME_FORMAT_FRAME );
    vo->pMediaSeeking->GetDuration( &duration );
    vo->frames = (int)duration;

    vo->pVidWin->put_Owner((OAHWND)hwnd);
    vo->pVidWin->put_WindowStyle(WS_CHILD);

    hr = vo->pMediaControl->Pause();

    {
	OAFilterState fs;
	hr = vo->pMediaControl->GetState( INFINITE, &fs );
#if 0
	switch( fs )
	{
	  case State_Stopped:
	    printf( "stopped\n" );
	    break;
	    
	  case State_Paused:
	    printf( "paused\n" );
	    break;
	    
	  case State_Running:
	    printf( "running\n" );
	    break;
	}
#endif
    }

    vo->pVidWin->put_Visible( OAFALSE );
    
    vo->state = VSTATE_PAUSE;
    vo->rate = VRATE_NORM;
    vo->loop = loop;

    return PyCObject_FromVoidPtr( (void*)vo, free_video_object );
}

PyObject* video_place( PyObject* self, PyObject* args )
{
    PyObject* pvo;
    video_object* vo = NULL;
    long width, height;
    
    double x1, y1, x2, y2;
    int fit;
    int x, y, w, h;

    if ( !PyArg_ParseTuple( args, "Oddddi", &pvo, &x1, &y1, &x2, &y2, &fit ) )
	return NULL;

    vo = (video_object*)PyCObject_AsVoidPtr( pvo );
    
    vo->pBasic->GetVideoSize( &width, &height );  
    fit_video_frame( x1, y1, x2, y2, fit, width, height, &x, &y, &w, &h );
    vo->pVidWin->SetWindowPosition( x, y, w, h );

    vo->pVidWin->put_Visible( OATRUE );

    {
	LONGLONG pos = 0;
	vo->pMediaSeeking->SetPositions( &pos, AM_SEEKING_AbsolutePositioning,
					 NULL, AM_SEEKING_NoPositioning );
    }

    
    Py_INCREF( Py_None );
    return Py_None;
}


static void fit_video_frame( double x1, double y1, double x2, double y2, int fit,
			     int vwidth, int vheight,
			     int* ix, int* iy, int* iw, int* ih )
{
    double x, y, w, h;

    x = x1 < x2 ? x1 : x2;
    y = y1 < y2 ? y1 : y2;
    w = fabs(x2 - x1);
    h = fabs(y2 - y1);
    switch( fit )
    {
      case IMAGE_STRETCH:
	break;
	    
      case IMAGE_WIDTH:
	h = w / vwidth * vheight;
	break;
	    
      case IMAGE_HEIGHT:
	w = h / vheight * vwidth;
	break;
	    
      default:
	if ( h * vwidth > w * vheight )
	    h = w / vwidth * vheight;
	else
	    w = h / vheight * vwidth;
	break;
    }
	
    x += (fabs(x2-x1) - w) * 0.5;
    y += (fabs(y2-y1) - h) * 0.5;

    *ix = (int)x;
    *iy = (int)y;
    *iw = (int)w;
    *ih = (int)h;
}

extern "C" void free_video_object( void* vovo )
{
    video_object* vo = (video_object*)vovo;

    if ( vo->pGraph )
    {
	vo->pVidWin->put_Visible(OAFALSE);
	vo->pMediaControl->Stop();
	
	vo->pVidWin->put_Owner(NULL);
	
	// Clean up.
	vo->pBasic->Release();
	vo->pVidWin->Release();
	vo->pMediaControl->Release();
	vo->pMediaSeeking->Release();
	vo->pEvent->Release();
	vo->pEventEx->Release();
	vo->pGraph->Release();
    }

    free( vo );
}

PyObject* video_hide( PyObject* self, PyObject* args )
{
    video_object* vo;
    PyObject* pvo;

    if ( !PyArg_ParseTuple( args, "O", &pvo ) )
	return NULL;

    vo = (video_object*)PyCObject_AsVoidPtr( pvo );

    vo->state = VSTATE_PAUSE;
    vo->pMediaControl->Pause();
    vo->pVidWin->put_Visible(OAFALSE);
    
    Py_INCREF( Py_None );
    return Py_None;
}

PyObject* video_eventcheck( PyObject* self, PyObject* args )
{
    video_object* vo;
    PyObject* pvo;
    long evCode, param1, param2;

    if ( !PyArg_ParseTuple( args, "O", &pvo ) )
	return NULL;

    vo = (video_object*)PyCObject_AsVoidPtr( pvo );

    while ( SUCCEEDED( vo->pEvent->GetEvent( &evCode, &param1, &param2, 0 ) ) )
    {
	vo->pEvent->FreeEventParams( evCode, param1, param2 );

	if ( evCode == EC_COMPLETE )
	{
	    if ( vo->loop )
	    {
		LONGLONG pos = 0;
		vo->pMediaSeeking->SetPositions( &pos, AM_SEEKING_AbsolutePositioning,
						 NULL, AM_SEEKING_NoPositioning );
		vo->pMediaControl->Run();
	    }
	}
    }
    
    Py_INCREF( Py_None );
    return Py_None;
}

PyObject* video_destroy( PyObject* self, PyObject* args )
{
    video_object* vo;
    PyObject* pvo;

    if ( !PyArg_ParseTuple( args, "O", &pvo ) )
	return NULL;

    vo = (video_object*)PyCObject_AsVoidPtr( pvo );
    
    vo->pMediaControl->Stop();
    
    // Clean up.
    vo->pBasic->Release();
    vo->pVidWin->Release();
    vo->pMediaControl->Release();
    vo->pMediaSeeking->Release();
    vo->pEvent->Release();
    vo->pEventEx->Release();
    vo->pGraph->Release();
    
    vo->pGraph = NULL;
    
    Py_INCREF( Py_None );
    return Py_None;
}

PyObject* video_seek( PyObject* self, PyObject* args )
{
    video_object* vo;
    PyObject* pvo;
    int step;
    int abs;
    LONGLONG pos, stoppos;
    OAFilterState state;
    HRESULT hr;

    if ( !PyArg_ParseTuple( args, "Oii", &pvo, &step, &abs ) )
	return NULL;

    vo = (video_object*)PyCObject_AsVoidPtr( pvo );
    
    hr = vo->pMediaControl->GetState( 10, &state );
    if ( hr == S_OK && (state == State_Stopped || state == State_Running) )
    {
	vo->pMediaControl->Pause();
	vo->state = VSTATE_PAUSE;
    }
    
    if ( abs )
	pos = 0;
    else
    {
	vo->pMediaSeeking->GetPositions( &pos, &stoppos );
	pos += step;
    }
    vo->pMediaSeeking->SetPositions( &pos, AM_SEEKING_AbsolutePositioning,
				     NULL, AM_SEEKING_NoPositioning );

    Py_INCREF( Py_None );
    return Py_None;
}

PyObject* video_makevisible( PyObject* self, PyObject* args )
{
    video_object* vo;
    PyObject* pvo;

    if ( !PyArg_ParseTuple( args, "O", &pvo ) )
	return NULL;

    vo = (video_object*)PyCObject_AsVoidPtr( pvo );
    
    vo->pVidWin->put_Visible( OATRUE );

    Py_INCREF( Py_None );
    return Py_None;
}

PyObject* video_play( PyObject* self, PyObject* args )
{
    video_object* vo;
    PyObject* pvo;

    if ( !PyArg_ParseTuple( args, "O", &pvo ) )
	return NULL;

    vo = (video_object*)PyCObject_AsVoidPtr( pvo );
    
    // Run the graph.
    vo->pMediaControl->Run();
    vo->state = VSTATE_RUN;

    Py_INCREF( Py_None );
    return Py_None;
}

PyObject* video_pause( PyObject* self, PyObject* args )
{
    video_object* vo;
    PyObject* pvo;
    int mode;

    if ( !PyArg_ParseTuple( args, "Oi", &pvo, &mode ) )
	return NULL;

    vo = (video_object*)PyCObject_AsVoidPtr( pvo );
    
    if ( vo->state == VSTATE_RUN && (mode == 0 || mode == -1) )
    {
	vo->pMediaControl->Pause();
	vo->state = VSTATE_PAUSE;
    }
    else if ( (vo->state == VSTATE_STOP || vo->state == VSTATE_PAUSE) &&
	      (mode == 1 || mode ==-1) )
    {
	vo->pMediaControl->Run();
	vo->state = VSTATE_RUN;
    }
	
    Py_INCREF( Py_None );
    return Py_None;
}

PyObject* video_stop( PyObject* self, PyObject* args )
{
    video_object* vo;
    PyObject* pvo;

    if ( !PyArg_ParseTuple( args, "O", &pvo ) )
	return NULL;

    vo = (video_object*)PyCObject_AsVoidPtr( pvo );
    
    vo->pMediaControl->Stop();

    Py_INCREF( Py_None );
    return Py_None;
}

PyObject* video_rate( PyObject* self, PyObject* args )
{
    video_object* vo;
    PyObject* pvo;
    int change;

    if ( !PyArg_ParseTuple( args, "Oi", &pvo, &change ) )
	return NULL;

    vo = (video_object*)PyCObject_AsVoidPtr( pvo );
    
    if ( change == 0 )
	vo->rate = VRATE_NORM;
    else
    {
	vo->rate += change;
	if ( vo->rate < 0 )
	    vo->rate = 0;
	else if ( vo->rate > VRATE_MAX )
	    vo->rate = VRATE_MAX;
    }
    
    vo->pMediaSeeking->SetRate( rates[vo->rate] );
    
    Py_INCREF( Py_None );
    return Py_None;
}

/** Local Variables: **/
/** dsp-name:"stage" **/
/** End: **/
