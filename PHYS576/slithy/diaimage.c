#include "sl_common.h"
#include "diaimage.h"

#ifndef FUNC_PY_STATIC
#define FUNC_PY_STATIC(fn)  static PyObject* fn( PyObject* self, PyObject* args )
#endif

PyObject* ImageError;

static void free_old_textures( void );
static GLuint* old_textures = NULL;
static int old_textures_alloc = 0;
static int old_textures_used = 0;

static void image_dealloc( PyObject* self )
{
    ImageObject* obj = (ImageObject*) self;

    free( obj->data );
    
    if ( obj->has_texture )
    {
	if ( old_textures_used + 1 > old_textures_alloc )
	{
	    old_textures_alloc += 8;
	    old_textures = realloc( old_textures, old_textures_alloc * sizeof( GLuint ) );
	}
	old_textures[old_textures_used++] = obj->texture;
    }

    PyObject_Del( self );
}

static void free_old_textures( void )
{
    GL_ERROR_CHECK( "top of free_old" );
    
    if ( old_textures_used > 0 )
    {
	glDeleteTextures( old_textures_used, old_textures );
	old_textures_used = 0;
    }

    GL_ERROR_CHECK( "bottom of free_old" );
}

static void draw_image( ImageObject* obj, double x, double y, double w, double h, double ax, double ay );
static int make_texture( ImageObject* obj );
static ImageObjectMethods c_methods = {
    draw_image, make_texture,
};

static PyObject* image_get_c_methods( PyObject* self )
{
    return PyCObject_FromVoidPtr( (void*)&c_methods, NULL );
}

static PyObject* image_preload( PyObject* self )
{
    make_texture( (ImageObject*)self );

    Py_INCREF( Py_None );
    return Py_None;
}

static PyMethodDef image_methods[] = {
    { "c_methods", (PyCFunction)image_get_c_methods, METH_NOARGS },
    { "preload", (PyCFunction)image_preload, METH_NOARGS },
    { NULL, NULL }
};

static PyObject* image_getattr( PyObject* self, char* attrname )
{
    PyObject* result = NULL;
    ImageObject* obj = (ImageObject*) self;

    if ( strcmp( attrname, "size" ) == 0 )
	result = Py_BuildValue( "ii", obj->w, obj->h );
    else if ( strcmp( attrname, "aspect" ) == 0 )
	result = Py_BuildValue( "d", obj->w / (double)(obj->h) );
    else if ( strcmp( attrname, "texturesize" ) == 0 )
	result = Py_BuildValue( "ii", obj->tw, obj->th );
    else if ( strcmp( attrname, "filename" ) == 0 )
    {
	Py_INCREF( obj->filename );
	result = obj->filename;
    }
    else
	result = Py_FindMethod( image_methods, self, attrname );

    return result;
}

PyTypeObject ImageType = {
    PyObject_HEAD_INIT(NULL)
    0,
    "DiagramImage",
    sizeof( ImageObject ),
    0,
    image_dealloc,
    0,                    // tp_print
    image_getattr,        // tp_getattr
    0,                    // tp_setattr
    0,                    // tp_compare
    0,                    // tp_repr
    0,                    // tp_as_number
    0,                    // tp_as_sequence
    0,                    // tp_as_mapping
    0,                    // tp_hash
    0,                                 // tp_call 
    0,                                 // tp_str 
    0,                                 // tp_getattro 
    0,                                 // tp_setattro 
    0,                                 // tp_as_buffer 
    Py_TPFLAGS_DEFAULT,                // tp_flags 
    0,                                 // tp_doc 
    0,                                 // tp_traverse 
    0,                                 // tp_clear 
    0,                                 // tp_richcompare 
    0,                                 // tp_weaklistoffset 
    0,             // tp_iter 
    0,                                 // tp_iternext 
};

static PyObject* image_new( PyObject* self, PyObject* args )
{
    ImageObject* imageobj = NULL;
    PyObject* pilobj = NULL;
    PyObject* modestrobj = NULL;
    PyObject* sizeobj = NULL;
    PyObject* datastrobj = NULL;
    unsigned char* data;
    char* modestr;
    int i;
    int pixelsize;
    static PyObject* PILmodule = NULL;
    PyObject* oldpilobj;
    PyObject* nameobj;

    if ( !PyArg_ParseTuple( args, "O:new_image", &pilobj ) )
	return NULL;
    Py_INCREF( pilobj );

    if ( PyString_Check( pilobj ) )
    {
	// object is a string; assume it's a filename and attempt to create the PIL object ourselves;
#if 0
	printf( "fine, creating the PIL object myself\n" );
#endif

	if ( PILmodule == NULL )
	{
	    PyObject* name = PyString_FromString( "Image" );
	    PILmodule = PyImport_Import( name );
	    Py_DECREF( name );
	    if ( PILmodule == NULL )
	    {
		PyErr_SetString( ImageError, "failed to find PIL module" );
		goto error;
	    }
	}

	nameobj = pilobj;
	
	oldpilobj = pilobj;
	pilobj = PyObject_CallMethod( PILmodule, "open", "O", pilobj );
	Py_DECREF( oldpilobj );
	if ( pilobj == NULL )
	    goto error;
	Py_INCREF( pilobj );
    }
    else
	nameobj = Py_None;
    
    imageobj = PyObject_New( ImageObject, &ImageType );

    Py_INCREF( nameobj );
    imageobj->filename = nameobj;

    modestrobj = PyObject_GetAttrString( pilobj, "mode" );
    modestr = PyString_AsString( modestrobj );
    if ( strcmp( modestr, "RGBA" ) == 0 )
    {
	pixelsize = 4;
    }
    else if ( strcmp( modestr, "RGB" ) == 0 )
    {
	pixelsize = 3;
    }
    else
    {
#if 0
	printf( "attempting conversion to RGB\n" );
#endif
	
	// need to convert image to RGB
	oldpilobj = pilobj;
	pilobj = PyObject_CallMethod( pilobj, "convert", "s", "RGB" );
	Py_DECREF( oldpilobj );
	if ( pilobj == NULL )
	    goto error;
	Py_INCREF( pilobj );

	pixelsize = 3;
    }

    sizeobj = PyObject_GetAttrString( pilobj, "size" );
    if ( !PyTuple_Check( sizeobj ) || PyTuple_Size( sizeobj ) != 2 )
    {
	PyErr_SetString( ImageError, "size attribute is wrong wrong wrong" );
	Py_DECREF( sizeobj );
	Py_DECREF( pilobj );
	return NULL;
    }

    imageobj->w = PyInt_AsLong( PyTuple_GetItem( sizeobj, 0 ) );
    imageobj->h = PyInt_AsLong( PyTuple_GetItem( sizeobj, 1 ) );
    if ( imageobj->w < 1 || imageobj->h < 1 )
    {
	PyErr_Format( ImageError, "PIL says image size is %d x %d; that can't be right",
		      imageobj->w, imageobj->h );
	goto error;
    }
#if 0
    printf( "image is %d x %d\n", imageobj->w, imageobj->h );
#endif

    imageobj->tw = 1;
    while( imageobj->tw < imageobj->w )
    {
	imageobj->tw <<= 1;
	if ( imageobj->tw <= 0 )
	{
	    PyErr_SetString( ImageError, "image is too wide" );
	    goto error;
	}
    }
    imageobj->th = 1;
    while( imageobj->th < imageobj->h )
    {
	imageobj->th <<= 1;
	if ( imageobj->th <= 0 )
	{
	    PyErr_SetString( ImageError, "image is too tall" );
	    goto error;
	}
    }

#if 0
    printf( "texture must be %d x %d\n", imageobj->tw, imageobj->th );
#endif

    datastrobj = PyObject_CallMethod( pilobj, "tostring", NULL );
    if ( datastrobj == NULL )
	goto error;
    PyString_AsStringAndSize( datastrobj, (char**)&data, &i );
    if ( i != imageobj->w * imageobj->h * pixelsize )
    {
	PyErr_Format( ImageError, "got %d bytes of image data; expected %d", i,
		      imageobj->w * imageobj->h * pixelsize );
	goto error;
    }

    imageobj->data = malloc( imageobj->tw * imageobj->th * pixelsize );
    for ( i = 0; i < imageobj->h; ++i )
	memcpy( imageobj->data + i * (pixelsize * imageobj->tw),
		data + i * (pixelsize * imageobj->w),
		pixelsize*imageobj->w );
    if ( imageobj->w < imageobj->tw )
    {
	for ( i = 0; i < imageobj->h; ++i )
	    memcpy( imageobj->data + i * (pixelsize * imageobj->tw) + pixelsize * imageobj->w,
		    imageobj->data + i * (pixelsize * imageobj->tw) + pixelsize * (imageobj->w-1),
		    pixelsize );
    }
    if ( imageobj->h < imageobj->th )
    {
	memcpy( imageobj->data + imageobj->tw * pixelsize * imageobj->h,
		imageobj->data + imageobj->tw * pixelsize * (imageobj->h-1),
		imageobj->tw * pixelsize );
    }
		    
    
    imageobj->has_alpha = (pixelsize == 4);
    imageobj->has_texture = 0;
    imageobj->tright = ((double)imageobj->w) / imageobj->tw;
    imageobj->tbottom = ((double)imageobj->h) / imageobj->th;

    Py_DECREF( datastrobj );
    Py_DECREF( sizeobj );
    Py_DECREF( modestrobj );
    Py_DECREF( pilobj );
    
    return (PyObject*)imageobj;

 error:
    if ( imageobj )
	PyObject_Del( imageobj );
    Py_XDECREF( modestrobj );
    Py_XDECREF( sizeobj );
    Py_XDECREF( datastrobj );
    Py_XDECREF( pilobj );
    return NULL;
}

static PyObject* image_get( PyObject* self, PyObject* args )
{
    static PyObject* cache = NULL;
    PyObject* argobj;
    static PyObject* ospath = NULL;
    PyObject* key;
    PyObject* item;

    if( !PyArg_ParseTuple( args, "O", &argobj ) )
	return NULL;

    if ( cache == NULL )
	cache = PyDict_New();

    if ( PyString_Check( argobj ) )
    {
	// arg is a string; assume it's a filename

	if ( ospath == NULL )
	{
	    PyObject* name = PyString_FromString( "os.path" );
	    ospath = PyImport_Import( name );
	    Py_DECREF( name );
	    if ( ospath == NULL )
		return NULL;
	}

	if ( !(key = PyObject_CallMethod( ospath, "abspath", "O", argobj )) )
	    return NULL;
    }
    else
    {
	key = argobj;
	Py_INCREF( key );
    }
    
    if ( ( item = PyDict_GetItem( cache, key ) ) != 0 )
    {
	// hooray, in the cache
	Py_DECREF( key );
	Py_INCREF( item );
	return item;
    }
    else
    {
	argobj = PyTuple_New( 1 );
	PyTuple_SetItem( argobj, 0, key );
	item = image_new( NULL, argobj );
	Py_DECREF( argobj );
	if ( item != NULL )
	{
	    Py_INCREF( item );
	    PyDict_SetItem( cache, key, item );
	}
	return item;
    }
}

static PyMethodDef module_methods[] = {
    { "get_image", image_get, METH_VARARGS },
    { "new_image", image_new, METH_VARARGS },
    { NULL, NULL }
};

#ifdef WIN32
__declspec(dllexport)
#endif
void initdiaimage( void )
{
    PyObject *m;
    PyObject *d;
    
    ImageType.ob_type = &PyType_Type;

    m = Py_InitModule( "diaimage", module_methods );
    d = PyModule_GetDict( m );
    ImageError = PyErr_NewException( "diaimage.ImageError", NULL, NULL );
    PyDict_SetItemString( d, "DiaImageError", ImageError );

    Py_INCREF( &ImageType );
    PyModule_AddObject( m, "DiaImage", (PyObject*)&ImageType );
}

static int make_texture( ImageObject* obj )
{
    int bound = 0;
    
    free_old_textures();
    
    if ( obj->has_texture == 0 )
    {
	GLuint t;

	glGenTextures( 1, &t );

	glBindTexture( GL_TEXTURE_2D, t );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	glTexImage2D( GL_TEXTURE_2D, 0, obj->has_alpha ? GL_RGBA : GL_RGB,
		      obj->tw, obj->th, 0, obj->has_alpha ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE,
		      obj->data );

	GL_ERROR_CHECK( "draw_image - created texture" );
	
	obj->texture = t;
	obj->has_texture = 1;
	bound = 1;

	free( obj->data );
	obj->data = NULL;
    }

    return bound;
}

static void draw_image( ImageObject* obj, double x, double y, double w, double h, double ax, double ay )
{
    GL_ERROR_CHECK( "top of draw_image" );

    // would like to save GL_TEXTURE_BIT as well, but it tweaks a bug in
    // ATI's radeon 7500 drivers.
    
    glPushAttrib( GL_ENABLE_BIT | GL_TRANSFORM_BIT );
    glEnable( GL_TEXTURE_2D );

    if ( ! make_texture( obj ) )
	glBindTexture( GL_TEXTURE_2D, obj->texture );

    glMatrixMode( GL_MODELVIEW );
    glPushMatrix();
    glTranslated( x - w * ax, y - h * ay, 0 );

    glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );

    glBegin( GL_QUADS );

    glTexCoord2d( 0.0, obj->tbottom );
    glVertex2d( 0, 0 );

    glTexCoord2d( obj->tright, obj->tbottom );
    glVertex2d( w, 0 );

    glTexCoord2d( obj->tright, 0.0 );
    glVertex2d( w, h );

    glTexCoord2d( 0.0, 0.0 );
    glVertex2d( 0, h );

    glEnd();

    glPopMatrix();
    glPopAttrib();
}

    

/** Local Variables: **/
/** dsp-name:"slithy" **/
/** End: **/
