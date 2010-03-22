#ifndef _DR_DRAW_H
#define _DR_DRAW_H

#include "sl_common.h"

extern PyObject* color_class;
extern PyObject* font_class;

extern double global_alpha;
extern double line_thickness;
extern int current_id;
extern double current_id_depth;

extern PyObject* current_camera;
extern PyObject* current_visible;
extern double viewport_aspect;
extern int auto_mark;
extern PyObject* context_name;

extern GLint viewport[4];
extern int current_IPM_dirty;
extern double current_IPM[16];

#ifndef FUNC_PY
#define FUNC_PY(fn)  PyObject* fn( PyObject* self, PyObject* args )
#endif
#ifndef FUNC_PY_NOARGS
#define FUNC_PY_NOARGS(fn)  PyObject* fn( PyObject* self )
#endif
#ifndef FUNC_PY_KEYWD
#define FUNC_PY_KEYWD(fn)  PyObject* fn( PyObject* self, PyObject* args, PyObject* keywds )
#endif


// mark.c

FUNC_PY( save_marked_cameras );
FUNC_PY( mark );
FUNC_PY( unproject );
FUNC_PY( draw_unproject );
FUNC_PY( draw_project );
PyObject* internal_mark( PyObject* name );

// xform.c

FUNC_PY( draw_translate );
FUNC_PY( draw_scale );
FUNC_PY( draw_rotate );
FUNC_PY( draw_shear );

// bezier.c

FUNC_PY( draw_bezier );

// path.c

FUNC_PY( draw_path_wide_stroke );
FUNC_PY( draw_path_stroke );
FUNC_PY( draw_path_arrow );
FUNC_PY( draw_path_fill );
FUNC_PY( compute_path_bbox );
void delete_invalid_lists( PyObject* pobj );

void wide_line_segment( double width, double x1, double y1, double x2, double y2, double first[], double last[] );
void wide_curve_segment( double width, int order, double c[], double first[], double last[] );
void wide_join( double a[4], double b[4] );

// grid.c

FUNC_PY( draw_grid );

// text.c

FUNC_PY_KEYWD( draw_diatext );

#define TEXT_RESET        0
#define TEXT_RESETFONT    1
#define TEXT_RESETCOLOR   2

// also needed in pyvideo.c
#define IMAGE_BOTH          0
#define IMAGE_WIDTH         1
#define IMAGE_HEIGHT        2
#define IMAGE_STRETCH       3

// color.c

double* get_color_from_args( PyObject* args );
double* get_color_from_object( PyObject* object );

// image.c

FUNC_PY_KEYWD( draw_image );

// stack.c

FUNC_PY_NOARGS( draw_push );
FUNC_PY_NOARGS( draw_pop );
void initialize_stack( void );
void clear_stack( void );

#define STACK_DEPTH_CHUNK  32
#define MAX_STACK_DEPTH    4096



extern PyObject* DrawError;

double* bezier_points( int* count, int order, double c[], double flatness, int tangents );

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef GL_ERROR_CHECK
#define GL_ERROR_CHECK(str) {int qqqq;if((qqqq=glGetError())!=GL_NO_ERROR)printf("%s: GL error %x\n", str, qqqq);}
//#define GL_ERROR_CHECK(str) printf("%s: GL error %x\n", str, glGetError())
#endif

typedef struct _gstate
{
    int separator;
    double transform[16];
    double color[4];
    double linewidth;
    int id;
    double id_depth;
} gstate;

typedef struct _drstate
{
    PyObject* camera;
    PyObject* visible;
    double viewport_aspect;
    double global_alpha;
    int auto_mark;
    PyObject* context_name;
} drstate;

#endif

