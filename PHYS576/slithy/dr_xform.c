#include "sl_common.h"
#include "dr_draw.h"

FUNC_PY( draw_shear )
{
    double sx, sy;
    double M[16] = { 1.0, 0.0, 0.0, 0.0,
		     0.0, 1.0, 0.0, 0.0, 
		     0.0, 0.0, 1.0, 0.0,
		     0.0, 0.0, 0.0, 1.0 };

    if ( !PyArg_ParseTuple( args, "dd", &sx, &sy ) )
	return NULL;

    M[1] = sx;
    M[4] = sy;

    glPopMatrix();
    glMultMatrixd( M );
    glPushMatrix();
    glTranslated( 0, 0, current_id_depth );
    
    current_IPM_dirty = 1;

    Py_INCREF( Py_None );
    return Py_None;
}

    
    

FUNC_PY( draw_translate )
{
    double x, y;

    if ( !PyArg_ParseTuple( args, "dd", &x, &y ) )
	return NULL;

    glPopMatrix();
    glTranslated( x, y, 0.0 );
    glPushMatrix();
    glTranslated( 0, 0, current_id_depth );
    
    current_IPM_dirty = 1;

    Py_INCREF( Py_None );
    return Py_None;
}
	
FUNC_PY( draw_scale )
{
    double x, y;
    double cx, cy;

    switch( PyTuple_Size( args ) )
    {
      case 1:
	if ( !PyArg_ParseTuple( args, "d", &x ) )
	    return NULL;
	y = x;
	cx = cy = 0.0;
	break;

      case 2:
	if ( !PyArg_ParseTuple( args, "dd", &x, &y ) )
	    return NULL;
	cx = cy = 0.0;
	break;

      case 3:
	if ( !PyArg_ParseTuple( args, "ddd", &x, &cx, &cy ) )
	    return NULL;
	y = x;
	break;

      case 4:
	if ( !PyArg_ParseTuple( args, "dddd", &x, &y, &cx, &cy ) )
	    return NULL;
	break;

    }

    glPopMatrix();
    glTranslated( cx, cy, 0.0 );
    glScaled( x, y, 1.0 );
    glTranslated( -cx, -cy, 0.0 );
    glPushMatrix();
    glTranslated( 0, 0, current_id_depth );
    current_IPM_dirty = 1;

    Py_INCREF( Py_None );
    return Py_None;
}
	
FUNC_PY( draw_rotate )
{
    double theta, x = 0.0, y = 0.0;

    if ( !PyArg_ParseTuple( args, "d|dd", &theta, &x, &y ) )
	return NULL;

    glPopMatrix();
    glTranslated( x, y, 0.0 );
    glRotated( theta, 0, 0, 1 );
    glTranslated( -x, -y, 0.0 );
    glPushMatrix();
    glTranslated( 0, 0, current_id_depth );
    current_IPM_dirty = 1;

    Py_INCREF( Py_None );
    return Py_None;
}

