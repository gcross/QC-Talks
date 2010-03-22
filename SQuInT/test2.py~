from __future__ import division

from fonts import fonts

from slithy.presentation import *
from slithy.library import *
from slithy.transition import *

from math import exp, log

class _LogarithmicTransition(TransitionStyle):
    def __init__( self, starttime, duration, start, end, params, type ):
        TransitionStyle.__init__( self, starttime, duration, start, end, params, type )
        self.s = params.get("s",0.1)
        self.l = log((1+self.s)/self.s)
        
    def __call__( self, t ):
        if self.duration == 0.0:
            f = 1
        else:
            f = (t - self.starttime) / self.duration

        f = self.s * exp(self.l * f) - self.s
            
        if hasattr( self, 'range' ):
            return self.start + self.range * f
        else:
            return self.start.interp( self.end, f )

Transition.styles["log"] = _LogarithmicTransition

logarithmic = Transition(style="log")

bg = Fill( style = 'horz', color = black, color2 = blue )

def draw_thousand_circles():
    set_camera(Rect(0,0,width=1000,height=1,anchor="w"))

    for i in xrange(1000):
        dot(0.3,i,0)

def test_slide():
    starting_circles_rect = get_camera().restrict_aspect(1000)
    ending_circles_rect = starting_circles_rect.outset(100)

    text_v = get_camera().bottom(0.3)
    text = Text(text_v, text="look ma, I've got text!", font=fonts["title"],size=30,color=yellow,_alpha=0)

    circle_v = viewport.interp(starting_circles_rect,ending_circles_rect)

    thousand_circles = Drawable(circle_v,draw_thousand_circles)

    start_animation(bg,thousand_circles, text)

    pause()

    logarithmic( 10, circle_v.x, 1, s=0.1 )

    pause()

    fade_in(1, text)

    return end_animation()

test_slide = test_slide()

play(test_slide)

run_presentation() 
