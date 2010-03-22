import draw
import timeline
from rect import Rect

class AnimatedViewport(object):
    def make_timelines( self ):
        if hasattr( self, '_local_knobs' ):
            return
        
        self._local_knobs = {}
        for name, ptype, default in self.params:
            self._local_knobs[name] = timeline.Timeline( ptype,
                                                         self._defaults.get( name, default ) )
        self.__dict__.update( self._local_knobs )

    def finish_timelines( self ):
        basedict = {}
        timelines = []
        for name, var in self._local_knobs.iteritems():
            if var.is_trivial():
                basedict[name] = var.trivial_value()
            else:
                timelines.append( (name, var) )

        result = basedict, timelines

        for i in self._local_knobs:
            del self.__dict__[i]
        del self._local_knobs

        return result

    def cancel_timelines( self ):
        for i in self._local_knobs:
            del self.__dict__[i]
        del self._local_knobs

    def eval_finished( self, (basedict, timelines), t ):
        d = basedict.copy()
        for n, v in timelines:
            d[n] = v.eval( t )
        return self.eval( **d )

    def eval_unfinished( self, t ):
        d = {}
        for n, v in self._local_knobs.iteritems():
            d[n] = v.eval( t )
        return self.eval( **d )

class interp(AnimatedViewport):
    def __init__( self, *pos, **kw ):
        self._defaults = {}
        self.pos = pos
        self.max = len(pos) - 1
        self.params = (('x', 'scalar', kw.get('x',0)),)

    def eval( self, x ):
        if x <= 0.0:
            return self.pos[0]
        elif x >= self.max:
            return self.pos[-1]
        
        start = int(x)
        frac = x - start
        if frac == 0:
            return self.pos[start]
        
        return self.pos[start].interp( self.pos[start+1], frac )
    

    
    
