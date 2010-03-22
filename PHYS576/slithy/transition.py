import math, anim

class InterpInt(int):
    def interp( self, other, frac ):
        v = self * (1.0-frac) + other * frac
        return int( v + 0.5 )
        

class TransitionStyle:
    def __init__( self, starttime, duration, start, end, params, type ):
        self.starttime = starttime
        self.duration = duration
        if type == 'integer':
            self.start = InterpInt(start)
            self.end = InterpInt(end)
        else:
            self.start = start
            self.end = end
        if not hasattr( self.start, 'interp' ) or not callable( self.start.interp ):
            self.range = end - start
        self.type = type

class _LinearTransition(TransitionStyle):
    def __call__( self, t ):
        if self.duration == 0.0:
            f = 1
        else:
            f = (t - self.starttime) / self.duration
            
        if hasattr( self, 'range' ):
            return self.start + self.range * f
        else:
            return self.start.interp( self.end, f )

class _SmoothTransition(TransitionStyle):
    def __init__( self, starttime, duration, start, end, params, type ):
        TransitionStyle.__init__( self, starttime, duration, start, end, params, type )
        self.s = params.get( 's', 0.0 )
        self.e = params.get( 'e', 0.0 )
            
    def __call__( self, t ):
        if self.duration == 0.0:
            u = 1
        else:
            u = (t - self.starttime) / self.duration
        f = u**3 + 3.0 * u * (1.0-u) * ( (1.0-self.e) * u + self.s * (1.0-u) )
        
        if hasattr( self, 'range' ):
            return self.start + self.range * f
        else:
            return self.start.interp( self.end, f )
    

class Transition:
    styles = { 'linear' : _LinearTransition,
               'smooth' : _SmoothTransition }
    
    def __init__( self, **params ):
        self.params = params

    def __call__( self, duration, var, v1, v2 = None, **params ):
        kw = self.params.copy()
        kw.update( params )

        type = var.type

        now = anim.get(var)
        if kw.has_key('rel') and kw['rel']:
            if v2 is None:
                start, end = now, now+v1
            else:
                start, end = now+v1, now+v2
        else:
            if v2 is None:
                start, end = now, v1
            else:
                start, end = v1, v2

        if not kw.has_key('style'):
            style = _LinearTransition
        else:
            try:
                style = Transition.styles[kw['style']]
            except KeyError:
                raise ValueError, "unknown transition style \"%s\"" % kw['style']

        starttime = anim.current_time()
        interp = style( starttime, duration, start, end, kw, type )
        if duration > 0.0:
            var.add_span( starttime, starttime+duration, interp )
        var.add_span( starttime+duration, None, end )
        anim.advance_clock( duration )


default = Transition()
