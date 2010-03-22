import math, anim

class UndulationStyle:
    def __init__( self, starttime, period, min, max ):
        self.starttime = starttime
        self.period = period
        self.min = min
        self.max = max
        if not hasattr( self.min, 'interp' ) or not callable( self.min.interp ):
            self.range = max - min

class _SineWave(UndulationStyle):
    def __call__( self, t ):
        u = (t - self.starttime) / self.period
        f = 0.5 + 0.5 * math.sin( u * 2 * math.pi )
        if hasattr( self, 'range' ):
            return self.min + self.range * f
        else:
            return self.min.interp( self.max, f )

class _SawtoothUpWave(UndulationStyle):
    def __call__( self, t ):
        f, junk = math.modf( (t - self.starttime) / self.period )
        if hasattr( self, 'range' ):
            return self.min + self.range * f
        else:
            return self.min.interp( self.max, f )

class _SawtoothDownWave(UndulationStyle):
    def __call__( self, t ):
        f, junk = math.modf( (t - self.starttime) / self.period )
        if hasattr( self, 'range' ):
            return self.min + self.range * (1.0-f)
        else:
            return self.min.interp( self.max, 1.0-f )

class _TriangleWave(UndulationStyle):
    def __call__( self, t ):
        f = math.modf( (t - self.starttime) / self.period )[0] * 2
        f = 1-abs(1-f)
        if hasattr( self, 'range' ):
            return self.min + self.range * f
        else:
            return self.min.interp( self.max, f )
    

class Undulation:
    styles = { 'sin' : _SineWave,
               'sawup' : _SawtoothUpWave,
               'sawdown' : _SawtoothDownWave,
               'tri' : _TriangleWave }
    
    def __init__( self, **params ):
        self.params = params

    def parse_dictionary( self, kw ):
        if not kw.has_key( 'style' ):
            # the default style
            style = _SineWave
        else:
            try:
                style = Undulation.styles[kw['style']]
            except KeyError:
                raise ValueError, "unknown undulation style \"%s\"" % kw['style']
            
        d = kw.has_key( 'duration' )
        p = kw.has_key( 'period' )
        c = kw.has_key( 'cycles' )
        
        if d:
            duration = kw['duration']
            if p and not c:
                period = kw['period']
            elif c and not p:
                period = duration / kw['cycles']
            else:
                raise ValueError, "undulation must specify exactly two of {period,duration,cycles} or period only"
        elif p and c:
            period = kw['period']
            duration = period * kw['cycles']
        elif p:
            period = kw['period']
            duration = None
        else:
            raise ValueError, "undulation must specify exactly two of {period,duration,cycles} or period only"

        n = kw.has_key( 'min' )
        x = kw.has_key( 'max' )
        m = kw.has_key( 'mean' )
        a = kw.has_key( 'amplitude' )

        if n + x + m + a != 2:
            raise ValueError, "undulation must specify exactly two of {min,max,mean,amplitude}"

        if n:
            min = kw['min']
            if x:
                max = kw['max']
            elif m:
                max = 2 * kw['mean'] - min
            else:
                max = min + 2 * kw['amplitude']
        elif x:
            max = kw['max']
            if m:
                min = 2 * kw['mean'] - max
            else:
                min = max - 2 * kw['amplitude']
        else:
            min = kw['mean'] - kw['amplitude']
            max = kw['mean'] + kw['amplitude']

        if kw.has_key( 'relative' ) and kw['relative']:
            min += anim.get(var)

        return style, period, duration, min, max
        
    def __call__( self, var, **overridekw ):
        kw = self.params.copy()
        kw.update( overridekw )

        style, period, duration, min, max = self.parse_dictionary( kw )

        starttime = anim.current_time()
        interp = style( starttime, period, min, max )
        if duration is None:
            # run undulator from now until end of time
            var.add_span( starttime, None, interp )
        else:
            # run undulator for finite time and then stop
            var.add_span( starttime, starttime+duration, interp )
            var.add_span( starttime+duration, None, interp(starttime+duration) )
            anim.advance_clock( duration )

#      def get_interpolator( self, start, overridekw ):
#          kw = self.params.copy()
#          kw.update( overridekw )
#          style, period, duration, min, range = self.parse_dictionary( kw )
#          return style( start, period, min, range )
    
