#@+leo-ver=4
#@+node:@file slides.py
from __future__ import division
from timetracker import *

from objects import *

from numpy import arange, array, complex128, zeros, argsort, dot as dot_product, real, trace, identity, conj
from numpy.linalg import norm
from numpy.random import rand

from Graph import Graph, Subgraph

from random import shuffle

from InfiniteOpenProductState import multiply_tensor_by_matrix_at_index, special_left_multiply, special_right_multiply, my_eigen

#@+others
#@+node:Slides
#@+node:Logo
def logo(
    circle_position=(SCALAR,-10,10,-2),
    circle_width=(SCALAR,0,2,0.5),
    line_1_x1=(SCALAR,-10,10,0),
    line_1_y1=(SCALAR,-2,2,0.75),
    line_1_x2=(SCALAR,-10,10,0),
    line_1_y2=(SCALAR,-2,2,-0.75),
    line_2_x1=(SCALAR,-10,10,0.75),
    line_2_y1=(SCALAR,-2,2,0),
    line_2_x2=(SCALAR,-10,10,-0.75),
    line_2_y2=(SCALAR,-2,2,-0),
    line_3_x1=(SCALAR,-10,10,2),
    line_3_y1=(SCALAR,-2,2,1),
    line_3_x2=(SCALAR,-10,10,2),
    line_3_y2=(SCALAR,-2,2,-1),
    line_4_x1=(SCALAR,-10,10,-1),
    line_4_y1=(SCALAR,-2,2,0),
    line_4_x2=(SCALAR,-10,10,-1),
    line_4_y2=(SCALAR,-2,2,0),
    dot_x=(SCALAR,-10,10,-1),
    dot_y=(SCALAR,0,0,0),
    dot_radius=(SCALAR,0,0.5,0),
    dot_color=(COLOR,Color(3/4,0,0,0)),
    bracket_position=(SCALAR,-10,10,5.5),
    path_alpha=(SCALAR,0,1,0),
    ):
    set_camera(Rect(0,0,width=10,height=4))
    color(white)
    thickness(0.2)

    def drawit():
        push()
        scale(circle_width,1,circle_position,0)
        circle(1,circle_position,0)
        pop()
        line(line_1_x1,line_1_y1,line_1_x2,line_1_y2)
        line(line_2_x1,line_2_y1,line_2_x2,line_2_y2)
        line(line_3_x1,line_3_y1,line_3_x2,line_3_y2)
        line(line_4_x1,line_4_y1,line_4_x2,line_4_y2)

    path = (
        Path()
            .moveto(line_4_x1,line_4_y1)
            .lineto(line_4_x2,line_4_y2)
            .lineto(line_2_x1,line_2_y1)
            .lineto(line_2_x2,line_2_y2)
            .lineto(line_3_x2,line_3_y2)
    )

    push()
    translate(0.1,0.1)
    color(black)
    drawit()
    color(black,path_alpha)
    widestroke(path,0.2)
    pop()

    color(1,0,1)
    drawit()
    color(1,1/8,1,path_alpha)
    widestroke(path,0.2)        

    color(dot_color)
    dot(dot_radius,dot_x,dot_y)

    color(1,1/2,1)

    line(-bracket_position,1.5,-bracket_position,-1.5)

    widestroke(
        Path()
            .moveto(bracket_position,1.5)
            .lineto(bracket_position+1,0)
            .lineto(bracket_position,-1.5)
        ,
        0.2
        )

bg = Fill( style = 'horz', color = black, color2 = blue )

def animate_logo():
    c = get_camera().restrict_aspect(10.0/4)

    my_logo = Drawable(c,logo)

    start_animation(my_logo)

    parallel()
    dt=1/2
    linear(dt,my_logo.circle_position,-1)
    linear(dt,my_logo.circle_width,0.8)
    linear(dt,my_logo.line_1_x1,-0.25)
    linear(dt,my_logo.line_1_y1,-1)
    linear(dt,my_logo.line_1_x2,0.5)
    linear(dt,my_logo.line_1_y2,1)
    linear(dt,my_logo.line_2_x1,0.5)
    linear(dt,my_logo.line_2_y1,1)
    linear(dt,my_logo.line_2_x2,1.25)
    linear(dt,my_logo.line_2_y2,-1)
    linear(dt,my_logo.line_3_x1,1.25)
    linear(dt,my_logo.line_3_y1,-1)
    linear(dt,my_logo.line_3_x2,2)
    linear(dt,my_logo.line_3_y2,1)
    end()

    parallel()
    dt=0.6/2
    smooth(dt,my_logo.dot_radius,0.15,e=-1.5)

    serial()
    wait(dt/2)
    smooth(dt/2,my_logo.dot_color,Color(3/4,1/4,0,1),e=-1.5)
    end()

    end()

    wait(0.2)

    parallel()
    dt=0.3/2
    linear(dt,my_logo.dot_x,-0.25)
    linear(dt,my_logo.dot_y,-1)
    linear(dt,my_logo.line_4_x2,-0.25)
    linear(dt,my_logo.line_4_y2,-1)
    end()

    parallel()

    dt=0.6/2

    parallel()
    linear(dt,my_logo.dot_x,0)
    linear(dt,my_logo.dot_y,0)
    linear(dt,my_logo.dot_radius,2)
    end()

    serial()
    wait(dt/4)
    parallel()
    linear(3*dt/4,my_logo.dot_color,Color(1,1/8,1,0))
    linear(3*dt/4,my_logo.path_alpha,1)
    end()
    end()

    end()

    smooth(1/2,my_logo.bracket_position,2.5,e=-0.5)

    return end_animation()

animate_logo = animate_logo()

#@-node:Logo
#@+node:Title Slide
def title_slide():
    junk, title_v, junk, url_v = get_camera().ysplit( 1, 3, 1, 3 )
    title = Text( title_v.inset( 0.1 ), text = 'Cutting infinite systems down to size with matrix product states', size = 30,
                  font = fonts['title'], color = white, justify = 0.5, vjustify = 0.5, _alpha = 0.0 )
    url = Text( url_v.inset( 0.1 ), text = 'Gregory Crosswhite\nUniversity of Washington, Seattle', size = 14,
                font = fonts['title'], color = 0.8, justify = 0.5, vjustify = 0.5, _alpha = 0.0 )

    logo_v = viewport.interp(get_camera(),get_camera().inset(0.25).move_down(0.25).move_left(.025),_alpha=0)

    logo_animation = Anim(logo_v)

    start_animation( bg, title, url, logo_animation )

    pause()

    logo_animation.fade_in_start(animate_logo,0.5)    
    logo_animation.play(animate_logo)

    parallel()

    smooth(1,logo_v.x,1)

    serial()
    wait(0.5)
    fade_in( 1, title, url )
    end()

    end()

    pause()

    fade_out( 0.5, title, url, logo_animation )

    return end_animation()
title_slide = title_slide()

#@-node:Title Slide
#@+node:Logo Slide
def logo_slide():
    logo_animation = Anim(get_camera())

    start_animation(bg,logo_animation)

    pause()

    logo_animation.fade_in_start(animate_logo,0.5)

    wait(0.5)

    logo_animation.play(animate_logo)

    wait(0.5)

    logo_animation.fade_out_end(animate_logo,0.5)

    return end_animation()
logo_slide = logo_slide()
#@-node:Logo Slide
#@+node:Grey Slide
def grey_slide():
    grey_square = Fill(color=grey,color2=grey,_alpha=0)

    start_animation(bg,grey_square)

    pause()

    fade_in(0.5, grey_square)

    pause()

    fade_out(0.5, grey_square)

    return end_animation()
grey_slide = grey_slide()
#@-node:Grey Slide
#@+node:Outline Slide
def outline_slide():
    r = get_camera().top( 0.2 ).bottom( 0.85 ).inset( 0.05 )
    title = Text( r, text = 'Overview', font = fonts['title'], size = 30, justify = 0.0,
                  color = yellow, _alpha = 0 )
    r = get_camera().bottom( 0.85 ).inset( 0.05 )
    bl = BulletedList( r, font = fonts['roman'], color = white,
                       bullet = [fonts['dingbats'], 'w'],
                       size = 18 )

    start_animation( bg, title, bl )

    pause()

    fade_in(0.5,title)

    pause()
    bl.add_item( 0, [fonts['fancy'],'Finding the ground state of an infinite systems!'] )
    pause()
    bl.add_item( 0, ['Intuitive physical picture of algorithm'] )
    pause()
    bl.add_item( 0, ['Matrix product states'] )
    pause()
    bl.add_item( 1, ['Motivation: Signal exchange picture'] )
    pause()
    bl.add_item( 1, ['Mathematical formalism'] )
    pause()
    bl.add_item( 1, ['Expectation values --> Relaxation'] )
    pause()
    bl.add_item( 0, ['Results'] )
    pause()
    bl.add_item( 1, ['Transverse Ising Model'] )
    pause()
    bl.add_item( 1, ['Haldane-Shastry Model'] )
    pause()

    parallel()
    fade_out(0.5,bl)
    fade_out(0.5,title)
    end()
    return end_animation()
outline_slide = outline_slide()
#@-node:Outline Slide
#@+node:Animation
start_animation(bg)

#@+others
#@+node:Interactions
#@+node:Create waves for animation
crazy_interaction_duration = 25
wait_before_long_waves_start = 8
pause_to_show_thermometer = 15

wave_speed = 3

short_wave_density = 3
long_wave_density = 3

#@+node:Short waves
short_wave_lag_times = 8/wave_speed

short_waves = []
for i in xrange(crazy_interaction_duration*short_wave_density):
    short_waves.append((
        randint(-2,2),
        random()*0.3-0.15,
        random_choice((0,180)),
        random()*(crazy_interaction_duration-4)*wave_speed,
        Color(random(),random(),random())
    ))
#@-node:Short waves
#@+node:Long waves
long_waves = []
for i in xrange((crazy_interaction_duration-wait_before_long_waves_start)*long_wave_density):

    dot_number = randint(-2,2)
    start_time = (wait_before_long_waves_start+random()*(crazy_interaction_duration-wait_before_long_waves_start-10))*wave_speed
    angle = random()*90+random_choice((45,225))
    wave_color = Color(random(),random(),random())

    if random_choice((True,False)):
        long_waves.append((
            dot_number,
            0,
            angle,
            start_time,
            wave_color
        ))
    else:
        angle_in_radians = angle/180*pi
        r = abs(sin(angle_in_radians)/((5/2)*(3/4))) + 0.3
        long_waves.append((
            dot_number+2.5*cos(angle_in_radians),
            2.5*sin(angle_in_radians),
            angle-180,
            start_time-(2.5-r),
            wave_color
        ))
#@-node:Long waves
#@-node:Create waves for animation
#@+node:Zoom
starting_circles_rect = get_camera().restrict_aspect(1000)
ending_circles_rect = starting_circles_rect.outset(99.5)

circle_v = viewport.interp(starting_circles_rect,ending_circles_rect)

thousand_circles = Drawable(circle_v,draw_thousand_circles)

pause()

enter(thousand_circles)

exponential( 5, circle_v.x, 1, s=0.1 )

pause()

exit(thousand_circles)

del starting_circles_rect, ending_circles_rect, circle_v, thousand_circles
#@-node:Zoom
#@+node:Zero temperature = ground state
#@+node:Initialization
#@+at
# Create the five circles and put them on the screen.
#@-at
#@@c

circles = Drawable(get_camera(),draw_five_circles)

enter(circles)

#@+at
# Attach the waves to the circle drawing.
#@-at
#@@c

set(circles.short_waves,short_waves)
set(circles.long_waves,long_waves)
#@nonl
#@-node:Initialization
#@+node:First part
#@+at
# Do the first part of the interaction animation.
#@-at
#@@c

parallel()
linear(
    pause_to_show_thermometer,
    circles.wave_time,
    pause_to_show_thermometer*wave_speed
)
for i in xrange(5):
    serial()
    for j in xrange(pause_to_show_thermometer//5):
        linear(
            5,
            getattr(circles,"color%i"%i),
            Color(random(),random(),random())
        )
    end()
end()

pause()
#@nonl
#@-node:First part
#@+node:Thermometer
#@+at
# Create and fade in the thermometer.
#@-at
#@@c

thermometer = Drawable(get_camera().right(0.2).top(0.3).inset(0.05),draw_thermometer,_alpha=0)
enter(thermometer)
fade_in(1,thermometer)

pause()

#@+at
# Lower the temperature.
#@-at
#@@c

smooth(2,thermometer.temperature,0)

pause()
#@nonl
#@-node:Thermometer
#@+node:Second part
#@+at
# Finish up the interaction animation, ending in the equilibrium state.
#@-at
#@@c

parallel()
linear(
    crazy_interaction_duration-pause_to_show_thermometer,
    circles.wave_time,
    crazy_interaction_duration*wave_speed
)

n = (crazy_interaction_duration-pause_to_show_thermometer)//5
for i in xrange(5):
    serial()
    for j in xrange(n):
        linear(
            crazy_interaction_duration*(1/2)*(1/4),
            getattr(circles,"color%i"%i),
            Color(*[random()*(n-(j+1))/n+(j+1)/n*0.5 for dummy in xrange(3)])
        )
    end()
end()

pause()


#@-node:Second part
#@+node:Cleanup
#@+at
# We don't need the waves any more, so stop redrawing them.
#@-at
#@@c

set(circles.short_waves,[])
set(circles.long_waves,[])
#@-node:Cleanup
#@-node:Zero temperature = ground state
#@+node:New sites match the old
#@+node:Add a new site
#@+at
# Fade in the additional site.
#@-at
#@@c

linear(0.5,circles.color5,Color(get(circles.color5),1))

#@+at
# Make room for it!
#@-at
#@@c

parallel()

for i in xrange(3):
    serial()
    wait(0.2*(2-i))
    smooth(1,getattr(circles,"x%i"%i),i-3,e=-0.5)
    end()

#@+at
# Move it into the newly available slot.
#@-at
#@@c

serial()
wait(1.2)
smooth(0.5,circles.y5,0)
end()

end()

pause()

#@-node:Add a new site
#@+node:Let it reach equilibrium
#@+node:Create a new set of waves
#@+at
# Create new set of waves
#@-at
#@@c

duration = 10

short_waves = []

def create_wave_centered_at(dot_number,start_time_offset,start_time_range):
    x, angle = random_choice([(dot_number-1,0),(dot_number,0),(dot_number,180),(dot_number+1,180)])

    short_waves.append((
        x,
        random()*0.3-0.15,
        angle,
        (start_time_offset+random()*start_time_range)*wave_speed,
        Color(random(),random(),random())
    ))

for i in xrange(3):
    for dummy in xrange((i+1)*10):
        create_wave_centered_at(randint(-i,i),(duration-4)/3*i,(duration-4)/3)

#@+at
# Attach the waves to the diagram, and reset the time.
#@-at
#@@c

set(circles.short_waves,short_waves)
set(circles.wave_time,0)
#@nonl
#@-node:Create a new set of waves
#@+node:Animate
#@+at
# Animate the waves and the relaxation of the site.
#@-at
#@@c

parallel()

linear(duration,circles.wave_time,duration*wave_speed)

serial()
linear(3,circles.color5,Color(*[0.125 + 0.75*random() for dummy in xrange(3)]))
linear(3,circles.color5,Color(*[0.25 + 0.5*random() for dummy in xrange(3)]))
linear(4,circles.color5,Color(0.5,0.5,0.5))
end()

serial()
wait(2)
linear(2,circles.color2,Color(*[0.125 + 0.75*random() for dummy in xrange(3)]))
linear(3,circles.color2,Color(*[0.25 + 0.5*random() for dummy in xrange(3)]))
linear(3,circles.color2,Color(0.5,0.5,0.5))
end()

serial()
wait(2)
linear(2,circles.color3,Color(*[0.125 + 0.75*random() for dummy in xrange(3)]))
linear(3,circles.color3,Color(*[0.25 + 0.5*random() for dummy in xrange(3)]))
linear(3,circles.color3,Color(0.5,0.5,0.5))
end()

serial()
wait(4)
linear(3,circles.color1,Color(*[0.25 + 0.5*random() for dummy in xrange(3)]))
linear(3,circles.color1,Color(0.5,0.5,0.5))
end()

serial()
wait(4)
linear(3,circles.color4,Color(*[0.25 + 0.5*random() for dummy in xrange(3)]))
linear(3,circles.color4,Color(0.5,0.5,0.5))
end()

end()

pause()
#@nonl
#@-node:Animate
#@-node:Let it reach equilibrium
#@-node:New sites match the old
#@+node:Simulate the environment
#@+node:Add a new site
#@+at
# Fade in the additional site.
#@-at
#@@c

linear(0.5,circles.color6,Color(get(circles.color6),1))

#@+at
# Make room for it!
#@-at
#@@c

parallel()

wait_time = 0
for i in [5,3,4]:
    serial()
    wait(wait_time)
    wait_time += 0.2
    smooth(1,getattr(circles,"x%i"%i),get(getattr(circles,"x%i"%i))+1,e=-0.5)
    end()

end()

pause()

#@-node:Add a new site
#@+node:Replace sites with environment
#@+at
# Replace the left and right sites with boexes denoting the environment.
#@-at
#@@c

linear(1,circles.environment_alpha,1)

pause()
#@-node:Replace sites with environment
#@+node:Move site into slot
#@+at
# Move it into the newly available slot.
#@-at
#@@c

smooth(0.5,circles.y6,0)

pause()
#@-node:Move site into slot
#@+node:Interact site with environment
#@+at
# Create and attach new set of short waves.
#@-at
#@@c

short_waves = []

for dummy in xrange(10):
    create_wave_centered_at(0,0,3)

set(circles.short_waves,short_waves)
set(circles.wave_time,0)


#@+at
# Animate!
#@-at
#@@c

parallel()

linear(7,circles.wave_time,7*wave_speed)

serial()
linear(3,circles.color6,Color(*([0.25 + 0.5*random() for dummy in xrange(3)])))
linear(4,circles.color6,Color(0.5))
end()

end()

pause()
#@-node:Interact site with environment
#@+node:Take site away
#@+at
# Fade the site out.
#@-at
#@@c

linear(1,circles.color6,Color(0.5,0))

#@+at
# Move it to the starting position and reset its color so that it appears to 
# be a new site.
#@-at
#@@c

set(circles.color6,Color(random(),random(),random(),0))
set(circles.x6,0)
set(circles.y6,1.5)

pause()
#@nonl
#@-node:Take site away
#@-node:Simulate the environment
#@+node:Suppose we don't have environment
#@+node:Change shade of environment
#@+at
# Change the shade of the environment to indicate a "guess".
#@-at
#@@c

parallel()
linear(1,circles.left_environment_background,Color(0.75,0.75,0))
linear(1,circles.right_environment_background,Color(0.75,0,0.75))
end()

pause()

#@-node:Change shade of environment
#@+node:Add and interact new site
#@+at
# Fade site in.
#@-at
#@@c

linear(0.5,circles.color6,Color(get(circles.color6),1))

#@+at
# Move it into the slot.
#@-at
#@@c

smooth(0.5,circles.y6,0)

#@+at
# Interact!
#@-at
#@@c

#@+at
# Create and attach new set of short waves.
#@-at
#@@c

short_waves = []

for dummy in xrange(4):
    create_wave_centered_at(0,0,1)

set(circles.short_waves,short_waves)
set(circles.wave_time,0)


#@+at
# Animate!
#@-at
#@@c

parallel()

linear(4,circles.wave_time,7*wave_speed)

linear(4,circles.color6,Color(random(),random(),random()))

end()

pause()


#@+at
# Slide to left and update environment.
#@-at
#@@c

parallel()

smooth(1,circles.x6,-1)

grey = Color(0.5)

serial()
wait(0.5)
linear(1,circles.left_environment_background,get(circles.left_environment_background).interp(grey,0.4))
end()

end()

pause()

#@+at
# Move site to the starting position and reset its color so that it appears to 
# be a new site.
#@-at
#@@c

set(circles.color6,Color(random(),random(),random(),0))
set(circles.x6,0)
set(circles.y6,1.5)

#@+at
# Kill sine waves.
#@-at
#@@c

set(circles.short_waves,[])

#@-node:Add and interact new site
#@+node:Converge!
current_left = 0.4
current_right = 0

def add_interact_and_move(absorb_direction):
    #@    @+others
    #@+node:Add site
    #@+at
    # Fade site in while moving it into the slot.
    #@-at
    #@@c

    parallel()

    linear(0.5,circles.color6,Color(get(circles.color6),1))
    smooth(0.5,circles.y6,0)

    end()
    #@-node:Add site
    #@+node:Interact site
    #@+at
    # Change the site's color to show how it relaxes.
    #@-at
    #@@c

    global current_left, current_right

    width = 1-(current_left+current_right)/2

    if width < 0:
        width = 0

    linear(0.5,circles.color6,Color(*[(1-width)/2+width*random() for dummy in xrange(3)]))

    wait(0.25)
    #@-node:Interact site
    #@+node:Absorb site
    #@+at
    # Slide to left/right and update environment.
    #@-at
    #@@c

    if absorb_direction == -1:
        environment_background = circles.left_environment_background
        current_left += 0.4
        current = current_left
    elif absorb_direction == +1:
        environment_background = circles.right_environment_background
        current_right += 0.4
        current = current_right
    else:
        return

    parallel()

    smooth(0.5,circles.x6,absorb_direction)

    grey = Color(0.5)

    serial()
    wait(0.25)

    linear(0.5,environment_background,get(environment_background).interp(grey,current))
    end()

    end()

    #@-node:Absorb site
    #@+node:Remove and reset site
    #@+at
    # Move site to the starting position and reset its color so that it 
    # appears to be a new site.
    #@-at
    #@@c

    set(circles.color6,Color(random(),random(),random(),0))
    set(circles.x6,0)
    set(circles.y6,1.5)
    #@-node:Remove and reset site
    #@-others

add_interact_and_move(+1)

for i in xrange(3):
  add_interact_and_move(-1)
  add_interact_and_move(+1)

add_interact_and_move(0)

pause()
#@-node:Converge!
#@-node:Suppose we don't have environment
#@-node:Interactions
#@+node:Signals
#@+node:Transition to signal drawing
wires = Drawable(get_camera(),draw_wires,_alpha=0)

wheres_the_cat = Text(get_camera(),text="Where's the cat???",color=yellow,justify=0.5,vjustify=0.5,_alpha=0,font=fonts['big'],size=32)

enter(wires,wheres_the_cat)

parallel()
linear(1,wires._alpha,1)
linear(1,wires.height,1)
linear(1,wheres_the_cat._alpha,1)
linear(1,circles._alpha,0)
linear(1,thermometer._alpha,0)
end()

exit(circles,thermometer)

pause()
#@-node:Transition to signal drawing
#@+node:Highlight top and bottom rows
#@+at
# Highlight the top row as I mention it.
#@-at
#@@c

parallel()
linear(0.5,wires.top_highlight,1)
linear(0.5,wires.bottom_alpha,0)
end()

pause()

#@+at
# Now highlight the bottom row.
#@-at
#@@c


parallel()
linear(0.5,wires.top_highlight,0)
linear(0.5,wires.top_alpha,0)
linear(0.5,wires.bottom_alpha,1)
linear(0.5,wires.bottom_highlight,1)
end()

pause()

#@+at
# Return to the original slide.
#@-at
#@@c


parallel()
linear(0.5,wires.top_alpha,1)
linear(0.5,wires.bottom_highlight,0)
end()

pause()
#@-node:Highlight top and bottom rows
#@+node:Highlight center site
#@+at
# Highlight the center site while fading away the outside sites and the 
# caption.
#@-at
#@@c

parallel()
fade_out(0.5,wheres_the_cat)
linear(0.5,wires.center_rectangle_alpha,1)
linear(0.5,wires.outer_alpha,0.25)
end()

exit(wheres_the_cat)

pause()
#@-node:Highlight center site
#@+node:Show example of wrong configuration
#@+at
# Show an example of a wrong configuration.
#@-at
#@@c

parallel()
linear(0.5,wires.center_rectangle_alpha,0)
linear(0.5,wires.center_site_alpha,0.25)
linear(0.5,wires.middle_alpha,1)
end()

pause()

#@+at
# Draw an X through this configuration.
#@-at
#@@c

linear(0.5,wires.horribly_wrong_alpha,1)

pause()

#@+at
# Restore back to the original configuration.
#@-at
#@@c

parallel()
linear(0.5,wires.horribly_wrong_alpha,0)
linear(0.5,wires.middle_alpha,0)
linear(0.5,wires.center_site_alpha,1)
linear(0.5,wires.outer_alpha,1)
end()

pause()

#@-node:Show example of wrong configuration
#@+node:Zoom out to emphasize size of state
#@+at
# Zoom out to emphasize how big the state is.
#@-at
#@@c

exponential(9,wires.window,30,s=.03)

pause()

#@+at
# Now zoom back in... *but*, don't go all the way back, so that we can have a 
# little bit of extra space for a margin on the sides.
#@-at
#@@c

logarithmic(3,wires.window,1.15,s=30)

pause()
#@-node:Zoom out to emphasize size of state
#@+node:Dot dot dots
#@+at
# Fade in dot-dot-dots.
#@-at
#@@c

linear(0.5,wires.dot_alpha,0.5)

pause()

#@+at
# Combine dots to form ports.
#@-at
#@@c

smooth(1,wires.dot_slider,0)

pause()

#@-node:Dot dot dots
#@+node:Signals
#@+node:Upper signal
#@+at
# Incoming signal in upper port!
#@-at
#@@c

parallel()
smooth(0.5,wires.upper_input_port_signal_radius,0.1,e=-0.5)
serial()
linear(0.4,wires.upper_input_port_signal_color,yellow)
linear(0.2,wires.upper_input_port_signal_color,red)
end()
end()

pause()

#@+at
# Highlight activated state component.
#@-at
#@@c

smooth(0.5,wires.top_highlight,1)

pause()

#@+at
# Route signal to correspoding outer port.
#@-at
#@@c

linear(1,wires.upper_connection_length,1)

#@+at
# Send signal out port
#@-at
#@@c

parallel()
smooth(0.5,wires.upper_output_port_signal_radius,0.1,e=-0.5)
serial()
linear(0.4,wires.upper_output_port_signal_color,yellow)
linear(0.2,wires.upper_output_port_signal_color,red)
end()
end()

pause()

#@+at
# Dehighlight everything.
#@-at
#@@c

parallel()
linear(1,wires.upper_connection_color,grey)
linear(1,wires.upper_input_port_signal_color,Color(red,0))
linear(1,wires.upper_output_port_signal_color,Color(red,0))
linear(1,wires.top_highlight,0)
end()

pause()
#@-node:Upper signal
#@+node:Lower signal
#@+at
# Incoming signal in lower port!
#@-at
#@@c

parallel()
smooth(0.5,wires.lower_input_port_signal_radius,0.1,e=-0.5)
serial()
linear(0.4,wires.lower_input_port_signal_color,yellow)
linear(0.2,wires.lower_input_port_signal_color,red)
end()
end()


#@+at
# Route signal to correspoding outer port while highlighting the corresponding 
# configuration.
#@-at
#@@c

parallel()
smooth(1,wires.bottom_highlight,1)
linear(1,wires.lower_connection_length,1)
end()

#@+at
# Send signal out port.
#@-at
#@@c

parallel()
smooth(0.5,wires.lower_output_port_signal_radius,0.1,e=-0.5)
serial()
linear(0.4,wires.lower_output_port_signal_color,yellow)
linear(0.2,wires.lower_output_port_signal_color,red)
end()
end()

pause()

#@+at
# Dehighlight everything.
#@-at
#@@c

parallel()
linear(1,wires.lower_connection_color,grey)
linear(1,wires.lower_input_port_signal_color,Color(red,0))
linear(1,wires.lower_output_port_signal_color,Color(red,0))
linear(1,wires.bottom_highlight,0)
end()

pause()
#@nonl
#@-node:Lower signal
#@-node:Signals
#@+node:Fade in everything else
#@+at
# Fade in the rest of the wires.
#@-at
#@@c

linear(1,wires.everything_else_alpha,1)

pause()

#@+at
# Put a box around the center.
#@-at
#@@c

linear(0.5,wires.center_rectangle_alpha,1)

pause()

#@-node:Fade in everything else
#@+node:Fade it all out
#@+at
# Take it all away.
#@-at
#@@c

fade_out(0.5,wires)
exit(wires)

del wires

pause()
#@-node:Fade it all out
#@-node:Signals
#@+node:W state
#@+node:Fade in W state
#@+at
# Fade in a picture of the W state.
#@-at
#@@c

W_state = Drawable(get_camera(),draw_W_state,_alpha=0)

enter(W_state)
fade_in(0.5,W_state)

pause()
#@-node:Fade in W state
#@+node:Rectangle around 3rd site
#@+at
# Draw a rectangle around the third site.
#@-at
#@@c

linear(0.5,W_state.rectangle_alpha,1)

pause()
#@nonl
#@-node:Rectangle around 3rd site
#@+node:Highlight regions
r_last = W_state.highlights1
h_last = W_state.highlight_slider1

r_current = W_state.highlights2
h_current = W_state.highlight_slider2

for region in [
    [(0,0),(0,1),(1,0),(1,1)],
    [(0,2),(0,3),(1,2),(1,3)],
    [(3,0),(3,1),(3,2)],
    [(3,3)]
    ]:

    set(r_last,get(r_current))
    set(h_last,get(h_current))

    set(r_current,region)
    set(h_current,0)

    parallel()
    linear(0.5,h_last,0)
    linear(0.5,h_current,1)
    end()

    pause()
#@-node:Highlight regions
#@+node:Fade out W state
fade_out(0.5,W_state)

exit(W_state)
#@-node:Fade out W state
#@-node:W state
#@+node:W state wiring
#@+node:Fade in new diagram
#@+at
# Fade in W state with wiring
#@-at
#@@c

W_state_wirings = Drawable(get_camera(),draw_W_state_wirings,_alpha=0)

enter(W_state_wirings)
fade_in(1,W_state_wirings)

pause()
#@-node:Fade in new diagram
#@+node:Wire connections

#@+node:Upper port
#@+at
# Incoming signal in upper port!
#@-at
#@@c

parallel()
smooth(0.5,W_state_wirings.signal_radius_00,0.1,e=-0.5)
serial()
linear(0.4,W_state_wirings.signal_color_00,yellow)
linear(0.2,W_state_wirings.signal_color_00,red)
end()
end()

pause()


#@+at
# Choice:  spin down here.
#@-at
#@@c

parallel()
linear(1,W_state_wirings.top_qubit_alpha,1)
smooth(1,W_state_wirings.top_qubit_highlight,1)
linear(1,W_state_wirings.top_connection_length,1)
end()

parallel()
smooth(0.5,W_state_wirings.signal_radius_10,0.1,e=-0.5)
serial()
linear(0.4,W_state_wirings.signal_color_10,yellow)
linear(0.2,W_state_wirings.signal_color_10,red)
end()
end()

pause()

parallel()
linear(0.5,W_state_wirings.signal_color_10,Color(red,0))
linear(0.5,W_state_wirings.top_connection_color,grey)
smooth(0.5,W_state_wirings.top_qubit_highlight,0)
end()

pause()



#@+at
# Choice:  spin up here.
#@-at
#@@c

parallel()
linear(1,W_state_wirings.middle_qubit_alpha,1)
smooth(1,W_state_wirings.middle_qubit_highlight,1)
linear(1,W_state_wirings.middle_connection_length,1)
end()

parallel()
smooth(0.5,W_state_wirings.signal_radius_11,0.1,e=-0.5)
serial()
linear(0.4,W_state_wirings.signal_color_11,yellow)
linear(0.2,W_state_wirings.signal_color_11,red)
end()
end()

pause()

parallel()
linear(0.5,W_state_wirings.signal_color_11,Color(red,0))
linear(0.5,W_state_wirings.signal_color_00,Color(red,0))
linear(0.5,W_state_wirings.middle_connection_color,grey)
smooth(0.5,W_state_wirings.middle_qubit_highlight,0)
end()

set(W_state_wirings.signal_radius_11,0.2)

pause()

#@-node:Upper port
#@+node:Lower port
#@+at
# Incoming signal in lower port!
#@-at
#@@c

parallel()
smooth(0.5,W_state_wirings.signal_radius_01,0.1,e=-0.5)
serial()
linear(0.4,W_state_wirings.signal_color_01,yellow)
linear(0.2,W_state_wirings.signal_color_01,red)
end()
end()

pause()


#@+at
# Choice:  spin down here.
#@-at
#@@c

parallel()
linear(1,W_state_wirings.bottom_qubit_alpha,1)
smooth(1,W_state_wirings.bottom_qubit_highlight,1)
linear(1,W_state_wirings.bottom_connection_length,1)
end()

parallel()
smooth(0.5,W_state_wirings.signal_radius_11,0.1,e=-0.5)
serial()
linear(0.4,W_state_wirings.signal_color_11,yellow)
linear(0.2,W_state_wirings.signal_color_11,red)
end()
end()

pause()

parallel()
linear(0.5,W_state_wirings.signal_color_11,Color(red,0))
linear(0.5,W_state_wirings.signal_color_01,Color(red,0))
linear(0.5,W_state_wirings.bottom_connection_color,grey)
smooth(0.5,W_state_wirings.bottom_qubit_highlight,0)
end()

pause()


#@-node:Lower port
#@-node:Wire connections
#@+node:Zoom out to show matrix
#@+at
# Zoom out to show the matrix under the wirings.
#@-at
#@@c

smooth(1,W_state_wirings.window,2.9)
linear(0.5,W_state_wirings.center_matrix_alpha,1)

pause()
#@-node:Zoom out to show matrix
#@+node:Fill in matrix
#@+at
# Highlight and fade in each matrix element.
#@-at
#@@c

set(W_state_wirings.signal_radius_00,0.05)
set(W_state_wirings.signal_radius_01,0.05)
set(W_state_wirings.signal_radius_10,0.05)
set(W_state_wirings.signal_radius_11,0.05)

set(W_state_wirings.signal_color_00,Color(purple,0))
set(W_state_wirings.signal_color_01,Color(purple,0))
set(W_state_wirings.signal_color_10,Color(purple,0))
set(W_state_wirings.signal_color_11,Color(purple,0))

linear(0.5,W_state_wirings.matrix_index_labels_alpha,1)

pause()

parallel()
smooth(0.5,W_state_wirings.top_qubit_highlight,1)
linear(0.5,W_state_wirings.center_matrix_element_alpha_00,1)
smooth(0.5,W_state_wirings.center_matrix_element_highlight_00,1)
linear(0.5,W_state_wirings.signal_color_00,purple)
linear(0.5,W_state_wirings.signal_color_10,purple)
linear(0.5,W_state_wirings.top_connection_color,purple)
smooth(0.5,W_state_wirings.row_1_highlight,1)
smooth(0.5,W_state_wirings.column_1_highlight,1)
end()

pause()


parallel()
smooth(0.5,W_state_wirings.top_qubit_highlight,0)
smooth(0.5,W_state_wirings.center_matrix_element_highlight_00,0)
smooth(0.5,W_state_wirings.middle_qubit_highlight,1)
linear(0.5,W_state_wirings.center_matrix_element_alpha_10,1)
smooth(0.5,W_state_wirings.center_matrix_element_highlight_10,1)
linear(0.5,W_state_wirings.signal_color_10,Color(purple,0))
linear(0.5,W_state_wirings.signal_color_11,purple)
linear(0.5,W_state_wirings.top_connection_color,grey)
linear(0.5,W_state_wirings.middle_connection_color,purple)
smooth(0.5,W_state_wirings.column_1_highlight,0)
smooth(0.5,W_state_wirings.column_2_highlight,1)
end()

pause()


parallel()
smooth(0.5,W_state_wirings.middle_qubit_highlight,0)
smooth(0.5,W_state_wirings.center_matrix_element_highlight_10,0)
smooth(0.5,W_state_wirings.bottom_qubit_highlight,1)
linear(0.5,W_state_wirings.center_matrix_element_alpha_11,1)
smooth(0.5,W_state_wirings.center_matrix_element_highlight_11,1)
linear(0.5,W_state_wirings.signal_color_00,Color(purple,0))
linear(0.5,W_state_wirings.signal_color_01,purple)
linear(0.5,W_state_wirings.middle_connection_color,grey)
linear(0.5,W_state_wirings.bottom_connection_color,purple)
smooth(0.5,W_state_wirings.row_1_highlight,0)
smooth(0.5,W_state_wirings.row_2_highlight,1)
end()

pause()



parallel()
smooth(0.5,W_state_wirings.bottom_qubit_highlight,0)
smooth(0.5,W_state_wirings.center_matrix_element_highlight_11,0)
linear(0.5,W_state_wirings.center_matrix_element_alpha_01,1)
smooth(0.5,W_state_wirings.center_matrix_element_highlight_01,1)
linear(0.5,W_state_wirings.signal_color_11,Color(purple,0))
linear(0.5,W_state_wirings.signal_color_10,purple)
linear(0.5,W_state_wirings.bottom_connection_color,grey)
smooth(0.5,W_state_wirings.column_2_highlight,0)
smooth(0.5,W_state_wirings.column_1_highlight,1)
end()

pause()


parallel()
smooth(0.5,W_state_wirings.center_matrix_element_highlight_01,0)
linear(0.5,W_state_wirings.signal_color_10,Color(purple,0))
linear(0.5,W_state_wirings.signal_color_01,Color(purple,0))
smooth(0.5,W_state_wirings.column_1_highlight,0)
smooth(0.5,W_state_wirings.row_2_highlight,0)
linear(0.5,W_state_wirings.matrix_index_labels_alpha,0)
end()

pause()
#@-node:Fill in matrix
#@+node:Zoom out to show labels
#@+at
# Zoom out to show the matrix label.
#@-at
#@@c

smooth(1,W_state_wirings.window,3.55)
linear(0.5,W_state_wirings.center_label_alpha,1)

pause()
#@-node:Zoom out to show labels
#@+node:Point out meaning of sub/superscripts.
#@+at
# Highlight subscript and associated arrows.
#@-at
#@@c

parallel()
smooth(0.5,W_state_wirings.lower_index_highlight,1)
linear(0.5,W_state_wirings.arrow_alpha,1)
end()

pause()

#@+at
# Highlight superscript and matrix elements.
#@-at
#@@c

parallel()
smooth(0.5,W_state_wirings.lower_index_highlight,0)
smooth(0.5,W_state_wirings.upper_index_highlight,1)
linear(0.5,W_state_wirings.arrow_alpha,0)
smooth(0.5,W_state_wirings.center_matrix_element_highlight_00,1)
smooth(0.5,W_state_wirings.center_matrix_element_highlight_01,1)
smooth(0.5,W_state_wirings.center_matrix_element_highlight_10,1)
smooth(0.5,W_state_wirings.center_matrix_element_highlight_11,1)
end()

pause()

#@+at
# Replace matrix elements with vectors.
#@-at
#@@c

parallel()
linear(0.5,W_state_wirings.center_matrix_element_alpha_00,0)
linear(0.5,W_state_wirings.center_matrix_element_alpha_01,0)
linear(0.5,W_state_wirings.center_matrix_element_alpha_10,0)
linear(0.5,W_state_wirings.center_matrix_element_alpha_11,0)
linear(0.5,W_state_wirings.top_qubit_alpha,0)
linear(0.5,W_state_wirings.middle_qubit_alpha,0)
linear(0.5,W_state_wirings.bottom_qubit_alpha,0)
linear(0.5,W_state_wirings.vector_alpha,1)
end()

set(W_state_wirings.center_matrix_element_highlight_00,0)
set(W_state_wirings.center_matrix_element_highlight_01,0)
set(W_state_wirings.center_matrix_element_highlight_10,0)
set(W_state_wirings.center_matrix_element_highlight_11,0)

pause()

#@+at
# Dehighlight everything and fade labels back in.
#@-at
#@@c

parallel()
linear(0.5,W_state_wirings.vector_alpha,0)
linear(0.5,W_state_wirings.center_matrix_element_alpha_00,1)
linear(0.5,W_state_wirings.center_matrix_element_alpha_01,1)
linear(0.5,W_state_wirings.center_matrix_element_alpha_10,1)
linear(0.5,W_state_wirings.center_matrix_element_alpha_11,1)
linear(0.5,W_state_wirings.top_qubit_alpha,1)
linear(0.5,W_state_wirings.middle_qubit_alpha,1)
linear(0.5,W_state_wirings.bottom_qubit_alpha,1)
smooth(0.5,W_state_wirings.upper_index_highlight,0)
end()

pause()

#@-node:Point out meaning of sub/superscripts.
#@+node:Fade in outer sites
#@+at
# Fade in the outer sites.
#@-at
#@@c

linear(1,W_state_wirings.outer_sites_alpha,1)

pause()

#@-node:Fade in outer sites
#@+node:Zoom out to show equation
#@+at
# Zoom out to show the equation
#@-at
#@@c

smooth(1,W_state_wirings.window,4.5)
linear(0.5,W_state_wirings.equation_alpha,1)

pause()
#@-node:Zoom out to show equation
#@+node:Replace qubits with operators
#@+at
# Show that this also lets us get a formulation of the W operator.
#@-at
#@@c

linear(0.5,W_state_wirings.operator_slider,1)

pause()

#@-node:Replace qubits with operators
#@+node:Fade in environment picture
#@+at
# Fade in the old environment picture.
#@-at
#@@c

linear(0.5,W_state_wirings.diagram_alpha,0)
linear(0.5,W_state_wirings.environment_alpha,1)

pause()

#@+at
# Remind audience of earlier animation.
#@-at
#@@c

def drop_and_absorb(direction):
    smooth(0.5,W_state_wirings.center_circle_y,-1)
    linear(0.5,W_state_wirings.center_circle_color,grey)
    smooth(0.5,W_state_wirings.center_circle_x,direction)
    set(W_state_wirings.center_circle_color,Color(random(),random(),random()))
    set(W_state_wirings.center_circle_x,0)
    set(W_state_wirings.center_circle_y,1)

direction = -1
for i in xrange(4):
    drop_and_absorb(direction)
    direction *= -1

smooth(0.5,W_state_wirings.center_circle_y,-1)
linear(0.5,W_state_wirings.center_circle_color,grey)

pause()
#@-node:Fade in environment picture
#@+node:Display operator chain
#@+at
# Fade in the chain of operators atop the matrices.
#@-at
#@@c

linear(0.5,W_state_wirings.operator_chain_alpha,1)

set(W_state_wirings.draw_matrices,False)

pause()

#@+at
# Slide the labels into the operator chain
#@-at
#@@c

smooth(0.5,W_state_wirings.label_slider,1)
linear(0.5,W_state_wirings.outer_label_alpha,1)

pause()

#@+at
# Label the connections.
#@-at
#@@c

linear(0.5,W_state_wirings.connection_index_label_alpha,1)

pause()

#@+at
# Fade out connection labels.
#@-at
#@@c

linear(0.5,W_state_wirings.connection_index_label_alpha,0)

pause()

#@+at
# Replace tensor notation with "O"
#@-at
#@@c


linear(0.5,W_state_wirings.operator_label_alpha,0)
linear(0.5,W_state_wirings.uniform_operator_label_alpha,1)

pause()
#@-node:Display operator chain
#@+node:Display state chain
#@+at
# Replace tensor notation with "O"
#@-at
#@@c

set(W_state_wirings.center_circle_color,Color(random(),random(),random()))

parallel()
linear(0.5,W_state_wirings.equation_alpha,0)
linear(0.5,W_state_wirings.environment_alpha,0)
linear(0.5,W_state_wirings.state_chain_alpha,1)
end()

pause()
#@nonl
#@-node:Display state chain
#@+node:Connect expectation network
#@+at
# Connect the states and operators together to form an expectation network.
#@-at
#@@c

smooth(0.5,W_state_wirings.state_chain_height,1)

pause()
#@-node:Connect expectation network
#@+node:ENVIRONMONSTER!!!
#@+at
# Have the environmonster eat the left and right sites.
#@-at
#@@c

parallel()
smooth(1.5,W_state_wirings.left_environmonster_position,-2.625)

serial()
wait(0.75)
linear(0.75,W_state_wirings.left_environmonster_jaw_angle,0)
end()

end()

pause()

linear(0.5,W_state_wirings.left_environment_alpha,1)

set(W_state_wirings.left_environmonster_alpha,0)

pause()


parallel()
smooth(1.5,W_state_wirings.right_environmonster_position,2.625)

serial()
wait(0.75)
linear(0.75,W_state_wirings.right_environmonster_jaw_angle,0)
end()

end()

pause()

linear(0.5,W_state_wirings.right_environment_alpha,1)

set(W_state_wirings.right_environmonster_alpha,0)

pause()


#@-node:ENVIRONMONSTER!!!
#@+node:Converge!
left_hue = 96/256
right_hue = 224/256
current_left = 96/256
current_right = 96/256

ds = current_left/2

def add_interact_and_move(absorb_direction,with_pauses=False):
    #@    @+others
    #@+node:Interact site
    #@+at
    # Change the site's color to show how it relaxes.
    #@-at
    #@@c

    global current_left, current_right

    width = (current_left+current_right)/2

    if width < 0:
        width = 0

    linear(0.5,W_state_wirings.center_circle_color,hsv(random(),random()*width,0.5+(random()-0.5)*width))

    if with_pauses:
        pause()
    else:
        wait(0.25)
    #@-node:Interact site
    #@+node:Absorb site
    #@+at
    # Slide to left/right and update environment.
    #@-at
    #@@c

    if absorb_direction == -1:
        environment_background = W_state_wirings.left_environment_color
        current_left -= ds
        current_color = hsv(left_hue,current_left,0.5)
    elif absorb_direction == +1:
        environment_background = W_state_wirings.right_environment_color
        current_right -= ds
        current_color = hsv(right_hue,current_right,0.5)
    else:
        return

    parallel()

    smooth(0.5,W_state_wirings.center_circle_x,absorb_direction)

    grey = Color(0.5)

    serial()
    wait(0.25)

    linear(0.5,environment_background,current_color)
    end()

    end()

    if with_pauses:
        pause()

    #@-node:Absorb site
    #@+node:Remove and reset site
    #@+at
    # Move site to the starting position and reset its color so that it 
    # appears to be a new site.
    #@-at
    #@@c

    set(W_state_wirings.center_circle_alpha,0)
    set(W_state_wirings.center_circle_color,Color(random(),random(),random()))
    set(W_state_wirings.center_circle_x,0)
    #@-node:Remove and reset site
    #@+node:Add site
    #@+at
    # Fade site in while moving it into the slot.
    #@-at
    #@@c

    linear(0.5,W_state_wirings.center_circle_alpha,1)

    if with_pauses:
        pause()
    else:
        wait(0.25)
    #@-node:Add site
    #@-others

add_interact_and_move(-1,with_pauses=True)
add_interact_and_move(+1)
add_interact_and_move(-1)
add_interact_and_move(+1)

linear(0.5,W_state_wirings.center_circle_color,grey)

pause()
#@-node:Converge!
#@+node:Absorb environment into middle
#@+at
# Absorb the environment into the ubertensor.
#@-at
#@@c

set(W_state_wirings.outer_stuff_visible,False)
smooth(1,W_state_wirings.environment_absorbtion_slider,1)

pause()

smooth(0.5,W_state_wirings.index_merge_slider,1)

pause()

fade_out(0.5,W_state_wirings)
#@-node:Absorb environment into middle
#@-node:W state wiring
#@-others

crazy_interaction_animation = end_animation()


#test_objects(crazy_interaction_animation)
#@-node:Animation
#@+node:Results Slide
def results_slide():
    title1 = Text( get_camera().move_down(.005).move_right(.005), text = 'Results', size = 64,
                  font = fonts['big'], color = black, justify = 0.5, vjustify = 0.4, _alpha = 0.0 )
    title2 = Text( get_camera().move_up(.005).move_left(.005), text = 'Results', size = 64,
                  font = fonts['big'], color = yellow, justify = 0.5, vjustify = 0.4, _alpha = 0.0 )

    start_animation( bg, title1, title2 )

    pause()

    fade_in(0.5,title1,title2)

    pause()

    fade_out(0.5,title1,title2)

    return end_animation()
results_slide = results_slide()
#@-node:Results Slide
#@+node:Transverse Ising Model
title = Text(get_camera().top(1/4).inset(0.1),text="Transverse Ising Model",color=yellow,font=fonts["title"],size=24,justify=0.5,vjustify=0.5,_alpha=0)

leftover = get_camera().bottom(3/4)

tim = Image(leftover.top(2/3).inset(0.2),image=load_image("tim.png"),fit=HEIGHT,_alpha=0)

lam = Image(leftover.bottom(1/3).inset(1/3),image=load_image("lam.png"),fit=HEIGHT,_alpha=0)

start_animation(bg,title,tim,lam)

pause()

fade_in(0.5,title)

pause()

fade_in(0.5,tim)

pause()

fade_in(0.5,lam)

pause()

parallel()
fade_out(0.5,title)
fade_out(0.5,tim)
fade_out(0.5,lam)
end()

tim_slide = end_animation()
#@-node:Transverse Ising Model
#@+node:Visualization Guide
title = Text(get_camera().top(1/4).inset(0.1),text="Qubit Color Coding",color=yellow,font=fonts["title"],size=24,justify=0.5,vjustify=0.5,_alpha=0)

splits = get_camera().bottom(3/4).ysplit(0.1,1,0.1,1,0.1,1,0.1)
thirds = [splits[1],splits[3],splits[5]]

qubit1 = Drawable(thirds[0].left(1/3).restrict_aspect(1),draw_qubit,_alpha=0)
connector1 = Drawable(thirds[0].right(2/3).inset(0.2).restrict_aspect(4/1.5),draw_connector,_alpha=0)

qubit2 = Drawable(thirds[1].left(1/3).restrict_aspect(1),draw_qubit,angle=180,_alpha=0)
connector2 = Drawable(thirds[1].right(2/3).inset(0.2).restrict_aspect(4/1.5),draw_connector,angle=180,_alpha=0)

qubit3 = Drawable(thirds[2].left(1/3).restrict_aspect(1),draw_qubit,angle=90,_alpha=0)
connector3 = Drawable(thirds[2].right(2/3).inset(0.2).restrict_aspect(4/1.5),draw_connector,angle=90,_alpha=0)

start_animation(bg,title,qubit1,connector1,qubit2,connector2,qubit3,connector3)

pause()

fade_in(0.5,title)

pause()

parallel()
fade_in(0.5,qubit1)
fade_in(0.5,connector1)
end()

pause()

parallel()
fade_in(0.5,qubit2)
fade_in(0.5,connector2)
end()

pause()

parallel()
fade_in(0.5,qubit3)
fade_in(0.5,connector3)
end()

pause()

parallel()
linear(1,qubit1.angle,135)
linear(1,connector1.angle,135)
end()

pause()

parallel()
linear(1,qubit2.angle,135+180)
linear(1,connector2.angle,135+180)
end()

pause()

parallel()
linear(1,qubit3.angle,135+90)
linear(1,connector3.angle,135+90)
end()


pause()

parallel()
smooth(1,qubit1.qubit_scale,1.8)
smooth(1,connector1.width,.9)
smooth(1,qubit2.qubit_scale,0.5)
smooth(1,connector2.width,0.25)
end()

pause()

parallel()
smooth(1,qubit1.qubit_scale,0.75)
smooth(1,connector1.width,0.375)
smooth(1,qubit3.qubit_scale,1.5)
smooth(1,connector3.width,0.625)
end()

pause()

parallel()
fade_out(0.5,title)
fade_out(0.5,qubit1)
fade_out(0.5,qubit2)
fade_out(0.5,qubit3)
fade_out(0.5,connector1)
fade_out(0.5,connector2)
fade_out(0.5,connector3)
end()

visualization_guide = end_animation()
#@-node:Visualization Guide
#@+node:Demonstration
print "Initializing simulation..."
print "Faking data... (heh heh heh, the audience will NEVER know...)"
print "\t[TODO:  Delete above line before giving presentation!]"

#@<< Initialize system >>
#@+node:<< Initialize system >>
from InfiniteOpenProductState import OneDimensionalSystem, MatrixProductOperatorTerm

auxiliary_dimension = 2
lam = 0.9

number_of_sites=10

#timestamp = start_timing_event("Initializing configuration (including pre-normalization of sites array)...")

open_site_boundary = rand(auxiliary_dimension)
#open_site_boundary = array([1,]+[1,]*(auxiliary_dimension-1))

system = OneDimensionalSystem(
    site_dimension=2,
    auxiliary_dimension=auxiliary_dimension,
    initial_left_site_boundary=open_site_boundary,
    initial_right_site_boundary=open_site_boundary,
    )

#finished_timing_event(timestamp,"done; %i seconds taken.")

#@+at
# Create the Hamiltonian.
#@-at
#@@c

Z = array([1,0,0,-1],complex128).reshape(2,2)
Y = array([0,1j,-1j, 0],complex128).reshape(2,2)
X = array([0,1,1, 0],complex128).reshape(2,2)
I = array([1,0,0, 1],complex128).reshape(2,2)

#@<< Dense operators >>
#@+node:<< Dense operators >>
spin_coupling_operator_matrix = zeros((3,3,2,2),complex128)
spin_coupling_operator_matrix[0,0] =  I
spin_coupling_operator_matrix[2,2] =  I
spin_coupling_operator_matrix[0,1] =  X*(-lam)
spin_coupling_operator_matrix[1,2] =  X
spin_coupling_operator_matrix[0,2] = -Z
spin_coupling_term = MatrixProductOperatorTerm(
    system,
    spin_coupling_operator_matrix,
    array([1,0,0]),
    array([0,0,1])
    )
#@-node:<< Dense operators >>
#@nl

system.terms.append(spin_coupling_term)
#@-node:<< Initialize system >>
#@nl

start_animation(bg)

#@+others
#@+node:class Result Cache
class ResultCache:
    #@    @+others
    #@+node:__init__
    def __init__(self,system,increment_list={}):
        self.system = system
        self.matrices = {}
        self.unnormalized_matrices = {}
        self.old_matrices = {}
        self.energy_agreements = {}
        self.site_agreements = {}
        self.energy_residuals = {}
        self.energies = {}
        self.total_energies = {}
        self.increment_list = increment_list
        self.last_direction = +1
        self.last_index = -1
        self.system_size = 0
    #@-node:__init__
    #@+node:matrix fetching routines
    def get_matrix(self,index):
        self.ensure_in_cache(index)
        try:
            return self.matrices[index]
        except KeyError:
            print self.matrices.keys()
            raise


    def get_unnormalized_matrix(self,index):
        self.ensure_in_cache(index)
        return self.unnormalized_matrices[index]

    def get_old_matrix(self,index):
        self.ensure_in_cache(index)
        return self.old_matrices[index]
    #@-node:matrix fetching routines
    #@+node:energy fetching routines
    def get_site_agreement(self,index):
        self.ensure_in_cache(index)
        return self.site_agreements[index]

    def get_energy_agreement(self,index):
        self.ensure_in_cache(index)
        return self.energy_agreements[index]

    def get_energy_residual(self,index):
        self.ensure_in_cache(index)
        return self.energy_residuals[index]
    #@-node:energy fetching routines
    #@+node:ensure_in_cache
    def ensure_in_cache(self,requested_index):
        if self.last_index >= requested_index:
            return

        while self.last_index < requested_index:
            self.last_index += 1
            self.last_direction *= -1
            self.system_size += 1

            self.system.single_site_run(self.last_direction)

            self.matrices[self.last_index] = self.system.site_matrix.copy()
            self.unnormalized_matrices[self.last_index] = self.system.unnormalized_site_matrix.copy()

            self.total_energies[self.last_index] = system.compute_energy()
            self.energies[self.last_index] = infinite_energy_limit(self.matrices[self.last_index])
            self.energy_residuals[self.last_index] = -log(infinite_energy_residual(self.energies[self.last_index]))/log(10)

            if self.last_index > 0:
                #self.energies[self.last_index] = self.total_energies[self.last_index]-self.total_energies[self.last_index-1]
                #self.energy_residuals[self.last_index] = -log(infinite_energy_residual(self.energies[self.last_index]))/log(10)
                self.site_agreements[self.last_index]  = -log(norm(self.unnormalized_matrices[self.last_index]-self.unnormalized_matrices[self.last_index-1]))/log(10)

            if self.last_index > 1:
                self.energy_agreements[self.last_index] = -log(abs(self.energies[self.last_index]-self.energies[self.last_index-1]))/log(10)

            if self.last_index in self.increment_list:
                self.old_matrices[self.last_index+10] = self.matrices[self.last_index]
                self.old_matrices[self.last_index+ 9] = self.matrices[self.last_index-1]
                unitary = self.system.increase_auxiliary_dimension_by(self.increment_list[self.last_index])
                self.matrices[self.last_index+10] = multiply_tensor_by_matrix_at_index(self.matrices[self.last_index],unitary.conj(),1)
                self.matrices[self.last_index+ 9] = multiply_tensor_by_matrix_at_index(self.matrices[self.last_index-1],unitary,2)

                self.unnormalized_matrices[self.last_index+10] = multiply_tensor_by_matrix_at_index(self.unnormalized_matrices[self.last_index],unitary.conj(),1)
                self.unnormalized_matrices[self.last_index+10] = multiply_tensor_by_matrix_at_index(self.unnormalized_matrices[self.last_index+10],unitary,2)

                self.site_agreements[self.last_index+10] = self.site_agreements[self.last_index]
                self.energy_agreements[self.last_index+10] = self.energy_agreements[self.last_index]
                self.energy_residuals[self.last_index+10] = self.energy_residuals[self.last_index]
                self.energies[self.last_index+10] = self.energies[self.last_index]
                self.energies[self.last_index+10] = self.total_energies[self.last_index]

                #self.site_agreements[self.last_index+9] = self.site_agreements[self.last_index-1]
                #self.energy_agreements[self.last_index+9] = self.energy_agreements[self.last_index-1]
                #self.energy_residuals[self.last_index+9] = self.energy_residuals[self.last_index-1]

                self.last_index += 10

    #@-node:ensure_in_cache
    #@-others
#@-node:class Result Cache
#@+node:Functions
#@+node:my_eigen_schur
import arpack
import numpy as sb

def my_eigen_schur(matvec,n,k=2,ncv=5,
          maxiter=None,tol=0,guess=None,which='LM'):

    # some defaults
    if ncv is None:
        ncv=2*k+1
    ncv=min(ncv,n)
    if maxiter==None:
        maxiter=n*10

    typ = 'D'

    # some sanity checks
    if k <= 0:
        raise ValueError("k must be positive, k=%d"%k)
    if k >= n:
        raise ValueError("k must be less than rank(A), k=%d"%k)
    if maxiter <= 0:
        raise ValueError("maxiter must be positive, maxiter=%d"%maxiter)
    if ncv > n or ncv < k:
        raise ValueError("ncv must be k<=ncv<=n, ncv=%s"%ncv)

    eigsolver = arpack._arpack.znaupd
    eigextract = arpack._arpack.zneupd

    v = sb.zeros((n,ncv),typ) # holds Ritz vectors
    if guess is not None:
        guess = guess.copy().ravel()
        if guess.shape != (n,):
            raise ValueError, "guess has invalid dimensions [%s!=(%i,)]" % (guess.shape,n)
        resid = guess
        info = 1
    else:
        resid = sb.zeros(n,typ) # residual
        info = 0
    workd = sb.zeros(3*n,typ) # workspace
    workl = sb.zeros(3*ncv*ncv+6*ncv,typ) # workspace
    iparam = sb.zeros(11,'int') # problem parameters
    ipntr = sb.zeros(14,'int') # pointers into workspaces
    ido = 0

    rwork = sb.zeros(ncv,typ.lower())

    bmat = 'I'
    mode1 = 1

    ishfts = 1
    iparam[0] = ishfts
    iparam[2] = maxiter
    iparam[6] = mode1

    while ido != 99:

        ido,resid,v,iparam,ipntr,info =\
            eigsolver(ido,bmat,which,k,tol,resid,v,iparam,ipntr,
                      workd,workl,rwork,info)
        #if ido == 99:
        #    break
        #else:
        #source_slice      = slice(ipntr[0]-1, ipntr[0]-1+n)
        #destination_slice = slice(ipntr[1]-1, ipntr[1]-1+n)
        #workd[destination_slice]=matvec(workd[source_slice])
        workd[ipntr[1]-1:ipntr[1]-1+n] = matvec(workd[ipntr[0]-1:ipntr[0]-1+n])

    if info < -1 :
        raise RuntimeError("Error info=%d in arpack"%info)
        return None
    if info == -1:
        warnings.warn("Maximum number of iterations taken: %s"%iparam[2])

    # now extract eigenvalues and (optionally) eigenvectors        
    rvec = True #return_eigenvectors
    ierr = 0
    howmny = 'P' # return all eigenvectors
    sselect = sb.zeros(ncv,'int') # unused
    sigmai = 0
    sigmar = 0

    workev = sb.zeros(3*ncv,typ) 

    d,z,info =\
          eigextract(rvec,howmny,sselect,sigmar,workev,
                     bmat,which,k,tol,resid,v,iparam,ipntr,
                     workd,workl,rwork,ierr)   

    if ierr != 0:
        raise RuntimeError("Error info=%d in arpack"%info)
        return None

    my_eigen_schur.number_of_iterations = iparam[2]
    my_eigen_schur.resid = resid

    return d,z

#@-node:my_eigen_schur
#@+node:infinite_energy_limit

def infinite_energy_limit(site_matrix):

    auxiliary_dimension = system.auxiliary_dimension

    evals, evecs = my_eigen_schur(
        lambda R: special_left_multiply(R.reshape(auxiliary_dimension,auxiliary_dimension,3),site_matrix,spin_coupling_operator_matrix).ravel(),
        auxiliary_dimension**2 * 3,
        k=2,
        ncv=5,
        maxiter=1000,
        #tol=1e-13
        )

    sorted_indices = argsort(abs(evals))

    #print evals

    evecs = evecs[:,sorted_indices[-2:]]

    matrix = zeros((2,2),complex128)

    for i in xrange(2):
        for j in xrange(2):
            matrix[i,j] = dot(evecs[:,i].conj(),special_left_multiply(evecs[:,j].reshape(auxiliary_dimension,auxiliary_dimension,3),site_matrix,spin_coupling_operator_matrix).ravel())

    epsilon = sqrt(trace(dot(matrix,matrix.transpose().conj()))-2)



    new_right_evecs = [dot(evecs[:,i].reshape(auxiliary_dimension,auxiliary_dimension,1,3),array([0,0,1])) for i in [0,1]]
    new_left_evecs = [dot(evecs[:,i].reshape(auxiliary_dimension,auxiliary_dimension,1,3),array([1,0,0])) for i in [0,1]]

    #print matrix

    matrix = zeros((2,2),complex128)

    for i in xrange(2):
        for j in xrange(2):
            matrix[i,j] = dot(new_left_evecs[i].conj().ravel(),special_left_multiply(new_right_evecs[j],site_matrix,identity(2).reshape(1,1,2,2)).ravel())

    #print matrix

    epsilon_norm = sqrt(trace(dot(matrix,matrix.transpose().conj())))

    #print epsilon, epsilon_norm, epsilon/epsilon_norm

    return -real(epsilon/epsilon_norm)
#@-node:infinite_energy_limit
#@+node:function infinite_energy_limit
def infinite_energy_limit(site_matrix):
    #@    @+others
    #@+node:Initialization
    A = site_matrix

    site_dimension = site_matrix.shape[0]
    auxiliary_dimension = site_matrix.shape[1]

    I = identity(2)

    #X = array([[0,1],[1,0]])
    #Z = array([[1,0],[0,-1]])
    #@-node:Initialization
    #@+node:Calculate left and right environment tensors
    S = site_matrix
    O = identity(site_dimension).reshape(1,1,site_dimension,site_dimension)

    def compute_environment(S,O):

        O_auxiliary_dimension = O.shape[0]

        retries = 0
        n = 0

        while n < 1e-10 and retries < 1:
            if retries > 0:
                print "REPEAT!!!"

            right_evals, right_evecs = my_eigen(
                lambda R: special_left_multiply(R.reshape(auxiliary_dimension,auxiliary_dimension,O_auxiliary_dimension),S,O).ravel(),
                O_auxiliary_dimension*auxiliary_dimension**2,k=1,ncv=3,which='LM')

            n = norm(right_evecs)

            #OO = compute_transfer_matrix(S,O).reshape((auxiliary_dimension**2,)*2)

            #print "REV:",right_evals
            #print [norm(right_evec) for right_evec in right_evecs.transpose()]

            #for eval in eigvals(OO):
            #    print eval

            #OO = compute_transfer_matrix(S,O).reshape((auxiliary_dimension,)*4).transpose(0,2,1,3).reshape((auxiliary_dimension**2,)*2)

            retries += 1

        retries = 0
        n = 0

        while n < 1e-10 and retries < 1:
            if retries > 0:
                print "REPEAT!!!"

            left_evals, left_evecs = my_eigen(
                lambda L: special_right_multiply(L.reshape(auxiliary_dimension,auxiliary_dimension,O_auxiliary_dimension),S,O).ravel(),
                O_auxiliary_dimension*auxiliary_dimension**2,k=1,ncv=3,which='LM')

            n = norm(left_evecs)
            retries += 1

            #print "LEV:",left_evals
            #print [norm(left_evec) for left_evec in left_evecs.transpose()]

        L = left_evecs[:,0].reshape((auxiliary_dimension,auxiliary_dimension,O_auxiliary_dimension))
        R = right_evecs[:,0].reshape((auxiliary_dimension,auxiliary_dimension,O_auxiliary_dimension))

        #print "EVALS:",left_evals,left_evecs.transpose(),dot(left_evecs[:,0],left_evecs[:,1])

        return L, R

    L, R = compute_environment(S,O)
    L = L.reshape((auxiliary_dimension,)*2)
    R = R.reshape((auxiliary_dimension,)*2)

    #compute_environment(S,magnetic_field_operator_matrix.to_array())
    #compute_environment(S,spin_coupling_operator_matrix.to_array())

    #@-node:Calculate left and right environment tensors
    #@+node:Compute Z term
    g = Graph()

    g.add_node(L)
    g.add_node(R)

    g.add_node(A)
    g.add_node(-Z)
    g.add_node(conj(A))

    g.connect(0,0,2,1)
    g.connect(0,1,4,1)
    g.connect(2,0,3,0)
    g.connect(4,0,3,1)
    g.connect(1,0,2,2)
    g.connect(1,1,4,2)

    s = Subgraph(g)
    s.add_node(0)
    s.add_node(1)
    s.add_node(2)
    s.add_node(3)
    s.add_node(4)

    Z_exp = s.merge_all().matrices[0]

    #print "Z_exp=",Z_exp

    #@-node:Compute Z term
    #@+node:Compute XX term
    g = Graph()

    g.add_node(L)
    g.add_node(R)

    g.add_node(A)
    g.add_node(-lam*X)
    g.add_node(conj(A))

    g.add_node(A)
    g.add_node(X)
    g.add_node(conj(A))

    g.connect(0,0,2,1)
    g.connect(0,1,4,1)
    g.connect(2,0,3,0)
    g.connect(4,0,3,1)

    g.connect(2,2,5,1)
    g.connect(4,2,7,1)
    g.connect(5,0,6,0)
    g.connect(7,0,6,1)

    g.connect(1,0,5,2)
    g.connect(1,1,7,2)

    s = Subgraph(g)
    s.add_node(0)
    s.add_node(1)
    s.add_node(2)
    s.add_node(3)
    s.add_node(4)
    s.add_node(5)
    s.add_node(6)
    s.add_node(7)

    XX_exp = s.merge_all().matrices[0]

    #print "XX_exp=",XX_exp

    #@-node:Compute XX term
    #@+node:Compute Z normalization
    g = Graph()

    g.add_node(L)
    g.add_node(R)

    g.add_node(A)
    g.add_node(I)
    g.add_node(conj(A))

    g.connect(0,0,2,1)
    g.connect(0,1,4,1)
    g.connect(2,0,3,0)
    g.connect(4,0,3,1)
    g.connect(1,0,2,2)
    g.connect(1,1,4,2)

    s = Subgraph(g)
    s.add_node(0)
    s.add_node(1)
    s.add_node(2)
    s.add_node(3)
    s.add_node(4)

    NZ_exp = s.merge_all().matrices[0]

    #print "NZ_exp=",NZ_exp

    #@-node:Compute Z normalization
    #@+node:Compute XX normalization
    g = Graph()

    g.add_node(L)
    g.add_node(R)

    g.add_node(A)
    g.add_node(I)
    g.add_node(conj(A))

    g.add_node(A)
    g.add_node(I)
    g.add_node(conj(A))

    g.connect(0,0,2,1)
    g.connect(0,1,4,1)
    g.connect(2,0,3,0)
    g.connect(4,0,3,1)

    g.connect(2,2,5,1)
    g.connect(4,2,7,1)
    g.connect(5,0,6,0)
    g.connect(7,0,6,1)

    g.connect(1,0,5,2)
    g.connect(1,1,7,2)

    s = Subgraph(g)
    s.add_node(0)
    s.add_node(1)
    s.add_node(2)
    s.add_node(3)
    s.add_node(4)
    s.add_node(5)
    s.add_node(6)
    s.add_node(7)

    NXX_exp = s.merge_all().matrices[0]

    #print "NXX_exp=",NXX_exp

    #@-node:Compute XX normalization
    #@+node:Compute energy
    #print (Z_exp+XX_exp)/N_exp
    #print Z_exp/NZ_exp+XX_exp/NXX_exp

    E = Z_exp/NZ_exp+XX_exp/NXX_exp

    #assert abs(imag(E)) < 1e-10

    return E

    #print real(E), "%.2f" % (-log(abs(E-correct_answer))/log(10))

    #@-node:Compute energy
    #@-others
#@-node:function infinite_energy_limit
#@+node:function display_result
def display_result(final_A,residual=None):
    print
    print
    print "...and the peasants rejoiced."
    print

    limit_energy_timestamp = start_timing_event("Calculating final energy...")

    E = infinite_energy_limit(final_A)

    finished_timing_event(limit_energy_timestamp,"done!  Took %.2f second.")
    print
    if abs(imag(E)) > 1e-10:
        print "Energy:",E
    else:
        print "Energy:",real(E)

    correct_answers = {
        1.1:-1.34286402273,
        1.01:-1.27970376371,
        1:-1.273239544735,
        0.99:-1.266972193465,
        0.95:-1.2432657042699999,
        0.9:-1.2160009141099999,
        0.5:-1.063544409975,
        }

    if lam in correct_answers:
        correct_answer = correct_answers[lam]

        print "This agrees to %.2f digits with expected answer. (residual=%.3g)" % (-log(abs(E-correct_answer))/log(10),abs(E-correct_answer))

    time_taken = (time()-uber_timestamp)/60
    print 
    print "%.1f minutes taken so far;  average rate is %.2f minutes/site" % (time_taken,time_taken/total_number_of_sites)


#@-node:function display_result
#@+node:function infinite_energy_residual
def infinite_energy_residual(E):
    E = real(E)

    correct_answers = {
        1.1:-1.34286402273,
        1.01:-1.27970376371,
        1.001:-1.27387751469,
        1.0001:-1.27330322388,
        1:-1.273239544735,
        0.9999:-1.27317589993,
        0.999:-1.27260427634,
        0.99:-1.266972193465,
        0.95:-1.2432657042699999,
        0.9:-1.2160009141099999,
        0.5:-1.063544409975,
        }

    assert lam in correct_answers

    return abs(E-correct_answers[lam])
    #return -log(abs(E-correct_answer))/log(10)

#@-node:function infinite_energy_residual
#@+node:callback
def callback(iteration_number,number_of_iterations,current_A,diff,time_elapsed):
    global total_time_elapsed
    global total_time_elapsed_this_run

    if not iteration_number % 500 == 0 or diff > 1e8:
        total_time_elapsed += time_elapsed
        total_time_elapsed_this_run += time_elapsed
        return

    converge_time_elapsed = time_elapsed

    residual_timestamp = time()
    residual = infinite_energy_residual(current_A)
    residual_time_elapsed = time()-residual_timestamp

    time_elapsed = converge_time_elapsed+residual_time_elapsed

    total_time_elapsed += time_elapsed
    total_time_elapsed_this_run += time_elapsed
    string = "Iteration %i/%i -- %.2fs+%.1fs, %.1fm tot, %i sites/minute; " % (iteration_number,number_of_iterations,converge_time_elapsed,residual_time_elapsed,total_time_elapsed/60.0,iteration_number/total_time_elapsed_this_run*60)


    energy_digits = -log(residual)/log(10)
    string +=  "energy ~ **%.2f digits**; site ~ %.2f digits" % (energy_digits,-log(diff)/log(10))
    global last_energy_digits
    last_energy_digits = energy_digits
    #string += str(residual)
    print string

    stdout.flush()

    udiffs.append(diff)
    number_of_sites.append(system.total_number_of_sites)
    residuals.append(residual)

#@-node:callback
#@+node:xcallback
def xcallback(iteration_number,number_of_iterations,current_A,diff,time_elapsed):
    global total_time_elapsed
    global total_time_elapsed_this_run

    converge_time_elapsed = time_elapsed

    residual_timestamp = time()
    residual = infinite_energy_residual(current_A)
    residual_time_elapsed = time()-residual_timestamp

    time_elapsed = converge_time_elapsed+residual_time_elapsed

    total_time_elapsed += time_elapsed
    total_time_elapsed_this_run += time_elapsed
    string = "Iteration %i/%i -- %.2fs+%.1fs, %.1fm tot, %i sites/minute; " % (iteration_number,number_of_iterations,converge_time_elapsed,residual_time_elapsed,total_time_elapsed/60.0,iteration_number/total_time_elapsed_this_run*60)


    energy_digits = -log(residual)/log(10)
    string +=  "energy ~ **%.2f digits**; site ~ %.2f digits" % (energy_digits,-log(diff)/log(10))
    global last_energy_digits
    last_energy_digits = energy_digits
    #string += str(residual)
    print string

    stdout.flush()

    udiffs.append(diff)
    number_of_sites.append(system.total_number_of_sites)
    residuals.append(residual)

#@-node:xcallback
#@-node:Functions
#@+node:Initialization
cache = ResultCache(system,increment_list={9:2,39:2,79:4,119:5})

visualizer = Drawable(get_camera(),draw_visualizer,cache=cache,unnormalized_shuffled_ports=array([1,0]),unnormalized_slider=1,_alpha=0)

meters = []
#@-node:Initialization
#@+node:def run_site
def run_site(direction,index,seconds_per_run=2,port_shuffle_number=None,window=2):

    for meter in meters:
        set(meter.old_index,get(meter.new_index))
        set(meter.new_index,index)
        set(meter.slider,0)

    if direction == -1:
        shift = visualizer.left_shift
        set(visualizer.break_left,True)
    else:
        shift = visualizer.right_shift
        set(visualizer.break_left,False)

    set(visualizer.middle_matrix_index,index)
    set(visualizer.unnormalized_slider,1)

    if port_shuffle_number:
        port_shuffle = range(port_shuffle_number)
        shuffle(port_shuffle)
        port_shuffle = array(port_shuffle)
        set(visualizer.unnormalized_shuffled_ports,port_shuffle)

    parallel()
    linear(seconds_per_run/2,visualizer.middle_matrix_alpha,1)
    for meter in meters:
        smooth(seconds_per_run/2,meter.slider,1)
    end()

    parallel()
    smooth(seconds_per_run/2,shift,1)
    linear(seconds_per_run/2,visualizer.unnormalized_slider,0)
    end()

    def add_index(var,index):
        value = get(var)
        value = [index] + value[:window]
        set(var,value)        

    if direction == -1:
        add_index(visualizer.left_matrix_indices,index)
    else:
        add_index(visualizer.right_matrix_indices,index)

    set(shift,0)
    set(visualizer.middle_matrix_alpha,0)
#@-node:def run_site
#@+node:Belabor first two sites
enter(visualizer)

pause()

fade_in(0.5,visualizer)

pause()

linear(1,visualizer.middle_matrix_alpha,1)

pause()

smooth(1,visualizer.left_shift,1)

pause()

linear(1,visualizer.unnormalized_slider,0)

pause()

set(visualizer.middle_matrix_alpha,0)
set(visualizer.middle_port_alpha,0)
set(visualizer.middle_matrix_index,1)
set(visualizer.left_matrix_indices,[0])
set(visualizer.break_left,False)
set(visualizer.left_shift,0)
set(visualizer.unnormalized_slider,1)

linear(1,visualizer.middle_port_alpha,1)

pause()

linear(1,visualizer.middle_matrix_alpha,1)

pause()

smooth(1,visualizer.right_shift,1)

pause()

linear(1,visualizer.unnormalized_slider,0)

pause()

set(visualizer.right_shift,0)
set(visualizer.middle_matrix_alpha,0)
set(visualizer.right_matrix_indices,[1])
#@-node:Belabor first two sites
#@+node:Run through a couple more
direction = -1

for index in xrange(2,4):
    run_site(direction,index,2)
    direction *= -1

pause()
#@-node:Run through a couple more
#@+node:Introduce meters
meter_spots = get_camera().bottom(1/3).xsplit(1,1,1)

meters = [None,None,None]

meters[0] = Drawable(
    meter_spots[0].inset(0.1).restrict_aspect(1),
    draw_meter,
    label="Site Agreement",
    fetcher=cache.get_site_agreement,
    old_index=2,
    new_index=3,
    slider=1,
    _alpha=0
    )
meters[1] = Drawable(
    meter_spots[1].inset(0.1).restrict_aspect(1),
    draw_meter,
    label="Energy Agreement",
    fetcher=cache.get_energy_agreement,
    old_index=2,
    new_index=3,
    slider=1,
    _alpha=0
    )
meters[2] = Drawable(
    meter_spots[2].inset(0.1).restrict_aspect(1),
    draw_meter,
    label="Energy Residual",
    fetcher=cache.get_energy_residual,
    old_index=2,
    new_index=3,
    slider=1,
    _alpha=0
    )

enter(*meters)

for meter in meters:
    fade_in(0.5,meter)

    pause()
#@-node:Introduce meters
#@+node:Run through up to ten
for index in xrange(4,10):
    run_site(direction,index,2)
    direction *= -1

pause()
#@-node:Run through up to ten
#@+node:def increase_to_reflect_chi
def increase_to_reflect_chi(old_chi,new_chi,period=1):
    def update_index(index_list):
        old_indices = get(index_list)
        set(index_list,[old_indices[0]+10]+old_indices[1:])

    update_index(visualizer.left_matrix_indices)
    update_index(visualizer.right_matrix_indices)
    set(visualizer.old_slider,1)

    port_shuffle = range(new_chi)
    shuffle(port_shuffle)
    port_shuffle = array(port_shuffle[:old_chi])

    set(visualizer.old_shuffled_ports,port_shuffle)

    linear(period,visualizer.old_slider,0)
    set(visualizer.old_shuffled_ports,[])

    for meter in meters:
        set(meter.new_index,get(meter.new_index)+10)
        set(meter.slider,1)


#@-node:def increase_to_reflect_chi
#@+node:Increase the number of ports
increase_to_reflect_chi(2,4)

pause()

#@-node:Increase the number of ports
#@+node:Run though more sites, getting faster
speed = 0.5

for index in xrange(20,40):
    run_site(direction,index,1/speed,4)
    direction *= -1
    speed += 0.2

pause()
#@-node:Run though more sites, getting faster
#@+node:Increase and do run with 6 ports
increase_to_reflect_chi(4,6)

pause()

speed = 1

for index in xrange(50,80):
    run_site(direction,index,1/speed,6,window=3)
    if speed < 3:
        speed += 0.1
    direction *= -1

pause()
#@-node:Increase and do run with 6 ports
#@+node:Increase and do run with 10 ports
increase_to_reflect_chi(6,10,2)

pause()

smooth(1,visualizer.height,1.5)

pause()

speed = 0.5

for index in xrange(90,120):
    run_site(direction,index,1/speed,10,window=4)
    if speed < 2:
        speed += 0.1
    direction *= -1

pause()
#@-node:Increase and do run with 10 ports
#@+node:Increase and do run with 15 ports
increase_to_reflect_chi(10,15,3)

pause()

smooth(1,visualizer.height,2)

pause()

speed = 0.3

for index in xrange(130,160):
    run_site(direction,index,1/speed,15,window=5)
    if speed < 1.5:
        speed += 0.1
    direction *= -1

pause()
#@-node:Increase and do run with 15 ports
#@-others

demonstration = end_animation()
#@-node:Demonstration
#@+node:Haldane-Shastry Model
title = Text(get_camera().top(1/8).inset(0.1),text="Haldane-Shastry Model",color=yellow,font=fonts["title"],size=24,justify=0.5,vjustify=0.5,_alpha=0)

leftover = get_camera().bottom(7/8)

hsm = Image(leftover.inset(0.1),image=load_image("hsm.png"),fit=WIDTH,_alpha=0)

correlators = Image(get_camera().bottom(7/8),image=load_image("correlators.png"),fit=HEIGHT,_alpha=0)

start_animation(bg,title,hsm,correlators)

pause()

fade_in(0.5,title)

pause()

fade_in(0.5,hsm)

pause()

fade_in(0.5,correlators)

#set(title._alpha,0)
set(hsm._alpha,0)

pause()

fade_out(0.5,title,correlators)

hsm_slide = end_animation()
#@-node:Haldane-Shastry Model
#@+node:Remarks
def remarks_slide():
    r = get_camera().top( 0.2 ).bottom( 0.85 ).inset( 0.05 )
    title = Text( r, text = 'Remarks', font = fonts['title'], size = 30, justify = 0.0,
                  color = yellow, _alpha = 0 )
    r = get_camera().bottom( 0.85 ).inset( 0.05 )
    bl = BulletedList( r, font = fonts['roman'], color = white,
                       bullet = [fonts['dingbats'], 'w'],
                       size = 18 )

    start_animation( bg, title, bl )

    pause()

    fade_in(0.5,title)

    pause()
    bl.add_item( 0, ['Algorithm is similar in spirit to DMRG'] )
    pause()
    bl.add_item( 1, ['K. Ueda, T. Nishino, K. Okunishi, Y. Hieida, R. Derian and A. Gendiar. Journal of the Physical Society of Japan. 75, no. 1, (2006): 14003'] )
    pause()
    bl.add_item( 0, ['Differences'] )
    pause()
    bl.add_item( 1, ['Infinite systems modeled directly'] )
    pause()
    bl.add_item( 1, ['Translationally invariance built-in'] )
    pause()
    bl.add_item( 1, ['Handles (factorizeable) long-range interactions'] )
    pause()
    bl.add_item( 1, ['Directly applied to 1D quantum systems'] )
    pause()
    bl.add_item( 1, ['Extendable to 2D quantum systems (in progress)'] )
    pause()

    parallel()
    fade_out(0.5,bl)
    fade_out(0.5,title)
    end()
    return end_animation()
remarks_slide = remarks_slide()
#@-node:Remarks
#@+node:Conclusions
def conclusions_slide():
    r = get_camera().top( 0.2 ).bottom( 0.85 ).inset( 0.05 )
    title = Text( r, text = 'Morals', font = fonts['title'], size = 30, justify = 0.0,
                  color = yellow, _alpha = 0 )
    r = get_camera().bottom( 0.85 ).inset( 0.05 )
    bl = BulletedList( r, font = fonts['roman'], color = white,
                       bullet = [fonts['dingbats'], 'w'],
                       size = 18 )

    start_animation( bg, title, bl )

    pause()

    fade_in(0.5,title)

    pause()
    bl.add_item( 0, ['Physical picture of relaxation --> variational algorithm'] )
    pause()
    bl.add_item( 0, ['Matrix product states allow global entanglement properties to be described locally.'] )
    pause()
    bl.add_item( 0, ["This works in part because it formalizes our own intuitive pictures."] )
    pause()

    return end_animation()
conclusions_slide = conclusions_slide()
#@-node:Conclusions
#@-node:Slides
#@-others
#@-node:@file slides.py
#@-leo
