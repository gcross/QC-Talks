from anim import *
from transition import Transition
from undulate import Undulation

linear = Transition( style = 'linear' )
smooth = Transition( style = 'smooth' )
wave = Undulation( style = 'sin' )

def fade_out( duration, *vlist ):
    parallel()
    for obj in vlist:
        linear( duration, obj._alpha, 0.0 )
    end()

def fade_in( duration, *vlist ):
    parallel()
    for obj in vlist:
        linear( duration, obj._alpha, 1.0 )
    end()

def make_invisible( *vlist ):
    for obj in vlist:
        set( obj.alpha, 0.0 )
        
__all__ = ['linear', 'smooth', 'wave', 'fade_out', 'fade_in', 'make_invisible']
