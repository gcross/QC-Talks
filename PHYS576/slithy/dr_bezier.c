#include "sl_common.h"
#include "dr_draw.h"

#define ALLOC_CHUNK  512


#define EPSILON .000001

static int flat_enough( double c[], int order, double flatness );
    
static void quadratic_adaptive_recurse( double** out, int* alloc, int* count,
					int order, double c[], double flatness, int tangents );
static void cubic_adaptive_recurse( double** out, int* alloc, int* count,
				    int order, double c[], double flatness, int tangents );
static void general_adaptive_recurse( double** out, int* alloc, int* count,
				      int order, double c[], double flatness, int tangents );

void clip_bezier_points( double c[], int order, double u, int max );


PyObject* draw_bezier( PyObject* self, PyObject* args )
{
    double umin = 0.0, umax = 1.0;
    PyObject* clist;
    PyObject* point;
    static double* c = NULL;
    static int alloc = 0;
    int n;
    double* xy;
    int count;
    int i;

    if ( !PyArg_ParseTuple( args, "O|dd", &clist, &umin, &umax ) )
	return NULL;

    if ( umin >= umax )
	goto done;

    n = PyList_Size( clist );
    if ( n <= 2 )
	goto done;

    if ( alloc < n )
    {
	alloc = n;
	c = realloc( c, alloc * 2 * sizeof( double ) );
    }

    for ( i = 0; i < n; ++i )
    {
	point = PyList_GetItem( clist, i );
	c[i*2+0] = PyFloat_AsDouble( PyTuple_GetItem( point, 0 ) );
	c[i*2+1] = PyFloat_AsDouble( PyTuple_GetItem( point, 1 ) );
    }

    if ( umax != 1.0 )
	clip_bezier_points( c, n, umax, 1 );
    if ( umin != 0.0 )
	clip_bezier_points( c, n, umin / umax, 0 );

    xy = bezier_points( &count, n, c, 0.001, 0 );

#if 1
    glVertexPointer( 2, GL_DOUBLE, 0, xy );
    glDrawArrays( GL_LINE_STRIP, 0, count );
#endif

#if 0
    glBegin( GL_LINE_STRIP );
    for ( i = 0; i < count; ++i )
    {
	printf( "   %.4f  %.4f\n", xy[i*2], xy[i*2+1] );
	glVertex2d( xy[i*2], xy[i*2+1] );
    }
    glEnd();
#endif

 done:
    Py_INCREF( Py_None );
    return Py_None;
}
    
void clip_bezier_points( double c[], int order, double u, int max )
{
    static double* nc = NULL;
    static int alloc = 0;
    int j, k;

    if ( order > alloc )
    {
	alloc = order;
	nc = realloc( nc, alloc * 2 * sizeof( double ) );
    }

    if ( max )
    {
	nc[0] = c[0];
	nc[1] = c[1];
    }
    else
    {
	nc[(order-1)*2] = c[(order-1)*2];
	nc[(order-1)*2+1] = c[(order-1)*2+1];
    }

    for ( j = order-2; j >= 0; --j )
    {
	for ( k = 0; k <= j; ++k )
	{
	    c[k*2+0] = c[k*2+0] * (1.0-u) + c[k*2+2] * u;
	    c[k*2+1] = c[k*2+1] * (1.0-u) + c[k*2+3] * u;
	}
	if ( max )
	{
	    nc[(order-1-j)*2+0] = c[0];
	    nc[(order-1-j)*2+1] = c[1];
	}
	else
	{
	    nc[j*2+0] = c[j*2+0];
	    nc[j*2+1] = c[j*2+1];
	}
    }

    memcpy( c, nc, order * 2 * sizeof( double ) );
}

    

double* bezier_points( int* count, int order, double c[], double flatness, int tangents )
{
    static double* out = NULL;
    static int alloc = 0;
    int i;

    *count = 1;
    while ( *count * (tangents?4:2) > alloc )
    {
	alloc += ALLOC_CHUNK;
	out = realloc( out, alloc * (tangents?4:2) * sizeof( double ) );
    }

    out[0] = c[0];
    out[1] = c[1];
    if ( tangents )
    {
	for ( i = 1; i < order; ++i )
	{
	    out[2] = c[i*2+0] - c[0];
	    out[3] = c[i*2+1] - c[1];
	    if ( fabs(out[2]) > EPSILON || fabs(out[3]) > EPSILON )
		break;
	}
    }

    switch( order )
    {
      case 3:
	quadratic_adaptive_recurse( &out, &alloc, count, order, c, 1.0 + flatness, tangents );
	break;
      case 4:
	cubic_adaptive_recurse( &out, &alloc, count, order, c, 1.0 + flatness, tangents );
	break;
      default:
	general_adaptive_recurse( &out, &alloc, count, order, c, 1.0 + flatness, tangents );
	break;
    }

    return out;
}

static void general_adaptive_recurse( double** out, int* alloc, int* count,
				      int order, double c[], double flatness, int tangents )
{
#if 0
    int i;
    printf( "points:\n" );
    for ( i = 0; i < order; ++i )
	printf( "  %.3f,%.3f", c[i*2], c[i*2+1] );
    printf( "\n" );
#endif
    
    if ( flat_enough( c, order, flatness ) )
    {
	while( (*count + 1) * (tangents?4:2) > *alloc )
	{
	    *alloc += ALLOC_CHUNK;
	    *out = realloc( *out, *alloc * sizeof( double ) );
	}

	if ( tangents )
	{
	    (*out)[*count*4] = c[order*2-2];
	    (*out)[*count*4+1] = c[order*2-1];
	    (*out)[*count*4+2] = c[order*2-2] - c[order*2-4];
	    (*out)[*count*4+3] = c[order*2-1] - c[order*2-3];
	    if ( fabs((*out)[*count*4+2]) < EPSILON &&
		 fabs((*out)[*count*4+3]) < EPSILON )
	    {
		(*out)[*count*4+2] = c[order*2-2] - c[order*2-6];
		(*out)[*count*4+3] = c[order*2-1] - c[order*2-5];
	    }
	}
	else
	{
	    (*out)[*count*2] = c[order*2-2];
	    (*out)[*count*2+1] = c[order*2-1];
	}
	++*count;
    }
    else
    {
	double* left;
	double* right;
	int j, k;

	left = malloc( order * 2 * sizeof( double ) );
	right = malloc( order * 2 * sizeof( double ) );

	left[0] = c[0];
	left[1] = c[1];
	right[order*2-2] = c[order*2-2];
	right[order*2-1] = c[order*2-1];

	for ( j = order-2; j >= 0; --j )
	{
	    for ( k = 0; k <= j; ++k )
	    {
		c[k*2+0] = (c[k*2+0] + c[k*2+2]) * 0.5;
		c[k*2+1] = (c[k*2+1] + c[k*2+3]) * 0.5;
	    }
	    
	    left[(order-1-j)*2+0] = c[0];
	    left[(order-1-j)*2+1] = c[1];
	    right[j*2+0] = c[j*2+0];
	    right[j*2+1] = c[j*2+1];
	}

	general_adaptive_recurse( out, alloc, count, order, left, flatness, tangents );
	general_adaptive_recurse( out, alloc, count, order, right, flatness, tangents );

	free( right );
	free( left );
    }
}

static void quadratic_adaptive_recurse( double** out, int* alloc, int* count,
					int order, double c[6], double flatness, int tangents )
{
    if ( flat_enough( c, 3, flatness ) )
    {
	while( (*count + 1) * (tangents?4:2) > *alloc )
	{
	    *alloc += ALLOC_CHUNK;
	    *out = realloc( *out, *alloc * sizeof( double ) );
	}

	if ( tangents )
	{
	    (*out)[*count*4] = c[4];
	    (*out)[*count*4+1] = c[5];
	    (*out)[*count*4+2] = c[4] - c[2];
	    (*out)[*count*4+3] = c[5] - c[3];
	    if ( fabs((*out)[*count*4+2]) < EPSILON &&
		 fabs((*out)[*count*4+3]) < EPSILON )
	    {
		(*out)[*count*4+2] = c[4] - c[0];
		(*out)[*count*4+3] = c[5] - c[1];
	    }
	}
	else
	{
	    (*out)[*count*2] = c[4];
	    (*out)[*count*2+1] = c[5];
	}
	++*count;
    }
    else
    {
	double half[6];

	half[0] = c[0];
	half[1] = c[1];
	half[2] = (c[0] + c[2]) / 2.0;
	half[3] = (c[1] + c[3]) / 2.0;
	half[4] = (c[0] + c[2]*2 + c[4]) / 4.0;
	half[5] = (c[1] + c[3]*2 + c[5]) / 4.0;
	quadratic_adaptive_recurse( out, alloc, count, 4, half, flatness, tangents );

	half[0] = half[4];
	half[1] = half[5];
	half[2] = (c[2] + c[4]) / 2.0;
	half[3] = (c[3] + c[5]) / 2.0;
	half[4] = c[4];
	half[5] = c[5];
	quadratic_adaptive_recurse( out, alloc, count, 4, half, flatness, tangents );
    }
}

static void cubic_adaptive_recurse( double** out, int* alloc, int* count,
				    int order, double c[8], double flatness, int tangents )
{
    if ( flat_enough( c, 4, flatness ) )
    {
	while( (*count + 1) * (tangents?4:2) > *alloc )
	{
	    *alloc += ALLOC_CHUNK;
	    *out = realloc( *out, *alloc * sizeof( double ) );
	}

	if ( tangents )
	{
	    (*out)[*count*4] = c[6];
	    (*out)[*count*4+1] = c[7];
	    (*out)[*count*4+2] = c[6] - c[4];
	    (*out)[*count*4+3] = c[7] - c[5];
	    if ( fabs((*out)[*count*4+2]) < EPSILON &&
		 fabs((*out)[*count*4+3]) < EPSILON )
	    {
		(*out)[*count*4+2] = c[6] - c[2];
		(*out)[*count*4+3] = c[7] - c[3];
		if ( fabs((*out)[*count*4+2]) < EPSILON &&
		     fabs((*out)[*count*4+3]) < EPSILON )
		{
		    (*out)[*count*4+2] = c[6] - c[0];
		    (*out)[*count*4+3] = c[7] - c[1];
		}
	    }
	}
	else
	{
	    (*out)[*count*2] = c[6];
	    (*out)[*count*2+1] = c[7];
	}
	++*count;
    }
    else
    {
	double half[8];

	half[0] = c[0];
	half[1] = c[1];
	half[2] = (c[0] + c[2]) / 2.0;
	half[3] = (c[1] + c[3]) / 2.0;
	half[4] = (c[0] + c[2]*2.0 + c[4]) / 4.0;
	half[5] = (c[1] + c[3]*2.0 + c[5]) / 4.0;
	half[6] = (c[0] + c[2]*3.0 + c[4]*3.0 + c[6]) / 8.0;
	half[7] = (c[1] + c[3]*3.0 + c[5]*3.0 + c[7]) / 8.0;
	cubic_adaptive_recurse( out, alloc, count, 3, half, flatness, tangents );

	half[0] = half[6];
	half[1] = half[7];
	half[2] = (c[2] + c[4]*2.0 + c[6]) / 4.0;
	half[3] = (c[3] + c[5]*2.0 + c[7]) / 4.0;
	half[4] = (c[4] + c[6]) / 2.0;
	half[5] = (c[5] + c[7]) / 2.0;
	half[6] = c[6];
	half[7] = c[7];
	cubic_adaptive_recurse( out, alloc, count, 3, half, flatness, tangents );
    }
}

static int flat_enough( double c[], int order, double flatness )
{
    double direct, sum = 0.0;
    int i;

    for ( i = 0; i < order-1; ++i )
	sum += sqrt( (c[i*2]-c[i*2+2])*(c[i*2]-c[i*2+2]) + (c[i*2+1]-c[i*2+3])*(c[i*2+1]-c[i*2+3]) );

    direct = sqrt( (c[0]-c[order*2-2])*(c[0]-c[order*2-2]) + (c[1]-c[order*2-1])*(c[1]-c[order*2-1]) );

    return (direct == 0.0) ? (sum == 0.0) : ((sum/direct) < flatness);
}

/** Local Variables: **/
/** dsp-name:"draw" **/
/** End: **/
