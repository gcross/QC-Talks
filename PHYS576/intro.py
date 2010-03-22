from slithy.library import *

#from resources import fonts
from fonts import fonts
import common

#bg = Fill( style = 'horz', color = black, color2 = Color(0,0,0.7) )

bg = Fill( style = 'horz', color = black, color2 = blue )

def title_slide():
    junk, title_v, junk, url_v = get_camera().ysplit( 1, 3, 1, 3 )
    title = Text( title_v.inset( 0.1 ), text = 'An Intuitive View of Superconducting Qubits', size = 30,
                  font = fonts['title'], color = white, justify = 0.5, vjustify = 0.5, _alpha = 0.0 )
    url = Text( url_v.inset( 0.1 ), text = 'Gregory Crosswhite', size = 14,
                font = fonts['title'], color = 0.8, justify = 0.5, vjustify = 0.5, _alpha = 0.0 )

    start_animation( bg, title, url )

    fade_in( 0.5, title, url )
    pause()
    fade_out( 0.5, title, url )

    return end_animation()
title_slide = title_slide()

titlecolor = Color(0.0, 0.3, 0.8)

title = Text( Rect(20,260,380,280), color = titlecolor, font = fonts['bold'],
              _alpha = 0.0, size = 24 )
bl = BulletedList( Rect(20,0,380,240), font = fonts['text'], color = black,
                   size = 18.0, _alpha = 0.0, bullet = [fonts['dingbats'],'w'] )

def overview_slide():
    r = get_camera().top( 0.2 ).bottom( 0.85 ).inset( 0.05 )
    title = Text( r, text = 'Overview', font = fonts['title'], size = 30, justify = 0.0,
                  color = yellow, _alpha = 0 )
    r = get_camera().bottom( 0.85 ).inset( 0.05 )
    bl = BulletedList( r, font = fonts['roman'], color = white,
                       bullet = [fonts['dingbats'], 'w'],
                       size = 18 )

    start_animation( bg, title, bl )

    fade_in(0.5,title)
    
    pause()
    bl.add_item( 0, ['Superconductors:  ',fonts['fancy'],'The Easiest System on Earth'] )
    pause()
    bl.add_item( 0, ['Josephson Junctions:  Long distance relationships are frustrating'] )
    pause()
    bl.add_item( 1, ['Regimes (and how to overthrow them)'] )
    pause()
    bl.add_item( 1, ['Just like a harominic oscillator... only not'] )
    pause()
    bl.add_item( 0, ['Qubits:  Why we care'] )
    pause()
    bl.add_item( 1, ['Cooper Pair Box'] )
    pause()
    bl.add_item( 1, ['Flux Qubits'] )
    pause()
    bl.add_item( 1, ['Current/Phase Qubits (the power of clogs)'] )
    pause()

    parallel()
    fade_out(0.5,bl)
    fade_out(0.5,title)
    end()
    return end_animation()
overview_slide = overview_slide()
    

test_objects(title_slide, overview_slide)

