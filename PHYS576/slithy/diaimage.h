#ifndef _DIAIMAGE_H
#define _DIAIMAGE_H

#ifndef _MAKEDEPEND
#ifdef _DEBUG
#undef _DEBUG
#include <Python.h>
#define _DEBUG
#else
#include <Python.h>
#endif
#endif

extern PyObject* ImageError;
extern PyTypeObject ImageType;

typedef struct
{
    PyObject_HEAD
    
    int w, h;
    int tw, th;
    unsigned char* data;
    unsigned int texture;
    double tright, tbottom;
    int has_texture;
    int has_alpha;
    PyObject* filename;
} ImageObject;

typedef struct
{
    void (*draw_image)( ImageObject*, double, double, double, double, double, double );
    int (*make_texture)( ImageObject* );
} ImageObjectMethods;

#ifndef GL_ERROR_CHECK
#define GL_ERROR_CHECK(str) {int qqqq;if((qqqq=glGetError())!=GL_NO_ERROR)printf("%s: GL error %x\n", str, qqqq);}
#endif

#endif
