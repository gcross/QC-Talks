#include "sl_common.h"

#ifndef _MAKEDEPEND
#include <stdlib.h>
#include <ctype.h>
#endif

#include "dr_draw.h"
#include "diafont.h"

#define CHANGE_FONT    1
#define CHANGE_COLOR   2

static void parse_anchor( char* str, char* horz, char* vert )
{
    int len = str ? strlen( str ) : 0;

    if ( len == 2 )
    {
	switch ( str[1] )
	{
	  case 'e':
	  case 'E':
	  case 'r':
	  case 'R':
	    *horz = 'r';
	    break;

	  case 'w':
	  case 'W':
	  case 'l':
	  case 'L':
	    *horz = 'l';
	    break;

	  case 'c':
	  case 'C':
	  default:
	    *horz = 'c';
	    break;
	}

	switch( str[0] )
	{
	  case 't':
	  case 'T':
	  case 'n':
	  case 'N':
	    *vert = 't';
	    break;

	  case 'l':
	  case 'L':
	    *vert = 'l';
	    break;

	  case 'b':
	  case 'B':
	  case 's':
	  case 'S':
	    *vert = 'b';
	    break;
	    
	  case 'f':
	  case 'F':
	    *vert = 'f';
	    break;
	    
	  case 'c':
	  case 'C':
	  default:
	    *vert = 'c';
	    break;

	}
    }
    else if ( len == 1 )
    {
	switch( str[0] )
	{
	  case 'n':
	  case 'N':
	  case 't':
	  case 'T':
	    *horz = 'c';
	    *vert = 't';
	    break;

	  case 'e':
	  case 'E':
	  case 'r':
	  case 'R':
	    *horz = 'r';
	    *vert = 'c';
	    break;

	  case 's':
	  case 'S':
	  case 'b':
	  case 'B':
	    *horz = 'c';
	    *vert = 'b';
	    break;

	  case 'w':
	  case 'W':
	  case 'l':
	  case 'L':
	    *horz = 'l';
	    *vert = 'c';
	    break;

	  case 'c':
	  default:
	    *horz = 'c';
	    *vert = 'c';
	    break;
	}
    }
    else
    {
	*horz = 'c';
	*vert = 'c';
    }
}

FUNC_PY_KEYWD( draw_diatext )
{
    static char* kwlist[] = { "positionx", "positiony", "text", "font", "size", "justify",
			      "anchor", "wrap", "nodraw", "_alpha", NULL };
    
    FontObject* fontobj = NULL;
    static FontObjectMethods* c_methods = NULL;
    
    double sxpos, sypos;
    double size = 1.0;
    double justify = 0.0;
    double wrap = -1.0;
    double alpha = 1.0;
    PyObject* textobj = NULL;
    char* anchor = "c";
    PyObject* nodrawobj = NULL;
    
    int j, k, n;
    int textstrlen;
    char vert_anchor, horz_anchor;
    double bxmin, bxmax, bymin, bymax;
    double xpos, ypos;
    double xmin, xmax, ymin, ymax, aw, ah;
    int lines, breaks = 0;
    int ch;
    int el, elcount;
    PyObject* element;
    int islist;
    int charnum;
    int lastbreak;
    int lastbreakskip;
    double lastbreakstartpos, lastbreakendmin, lastbreakendoffset;
    
    FontObject* default_fontobj;
    double default_color[4];
    double* rgba;

    PyObject* retval;
    double left, right, top, bottom;
    
    static double* bounds = NULL;
    static int bounds_alloc = 0;
    static int* breaklist = NULL;
    static int breaks_alloc = 0;

    if ( !PyArg_ParseTupleAndKeywords( args, keywds, "ddOO|ddsdOd", kwlist,
				       &sxpos, &sypos, &textobj, &fontobj,
				       &size, &justify, &anchor, &wrap, &nodrawobj, &alpha ) )
	return NULL;

    islist = PyList_Check( textobj );
    if ( !islist && !PyString_Check( textobj ) && !PyUnicode_Check( textobj ) )
    {
	PyErr_SetString( DrawError, "text argument must be string or list or unicode" );
	return NULL;
    }

    if ( !PyObject_HasAttrString( (PyObject*)fontobj, "diafont_obj" ) )
    {
	PyErr_SetString( DrawError, "bad font object" );
	return NULL;
    }
    

    parse_anchor( anchor, &horz_anchor, &vert_anchor );

    default_fontobj = fontobj;

    if ( wrap < 0.0 )
	wrap = 0.0;
    else
	wrap /= size;
    
    
    //
    //  get the C methods structure if we don't already have it
    //
    
    if ( c_methods == NULL )
    {
	PyObject* addrobj;

	addrobj = PyObject_CallMethod( (PyObject*)fontobj, "c_methods", NULL );
	if ( addrobj == NULL )
	{
	    PyErr_SetString( PyExc_ValueError, "bad font object" );
	    return NULL;
	}
	
	c_methods = (FontObjectMethods*)PyCObject_AsVoidPtr( addrobj );
	Py_DECREF( addrobj );
    }

    //
    //  allocate the bounds list
    //

    if ( bounds == NULL )
    {
	bounds_alloc = 32;
	bounds = malloc( bounds_alloc * 2 * sizeof( double ) );
    }
    if ( breaklist == NULL )
    {
	breaks_alloc = 32;
	breaklist = malloc( breaks_alloc * 2 * sizeof( int ) );
    }
    
    bxmin = 1e38;
    bxmax = -1e38;
    bymin = 1e38;
    bymax = -1e38;

    //
    // compute X bounds and line breaks
    //
    
    xpos = 0.0;
    ypos = 0.0;
    j = 0;
    bounds[0] = 0.0;
    bounds[1] = -1.0;
    lines = 1;

    elcount = islist ? PyList_Size( textobj ) : 1;
    charnum = 0;
    lastbreak = -1;
    lastbreakskip = -1;
    breaks = 0;
    lastbreakendmin = -1.0;
    for ( el = 0; el < elcount; ++el )
    {
	element = islist ? PyList_GetItem( textobj, el ) : textobj;

	if ( PyString_Check( element ) )
	{
	    char* textstr;

	    PyString_AsStringAndSize( element, &textstr, &textstrlen );

	    for ( k = 0; k < textstrlen; ++k, ++charnum )
	    {
		switch( textstr[k] )
		{
		  case '\n':
		    ++lines;
		    if ( lines > bounds_alloc )
		    {
			if ( bounds_alloc < 2048 )
			    bounds_alloc *= 2;
			else
			    bounds_alloc += 2048;
			bounds = realloc( bounds, bounds_alloc * 2 * sizeof( double ) );
		    }
		    
		    ++j;
		    bounds[j*2+0] = 0.0;
		    bounds[j*2+1] = -1.0;
		    
		    xpos = 0.0;

		    lastbreak = -1;
		    lastbreakskip = -1;
		    break;
		    
		  default:
		    if ( textstr[k] == ' ' )
		    {
			lastbreakendmin = -1.0;
			if ( charnum == lastbreak + lastbreakskip )
			    ++lastbreakskip;
			else
			{
			    lastbreakstartpos = bounds[j*2+1];
			    lastbreak = charnum;
			    lastbreakskip = 1;
#if 0
			    printf( "lastbreakstartpos %.4f\n", lastbreakstartpos );
#endif
			}
		    }
		    
		    if ( c_methods->get_dimensions( fontobj, textstr[k], &xmin, &xmax, &ymin, &ymax, &aw, &ah ) != 0 )
			continue;
		    xmin += xpos;
		    xmax += xpos;

		    if ( textstr[k] != ' ' )
		    {
			n = bounds[j*2+0] > bounds[j*2+1];
			if ( n || bounds[j*2+0] > xmin ) bounds[j*2+0] = xmin;
			if ( n || bounds[j*2+1] < xmax ) bounds[j*2+1] = xmax;
#if 0
			printf( "[%c] line %d  %.4f %.4f      %.4f   %.4f %.4f\n", textstr[k], j, bounds[j*2+0], bounds[j*2+1], xpos, xmin, xmax );
#endif
			if ( lastbreakendmin == -1.0 )
			{
			    lastbreakendmin = xmin;
			    lastbreakendoffset = xmin - xpos;
#if 0
			    printf( "lastbreakendmin is %.4f,  offset is %.4f\n",
				    lastbreakendmin, lastbreakendoffset );
#endif
			}
		    }
		    
		    xpos += aw;

		    if ( textstr[k] != ' ' && wrap > 0.0 && xmax > wrap && lastbreak != -1 )
		    {
			++breaks;
			if ( breaks > breaks_alloc )
			{
			    if ( breaks_alloc < 2048 )
				breaks_alloc *= 2;
			    else
				breaks_alloc += 2048;
			    breaklist = realloc( breaklist, breaks_alloc * 2 * sizeof( int ) );
			}

			breaklist[breaks*2-2] = lastbreak;
			breaklist[breaks*2-1] = lastbreakskip;
			
			++lines;
			if ( lines > bounds_alloc )
			{
			    if ( bounds_alloc < 2048 )
				bounds_alloc *= 2;
			    else
				bounds_alloc += 2048;
			    bounds = realloc( bounds, bounds_alloc * 2 * sizeof( double ) );
			}

			
			++j;
			xpos -= lastbreakendmin;
			bounds[j*2+0] = lastbreakendoffset;
			bounds[j*2+1] = bounds[j*2-1] - lastbreakendmin + lastbreakendoffset;
			bounds[j*2-1] = lastbreakstartpos;
#if 0
			printf( "  line %d now %.4f %.4f\n", j-1, bounds[j*2-2], bounds[j*2-1] );
			printf( "  line %d now %.4f %.4f\n", j, bounds[j*2], bounds[j*2+1] );
			printf( "  xpos now %.4f\n", xpos );
#endif
			

			lastbreak = -1;
			lastbreakskip = -1;
		    }
		}
	    }
	}
	else if ( PyUnicode_Check( element ) )
	{
	    Py_UNICODE* textstr;

	    textstr = PyUnicode_AsUnicode( element );
	    textstrlen = PyUnicode_GetSize( element );

	    for ( k = 0; k < textstrlen; ++k, ++charnum )
	    {
		switch( textstr[k] )
		{
		  case 0x000a:
		    ++lines;
		    if ( lines > bounds_alloc )
		    {
			if ( bounds_alloc < 2048 )
			    bounds_alloc *= 2;
			else
			    bounds_alloc += 2048;
			bounds = realloc( bounds, bounds_alloc * 2 * sizeof( double ) );
		    }
		    
		    ++j;
		    bounds[j*2+0] = 0.0;
		    bounds[j*2+1] = -1.0;
		    
		    xpos = 0.0;

		    lastbreak = -1;
		    lastbreakskip = -1;
		    break;
		    
		  default:
		    if ( textstr[k] == ' ' )
		    {
			lastbreakendmin = -1.0;
			if ( charnum == lastbreak + lastbreakskip )
			    ++lastbreakskip;
			else
			{
			    lastbreakstartpos = bounds[j*2+1];
			    lastbreak = charnum;
			    lastbreakskip = 1;
#if 0
			    printf( "lastbreakstartpos %.4f\n", lastbreakstartpos );
#endif
			}
		    }
		    
		    if ( c_methods->get_dimensions( fontobj, textstr[k], &xmin, &xmax, &ymin, &ymax, &aw, &ah ) != 0 )
			continue;
		    xmin += xpos;
		    xmax += xpos;

		    if ( textstr[k] != ' ' )
		    {
			n = bounds[j*2+0] > bounds[j*2+1];
			if ( n || bounds[j*2+0] > xmin ) bounds[j*2+0] = xmin;
			if ( n || bounds[j*2+1] < xmax ) bounds[j*2+1] = xmax;
#if 0
			printf( "[%c] line %d  %.4f %.4f      %.4f   %.4f %.4f\n", textstr[k], j, bounds[j*2+0], bounds[j*2+1], xpos, xmin, xmax );
#endif
			if ( lastbreakendmin == -1.0 )
			{
			    lastbreakendmin = xmin;
			    lastbreakendoffset = xmin - xpos;
#if 0
			    printf( "lastbreakendmin is %.4f,  offset is %.4f\n",
				    lastbreakendmin, lastbreakendoffset );
#endif
			}
		    }
		    
		    xpos += aw;

		    if ( textstr[k] != ' ' && wrap > 0.0 && xmax > wrap && lastbreak != -1 )
		    {
			++breaks;
			if ( breaks > breaks_alloc )
			{
			    if ( breaks_alloc < 2048 )
				breaks_alloc *= 2;
			    else
				breaks_alloc += 2048;
			    breaklist = realloc( breaklist, breaks_alloc * 2 * sizeof( int ) );
			}

			breaklist[breaks*2-2] = lastbreak;
			breaklist[breaks*2-1] = lastbreakskip;
			
			++lines;
			if ( lines > bounds_alloc )
			{
			    if ( bounds_alloc < 2048 )
				bounds_alloc *= 2;
			    else
				bounds_alloc += 2048;
			    bounds = realloc( bounds, bounds_alloc * 2 * sizeof( double ) );
			}

			
			++j;
			xpos -= lastbreakendmin;
			bounds[j*2+0] = lastbreakendoffset;
			bounds[j*2+1] = bounds[j*2-1] - lastbreakendmin + lastbreakendoffset;
			bounds[j*2-1] = lastbreakstartpos;
#if 0
			printf( "  line %d now %.4f %.4f\n", j-1, bounds[j*2-2], bounds[j*2-1] );
			printf( "  line %d now %.4f %.4f\n", j, bounds[j*2], bounds[j*2+1] );
			printf( "  xpos now %.4f\n", xpos );
#endif
			

			lastbreak = -1;
			lastbreakskip = -1;
		    }
		}
	    }
	}
	else if ( c_methods->font_class && PyObject_IsInstance( element, c_methods->font_class ) )
	{
	    fontobj = (FontObject*)element;
	}
	else if ( PyInt_Check( element ) )
	{
	    k = PyInt_AsLong( element );
	    if ( k == TEXT_RESET || k == TEXT_RESETFONT )
		fontobj = default_fontobj;
	}
	
    }

    bxmin = bounds[0];
    bxmax = bounds[1];
    for ( j = 1; j < lines; ++j )
    {
	if ( bxmin > bounds[j*2+0] ) bxmin = bounds[j*2+0];
	if ( bxmax < bounds[j*2+1] ) bxmax = bounds[j*2+1];
    }

#if 0
    for ( j = 0; j < lines; ++j )
	printf( "line %d:  %.4f %.4f\n", j, bounds[j*2], bounds[j*2+1] );
    printf( "bbox:  %.4f %.4f\n", bxmin, bxmax );
#endif
    
    if ( breaks+1 > breaks_alloc )
    {
	if ( breaks_alloc < 2048 )
	    breaks_alloc *= 2;
	else
	    breaks_alloc += 2048;
	breaklist = realloc( breaklist, breaks_alloc * 2 * sizeof( int ) );
    }
    breaklist[breaks*2] = -1;
    breaklist[breaks*2+1] = -1;

#if 0
    printf( "break table:\n" );
    for ( i = 0; i <= breaks; ++i )
	printf( " (%d,%d)", breaklist[i*2], breaklist[i*2+1] );
    printf( "\n" );
#endif


    //
    // compute Y bounds
    //

    j = -1;
    fontobj = default_fontobj;

    ypos = 1.0;
    n = 0;
    charnum = -1;
    lastbreakskip = 0;
    for ( el = 0; el < elcount; ++el )
    {
	element = islist ? PyList_GetItem( textobj, el ) : textobj;

	if ( PyString_Check( element ) )
	{
	    unsigned char* textstr;

	    PyString_AsStringAndSize( element, (char**)&textstr, &textstrlen );
	    
	    for ( k = ((j==-1)?-1:0); k < textstrlen; ++k, ++charnum )
	    {
		if ( lastbreakskip > 0 )
		{
		    --lastbreakskip;
		    continue;
		}
		
		ch = (j==-1) ? '\n' : textstr[k];

		if ( n < breaks && charnum == breaklist[n*2] )
		{
		    ch = '\n';
		    lastbreakskip = breaklist[n*2+1] - 1;
		    ++n;
		}

		switch( ch )
		{
		  case '\n':
		    ypos -= 1.0;
		    ++j;
		    break;
		    
		  default:
		    if ( c_methods->get_dimensions( fontobj, textstr[k], &xmin, &xmax, &ymin, &ymax, &aw, &ah ) != 0 )
			continue;
		    ymin += ypos;
		    ymax += ypos;
		    if ( bymin > ymin ) bymin = ymin;
		    if ( bymax < ymax ) bymax = ymax;
		}
	    }
	}
	else if ( PyUnicode_Check( element ) )
	{
	    Py_UNICODE* textstr;

	    textstr = PyUnicode_AsUnicode( element );
	    textstrlen = PyUnicode_GetSize( element );
	    
	    for ( k = ((j==-1)?-1:0); k < textstrlen; ++k, ++charnum )
	    {
		if ( lastbreakskip > 0 )
		{
		    --lastbreakskip;
		    continue;
		}
		
		ch = (j==-1) ? '\n' : textstr[k];

		if ( n < breaks && charnum == breaklist[n*2] )
		{
		    ch = '\n';
		    lastbreakskip = breaklist[n*2+1] - 1;
		    ++n;
		}

		switch( ch )
		{
		  case '\n':
		    ypos -= 1.0;
		    ++j;
		    break;
		    
		  default:
		    if ( c_methods->get_dimensions( fontobj, textstr[k], &xmin, &xmax, &ymin, &ymax, &aw, &ah ) != 0 )
			continue;
		    ymin += ypos;
		    ymax += ypos;
		    if ( bymin > ymin ) bymin = ymin;
		    if ( bymax < ymax ) bymax = ymax;
		    break;
		}
	    }
	}
	else if ( c_methods->font_class && PyObject_IsInstance( element, c_methods->font_class ) )
	{
	    fontobj = (FontObject*)element;
	}
	else if ( PyInt_Check( element ) )
	{
	    k = PyInt_AsLong( element );
	    if ( k == TEXT_RESET || k == TEXT_RESETFONT )
		fontobj = default_fontobj;
	}
    }
    
    //
    // place bbox relative to point
    //
    
    if ( wrap > 0.0 )
	bxmax = wrap;

    switch( vert_anchor )
    {
      case 't':
	top = sypos;
	bottom = (bymin - bymax) * size + sypos;
	break;
      case 'b':
	top = (bymax - bymin) * size + sypos;
	bottom = sypos;
	break;
      case 'c':
	top = (bymax - bymin) / 2 * size + sypos;
	bottom = (bymin - bymax) / 2 * size + sypos;
	break;
      case 'l':
	top = (bymax + lines - 1) * size + sypos;
	bottom = (bymin + lines - 1) * size + sypos;
	break;
      case 'f':
	top = bymax * size + sypos;
	bottom = bymin * size + sypos;
	break;
    }
    left = bxmin;
    right = bxmax;
    switch( horz_anchor )
    {
      case 'l':
	left -= bxmin;
	right -= bxmin;
	break;
      case 'r':
	left -= bxmax;
	right -= bxmax;
	break;
      case 'c':
	left += (bxmin - bxmax) / 2;
	right += (bxmin - bxmax) / 2;
	break;
    }
    left = left * size + sxpos;
    right = right * size + sxpos;
	
    retval =  Py_BuildValue( "{s:d,s:d,s:d,s:d,s:d,s:d,s:i}",
			     "width", right - left,
			     "height", top - bottom,
			     "left", left,
			     "right", right,
			     "bottom", bottom,
			     "top", top,
			     "lines", lines );

    if ( nodrawobj && PyObject_IsTrue( nodrawobj ) )
	goto skip_draw;
    
    glGetDoublev( GL_CURRENT_COLOR, default_color );
    glColor4d( default_color[0], default_color[1], default_color[2], default_color[3] * alpha );
    
    glMatrixMode( GL_MODELVIEW );
    glPushMatrix();
    
    glTranslated( sxpos, sypos, 0.0 );
    glScaled( size, size, 1.0 );

    xpos = 0.0;
    ypos = 1.0;
    switch( horz_anchor )
    {
      case 'l': glTranslated( -bxmin, 0.0, 0.0 ); break;
      case 'r': glTranslated( -bxmax, 0.0, 0.0 ); break;
      case 'c': glTranslated( (bxmin - bxmax) / 2, 0.0, 0.0); break;
    }
    switch( vert_anchor )
    {
      case 't': glTranslated( 0.0, -bymax, 0.0 ); break;
      case 'b': glTranslated( 0.0, -bymin, 0.0 ); break;
      case 'c': glTranslated( 0.0, -(bymax + bymin)/2.0, 0.0 ); break;
      case 'l': glTranslated( 0.0, lines-1, 0.0 ); break;
    }

    j = -1;
    fontobj = default_fontobj;
    c_methods->start_using_font( fontobj );

    n = 0;
    charnum = -1;
    lastbreakskip = 0;
    for ( el = 0; el < elcount; ++el )
    {
	element = islist ? PyList_GetItem( textobj, el ) : textobj;

	if ( PyString_Check( element ) )
	{
	    unsigned char* textstr;

	    PyString_AsStringAndSize( element, (char**)&textstr, &textstrlen );
	    
	    for ( k = ((j==-1)?-1:0); k < textstrlen; ++k, ++charnum )
	    {
		if ( lastbreakskip > 0 )
		{
		    --lastbreakskip;
		    continue;
		}
		
		ch = (j==-1) ? '\n' : textstr[k];

		if ( n < breaks && charnum == breaklist[n*2] )
		{
		    ch = '\n';
		    lastbreakskip = breaklist[n*2+1] - 1;
		    ++n;
		}

		switch( ch )
		{
		  case '\n':
		    xpos = 0.0;
		    ypos -= 1.0;
		    ++j;
		    if ( bounds[j*2+0] < bounds[j*2+1] )
		    {
			// this line is nonempty, so adjust xpos for justification
			xpos += -bounds[j*2+0] + (bxmax - (bounds[j*2+1]-bounds[j*2+0])) * justify;
		    }
		    break;
		    
		  default:
		    c_methods->draw_character( fontobj, textstr[k], xpos, ypos, &xpos, &ypos );
		    break;
		}
	    }
	}
	else if ( PyUnicode_Check( element ) )
	{
	    Py_UNICODE* textstr;

	    textstr = PyUnicode_AsUnicode( element );
	    textstrlen = PyUnicode_GetSize( element );
	    
	    for ( k = ((j==-1)?-1:0); k < textstrlen; ++k, ++charnum )
	    {
		if ( lastbreakskip > 0 )
		{
		    --lastbreakskip;
		    continue;
		}
		
		ch = (j==-1) ? '\n' : textstr[k];

		if ( n < breaks && charnum == breaklist[n*2] )
		{
		    ch = '\n';
		    lastbreakskip = breaklist[n*2+1] - 1;
		    ++n;
		}

		switch( ch )
		{
		  case '\n':
		    xpos = 0.0;
		    ypos -= 1.0;
		    ++j;
		    if ( bounds[j*2+0] < bounds[j*2+1] )
		    {
			// this line is nonempty, so adjust xpos for justification
			xpos += -bounds[j*2+0] + (bxmax - (bounds[j*2+1]-bounds[j*2+0])) * justify;
		    }
		    break;
		    
		  default:
		    c_methods->draw_character( fontobj, textstr[k], xpos, ypos, &xpos, &ypos );
		    break;
		}
	    }
	}
	else if ( color_class && PyObject_IsInstance( element, color_class ) )
	{
	    rgba = get_color_from_object( element );
	    if ( rgba )
	    {
		rgba[3] *= global_alpha * alpha;
		glColor4dv( rgba );
	    }
	}
	else if ( c_methods->font_class && PyObject_IsInstance( element, c_methods->font_class ) )
	{
	    c_methods->finish_using_font( fontobj );
	    fontobj = (FontObject*)element;
	    c_methods->start_using_font( fontobj );
	}
	else if ( PyInt_Check( element ) )
	{
	    k = PyInt_AsLong( element );
	    if ( k == TEXT_RESET || k == TEXT_RESETCOLOR )
		glColor4d( default_color[0], default_color[1], default_color[2], default_color[3] * alpha );
	    if ( k == TEXT_RESET || k == TEXT_RESETFONT )
	    {
		c_methods->finish_using_font( fontobj );
		fontobj = default_fontobj;
		c_methods->start_using_font( fontobj );
	    }
		
	}
    }
    c_methods->finish_using_font( fontobj );

    glColor4dv( default_color );
    glPopMatrix();

 skip_draw:
    return retval;
}

/** Local Variables: **/
/** dsp-name:"draw" **/
/** End: **/
