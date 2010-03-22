/*
 *   soggy.c
 *       a simple Tk widget that creates an OpenGL drawing area.
 *
 *   $Id: ob_slsoggy.c,v 1.7 2003/04/14 01:21:40 dougz Exp $
 *
 *   Copyright (c) 2001 Douglas Zongker
 */

#define SOGGY_VERSION  "0.5"

#include "sl_common.h"

#ifdef WIN32
/* this prevents us from using the stub libraries, unfortunately. */
HWND Tk_GetHWND( Window );
#endif

struct platform {
#ifdef WIN32
    /* w32 */
#define ContextType HCLRC
    HWND hwnd;
    ContextType hglrc;
#elif defined(USE_APPLE_AGL)
    /* apple under aqua */
#define ContextType AGLContext
    CGrafPtr draw;  /* is the same as a GWorldPtr  */
    Tk_Cursor offcursor;
    ContextType context;
#else
    /* linux */
#define ContextType GLXContext
    Tk_Cursor offcursor;
    ContextType cx;
#endif
};

typedef struct _soggy
{
    Tk_Window tkwin;
    Display* dpy;
    Tcl_Interp* interp;
    Tk_OptionTable optionTable;
    
    int x, y;
    int glwidth, glheight;
    int width, height;
    int updatePending;
    
    int showcursor;
    int cursorstate;
    
    struct platform pl;

    Tcl_Obj* reshapePtr;
    Tcl_Obj* redrawPtr;
    Tcl_Obj* initPtr;

    GLint red_bits, green_bits, blue_bits, alpha_bits;
    GLint red_accum_bits, green_accum_bits, blue_accum_bits, alpha_accum_bits;
    GLint depth_bits, stencil_bits;
} Soggy;

ContextType last_created_context = NULL;

static Tk_OptionSpec optionSpecs[] = {
    { TK_OPTION_PIXELS, "-width", "width", "Width", "200",
      -1, Tk_Offset( Soggy, width ), 0, 0, 0 },
    { TK_OPTION_PIXELS, "-height", "height", "Height", "200",
      -1, Tk_Offset( Soggy, height ), 0, 0, 0 },
    
    { TK_OPTION_STRING, "-reshape", "reshape", "Reshape", NULL,
      Tk_Offset( Soggy, reshapePtr ), -1, TK_OPTION_NULL_OK, 0, 0 },
    { TK_OPTION_STRING, "-redraw", "redraw", "Redraw", NULL,
      Tk_Offset( Soggy, redrawPtr ), -1, TK_OPTION_NULL_OK, 0, 0 },
    { TK_OPTION_STRING, "-init", "init", "Init", NULL,
      Tk_Offset( Soggy, initPtr ), -1, TK_OPTION_NULL_OK, 0, 0 },

    { TK_OPTION_BOOLEAN, "-showcursor", "showcursor", "Showcursor", "1",
      -1, Tk_Offset( Soggy, showcursor ), 0, 0, 0 },
    
    { TK_OPTION_INT, "-red", "red", "Red", "8",
      -1, Tk_Offset( Soggy, red_bits ), 0, 0, 0 },
    { TK_OPTION_INT, "-green", "green", "Green", "8",
      -1, Tk_Offset( Soggy, green_bits ), 0, 0, 0 },
    { TK_OPTION_INT, "-blue", "blue", "Blue", "8",
      -1, Tk_Offset( Soggy, blue_bits ), 0, 0, 0 },
    { TK_OPTION_INT, "-alpha", "alpha", "Alpha", "0",
      -1, Tk_Offset( Soggy, alpha_bits ), 0, 0, 0 },
    { TK_OPTION_INT, "-accumred", "accumred", "Accumred", "0",
      -1, Tk_Offset( Soggy, red_accum_bits ), 0, 0, 0 },
    { TK_OPTION_INT, "-accumgreen", "accumgreen", "Accumgreen", "0",
      -1, Tk_Offset( Soggy, green_accum_bits ), 0, 0, 0 },
    { TK_OPTION_INT, "-accumblue", "accumblue", "Accumblue", "0",
      -1, Tk_Offset( Soggy, blue_accum_bits ), 0, 0, 0 },
    { TK_OPTION_INT, "-accumalpha", "accumalpha", "Accumalpha", "0",
      -1, Tk_Offset( Soggy, alpha_accum_bits ), 0, 0, 0 },
    { TK_OPTION_INT, "-depth", "depth", "Depth", "24",
      -1, Tk_Offset( Soggy, depth_bits ), 0, 0, 0 },
    { TK_OPTION_INT, "-stencil", "stencil", "Stencil", "8",
      -1, Tk_Offset( Soggy, stencil_bits ), 0, 0, 0 },
      
    { TK_OPTION_END, NULL, NULL, NULL, NULL,
      0, -1, 0, 0, 0 }
};

#ifdef WIN32
HDC g_saveswap;
#else
Soggy* g_saveswap;
#endif

static int SoggyCmd( ClientData clientData, Tcl_Interp* interp, int objc, Tcl_Obj* const objv[] );
static int SoggyConfigure( Tcl_Interp* interp, Soggy* soggyPtr, int objc, Tcl_Obj* const objv[] );
static int SoggyWidgetCmd( ClientData clientdata, Tcl_Interp* interp, int objc, Tcl_Obj* const objv[] );
static void SoggyEventProc( ClientData clientdata, XEvent* eventPtr );
static int OpenGLSetup( Soggy* soggyPtr, Tcl_Interp* interp );
static void SoggyDisplay( ClientData clientdata );
static void SoggyDestroy( char* data );
static int OpenGLWrapEval( Soggy* soggyPtr, Tcl_Obj* evalobj );

#ifdef _DEBUG
void Debug( char* fmt, ... );
#endif

#ifdef WIN32
int __declspec( dllexport ) Slsoggy_Init( Tcl_Interp* interp );
BOOL __declspec( dllexport ) WINAPI DllMain( HINSTANCE, DWORD, LPVOID );
BOOL WINAPI DllMain( HINSTANCE hInst, DWORD dwReason, LPVOID reserved )
{
    return 1;
}
#endif

int Slsoggy_Init( Tcl_Interp* interp )
{
    Tcl_PkgProvide( interp, "Slsoggy", SOGGY_VERSION );
    
    Tcl_CreateObjCommand( interp, "Slsoggy::soggy", SoggyCmd, NULL, NULL );

#if 0
    printf( "-- slsoggy_init\n" );
#endif
    
    return TCL_OK;
}

static int SoggyCmd( ClientData clientData, Tcl_Interp* interp,
		     int objc, Tcl_Obj* const objv[] )
{
    Soggy* soggyPtr;
    Tk_Window tkwin;
    Tk_OptionTable optionTable;

    optionTable = (Tk_OptionTable) clientData;
    if ( optionTable == NULL )
    {
	Tcl_CmdInfo info;
	char* name;

	optionTable = Tk_CreateOptionTable( interp, optionSpecs );
	name = Tcl_GetString( objv[0] );
	Tcl_GetCommandInfo( interp, name, &info );
	info.objClientData = (ClientData)optionTable;
	Tcl_SetCommandInfo( interp, name, &info );
    }
    
    if ( objc < 2 )
    {
	Tcl_WrongNumArgs( interp, 1, objv, "pathname ?options?" );
	return TCL_ERROR;
    }

    tkwin = Tk_CreateWindowFromPath( interp, Tk_MainWindow( interp ), Tcl_GetString( objv[1] ), NULL );
    if ( tkwin == NULL )
	return TCL_ERROR;

    Tk_SetClass( tkwin, "Soggy" );

    soggyPtr = (Soggy*)malloc( sizeof( Soggy ) );
    soggyPtr->tkwin = tkwin;
    soggyPtr->dpy = Tk_Display( tkwin );
    soggyPtr->interp = interp;
    soggyPtr->optionTable = optionTable;
    soggyPtr->updatePending = 0;

    soggyPtr->x = soggyPtr->y = 0;
    soggyPtr->glwidth = soggyPtr->glheight = -1;
    soggyPtr->width = soggyPtr->height = 200;
    soggyPtr->reshapePtr = NULL;
    soggyPtr->redrawPtr = NULL;
    soggyPtr->initPtr = NULL;
    soggyPtr->cursorstate = 1;

    Tk_CreateEventHandler( tkwin, ExposureMask|StructureNotifyMask, SoggyEventProc, (ClientData)soggyPtr );
    Tcl_CreateObjCommand( interp, Tk_PathName( tkwin ), SoggyWidgetCmd, (ClientData)soggyPtr, NULL );
    
    if ( Tk_InitOptions( interp, (char*)soggyPtr, optionTable, tkwin ) != TCL_OK )
    {
	Tk_DestroyWindow( soggyPtr->tkwin );
	return TCL_ERROR;
    }
    if ( SoggyConfigure( interp, soggyPtr, objc-2, objv+2 ) != TCL_OK )
    {
	Tk_DestroyWindow( soggyPtr->tkwin );
	return TCL_ERROR;
    }

#if defined( WIN32 ) ||  defined(USE_APPLE_AGL)
    /* ensures that the window is there, placed above the
       control window */
    Tk_MakeWindowExist( soggyPtr->tkwin );
#endif
    if ( OpenGLSetup( soggyPtr, interp ) != TCL_OK )
    {
	Tk_DestroyWindow( soggyPtr->tkwin );
	return TCL_ERROR;
    }

    Tk_MapWindow( soggyPtr->tkwin );
    if ( soggyPtr->initPtr )
	OpenGLWrapEval( soggyPtr, soggyPtr->initPtr );

#ifndef WIN32
    {
	char zero = 0;
	soggyPtr->pl.offcursor = Tk_GetCursorFromData( soggyPtr->interp, soggyPtr->tkwin,
						    &zero, &zero, 1, 1, 0, 0,
						    Tk_GetUid("white"), Tk_GetUid("black") );
    }
#endif
    
    Tcl_SetStringObj( Tcl_GetObjResult( interp ), Tk_PathName( tkwin ), -1 );
    return TCL_OK;
}

static int SoggyConfigure( Tcl_Interp* interp, Soggy* soggyPtr, int objc, Tcl_Obj* const objv[] )
{
    Tk_SavedOptions savedOptions;
    Tcl_Obj* errorResult = NULL;
    
    if ( Tk_SetOptions( interp, (char*)soggyPtr, soggyPtr->optionTable, objc, objv, soggyPtr->tkwin,
			&savedOptions, (int*)NULL ) != TCL_OK )
    {
	errorResult = Tcl_GetObjResult( interp );
	Tcl_IncrRefCount( errorResult );
	Tk_RestoreSavedOptions( &savedOptions );
    }
    else 
	Tk_FreeSavedOptions( &savedOptions );

    Tk_GeometryRequest( soggyPtr->tkwin, soggyPtr->width, soggyPtr->height );
    Tk_SetInternalBorder( soggyPtr->tkwin, 0 );
    
    if ( !soggyPtr->updatePending )
    {
	Tk_DoWhenIdle( SoggyDisplay, (ClientData)soggyPtr );
	soggyPtr->updatePending = 1;
    }
    
    if ( errorResult )
    {
	Tcl_SetObjResult( interp, errorResult );
	Tcl_DecrRefCount( errorResult );
 	return TCL_ERROR;
    }
    else
	return TCL_OK;
}

static int SoggyWidgetCmd( ClientData clientdata, Tcl_Interp* interp, int objc, Tcl_Obj* const objv[] )
{
#ifdef WIN32
    char* cmdtable[] = { "configure", "cget", "init", "reshape", "redraw", "eval", "gethwnd", NULL };
#else
    const char* cmdtable[] = { "configure", "cget", "init", "reshape", "redraw", "eval", NULL };
#endif
    int cmd;
    Soggy* soggyPtr = (Soggy*)clientdata;
    int result = TCL_OK;
    Tcl_Obj* objPtr;

    assert(soggyPtr != NULL);

    if ( objc < 2 )
    {
	Tcl_AppendResult( interp, "wrong # args: should be \"", objv[0], " option ?arg arg ...?\"", NULL );
	return TCL_ERROR;
    }

    Tk_Preserve( (ClientData)soggyPtr );

    if ( Tcl_GetIndexFromObj( interp, objv[1], cmdtable, "command", 0, &cmd ) != TCL_OK )
	goto error;

    switch ( cmd )
    {
      case 0:   // configure
	if ( objc <= 3 )
	{
	    objPtr = Tk_GetOptionInfo( interp, (char*)soggyPtr, soggyPtr->optionTable,
				       (objc==3) ? objv[2] : (Tcl_Obj*)NULL, soggyPtr->tkwin );
	    if ( objPtr == NULL )
		goto error;
		
	    Tcl_SetObjResult( interp, objPtr );
	}
	else
	{
	    result = SoggyConfigure( interp, soggyPtr, objc-2, objv+2 );
	}
	break;

      case 1:  // cget
	if ( objc != 3 )
	{
	    Tcl_WrongNumArgs( interp, 1, objv, "cget option" );
	    goto error;
	}
	objPtr = Tk_GetOptionValue( interp, (char*)soggyPtr, soggyPtr->optionTable,
				    objv[2], soggyPtr->tkwin );
	if ( objPtr == NULL )
	    goto error;

	Tcl_SetObjResult( interp, objPtr );
	break;

      case 2:   // init
	if ( soggyPtr->initPtr )
	    OpenGLWrapEval( soggyPtr, soggyPtr->initPtr );
	break;

      case 3:   // reshape
	if ( soggyPtr->reshapePtr )
	{
	    Tcl_Obj* tempobj;

	    tempobj = Tcl_DuplicateObj( soggyPtr->reshapePtr );
	    Tcl_IncrRefCount( tempobj );
	    Tcl_ListObjAppendElement( soggyPtr->interp, tempobj, Tcl_NewIntObj( soggyPtr->width ) );
	    Tcl_ListObjAppendElement( soggyPtr->interp, tempobj, Tcl_NewIntObj( soggyPtr->height ) );

	    OpenGLWrapEval( soggyPtr, tempobj );
		
	    Tcl_DecrRefCount( tempobj );
	}
	break;

      case 4:   // redraw
	// do nothing, just schedule an update
	break;

      case 5:
	if ( objc != 3 )
	{
	    Tcl_WrongNumArgs( interp, 1, objv, "eval script" );
	    goto error;
	}
	result = OpenGLWrapEval( soggyPtr, objv[2] );
	break;
	
#ifdef WIN32
      case 6:
      {
	  char* hwndstring = malloc( 20 );
	  sprintf( hwndstring, "%p", (void*)soggyPtr->hwnd );
	  Tcl_SetResult( soggyPtr->interp, hwndstring, free );
      }
      break;
#endif
    }

    if ( !soggyPtr->updatePending )
    {
	Tk_DoWhenIdle( SoggyDisplay, (ClientData) soggyPtr );
	soggyPtr->updatePending = 1;
    }

    Tk_Release( (ClientData)soggyPtr );
    return result;

 error:
    Tk_Release( (ClientData)soggyPtr );
    return TCL_ERROR;
}

static void SoggyEventProc( ClientData clientdata, XEvent* eventPtr )
{
    Soggy* soggyPtr = (Soggy*) clientdata;

    assert(soggyPtr != NULL);
    
    if ( eventPtr->type == Expose )
    {
	if ( !soggyPtr->updatePending )
	{
	    Tk_DoWhenIdle( SoggyDisplay, (ClientData) soggyPtr );
	    soggyPtr->updatePending = 1;
	}
    }
    else if ( eventPtr->type == ConfigureNotify )
    {
	int width, height;

	width = Tk_Width( soggyPtr->tkwin );
	height = Tk_Height( soggyPtr->tkwin );

	if ( width != soggyPtr->glwidth || height != soggyPtr->glheight )
	{
	    if ( soggyPtr->reshapePtr )
	    {
		Tcl_Obj* tempobj;

		tempobj = Tcl_DuplicateObj( soggyPtr->reshapePtr );
		Tcl_IncrRefCount( tempobj );
		Tcl_ListObjAppendElement( soggyPtr->interp, tempobj, Tcl_NewIntObj( width ) );
		Tcl_ListObjAppendElement( soggyPtr->interp, tempobj, Tcl_NewIntObj( height ) );

		OpenGLWrapEval( soggyPtr, tempobj );
		
		Tcl_DecrRefCount( tempobj );
	    }
		
	    soggyPtr->glwidth = width;
	    soggyPtr->glheight = height;
	}
	
	if ( !soggyPtr->updatePending )
	{
	    Tk_DoWhenIdle( SoggyDisplay, (ClientData)soggyPtr );
	    soggyPtr->updatePending = 1;
	}
    }
    else if ( eventPtr->type == DestroyNotify )
    {
#ifndef WIN32
	Tcl_DeleteCommand( soggyPtr->interp, Tk_PathName( soggyPtr->tkwin ) );
	soggyPtr->tkwin = NULL;
	if ( soggyPtr->updatePending )
	    Tk_CancelIdleCall( SoggyDisplay, (ClientData) soggyPtr );
	Tcl_EventuallyFree( (ClientData)soggyPtr, SoggyDestroy );
#endif
    }
}

#ifdef WIN32
/*
 * this is the Win32 version of OpenGLSetup, which uses wgl
 */
static int OpenGLSetup( Soggy* soggyPtr, Tcl_Interp* interp )
{
    /* the pixel format descriptor tells OpenGL what features we need
       to be supported in the window. */
    PIXELFORMATDESCRIPTOR pfd = {
	sizeof( PIXELFORMATDESCRIPTOR ),
	1,
	/* request a double-buffered window */
	PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
	PFD_TYPE_RGBA,
	
	/* guess "cColorBits" should be the sum of the channel requests */
	soggyPtr->red_bits + soggyPtr->green_bits + soggyPtr->blue_bits + soggyPtr->alpha_bits,

	/* these are apparently ignored */
	soggyPtr->red_bits, 0,
	soggyPtr->green_bits, 0,
	soggyPtr->blue_bits, 0,
	soggyPtr->alpha_bits, 0,
	
	/* guess "cAccumBits" should be the sum of the channel requests */
	soggyPtr->red_accum_bits + soggyPtr->green_accum_bits + soggyPtr->blue_accum_bits + soggyPtr->alpha_accum_bits,

	/* more ignored fields, thank you MS. */
	soggyPtr->red_accum_bits,
	soggyPtr->green_accum_bits,
	soggyPtr->blue_accum_bits,
	soggyPtr->alpha_accum_bits,
	
	/* depth buffer */
	soggyPtr->depth_bits,
	
	/* stencil buffer */
	soggyPtr->stencil_bits,
	
	0,
	PFD_MAIN_PLANE,
	0,
	0,
	0,
	0
    };
    int myPixelFormatID;
    HDC hdc;

    soggyPtr->hwnd = Tk_GetHWND( Tk_WindowId( soggyPtr->tkwin ) );
    
    /* get a DC for the window. */
    hdc = GetDC( soggyPtr->hwnd );

    /* look for a pixel format that matches the requests we put in
       the pixel format descriptor. */
    myPixelFormatID = ChoosePixelFormat( hdc, &pfd );
    if ( myPixelFormatID == 0 )
    {
	/* if no appropriate pixel format could be found, fail to create widget. */
	Tcl_SetResult( interp, "can't get appropriate visual", TCL_STATIC );
	return TCL_ERROR;
    }

    /* see what we actually got. */
    DescribePixelFormat( hdc, myPixelFormatID, sizeof( pfd ), &pfd );
    soggyPtr->red_bits = pfd.cRedBits;
    soggyPtr->green_bits = pfd.cGreenBits;
    soggyPtr->blue_bits = pfd.cBlueBits;
    soggyPtr->alpha_bits = pfd.cAlphaBits;
    soggyPtr->red_accum_bits = pfd.cAccumRedBits;
    soggyPtr->green_accum_bits = pfd.cAccumGreenBits;
    soggyPtr->blue_accum_bits = pfd.cAccumBlueBits;
    soggyPtr->alpha_accum_bits = pfd.cAccumAlphaBits;
    soggyPtr->depth_bits = pfd.cDepthBits;
    soggyPtr->stencil_bits = pfd.cStencilBits;
    
    /* set up the window with this pixel format. */
    SetPixelFormat( hdc, myPixelFormatID, &pfd );

    /* create a rendering context for this window. */
    soggyPtr->hglrc = wglCreateContext( hdc );
    if ( soggyPtr->hglrc == NULL )
    {
	Tcl_SetResult( interp, "failed to create rendering context", TCL_STATIC );
	return TCL_ERROR;
    }
    if ( last_created_context != NULL )
	wglShareLists( last_created_context, soggyPtr->hglrc );
    last_created_context = soggyPtr->hglrc;

    /* release the DC. */
    ReleaseDC( soggyPtr->hwnd, hdc );

    return TCL_OK;
}

#elif defined(USE_APPLE_AGL)
/* we don't use CGL; this is a newer interface in OS X that 
   doesn't seem to have the hooks needed to embed in a window. */
#define CLE(x) do { \
    CGLError nerr = x; \
    if(nerr != 0) { \
        printf("%s:%d - %s failed: %s\n", __FILE__, __LINE__,  \
               #x, CGLErrorString(nerr));  \
        exit(EXIT_FAILURE); \
    }  \
} while(0)
#define ale(str) do { \
    GLenum nerr = aglGetError(); \
    if(nerr != AGL_NO_ERROR) { \
        printf("%s:%d - %s failed: %s\n", __FILE__, __LINE__,  \
               str, aglErrorString(nerr));  \
        exit(EXIT_FAILURE); \
    }  \
} while(0)


static int OpenGLSetup( Soggy* soggyPtr, Tcl_Interp* interp )
{
    AGLPixelFormat pix;
    AGLDevice gdevs[2];
    /* long nvirt;  number of virtual screens */
    GLsizei refresh_rate;
    const GLint  attribs[] = 
        { AGL_RGBA, AGL_DOUBLEBUFFER,
#ifdef APPLE_FULLSCREEN
          AGL_FULLSCREEN,
#endif
          AGL_RED_SIZE, soggyPtr->red_bits,
          AGL_GREEN_SIZE, soggyPtr->green_bits,
          AGL_BLUE_SIZE, soggyPtr->blue_bits,
          AGL_ALPHA_SIZE, soggyPtr->alpha_bits,
          AGL_ACCUM_RED_SIZE, soggyPtr->red_accum_bits,
          AGL_ACCUM_GREEN_SIZE, soggyPtr->green_accum_bits,
          AGL_ACCUM_BLUE_SIZE, soggyPtr->blue_accum_bits,
          AGL_ACCUM_ALPHA_SIZE, soggyPtr->alpha_accum_bits,
          AGL_STENCIL_SIZE, soggyPtr->stencil_bits,
          AGL_DEPTH_SIZE, soggyPtr->depth_bits,
          None };

    /* choose the last device available.  The hopeful hack
       here is that there will be two devices, the internal
       and the external, or just one, mirrored on to the
       external display.  The second one may be the external
       in either case.  There should probably be a scheme
       for overriding this decision */
    /* alternatively, it may be good to call DMMirrorDevices
       to force the presentation into the right state. */
    GDHandle hDevice = NULL;
    GDHandle iDevice;
    for (iDevice = GetDeviceList(); 
         iDevice != NULL; 
         iDevice = GetNextDevice(hDevice)) {
        hDevice = iDevice;
        printf("has a device %d\n", (*hDevice)->gdRefNum);
    }
    // hDevice = GetMainDevice();
    
    assert(hDevice != NULL);
    gdevs[0] = hDevice;
    gdevs[1] = NULL;
    
    assert(soggyPtr != NULL);
    assert(interp != NULL);
    printf("attempting to create context for apple\n");
    ale("pre opengl setup");
    pix = aglChoosePixelFormat(gdevs, 1, attribs);
    ale("aglChoosePixelFormat");
    assert(pix != NULL);
    
    soggyPtr->pl.context = aglCreateContext(pix, last_created_context);
    ale("aglCreateContext");
    assert( soggyPtr->pl.context );
    
    assert(aglDescribePixelFormat( pix, AGL_RED_SIZE, &soggyPtr->red_bits) == GL_TRUE);
    assert(aglDescribePixelFormat( pix, AGL_GREEN_SIZE, &soggyPtr->green_bits) == GL_TRUE);
    assert(aglDescribePixelFormat( pix, AGL_BLUE_SIZE, &soggyPtr->blue_bits) == GL_TRUE);
    assert(aglDescribePixelFormat( pix, AGL_ALPHA_SIZE, &soggyPtr->alpha_bits) == GL_TRUE);
    assert(aglDescribePixelFormat( pix, AGL_ACCUM_RED_SIZE, &soggyPtr->red_accum_bits) == GL_TRUE);
    assert(aglDescribePixelFormat( pix, AGL_ACCUM_GREEN_SIZE, &soggyPtr->green_accum_bits) == GL_TRUE);
    assert(aglDescribePixelFormat( pix, AGL_ACCUM_BLUE_SIZE, &soggyPtr->blue_accum_bits) == GL_TRUE);
    assert(aglDescribePixelFormat( pix, AGL_ACCUM_ALPHA_SIZE, &soggyPtr->alpha_accum_bits) == GL_TRUE);
    assert(aglDescribePixelFormat( pix, AGL_STENCIL_SIZE, &soggyPtr->stencil_bits) == GL_TRUE);
    assert(aglDescribePixelFormat( pix, AGL_DEPTH_SIZE, &soggyPtr->depth_bits) == GL_TRUE);
    ale("aglDescribePixelFormat");
    
#ifdef APPLE_FULLSCREEN
    /* should really support configurable screen size -- if we
       don't change the video mode, X might be less likely to
       cash */
    /* some code from http://developer.apple.com/qa/qa2001/qa1209.html -- 
     which also describes full screen CGL... */
    
    // both are -1. printf("%d x %d", soggyPtr->glwidth, soggyPtr->glheight);
    // printf("%d x %d", soggyPtr->width, soggyPtr->height);
    // use run_presentation(screen_size=(1024,768)) for speed,
    // or (800,600) for likely portability.
    refresh_rate = 60; /* had been zero, which worked with only some projectors. */
    aglSetFullScreen (soggyPtr->pl.context, soggyPtr->width, 
                      soggyPtr->height, refresh_rate, 0);
    ale("aglSetFullScreen");
    { 
        /* just in case we asked for something agl wouldn't give (?),
           use the code suggested by apple */
        GLint displayCaps [3];
        aglGetInteger (soggyPtr->pl.context, AGL_FULLSCREEN, displayCaps);
        glViewport (0, 0, displayCaps[0], displayCaps[1]);
        // glViewport( 0, 0, soggyPtr->width, soggyPtr->height );
    }
#else
    /* code that doesn't quite work for running within a tk window. */
    // soggyPtr->pl.draw = TkMacOSXGetDrawablePort(Tk_WindowId(soggyPtr->tkwin));
    soggyPtr->pl.draw = TkMacOSXGetDrawablePort(soggyPtr->tkwin);
    //  GetWindowPort(soggyPtr->tkwin);
    assert( soggyPtr->pl.draw !=NULL);
    SetPort(GetWindowPort(soggyPtr->pl.draw));
    assert( aglSetDrawable(soggyPtr->pl.context, 
                           GetWindowPort(soggyPtr->pl.draw)) == GL_TRUE );
    ale("aglSetDrawable");
#endif
    
    aglDestroyPixelFormat( pix );
    ale("aglDestroyPixelFormat");
    
    last_created_context = soggyPtr->pl.context;
    return TCL_OK;
}


#else

/*
 * this is the X version of OpenGLSetup, which uses glx
 */

static int OpenGLSetup( Soggy* soggyPtr, Tcl_Interp* interp )
{
    int attributeList[] = { GLX_RGBA,
			    GLX_DOUBLEBUFFER,
			    GLX_RED_SIZE, soggyPtr->red_bits,
			    GLX_GREEN_SIZE, soggyPtr->green_bits,
			    GLX_BLUE_SIZE, soggyPtr->blue_bits,
			    GLX_ALPHA_SIZE, soggyPtr->alpha_bits,
			    GLX_ACCUM_RED_SIZE, soggyPtr->red_accum_bits,
			    GLX_ACCUM_GREEN_SIZE, soggyPtr->green_accum_bits,
			    GLX_ACCUM_BLUE_SIZE, soggyPtr->blue_accum_bits,
			    GLX_ACCUM_ALPHA_SIZE, soggyPtr->alpha_accum_bits,
			    GLX_STENCIL_SIZE, soggyPtr->stencil_bits,
			    GLX_DEPTH_SIZE, soggyPtr->depth_bits,
			    None };
    
    XVisualInfo* vi;
    Colormap cmap;

#if 0
    printf( "requesting %d %d %d %d  %d %d\n",
	    soggyPtr->red_bits,
	    soggyPtr->green_bits,
	    soggyPtr->blue_bits,
	    soggyPtr->alpha_bits,
	    soggyPtr->depth_bits,
	    soggyPtr->stencil_bits );
#endif
    
    soggyPtr->pl.cx = NULL;
    
    vi = glXChooseVisual( soggyPtr->dpy, Tk_ScreenNumber( soggyPtr->tkwin ), attributeList );
    if ( vi == NULL )
    {
	Tcl_SetResult( interp, "can't get appropriate visual", TCL_STATIC );
	return TCL_ERROR;
    }

    soggyPtr->pl.cx = glXCreateContext( soggyPtr->dpy, vi, last_created_context, GL_TRUE );
    if ( soggyPtr->pl.cx == NULL )
    {
	Tcl_SetResult( interp, "failed to create rendering context", TCL_STATIC );
	return TCL_ERROR;
    }
    last_created_context = soggyPtr->pl.cx;

    cmap = XCreateColormap( soggyPtr->dpy, XRootWindow( soggyPtr->dpy, Tk_ScreenNumber( soggyPtr->tkwin ) ),
			    vi->visual, AllocNone );
    Tk_SetWindowVisual( soggyPtr->tkwin, vi->visual, vi->depth, cmap );

    glXGetConfig( soggyPtr->dpy, vi, GLX_RED_SIZE, &soggyPtr->red_bits );
    glXGetConfig( soggyPtr->dpy, vi, GLX_ACCUM_RED_SIZE, &soggyPtr->red_accum_bits );
    glXGetConfig( soggyPtr->dpy, vi, GLX_GREEN_SIZE, &soggyPtr->green_bits );
    glXGetConfig( soggyPtr->dpy, vi, GLX_ACCUM_GREEN_SIZE, &soggyPtr->green_accum_bits );
    glXGetConfig( soggyPtr->dpy, vi, GLX_BLUE_SIZE, &soggyPtr->blue_bits );
    glXGetConfig( soggyPtr->dpy, vi, GLX_ACCUM_BLUE_SIZE, &soggyPtr->blue_accum_bits );
    glXGetConfig( soggyPtr->dpy, vi, GLX_ALPHA_SIZE, &soggyPtr->alpha_bits );
    glXGetConfig( soggyPtr->dpy, vi, GLX_ACCUM_ALPHA_SIZE, &soggyPtr->alpha_accum_bits );
    glXGetConfig( soggyPtr->dpy, vi, GLX_DEPTH_SIZE, &soggyPtr->depth_bits );
    glXGetConfig( soggyPtr->dpy, vi, GLX_STENCIL_SIZE, &soggyPtr->stencil_bits );

    return TCL_OK;
}
#endif

static int OpenGLWrapEval( Soggy* soggyPtr, Tcl_Obj* evalobj )
{
    int ret;
#ifdef WIN32
    HDC hdc = GetDC( soggyPtr->hwnd );
    wglMakeCurrent( hdc, soggyPtr->hglrc );
    ret = Tcl_GlobalEvalObj( soggyPtr->interp, evalobj );
    wglMakeCurrent( NULL, NULL );
    ReleaseDC( soggyPtr->hwnd, hdc );
#elif defined(USE_APPLE_AGL)
    assert(soggyPtr != NULL);
    assert(soggyPtr->pl.context != NULL);
    aglSetCurrentContext(soggyPtr->pl.context);
    aglUpdateContext(soggyPtr->pl.context);
    ret = Tcl_GlobalEvalObj( soggyPtr->interp, evalobj );
    aglSetCurrentContext(NULL);
#else
    
#if 0
    printf( "display: %08x\n", soggyPtr->dpy );
    printf( "windowid: %08x\n", Tk_WindowId( soggyPtr->tkwin ) );
    printf( "context: %08x\n", soggyPtr->pl.cx );
#endif
    glXWaitX();
    glXMakeCurrent( soggyPtr->dpy, Tk_WindowId( soggyPtr->tkwin ), soggyPtr->pl.cx );
    ret = Tcl_GlobalEvalObj( soggyPtr->interp, evalobj );
    glXMakeCurrent( Tk_Display( soggyPtr->tkwin ), None, NULL );
#endif
    return ret;
}
    
static void SoggyDisplay( ClientData clientdata )
{
    Soggy* soggyPtr = (Soggy*)clientdata;

    assert(soggyPtr != NULL);

    soggyPtr->updatePending = 0;
    
    if ( ! Tk_IsMapped( soggyPtr->tkwin ) )
	return;

    if ( soggyPtr->showcursor && !soggyPtr->cursorstate )
    {
	// turn the cursor on
#ifdef WIN32
	ShowCursor( 1 );
#else
	Tk_DefineCursor( soggyPtr->tkwin, None );
#endif
	soggyPtr->cursorstate = 1;
    }
    else if ( !soggyPtr->showcursor && soggyPtr->cursorstate )
    {
#ifdef WIN32
	ShowCursor( 0 );
#else
	Tk_DefineCursor( soggyPtr->tkwin, soggyPtr->pl.offcursor );
#endif
	soggyPtr->cursorstate = 0;
    }
    
    if ( soggyPtr->redrawPtr )
    {
#ifdef WIN32
	HDC hdc;
	hdc = GetDC( soggyPtr->hwnd );
	wglMakeCurrent( hdc, soggyPtr->hglrc );
	g_saveswap = hdc;
#elif defined(USE_APPLE_AGL)
        assert(soggyPtr != NULL);
        assert(soggyPtr->pl.context != NULL);
        aglSetCurrentContext(soggyPtr->pl.context);
        aglUpdateContext(soggyPtr->pl.context);
	g_saveswap = soggyPtr; /* I'd bet. */
#else
	glXMakeCurrent( soggyPtr->dpy, Tk_WindowId( soggyPtr->tkwin ), soggyPtr->pl.cx );
	g_saveswap = soggyPtr;
#endif

	glDrawBuffer( GL_BACK );
	
	Tcl_GlobalEvalObj( soggyPtr->interp, soggyPtr->redrawPtr );
	glFlush();

#ifdef WIN32
	wglMakeCurrent( NULL, NULL );
	ReleaseDC( soggyPtr->hwnd, hdc );
#elif defined(USE_APPLE_AGL)
        assert(soggyPtr != NULL);
        aglSetCurrentContext(NULL);
#else
	glXMakeCurrent( soggyPtr->dpy, None, NULL );
#endif
    }
}

void SoggySwap( void )
{
#ifdef WIN32
    SwapBuffers( g_saveswap );
#elif defined(USE_APPLE_AGL)
    aglSwapBuffers( g_saveswap->pl.context );
    ale ( "aglSwapBuffers");
#else
    if ( g_saveswap )
	glXSwapBuffers( g_saveswap->dpy, Tk_WindowId( g_saveswap->tkwin ) );
#endif
}

static void SoggyDestroy( char* data )
{
    Soggy* soggyPtr = (Soggy*)data;

#ifdef WIN32
    wglDeleteContext( soggyPtr->hglrc );
#elif defined(USE_APPLE_AGL)
    aglSetCurrentContext(NULL);
    if ( soggyPtr->pl.context )
      aglDestroyContext(soggyPtr->pl.context); 
#else
    glXMakeCurrent( soggyPtr->dpy, None, NULL );
    if ( soggyPtr->pl.cx )
	glXDestroyContext( soggyPtr->dpy, soggyPtr->pl.cx );
#endif
    
    Tk_FreeConfigOptions( (char*)soggyPtr, soggyPtr->optionTable, soggyPtr->tkwin );
    free( soggyPtr );
}

#ifdef _DEBUG
void Debug( char* fmt, ... )
{
    char buffer[1024];
    va_list list;

    va_start( list, fmt );
    vsprintf( buffer, fmt, list );
    va_end( list );

#ifdef WIN32
    OutputDebugString( buffer );
#else
    fputs( buffer, stderr );
#endif
}
#endif

/** Local Variables: **/
/** tab-width: 8 **/
/** c-indent-level: 4 **/
/** c-basic-offset: 4 **/
/** dsp-name:"slithy" **/
/** End: **/

/* vim:set ts=8: */
