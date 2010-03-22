#include "sl_common.h"
#include "dr_draw.h"

static gstate* gstate_stack = NULL;
static int gstate_stack_alloc = 0;
static int gstate_stack_used = 0;

static drstate* drstate_stack = NULL;
static int drstate_stack_alloc = 0;
static int drstate_stack_used = 0;

FUNC_PY_NOARGS( draw_push )
{
    gstate* current_state;
    
    if ( gstate_stack_used > MAX_STACK_DEPTH )
    {
	PyErr_SetString( DrawError, "maximum state stack depth exceeded (unbalanced push/pop?)" );
	return NULL;
    }
    
    if ( gstate_stack_used + 1 > gstate_stack_alloc )
    {
	gstate_stack_alloc += STACK_DEPTH_CHUNK;
	gstate_stack = realloc( gstate_stack, gstate_stack_alloc * sizeof( gstate ) );
    }

    current_state = gstate_stack + gstate_stack_used;
    current_state->separator = 0;
    glPopMatrix();
    glGetDoublev( GL_MODELVIEW_MATRIX, current_state->transform );
    glPushMatrix();
    glTranslated( 0, 0, current_id_depth );
    glGetDoublev( GL_CURRENT_COLOR, current_state->color );
    current_state->linewidth = line_thickness;
    current_state->id = current_id;
    current_state->id_depth = current_id_depth;
    ++gstate_stack_used;
    
    Py_INCREF( Py_None );
    return Py_None;
}
	
FUNC_PY_NOARGS( draw_pop )
{
    gstate* current_state;
    
    if ( gstate_stack_used <= 0 || gstate_stack[gstate_stack_used-1].separator )
    {
	PyErr_SetString( DrawError, "state stack underflow" );
	return NULL;
    }

    --gstate_stack_used;
    current_state = gstate_stack + gstate_stack_used;
    
    glMatrixMode( GL_MODELVIEW );
    glPopMatrix();
    glLoadMatrixd( current_state->transform );
    glPushMatrix();
    glTranslated( 0, 0, current_state->id_depth );
    glDepthMask( (GLboolean)((current_state->id >= 0) ? GL_TRUE : GL_FALSE)  );
    
    glColor4dv( current_state->color );
    line_thickness = current_state->linewidth;
    current_id = current_state->id;
    current_id_depth = current_state->id_depth;
    
    current_IPM_dirty = 1;
    
    Py_INCREF( Py_None );
    return Py_None;
}

void initialize_stack( void )
{
    if ( gstate_stack_used + 1 > gstate_stack_alloc )
    {
	gstate_stack_alloc += STACK_DEPTH_CHUNK;
	gstate_stack = realloc( gstate_stack, gstate_stack_alloc * sizeof( gstate ) );
    }

    gstate_stack[gstate_stack_used].separator = 1;
    ++gstate_stack_used;
    
    if ( drstate_stack_used + 1 > drstate_stack_alloc )
    {
	drstate_stack_alloc += STACK_DEPTH_CHUNK;
	drstate_stack = realloc( drstate_stack, drstate_stack_alloc * sizeof( drstate ) );
    }

    drstate_stack[drstate_stack_used].camera = current_camera;
    drstate_stack[drstate_stack_used].visible = current_visible;
    drstate_stack[drstate_stack_used].viewport_aspect = viewport_aspect;
    drstate_stack[drstate_stack_used].global_alpha = global_alpha;
    drstate_stack[drstate_stack_used].auto_mark = auto_mark;
    drstate_stack[drstate_stack_used].context_name = context_name;
    ++drstate_stack_used;
}

void clear_stack( void )
{
    while ( gstate_stack_used > 0 && gstate_stack[gstate_stack_used-1].separator == 0 )
	--gstate_stack_used;

    if ( gstate_stack_used > 0 )
	--gstate_stack_used;

    --drstate_stack_used;
    current_camera = drstate_stack[drstate_stack_used].camera;
    current_visible = drstate_stack[drstate_stack_used].visible;
    viewport_aspect = drstate_stack[drstate_stack_used].viewport_aspect;
    global_alpha = drstate_stack[drstate_stack_used].global_alpha;
    auto_mark = drstate_stack[drstate_stack_used].auto_mark;
    context_name = drstate_stack[drstate_stack_used].context_name;
}
    


/** Local Variables: **/
/** dsp-name:"draw" **/
/** End: **/
