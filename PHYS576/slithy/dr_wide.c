#include "sl_common.h"
#include "dr_draw.h"

#define S_CLOSE  0
#define S_MOVE   1
#define S_LINE   2
#define S_CURVE  3
#define S_QCURVE 4

#define VERTEX_ALLOC_CHUNK  256
#define BLOCK_ALLOC_CHUNK   16

#define EPSILON  0.00001
#define FLATNESS 0.001

PyObject* draw_path_wide_stroke( PyObject* self, PyObject* args )
{
    PyObject* pobj;
    PyObject* plist = NULL;
    PyObject* cacheobj = NULL;
    int i;
    int size;
    PyObject* item;

    int displaylist = -1;
    
    int cset = 0;    // is there a current point?
    int drawn = 0;
    double cx, cy;
    double tx, ty;
    double sx, sy;
    double queue[3][4], start[4];
    int qpos = 0;
    
    double c[8];
    int j;
    double width;

    if ( !PyArg_ParseTuple( args, "Od", &pobj, &width ) )
	return NULL;

    delete_invalid_lists( pobj );

    cacheobj = PyObject_GetAttrString( pobj, "_widestroke_list" );
    if ( cacheobj )
    {
	if ( PyTuple_Check( cacheobj ) &&
	     PyTuple_Size( cacheobj ) == 2 )
	{
	    displaylist = PyInt_AsLong( PyTuple_GetItem( cacheobj, 0 ) );
	    if ( PyFloat_AsDouble( PyTuple_GetItem( cacheobj, 1 ) ) == width )
	    {
		glCallList( displaylist );
	    }
	    else
	    {
		// cached object is for a different stroke width
		// reuse the same display list number

		cacheobj = NULL;
	    }
	}
	else
	    cacheobj = NULL;
    }

    if ( cacheobj == NULL )
    {
	PyErr_Clear();  // don't care if attribute was not found
	
	plist = PyObject_GetAttrString( pobj, "raw" );
	if ( plist == NULL || !PyList_Check( plist ) )
	{
	    Py_XDECREF( cacheobj );
	    Py_XDECREF( plist );
	    PyErr_SetString( DrawError, "bad path object" );
	    return NULL;
	}

	if ( displaylist == -1 )
	    displaylist = glGenLists( 1 );
	
	PyObject_SetAttrString( pobj, "_widestroke_list", Py_BuildValue( "id", displaylist, width ) );
	glNewList( displaylist, GL_COMPILE_AND_EXECUTE );
	
	// first, convert the path to a list of points and segments
	
	size = PyList_Size( plist );
	
	for ( i = 0; i < size; ++i )
	{
	    item = PyList_GetItem( plist, i );
	    
	    switch( PyInt_AsLong( PyList_GetItem( item, 0 ) ) )
	    {
		case S_CLOSE:
		    if ( fabs(cx-sx) > EPSILON || fabs(cy-sy) > EPSILON )
		    {
			wide_line_segment( width, cx, cy, sx, sy, queue[(qpos+1)%3], queue[qpos] );
			
			if ( drawn )
			    wide_join( queue[(qpos+2)%3], queue[(qpos+1)%3] );
			else
			    memcpy( start, queue[(qpos+1)%3], 4 * sizeof( double ) );
			qpos = (qpos+1) % 3;
		    }

		    if ( drawn )
			wide_join( queue[(qpos+2)%3], start );
		    
		    cset = 0;
		    break;
		    
		case S_MOVE:
		    sx = cx = PyFloat_AsDouble( PyList_GetItem( item, 1 ) );
		    sy = cy = PyFloat_AsDouble( PyList_GetItem( item, 2 ) );
		    
		    cset = 1;
		    drawn = 0;
		    break;
		    
		case S_LINE:
		    tx = PyFloat_AsDouble( PyList_GetItem( item, 1 ) );
		    ty = PyFloat_AsDouble( PyList_GetItem( item, 2 ) );
		    if ( fabs(tx-cx) < EPSILON && fabs(ty-cy) < EPSILON )
			break;

		    wide_line_segment( width, cx, cy, tx, ty, queue[(qpos+1)%3], queue[qpos] );

		    if ( drawn )
			wide_join( queue[(qpos+2)%3], queue[(qpos+1)%3] );
		    else
		    {
			memcpy( start, queue[(qpos+1)%3], 4 * sizeof( double ) );
			drawn = 1;
		    }
		    qpos = (qpos+1) % 3;
		    
		    
		    cx = tx;
		    cy = ty;
		
		    break;

		case S_CURVE:
		    c[0] = cx;
		    c[1] = cy;
		    for ( j = 0; j < 6; ++j )
			c[j+2] = PyFloat_AsDouble( PyList_GetItem( item, j+1 ) );
		    cx = c[6];
		    cy = c[7];

		    wide_curve_segment( width, 4, c, queue[(qpos+1)%3], queue[qpos] );
		
		    if ( drawn )
			wide_join( queue[(qpos+2)%3], queue[(qpos+1)%3] );
		    else
		    {
			memcpy( start, queue[(qpos+1)%3], 4 * sizeof( double ) );
			drawn = 1;
		    }
		    qpos = (qpos+1) % 3;
		    
		    break;

		case S_QCURVE:
		    c[0] = cx;
		    c[1] = cy;
		    for ( j = 0; j < 4; ++j )
			c[j+2] = PyFloat_AsDouble( PyList_GetItem( item, j+1 ) );
		    cx = c[4];
		    cy = c[5];

		    wide_curve_segment( width, 3, c, queue[(qpos+1)%3], queue[qpos] );
		
		    if ( drawn )
			wide_join( queue[(qpos+2)%3], queue[(qpos+1)%3] );
		    else
		    {
			memcpy( start, queue[(qpos+1)%3], 4 * sizeof( double ) );
			drawn = 1;
		    }
		    qpos = (qpos+1) % 3;
		    
		    break;
	    }
	}


	glEndList();
    }

    Py_XDECREF( plist );
    Py_XDECREF( cacheobj );

    Py_INCREF( Py_None );
    return Py_None;
}

void wide_join( double p[4], double q[4] )
{
    double t, a, b, c, d, x, y;

    x = p[0];
    y = p[1];
    a = q[2];
    b = q[3];
    c = p[2];
    d = p[3];

    t = (c*c + d*d - c*a - d*b) / (a*d - b*c);

    if ( fabs(t) < 20.0 )
    {
	glBegin( GL_TRIANGLE_STRIP );
	glVertex2d( x + a - t*b, y + b + t*a );
	glVertex2d( x + a, y + b );
	glVertex2d( x + c, y + d );
	glVertex2d( x, y );
	glEnd();

	glBegin( GL_TRIANGLE_STRIP );
	glVertex2d( x - a + t*b, y - b - t*a );
	glVertex2d( x - a, y - b );
	glVertex2d( x - c, y - d );
	glVertex2d( x, y );
	glEnd();
    }
    else
    {
	glBegin( GL_TRIANGLES );
	glVertex2d( x + a, y + b );
	glVertex2d( x + c, y + d );
	glVertex2d( x, y );
	
	glVertex2d( x - a, y - b );
	glVertex2d( x - c, y - d );
	glVertex2d( x, y );
	glEnd();
    }
	
    
}

void wide_line_segment( double width, double x1, double y1, double x2, double y2, double first[], double last[] )
{
    double tx, ty, px, py, norm;

    tx = x2 - x1;
    ty = y2 - y1;
    
    px = -ty;
    py = tx;

    norm = sqrt(px*px + py*py) / (width/2.0);
    px /= norm;
    py /= norm;

    glBegin( GL_TRIANGLE_STRIP );
    glVertex2d( x1 + px, y1 + py );
    glVertex2d( x1 - px, y1 - py );
    glVertex2d( x2 + px, y2 + py );
    glVertex2d( x2 - px, y2 - py );
    glEnd();

    first[0] = x1;
    first[1] = y1;
    first[2] = px;
    first[3] = py;
    last[0] = x2;
    last[1] = y2;
    last[2] = px;
    last[3] = py;
}

void wide_curve_segment( double width, int order, double c[], double first[], double last[] )
{
    double* xy;
    int i, k;
    double norm;
    double px, py;
    
    xy = bezier_points( &k, order, c, FLATNESS, 1 );

#if 0
    printf( "---\n" );
    for ( i = 0; i < k; ++i )
	printf( "%.4f  %.4f   %.4f  %.4f\n", xy[i*4+0],  xy[i*4+1], xy[i*4+2], xy[i*4+3] );
#endif

    glBegin( GL_TRIANGLE_STRIP );
    for ( i = 0; i < k; ++i )
    {
	norm = (width / 2.0) / sqrt( xy[i*4+2] * xy[i*4+2] + xy[i*4+3] * xy[i*4+3] );
	px = -xy[i*4+3] * norm;
	py = xy[i*4+2] * norm;
	glVertex2d( xy[i*4+0] + px, xy[i*4+1] + py );
	glVertex2d( xy[i*4+0] - px, xy[i*4+1] - py );
	if ( i == 0 )
	{
	    first[0] = xy[0];
	    first[1] = xy[1];
	    first[2] = px;
	    first[3] = py;
	}
	else if ( i == k-1 )
	{
	    last[0] = xy[i*4+0];
	    last[1] = xy[i*4+1];
	    last[2] = px;
	    last[3] = py;
	}
    }
    glEnd();
}






/** Local Variables: **/
/** dsp-name:"draw" **/
/** End: **/
