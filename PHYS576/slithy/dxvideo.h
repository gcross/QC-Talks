#ifndef _DXVIDEO_H
#define _DXVIDEO_H

#include <windows.h>
#include <dshow.h>

#ifdef _DEBUG
#undef _DEBUG
#include <Python.h>
#define _DEBUG
#else
#include <Python.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

    PyObject* video_initialize( PyObject* self );
    PyObject* video_uninitialize( PyObject* self );

    PyObject* video_create( PyObject* self, PyObject* args );
    PyObject* video_place( PyObject* self, PyObject* args );
    PyObject* video_hide( PyObject* self, PyObject* args );
    PyObject* video_eventcheck( PyObject* self, PyObject* args );
    PyObject* video_destroy( PyObject* self, PyObject* args );
    PyObject* video_makevisible( PyObject* self, PyObject* args );
	
    PyObject* video_pause( PyObject* self, PyObject* args );
    PyObject* video_play( PyObject* self, PyObject* args );
    PyObject* video_stop( PyObject* self, PyObject* args );
    PyObject* video_rate( PyObject* self, PyObject* args );
    PyObject* video_seek( PyObject* self, PyObject* args );
    PyObject* video_loop( PyObject* self, PyObject* args );
    

#ifdef __cplusplus
}
#endif

// note: values must match those of IMAGE_* constants in dr_draw.c
#define VIDEO_BOTH      0
#define VIDEO_WIDTH     1
#define VIDEO_HEIGHT    2
#define VIDEO_STRETCH   3

#endif

    

/** Local Variables: **/
/** dsp-name:"dxvideo" **/
/** End: **/
