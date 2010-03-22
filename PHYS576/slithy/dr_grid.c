#include "sl_common.h"
#include "dr_draw.h"

PyObject* draw_grid( PyObject* self, PyObject* args )
{
    double sx, sy;
    int xstart, ystart, xend, yend;
    double extra = 0.5;
    int i;

    if ( !PyArg_ParseTuple( args, "ddiiii|d", &sx, &sy, &xstart, &ystart, &xend, &yend, &extra ) )
	return NULL;

    glBegin( GL_LINES );
    for ( i = xstart; i <= xend; ++i )
    {
	glVertex2d( sx*i, (ystart-extra) * sy );
	glVertex2d( sx*i, (yend+extra) * sy );
    }

    for ( i = ystart; i <= yend; ++i )
    {
	glVertex2d( (xstart-extra) * sx, sy*i );
	glVertex2d( (xend+extra) * sx, sy*i );
    }
    glEnd();

    Py_INCREF( Py_None );
    return Py_None;
}

    
    

/** Local Variables: **/
/** dsp-name:"draw" **/
/** End: **/
