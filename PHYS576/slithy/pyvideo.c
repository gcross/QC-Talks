#ifndef _MAKEDEPEND

#ifdef _DEBUG
#undef _DEBUG
#include <Python.h>
#define _DEBUG
#else
#include <Python.h>
#endif

#include <windows.h>
#endif

#include "dxvideo.h"

static PyMethodDef DXVideoMethods[] = {
    { "initialize", video_initialize, METH_NOARGS },
    { "uninitialize", video_uninitialize, METH_NOARGS },
    { "create", video_create, METH_VARARGS },
    { "place", video_place, METH_VARARGS },
    { "hide", video_hide, METH_VARARGS },
    { "eventcheck", video_eventcheck, METH_VARARGS },
    { "destroy", video_destroy, METH_VARARGS },
    { "makevisible", video_makevisible, METH_VARARGS },
    { "play", video_play, METH_VARARGS },
    { "pause", video_pause, METH_VARARGS },
    { "stop", video_stop, METH_VARARGS },
    { "rate", video_rate, METH_VARARGS },
    { "seek", video_seek, METH_VARARGS },
    { "loop", video_loop, METH_VARARGS },
    { NULL, NULL }
};

void
#ifdef WIN32
__declspec( dllexport )
#endif
initdxvideo( void )
{
    PyObject* m;

    m = Py_InitModule( "dxvideo", DXVideoMethods );
}


    
    

/** Local Variables: **/
/** dsp-name:"dxvideo" **/
/** End: **/
