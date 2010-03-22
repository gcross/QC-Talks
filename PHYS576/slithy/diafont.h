#ifndef _DIAFONT_H
#define _DIAFONT_H

#ifndef _MAKEDEPEND

#ifdef _DEBUG
#undef _DEBUG
#include <Python.h>
#define _DEBUG
#else
#include <Python.h>
#endif

#include <ft2build.h>
#include FT_FREETYPE_H
#endif

extern PyObject* FontError;
extern PyTypeObject FontType;

typedef struct {
    double tleft, tright, tbottom, ttop;
    double width, height;
    double awidth, aheight;
    double offsetx, offsety;
} texture_character;

typedef struct
{
    PyObject_HEAD
    
    char* filename;
    int pixelsize;
    int num_glyphs;
    int symbolfont;
    unsigned int* char_index;
    int char_index_size;
    texture_character** tab;
    unsigned char* bitmap;
    int bw, bh;
    int has_texture;
    unsigned int texture;
    PyObject* name;
} FontObject;

typedef struct
{
    void (*start_using_font)( FontObject* );
    void (*finish_using_font)( FontObject* );
    void (*draw_character)( FontObject*, unsigned int, double, double, double*, double* );
    int (*get_dimensions)( FontObject*, unsigned int,
			   double*, double*,
			   double*, double*,
			   double*, double* );
    PyObject* font_class;
} FontObjectMethods;

#ifndef GL_ERROR_CHECK
#define GL_ERROR_CHECK(str) {int qqqq;if((qqqq=glGetError())!=GL_NO_ERROR)printf("%s: GL error %x\n", str, qqqq);}
#endif

#endif
