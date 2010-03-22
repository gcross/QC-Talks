
#ifndef _MAKEDEPEND
#ifndef _SL_COMMON_H
#define _SL_COMMON_H

#ifdef WIN32
#define _WIN32_WINNT 0x0500
#include <windows.h>
#else
#include <stdio.h>
#include <stdlib.h>
#endif

#include <math.h>

#ifdef WIN32
#include <gl\gl.h>
#include <gl\glu.h>
#elif defined(__APPLE__)
#include <gl.h>
#include <glu.h>
#include <agl.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glx.h>
#endif

#include <assert.h>

#define MAC_OSX_TK
#include <tcl.h>
#include <tk.h>

/* include Python last, allowing some precompiled mac headers to be included without warning.  */
#ifdef _DEBUG
#undef _DEBUG
#include <Python.h>
#define _DEBUG
#else
#include <Python.h>
#endif


/* multiple inclusion */
#endif
/* _MAKEDEPEND */
#endif

