#ifndef _MAKEDEPEND

#ifdef _DEBUG
#undef _DEBUG
#include <Python.h>
#define _DEBUG
#else
#include <Python.h>
#endif

#include <windows.h>
#include <dshow.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <gl/gl.h>
#endif

#include "dxvideo.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

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

PyObject* video_initialize( PyObject* self )
{
    printf( "initializing COM...\n" );
    CoInitialize( NULL );

    Py_INCREF( Py_None );
    return Py_None;
}

PyObject* video_uninitialize( PyObject* self )
{
    printf( "uninitializing COM...\n" );
    CoUninitialize();

    Py_INCREF( Py_None );
    return Py_None;
}
	
    
PyObject* video_create( PyObject* self, PyObject* args )
{
    video_object* vo = NULL;
    
    Py_UNICODE* filename = NULL;
    PyObject* soggy;
    PyObject* hwndstring;
    HWND hwnd;
    HRESULT hr;

    LONGLONG duration;

    if ( !PyArg_ParseTuple( args, "Ou", &soggy, &filename ) )
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

    vo->pVidWin->put_AutoShow( OAFALSE );
    vo->pVidWin->put_Visible( OAFALSE );
    
    vo->pEventEx->SetNotifyWindow( (OAHWND)hwnd, WM_KEYDOWN, 0 );
    
    vo->pMediaControl->RenderFile( filename );

    vo->pMediaSeeking->SetTimeFormat( &TIME_FORMAT_FRAME );
    vo->pMediaSeeking->GetDuration( &duration );
    vo->frames = (int)duration;

    vo->pVidWin->put_Owner((OAHWND)hwnd);
    vo->pVidWin->put_WindowStyle(WS_CHILD);

    hr = vo->pMediaControl->StopWhenReady();

#if 0
    {
	OAFilterState fs;
	hr = vo->pMediaControl->GetState( INFINITE, &fs );
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
    }
#endif

    vo->state = VSTATE_PAUSE;
    vo->rate = VRATE_NORM;
    vo->loop = 0;

    return PyCObject_FromVoidPtr( (void*)vo, free_video_object );
}

PyObject* video_place( PyObject* self, PyObject* args )
{
    PyObject* pvo;
    video_object* vo = NULL;
    long width, height;

    double ox, oy, ulen, theta, aspect;
    double x1, y1, x2, y2;
    int fit;
    int x, y, w, h;
    int winwidth, winheight;

    double MV[16];
    double P[16];
    int V[4];
    double tx, ty;
    
    if ( !PyArg_ParseTuple( args, "O(ddddd)(ii)i", &pvo,
			    &ox, &oy, &ulen, &theta, &aspect,
			    &winwidth, &winheight,
			    &fit ) )
	return NULL;

    // project coords of lower-left and upper-right corners
    
    glGetDoublev( GL_MODELVIEW_MATRIX, MV );
    glGetDoublev( GL_PROJECTION_MATRIX, P );
    glGetIntegerv( GL_VIEWPORT, V );

    x1 = MV[0] * ox + MV[4] * oy + MV[12];
    y1 = MV[1] * ox + MV[5] * oy + MV[13];

    tx = P[0] * x1 + P[4] * y1 + P[12];
    ty = P[1] * x1 + P[5] * y1 + P[13];

    x1 = V[0] + V[2] * (tx + 1.0) / 2.0;
    y1 = V[1] + V[3] * (ty + 1.0) / 2.0;

    theta *= M_PI / 180.0;
    tx = ox + ulen * cos(theta) - ulen/aspect * sin(theta);
    ty = oy + ulen * sin(theta) + ulen/aspect * cos(theta);

    x2 = MV[0] * tx + MV[4] * ty + MV[12];
    y2 = MV[1] * tx + MV[5] * ty + MV[13];

    tx = P[0] * x2 + P[4] * y2 + P[12];
    ty = P[1] * x2 + P[5] * y2 + P[13];

    x2 = V[0] + V[2] * (tx + 1.0) / 2.0;
    y2 = V[1] + V[3] * (ty + 1.0) / 2.0;

    y1 = winheight - y1;
    y2 = winheight - y2;

    // obtain size of video
    
    vo = (video_object*)PyCObject_AsVoidPtr( pvo );
    vo->pBasic->GetVideoSize( &width, &height );

    // compute position
    
    fit_video_frame( x1, y1, x2, y2, fit, width, height, &x, &y, &w, &h );
    vo->pVidWin->SetWindowPosition( x, y, w, h );
    //printf( "video window at (%d,%d) is %d x %d  [%d]\n", x, y, w, h, fit );

#if 1
    {
	// rewind to beginning
	
	LONGLONG pos = 0;
	vo->pMediaSeeking->SetPositions( &pos, AM_SEEKING_AbsolutePositioning,
					 NULL, AM_SEEKING_NoPositioning );
    }
#endif

    vo->pMediaControl->StopWhenReady();
    vo->pVidWin->put_Visible( OATRUE );
    //vo->pMediaControl->Pause();

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
      case VIDEO_STRETCH:
	break;
	    
      case VIDEO_WIDTH:
	h = w / vwidth * vheight;
	break;
	    
      case VIDEO_HEIGHT:
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
    vo->pMediaControl->StopWhenReady();
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
#if 0
	else
	{
	    printf( "event %d %x\n", evCode, evCode );
	}
#endif
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

PyObject* video_loop( PyObject* self, PyObject* args )
{
    video_object* vo;
    PyObject* pvo;
    int val;

    if ( !PyArg_ParseTuple( args, "Oi", &pvo, &val ) )
	return NULL;

    vo = (video_object*)PyCObject_AsVoidPtr( pvo );

    if ( val == 0 )
	vo->loop = 0;
    else if ( val == 1 )
	vo->loop = 1;
    else
	vo->loop = ! vo->loop;
    
    Py_INCREF( Py_None );
    return Py_None;
}

/** Local Variables: **/
/** dsp-name:"dxvideo" **/
/** End: **/
