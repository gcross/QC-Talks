#include "dr_draw.h"

double* get_color_from_args( PyObject* args )
{
    int ref = 0;
    double* result;
    
    if ( !PyTuple_Check( args ) )
	return NULL;

    if ( PyTuple_GET_SIZE( args ) == 1 )
    {
	args = PyTuple_GET_ITEM( args, 0 );
	if ( !PyTuple_Check( args ) )
	{
	    args = Py_BuildValue( "(O)", args );
	    ref = 1;
	}
    }

    result = get_color_from_object( args );

    if ( ref ) {
        /* Py_DECREF is a macro with if/else in its definition */
	Py_DECREF( args );
    }

    return result;
}
	
	    

double* get_color_from_object( PyObject* args )
{
    static double rgba[4];
    int ref = 0;

    if ( !PyObject_IsInstance( args, color_class ) )
    {
	args = PyObject_CallObject( color_class, args );
	if ( args == NULL )
	    return NULL;
	ref = 1;
    }
	    
    rgba[0] = PyFloat_AsDouble( PyTuple_GetItem( args, 0 ) );
    rgba[1] = PyFloat_AsDouble( PyTuple_GetItem( args, 1 ) );
    rgba[2] = PyFloat_AsDouble( PyTuple_GetItem( args, 2 ) );
    rgba[3] = PyFloat_AsDouble( PyTuple_GetItem( args, 3 ) );

    if ( ref ) {
	Py_DECREF( args );
    }

    return rgba;
}

    

