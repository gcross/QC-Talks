#include "sl_common.h"
#include "dr_draw.h"
#include "diaimage.h"

static int parse_image_anchor( PyObject* anchor, double* ax, double* ay )
{
    if ( anchor == NULL )
    {
	*ax = 0.5;
	*ay = 0.5;
	return 0;
    }
    else if ( PyString_Check( anchor ) )
    {
	char* str;
	int len;

	if ( PyString_AsStringAndSize( anchor, &str, &len ) == -1 )
	    return -1;

	if ( len > 2 || len < 1 )
	    return -1;

	if ( len == 1 )
	{
	    switch( str[0] )
	    {
	      case 'c':  *ax = 0.5; *ay = 0.5; break;
	      case 'n':  *ax = 0.5; *ay = 1.0; break;
	      case 's':  *ax = 0.5; *ay = 0.0; break;
	      case 'e':  *ax = 1.0; *ay = 0.5; break;
	      case 'w':  *ax = 0.0; *ay = 0.5; break;
	      default: return -1;
	    }
	}
	else
	{
	    switch( str[0] )
	    {
	      case 'c':
		*ay = 0.5;
		break;

	      case 'n':
	      case 't':
		*ay = 1.0;
		break;

	      case 's':
	      case 'b':
		*ay = 0.0;
		break;

	      default:
		return -1;
	    }
	    switch( str[1] )
	    {
	      case 'c':
		*ax = 0.5;
		break;

	      case 'w':
	      case 'l':
		*ax = 0.0;
		break;

	      case 'e':
	      case 'r':
		*ax = 1.0;
		break;

	      default:
		return -1;
	    }
	}

	return 0;
    }
    else if ( PySequence_Check( anchor ) && PySequence_Length( anchor ) == 2 )
    {
	PyObject* temp;

	temp = PySequence_GetItem( anchor, 0 );
	*ax = PyFloat_AsDouble( temp );
	Py_DECREF( temp );

	temp = PySequence_GetItem( anchor, 1 );
	*ay = PyFloat_AsDouble( temp );
	Py_DECREF( temp );

	return 0;
    }
    
    return -1;
}

PyObject* draw_image( PyObject* self, PyObject* args, PyObject* keywds )
{
    static char* kwlist[] = { "positionx", "positiony", "image", "width", "height", "anchor", "alpha", NULL };
    
    ImageObject* imageobj;
    static ImageObjectMethods* c_methods = NULL;

    PyObject* anchorobj = NULL;
    PyObject* widthobj = NULL;
    PyObject* heightobj = NULL;
    double alpha = 1.0;
    double x, y, w = 1.0, h;
    double ax, ay;
    
    if ( !PyArg_ParseTupleAndKeywords( args, keywds, "ddO|OOOd", kwlist,
				       &x, &y, &imageobj,
				       &widthobj, &heightobj, &anchorobj, &alpha ) )
	return NULL;

    if ( widthobj == NULL && heightobj == NULL )
    {
	PyErr_SetString( DrawError, "must specify image width and/or height" );
	return NULL;
    }

    if ( parse_image_anchor( anchorobj, &ax, &ay ) )
    {
	PyErr_SetString( DrawError, "didn't understand image anchor" );
	return NULL;
    }

    if ( widthobj == Py_None )
	widthobj = NULL;
    if ( heightobj == Py_None )
	heightobj = NULL;
	     
    
    if ( widthobj )
	w = PyFloat_AsDouble( widthobj );
    if ( heightobj )
	h = PyFloat_AsDouble( heightobj );

    if ( !widthobj )
	w = h / imageobj->h * imageobj->w;
    else
	h = w / imageobj->w * imageobj->h;

    if ( c_methods == NULL )
    {
	PyObject* addrobj;

	addrobj = PyObject_CallMethod( (PyObject*)imageobj, "c_methods", NULL );
	c_methods = (ImageObjectMethods*)PyCObject_AsVoidPtr( addrobj );
	Py_DECREF( addrobj );
    }

    glPushAttrib( GL_CURRENT_BIT );
    glColor4d( 1.0, 1.0, 1.0, global_alpha * alpha );
    c_methods->draw_image( imageobj, x, y, w, h, ax, ay );
    glPopAttrib();

    return Py_BuildValue( "{s:d,s:d,s:d,s:d,s:d,s:d}",
			  "width", w,
			  "height", h,
			  "left", x - w * ax,
			  "right", x + w * (1.0-ax),
			  "bottom", y - h * ay,
			  "top", y + h * (1.0-ay) );
}









/** Local Variables: **/
/** dsp-name:"draw" **/
/** End: **/
