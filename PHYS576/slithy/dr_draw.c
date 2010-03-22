#include "sl_common.h"
#include "dr_draw.h"

void set_id_depth( int id );
static double compute_id_depth( int id );

/* from dr_draw.h, but static, so only useful here. */
static FUNC_PY( draw_set_camera );
static FUNC_PY_KEYWD( draw_embed_object );
static FUNC_PY_KEYWD( draw_embed );

static FUNC_PY( set_hooks );
static FUNC_PY_NOARGS( draw_camera );
static FUNC_PY_NOARGS( draw_visible );

static FUNC_PY( draw_clear );
static FUNC_PY( draw_clear_alpha );

static FUNC_PY( draw_line );
static FUNC_PY( draw_circle );
static FUNC_PY( draw_dot );
static FUNC_PY( draw_polygon );
static FUNC_PY_KEYWD( draw_rectangle );
static FUNC_PY( draw_frame );

static FUNC_PY( draw_color );
static FUNC_PY( draw_id );
static FUNC_PY( draw_thickness );

static FUNC_PY( draw_clip );
static FUNC_PY( draw_unclip );

static FUNC_PY( read_id );
/* end from dr_draw.h */

#define CIRCLE_STEPS 60

static FUNC_PY( pre_drawing );
static FUNC_PY( post_drawing );
static FUNC_PY( do_gldrawing );
static FUNC_PY_NOARGS( frame );

static PyMethodDef DrawMethods[] = {
    { "set_camera", draw_set_camera, METH_VARARGS },
    { "embed_object", (PyCFunction)draw_embed_object, METH_VARARGS | METH_KEYWORDS },
    { "_embed", (PyCFunction)draw_embed, METH_VARARGS },
    
    { "clear", draw_clear, METH_VARARGS },
    //{ "clear_alpha", draw_clear_alpha, METH_VARARGS },
    
    { "line", draw_line, METH_VARARGS },
    { "circle", draw_circle, METH_VARARGS },
    { "frame", draw_frame, METH_VARARGS },

    { "polygon", draw_polygon, METH_VARARGS },
    { "dot", draw_dot, METH_VARARGS },
    { "rectangle", (PyCFunction)draw_rectangle, METH_VARARGS | METH_KEYWORDS },
    
    { "push", (PyCFunction)draw_push, METH_NOARGS },
    { "pop", (PyCFunction)draw_pop, METH_NOARGS },
    
    { "translate", draw_translate, METH_VARARGS },
    { "rotate", draw_rotate, METH_VARARGS },
    { "shear", draw_shear, METH_VARARGS },
    { "scale", draw_scale, METH_VARARGS },

    { "color", draw_color, METH_VARARGS },
    { "id", draw_id, METH_VARARGS },
    { "thickness", draw_thickness, METH_VARARGS },

    { "widestroke", draw_path_wide_stroke, METH_VARARGS },
    { "stroke", draw_path_stroke, METH_VARARGS },
    { "arrow", draw_path_arrow, METH_VARARGS },
    { "fill", draw_path_fill, METH_VARARGS },
    
    { "grid", draw_grid, METH_VARARGS },

    { "bbox", compute_path_bbox, METH_VARARGS },

    { "text", (PyCFunction)draw_diatext, METH_VARARGS|METH_KEYWORDS },

    { "image", (PyCFunction)draw_image, METH_VARARGS|METH_KEYWORDS },

    { "clip", (PyCFunction)draw_clip, METH_VARARGS },
    { "unclip", (PyCFunction)draw_unclip, METH_VARARGS },

    { "_set_hooks", set_hooks, METH_VARARGS },
    { "camera", (PyCFunction)draw_camera, METH_NOARGS },
    { "visible", (PyCFunction)draw_visible, METH_NOARGS },
    { "unproject", draw_unproject, METH_VARARGS },
    { "project", draw_project, METH_VARARGS },

    { "_frame", (PyCFunction)frame, METH_NOARGS },
    { "_pre_drawing", pre_drawing, METH_VARARGS },
    { "_post_drawing", post_drawing, METH_VARARGS },
    { "_do_gldrawing", do_gldrawing, METH_VARARGS },

    { "_save_marked_cameras", save_marked_cameras, METH_VARARGS },
    { "mark", mark, METH_VARARGS },
    { "_unproject", unproject, METH_VARARGS },

    { "_read_id", (PyCFunction)read_id, METH_VARARGS },
    { NULL, NULL }
};

PyObject* color_class = NULL;
PyObject* font_class = NULL;

PyObject* DrawError;

PyObject* current_camera = NULL;
PyObject* current_visible = NULL;
double viewport_aspect;
static void compute_current_visible( void );

PyObject* global_draw_object_fn = NULL;
PyObject* complete_parameter_dict_fn = NULL;

double line_thickness = 1.0;
double global_alpha = 1.0;
int current_id = 0;
double current_id_depth = 0.0;
int auto_mark;
PyObject* context_name = NULL;

GLint viewport[4];
GLint depth_bits;

void
#ifdef WIN32
__declspec( dllexport )
#endif
initdraw( void )
{
    PyObject* m;
    PyObject* d;

    m = Py_InitModule( "draw", DrawMethods );
    d = PyModule_GetDict( m );
    DrawError = PyErr_NewException( "draw.DrawError", NULL, NULL );
    PyDict_SetItemString( d, "DrawError", DrawError );

    PyModule_AddIntConstant( m, "RESET", TEXT_RESET );
    PyModule_AddIntConstant( m, "RESETFONT", TEXT_RESETFONT );
    PyModule_AddIntConstant( m, "RESETCOLOR", TEXT_RESETCOLOR );
    PyModule_AddIntConstant( m, "BOTH", IMAGE_BOTH );
    PyModule_AddIntConstant( m, "WIDTH", IMAGE_WIDTH );
    PyModule_AddIntConstant( m, "HEIGHT", IMAGE_HEIGHT );
    PyModule_AddIntConstant( m, "STRETCH", IMAGE_STRETCH );
    
}

static int install_camera( PyObject* camera, double viewport_aspect )
{
    double ox = 0.0;
    double oy = 0.0;
    double ulen = 1.0;
    double utheta = 0.0;
    double aspect = 1.0;
    
    GL_ERROR_CHECK( "install camera" );

    glMatrixMode( GL_PROJECTION );
    glPopMatrix();
    glPushMatrix();
    
    if ( PyTuple_Check( camera ) && PyTuple_Size( camera ) == 5 )
    {
	ox = PyFloat_AsDouble( PyTuple_GetItem( camera, 0 ) );
	oy = PyFloat_AsDouble( PyTuple_GetItem( camera, 1 ) );
	ulen = PyFloat_AsDouble( PyTuple_GetItem( camera, 2 ) );
	utheta = PyFloat_AsDouble( PyTuple_GetItem( camera, 3 ) );
	aspect = PyFloat_AsDouble( PyTuple_GetItem( camera, 4 ) );
    }
    else if ( PyList_Check( camera ) && PyList_Size( camera ) == 5 ) 
    {
	ox = PyFloat_AsDouble( PyList_GetItem( camera, 0 ) );
	oy = PyFloat_AsDouble( PyList_GetItem( camera, 1 ) );
	ulen = PyFloat_AsDouble( PyList_GetItem( camera, 2 ) );
	utheta = PyFloat_AsDouble( PyList_GetItem( camera, 3 ) );
	aspect = PyFloat_AsDouble( PyList_GetItem( camera, 4 ) );
    }
    else
    {
	PyErr_SetString( PyExc_ValueError, "bad camera for install_camera" );
	return -1;
    }

    //printf( "installing camera (ulen %.3f)\n", ulen );
    
    // (ox, oy) is lower left corner
    // (ux, uy) is vector to lower right corner
    // aspect is viewbox width/height

    if ( aspect > viewport_aspect )
    {
	// fit viewbox in the width of the viewport

	glTranslated( 0.0, 1.0 - (viewport_aspect / aspect), 0.0 );

	glTranslated( -viewport_aspect, -1.0, 0.0 );
	glScaled( viewport_aspect * 2.0 / ulen, viewport_aspect * 2.0 / ulen, 1.0 );
	glRotated( -utheta, 0.0, 0.0, 1.0 );
	glTranslated( -ox, -oy, 0.0 );
    }
    else
    {
	// fit viewbox in the height of the viewport

	glTranslated( viewport_aspect - aspect, 0.0, 0.0 );
	
	glTranslated( -viewport_aspect, -1.0, 0.0 );
	glScaled( aspect * 2.0 / ulen, aspect * 2.0 / ulen, 1.0 );
	glRotated( -utheta, 0.0, 0.0, 1.0 );
	glTranslated( -ox, -oy, 0.0 );
    }

    glMatrixMode( GL_MODELVIEW );

    current_IPM_dirty = 1;
    
    return 0;
}
    
static FUNC_PY_NOARGS( frame )
{
    glGetIntegerv( GL_VIEWPORT, viewport );
    
    Py_INCREF( Py_None );
    return Py_None;
}

static FUNC_PY( pre_drawing )
{
#if 0
    int depth, max;
    glGetIntegerv( GL_PROJECTION_STACK_DEPTH, &depth );
    glGetIntegerv( GL_MAX_PROJECTION_STACK_DEPTH, &max );
    printf( "pre_drawing: projection stack depth %d / %d\n", depth, max );
#endif

    double new_viewport_aspect, new_alpha;
    PyObject* new_context_name;
    
    GL_ERROR_CHECK( "top of pre_drawing" );
    
    if ( !PyArg_ParseTuple( args, "ddO!", &new_viewport_aspect, &new_alpha, &PyString_Type, &new_context_name ) )
	return NULL;
    
    initialize_stack();

    context_name = new_context_name;
    Py_INCREF( context_name );
    auto_mark = 0;
    global_alpha *= new_alpha;
    viewport_aspect = new_viewport_aspect;
    current_camera = Py_BuildValue( "(ddddd)", -viewport_aspect, -1.0, viewport_aspect * 2, 0.0, viewport_aspect );
#if 0
    PyObject_Print( current_camera, stdout, 0 );
    printf( "  %f\n", viewport_aspect );
#endif
    current_visible = NULL;

    glColor4d( 0.0, 0.0, 0.0, global_alpha );
    glLineWidth( 1.0 );

    glGetIntegerv( GL_DEPTH_BITS, &depth_bits );

    glMatrixMode( GL_MODELVIEW );
    glPushMatrix();
    set_id_depth( -1 );

    install_camera( current_camera, viewport_aspect );
    
    GL_ERROR_CHECK( "bottom of pre_drawing" );
    
    Py_INCREF( Py_None );
    return Py_None;
}

static FUNC_PY( post_drawing )
{
    GL_ERROR_CHECK( "top of post_drawing" );
    
    glPopMatrix();

#if 0
    {
	GLfloat d;
	glReadPixels( 400, 300, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &d );
	printf( "-- post_drawing %.16f\n", d );
    }
#endif
    
    if ( auto_mark == 0 )
    {
	PyObject* temp;
	temp = internal_mark( context_name );
	Py_DECREF( temp );
	auto_mark = 1;
    }
    
    Py_DECREF( current_camera );
    Py_XDECREF( current_visible );
    Py_DECREF( context_name );

    clear_stack();
    
#if 0
    {
	GLfloat d;
	glReadPixels( 400, 300, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &d );
	printf( "-- bottom of post_drawing %.16f\n", d );
    }
#endif
    GL_ERROR_CHECK( "bottom of post_drawing" );
    Py_INCREF( Py_None );
    return Py_None;
}

static void show_matrix( GLenum which )
{
    double m[16];
    
    if ( which == GL_PROJECTION_MATRIX )
	printf( "projection:\n" );
    else
	printf( "model-view:\n" );

    glGetDoublev( which, m );
    printf( "[ %9.4f  %9.4f  %9.4f  %9.4f ]\n", m[0], m[4], m[8], m[12] );
    printf( "[ %9.4f  %9.4f  %9.4f  %9.4f ]\n", m[1], m[5], m[9], m[13] );
    printf( "[ %9.4f  %9.4f  %9.4f  %9.4f ]\n", m[2], m[6], m[10], m[14] );
    printf( "[ %9.4f  %9.4f  %9.4f  %9.4f ]\n", m[3], m[7], m[11], m[15] );
}



static FUNC_PY( do_gldrawing )
{
    double aspect, alpha;
    PyObject* callback;
    PyObject* pdict;
    double proj_camera[16];
    double proj_newcamera[16];
    PyObject* result;

    GL_ERROR_CHECK( "top of pre_gldrawing" );
    
    if ( !PyArg_ParseTuple( args, "OOdd", &callback, &pdict, &aspect, &alpha ) )
	return NULL;
    
    glColor4d( 0.0, 0.0, 0.0, 1.0 );
    glLineWidth( 1.0 );

    glMatrixMode( GL_PROJECTION );
    glGetDoublev( GL_PROJECTION_MATRIX, proj_camera );
    memcpy( proj_newcamera, proj_camera, 16 * sizeof( double ) );
    proj_newcamera[10] = 1.0;
    proj_newcamera[14] = 0.0;
    glLoadMatrixd( proj_newcamera );
    
    glMatrixMode( GL_MODELVIEW );
    glPushMatrix();
    glLoadIdentity();
    
    glPushAttrib( GL_ALL_ATTRIB_BITS );

    glDisable( GL_DEPTH_TEST );
    glDepthFunc( GL_LESS );
    glDepthMask( GL_TRUE );

    GL_ERROR_CHECK( "bottom of pre_gldrawing" );
    result = PyObject_CallFunction( callback, "Odd", pdict, aspect, global_alpha * alpha );
    GL_ERROR_CHECK( "top of post_gldrawing" );

    glPopAttrib();
    
    glMatrixMode( GL_PROJECTION );
    glLoadMatrixd( proj_camera );

    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();
    set_id_depth( 0 );
    
    glEnable( GL_DEPTH_TEST );
    glDepthFunc( GL_ALWAYS );
    glColorMask( GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE );
    glBegin( GL_TRIANGLE_STRIP );
    glVertex2d( -aspect, -1.0 );
    glVertex2d(  aspect, -1.0 );
    glVertex2d( -aspect,  1.0 );
    glVertex2d(  aspect,  1.0 );
    glEnd();
    glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE );

    glPopMatrix();
    
    GL_ERROR_CHECK( "bottom of post_gldrawing" );
    
    return result;
}

static FUNC_PY( draw_id )
{
    int id;

    if ( !PyArg_ParseTuple( args, "i", &id ) )
	return NULL;

    set_id_depth( id );

    Py_INCREF( Py_None );
    return Py_None;
}

static FUNC_PY( draw_set_camera )
{
    PyObject* new_camera;

    GL_ERROR_CHECK( "top of set_camera" );
    
    if ( !PyArg_ParseTuple( args, "O", &new_camera ) )
	return NULL;

    Py_DECREF( current_camera );
    Py_XDECREF( current_visible );
    current_visible = NULL;
    current_camera = new_camera;
    Py_INCREF( current_camera );
    if ( install_camera( current_camera, viewport_aspect ) )
	return NULL;

    if ( auto_mark == 0 )
    {
	PyObject* temp;
	temp = internal_mark( context_name );
	Py_DECREF( temp );
	auto_mark = 1;
    }
    
    GL_ERROR_CHECK( "bottom of set_camera" );
    Py_INCREF( Py_None );
    return Py_None;
}    

static FUNC_PY_NOARGS( draw_camera )
{
    Py_INCREF( current_camera );
    return current_camera;
}

static void compute_current_visible( void )
{
    double ox, oy, ulen, utheta, aspect;
    double d, th;

    ox = PyFloat_AsDouble( PyTuple_GetItem( current_camera, 0 ) );
    oy = PyFloat_AsDouble( PyTuple_GetItem( current_camera, 1 ) );
    ulen = PyFloat_AsDouble( PyTuple_GetItem( current_camera, 2 ) );
    utheta = PyFloat_AsDouble( PyTuple_GetItem( current_camera, 3 ) );
    aspect = PyFloat_AsDouble( PyTuple_GetItem( current_camera, 4 ) );
    
    if ( aspect > viewport_aspect )
    {
	th = (utheta-90) * M_PI / 180.0;
	d = ulen * (1.0 - viewport_aspect / aspect) / (2 * viewport_aspect);
	current_visible = Py_BuildValue( "(ddddd)", ox + d * cos(th), oy + d * sin(th),
					 ulen, utheta, viewport_aspect );
    }
    else
    {
	th = (utheta+180) * M_PI / 180.0;
	d = ulen * (viewport_aspect - aspect) / (2 * aspect);
	current_visible = Py_BuildValue( "(ddddd)", ox + d * cos(th), oy + d * sin(th),
					 ulen * viewport_aspect / aspect, utheta, viewport_aspect );
    }
}

static FUNC_PY_NOARGS( draw_visible )
{
    if ( current_visible == NULL )
	compute_current_visible();
    
    Py_INCREF( current_visible );
    return current_visible;
}

static FUNC_PY_KEYWD( draw_embed_object )
{
    PyObject* obj;
    PyObject* pdict;
    PyObject* pdict_complete;
    PyObject* temp;
    PyObject* result;
    PyObject* recttuple;
    GLdouble proj_camera[16];
    GLdouble proj_base[16];
    GLdouble mv[16];
    double ox, oy, ulen, theta, aspect;
    double t;
    double alpha = 1.0;
    int clip = 1;
    GLboolean already_enabled;
    GLint ref;
    static char *kwlist[] = { "rect", "object", "params", "clip", "_alpha", NULL };

    GL_ERROR_CHECK( "top of embed" );
	
    if ( !PyArg_ParseTupleAndKeywords( args, keywds, "OOO|id", kwlist, &recttuple, &obj, &pdict, &clip, &alpha ) )
	return NULL;
    if ( !PyTuple_Check( recttuple ) || (PyTuple_Size( recttuple ) != 5 && PyTuple_Size( recttuple ) != 4) )
    {
	PyErr_SetString( PyExc_TypeError, "bad rectangle specification" );
	return NULL;
    }
    if ( PyTuple_Size( recttuple ) == 5 )
    {
	if ( !PyArg_ParseTuple( recttuple, "ddddd", &ox, &oy, &ulen, &theta, &aspect ) )
	    return NULL;
    }
    else
    {
	double x1, x2, y1, y2;
	if ( !PyArg_ParseTuple( recttuple, "dddd", &x1, &y1, &x2, &y2 ) )
	    return NULL;
	ox = x1;
	oy = y1;
	ulen = x2 - x1;
	theta = 0.0;
	aspect = (x2-x1) / (y2-y1);
    }
    t = ulen / (aspect * 2.0);

    pdict_complete = PyObject_CallFunction( complete_parameter_dict_fn, "OO", obj, pdict );
    if ( pdict_complete == NULL )
	return NULL;
    
    // save state
    temp = draw_push( Py_None );
    Py_DECREF( temp );
    glMatrixMode( GL_MODELVIEW );
    glPopMatrix();        // lose the depth matrix
    glMatrixMode( GL_PROJECTION );
    glGetDoublev( GL_PROJECTION_MATRIX, proj_camera );
    glPopMatrix();
    glGetDoublev( GL_PROJECTION_MATRIX, proj_base );

    // squish the base, camera, and modelview matrices together into the new base.
    glGetDoublev( GL_MODELVIEW_MATRIX, mv );
    glLoadMatrixd( proj_camera );
    glMultMatrixd( mv );
    if ( theta == 0.0 )
    {
	glTranslated( ox + ulen / 2.0, oy + t, 0.0 );
	glScaled( t, t, 1.0 );
    }
    else
    {
	glTranslated( ox, oy, 0.0 );
	glRotated( theta, 0, 0, 1 );
	glTranslated( ulen / 2.0, t, 0.0 );
	glScaled( t, t, 1.0 );
    }
    glPushMatrix();
    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();

    if ( clip )
    {
	glDepthMask( GL_FALSE );
	glGetBooleanv( GL_STENCIL_TEST, &already_enabled );
	if ( already_enabled )
	{
	    glGetIntegerv( GL_STENCIL_REF, &ref );
	    ++ref;
	}
	else
	{
	    glEnable( GL_STENCIL_TEST );
	    ref = 1;
	}

	glStencilFunc( GL_ALWAYS, 1, 1 );
	glStencilOp( GL_INCR, GL_INCR, GL_INCR );
	glColorMask( GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE );
	glBegin( GL_TRIANGLE_STRIP );
	glVertex2d( -aspect, -1.0 );
	glVertex2d(  aspect, -1.0 );
	glVertex2d( -aspect, 1.0 );
	glVertex2d(  aspect, 1.0 );
	glEnd();
	glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE );
	glStencilOp( GL_KEEP, GL_KEEP, GL_KEEP );
	glStencilFunc( GL_EQUAL, ref, 0xffff );
	if ( current_id >= 0 )
	    glDepthMask( GL_TRUE );
    }
    
    GL_ERROR_CHECK( "before embed callback" );
    result = PyObject_CallFunction( global_draw_object_fn, "OOdd", obj, pdict_complete, aspect, alpha );
    GL_ERROR_CHECK( "after embed callback" );
    Py_DECREF( pdict_complete );

    GL_ERROR_CHECK( "about to unclip" );
    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();
    glMatrixMode( GL_PROJECTION );
    glPopMatrix();
    
    if ( clip )
    {
	glDepthMask( GL_FALSE );
	glStencilFunc( GL_ALWAYS, 1, 1 );
	glStencilOp( GL_DECR, GL_DECR, GL_DECR );
	glColorMask( GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE );
	glBegin( GL_TRIANGLE_STRIP );
	glVertex2d( -aspect, -1.0 );
	glVertex2d(  aspect, -1.0 );
	glVertex2d( -aspect, 1.0 );
	glVertex2d(  aspect, 1.0 );
	glEnd();
	glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE );
	glStencilOp( GL_KEEP, GL_KEEP, GL_KEEP );

	if ( already_enabled )
	    glStencilFunc( GL_EQUAL, ref-1, 0xffff );
	else
	    glDisable( GL_STENCIL_TEST );
	if ( current_id >= 0 )
	    glDepthMask( GL_TRUE );
    }
    GL_ERROR_CHECK( "about to restore state" );
    
    // restore state
    glLoadMatrixd( proj_base );
    glPushMatrix();
    glLoadMatrixd( proj_camera );
    glMatrixMode( GL_MODELVIEW );
    glPushMatrix();
    temp = draw_pop( Py_None );
    Py_DECREF( temp );
    current_IPM_dirty = 1;
    
    GL_ERROR_CHECK( "end of embed" );
    return result;
}

static FUNC_PY_KEYWD( draw_embed )
{
    PyObject* obj;
    PyObject* objargs;
    PyObject* result;
    PyObject* temp;
    GLdouble proj_camera[16];
    GLdouble proj_base[16];
    GLdouble mv[16];
    double ox, oy, ulen, theta, aspect;
    double t;
    int clip = 1;
    GLboolean already_enabled;
    GLint ref;

    GL_ERROR_CHECK( "top of _embed" );
    
    if ( !PyArg_ParseTuple( args, "(ddddd)iOO", &ox, &oy, &ulen, &theta, &aspect,
			    &clip, &obj, &objargs ) )
	return NULL;
    
    t = ulen / (aspect * 2.0);

    // save state
    temp = draw_push( Py_None );
    Py_DECREF( temp );
    glMatrixMode( GL_MODELVIEW );
    glPopMatrix();        // lose the depth matrix
    glMatrixMode( GL_PROJECTION );
    glGetDoublev( GL_PROJECTION_MATRIX, proj_camera );
    glPopMatrix();
    glGetDoublev( GL_PROJECTION_MATRIX, proj_base );

    // squish the base, camera, and modelview matrices together into the new base.
    glGetDoublev( GL_MODELVIEW_MATRIX, mv );
    glLoadMatrixd( proj_camera );
    glMultMatrixd( mv );
    if ( theta == 0.0 )
    {
	glTranslated( ox + ulen / 2.0, oy + t, 0.0 );
	glScaled( t, t, 1.0 );
    }
    else
    {
	glTranslated( ox, oy, 0.0 );
	glRotated( theta, 0, 0, 1 );
	glTranslated( ulen / 2.0, t, 0.0 );
	glScaled( t, t, 1.0 );
    }
    glPushMatrix();
    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();

    if ( clip )
    {
	glGetBooleanv( GL_STENCIL_TEST, &already_enabled );
	if ( already_enabled )
	{
	    glGetIntegerv( GL_STENCIL_REF, &ref );
	    ++ref;
	}
	else
	{
	    glEnable( GL_STENCIL_TEST );
	    ref = 1;
	}

	glDepthMask( GL_FALSE );
	glStencilFunc( GL_ALWAYS, 1, 1 );
	glStencilOp( GL_INCR, GL_INCR, GL_INCR );
	glColorMask( GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE );
	glBegin( GL_TRIANGLE_STRIP );
	glVertex2d( -aspect, -1.0 );
	glVertex2d(  aspect, -1.0 );
	glVertex2d( -aspect, 1.0 );
	glVertex2d(  aspect, 1.0 );
	glEnd();
	glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE );
	glStencilOp( GL_KEEP, GL_KEEP, GL_KEEP );
	glStencilFunc( GL_EQUAL, ref, 0xffff );
	if ( current_id >= 0 )
	    glDepthMask( GL_TRUE );
    }

    result = PyObject_CallObject( obj, objargs );

    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();
    glMatrixMode( GL_PROJECTION );
    glPopMatrix();
    
    if ( clip )
    {
	glDepthMask( GL_FALSE );
	glStencilFunc( GL_ALWAYS, 1, 1 );
	glStencilOp( GL_DECR, GL_DECR, GL_DECR );
	glColorMask( GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE );
	glBegin( GL_TRIANGLE_STRIP );
	glVertex2d( -aspect, -1.0 );
	glVertex2d(  aspect, -1.0 );
	glVertex2d( -aspect, 1.0 );
	glVertex2d(  aspect, 1.0 );
	glEnd();
	glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE );
	glStencilOp( GL_KEEP, GL_KEEP, GL_KEEP );

	if ( already_enabled )
	    glStencilFunc( GL_EQUAL, ref-1, 0xffff );
	else
	    glDisable( GL_STENCIL_TEST );
	
	if ( current_id >= 0 )
	    glDepthMask( GL_TRUE );
    }
    
    // restore state
    glLoadMatrixd( proj_base );
    glPushMatrix();
    glLoadMatrixd( proj_camera );
    glMatrixMode( GL_MODELVIEW );
    glPushMatrix();
    temp = draw_pop( Py_None );
    Py_DECREF( temp );
    current_IPM_dirty = 1;

#if 0
    {
	GLfloat d;
	glReadPixels( 400, 300, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &d );
	printf( "-- end of embed %f\n", d );
    }
#endif

    GL_ERROR_CHECK( "bottom of _embed" );
    
    
    return result;
}

static FUNC_PY( set_hooks )
{
    if ( !PyArg_ParseTuple( args, "OOO", &global_draw_object_fn,
			    &complete_parameter_dict_fn, &color_class ) )
	return NULL;

    Py_INCREF( global_draw_object_fn );
    Py_INCREF( complete_parameter_dict_fn );
    Py_INCREF( color_class );

    Py_INCREF( Py_None );
    return Py_None;
}


static FUNC_PY( draw_line )
{
    int i;
    double tx, ty;
    double sx, sy;
    int n = PyTuple_Size( args );
    double q[3][4];

    if ( n & 1 )
    {
	PyErr_SetString( PyExc_ValueError, "call to line has odd number of coordinates" );
	return NULL;
    }
    n /= 2;
    
    if ( n < 2 )
	goto done;

    sx = PyFloat_AsDouble( PyTuple_GetItem( args, 0 ) );
    sy = PyFloat_AsDouble( PyTuple_GetItem( args, 1 ) );
    
    for ( i = 1; i < n; ++i )
    {
	tx = PyFloat_AsDouble( PyTuple_GetItem( args, i*2+0 ) );
	ty = PyFloat_AsDouble( PyTuple_GetItem( args, i*2+1 ) );

	wide_line_segment( line_thickness, sx, sy, tx, ty, q[0], q[1] );

	sx = tx;
	sy = ty;
    }

 done:
    Py_INCREF( Py_None );
    return Py_None;
}

static FUNC_PY_KEYWD( draw_rectangle )
{
    double x1, y1, x2, y2;
    double ox, oy, ulen, theta, aspect;

#if 0
    PyObject_Print( args, stdout, 0 );
    printf( "   %08x\n", args );
    PyObject_Print( keywds, stdout, 0 );
    printf( "   %08x\n", keywds );
#endif

    if ( keywds == NULL || PyDict_Size( keywds ) == 0 )
    {
	// the common case, with no corner colors
	 
	switch( PyTuple_Size( args ) )
	{
	  case 4:
	    if ( !PyArg_ParseTuple( args, "dddd", &x1, &y1, &x2, &y2 ) )
		return NULL;
	    
	    glBegin( GL_TRIANGLE_STRIP );
	    glVertex2d( x1, y1 );
	    glVertex2d( x2, y1 );
	    glVertex2d( x1, y2 );
	    glVertex2d( x2, y2 );
	    glEnd();
	    
	    break;
	    
	  case 1:
	    if ( !PyArg_ParseTuple( args, "(ddddd)", &ox, &oy, &ulen, &theta, &aspect ) )
		return NULL;
	    
	    glPushMatrix();
	    glTranslated( ox, oy, 0.0 );
	    glRotated( theta, 0, 0, 1 );
	    
	    glBegin( GL_TRIANGLE_STRIP );
	    glVertex2d( 0, 0 );
	    glVertex2d( ulen, 0 );
	    glVertex2d( 0, ulen / aspect );
	    glVertex2d( ulen, ulen / aspect );
	    glEnd();
	    
	    glPopMatrix();
	    
	    break;
	    
	  default:
	    PyErr_SetString( PyExc_ValueError, "bad argument to rectangle" );
	    return NULL;
	}
    }
    else
    {
	double start_color[4];
	double* temp;
	double swrgba[4];
	double sergba[4];
	double nwrgba[4];
	double nergba[4];
	
	PyObject* cobj;
	
	// corner-colors given, maybe

	glGetDoublev( GL_CURRENT_COLOR, start_color );

#define getcolor(dir) { \
           cobj = PyDict_GetItemString( keywds, #dir ); \
	   if ( cobj ) { \
              if ( !( temp = get_color_from_object( cobj ) ) ) { \
                 PyErr_SetString( PyExc_ValueError, "bad color object for '" #dir "' corner" ); \
 	         return NULL; \
              } \
           } else temp = start_color; \
           memcpy( dir##rgba, temp, 4 * sizeof(double) ); \
        }

	getcolor(sw);
	getcolor(se);
	getcolor(nw);
	getcolor(ne);
#undef getcolor

#if 0
#define test(dir) printf(#dir ": %.3f %.3f %.3f  %.3f\n", dir##rgba[0], dir##rgba[1], dir##rgba[2], dir##rgba[3] );
	test(sw);
	test(se);
	test(nw);
	test(ne);
#undef test
#endif

	swrgba[3] *= global_alpha;
	sergba[3] *= global_alpha;
	nwrgba[3] *= global_alpha;
	nergba[3] *= global_alpha;
	
	switch( PyTuple_Size( args ) )
	{
	  case 4:
	    if ( !PyArg_ParseTuple( args, "dddd", &x1, &y1, &x2, &y2 ) )
		return NULL;
	    
	    glBegin( GL_TRIANGLE_STRIP );
	    glColor4dv( swrgba );
	    glVertex2d( x1, y1 );
	    glColor4dv( sergba );
	    glVertex2d( x2, y1 );
	    glColor4dv( nwrgba );
	    glVertex2d( x1, y2 );
	    glColor4dv( nergba );
	    glVertex2d( x2, y2 );
	    glEnd();
	    
	    break;
	    
	  case 1:
	    if ( !PyArg_ParseTuple( args, "(ddddd)", &ox, &oy, &ulen, &theta, &aspect ) )
		return NULL;
	    
	    glPushMatrix();
	    glTranslated( ox, oy, 0.0 );
	    glRotated( theta, 0, 0, 1 );
	    
	    glBegin( GL_TRIANGLE_STRIP );
	    glColor4dv( swrgba );
	    glVertex2d( 0, 0 );
	    glColor4dv( sergba );
	    glVertex2d( ulen, 0 );
	    glColor4dv( nwrgba );
	    glVertex2d( 0, ulen / aspect );
	    glColor4dv( nergba );
	    glVertex2d( ulen, ulen / aspect );
	    glEnd();
	    
	    glPopMatrix();
	    
	    break;
	    
	  default:
	    PyErr_SetString( PyExc_ValueError, "bad argument to rectangle" );
	    return NULL;
	}

	glColor4dv( start_color );
    }

    Py_INCREF( Py_None );
    return Py_None;
}

static FUNC_PY( draw_frame )
{
    double x1, y1, x2, y2;
    double ox, oy, ulen, theta, aspect;
    double t;

    switch( PyTuple_Size( args ) )
    {
      case 4:
	if ( !PyArg_ParseTuple( args, "dddd", &x1, &y1, &x2, &y2 ) )
	    return NULL;

	if ( x1 > x2 ) {t = x1; x1 = x2; x2 = t;}
	if ( y1 > y2 ) {t = y1; y1 = y2; y2 = t;}
	
	glBegin( GL_TRIANGLE_STRIP );
	glVertex2d( x1 - line_thickness / 2, y1 - line_thickness / 2 );
	glVertex2d( x1 + line_thickness / 2, y1 + line_thickness / 2 );
	
	glVertex2d( x2 + line_thickness / 2, y1 - line_thickness / 2 );
	glVertex2d( x2 - line_thickness / 2, y1 + line_thickness / 2 );
	
	glVertex2d( x2 + line_thickness / 2, y2 + line_thickness / 2 );
	glVertex2d( x2 - line_thickness / 2, y2 - line_thickness / 2 );
	
	glVertex2d( x1 - line_thickness / 2, y2 + line_thickness / 2 );
	glVertex2d( x1 + line_thickness / 2, y2 - line_thickness / 2 );
	
	glVertex2d( x1 - line_thickness / 2, y1 - line_thickness / 2 );
	glVertex2d( x1 + line_thickness / 2, y1 + line_thickness / 2 );
	glEnd();

	break;

      case 1:
	if ( !PyArg_ParseTuple( args, "(ddddd)", &ox, &oy, &ulen, &theta, &aspect ) )
	    return NULL;

	glPushMatrix();
	glTranslated( ox, oy, 0.0 );
	glRotated( theta, 0, 0, 1 );
	
	glBegin( GL_TRIANGLE_STRIP );
	glVertex2d( -line_thickness / 2, -line_thickness / 2 );
	glVertex2d(  line_thickness / 2,  line_thickness / 2 );
	
	glVertex2d( ulen + line_thickness / 2, -line_thickness / 2 );
	glVertex2d( ulen - line_thickness / 2,  line_thickness / 2 );
	
	glVertex2d( ulen + line_thickness / 2, ulen / aspect + line_thickness / 2 );
	glVertex2d( ulen - line_thickness / 2, ulen / aspect - line_thickness / 2 );
	
	glVertex2d( -line_thickness / 2, ulen / aspect + line_thickness / 2 );
	glVertex2d( +line_thickness / 2, ulen / aspect - line_thickness / 2 );
	
	glVertex2d( -line_thickness / 2, -line_thickness / 2 );
	glVertex2d(  line_thickness / 2,  line_thickness / 2 );
	glEnd();

	glPopMatrix();

	break;

      default:
	PyErr_SetString( PyExc_ValueError, "bad argument to frame" );
	return NULL;
    }
	
	
    Py_INCREF( Py_None );
    return Py_None;
}

static FUNC_PY( draw_circle )
{
    double r, x = 0.0, y = 0.0;
    int i;
    static double cs[CIRCLE_STEPS * 2];
    static int cs_full = 0;

    if ( !PyArg_ParseTuple( args, "d|dd", &r, &x, &y ) )
	return NULL;

    if ( !cs_full )
    {
	for ( i = 0; i < CIRCLE_STEPS; ++i )
	{
	    cs[i*2+0] = cos( i * 2 * M_PI / CIRCLE_STEPS );
	    cs[i*2+1] = sin( i * 2 * M_PI / CIRCLE_STEPS );
	}
	cs_full = 1;
    }
    
    glPushMatrix();
    glTranslated( x, y, 0 );

    glBegin( GL_TRIANGLE_STRIP );
    for ( i = 0; i < CIRCLE_STEPS; ++i )
    {
	glVertex2d( (r + line_thickness / 2) * cs[i*2+0],
		    (r + line_thickness / 2) * cs[i*2+1] );
	glVertex2d( (r - line_thickness / 2) * cs[i*2+0],
		    (r - line_thickness / 2) * cs[i*2+1] );
    }
    glVertex2d( r + line_thickness / 2, 0 );
    glVertex2d( r - line_thickness / 2, 0 );
    glEnd();

    glPopMatrix();
			  
    Py_INCREF( Py_None );
    return Py_None;
}

static FUNC_PY( draw_dot )
{
    double r, x = 0.0, y = 0.0;
    int i;
    static int displaylist = -1;

    if ( !PyArg_ParseTuple( args, "d|dd", &r, &x, &y ) )
	return NULL;

    glPushMatrix();
    glTranslated( x, y, 0 );
    glScaled( r, r, 1 );
	
    if ( displaylist == -1 )
    {
	displaylist = glGenLists( 1 );

	glNewList( displaylist, GL_COMPILE_AND_EXECUTE );
	
	glBegin( GL_TRIANGLE_FAN );
	for ( i = 0; i < CIRCLE_STEPS; ++i )
	    glVertex2d( cos( i * 2 * M_PI / CIRCLE_STEPS ),
			sin( i * 2 * M_PI / CIRCLE_STEPS ) );
	glVertex2d( 1.0, 0.0 );
	glEnd();

	glEndList();
    }
    else
	glCallList( displaylist );

    glPopMatrix();

    Py_INCREF( Py_None );
    return Py_None;
}

#if 0
void vertex_cb( GLvoid* ptr )
{
    GLdouble* xyz = ptr;

    printf( "vertex %.2f %.2f\n", xyz[0], xyz[1] );
    glVertex2dv( xyz );
}

void begin_cb( GLenum which )
{
    switch( which )
    {
      case GL_TRIANGLES:
	printf( "triangles\n" );
	break;
      case GL_TRIANGLE_STRIP:
	printf( "triangle strip\n" );
	break;
      case GL_TRIANGLE_FAN:
	printf( "triangle fan\n" );
	break;
      default:
	printf( "other\n" );
	break;
    }

    glBegin( which );
}
#endif

void
#ifdef WIN32
CALLBACK
#endif
combine_cb( GLdouble coords[3], GLdouble* vertex_data[4],
		   GLfloat weight[4], GLdouble** dataout )
{
    GLdouble* vertex;

    vertex = (GLdouble*)malloc( 2 * sizeof( GLdouble ) );
    vertex[0] = coords[0];
    vertex[1] = coords[1];
    *dataout = vertex;
}

static FUNC_PY( draw_polygon )
{
    static GLUtesselator* tess = NULL;
    int i, n;
    double* xyz;

    n = PyTuple_Size( args );
    if ( n % 2 != 0 )
    {
	PyErr_SetString( PyExc_ValueError, "polygon() requires an even number of coordinates" );
	return NULL;
    }
    n /= 2;
    xyz = malloc( n * 3 * sizeof( double ) );

    if ( tess == NULL )
    {
	tess = gluNewTess();
    }
    
    gluTessCallback( tess, GLU_TESS_VERTEX, glVertex2dv );
    gluTessCallback( tess, GLU_TESS_BEGIN, glBegin );
    gluTessCallback( tess, GLU_TESS_END, glEnd );
    gluTessCallback( tess, GLU_TESS_COMBINE, combine_cb );
    gluTessProperty( tess, GLU_TESS_BOUNDARY_ONLY, GL_FALSE );

    gluTessBeginPolygon( tess, NULL );
    gluTessBeginContour( tess );
    for ( i = 0; i < n; ++i )
    {
	xyz[i*3+0] = PyFloat_AsDouble( PyTuple_GetItem( args, i*2 ) );
	xyz[i*3+1] = PyFloat_AsDouble( PyTuple_GetItem( args, i*2+1 ) );
	xyz[i*3+2] = 0.0;
	gluTessVertex( tess, xyz + i*3, xyz + i*3 );
    }
    gluTessEndContour( tess );
    gluTessEndPolygon( tess );

    free( xyz );
    
    Py_INCREF( Py_None );
    return Py_None;
}

static FUNC_PY( draw_color )
{
    double* rgba;

    rgba = get_color_from_args( args );
    if ( rgba == NULL )
    {
	PyErr_SetString( DrawError, "bad color specification" );
	return NULL;
    }

    rgba[3] *= global_alpha;
    glColor4dv( rgba );

    GL_ERROR_CHECK( "end of draw_color" );
    
    Py_INCREF( Py_None );
    return Py_None;
}
    
static FUNC_PY( draw_thickness )
{
    if ( !PyArg_ParseTuple( args, "d", &line_thickness ) )
	return NULL;

    Py_INCREF( Py_None );
    return Py_None;
}

static FUNC_PY( draw_clear )
{
    double* rgba;
    double ox, oy, ulen, theta, aspect;

    GL_ERROR_CHECK( "before clear" );

    rgba = get_color_from_args( args );
    rgba[3] *= global_alpha;

    glPushAttrib( GL_CURRENT_BIT );
    glColor4dv( rgba );
    
    if ( current_visible == NULL )
	compute_current_visible();

    ox = PyFloat_AsDouble( PyTuple_GetItem( current_visible, 0 ) );
    oy = PyFloat_AsDouble( PyTuple_GetItem( current_visible, 1 ) );
    ulen = PyFloat_AsDouble( PyTuple_GetItem( current_visible, 2 ) );
    theta = PyFloat_AsDouble( PyTuple_GetItem( current_visible, 3 ) );
    aspect = PyFloat_AsDouble( PyTuple_GetItem( current_visible, 4 ) );
    
    glPushMatrix();
    glTranslated( ox, oy, 0.0 );
    glRotated( theta, 0, 0, 1 );
    
    glBegin( GL_TRIANGLE_STRIP );
    glVertex2d( 0, 0 );
    glVertex2d( ulen, 0 );
    glVertex2d( 0, ulen / aspect );
    glVertex2d( ulen, ulen / aspect );
    glEnd();
    
    glPopMatrix();
    glPopAttrib();
	    
    GL_ERROR_CHECK( "after clear" );

    Py_INCREF( Py_None );
    return Py_None;
}
    
static FUNC_PY( draw_clear_alpha )
{
    double* rgba;

    GL_ERROR_CHECK( "before clear_alpha" );
    
    rgba = get_color_from_args( args );
    rgba[3] *= global_alpha;
    
    glPushAttrib( GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_TRANSFORM_BIT | GL_CURRENT_BIT );
    //glEnable( GL_DEPTH_TEST );
    //glDepthFunc( GL_ALWAYS );
    glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE );
    glColor4dv( rgba );
    glMatrixMode( GL_MODELVIEW );
    glPushMatrix();
    glLoadIdentity();
    glMatrixMode( GL_PROJECTION );
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D( -1.0, 1.0, -1.0, 1.0 );
    glBegin( GL_TRIANGLE_STRIP );
    glVertex3d( -1.0, -1.0, 1.0 );
    glVertex3d(  1.0, -1.0, 1.0 );
    glVertex3d( -1.0,  1.0, 1.0 );
    glVertex3d(  1.0,  1.0, 1.0 );
    glEnd();
    glPopMatrix();
    glMatrixMode( GL_MODELVIEW );
    glPopMatrix();
    glPopAttrib();

    GL_ERROR_CHECK( "after clear_alpha" );

    Py_INCREF( Py_None );
    return Py_None;
}
    
static FUNC_PY( draw_clip )
{
    GLboolean already_enabled;
    GLint ref;
    
    double ox, oy, ulen, theta, aspect;

    if ( !PyArg_ParseTuple( args, "(ddddd)", &ox, &oy, &ulen, &theta, &aspect ) )
	return NULL;
    
    glGetBooleanv( GL_STENCIL_TEST, &already_enabled );
    if ( already_enabled )
    {
	glGetIntegerv( GL_STENCIL_REF, &ref );
	++ref;
    }
    else
    {
	glEnable( GL_STENCIL_TEST );
	ref = 1;
    }
    
    glStencilFunc( GL_ALWAYS, 1, 1 );
    glStencilOp( GL_INCR, GL_INCR, GL_INCR );
    glColorMask( GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE );
    
    glPushMatrix();
    glTranslated( ox, oy, 0.0 );
    glRotated( theta, 0, 0, 1 );
    glBegin( GL_TRIANGLE_STRIP );
    glVertex2d( 0, 0 );
    glVertex2d( ulen, 0 );
    glVertex2d( 0, ulen / aspect );
    glVertex2d( ulen, ulen / aspect );
    glEnd();
    glPopMatrix();
    
    glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE );
    glStencilOp( GL_KEEP, GL_KEEP, GL_KEEP );
    glStencilFunc( GL_EQUAL, ref, 0xffff );

    return Py_BuildValue( "iiO", already_enabled, ref, PyTuple_GetItem( args, 0 ) );
}

static FUNC_PY( draw_unclip )
{
    int already_enabled, ref;
    double ox, oy, ulen, theta, aspect;

    if ( !PyArg_ParseTuple( args, "(ii(ddddd))", &already_enabled, &ref,
			    &ox, &oy, &ulen, &theta, &aspect ) )
	return NULL;

    glStencilFunc( GL_ALWAYS, 1, 1 );
    glStencilOp( GL_DECR, GL_DECR, GL_DECR );
    glColorMask( GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE );
    
    glPushMatrix();
    glTranslated( ox, oy, 0.0 );
    glRotated( theta, 0, 0, 1 );
    glBegin( GL_TRIANGLE_STRIP );
    glVertex2d( 0, 0 );
    glVertex2d( ulen, 0 );
    glVertex2d( 0, ulen / aspect );
    glVertex2d( ulen, ulen / aspect );
    glEnd();
    glPopMatrix();
    
    glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE );
    glStencilOp( GL_KEEP, GL_KEEP, GL_KEEP );
    
    if ( already_enabled )
	glStencilFunc( GL_EQUAL, ref-1, 0xffff );
    else
	glDisable( GL_STENCIL_TEST );

    Py_INCREF( Py_None );
    return Py_None;
}

static FUNC_PY( read_id )
{
    int x, y;
    GLfloat d;
    int id;
    int i, n;
    PyObject* result;

    n = PyTuple_Size(args);
    if ( n % 2 )
    {
	PyErr_SetString( PyExc_ValueError, "bad query object" );
	return NULL;
    }

    result = PyTuple_New( n / 2 );
    for( i = 0; i < n/2; ++i )
    {
	x = PyInt_AsLong( PyTuple_GET_ITEM( args, i*2 ) );
	y = PyInt_AsLong( PyTuple_GET_ITEM( args, i*2+1 ) );
	
	glReadPixels( x, y, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &d );
	id = (unsigned int)(d * (1<<(depth_bits-2)));
	
	PyTuple_SET_ITEM( result, i, PyInt_FromLong(id) );
    }

    return result;
}

void set_id_depth( int id )
{
    current_id = id;

    if ( id < 0 )
	current_id_depth = 0.0;
    else
	current_id_depth = compute_id_depth( id );

    glPopMatrix();
    glPushMatrix();
    glTranslated( 0, 0, current_id_depth );

    glDepthMask( (GLboolean)((current_id >= 0) ? GL_TRUE : GL_FALSE) );
}

static double compute_id_depth( int id )
{
    id &= (1<<depth_bits) - 1;
    return -(double)id / (1<<(depth_bits-2));
}



/** Local Variables: **/
/** dsp-name:"draw" **/
/** End: **/
