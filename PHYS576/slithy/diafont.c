#include "sl_common.h"

#ifndef _MAKEDEPEND
#include <math.h>
#include <string.h>
#ifndef __APPLE__
#include <search.h>
#endif

#include <ft2build.h>
#include FT_FREETYPE_H

/* _MAKEDEPEND */
#endif

#include "diafont.h"

#define BITMAP_WIDTH  512
#define BITMAP_START_HEIGHT  64
#define MARGIN 2

#ifndef FUNC_PY_STATIC
#define FUNC_PY_STATIC(fn)  static PyObject* fn( PyObject* self, PyObject* args )
#endif

FUNC_PY_STATIC( font_get_c_methods );
FUNC_PY_STATIC( font_astuple );

PyObject* FontError;
PyObject* extra_characters = NULL;
static FT_Library library;
static int library_initialized = 0;

static void index_glyphs( FontObject* f, FT_Face face, PyObject* extra_characters, int* glyphheights );
static int index_compare( const void *a, const void* b );
static int load_font_from_tuple( FontObject* f, PyObject* tuple );


static int load_font( FontObject* f, char* filename, int pixelsize );
static void load_glyph( FT_Face face, int k, FontObject* f, int pixelsize, 
			int* x, int* y, int* maxh, int* bitmap_height, unsigned char** bitmap );

static void make_texture( FontObject* f );
static void start_using_font( FontObject* f );
static void finish_using_font( FontObject* f );
static void draw_character( FontObject* f, unsigned int k, double x, double y, double* nx, double* ny );
static int get_dimensions( FontObject* f, unsigned int k,
			   double* xmin, double* xmax,
			   double* ymin, double* ymax,
			   double* aw, double* ah );


static void free_old_textures( void );
static GLuint* old_textures = NULL;
static int old_textures_alloc = 0;
static int old_textures_used = 0;

static void font_dealloc( PyObject* self )
{
    FontObject* obj = (FontObject*) self;
    int i;

    Py_DECREF( obj->name );    
    free( obj->filename );

    for ( i = 0; i < obj->num_glyphs; ++i )
	free( obj->tab[i] );
    free( obj->tab );
    free( obj->bitmap );
    free( obj->char_index );
    
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
    if ( old_textures_used > 0 )
    {
	glDeleteTextures( old_textures_used, old_textures );
	old_textures_used = 0;
    }
}

static PyMethodDef font_methods[] = {
    { "c_methods", font_get_c_methods },
    { "astuple", font_astuple },
    { NULL, NULL }
};

static FontObjectMethods c_methods = {
    start_using_font,
    finish_using_font,
    draw_character,
    get_dimensions,
    NULL
};

static PyObject* font_get_c_methods( PyObject* self, PyObject* args )
{
    return PyCObject_FromVoidPtr( (void*)&c_methods, NULL );
}
	

static PyObject* font_getattr( PyObject* self, char* attrname )
{
    PyObject* result = NULL;
    FontObject* obj = (FontObject*) self;

    if ( strcmp( attrname, "filename" ) == 0 )
	result = Py_BuildValue( "s", obj->filename );
    else if ( strcmp ( attrname, "name" ) == 0 )
    {
	result = obj->name;
	Py_INCREF( result );
    }
    else if ( strcmp( attrname, "pixelsize" ) == 0 )
	result = Py_BuildValue( "i", obj->pixelsize );
    else if ( strcmp( attrname, "diafont_obj" ) == 0 )
    {
	Py_INCREF( Py_None );
	result = Py_None;
    }
    else
	result = Py_FindMethod( font_methods, self, attrname );

    return result;
}

PyTypeObject FontType = {
    PyObject_HEAD_INIT(NULL)
    0,
    "Font",
    sizeof( FontObject ),
    0,
    font_dealloc,         // tp_dealloc
    0,                    // tp_print
    font_getattr,         // tp_getattr
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

static PyObject* font_new( PyObject* self, PyObject* args )
{
    FontObject* fontobj;

    if ( PyTuple_Size(args) == 2 )
    {
	char* filename;
	int pixelsize;
	
	if ( !PyArg_ParseTuple( args, "si:new_font", &filename, &pixelsize ) )
	    return NULL;

	fontobj = PyObject_New( FontObject, &FontType );
	if ( load_font( fontobj, filename, pixelsize ) != 0 )
	{
	    PyObject_Del( fontobj );
	    return NULL;
	}
    }
    else
    {
	PyObject* tuple;
	
	if ( !PyArg_ParseTuple( args, "O!", &PyTuple_Type, &tuple ) )
	    return NULL;

	fontobj = PyObject_New( FontObject, &FontType );
	if ( load_font_from_tuple( fontobj, tuple ) != 0 )
	{
	    PyObject_Del( fontobj );
	    return NULL;
	}
    }

    return (PyObject*)fontobj;
}

static PyObject* add_extra_characters( PyObject* self, PyObject* args )
{
    PyObject* newextra;
    PyObject* temp;
    
    if ( !PyArg_ParseTuple( args, "O!", &PyUnicode_Type, &newextra ) )
	return NULL;

    if ( extra_characters == NULL )
    {
	extra_characters = newextra;
	Py_INCREF( extra_characters );
    }
    else
    {
	temp = PyUnicode_Concat( extra_characters, newextra );
	Py_DECREF( extra_characters );
	extra_characters = temp;
	Py_INCREF( extra_characters );
    }

    Py_INCREF( extra_characters );
    return extra_characters;
}

static PyObject* font_getinfo( PyObject* self, PyObject* args )
{
    char* filename;
    FT_Face face;
    int error;
    PyObject* result;

    if ( !PyArg_ParseTuple( args, "s", &filename ) )
	return NULL;

    if ( !library_initialized )
    {
	error = FT_Init_FreeType( &library );
	if ( error )
	{
	    PyErr_Format( FontError, "error %d in FT_Init_FreeType", error );
	    return NULL;
	}
	library_initialized = 1;
    }
    error = FT_New_Face( library, filename, 0, &face );
    if ( error )
    {
	PyErr_Format( FontError, "error %d in FT_New_Face", error );
	return NULL;
    }

    result = Py_BuildValue( "sii", face->family_name,
			    !!(face->style_flags & FT_STYLE_FLAG_BOLD),
			    !!(face->style_flags & FT_STYLE_FLAG_ITALIC) );

    FT_Done_Face( face );

    return result;
}

static PyMethodDef module_methods[] = {
    { "new_font", font_new, METH_VARARGS },
    { "add_extra_characters", add_extra_characters, METH_VARARGS },
    { "get_font_info", font_getinfo, METH_VARARGS },
    { NULL, NULL }
};

#ifdef WIN32
__declspec(dllexport)
#endif
void initdiafont( void )
{
    PyObject *m;
    PyObject *d;
    
    FontType.ob_type = &PyType_Type;

    m = Py_InitModule( "diafont", module_methods );
    d = PyModule_GetDict( m );
    FontError = PyErr_NewException( "diafont.FontError", NULL, NULL );
    PyDict_SetItemString( d, "FontError", FontError );

    Py_INCREF( &FontType );
    PyModule_AddObject( m, "Font", (PyObject*)&FontType );

    Py_INCREF( &FontType );
    c_methods.font_class = (PyObject*)&FontType;
}

	
static int create_character( FT_Face face, int k, int* w, int* h,
			     double* awidth, double* aheight,
			     double* offsetx, double* offsety );
static int glyph_height( FT_Face face, int k );

static int glyph_height_compare( const void* a, const void* b )
{
    return (*(int*)b) - (*(int*)a);
}

static int load_font( FontObject* f, char* filename, int pixelsize )
{
    int error;
    int i;
    unsigned char* bitmap = NULL;
    int bitmap_height;
    int x, y, h, maxh;
    int k;
    FT_Face face;
    int* glyphheights;

    if ( !library_initialized )
    {
	error = FT_Init_FreeType( &library );
	if ( error )
	{
	    PyErr_Format( FontError, "error %d in FT_Init_FreeType", error );
	    return -1;
	}
	library_initialized = 1;
    }
    error = FT_New_Face( library, filename, 0, &face );
    if ( error )
    {
	PyErr_Format( FontError, "error %d in FT_New_Face", error );
	return -1;
    }

    if ( FT_Select_Charmap( face, ft_encoding_unicode ) )
    {
	if ( FT_Select_Charmap( face, ft_encoding_symbol ) )
	{
	    PyErr_SetString( FontError, "this font doesn't have a usable encoding" );
	    return -1;
	}
	else
	    f->symbolfont = 1;
    }
    else
    {
	f->symbolfont = 0;
    }
    
#if 0
    printf( "face has %d charmaps\n", face->num_charmaps );
    for ( i = 0; i < face->num_charmaps; ++i )
    {
	FT_Encoding e = face->charmaps[i]->encoding;
	printf( "   encoding  \"%c%c%c%c\"  platform %d encoding_id %d\n",
		(e >> 24) & 0xff,
		(e >> 16) & 0xff,
		(e >> 8) & 0xff,
		(e >> 0) & 0xff,
		face->charmaps[i]->platform_id,
		face->charmaps[i]->encoding_id );
    }
#endif
    
    if ( face->style_name == NULL )
	f->name = PyString_FromString( face->family_name );
    else
	f->name = PyString_FromFormat( "%s %s", face->family_name, face->style_name );

    error = FT_Set_Pixel_Sizes( face, 0, pixelsize );
    if ( error )
    {
	PyErr_Format( FontError, "error %d in FT_Set_Pixel_Sizes", error );
	return -1;
    }

    f->filename = strdup( filename );
    f->pixelsize = pixelsize;
    f->tab = malloc( face->num_glyphs * sizeof( texture_character* ) );
    for ( i = 0; i < face->num_glyphs; ++i )
	f->tab[i] = NULL;

    glyphheights = malloc( face->num_glyphs * 2 * sizeof( int ) );
    for ( i = 0; i < face->num_glyphs; ++i )
    {
	glyphheights[i*2+0] = -1;
	glyphheights[i*2+1] = i;
    }
    index_glyphs( f, face, extra_characters, glyphheights );
    
    // do glyphs in order of height
    qsort( glyphheights, face->num_glyphs, 2 * sizeof( int ), glyph_height_compare );

    //
    // create a bitmap
    //

    bitmap_height = BITMAP_START_HEIGHT;
    bitmap = malloc( BITMAP_WIDTH * bitmap_height );
    memset( bitmap, 0, BITMAP_WIDTH * bitmap_height );

    x = MARGIN;
    y = MARGIN;
    maxh = 0;
    f->num_glyphs = face->num_glyphs;

    for ( i = 0; i < face->num_glyphs; ++i )
	if ( glyphheights[i*2+0] >= 0 )
	    load_glyph( face, glyphheights[i*2+1], f, pixelsize,
			&x, &y, &maxh, &bitmap_height, &bitmap );
    
    for ( k = 0; k < face->num_glyphs; ++k )
    {
	if ( f->tab[k] )
	{
	    y = (int)f->tab[k]->tbottom;
	    h = (int)f->tab[k]->ttop;
	    f->tab[k]->tbottom = (double)(y+h+1) / bitmap_height;
	    f->tab[k]->ttop = (double)(y) / bitmap_height;
	}
    }

#if 0
    {
	FILE* ff;
	char filename[50];
	sprintf( filename, "font%08x.pgm", f );
	printf( "%s: bitmap is %d x %d\n", filename, BITMAP_WIDTH, bitmap_height );
	ff = fopen( filename, "wb" );
	fprintf( ff, "P5\n%d %d\n255\n", BITMAP_WIDTH, bitmap_height );
	fwrite( bitmap, BITMAP_WIDTH, bitmap_height, ff );
	fclose( ff );
    }
#endif

    f->bitmap = bitmap;
    f->bw = BITMAP_WIDTH;
    f->bh = bitmap_height;
    f->has_texture = 0;

    free( glyphheights );

    return 0;
}

static void index_glyphs( FontObject* f, FT_Face face, PyObject* extra_characters,
			  int* glyphheights )
{
    unsigned int charcode;
    unsigned int glyph;
    unsigned int* index;
    int used, alloc;
    int extra_count;
    Py_UNICODE* extra;
    int i;
    int j;

    if ( extra_characters )
    {
	extra_count = PyUnicode_GetSize( extra_characters );
	extra = PyUnicode_AsUnicode( extra_characters );
    }
    else
	extra_count = 0;

    used = 0;
    alloc = 256;
    index = malloc( alloc * 2 * sizeof(unsigned int) );

    charcode = FT_Get_First_Char( face, &glyph );
    while ( glyph )
    {
	if ( charcode < 0x100 || f->symbolfont )
	    j = 1;
	else
	{
	    j = 0;
	    for ( i = 0; i < extra_count; ++i )
		if ( charcode == extra[i] )
		{
		    j = 1;
		    break;
		}
	}

	if ( j )
	{
	    if ( used >= alloc )
	    {
		alloc += 256;
		extra = realloc( extra, alloc * 2 * sizeof(unsigned int) );
	    }
	    index[used*2] = charcode;
	    index[used*2+1] = glyph;
	    ++used;
	    
	    glyphheights[glyph*2] = glyph_height( face, glyph );
	}
	
	//printf( "%s charcode %x   glyph %d\n", j ? "**" : "  ", charcode, glyph );
	charcode = FT_Get_Next_Char( face, charcode, &glyph );
    }

    qsort( index, used, 2 * sizeof(unsigned int), index_compare );

    index = realloc( index, used * 2 * sizeof(unsigned int) );

    f->char_index = index;
    f->char_index_size = used;
}

static int index_compare( const void *a, const void* b )
{
    if ( *(unsigned int*)a < *(unsigned int*)b )
	return -1;
    else if ( *(unsigned int*)a > *(unsigned int*)b )
	return 1;
    else
	return 0;
}

static int glyph_height( FT_Face face, int k )
{
    int w, h;
    double awidth, aheight;
    double offsetx, offsety;

    if ( create_character( face, k, &w, &h, &awidth, &aheight, &offsetx, &offsety ) )
	return -1;

    return h;
}

static void load_glyph( FT_Face face, int k, FontObject* f, int pixelsize, 
			int* x, int* y, int* maxh, int* bitmap_height, unsigned char** bitmap )
{
    int w, h;
    double awidth, aheight;
    double offsetx, offsety;
    int i;

    if ( f->tab[k] )
	return;
    
    if ( create_character( face, k, &w, &h, &awidth, &aheight, &offsetx, &offsety ) )
	return;
    
    if ( *x + w > BITMAP_WIDTH - MARGIN )
    {
	*y += *maxh + MARGIN;
	*x = MARGIN;
	*maxh = 0;
    }
    
    if ( h > *maxh ) *maxh = h;
    
    if ( *y+h+1 > *bitmap_height )
    {
	*bitmap_height *= 2;
	*bitmap = realloc( *bitmap, BITMAP_WIDTH * *bitmap_height );
	memset( *bitmap + BITMAP_WIDTH * (*bitmap_height/2), 0, BITMAP_WIDTH * (*bitmap_height/2) );
    }

    for ( i = 0; i < h; ++i )
	memcpy( *bitmap + (*y+i)*BITMAP_WIDTH + *x, face->glyph->bitmap.buffer + i * face->glyph->bitmap.pitch, w );
    
    f->tab[k] = malloc( sizeof( texture_character ) );
    
    f->tab[k]->tleft = (double)*x / BITMAP_WIDTH;
    f->tab[k]->tright = (double)(*x+w) / BITMAP_WIDTH;
    f->tab[k]->tbottom = *y;
    f->tab[k]->ttop = h;
    
    f->tab[k]->width = (double)w / pixelsize;
    f->tab[k]->height = (double)h / pixelsize;
    f->tab[k]->awidth = awidth / pixelsize;
    f->tab[k]->aheight = aheight / pixelsize;
    f->tab[k]->offsetx = offsetx / pixelsize;
    f->tab[k]->offsety = offsety / pixelsize;
    
    *x += w + MARGIN;
}
    

static int create_character( FT_Face face, int k, int* w, int* h,
			     double* awidth, double* aheight,
			     double* offsetx, double* offsety )
{
    FT_Bitmap bitmap;
    int error;

    // render character to the glyph slot's bitmap
    error = FT_Load_Glyph( face, k, FT_LOAD_DEFAULT );
    if ( error )
    {
	fprintf( stderr, "didn't find glyph %d\n", k );
	return -1;
    }
    
    if ( face->glyph->format != ft_glyph_format_bitmap )
    {
	error = FT_Render_Glyph( face->glyph, ft_render_mode_normal );
	if ( error ) return -1;
    }
    bitmap = face->glyph->bitmap;

    *w = bitmap.width;
    *h = bitmap.rows;

    *awidth = (double) (face->glyph->advance.x >> 6);
    *aheight = (double) (face->glyph->advance.y >> 6);
    *offsetx = (double) face->glyph->bitmap_left;
    *offsety = (double) face->glyph->bitmap_top;
    
    return 0;
}

static void make_texture( FontObject* f )
{
    GLuint t;

    glGenTextures( 1, &t );
    f->texture = t;

#if 0
    printf( "creating texture %d\n", f->texture );
    printf( "  bitmap is %d x %d\n", f->bw, f->bh );
#endif
    
    glBindTexture( GL_TEXTURE_2D, t );
    glTexImage2D( GL_TEXTURE_2D, 0, GL_ALPHA,
		  f->bw, f->bh, 0, GL_ALPHA, GL_UNSIGNED_BYTE,
		  f->bitmap );
    
    GL_ERROR_CHECK( "end of make_texture" );
    
    f->has_texture = 1;
}

static void start_using_font( FontObject* f )
{
    free_old_textures();
    
    glPushAttrib( GL_ENABLE_BIT );
    glEnable( GL_TEXTURE_2D );
    
    if ( f->has_texture == 0 )
	make_texture( f );
    else
	glBindTexture( GL_TEXTURE_2D, f->texture );
    
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    
    glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
    //glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE );

    GL_ERROR_CHECK( "end of start_using_font" );
}

static void finish_using_font( FontObject* f )
{
    glPopAttrib();
}


static int charindex_lookup( FontObject* f, unsigned int charcode )
{
    unsigned int* found;

    found = bsearch( &charcode, f->char_index, f->char_index_size, 2 * sizeof(unsigned int),
		     index_compare );

    if ( !found )
	return -1;
    else
	return found[1];
}

static void draw_character( FontObject* f, unsigned int j, double x, double y, double* nx, double* ny )
{
    texture_character* tc;
    int k;

    if ( j == 0xa0 )  // hack hack hack
	j = 0x20;
    
    if ( f->symbolfont ) j |= 0xf000;
    k = charindex_lookup( f, j );

#if 0
    printf( "character %x index %d\n", j, k );
#endif
    
    if ( k == -1 )
    {
	printf( "missing character %x\n", j );
	return;
    }
    
    tc = f->tab[k];
    if ( tc == NULL )
	return;

    glBegin( GL_QUADS );

    glTexCoord2d( tc->tleft, tc->tbottom );
    glVertex2d( x + tc->offsetx, y + tc->offsety - tc->height );
	
    glTexCoord2d( tc->tleft, tc->ttop );
    glVertex2d( x + tc->offsetx, y + tc->offsety );
    
    glTexCoord2d( tc->tright, tc->ttop );
    glVertex2d( x + tc->offsetx + tc->width, y + tc->offsety );
    
    glTexCoord2d( tc->tright, tc->tbottom );
    glVertex2d( x + tc->offsetx + tc->width, y + tc->offsety - tc->height );

    glEnd();

#if 0
    printf( "drawing [%c]   %.4f  x: %.4f %.4f\n", j, x, x+tc->offsetx, x+tc->offsetx+tc->width );
#endif
    
    GL_ERROR_CHECK( "end of draw_character" );
    
    if ( nx )
	*nx = x + tc->awidth;
    if ( ny )
	*ny = y + tc->aheight;
}
	
static int get_dimensions( FontObject* f, unsigned int k, double* xmin, double* xmax,
			   double* ymin, double* ymax, double* aw, double* ah )
{
    texture_character* tc;
#if 0
    unsigned int orig_k = k;
#endif
    int glyph;

    if ( k == 0xa0 )   // hack hack hack
	k = 0x20;
    
    if ( f->symbolfont ) k |= 0xf000;
    glyph = charindex_lookup( f, k );
    if ( glyph == -1 )
	return -1;
    
    tc = f->tab[glyph];
    if ( tc == NULL )
	return -1;

    *xmin = tc->offsetx;
    *xmax = tc->offsetx + tc->width;
    *ymin = tc->offsety - tc->height;
    *ymax = tc->offsety;
    *aw = tc->awidth;
    *ah = tc->aheight;

#if 0
    printf( "[%c] min: %.4f %.4f  max: %.4f %.4f  adv: %.4f %.4f\n",
	    orig_k, *xmin, *ymin, *xmax, *ymax, *aw, *ah );
    printf( "    offset: %.4f %.4f  size: %.4f %.4f\n", tc->offsetx, tc->offsety,
	    tc->width, tc->height );
#endif
    
    return 0;
}

#define SLITHY_FONT_TAG  "slithy_font_001"

static PyObject* font_astuple( PyObject* self, PyObject* args )
{
#if 0
    static PyObject* zlibmodule = NULL;
#endif
    static PyObject* tag = NULL;
    FontObject* f = (FontObject*)self;
    PyObject* bitmap;
    PyObject* bitmapz;
    PyObject* index;
    PyObject* tab;
    PyObject* result;
    int tabused;
    int i, j;

    if ( tag == NULL )
	tag = PyString_FromString( SLITHY_FONT_TAG );

#if 0
    if ( zlibmodule == NULL )
    {
	zlibmodule = PyImport_ImportModule( "zlib" );
	if ( zlibmodule == NULL )
	    return NULL;
    }
#endif

    bitmap = PyString_FromStringAndSize( f->bitmap, f->bw * f->bh );
    if ( bitmap == NULL )
	return NULL;
    //bitmapz = PyObject_CallMethod( zlibmodule, "compress", "O", bitmap );
    bitmapz = PyObject_CallMethod( bitmap, "encode", "s", "zlib" );
    Py_XDECREF( bitmap );
    if ( bitmapz == NULL )
	return NULL;
    
    index = PyTuple_New( f->char_index_size * 2 );
    for ( i = 0; i < f->char_index_size * 2; ++i )
	PyTuple_SET_ITEM( index, i, PyInt_FromLong( f->char_index[i] ) );

    tabused = 0;
    for ( i = 0; i < f->num_glyphs; ++i )
	if ( f->tab[i] )
	    ++tabused;

    tab = PyTuple_New( tabused * 11 );
    j = 0;
    for ( i = 0; i < f->num_glyphs; ++i )
	if ( f->tab[i] )
	{
	    PyTuple_SET_ITEM( tab, j * 11, PyInt_FromLong( i ) );
	    PyTuple_SET_ITEM( tab, j * 11 + 1, PyFloat_FromDouble( f->tab[i]->tleft ) );
	    PyTuple_SET_ITEM( tab, j * 11 + 2, PyFloat_FromDouble( f->tab[i]->tright ) );
	    PyTuple_SET_ITEM( tab, j * 11 + 3, PyFloat_FromDouble( f->tab[i]->tbottom ) );
	    PyTuple_SET_ITEM( tab, j * 11 + 4, PyFloat_FromDouble( f->tab[i]->ttop ) );
	    PyTuple_SET_ITEM( tab, j * 11 + 5, PyFloat_FromDouble( f->tab[i]->width ) );
	    PyTuple_SET_ITEM( tab, j * 11 + 6, PyFloat_FromDouble( f->tab[i]->height ) );
	    PyTuple_SET_ITEM( tab, j * 11 + 7, PyFloat_FromDouble( f->tab[i]->awidth ) );
	    PyTuple_SET_ITEM( tab, j * 11 + 8, PyFloat_FromDouble( f->tab[i]->aheight ) );
	    PyTuple_SET_ITEM( tab, j * 11 + 9, PyFloat_FromDouble( f->tab[i]->offsetx ) );
	    PyTuple_SET_ITEM( tab, j * 11 + 10, PyFloat_FromDouble( f->tab[i]->offsety ) );
	    ++j;
	}
    
    result = Py_BuildValue( "OiiiOOOiiO", tag, f->pixelsize, f->num_glyphs, f->symbolfont,
			    index, tab, bitmapz, f->bw, f->bh, f->name );

    Py_DECREF( bitmapz );
    Py_DECREF( index );
    Py_DECREF( tab );

    return result;
}

static int load_font_from_tuple( FontObject* f, PyObject* tuple )
{
#if 0
    static PyObject* zlibmodule;
#endif
    PyObject* tag;
    PyObject* bitmap;
    PyObject* bitmapz;
    PyObject* index;
    PyObject* tab;
    int i, j;
    int tabused;
    
    if ( !PyArg_ParseTuple( tuple, "SiiiO!O!SiiS", &tag,
			    &(f->pixelsize), &(f->num_glyphs), &(f->symbolfont),
			    &PyTuple_Type, &index,
			    &PyTuple_Type, &tab,
			    &bitmapz,
			    &(f->bw), &(f->bh),
			    &(f->name) ) )
	return -1;

#if 0
    if ( zlibmodule == NULL )
    {
	zlibmodule = PyImport_ImportModule( "zlib" );
	if ( zlibmodule == NULL )
	    return -1;
    }
#endif

    if ( strncmp(PyString_AsString(tag), SLITHY_FONT_TAG, strlen(SLITHY_FONT_TAG)) )
    {
	PyErr_SetString( PyExc_ValueError, "font tuple has incorrect tag" );
	return -1;
    }

    f->char_index_size = PyTuple_Size( index ) / 2;
    f->char_index = malloc( f->char_index_size * 2 * sizeof(unsigned int) );
    for ( i = 0; i < f->char_index_size * 2; ++i )
	f->char_index[i] = PyInt_AsLong( PyTuple_GetItem( index, i ) );

    f->tab = malloc( f->num_glyphs * sizeof(texture_character*) );
    for ( i = 0; i < f->num_glyphs; ++i )
	f->tab[i] = NULL;

    tabused = PyTuple_Size( tab ) / 11;
    for ( i = 0; i < tabused; ++i )
    {
	j = PyInt_AsLong( PyTuple_GetItem( tab, i*11 ) );
	f->tab[j] = malloc( sizeof( texture_character ) );
	f->tab[j]->tleft    = PyFloat_AsDouble( PyTuple_GetItem( tab, i*11+1 ) );
	f->tab[j]->tright  = PyFloat_AsDouble( PyTuple_GetItem( tab, i*11+2 ) );
	f->tab[j]->tbottom = PyFloat_AsDouble( PyTuple_GetItem( tab, i*11+3 ) );
	f->tab[j]->ttop    = PyFloat_AsDouble( PyTuple_GetItem( tab, i*11+4 ) );
	f->tab[j]->width   = PyFloat_AsDouble( PyTuple_GetItem( tab, i*11+5 ) );
	f->tab[j]->height  = PyFloat_AsDouble( PyTuple_GetItem( tab, i*11+6 ) );
	f->tab[j]->awidth  = PyFloat_AsDouble( PyTuple_GetItem( tab, i*11+7 ) );
	f->tab[j]->aheight = PyFloat_AsDouble( PyTuple_GetItem( tab, i*11+8 ) );
	f->tab[j]->offsetx = PyFloat_AsDouble( PyTuple_GetItem( tab, i*11+9 ) );
	f->tab[j]->offsety = PyFloat_AsDouble( PyTuple_GetItem( tab, i*11+10 ) );
    }
    
    Py_INCREF( bitmapz );
    //bitmap = PyObject_CallMethod( zlibmodule, "decompress", "O", bitmapz );
    bitmap = PyObject_CallMethod( bitmapz, "decode", "s", "zlib" );
    Py_DECREF( bitmapz );
    
    if ( bitmap == NULL )
    {
	//PyErr_SetString( PyExc_ValueError, "bitmap failed to decompress" );
	return -1;
    }
    f->bitmap = malloc( f->bw * f->bh );
    memcpy( f->bitmap, PyString_AsString( bitmap ), f->bw * f->bh );
    Py_DECREF( bitmap );

    f->has_texture = 0;
    f->filename = NULL;
    Py_INCREF( f->name );
    
    return 0;
}

    

    
	


    
    

/** Local Variables: **/
/** dsp-name:"slithy" **/
/** End: **/
