#include "sl_common.h"
#include "dr_draw.h"

static PyObject* marklist = NULL;

int current_IPM_dirty = 1;
double current_IPM[16];

static int invert_matrix( const double src[16], double inverse[16] );
static void update_current_IPM( void );

FUNC_PY( save_marked_cameras )
{
    PyObject* save;

    if ( !PyArg_ParseTuple( args, "O", &save ) )
	return NULL;

    Py_XDECREF( marklist );
    if ( PyObject_IsTrue( save ) )
    {
	int* v;

	marklist = PyList_New( 1 );

	v = malloc( 4 * sizeof( int ) );
	memcpy( v, viewport, 4 * sizeof( int ) );
	PyList_SetItem( marklist, 0, PyCObject_FromVoidPtr( (void*)v, free ) );
	
	Py_INCREF( marklist );
	return marklist;
    }
    else
    {
	marklist = NULL;

	Py_INCREF( Py_None );
	return Py_None;
    }
    
}

FUNC_PY( mark )
{
    PyObject* name = NULL;
    PyObject* result;
    PyObject* temp;
    PyObject* temp2;
    double *v;

    if ( !PyArg_ParseTuple( args, "|O!", &PyString_Type, &name ) )
    {
	printf( "mark failed: bad args\n" );
	return NULL;
    }

    if ( (temp = internal_mark( name )) == NULL )
    {
	PyErr_SetString( PyExc_ValueError, "can't mark degenerate coordinate system" );
	return NULL;
    }

    v = malloc( 4 * sizeof( int ) );
    memcpy( v, viewport, 4 * sizeof( int ) );
    temp2 = PyCObject_FromVoidPtr( (void*)v, free );

    result = Py_BuildValue( "OO", temp, temp2 );
    Py_DECREF( temp );
    Py_DECREF( temp2 );
    
    return result;
}

PyObject* internal_mark( PyObject* name )
{
    PyObject* item;
    PyObject* temp;
    double* IPM;

    if ( current_IPM_dirty )
	update_current_IPM();

    IPM = malloc( 16 * sizeof( double ) );
    memcpy( IPM, current_IPM, 16 * sizeof( double ) );

    item = PyCObject_FromVoidPtr( (void*)IPM, free );
    
    if ( marklist && name )
    {
	temp = Py_BuildValue( "OO", name, item );
	PyList_Append( marklist, temp );
	Py_DECREF( temp );
    }

    return item;
}

static void update_current_IPM( void )
{
    double proj[16];
    double mv[16];
    double PM[16];
    int i, j;
    
    glGetDoublev( GL_MODELVIEW_MATRIX, mv );
    glGetDoublev( GL_PROJECTION_MATRIX, proj );

    for ( i = 0; i < 4; ++i )
	for ( j = 0; j < 4; ++j )
	    PM[i*4+j] =
		proj[j] * mv[i*4] +
		proj[j+4] * mv[i*4+1] +
		proj[j+8] * mv[i*4+2] +
		proj[j+12] * mv[i*4+3];

    if ( invert_matrix( PM, current_IPM ) )
    {
	for ( i = 0; i < 16; ++i )
	    current_IPM[i] = (i%5==0);
    }

    current_IPM_dirty = 0;
}

FUNC_PY( unproject )
{
    double x, y;
    double nx, ny;
    double ox, oy;
    PyObject* vobj;
    PyObject* iobj;
    int* v;
    double* IPM;

    if ( !PyArg_ParseTuple( args, "ddO!O!", &x, &y, &PyCObject_Type, &vobj, &PyCObject_Type, &iobj ) )
	return NULL;

    v = (int*)PyCObject_AsVoidPtr( vobj );
    IPM = (double*)PyCObject_AsVoidPtr( iobj );

    nx = 2 * (x - v[0]) / v[2] - 1.0;
    ny = 2 * (y - v[1]) / v[3] - 1.0;

    ox = IPM[0] * nx + IPM[4] * ny + IPM[12];
    oy = IPM[1] * nx + IPM[5] * ny + IPM[13];

    return Py_BuildValue( "dd", ox, oy );
}

FUNC_PY( draw_unproject )
{
    double x, y;
    double nx, ny;
    double ox, oy;

    if ( PyTuple_Size( args ) == 2 )
    {
	if ( !PyArg_ParseTuple( args, "dd", &x, &y ) )
	    return NULL;
	
	if ( current_IPM_dirty )
	    update_current_IPM();
	
	nx = 2 * (x - viewport[0]) / viewport[2] - 1.0;
	ny = 2 * (y - viewport[1]) / viewport[3] - 1.0;
	
	ox = current_IPM[0] * nx + current_IPM[4] * ny + current_IPM[12];
	oy = current_IPM[1] * nx + current_IPM[5] * ny + current_IPM[13];
	
	return Py_BuildValue( "dd", ox, oy );
    }
    else
    {
	PyObject* vobj;
	PyObject* ipmobj;
	int* v;
	double* ipm;
	
	if ( !PyArg_ParseTuple( args, "dd(OO)", &x, &y, &ipmobj, &vobj ) )
	    return NULL;

	v = PyCObject_AsVoidPtr( vobj );
	ipm = PyCObject_AsVoidPtr( ipmobj );

	nx = 2 * (x - v[0]) / v[2] - 1.0;
	ny = 2 * (y - v[1]) / v[3] - 1.0;
	
	ox = ipm[0] * nx + ipm[4] * ny + ipm[12];
	oy = ipm[1] * nx + ipm[5] * ny + ipm[13];
	
	return Py_BuildValue( "dd", ox, oy );
    }
}

FUNC_PY( draw_project )
{
    double x, y;
    double tx, ty;
    double MV[16];
    double P[16];
    GLint V[4];

    if ( !PyArg_ParseTuple( args, "dd", &x, &y ) )
	return NULL;

    glGetDoublev( GL_MODELVIEW_MATRIX, MV );
    glGetDoublev( GL_PROJECTION_MATRIX, P );
    glGetIntegerv( GL_VIEWPORT, V );

    tx = MV[0] * x + MV[4] * y + MV[12];
    ty = MV[1] * x + MV[5] * y + MV[13];

    x = P[0] * tx + P[4] * ty + P[12];
    y = P[1] * tx + P[5] * ty + P[13];

    tx = V[0] + V[2] * (x + 1.0) / 2.0;
    ty = V[1] + V[3] * (y + 1.0) / 2.0;

    return Py_BuildValue( "dd", tx, ty );
}

static int invert_matrix( const double src[16], double inverse[16] )
{
    int i, j, k, swap;
    double t;
    double temp[4][4];
    
    for ( i = 0; i < 4; ++i )
	for ( j = 0; j < 4; ++j )
	    temp[i][j] = src[i*4+j];
    for ( i = 0; i < 16; ++i )
	inverse[i] = ((i%5)==0);
    
    for ( i = 0; i < 4; ++i)
    {
	// look for largest element in column 
	swap = i;
	for ( j = i + 1; j < 4; j++ ) 
	    if ( fabs(temp[j][i]) > fabs(temp[i][i]) )
		swap = j;
	
	if ( swap != i )
	{
	    // swap rows
	    for ( k = 0; k < 4; k++ )
	    {
		t = temp[i][k];
		temp[i][k] = temp[swap][k];
		temp[swap][k] = t;
		
		t = inverse[i * 4 + k];
		inverse[i * 4 + k] = inverse[swap * 4 + k];
		inverse[swap * 4 + k] = t;
	    }
	}
	if ( temp[i][i] == 0 )
	{
	    // No non-zero pivot.  The matrix is singular, which
	    // shouldn't happen.  This means the user gave us a bad
	    // matrix.  Bad, bad user.
	    return -1;
	}
	
	t = temp[i][i];
	for ( k = 0; k < 4; ++k )
	{
	    temp[i][k] /= t;
	    inverse[i * 4 + k] /= t;
	}
	
	for ( j = 0; j < 4; j++ )
	{
	    if (j != i)
	    {
		t = temp[j][i];
		for ( k = 0; k < 4; ++k )
		{
		    temp[j][k] -= temp[i][k] * t;
		    inverse[j * 4 + k] -= inverse[i * 4 + k] * t;
		}
	    }
	}
    }
    return 0;
}

/** Local Variables: **/
/** dsp-name:"draw" **/
/** End: **/
