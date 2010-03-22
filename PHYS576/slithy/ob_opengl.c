#include "sl_common.h"

#ifndef GL_ERROR_CHECK
#define GL_ERROR_CHECK(str) {int qqqq;if((qqqq=glGetError())!=GL_NO_ERROR)printf("%s: GL error %x\n", str, qqqq);}
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static PyObject* opengl_init( PyObject* self, PyObject* args );
static PyObject* opengl_reshape( PyObject* self, PyObject* args );
static PyObject* opengl_startdraw( PyObject* self, PyObject* args );
static PyObject* opengl_enddraw( PyObject* self, PyObject* args );
static PyObject* opengl_blank( PyObject* self, PyObject* args );
static PyObject* opengl_unproject( PyObject* self, PyObject* args );
static PyObject* opengl_notify( PyObject* self, PyObject* args );

int Slsoggy_Init( Tcl_Interp* );
void SoggySwap( void );

double window_aspect = 1.0;
int window_width, window_height;

PyObject* glcamera;

static PyMethodDef OpenGLMethods[] = {
    { "init", opengl_init, METH_VARARGS },
    { "reshape", opengl_reshape, METH_VARARGS },
    { "startdraw", opengl_startdraw, METH_VARARGS },
    { "enddraw", opengl_enddraw, METH_VARARGS },
    { "blank", opengl_blank, METH_VARARGS },
    { "unproject", opengl_unproject, METH_VARARGS },
    { "notify", opengl_notify, METH_VARARGS },
    { NULL, NULL }
};

void
#ifdef WIN32
__declspec( dllexport )
#endif
initdobj( void )
{
    PyObject* m;

    m = Py_InitModule( "dobj", OpenGLMethods );

    Tcl_StaticPackage( NULL, "Slsoggy", Slsoggy_Init, NULL );

}

static PyObject* opengl_init( PyObject* self, PyObject* args )
{
    GL_ERROR_CHECK("top of opengl_init");
    printf( "     vendor : %s\n", glGetString( GL_VENDOR ) );
    printf( "   renderer : %s\n", glGetString( GL_RENDERER ) );
    printf( "    version : %s\n", glGetString( GL_VERSION ) );
    printf( "glu version : %s\n", gluGetString( GLU_VERSION ) );
    /*printf( "extensions : %s\n", glGetString( GL_EXTENSIONS ) );*/
    
    glEnable( GL_VERTEX_ARRAY );
    /* on mac, we expect that to fail; must be enabled as 
       client state instead, */
    if(glGetError() != GL_NO_ERROR) {
      glEnableClientState( GL_VERTEX_ARRAY );
      GL_ERROR_CHECK( "enable client gl_vertex_array" );
    }
    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

    glEnable( GL_DEPTH_TEST );
    glDepthFunc( GL_ALWAYS );

    glClearDepth( 0.0 );
    GL_ERROR_CHECK( "end of opengl_init" );
    

    //glEnable( GL_POLYGON_SMOOTH );
    
    Py_INCREF( Py_None );
    return Py_None;
}

static PyObject* opengl_reshape( PyObject* self, PyObject* args )
{
    if ( !PyArg_ParseTuple( args, "ii", &window_width, &window_height ) )
	return NULL;

    window_aspect = window_width / (double)window_height;
    
    glViewport( 0, 0, window_width, window_height );
    GL_ERROR_CHECK( "glViewport" );

    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();
    
    Py_INCREF( Py_None );
    return Py_None;
}

PyObject* query_object;


static PyObject* opengl_startdraw( PyObject* self, PyObject* args )
{
    PyObject* clear_color;

    query_object = NULL;

    GL_ERROR_CHECK( "top of opengl_startdraw" );
    
    if ( !PyArg_ParseTuple( args, "O|O", &clear_color, &query_object ) )
	return NULL;

    if ( query_object )
    {
	if ( !PyTuple_Check( query_object ) || PyTuple_GET_SIZE(query_object) % 2 != 0 )
	{
	    PyErr_SetString( PyExc_ValueError, "bad query object" );
	    return NULL;
	}
    }

    if ( PyTuple_Check( clear_color ) && PyTuple_Size( clear_color ) == 4 )
    {
	glClearColor( (float)PyFloat_AsDouble( PyTuple_GetItem( clear_color, 0 ) ),
		      (float)PyFloat_AsDouble( PyTuple_GetItem( clear_color, 1 ) ),
		      (float)PyFloat_AsDouble( PyTuple_GetItem( clear_color, 2 ) ),
		      0.0 );
    }
    else
	glClearColor( 1.0, 1.0, 1.0, 0.0 );
    
    glClearStencil( 0 );
    glDisable( GL_STENCIL_TEST );
    glClearDepth( 0.0 );
    glDepthMask( GL_TRUE );
    glClear( GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );

    glColor4d( 0.0, 0.0, 0.0, 1.0 );
    glLineWidth( 1.0 );

    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();
    /* left, right, bottom, top, zNear, zFar (in distance from eye) */
    glOrtho( -window_aspect, window_aspect, -1.0, 1.0, 0.0, 1.0 ); 
    glPushMatrix();

    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();

    glReadBuffer( GL_BACK );
    
    GL_ERROR_CHECK( "after opengl_redraw setup" );

    Py_INCREF( Py_None );
    return Py_None;
}

static PyObject* opengl_enddraw( PyObject* self, PyObject* args )
{
    char* filename = NULL;

    if ( !PyArg_ParseTuple( args, "|s", &filename ) )
	return NULL;

    GL_ERROR_CHECK( "top of opengl_enddraw" );

#if 0
    {
	GLfloat d;
	glReadPixels( 400, 300, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &d );
	printf( "-- enddraw %.16f\n", d );
    }
#endif

    
    glMatrixMode( GL_PROJECTION );
    glPopMatrix();
    
    if ( query_object )
    {
	PyObject* result;
	int n = PyTuple_GET_SIZE( query_object );
	int i, x, y;
	GLfloat d;
	GLint depth_bits;
	unsigned int id;
	
	glGetIntegerv( GL_DEPTH_BITS, &depth_bits );
	
	result = PyTuple_New( n / 2 );
	for( i = 0; i < n/2; ++i )
	{
	    x = PyInt_AsLong( PyTuple_GET_ITEM( query_object, i*2 ) );
	    y = PyInt_AsLong( PyTuple_GET_ITEM( query_object, i*2+1 ) );
	    
	    glReadPixels( x, y, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &d );
	    id = (unsigned int)(d * (1<<(depth_bits-2)));

	    PyTuple_SET_ITEM( result, i, PyInt_FromLong(id) );
	}

        GL_ERROR_CHECK( "another end to opengl_enddraw" );
	return result;
    }
    
    SoggySwap();

    if ( filename )
    {
	static PyObject* module;
	PyObject* im;
	PyObject* ret;
	PyObject* data;
	
	fprintf( stderr, "saving to [%s]\n", filename );

	if ( module == NULL )
	{
	    PyObject* name = PyString_FromString( "Image" );
	    module = PyImport_Import( name );
	    Py_DECREF( name );
	    if ( module == NULL )
	    {
		PyErr_SetString( PyExc_ImportError, "failed to find PIL module" );
		return NULL;
	    }
	}

	data = PyString_FromStringAndSize( NULL, window_width * window_height * 3 );
	glPixelStorei( GL_PACK_ALIGNMENT, 1 );
	glReadBuffer( GL_FRONT );
	glReadPixels( 0, 0, window_width, window_height, GL_RGB, GL_UNSIGNED_BYTE,
		      PyString_AsString( data ) );
        GL_ERROR_CHECK( "after glReadPixels" );
	
	im = PyObject_CallMethod( module, "fromstring", "s(ii)Ossii", "RGB", window_width, window_height,
				  data, "raw", "RGB", 0, -1 );

	ret = PyObject_CallMethod( im, "save", "s", filename );

	Py_XDECREF( ret );
	Py_XDECREF( im );
	Py_XDECREF( data );
    }

    GL_ERROR_CHECK( "an end to opengl_enddraw" );
    Py_INCREF( Py_None );
    return Py_None;
}

static PyObject* opengl_blank( PyObject* self, PyObject* args )
{
    double r = 0.2, g = 0.4, b = 0.6;

    if ( !PyArg_ParseTuple( args, "|ddd", &r, &g, &b ) )
	return NULL;

    glClearColor( (GLfloat)r, (GLfloat)g, (GLfloat)b, 0.0 );
    glClear( GL_COLOR_BUFFER_BIT );
    SoggySwap();

    Py_INCREF( Py_None );
    return Py_None;
}

static PyObject* opengl_unproject( PyObject* self, PyObject* args )
{
    double winx, winy;
    double ndcx, ndcy;
    double objx, objy;
    double ox, oy, ulen, utheta, aspect;

    if ( !PyArg_ParseTuple( args, "(ddddd)dd",
			    &ox, &oy, &ulen, &utheta, &aspect,
			    &winx, &winy ) )
	return NULL;

    window_aspect = window_aspect;
    
    winy = window_height - 1 - winy;
	
    ndcx = 2.0 * winx / window_width - 1.0;
    ndcy = 2.0 * winy / window_height - 1.0;
    
    // undo the gluOrtho2D effect
    ndcx *= window_aspect;
    
    if ( aspect > window_aspect )
    {
	objx = (ndcx + window_aspect) * ulen / (2.0 * window_aspect);
	objy = (ndcy + (window_aspect/aspect)) * ulen / (2.0 * window_aspect);
    }
    else
    {
	objx = (ndcx + aspect) * ulen / (2.0 * aspect);
	objy = (ndcy + 1.0) * ulen / (2.0 * aspect);
    }

    if ( utheta != 0.0 )
    {
	double c = cos( utheta * M_PI / 180.0 );
	double s = sin( utheta * M_PI / 180.0 );
	
	ndcx = objx;
	ndcy = objy;
	
	objx = c * ndcx - s * ndcy;
	objy = s * ndcx + c * ndcy;
    }
    
    objx += ox;
    objy += oy;
	
    return Py_BuildValue( "dd", objx, objy );
}



static PyObject* opengl_notify( PyObject* self, PyObject* args )
{
    int bit = 0;
    int state, toggle = 1;

    if ( !PyArg_ParseTuple( args, "i", &bit ) )
	return NULL;

    /* presumably this is the code to frob scroll lock while
       a slide is still animating? */
#ifdef WIN32
    state = GetKeyState( VK_SCROLL );
    state = state & 1;

    toggle = (!!bit) ^ state;
    
    if ( toggle )
    {
	INPUT input[2];
	
	input[0].type = INPUT_KEYBOARD;
	input[0].ki.wVk = VK_SCROLL;
	input[0].ki.wScan = 0x45;
	input[0].ki.dwFlags = KEYEVENTF_EXTENDEDKEY;
	input[0].ki.time = 0;
	input[0].ki.dwExtraInfo = 0;
	
	input[1].type = INPUT_KEYBOARD;
	input[1].ki.wVk = VK_SCROLL;
	input[1].ki.wScan = 0x45;
	input[1].ki.dwFlags = KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP;
	input[1].ki.time = 0;
	input[1].ki.dwExtraInfo = 0;

	SendInput( 2, input, sizeof(INPUT) );
    }
#endif

    Py_INCREF( Py_None );
    return Py_None;
}
    

    

/** Local Variables: **/
/** dsp-name:"dobj" **/
/** End: **/
