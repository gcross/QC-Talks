import anim
import objects
import timeline


class Controller(object):
    def __init__( self ):
        if anim.current is not None:
            raise RuntimeError, "can't instance interaction while defining an animation"

        self._anim = anim.Interaction( self )
        self.camera = self._anim.camera

        try:
            start = self.create_objects()
            if not isinstance( start, tuple ):
                start = (start,)
        except AttributeError:
            start = ()

        self._anim.enter( start )

        self._anim.event( 0.0, ('start',) )
        
