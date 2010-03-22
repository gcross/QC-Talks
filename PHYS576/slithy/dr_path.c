#include "sl_common.h"
#include "dr_draw.h"

#define S_CLOSE  0
#define S_MOVE   1
#define S_LINE   2
#define S_CURVE  3
#define S_QCURVE 4

#define VERTEX_ALLOC_CHUNK  256
#define BLOCK_ALLOC_CHUNK   16

/*#define SHOW_TRIANGULATION*/

static void
#ifdef WIN32
CALLBACK
#endif
combine_cb( GLdouble coords[3], void* vertex_data[4],
		 GLfloat weight[4], GLdouble** dataout );

static void draw_arrow( double x, double y, double tx, double ty,
			int style, double width, double length );

PyObject* draw_path_stroke( PyObject* self, PyObject* args )
{
    PyObject* pobj;
    PyObject* plist = NULL;
    PyObject* cacheobj = NULL;
    int i;
    int size;
    PyObject* item;

    int displaylist;
    
    int cset = 0;    // is there a current point?
    double sx, sy;   // start of subpath
    double cx, cy;   // current point
    
    double c[8];
    int j, k;
    double *xy;
    
    if ( !PyArg_ParseTuple( args, "O", &pobj ) )
	return NULL;

    delete_invalid_lists( pobj );

    cacheobj = PyObject_GetAttrString( pobj, "_stroke_list" );
    if ( cacheobj && PyInt_Check( cacheobj ) )
    {
	displaylist = PyInt_AsLong( cacheobj );
	glCallList( displaylist );
    }
    else
    {
	PyErr_Clear();  // don't care if attribute was not found
	
	plist = PyObject_GetAttrString( pobj, "raw" );
	if ( plist == NULL || !PyList_Check( plist ) )
	{
	    Py_XDECREF( plist );
	    PyErr_SetString( DrawError, "bad path object" );
	    return NULL;
	}

	displaylist = glGenLists( 1 );
	glNewList( displaylist, GL_COMPILE_AND_EXECUTE );
	
	size = PyList_Size( plist );
	
	for ( i = 0; i < size; ++i )
	{
	    item = PyList_GetItem( plist, i );
	    
	    switch( PyInt_AsLong( PyList_GetItem( item, 0 ) ) )
	    {
	      case S_CLOSE:
		glVertex2d( sx, sy );
		glEnd();
		cset = 0;
		break;

	      case S_MOVE:
		if ( cset ) glEnd();
		glBegin( GL_LINE_STRIP );
		sx = cx = PyFloat_AsDouble( PyList_GetItem( item, 1 ) );
		sy = cy = PyFloat_AsDouble( PyList_GetItem( item, 2 ) );
		glVertex2d( cx, cy );
		cset = 1;
		break;

	      case S_LINE:
		cx = PyFloat_AsDouble( PyList_GetItem( item, 1 ) );
		cy = PyFloat_AsDouble( PyList_GetItem( item, 2 ) );
		glVertex2d( cx, cy );
		break;

	      case S_CURVE:
		c[0] = cx;
		c[1] = cy;
		for ( j = 0; j < 6; ++j )
		    c[j+2] = PyFloat_AsDouble( PyList_GetItem( item, j+1 ) );
		cx = c[6];
		cy = c[7];
	    
		xy = bezier_points( &k, 4, c, 0.001, 0 );
		for ( j = 1; j < k; ++j )
		    glVertex2d( xy[j*2], xy[j*2+1] );
		break;

	      case S_QCURVE:
		c[0] = cx;
		c[1] = cy;
		for ( j = 0; j < 4; ++j )
		    c[j+2] = PyFloat_AsDouble( PyList_GetItem( item, j+1 ) );
		cx = c[4];
		cy = c[5];

		xy = bezier_points( &k, 3, c, 0.001, 0 );
		for ( j = 1; j < k; ++j )
		    glVertex2d( xy[j*2], xy[j*2+1] );
		break;
	    }
	}

	if ( cset )
	    glEnd();

	glEndList();

	PyObject_SetAttrString( pobj, "_stroke_list", PyInt_FromLong( displaylist ) );
    }

    Py_XDECREF( plist );
    Py_XDECREF( cacheobj );

    Py_INCREF( Py_None );
    return Py_None;
}

#ifdef SHOW_TRIANGULATION
void random_color_vertex( const GLdouble* v )
{
    glColor3d( drand48(), drand48(), drand48() );
    glVertex2dv( v );
}
#endif

PyObject* draw_path_fill( PyObject* self, PyObject* args )
{
    PyObject* pobj;
    PyObject* plist = NULL;
    
    PyObject* cacheobj;
    int displaylist;
    
    int i;
    int size;
    PyObject* item;
    static GLUtesselator* tess = NULL;
    static double* v = NULL;
    static int alloc = 0;
    int used = 0;
    
    int cset = 0;    // is there a current point?
    double sx, sy;   // start of subpath
    double cx, cy;   // current point

    double c[8];
    int j, k;
    double *xy;
    static double** blocks = NULL;
    static int block_alloc = 0;
    int block_used = 0;

    if ( !PyArg_ParseTuple( args, "O", &pobj ) )
	return NULL;

    delete_invalid_lists( pobj );
    
    cacheobj = PyObject_GetAttrString( pobj, "_fill_list" );
    if ( cacheobj && PyInt_Check(cacheobj) )
    {
	displaylist = PyInt_AsLong( cacheobj );
	glCallList( displaylist );
    }
    else
    {
	PyErr_Clear();  // don't care if attribute was not found
	
	// generate and store triangulation
	
	plist = PyObject_GetAttrString( pobj, "raw" );
	if ( plist == NULL || !PyList_Check( plist ) )
	{
	    Py_XDECREF( plist );
	    PyErr_SetString( DrawError, "bad path object" );
	    return NULL;
	}

	displaylist = glGenLists( 1 );
	glNewList( displaylist, GL_COMPILE_AND_EXECUTE );
    
	if ( tess == NULL )
	    tess = gluNewTess();
	
#ifdef SHOW_TRIANGULATION
	glPushAttrib( GL_LIGHTING_BIT );
	glShadeModel( GL_FLAT );
	gluTessCallback( tess, GLU_TESS_VERTEX, random_color_vertex );
#else
	gluTessCallback( tess, GLU_TESS_VERTEX, glVertex2dv );
#endif
	
	gluTessCallback( tess, GLU_TESS_BEGIN, glBegin );
	gluTessCallback( tess, GLU_TESS_END, glEnd );
	gluTessCallback( tess, GLU_TESS_COMBINE, combine_cb );
	gluTessProperty( tess, GLU_TESS_BOUNDARY_ONLY, GL_FALSE );
	
	size = PyList_Size( plist );
	if ( size > alloc )
	{
	    alloc = size + VERTEX_ALLOC_CHUNK;
	    v = realloc( v, alloc * 3 * sizeof( double ) );
	}
	memset( v, 0, alloc * 3 * sizeof( double ) );
	
#define VERTEX(x,y)  {v[used*3+0]=(x);v[used*3+1]=(y);gluTessVertex(tess,v+used*3,v+used*3);++used;}
	
	gluTessBeginPolygon( tess, NULL );
	for ( i = 0; i < size; ++i )
	{
	    item = PyList_GetItem( plist, i );
	    
	    switch( PyInt_AsLong( PyList_GetItem( item, 0 ) ) )
	    {
	      case S_CLOSE:
		VERTEX( sx, sy );
		gluTessEndContour( tess );
		cset = 0;
		break;
		
	      case S_MOVE:
		if ( cset ) gluTessEndContour( tess );
		gluTessBeginContour( tess );
		sx = cx = PyFloat_AsDouble( PyList_GetItem( item, 1 ) );
		sy = cy = PyFloat_AsDouble( PyList_GetItem( item, 2 ) );
		VERTEX( cx, cy );
		cset = 1;
		break;
		
	      case S_LINE:
		cx = PyFloat_AsDouble( PyList_GetItem( item, 1 ) );
		cy = PyFloat_AsDouble( PyList_GetItem( item, 2 ) );
		VERTEX( cx, cy );
		break;
		
	      case S_CURVE:
		if ( block_used + 1 > block_alloc )
		{
		    block_alloc += BLOCK_ALLOC_CHUNK;
		    blocks = realloc( blocks, block_alloc * sizeof( double* ) );
		}
		
		c[0] = cx;
		c[1] = cy;
		for ( j = 0; j < 6; ++j )
		    c[j+2] = PyFloat_AsDouble( PyList_GetItem( item, j+1 ) );
		xy = bezier_points( &k, 4, c, 0.0001, 0 );
		cx = c[6];
		cy = c[7];
		
		blocks[block_used] = malloc( (k-1) * 3 * sizeof( double ) );
		for ( j = 1; j < k; ++j )
		{
		    blocks[block_used][(j-1)*3+0] = xy[j*2];
		    blocks[block_used][(j-1)*3+1] = xy[j*2+1];
		    blocks[block_used][(j-1)*3+2] = 0.0;
		}
		
		for ( j = 0; j < k-1; ++j )
		    gluTessVertex( tess, blocks[block_used] + j*3, blocks[block_used] + j*3 );
		
		++block_used;
		break;
		
	      case S_QCURVE:
		if ( block_used + 1 > block_alloc )
		{
		    block_alloc += BLOCK_ALLOC_CHUNK;
		    blocks = realloc( blocks, block_alloc * sizeof( double* ) );
		}

		c[0] = cx;
		c[1] = cy;
		for ( j = 0; j < 4; ++j )
		    c[j+2] = PyFloat_AsDouble( PyList_GetItem( item, j+1 ) );
		xy = bezier_points( &k, 3, c, 0.0001, 0 );
		cx = c[4];
		cy = c[5];

		blocks[block_used] = malloc( (k-1) * 3 * sizeof( double ) );
		for ( j = 1; j < k; ++j )
		{
		    blocks[block_used][(j-1)*3+0] = xy[j*2];
		    blocks[block_used][(j-1)*3+1] = xy[j*2+1];
		    blocks[block_used][(j-1)*3+2] = 0.0;
		}
	    
		for ( j = 0; j < k-1; ++j )
		    gluTessVertex( tess, blocks[block_used] + j*3, blocks[block_used] + j*3 );

		++block_used;
		break;
	    }
	}

	if ( cset )
	    gluTessEndContour( tess );
	gluTessEndPolygon( tess );

	for ( i = 0; i < block_used; ++i )
	    free( blocks[i] );
	
#ifdef SHOW_TRIANGULATION
	glPopAttrib();
#endif

	glEndList();

	PyObject_SetAttrString( pobj, "_fill_list", PyInt_FromLong( displaylist ) );
    }

    Py_XDECREF( cacheobj );
    Py_XDECREF( plist );

    Py_INCREF( Py_None );
    return Py_None;
}

static void
#ifdef WIN32
CALLBACK
#endif
combine_cb( GLdouble coords[3], void* vertex_data[4], GLfloat weight[4], GLdouble** dataout )
{
    GLdouble* vertex;

    vertex = (GLdouble*)malloc( 2 * sizeof( GLdouble ) );
    vertex[0] = coords[0];
    vertex[1] = coords[1];
    *dataout = vertex;
}

void delete_invalid_lists( PyObject* pobj )
{
    PyObject* pclass = NULL;
    PyObject* plist = NULL;
    PyObject* temp;
    int i, d, n;

    pclass = PyObject_GetAttrString( pobj, "__class__" );
    if ( pclass == NULL )
	goto done;

    plist = PyObject_GetAttrString( pclass, "invalid" );
    if ( plist == NULL )
	goto done;

    if ( !PyList_Check( plist ) ) goto done;

    n = PyList_Size( plist );
    if ( n == 0 ) goto done;
#if 0
    printf( "deleting old displaylists: " );
#endif
    for ( i = 0; i < n; ++i )
    {
	temp = PyList_GetItem( plist, i );
	if ( PyInt_Check( temp ) ) {
	    d = PyInt_AsLong( temp );
            glDeleteLists( d, 1 );
        } else if ( PyTuple_Check( temp ) ) {
	    d = PyInt_AsLong( PyTuple_GetItem( temp, 0 ) );
            glDeleteLists( d, 1 );
	} else { 
	    fprintf( stderr, "delete_invalid_lists:  bad cache value\n" );
        }
#if 0
	printf( " %d", d );
#endif
    }
#if 0
    printf( "\n" );
#endif

    PyObject_SetAttrString( pclass, "invalid", PyList_New(0) );
    
 done:
    Py_XDECREF( pclass );
    Py_XDECREF( plist );
}


PyObject* compute_path_bbox( PyObject* self, PyObject* args )
{
    PyObject* pobj;
    PyObject* plist = NULL;
    PyObject* cacheobj = NULL;
    int i;
    int size;
    PyObject* item;

    int cset = 0;    // is there a current point?
    double sx, sy;   // start of subpath
    double cx, cy;   // current point
    
    double c[8];
    int j, k;
    double *xy;

    int empty = 1;
    double minx, maxx, miny, maxy;

    if ( !PyArg_ParseTuple( args, "O", &pobj ) )
	return NULL;

    cacheobj = PyObject_GetAttrString( pobj, "_bbox" );
    if ( cacheobj && cacheobj != Py_None )
    {
	Py_INCREF( cacheobj );
    }
    else
    {
	plist = PyObject_GetAttrString( pobj, "raw" );
	if ( plist == NULL || !PyList_Check( plist ) )
	{
	    Py_XDECREF( plist );
	    PyErr_SetString( DrawError, "bad path object" );
	    return NULL;
	}

	size = PyList_Size( plist );
	
	for ( i = 0; i < size; ++i )
	{
	    item = PyList_GetItem( plist, i );

#define ADDPOINT(x,y)  {if(empty){empty=0;minx=maxx=(x);miny=maxy=(y);}else{ if((x)<minx)minx=(x);else if((x)>maxx)maxx=(x); if((y)<miny)miny=(y);else if((y)>maxy)maxy=(y);}}

	    
	    switch( PyInt_AsLong( PyList_GetItem( item, 0 ) ) )
	    {
	      case S_CLOSE:
		ADDPOINT( sx, sy );
		cset = 0;
		break;

	      case S_MOVE:
		sx = cx = PyFloat_AsDouble( PyList_GetItem( item, 1 ) );
		sy = cy = PyFloat_AsDouble( PyList_GetItem( item, 2 ) );
		cset = 1;
		break;

	      case S_LINE:
		ADDPOINT( cx, cy );
		cx = PyFloat_AsDouble( PyList_GetItem( item, 1 ) );
		cy = PyFloat_AsDouble( PyList_GetItem( item, 2 ) );
		ADDPOINT( cx, cy );
		break;

	      case S_CURVE:
		c[0] = cx;
		c[1] = cy;
		for ( j = 0; j < 6; ++j )
		    c[j+2] = PyFloat_AsDouble( PyList_GetItem( item, j+1 ) );
		cx = c[6];
		cy = c[7];
	    
		xy = bezier_points( &k, 4, c, 0.001, 0 );
		for ( j = 0; j < k; ++j )
		    ADDPOINT( xy[j*2], xy[j*2+1] );
		break;

	      case S_QCURVE:
		c[0] = cx;
		c[1] = cy;
		for ( j = 0; j < 4; ++j )
		    c[j+2] = PyFloat_AsDouble( PyList_GetItem( item, j+1 ) );
		cx = c[4];
		cy = c[5];

		xy = bezier_points( &k, 3, c, 0.001, 0 );
		for ( j = 0; j < k; ++j )
		    ADDPOINT( xy[j*2], xy[j*2+1] );
		break;
	    }
	}

	cacheobj = Py_BuildValue( "dddd", minx, miny, maxx, maxy );
	PyObject_SetAttrString( pobj, "_bbox", cacheobj );
    }

    Py_XDECREF( plist );

    Py_INCREF( Py_None );
    return cacheobj;
}

PyObject* draw_path_arrow( PyObject* self, PyObject* args )
{
    PyObject* pobj;
    PyObject* endarrow = NULL;
    PyObject* startarrow = NULL;
    int endsonly = 0;
    
    int endstyle, startstyle;
    double endwidth, startwidth;
    double endlength, startlength;
    
    PyObject* plist = NULL;
    int i;
    int size;
    PyObject* item;

    int cset = 0;    // is there a current point?
    double sx, sy, stx, sty;
    double cx, cy;   // current point
    double nx, ny;
    int open = 0, first = 1;
    
    double tx, ty;
    
    
    if ( !PyArg_ParseTuple( args, "OO|Oi", &pobj, &endarrow, &startarrow, &endsonly ) )
	return NULL;

    if ( endarrow == Py_None )
 	endarrow = NULL; 
    
    if ( endarrow )
    {
	if ( !PyTuple_Check( endarrow ) || PyTuple_Size( endarrow ) != 3 )
	{
	    PyErr_SetString( PyExc_ValueError, "bad value for endarrow" );
	    return NULL;
	}
	endstyle = PyInt_AsLong( PyTuple_GetItem( endarrow, 0 ) );
	endwidth = PyFloat_AsDouble( PyTuple_GetItem( endarrow, 1 ) );
	endlength = PyFloat_AsDouble( PyTuple_GetItem( endarrow, 2 ) );
    }
	
    if ( startarrow == Py_None ) 
	startarrow = NULL;
    
    if ( startarrow )
    {
	if ( !PyTuple_Check( startarrow ) || PyTuple_Size( startarrow ) != 3 )
	{
	    PyErr_SetString( PyExc_ValueError, "bad value for startarrow" );
	    return NULL;
	}
	startstyle = PyInt_AsLong( PyTuple_GetItem( startarrow, 0 ) );
	startwidth = PyFloat_AsDouble( PyTuple_GetItem( startarrow, 1 ) );
	startlength = PyFloat_AsDouble( PyTuple_GetItem( startarrow, 2 ) );
    }
    
    delete_invalid_lists( pobj );

    plist = PyObject_GetAttrString( pobj, "raw" );
    if ( plist == NULL || !PyList_Check( plist ) )
    {
	Py_XDECREF( plist );
	PyErr_SetString( DrawError, "bad path object" );
	return NULL;
    }
    
    size = PyList_Size( plist );

    open = 0;
    for ( i = 0; i < size; ++i )
    {
	item = PyList_GetItem( plist, i );
	
	switch( PyInt_AsLong( PyList_GetItem( item, 0 ) ) )
	{
	  case S_CLOSE:
	    open = 0;
	    break;
	    
	  case S_MOVE:
	    if ( open )
	    {
		if ( startarrow && (first || !endsonly) )
		    draw_arrow( sx, sy, stx, sty, startstyle, startwidth, startlength );
		if ( endarrow && !endsonly )
		    draw_arrow( cx, cy, tx, ty, endstyle, endwidth, endlength );
		first = 0;
	    }
	    cx = PyFloat_AsDouble( PyList_GetItem( item, 1 ) );
	    cy = PyFloat_AsDouble( PyList_GetItem( item, 2 ) );
	    open = 0;
	    break;
	    
	  case S_LINE:
	    nx = PyFloat_AsDouble( PyList_GetItem( item, 1 ) );
	    ny = PyFloat_AsDouble( PyList_GetItem( item, 2 ) );
	    tx = nx - cx;
	    ty = ny - cy;
	    if ( !open )
	    {
		sx = cx;
		sy = cy;
		stx = -tx;
		sty = -ty;
	    }
	    cx = nx;
	    cy = ny;
	    open = 1;
	    break;
	    
	  case S_CURVE:
	    if ( !open )
	    {
		sx = cx;
		sy = cy;
		stx = cx - PyFloat_AsDouble( PyList_GetItem( item, 1 ) );
		sty = cy - PyFloat_AsDouble( PyList_GetItem( item, 2 ) );
	    }
		
	    cx = PyFloat_AsDouble( PyList_GetItem( item, 5 ) );
	    cy = PyFloat_AsDouble( PyList_GetItem( item, 6 ) );
	    tx = cx - PyFloat_AsDouble( PyList_GetItem( item, 3 ) );
	    ty = cy - PyFloat_AsDouble( PyList_GetItem( item, 4 ) );
	    open = 1;
	    break;
	    
	  case S_QCURVE:
	    nx = PyFloat_AsDouble( PyList_GetItem( item, 1 ) );
	    ny = PyFloat_AsDouble( PyList_GetItem( item, 2 ) );
	    if ( !open )
	    {
		sx = cx;
		sy = cy;
		stx = cx - nx;
		sty = cy - ny;
	    }
	    cx = PyFloat_AsDouble( PyList_GetItem( item, 3 ) );
	    cy = PyFloat_AsDouble( PyList_GetItem( item, 4 ) );
	    tx = cx - nx;
	    ty = cy - ny;
	    open = 1;
	    break;
	}
    }

    if ( open )
    {
	if ( startarrow && (first || !endsonly) )
	    draw_arrow( sx, sy, stx, sty, startstyle, startwidth, startlength );
	if ( endarrow )
	    draw_arrow( cx, cy, tx, ty, endstyle, endwidth, endlength );
    }
    
    Py_XDECREF( plist );

    Py_INCREF( Py_None );
    return Py_None;
}

static void draw_arrow( double x, double y, double tx, double ty,
			int style, double width, double length )
{
    glPushMatrix();
    glTranslated( x, y, 0.0 );
    glRotated( atan2( ty, tx ) * 180.0 / M_PI, 0, 0, 1 );

    if ( style == 0 )
    {
	glBegin( GL_TRIANGLES );
	glVertex2d( 0, width/2 );
	glVertex2d( length, 0 );
	glVertex2d( 0, -width/2 );
	glEnd();
    }
    else if ( style == 1 )
    {
	glBegin( GL_QUADS );
	glVertex2d( 0, width/2 );
	glVertex2d( 0, -width/2 );
	glVertex2d( length, -width/2 );
	glVertex2d( length, width/2 );
	glEnd();
    }

    glPopMatrix();
}

	
	
	

/** Local Variables: **/
/** dsp-name:"draw" **/
/** End: **/
