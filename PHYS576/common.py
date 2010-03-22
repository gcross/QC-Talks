from slithy.library import *

bg = Fill( style = 'horz', color = black, color2 = Color(0,0,0.5) )

def to_black():
    start_animation( Fill(color=black), bg )
    fade_out( 0.5, bg )
    return end_animation()
to_black = to_black()
