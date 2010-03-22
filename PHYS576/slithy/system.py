import sys, types
import anim
import controller
import draw
import colors
import parameters

def global_draw_object( obj, pdict, aspect, alpha ):
    #print 'GDO: %.2f %s' % (aspect, obj)
    
    if isinstance( obj, anim.Animation ):
        return obj.render( pdict['t'], aspect, alpha )

    elif isinstance( obj, controller.Controller ):
        obj._anim.render( pdict['t'], aspect, alpha )

    elif isinstance( obj, tuple ):
        draw._do_gldrawing( obj[0], pdict, aspect, alpha )
        
    elif callable( obj ):
        # object must be a parameterized diagram
        try:
            #print 'diagram pre'
            draw._pre_drawing( aspect, alpha, obj.func_name )
            obj( **pdict )
        finally:
            draw._post_drawing()

    else:
        raise ValueError, ('global_draw_object on object', obj)

def global_send_event( obj, t, ev ):
    if isinstance( obj, controller.Controller ):
        obj._anim.event( t, ev )


def get_object_params( obj ):
    if isinstance( obj, anim.Animation ):
        return [('t', parameters.SCALAR, 0.0)]

    elif isinstance( obj, controller.Controller ) or (isinstance(obj,type) and issubclass(obj,controller.Controller)):
        return [('t', parameters.SCALAR, 0.0)]

    elif isinstance( obj, tuple ):
        return [i[:3] for i in obj[1]]
    
    elif callable( obj ):
        return parameters.analyze_diagram_parameters( obj )

    else:
        raise ValueError, ('get_object_params on object', obj)

def get_object_param_ranges( obj ):
    if isinstance( obj, anim.Animation ):
        return { 't' : (0.0, obj.length) }

    elif isinstance( obj, controller.Controller ) or (isinstance(obj,type) and issubclass(obj,controller.Controller)):
        return { 't' : (0.0, None) }

    elif isinstance( obj, tuple ):
        return parameters.gldiagram_parameter_ranges( obj )

    elif callable( obj ):
        return parameters.diagram_parameter_ranges( obj )

    else:
        raise ValueError, ('get_object_param_ranges on object', obj)

def complete_parameter_dict( obj, dict ):
    d = dict
    
    if isinstance( obj, anim.Animation ) or isinstance( obj, controller.Controller ) or \
           (isinstance(obj,type) and issubclass(obj,controller.Controller)):
        if 't' not in dict:
            d = dict.copy()
            d['t'] = 0.0
        
    elif isinstance( obj, tuple ):     # GL diagram
        d = dict
        for i in obj[1]:
            if i[0] not in d:
                if d is dict:
                    d = dict.copy()
                d[i[0]] = i[-1]
        
    elif callable( obj ):              # regular diagram
        d = dict
        for i in parameters.analyze_diagram_parameters(obj):
            if i[0] not in d:
                if d is dict:
                    d = dict.copy()
                d[i[0]] = i[-1]

    else:
        raise ValueError, ('complete_parameter_dict on object', obj)

    return d

draw._set_hooks( global_draw_object, complete_parameter_dict, colors.Color )        

class _EventReceiver:
    def __init__( self ):
        self.receivers = []

    def reset( self ):
        self.receivers = []

    def register( self, obj ):
        self.receivers.append( obj )

    def send_event( self, t, ev ):
        for i in self.receivers:
            i.event( t, ev )

event_receivers = _EventReceiver()

class Struct:
    pass
globals = Struct()

def parse_visual_arg( s ):
    try:
        red, green, blue, alpha, depth, stencil = [int(i) for i in s.split('/')]
    except ValueError:
        raise ValueError, 'bad visual specification: "%s"' % (s,)

    return { 'red' : red,
             'green' : green,
             'blue' : blue,
             'alpha' : alpha,
             'depth' : depth,
             'stencil' : stencil }

    
             
        
        
