import sys, math, time

import system
import timeline
import transition
import viewport
import colors
import anim
import parameters
import rect
import os.path

from draw import *
from draw import _embed

try:
    import dxvideo
    video_available = 1
    video_select_keys = '0123456789'
except ImportError:
    video_available = 0

linear = transition.Transition()

exports = ['Drawable', 'Text', 'Image', 'Fill', 'BulletedList', 'Anim', 'Interactive', 'Video']

default_camera = rect.Rect(0, 0, 400, 0, 4.0 / 3.0)

class Element(object):
    def __str__( self ):
        return '<animation element>'

    def __getattr__( self, name ):
        if name == '_current_params' or name == '_params':
            raise AttributeError
        plist = [i[0] for i in getattr( self, '_current_params', self._params )]
        if name in plist:
            raise AttributeError, "'%s' element is not on stage; can't access '%s' parameter" % (self.__class__.__name__, name)
        else:
            raise AttributeError, "'%s' element has no '%s' parameter" % (self.__class__.__name__, name)
    
    def make_timelines( self, params = None ):
        if params is None:
            params = self._params
        self._current_params = params

        self._local_knobs = {}
        for name, ptype, default in params:
            self._local_knobs[name] = timeline.Timeline( ptype,
                                                         self._defaults.get( name, default ) )
        self.__dict__.update( self._local_knobs )

        if isinstance( self._viewport, viewport.AnimatedViewport ):
            self._viewport.make_timelines()
            self.viewport = self._viewport
            

    def finish_timelines( self ):
        basedict = {}
        timelines = []
        for name, var in self._local_knobs.iteritems():
            if var.is_trivial():
                basedict[name] = var.trivial_value()
            else:
                timelines.append( (name, var) )

        if isinstance( self._viewport, viewport.AnimatedViewport ):
            view = self._viewport.finish_timelines()
            del self.viewport
        else:
            view = None

        result = basedict, timelines, view

        for i in self._local_knobs:
            del self.__dict__[i]
        del self._local_knobs

        return result

    def cancel_timelines( self ):
        for i in self._local_knobs:
            del self.__dict__[i]
        del self._local_knobs

        if isinstance( self._viewport, viewport.AnimatedViewport ):
            self._viewport.cancel_timelines()
            del self.viewport

    def eval_finished( self, (basedict, timelines, view), t ):
        d = basedict.copy()
        for name, var in timelines:
            d[name] = var.eval( t )

        if isinstance( self._viewport, viewport.AnimatedViewport ):
            view = self._viewport.eval_finished( view, t )
        else:
            view = self._viewport

        return d, view

    def eval_unfinished( self, t ):
        d = {}
        for n, v in self._local_knobs.iteritems():
            d[n] = v.eval( t )

        if isinstance( self._viewport, viewport.AnimatedViewport ):
            view = self._viewport.eval_unfinished( t )
        else:
            view = self._viewport

        return d, view

    def no_render( self ):
        pass

class Video(Element):
    all_videos = []
    
    def __init__( self, _viewport, _filename, select = None, **_params ):
        self._params = ( ('fit', 'object', BOTH),
                         )

        bad = parameter_check( self._params, _params )
        if bad:
            raise ValueError, "no parameter '%s' for Video()" % (bad,)
        self._defaults = _params
        self._viewport = _viewport
        self._filename = unicode(os.path.normpath(_filename))
        self._selected = 1
        self._selectkeys = select

        self._instance = None
        self._visible = None
        Video.all_videos.append( self )

    if video_available:
        def render( self, d, view, t ):
            if hasattr(system.globals, 'size') and (not self._visible or self._visible != system.globals.size):
                print 'showing video window', self._instance
                dxvideo.place( self._instance, view, system.globals.size,
                               d['fit'] )
                self._selected = 1
                self._visible = system.globals.size

            if self._visible:
                system.event_receivers.register( self )

        def no_render( self ):
            if self._visible:
                print 'hiding video window', self._instance
                dxvideo.hide( self._instance )
                self._visible = None

        def event( self, t, ev ):
            if ev[0] == 'key':
                key, x, y, mod = ev[1:]

                if key == '??':
                    dxvideo.eventcheck( self._instance )

                if len(key) == 1 and key in video_select_keys and self._selectkeys is not None:
                    self._selected = (key in self._selectkeys)

                if self._selected:
                    if key == 'quoteleft':
                        if 'control' in mod:
                            dxvideo.loop( self._instance, 1 )
                        else:
                            dxvideo.loop( self._instance, 0 )
                        dxvideo.pause( self._instance, -1 )   # toggle
                    elif key == 'insert':
                        if 'control' in mod:
                            dxvideo.loop( self._instance, 1 )
                        else:
                            dxvideo.loop( self._instance, 0 )
                        dxvideo.pause( self._instance, 1 )
                    elif key == 'delete':
                        dxvideo.pause( self._instance, 0 )

                    elif key == 'left':
                        dxvideo.seek( self._instance, -1, 0 )
                    elif key == 'right':
                        dxvideo.seek( self._instance, +1, 0 )
                    elif key == 'home':
                        dxvideo.seek( self._instance, 0, 1 )
                    elif key == 'backspace':
                        dxvideo.seek( self._instance, 0, 1 )
                        dxvideo.pause( self._instance, 1 )

                    elif key == 'end':
                        dxvideo.rate( self._instance, 0 )
                    elif key == 'up':
                        dxvideo.rate( self._instance, +1 )
                    elif key == 'down':
                        dxvideo.rate( self._instance, -1 )

                    

        def create_instance( self ):
            self._instance = dxvideo.create( system.globals.soggy,
                                             self._filename )
            dxvideo.hide( self._instance )
    else:
        def render( self, d, view, t ):
            pass
        def no_render( self ):
            pass
        def create_instance( self ):
            pass

    def __str__( self ):
        return "<video window `%s`>" % (str(self._filename),)
                
        
                
                

class Interactive(Element):
    def __init__( self, _viewport, **_params ):
        self._params = ( ('_alpha', 'scalar', 1.0),
                         ('controller', 'object', None),
                         )
        bad = parameter_check( self._params, _params )
        if bad:
            raise ValueError, "no parameter '%s' for Interactive()" % (bad,)
        self._defaults = _params
        self._viewport = _viewport

        self._instance = None
        self._instance_start = None

    def render( self, d, view, t ):
        c = d['controller']
        if c is not None:
            now = time.time()
            if self._instance is None:
                self._instance = c()
                self._instance_start = now
                print 'instance is', self._instance
            t = now - self._instance_start
            _embed( view, 1, system.global_draw_object, (self._instance, {'t' : t}, view[4], d['_alpha']) )
            system.event_receivers.register( self )
            return 1
        else:
            if self._instance is not None:
                self._instance = None

    def no_render( self ):
        self._instance = None

    def event( self, t, ev ):
        if self._instance:
            system.global_send_event( self._instance, time.time() - self._instance_start, ev )

    
        

class Anim(Element):
    def __init__( self, _viewport, **_params ):
        self._params = ( ('_alpha', 'scalar', 1.0),
                         ('anim', 'object', None),
                         ('t', 'scalar', 0.0),
                         )

        bad = parameter_check( self._params, _params )
        if bad:
            raise ValueError, "no parameter '%s' for Anim()" % (bad,)
        self._defaults = _params
        self._viewport = _viewport

    def render( self, d, view, t ):
        if d['anim'] is not None:
            return _embed( view, 1, system.global_draw_object,
                           (d['anim'], {'t':d['t']}, view[4], d['_alpha']) )

    def __str__( self ):
        return '<Anim element>'

    def play( self, obj, pause = 1, fade = 0, fade_duration = 0.5, duration = None ):
        if not isinstance( obj, list ):
            obj = [obj]
            
        if len(obj) == 0:
            return

        if fade:
            self.fade_in_start( obj[0], fade_duration )
            if pause:
                anim.pause()

        anim.set( self.anim, obj[0] )
        if duration is None:
            d = obj[0].length
        else:
            d = duration
        linear( d, self.t, 0.0, obj[0].length )
        for i in obj[1:]:
            if pause:
                anim.pause()
            anim.set( self.anim, i )
            if duration is None:
                d = i.length
            else:
                d = duration
            linear( d, self.t, 0.0, i.length )

        if fade:
            if pause:
                anim.pause()
            self.fade_out_end( obj[-1], fade_duration )

    def show( self, obj, t = 0.0 ):
        if isinstance( obj, list ):
            obj = obj[0]
        print 'showing', obj
        anim.set( self.anim, obj )
        anim.set( self.t, t )

    def clear( self ):
        anim.set( self.anim, None )

    def fade_in_start( self, obj, duration ):
        if isinstance( obj, list ):
            obj = obj[0]
        anim.set( self.anim, obj )
        anim.set( self.t, 0.0 )
        linear( duration, self._alpha, 0.0, 1.0 )

    def fade_out_end( self, obj, duration ):
        if isinstance( obj, list ):
            obj = obj[-1]
        anim.set( self.anim, obj )
        anim.set( self.t, obj.length )
        linear( duration, self._alpha, 0.0 )
        

class Drawable(Element):
    def __init__( self, _viewport, _drawable = None, **_params ):
        self._viewport = _viewport
        self._drawable = _drawable
        self._defaults = _params
        self._params = None
        
    def make_timelines( self ):
        if not self._drawable:
            return
        dr = self._drawable

        params = system.get_object_params( dr )
        params.append( ('_alpha', parameters.SCALAR, 1.0) )
        Element.make_timelines( self, params )

    def finish_timelines( self ):
        return Element.finish_timelines( self ), self._drawable

    def eval_finished( self, stuff, t ):
        d, view = Element.eval_finished( self, stuff[0], t )
        d['_drawable'] = stuff[1]
        return d, view

    def eval_unfinished( self, t ):
        d, view = Element.eval_unfinished( self, t )
        d['_drawable'] = self._drawable
        return d, view
        
    def render( self, d, view, t ):
        alpha = d['_alpha']
        del d['_alpha']
        drawable = d['_drawable']
        del d['_drawable']

        _embed( view, 1, system.global_draw_object, (drawable, d, view[4], alpha) )

    def __str__( self ):
        return '<drawable element: %s>' % (self._drawable.func_name,)
    

class Image(Element):
    anchors = { 'c' : (0.5, 0.5),
                'n' : (0.5, 1.0),
                's' : (0.5, 0.0),
                'w' : (0.0, 0.5),
                'e' : (1.0, 0.5),
                'sw' : (0.0, 0.0),
                'nw' : (0.0, 1.0),
                'ne' : (1.0, 1.0),
                'se' : (1.0, 0.0) }
    
    def __init__( self, _viewport, **_params ):
        self._params = ( ('image', 'object', None),
                         ('_alpha', 'scalar', 1.0),
                         ('fit', 'object', BOTH),
                         ('anchor', 'object', 'c') )

        bad = parameter_check( self._params, _params )
        if bad:
            raise ValueError, "no parameter '%s' for Image()" % (bad,)
        self._defaults = _params
        self._viewport = _viewport

    def __str__( self ):
        return '<image element>'
    
    def render( self, d, view, t ):
        push()
        try:
            ox, oy, ulen, utheta, aspect = view
            if utheta == 0.0:
                x = ox
                y = oy
            else:
                translate( ox, oy )
                rotate( utheta )
                x = 0
                y = 0
                
            a = Image.anchors.get( d['anchor'], (0.5, 0.5) )

            im = d['image']
            f = d['fit']
            
            if f == BOTH:
                if aspect > im.aspect:
                    f = HEIGHT
                else:
                    f = WIDTH
            
            if f == WIDTH:
                fd = { 'width' : ulen }
                y += a[1] * ulen * (1.0 / aspect - 1.0 / im.aspect)
            elif f == HEIGHT:
                fd = { 'height' : ulen / aspect }
                x += a[0] * ulen * (1.0 - im.aspect / aspect)
            else:
                fd = { 'width' : ulen, 'height' : ulen / aspect }

            image( x, y, im, anchor = 'sw', alpha = d['_alpha'], **fd )
            
        finally:
            pop()
        

class BulletedList(Element):
    def __init__( self, _viewport, **_params ):
        initparams = ( ('font', 'object', None),
                       ('color', 'color', colors.black),
                       ('bullet', 'object', '-'),
                       ('bulletsep', 'scalar', 0.5),
                       ('size', 'scalar', 1.0),
                       ('sizescale', 'scalar', 0.8),
                       ('indent', 'scalar', 1.0),
                       ('leading', 'scalar', 0.5),
                       ('_alpha', 'scalar', 1.0),
                       ('show', 'scalar', 0.0),
                       )

        bad = parameter_check( initparams, _params )
        if bad:
            raise ValueError, "no parameter '%s' for BulletedList()" % (bad,)

        self._startitemlist = []
        moreparams = ( ('items', 'object', self._startitemlist),
                       )

        self._params = initparams + moreparams
        self._defaults = _params
        self._viewport = _viewport

    def add_item( self, level, text, duration = 0.0, trans = transition.default ):
        if anim.defining_animation():
            before = anim.get( self.items )
            after = before + [(level,text)]
            anim.set( self.items, after )
            if duration:
                trans( duration, self.show, len(after) )
            else:
                anim.set( self.show, len(after) )
        else:
            self._startitemlist.append( (level, text) )

    def remove_item( self, duration = 0.0, trans = transition.default ):
        if anim.defining_animation():
            before = anim.get( self.items )
            if not before:
                return
            if duration:
                trans( duration, self.show, len(before)-1 )
            anim.set( self.items, before[:-1] )
        else:
            self._startitemlist.pop()
            
    def render( self, d, view, t ):
        push()
        try:
            items = d['items']
            
            ox, oy, ulen, utheta, aspect = view
            if utheta == 0.0:
                x = ox
                y = oy + ulen / aspect
            else:
                translate( ox, oy )
                rotate( utheta )
                x = 0
                y = ulen / aspect
                
            basesize = d['size']
            sizescale = d['sizescale']
            indent = d['indent'] * basesize
            leading = d['leading'] + 1
            font = d['font']
            bullet = d['bullet']
            bulletsep = d['bulletsep']
            alpha = d['_alpha']
            show = d['show']
            
            scales = [(basesize, 0.0)]
            
            color( d['color'] )

            ypos = None
            k = 0
            for l, t in items:
                if show < k:
                    break
                if int(show) == k:
                    showalpha = show - k
                else:
                    showalpha = 1
                k += 1
                
                while len(scales) <= l:
                    s, i = scales[-1]
                    scales.append( (s * sizescale, i + indent) )

                s, i = scales[l]
                if ypos is None:
                    ypos = y - s
                else:
                    ypos -= leading * s

                if bullet:
                    p = text( x + i, ypos, bullet, font, size = s, anchor = 'fw', _alpha = alpha * showalpha )
                    i += p['width'] + bulletsep * s
                p = text( x + i, ypos, t, font, size = s, anchor = 'fw', wrap = ulen - i, _alpha = alpha * showalpha )
                ypos -= (p['lines']-1) * s 
                
        finally:
            pop()
        
    def __str__( self ):
        return '<bulletedlist element>'
    
        
        
                         
            
        

class Text(Element):
    def __init__( self, _viewport, **_params ):
        self._params = ( ('text', 'string', ''),
                         ('color', 'color', colors.black),
                         ('_alpha', 'scalar', 1.0),
                         ('font', 'object', None),
                         ('size', 'scalar', 1.0),
                         ('justify', 'scalar', 0.0),
                         ('vjustify', 'scalar', 0.0),
                         ('show_viewport', object, None),
                         )
        
        bad = parameter_check( self._params, _params )
        if bad:
            raise ValueError, "no parameter '%s' for Text()" % (bad,)

        self._defaults = _params
        self._viewport = _viewport
        
    def render( self, d, view, t ):
        push()
        try:
            if d['show_viewport']:
                color( d['color'], 0.1 )
                rectangle( view )
            
            ox, oy, ulen, utheta, aspect = view
            if utheta == 0.0:
                x = ox
                y = oy + ulen / aspect
            else:
                translate( ox, oy )
                rotate( utheta )
                x = 0
                y = ulen / aspect
                
            size = d['size']
            color( d['color'] )
            vert = d['vjustify']
            if vert == 0.0:
                text( x, y - size, d['text'], d['font'],
                           size = size, justify = d['justify'], anchor = 'fw',
                           wrap = ulen, _alpha = d['_alpha'] )
            else:
                bbox = text( x, y - size, d['text'], d['font'],
                             size = size, justify = d['justify'], anchor = 'fw',
                             wrap = ulen, _alpha = d['_alpha'], nodraw = 1 )
                y -= vert * ((ulen/aspect) - size * bbox['lines']) + size
                text( x, y, d['text'], d['font'],
                      size = size, justify = d['justify'], anchor = 'fw',
                      wrap = ulen, _alpha = d['_alpha'] )
        finally:
            pop()
        
    def __str__( self ):
        return '<text element>'
    


class Fill(Element):
    def __init__( self, _viewport = None, **_params ):
        self._params = ( ('style', 'string', 'solid'),
                         ('color', 'color', colors.black),
                         ('color2', 'color', colors.black),
                         ('_alpha', 'scalar', 1.0) )

        bad = parameter_check( self._params, _params )
        if bad:
            raise ValueError, "no parameter '%s' for Fill()" % (bad,)
        self._defaults = _params
        self._viewport = _viewport

    def render( self, d, view, t ):
        if view is None:
            view = visible()

        push()
        try:
            style = d['style']
            if style == 'solid':
                color( d['color'], d['_alpha'] )
                rectangle( view )
            elif style == 'horz':
                alpha = d['_alpha']
                top = colors.Color( d['color'], alpha )
                bottom = colors.Color( d['color2'], alpha )
                rectangle( view, sw = bottom, se = bottom, nw = top, ne = top )
            elif style == 'vert':
                alpha = d['_alpha']
                left = colors.Color( d['color'], alpha )
                right = colors.Color( d['color2'], alpha )
                rectangle( view, sw = left, se = right, nw = left, ne = right )
            else:
                raise ValueError, "Fill() element doesn't understand style '%s'" % (style,)

        finally:
            pop()
        
    def __str__( self ):
        return '<fill element>'
    
    

def parameter_check( legal, d ):
    names = [i[0] for i in legal]
    for k in d.iterkeys():
        if k not in names:
            return k
    return None
        
# class Camera(object):
#     def __init__( self, start = None ):
#         if start is None:
#             start = default_camera
#         self.start = start

#     def make_timelines( self ):
#         start = self.start
        
#         self.timeline = ( timeline.Timeline( 'scalar', start[0] ),
#                           timeline.Timeline( 'scalar', start[1] ),
#                           timeline.Timeline( 'scalar', start[2] ),
#                           timeline.Timeline( 'scalar', start[3] ),
#                           timeline.Timeline( 'scalar', start[4] ) )

#     def finish_timelines( self ):
#         triv = 1
#         for i in self.timeline:
#             if not i.is_trivial():
#                 triv = 0
#             break

#         if triv:
#             baseval = rect.Rect([v.trivial_value() for v in self.timeline])
#             result = baseval, None
#         else:
#             result = None, self.timeline
                
#         del self.timeline
#         return result

#     def cancel_timelines( self ):
#         del self.timeline

#     def eval_finished( self, (baseval, tl), t ):
#         if baseval is None:
#             return rect.Rect([i.eval(t) for i in tl])
#         else:
#             return baseval

#     def eval_unfinished( self, t ):
#         return rect.Rect([i.eval(t) for i in self.timeline])
            
#     def view( self, rect, duration = 0.0, trans = transition.default ):
#         if duration <= 0.0:
#             for v, t in zip( rect, self.timeline ):
#                 anim.set( t, v )
#         else:
#             anim.parallel()
#             for v, t in zip( rect, self.timeline ):
#                 trans( duration, t, v )
#             anim.end()
            
class Camera(object):
    def __init__( self, start = None ):
        if start is None:
            start = default_camera
        self.start = start

    def make_timelines( self ):
        self.rect = timeline.Timeline( 'rectangle', self.start )

    def finish_timelines( self ):
        if self.rect.is_trivial():
            result = self.rect.trivial_value(), None
        else:
            result = None, (self.rect,)
                
        del self.rect
        return result

    def cancel_timelines( self ):
        del self.rect

    def eval_finished( self, (baseval, tl), t ):
        if baseval is None:
            return tl[0].eval(t)
        else:
            return baseval

    def eval_unfinished( self, t ):
        return self.rect.eval(t)
            
    def view( self, rect, duration = 0.0, trans = transition.default ):
        if duration <= 0.0:
            anim.set( self.rect, rect )
        else:
            trans( duration, self.rect, rect )
            
                
        
    
        
            
