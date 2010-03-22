import timeline
import sys
import types
import error
import draw
import objects
import inspect
from rect import Rect

current = None

class Animation(object):
    def __init__( self, initial_cast, startcamera = None, name = None ):
        self.name = name
        self.icon = None

        self.stack = [ ['base'], ['sequence', 0.0, 0.0] ]

        self.camera = objects.Camera( startcamera )
        self.camera.make_timelines()
        
        self.partial_start = 0.0
        self.partial_used = 0

        self.cast = []
        self.working_set = timeline.Timeline( 'tuple', () )
        self.extras = timeline.Timeline( 'tuple', () )

        self.enter( initial_cast )

    def __repr__( self ):
        if self.name is None:
            return object.__repr__( self )
        else:
            return '<animation object "%s">' % (self.name,)

    def mode( self ):
        return self.stack[-1][0]

    def time( self ):
        top = self.stack[-1]
        if top[0] == 'sequence':
            return top[2]
        elif top[0] == 'parallel':
            return top[1]
        else:
            raise RuntimeError, 'unknown animation stack element type'

    def restack_modifier( self, movers, ref, abovebelow, old ):
        old = list(old)

        if ref is None or ref not in old:
            if abovebelow:
                split = len( old )
            else:
                split = 0
        else:
            split = old.index( ref )
            if abovebelow:
                split += 1
        
        index = [0] * len(old)
        for m in movers:
            try:
                i = old.index( m )
            except ValueError:
                pass
            index[i] = 1

        left = []
        center = []
        right = []

        for m, i in zip(old[:split],index[:split]):
            if i:
                center.append( m )
            else:
                left.append( m )
        for m, i in zip(old[split:],index[split:]):
            if i:
                center.append( m )
            else:
                right.append( m )

        newws = left + center + right
        return tuple(newws)
        
    def restack( self, movers, ref, abovebelow ):
        self.working_set.merge_span( self.time(), None,
                                     lambda x: self.restack_modifier( movers, ref, abovebelow, x ) )

            
    def enter_modifier( self, newmembers, old ):
        old = list(old)
        for i in newmembers:
            if i not in old:
                old.append( i )
                if i not in self.cast:
                    self.cast.append( i )
                    i.make_timelines()
        return tuple(old)
    
    def enter( self, members ):
        self.working_set.merge_span( self.time(), None, lambda x: self.enter_modifier( members, x ) )

    def exit_modifier( self, toremove, old ):
        old = list(old)
        for i in toremove:
            if i in old:
                old.remove( i )
        return tuple(old)

    def exit( self, members ):
        self.working_set.merge_span( self.time(), None, lambda x: self.exit_modifier( members, x ) )

    ##
    ## manage animation extras
    ##

    def add_extra_modifier( self, toadd, old ):
        return old + (toadd,)
    
    def add_extra( self, obj ):
        self.extras.merge_span( self.time(), None, lambda x: self.add_extra_modifier( obj, x ) )

    def remove_extra_modifier( self, toremove, old ):
        old = list(old)
        try:
            old.remove( toremove )
        except ValueError:
            pass
        return tuple(old)

    def remove_extra( self, obj ):
        self.extras.merge_span( self.time(), None, lambda x: self.remove_extra_modifier( obj, x ) )

    def finish( self ):
        # close any outstanding sequence()s or parallel()s
        while len(self.stack) > 2:
            end()

        # total length of the animation is the last thing we need from
        # the stack
        self.length = self.stack[-1][2]
        del self.stack

        # let each cast member package up its timelines
        self.timelines = {}
        for member in self.cast:
            self.timelines[member] = member.finish_timelines()
        self.timelines[self.camera] = self.camera.finish_timelines()

        if self.icon is None:
            self.icon = self.length / 2.0

    def cancel( self ):
        del self.stack
        for member in self.cast:
            member.cancel_timelines()
        self.camera.cancel_timelines()
        

    def render( self, t, aspect, alpha ):
        camera = self.camera.eval_finished( self.timelines[self.camera], t )
        #print 'anim pre'
        draw._pre_drawing( aspect, alpha, self.name )
        keepalive = 0
        try:
            draw.set_camera( camera )
            torender = self.working_set.eval( t )
            for i in torender:
                d, view = i.eval_finished( self.timelines[i], t )
                if view is None: view = draw.visible()
                if i.render( d, view, t ):
                    keepalive = 1
                
            for i in self.cast:
                if i not in torender:
                    i.no_render()
        finally:
            draw._post_drawing()
        return keepalive
 
class Interaction(Animation):
    def __init__( self, controller, startcamera = None ):
        self.controller = controller
        self.name = 'foo'
        
        self.camera = objects.Camera( startcamera )
        self.camera.make_timelines()

        self.cast = []
        self.working_set = timeline.Timeline( 'tuple', () )
        
        self.stack = [ ['base'], ['sequence', 0.0, 0.0] ]

    def set_time( self, t ):
        self.stack = [ ['base'], ['sequence', t, t] ]

    def render( self, t, aspect, alpha ):
        camera = self.camera.eval_unfinished( t )
        #print 'inter pre'
        draw._pre_drawing( aspect, alpha, self.name )
        try:
            draw.set_camera( camera )
            viewall = Rect( draw.visible() )
            for i in self.working_set.eval( t ):
                d, view = i.eval_unfinished( t )
                if view is None:
                    view = viewall
                i.render( d, view, t )
        finally:
            draw._post_drawing()

    def event( self, t, ev ):
        global current
        if not hasattr( self.controller, ev[0] ):
            return

        self.set_time( t )
        current = self
        getattr( self.controller, ev[0] )( *ev[1:] )
        while len( self.stack ) > 2:
            end()
        current = None
        

class PartialAnimation(Animation):
    def __init__( self, anim, starttime, endtime, name = None ):
        self.name = name
        self.length = endtime - starttime
        self.anim = anim
        self.starttime = starttime
        self.endtime = endtime
        if anim.icon is None:
            self.icon = self.length / 2
        else:
            self.icon = anim.icon - starttime
            anim.icon = None

    def render( self, t, aspect, alpha ):
        if t > self.length:
            t = self.length
        t += self.starttime

        return self.anim.render( t, aspect, alpha )

class ScaledAnimation(Animation):
    def __init__( self, anim, newlength ):
        if anim.name:
            self.name = anim.name + ' (%.2f sec)' % (newlength,)
        self.anim = anim
        if newlength == 0.0:
            self.scale = 0
        else:
            self.scale = anim.length / newlength 
        self.length = newlength
        self.icon = newlength / 2

    def render( self, t, aspect, alpha ):
        return self.anim.render( t * self.scale, aspect, alpha )

class StackedAnimation(Animation):
    def __init__( self, *anims, **keywd ):
        if not anims:
            raise ValueError, 'StackedAnimation must contain at least one animation' 
        
        self.name = keywd.get( 'name', '<%d stacked animations>' % (len(anims),) )
        align = keywd.get( 'align', 0.0 )

        maxlen = anims[0].length
        for i in anims:
            if maxlen < i.length:
                maxlen = i.length

        a = [(i, align * (maxlen - i.length)) for i in anims]
        self.anims = tuple(a)
        self.length = maxlen
        

    def render( self, t, aspect, alpha ):
        for anim, offset in self.anims:
            ta = t - offset
            if ta < 0.0:
                ta = 0.0
            elif ta > anim.length:
                ta = anim.length
            anim.render( ta, aspect, alpha )

stack = StackedAnimation
        
        
class GluedAnimation(Animation):
    def __init__( self, *anims ):
        self.name = ''
        self.anims = anims
        dur = 0.0
        self.breaks = [0.0]
        for i in anims:
            dur += i.length
            self.breaks.append( dur )
        self.length = dur
        self.icon = self.length / 2

    def draw_at_time( self, now ):
        i = 0
        for j in self.breaks[1:]:
            if now < j:
                return self.anims[i].draw_at_time( now - self.breaks[i] )
            i += 1
        return self.anims[-1].draw_at_time( now - self.breaks[-2] )

    def __str__( self ):
        return '<' + ','.join( [str(i) for i in self.breaks] ) + '>'
    
def start_animation( *cast, **keywd ):
    global current
    if current is not None:
        raise RuntimeError, "can't start animation before finishing the last one"

    if 'name' in keywd:
        name = keywd['name']
        if not isinstance( name, str ):
            raise ValueError, "animation name must be string"
    else:
        try:
            name = inspect.stack()[1][3]
        except IndexError:
            name = '<unnamed>'

    startcamera = keywd.get( 'camera', None )

    current = Animation( cast, startcamera, name )

    current.partials = None

    return current.camera

def cancel_animation():
    global current
    if current:
        current.cancel()
    current = None

def end_animation():
    global current
    if current is None:
        raise RuntimeError, "must start animation before finishing it"

    if current.partials is not None:
        pause()
    
    current.finish()

    if current.partials is None:
        result = [current]
    else:
        count = len(current.partials)
        for p, n in zip( current.partials, range(1,count+1)):
            p.name = '%s <%d/%d>' % (current.name, n, count)
        result = current.partials

    current = None
    return result
                 
def pause():
    global current
    if current is None:
        raise RuntimeError, "pause() only works while defining an animation"
    
    t = current_time()
    a = PartialAnimation( current, current.partial_start, t )
    wait( 0.0001 )
    current.partial_start = current_time()
    current.partial_used = 1

    if current.partials is None:
        current.partials = [a]
    else:
        current.partials.append( a )

    return a

def icon( rel = 0.0 ):
    global current
    current.icon = current_time() + rel

def set( var, val ):
    var.add_span( current_time(), None, val )

def enter( *objs ):
    global current
    current.enter( objs )

def exit( *objs ):
    global current
    current.exit( objs )

def lift( *objs, **kw ):
    global current
    if kw.has_key( 'above' ):
        current.restack( list(objs), kw['above'], 1 )
    else:
        current.restack( list(objs), None, 1 )

def lower( *objs, **kw ):
    global current
    if kw.has_key( 'below' ):
        current.restack( list(objs), kw['below'], 0 )
    else:
        current.restack( list(objs), None, 0 )
    
        
def current_time():
    global current
    return current.time()
        
def advance_clock( duration ):
    global current
    top = current.stack[-1]
    if top[0] == 'sequence':
        top[2] += duration
    elif top[0] == 'parallel':
        top[2] = max( top[2], top[1] + duration )
    else:
        raise RuntimeError, 'unknown _stack element type'
    
wait = advance_clock   # the user-level synonym

def sequence( w = None ):
    global current
    t = current_time()
    current.stack.append( [ 'sequence', t, t ] )
    if w is not None:
        wait( w )

serial = sequence

def parallel():
    global current
    t = current_time()
    current.stack.append( [ 'parallel', t, t ] )

def end():
    global current
    if current.stack[-2][0] == 'base':
        raise RuntimeError, 'too many end()s!'
    top = current.stack.pop()
    duration = top[2] - top[1]
    advance_clock( duration )

def get( timeline, t = None ):
    if t is None:
        t = current_time()
    return timeline.eval( t )

def defining_animation():
    return current is not None

def get_camera():
    global current
    if current is None:
        return objects.default_camera
    else:
        return current.camera.eval_unfinished( current.time() )
    
def get_camera_object():
    global current
    return current.camera
    
def scale_length( anim, newlength ):
    return ScaledAnimation( anim, newlength )

def onstage( t = None ):
    if t is None:
        t = current_time()
    return current.working_set.eval( t )
