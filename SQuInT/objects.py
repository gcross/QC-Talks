#@+leo-ver=4
#@+node:@file objects.py
from __future__ import division

from random import random, randrange, randint, choice as random_choice

from fonts import fonts

from slithy.library import *
from slithy.transition import *

from math import *

from numpy import arange

#@+others
#@+node:Transitions
#@+node:Exponential Transition
from math import exp, log

class _ExponentialTransition(TransitionStyle):
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

Transition.styles["exp"] = _ExponentialTransition

exponential = Transition(style="exp")
#@-node:Exponential Transition
#@+node:Logarithmic Transition
from math import exp, log

class _LogarithmicTransition(TransitionStyle):
    def __init__( self, starttime, duration, start, end, params, type ):
        TransitionStyle.__init__( self, starttime, duration, start, end, params, type )
        self.s = params.get("s",10)
        self.l = log(1+self.s)

    def __call__( self, t ):
        if self.duration == 0.0:
            f = 1
        else:
            f = (t - self.starttime) / self.duration

        f = log(1+self.s * f)/self.l

        if hasattr( self, 'range' ):
            return self.start + self.range * f
        else:
            return self.start.interp( self.end, f )

Transition.styles["log"] = _LogarithmicTransition

logarithmic = Transition(style="log")
#@-node:Logarithmic Transition
#@-node:Transitions
#@+node:Background
bg = Fill( style = 'horz', color = black, color2 = blue )
#@-node:Background
#@+node:Images
mps_equation = load_image("mps.png")
mpo_equation = load_image("mpo.png")
#@-node:Images
#@+node:Colors
grey = Color(0.5,0.5,0.5)
port_fill_color = white.interp(grey,0.5)
#@-node:Colors
#@+node:Paths
#@+node:Sine paths
def create_sine_path(number_of_waves):

    sine_path = Path().moveto(0,0)

    for i in xrange(number_of_waves):
        sine_path.rcurveto(0.125, 0.375,0.375, 0.375,0.5,0)
        sine_path.rcurveto(0.125,-0.375,0.375,-0.375,0.5,0)

    return sine_path

long_sine_path = create_sine_path(25)
short_sine_path = create_sine_path(8)
#@-node:Sine paths
#@+node:Qubit path
qubit = Path().moveto(0,0).rlineto(0,0.5)
#@nonl
#@-node:Qubit path
#@-node:Paths
#@+node:Functions
#@+node:def draw_with_shadows
def draw_with_shadows(x,y,offset,f,top_color,alpha=1):
    push()
    translate(x+offset/2,y-offset/2)
    color(black,alpha)
    f()
    translate(-offset,offset)
    color(top_color,alpha)
    f()
    pop()
#@-node:def draw_with_shadows
#@+node:def draw_centered_rectangle
def draw_centered_rectangle(x,y,width,height,border_color,fill_color):
    color(fill_color)
    rectangle(x-width/2,y-height/2,x+width/2,y+height/2)
    color(border_color)
    frame(x-width/2,y-height/2,x+width/2,y+height/2)
#@-node:def draw_centered_rectangle
#@+node:def faded_rectangle
def faded_rectangle(c,x1,x2,y,height):
    dx = (x2-x1)/20
    x = x1
    y1 = y-height/2
    y2 = y+height/2
    c_final = Color(c,0)
    for i in xrange(20):
        color(c.interp(c_final,i/20))
        rectangle(x,y1,x+dx,y2)
        x += dx
#@-node:def faded_rectangle
#@-node:Functions
#@+node:Objects
#@+others
#@+node:Circles
#@+node:Generate colors
thousand_colors = []
for i in xrange(1000):
    thousand_colors.append(Color(random(),random(),random()))

initial_five_colors = thousand_colors[498:503]
#@-node:Generate colors
#@+node:Thousand circles
def draw_thousand_circles():
    set_camera(Rect(0,0,width=1000,height=1,anchor="w"))

    for i in xrange(1000):
        color(thousand_colors[i])
        dot(0.3,i,0)
#@nonl
#@-node:Thousand circles
#@+node:Circles w/ waves
def draw_five_circles(
    short_waves=(OBJECT,[]),
    long_waves=(OBJECT,[]),
    wave_time=(SCALAR,0,1000,0),
    color0=(COLOR,initial_five_colors[0]),
    color1=(COLOR,initial_five_colors[1]),
    color2=(COLOR,initial_five_colors[2]),
    color3=(COLOR,initial_five_colors[3]),
    color4=(COLOR,initial_five_colors[4]),
    color5=(COLOR,hsv(random(),1,0.5+0.5*random(),0)),
    color6=(COLOR,hsv(random(),1,0.5+0.5*random(),0)),
    x0=(SCALAR,-10,10,-2),
    x1=(SCALAR,-10,10,-1),
    x2=(SCALAR,-10,10, 0),
    x3=(SCALAR,-10,10, 1),
    x4=(SCALAR,-10,10, 2),
    x5=(SCALAR,-10,10, 0),
    y5=(SCALAR,-5,5,1.5),
    x6=(SCALAR,-10,10, 0),
    y6=(SCALAR,-5,5,1.5),
    environment_alpha=(SCALAR,0,1,0),
    left_environment_background=(COLOR,Color(0.5,0.5,0.5)),
    right_environment_background=(COLOR,Color(0.5,0.5,0.5)),
    ):
    set_camera(Rect(0,0,width=5,height=5*(3/4)))

    for (waves,path,full_wavelength) in [(short_waves,short_sine_path,8),(long_waves,long_sine_path,25)]:
        for (x,y,angle,start_time,wave_color) in waves:
            #if start_time > wave_time or (wave_time-start_time) > full_wavelength:
            #    continue
            push()
            translate(x,y)
            rotate(angle)
            scale(0.1)
            token = clip(Rect(wave_time-start_time,0,width=3,height=1,anchor="w"))
            color(wave_color)
            widestroke(path,0.1)
            unclip(token)
            pop()

    y0 = y1 = y2 = y3 = y4 = 0

    v = vars()

    for (x,y,color_) in ([v["%s%i"%(s,i)] for s in ["x","y","color"]] for i in xrange(7)):
        color(color_)
        dot(0.3,x,y)

    if environment_alpha > 0:
        color(left_environment_background, environment_alpha)
        rectangle(-3,-0.5,-0.5,0.5)
        color(right_environment_background, environment_alpha)
        rectangle(0.5,-0.5,3,0.5)

        thickness(0.01)
        color(left_environment_background.interp(white,0.5), environment_alpha)
        frame(-3,-0.5,-0.5,0.5)
        color(right_environment_background.interp(white,0.5), environment_alpha)
        frame(0.5,-0.5,3,0.5)

        color(black,environment_alpha)
        text(-1.5+0.01,0-0.01,"Left",fonts['big'],size=0.5)
        text( 1.5+0.01,0-0.01,"Right",fonts['big'],size=0.5)

        color(white,environment_alpha)
        text(-1.5-0.01,0+0.01,"Left",fonts['big'],size=0.5)
        text( 1.5-0.01,0+0.01,"Right",fonts['big'],size=0.5)
#@-node:Circles w/ waves
#@-node:Circles
#@+node:Wires
def draw_wires(
     height=(SCALAR,0,1,0),
     top_highlight=(SCALAR,0,1,0),
     bottom_highlight=(SCALAR,0,1,0),
     top_alpha=(SCALAR,0,1,1),
     bottom_alpha=(SCALAR,0,1,1),
     outer_alpha=(SCALAR,0,1,1),
     center_rectangle_alpha=(SCALAR,0,1,0),
     center_site_alpha=(SCALAR,0,1,1),
     middle_alpha=(SCALAR,0,1,0),
     horribly_wrong_alpha=(SCALAR,0,1,0),
     window=(SCALAR,1,100,1),
     dot_slider=(SCALAR,0,1,1),
     dot_alpha=(SCALAR,0,1,0),
     upper_connection_length=(SCALAR,0,1,0),
     lower_connection_length=(SCALAR,0,1,0),
     upper_connection_color=(COLOR,red),
     lower_connection_color=(COLOR,red),
     upper_input_port_signal_color=(COLOR,Color(red,0)),
     lower_input_port_signal_color=(COLOR,Color(red,0)),
     upper_output_port_signal_color=(COLOR,Color(red,0)),
     lower_output_port_signal_color=(COLOR,Color(red,0)),
     upper_input_port_signal_radius=(SCALAR,0,0.3,0),
     lower_input_port_signal_radius=(SCALAR,0,0.3,0),
     upper_output_port_signal_radius=(SCALAR,0,0.3,0.15),
     lower_output_port_signal_radius=(SCALAR,0,0.3,0.15),
     everything_else_alpha=(SCALAR,0,1,0),
     ):
    set_camera(Rect(0,0,width=5*window,height=5*(3/4)*window))

    #@    @+others
    #@+node:Draw top wire
    push()
    translate(-2,height)
    for i in xrange(5):

        push()

        scale(0.75,0.75)

        push()
        color(black,(1-top_highlight)*top_alpha*(outer_alpha if i != 2 else center_site_alpha))
        translate(0.03*(1-top_highlight),-0.03*(1-top_highlight))
        widestroke(qubit,0.1)
        arrow(qubit,(0,0.2,0.2),(1,0.2,0.2))
        pop()

        scale(1+(3/2-1)*top_highlight)

        color(white,top_alpha*(outer_alpha if i != 2 else center_site_alpha))
        widestroke(qubit,0.1)
        arrow(qubit,(0,0.2,0.2),(1,0.2,0.2))

        pop()

        translate(1,0)
    pop()
    #@-node:Draw top wire
    #@+node:Draw bottom wire
    push()
    scale(1,-1)
    translate(-2,height)
    for i in xrange(5):

        push()

        scale(0.75,0.75)

        push()
        color(black,(1-bottom_highlight)*bottom_alpha*(outer_alpha if i != 2 else center_site_alpha))
        translate(0.03*(1-bottom_highlight),+0.03*(1-bottom_highlight))
        widestroke(qubit,0.1)
        arrow(qubit,(0,0.2,0.2),(1,0.2,0.2))
        pop()

        scale(1+(3/2-1)*bottom_highlight)

        color(white,bottom_alpha*(outer_alpha if i != 2 else center_site_alpha))
        widestroke(qubit,0.1)
        arrow(qubit,(0,0.2,0.2),(1,0.2,0.2))

        pop()

        translate(1,0)
    pop()
    #@-node:Draw bottom wire
    #@+node:Draw middle wire
    if middle_alpha > 0:
        configuration = [-1,1,1,-1,-1]

        push()
        translate(-2,0)
        for i in xrange(5):

            push()

            translate(0,configuration[i]*0.075)

            scale(0.75,configuration[i]*0.75)

            push()
            color(black,middle_alpha)
            translate(0.03,-configuration[i]*0.03)
            widestroke(qubit,0.1)
            arrow(qubit,(0,0.2,0.2),(1,0.2,0.2))
            pop()

            color(white,middle_alpha)
            widestroke(qubit,0.1)
            arrow(qubit,(0,0.2,0.2),(1,0.2,0.2))

            pop()

            translate(1,0)
        pop()
    #@-node:Draw middle wire
    #@+node:Draw box around center site
    color(white,center_rectangle_alpha)
    thickness(0.01)
    frame(-0.5,-1.75,0.5,1.75)
    #@-node:Draw box around center site
    #@+node:Draw X around middle wire
    if horribly_wrong_alpha > 0:
        color(red,horribly_wrong_alpha)
        thickness(0.1)
        line(-2.25,-0.6,2.25, 0.6)
        line(-2.25, 0.6,2.25,-0.6)
    #@-node:Draw X around middle wire
    #@+node:Draw long-scale wires
    if window > 1.15001:
        #@    @+others
        #@+node:Draw top wire
        push()
        translate(-250,height)
        for i in xrange(501):

            push()

            scale(0.75,0.75)

            push()
            color(black)
            translate(0.03,-0.03)
            widestroke(qubit,0.1)
            arrow(qubit,(0,0.2,0.2),(1,0.2,0.2))
            pop()

            color(white)
            widestroke(qubit,0.1)
            arrow(qubit,(0,0.2,0.2),(1,0.2,0.2))

            pop()

            translate(1,0)
        pop()
        #@-node:Draw top wire
        #@+node:Draw bottom wire
        push()
        scale(1,-1)
        translate(-250,height)
        for i in xrange(501):

            push()

            scale(0.75,0.75)

            push()
            color(black)
            translate(0.03,+0.03)
            widestroke(qubit,0.1)
            arrow(qubit,(0,0.2,0.2),(1,0.2,0.2))
            pop()

            color(white)
            widestroke(qubit,0.1)
            arrow(qubit,(0,0.2,0.2),(1,0.2,0.2))

            pop()

            translate(1,0)
        pop()
        #@-node:Draw bottom wire
        #@-others

    #@-node:Draw long-scale wires
    #@+node:Draw dot-dot-dots
    if dot_alpha > 0:
        #@    @+others
        #@+node:Draw connection arrows
        color(upper_connection_color)
        rectangle(-2.5, 0.5-0.05,-2.5+upper_connection_length*5, 0.5+0.05)

        color(lower_connection_color)
        rectangle(-2.5,-0.5-0.05,-2.5+lower_connection_length*5,-0.5+0.05)

        color(grey,everything_else_alpha)
        rectangle(-3, 0.5-0.05,4, 0.5+0.05)
        rectangle(-3,-0.5-0.05,4,-0.5+0.05)
        #@-node:Draw connection arrows
        #@+node:Draw dots
        push()

        def draw_dots(signal_color,signal_radius):
            color(white.interp(grey,(1-dot_slider)*0.5),dot_alpha+(1-dot_slider)*0.5)
            dot(0.05+(1-dot_slider)*0.05,-0.2*dot_slider,0.7*dot_slider)
            dot(0.05+(1-dot_slider)*0.05, 0          ,0.7*dot_slider)
            dot(0.05+(1-dot_slider)*0.05, 0.2*dot_slider,0.7*dot_slider)
            color(black,(1-dot_slider)*dot_alpha)
            circle(0.05+(1-dot_slider)*0.03,-0.2*dot_slider,0.7*dot_slider)
            circle(0.05+(1-dot_slider)*0.03, 0             ,0.7*dot_slider)
            circle(0.05+(1-dot_slider)*0.03, 0.2*dot_slider,0.7*dot_slider)
            color(signal_color)
            dot(signal_radius,0,0)

        thickness(0.01)
        translate(-2.5,0.5)
        draw_dots(upper_input_port_signal_color,upper_input_port_signal_radius)

        thickness(0.01)
        translate(5,0)
        draw_dots(upper_output_port_signal_color,upper_output_port_signal_radius)

        scale(1,-1)

        thickness(0.01)
        translate(-5,1)
        draw_dots(lower_input_port_signal_color,lower_input_port_signal_radius)

        thickness(0.01)
        translate(5,0)
        draw_dots(lower_output_port_signal_color,lower_output_port_signal_radius)

        pop()
        #@-node:Draw dots
        #@+node:Draw port labels
        color(yellow,(1-dot_slider)*(1-everything_else_alpha))
        text(-2.5,0,"INPUT\nPORTS",fonts["roman"],size=0.2,justify=0.5)
        text( 2.5,0,"OUTPUT\nPORTS",fonts["roman"],size=0.2,justify=0.5)
        #@-node:Draw port labels
        #@-others
    #@nonl
    #@-node:Draw dot-dot-dots
    #@+node:Draw remaining connections
    if everything_else_alpha > 0:
        i = -1.5
        while i <= 1.5:
            color(white.interp(grey,0.5),everything_else_alpha)
            dot(0.1,i,0.5)
            dot(0.1,i,-0.5)
            color(black,everything_else_alpha)
            circle(0.08,i,0.5)
            circle(0.08,i,-0.5)
            i += 1

    #@-node:Draw remaining connections
    #@-others
#@-node:Wires
#@+node:W state
def draw_W_state(
     highlights1=(OBJECT,[]),
     highlight_slider1=(SCALAR,0,1,0),
     highlights2=(OBJECT,[]),
     highlight_slider2=(SCALAR,0,1,0),
     rectangle_alpha=(SCALAR,0,1,0)
    ):
    set_camera(Rect(0,0,width=5,height=5))

    color(white,rectangle_alpha)
    thickness(0.01)
    frame(0,-2,1,2)

    push()

    translate(-1.5,2.5)
    for j in xrange(4):

        translate(0,-1)

        color(white,0.5)
        dot(0.03,-0.375-0.5,0)
        dot(0.03,-0.500-0.5,0)
        dot(0.03,-0.625-0.5,0)

        push()
        for i in xrange(4):

            if (i,j) in highlights1:
                highlight_slider = highlight_slider1
                highlighted = True
            elif (i,j) in highlights2:
                highlight_slider = highlight_slider2
                highlighted = True
            else:
                highlight_slider = 0
                highlighted = False

            push()

            scale_factor = 0.5+(0.125*highlight_slider if highlighted else 0)

            if i == j:
                scale(scale_factor, scale_factor)
            else:
                scale(scale_factor,-scale_factor)

            push()
            color(black,(1-highlight_slider) if highlighted else 1)

            offset = 0.075*((1-highlight_slider) if highlighted else 1)

            if i == j:
                translate(offset,-offset)
            else:
                translate(offset, offset)

            widestroke(qubit,0.1)
            arrow(qubit,(0,0.2,0.2),(1,0.2,0.2))
            pop()

            color(white.interp(yellow,0.5*highlight_slider))
            widestroke(qubit,0.1)
            arrow(qubit,(0,0.2,0.2),(1,0.2,0.2))

            pop()

            translate(1,0)

        translate(-1,0)

        color(white,0.5)
        dot(0.03,0.375+0.5,0)
        dot(0.03,0.500+0.5,0)
        dot(0.03,0.625+0.5,0)

        pop()

    pop()

    color(white,0.5)
    dot(0.03,0,1.875)
    dot(0.03,0,2)
    dot(0.03,0,2.125)
    dot(0.03,0,-1.875)
    dot(0.03,0,-2)
    dot(0.03,0,-2.125)
#@-node:W state
#@+node:W state wirings
def draw_W_state_wirings(
        window=(SCALAR,0,4.5,1.75),
        outer_sites_alpha=(SCALAR,0,1,0),
        center_matrix_alpha=(SCALAR,0,1,0),
        center_label_alpha=(SCALAR,0,1,0),
        equation_alpha=(SCALAR,0,1,0),
        top_qubit_alpha=(SCALAR,0,1,0),
        top_qubit_highlight=(SCALAR,0,1,0),
        top_connection_length=(SCALAR,0,1,0),
        top_connection_color=(COLOR,red),
        middle_qubit_alpha=(SCALAR,0,1,0),
        middle_qubit_highlight=(SCALAR,0,1,0),
        middle_connection_length=(SCALAR,0,1,0),
        middle_connection_color=(COLOR,red),
        bottom_qubit_alpha=(SCALAR,0,1,0),
        bottom_qubit_highlight=(SCALAR,0,1,0),
        bottom_connection_length=(SCALAR,0,1,0),
        bottom_connection_color=(COLOR,red),
        center_matrix_element_alpha_00=(SCALAR,0,1,0),
        center_matrix_element_highlight_00=(SCALAR,0,1,0),
        center_matrix_element_alpha_01=(SCALAR,0,1,0),
        center_matrix_element_highlight_01=(SCALAR,0,1,0),
        center_matrix_element_alpha_10=(SCALAR,0,1,0),
        center_matrix_element_highlight_10=(SCALAR,0,1,0),
        center_matrix_element_alpha_11=(SCALAR,0,1,0),
        center_matrix_element_highlight_11=(SCALAR,0,1,0),
        signal_color_00=(COLOR,Color(red,0)),
        signal_radius_00=(SCALAR,0,0.3,0.03),
        signal_color_01=(COLOR,Color(red,0)),
        signal_radius_01=(SCALAR,0,0.3,0.03),
        signal_color_10=(COLOR,Color(red,0)),
        signal_radius_10=(SCALAR,0,0.3,0.15),
        signal_color_11=(COLOR,Color(red,0)),
        signal_radius_11=(SCALAR,0,0.3,0.15),
        row_1_highlight=(SCALAR,0,1,0),
        row_2_highlight=(SCALAR,0,1,0),
        column_1_highlight=(SCALAR,0,1,0),
        column_2_highlight=(SCALAR,0,1,0),
        matrix_index_labels_alpha=(SCALAR,0,1,0),
        arrow_alpha=(SCALAR,0,1,0),
        upper_index_highlight=(SCALAR,0,1,0),
        lower_index_highlight=(SCALAR,0,1,0),
        vector_alpha=(SCALAR,0,1,0),
        operator_slider=(SCALAR,0,1,0),
        operator_chain_alpha=(SCALAR,0,1,0),
        label_slider=(SCALAR,0,1,0),
        outer_label_alpha=(SCALAR,0,1,0),
        connection_index_label_alpha=(SCALAR,0,1,0),
        diagram_alpha=(SCALAR,0,1,1),
        environment_alpha=(SCALAR,0,1,0),
        draw_matrices=(BOOLEAN,True),
        center_circle_color=(COLOR,initial_five_colors[2]),
        center_circle_alpha=(SCALAR,0,1,1),
        center_circle_x=(SCALAR,-3,3,0),
        center_circle_y=(SCALAR,-1,1,1),
        operator_label_alpha=(SCALAR,0,1,1),
        uniform_operator_label_alpha=(SCALAR,0,1,0),
        state_chain_alpha=(SCALAR,0,1,0),
        state_chain_height=(SCALAR,0,1.375,1.375),
        left_environmonster_position=(SCALAR,-5,0,-5),
        left_environmonster_jaw_angle=(SCALAR,0,90,45),
        left_environmonster_alpha=(SCALAR,0,1,1),
        right_environmonster_position=(SCALAR,0,5,5),
        right_environmonster_jaw_angle=(SCALAR,0,90,45),
        right_environmonster_alpha=(SCALAR,0,1,1),
        left_environment_alpha=(SCALAR,0,1,0),
        right_environment_alpha=(SCALAR,0,1,0),
        left_environment_color=(COLOR,hsv(96/256,96/256,128/256)),
        right_environment_color=(COLOR,hsv(224/256,96/256,128/256)),
        environment_absorbtion_slider=(SCALAR,0,1,0),
        outer_stuff_visible=(BOOLEAN,True),
        index_merge_slider=(SCALAR,0,1,0),
        ):
    set_camera(Rect(0,-0.2,width=(4/3)*(window-0.2),height=window-0.2,anchor="n"))
    #@    @+others
    #@+node:Utility functions
    #@+node:def draw_qubit
    qubit_alpha = 1-operator_slider

    def draw_qubit(state,alpha=1,scale_factor=1,highlight=0):
        push()

        scale_factor *= (1+0.5*highlight)

        scale(0.5*scale_factor,state*0.375*scale_factor)

        push()
        color(black,(1-highlight)*alpha*qubit_alpha)
        translate(0.03*(1-highlight),-state*0.03*(1-highlight))
        widestroke(qubit,0.1)
        arrow(qubit,(0,0.2,0.2),(1,0.2,0.2))
        pop()

        color(white,alpha*qubit_alpha)
        widestroke(qubit,0.1)
        arrow(qubit,(0,0.2,0.2),(1,0.2,0.2))

        pop()

        translate(1,0)
    #@-node:def draw_qubit
    #@-node:Utility functions
    #@+node:Draw diagram
    scale(1,-1)

    if diagram_alpha > 0:

        #@    @+others
        #@+node:Draw connections
        #@+node:Draw center connections
        color(top_connection_color,diagram_alpha)
        rectangle(-0.5,0.5-0.05,-0.5+top_connection_length,0.5+0.05)
        color(bottom_connection_color,diagram_alpha)
        rectangle(-0.5,1.5-0.05,-0.5+bottom_connection_length,1.5+0.05)

        push()
        color(middle_connection_color,diagram_alpha)
        translate(-0.5,0.5)
        rotate(45)
        rectangle(0,-0.05,sqrt(2)*middle_connection_length,0.05)
        pop()

        #@-node:Draw center connections
        #@+node:Draw outer connections
        if outer_sites_alpha > 0:

            connection_color = Color(grey,outer_sites_alpha*diagram_alpha)

            color(connection_color)
            rectangle(-2,0.5-0.05,2,0.5+0.05)
            rectangle(-2,1.5-0.05,2,1.5+0.05)

            for x in [-1.5,0.5]:
                push()
                translate(x,0.5)
                rotate(45)
                rectangle(0,-0.05,sqrt(2),0.05)
                pop()

            faded_rectangle(connection_color,-2,-2.25,0.5,0.1)
            faded_rectangle(connection_color,-2,-2.25,1.5,0.1)
            faded_rectangle(connection_color, 2, 2.25,0.5,0.1)
            faded_rectangle(connection_color, 2, 2.25,1.5,0.1)
        #@+at
        #     i = 0.0125
        #     while i <= 0.25:
        #         color(grey,(1-4*i)*outer_sites_alpha)
        #         rectangle(-2-i,0.5-0.05,-2-i+0.0125,0.5+0.05)
        #         rectangle(-2-i,1.5-0.05,-2-i+0.0125,1.5+0.05)
        #         rectangle( 2+i-0.0125,0.5-0.05,2+i,0.5+0.05)
        #         rectangle( 2+i-0.0125,1.5-0.05,2+i,1.5+0.05)
        #         i += 0.0125
        #@-at
        #@@c
        #@-node:Draw outer connections
        #@-node:Draw connections
        #@+node:Draw ports
        #@+node:Draw center ports
        thickness(0.01)

        signal_colors = [[signal_color_00,signal_color_01,],[signal_color_10,signal_color_11]]

        signal_radii = [[signal_radius_00,signal_radius_01,],[signal_radius_10,signal_radius_11]]


        for i,x in enumerate([-0.5,0.5]):
            for j,y in enumerate([0.5,1.5]):
                color(white.interp(grey,0.5),diagram_alpha)
                dot(0.1,x,y)
                color(black,diagram_alpha)
                circle(0.08,x,y)

                color(signal_colors[i][j])
                dot(signal_radii[i][j],x,y)

        #@-node:Draw center ports
        #@+node:Draw outer ports
        if outer_sites_alpha > 0:

            for x in [-1.5,1.5]:
                for y in [0.5,1.5]:
                    color(white.interp(grey,0.5),outer_sites_alpha*diagram_alpha)
                    dot(0.1,x,y)
                    color(black,outer_sites_alpha*diagram_alpha)
                    circle(0.08,x,y)

        #@-node:Draw outer ports
        #@-node:Draw ports
        #@+node:Draw qubits
        qubit_alpha = 1-operator_slider
        #@nonl
        #@+node:Draw center qubits
        push()
        translate(0,0.4)
        draw_qubit(+1,alpha=top_qubit_alpha*diagram_alpha,highlight=top_qubit_highlight)
        pop()

        push()
        translate(0,1.4)
        draw_qubit(+1,alpha=bottom_qubit_alpha*diagram_alpha,highlight=bottom_qubit_highlight)
        pop()


        push()
        translate(0,1.1)
        draw_qubit(-1,alpha=middle_qubit_alpha*diagram_alpha,highlight=middle_qubit_highlight)
        pop()
        #@-node:Draw center qubits
        #@+node:Draw outside qubits
        if outer_sites_alpha > 0:

            push()
            translate(-1,0.4)
            draw_qubit(+1,alpha=outer_sites_alpha*diagram_alpha)
            pop()

            push()
            translate(-1,1.4)
            draw_qubit(+1,alpha=outer_sites_alpha*diagram_alpha)
            pop()


            push()
            translate(-1,1.1)
            draw_qubit(-1,alpha=outer_sites_alpha*diagram_alpha)
            pop()


            push()
            translate(+1,0.4)
            draw_qubit(+1,alpha=outer_sites_alpha*diagram_alpha)
            pop()

            push()
            translate(+1,1.4)
            draw_qubit(+1,alpha=outer_sites_alpha*diagram_alpha)
            pop()


            push()
            translate(+1,1.1)
            draw_qubit(-1,alpha=outer_sites_alpha*diagram_alpha)
            pop()
        #@-node:Draw outside qubits
        #@-node:Draw qubits
        #@-others

    scale(1,-1)

    #@-node:Draw diagram
    #@+node:Draw matrices
    scale(1,-1)

    if draw_matrices:
        #@    @+others
        #@+node:def draw_matrix
        def draw_matrix(
                alpha=1,
                alpha_00=1,highlight_00=0,
                alpha_01=1,highlight_01=0,
                alpha_10=1,highlight_10=0,
                alpha_11=1,highlight_11=0,
            ):
            color(white,alpha)
            thickness(0.01)
            line(-0.125,2,-0.375,2,-0.375,2.75,-0.125,2.75)
            line(+0.125,2,+0.375,2,+0.375,2.75,+0.125,2.75)

            push()
            translate(-0.175,2.15)
            draw_qubit(+1,alpha_00,highlight=highlight_00,scale_factor=0.5)
            pop()

            push()
            translate(+0.175,2.50)
            draw_qubit(+1,alpha_11,highlight=highlight_11,scale_factor=0.5)
            pop()

            push()
            translate(+0.175,2.25)
            draw_qubit(-1,alpha_10,highlight=highlight_10,scale_factor=0.5)
            pop()

            color(white,alpha_01)
            text(-0.175,2.55,"0",fonts["roman"],size=0.3+0.1*highlight_01,justify=0.25)

            push()
            scale(1,-1)

            push()
            translate(0.02,-0.02)
            color(black,operator_slider)
            text(+0.175,-2.20,"Z",fonts["bold"],size=0.3,justify=0.25)
            text(+0.175,-2.55,"I",fonts["bold"],size=0.3,justify=0.25)
            text(-0.175,-2.20,"I",fonts["bold"],size=0.3,justify=0.25)
            pop()

            color(white,operator_slider)
            text(+0.175,-2.20,"Z",fonts["bold"],size=0.3,justify=0.25)
            text(+0.175,-2.55,"I",fonts["bold"],size=0.3,justify=0.25)
            text(-0.175,-2.20,"I",fonts["bold"],size=0.3,justify=0.25)
            pop()


        #@-node:def draw_matrix
        #@+node:Draw center matrix
        draw_matrix(
            center_matrix_alpha,
            center_matrix_element_alpha_00,
            center_matrix_element_highlight_00,
            center_matrix_element_alpha_01,
            center_matrix_element_highlight_01,
            center_matrix_element_alpha_10,
            center_matrix_element_highlight_10,
            center_matrix_element_alpha_11,
            center_matrix_element_highlight_11,
        )
        #@-node:Draw center matrix
        #@+node:Draw outer matrices
        if outer_sites_alpha > 0:
            push()
            translate(-1,0)
            draw_matrix(alpha=outer_sites_alpha)
            translate(+2,0)
            draw_matrix(alpha=outer_sites_alpha)
            pop()

            color(white,1*outer_sites_alpha)
            dot(0.02,-0.5,2.375)
            dot(0.02,+0.5,2.375)

            color(white,0.8*outer_sites_alpha)
            dot(0.02,-1.5,2.375)
            dot(0.02,+1.5,2.375)

            color(white,0.6*outer_sites_alpha)
            dot(0.02,-1.6,2.375)
            dot(0.02,+1.6,2.375)

            color(white,0.4*outer_sites_alpha)
            dot(0.02,-1.7,2.375)
            dot(0.02,+1.7,2.375)
        #@-node:Draw outer matrices
        #@-others

    scale(1,-1)
    #@-node:Draw matrices
    #@+node:Draw hints to connect matrix with diagram
    #@+node:Draw index labels
    color(yellow,matrix_index_labels_alpha)
    text(-0.9,-0.5,"1",fonts["big"],size=0.3+0.1*row_1_highlight)
    text(-0.9,-1.5,"2",fonts["big"],size=0.3+0.1*row_2_highlight)
    text(-0.5,-2.15,"1",fonts["big"],size=0.2+0.05*row_1_highlight,anchor="cr")
    text(-0.5,-2.55,"2",fonts["big"],size=0.2+0.05*row_2_highlight,anchor="cr")

    color(0,1,1,matrix_index_labels_alpha)
    text(+0.9,-0.5,"1",fonts["big"],size=0.3+0.1*column_1_highlight)
    text(+0.9,-1.5,"2",fonts["big"],size=0.3+0.1*column_2_highlight)
    text(-0.175,-1.8,"1",fonts["big"],size=0.2+0.05*column_1_highlight)
    text(+0.175,-1.8,"2",fonts["big"],size=0.2+0.05*column_2_highlight)
    #@-node:Draw index labels
    #@+node:Draw arrows
    if arrow_alpha > 0:
        def draw_arrow(alpha=1):
            color(yellow,alpha)
            thickness(0.05)
            line(0,-0.375,0,+0.375)
            polygon(-0.1, 0.375,0, 0.5,+0.1, 0.375)
            polygon(-0.1,-0.375,0,-0.5,+0.1,-0.375)


        push()
        translate(-0.5,-2.375)
        scale(0.75,0.75)
        draw_arrow(arrow_alpha)
        pop()


        push()
        translate(0,-1.85)
        scale(0.75,0.75)
        rotate(90)
        draw_arrow(arrow_alpha)
        pop()

    #@-node:Draw arrows
    #@+node:Draw vectors
    if vector_alpha > 0:
        embed_object(Rect(0,-0.5,width=0.125,height=0.25),draw_vector,{},_alpha=vector_alpha)
        embed_object(Rect(0,-1.0,width=0.125,height=0.25),draw_vector,{"bottom":"1","top":"0"},_alpha=vector_alpha)
        embed_object(Rect(0,-1.5,width=0.125,height=0.25),draw_vector,{},_alpha=vector_alpha)

        embed_object(Rect(-0.175,-2.20,width=0.125,height=0.25),draw_vector,{},_alpha=vector_alpha) 
        embed_object(Rect(-0.175,-2.55,width=0.125,height=0.25),draw_vector,{"bottom":"0","top":"0"},_alpha=vector_alpha)    
        embed_object(Rect(+0.175,-2.20,width=0.125,height=0.25),draw_vector,{"bottom":"1","top":"0"},_alpha=vector_alpha) 
        embed_object(Rect(+0.175,-2.55,width=0.125,height=0.25),draw_vector,{},_alpha=vector_alpha)
    #@-node:Draw vectors
    #@-node:Draw hints to connect matrix with diagram
    #@+node:Draw operators
    if operator_slider > 0:

        push()
        color(black,operator_slider*diagram_alpha)
        translate(0.02,-0.02)
        for i in xrange(-1,2,1):
            text(i,-0.5,"I",fonts["bold"],size=0.3)
            text(i,-1.0,"Z",fonts["bold"],size=0.3)
            text(i,-1.5,"I",fonts["bold"],size=0.3)
        pop()

        color(white,operator_slider*diagram_alpha)
        for i in xrange(-1,2,1):
            text(i,-0.5,"I",fonts["bold"],size=0.3)
            text(i,-1.0,"Z",fonts["bold"],size=0.3)
            text(i,-1.5,"I",fonts["bold"],size=0.3)


    #@-node:Draw operators
    #@+node:Draw equation
    if equation_alpha > 0:
        image(0,-4,mps_equation,height=0.8,alpha=equation_alpha*(1-operator_slider))

        if operator_slider > 0:
            image(0,-4,mpo_equation,height=0.7,alpha=equation_alpha*operator_slider)

    #@-node:Draw equation
    #@+node:Draw operator chain
    color(white,operator_chain_alpha)
    thickness(0.1)

    if outer_stuff_visible:
        line(-2,-2.375,2,-2.375)

        faded_rectangle(Color(white,operator_chain_alpha),-2.375,-2.375-0.25,-2.375,0.1)
        faded_rectangle(Color(white,operator_chain_alpha),+2.375,+2.375+0.25,-2.375,0.1)
    else:
        line(-1.625*(1-environment_absorbtion_slider),-2.375,+1.625*(1-environment_absorbtion_slider),-2.375)

    def draw_line_with_cave(x,y,thickness,height):
        push()
        translate(x,y)
        scale(thickness/2,height/2)
        polygon(0,0,-1,0,-1,1,0,2/3,1,1,1,0)
        pop()

    border_color = Color(white,operator_chain_alpha)
    fill_color = Color(grey,operator_chain_alpha)
    thickness(0.01)

    if outer_stuff_visible:
        for x in xrange(-2,3):
            if x == 0:
                continue
            draw_centered_rectangle(x,-2.375,.75,.75,border_color,fill_color)
            draw_line_with_cave(x,-2,0.1,0.25)
            draw_line_with_cave(x,-2.75,0.1,-0.25)

    draw_centered_rectangle(center_circle_x,-2.375,.75,.75,Color(border_color,operator_chain_alpha*center_circle_alpha),Color(fill_color,operator_chain_alpha*center_circle_alpha))
    draw_line_with_cave(center_circle_x,-2,0.1,0.25)
    draw_line_with_cave(center_circle_x,-2.75,0.1,-0.25)
    #@-node:Draw operator chain
    #@+node:Draw matrix labels
    #@+node:def draw_matrix_label
    cyan = Color(0,1,1)
    dark_blue = Color(0,0,0.5)

    def draw_matrix_label(label,upper_index_label,lower_index_label,alpha=1,upper_index_highlight=0,lower_index_highlight=0,c1=cyan,c2=dark_blue):
        color(c1.interp(c2,label_slider),alpha)
        text(0,0,label,font=fonts["big"],size=0.5,anchor="cr")
        text(0.05,0.05,upper_index_label,font=fonts["big"],size=0.25+0.125*upper_index_highlight,anchor="bl")
        text(0.05,-0.05,lower_index_label,font=fonts["big"],size=0.25+0.125*lower_index_highlight,anchor="tl")
    #@-node:def draw_matrix_label
    #@+node:Draw labels
    push()

    if center_label_alpha > 0:

        translate(0,-3.125+0.75*label_slider)
        draw_matrix_label("B","b","jk",
            center_label_alpha*(1-operator_slider),
            upper_index_highlight=upper_index_highlight,
            lower_index_highlight=lower_index_highlight
            )

        if outer_sites_alpha > 0:

            translate(-1,0)
            draw_matrix_label("A","a","ij",outer_sites_alpha*(1-operator_slider))

            translate(2,0)
            draw_matrix_label("C","c","kl",outer_sites_alpha*(1-operator_slider))

        if operator_slider > 0:

            translate(-2,0)
            draw_matrix_label("A","aa'","ij",outer_sites_alpha*operator_slider*operator_label_alpha)

            translate(1,0)
            draw_matrix_label("B","bb'","jk",outer_sites_alpha*operator_slider*operator_label_alpha)

            translate(1,0)
            draw_matrix_label("C","cc'","kl",outer_sites_alpha*operator_slider*operator_label_alpha)

        if outer_label_alpha > 0:
            translate(-3,0.75*(1-label_slider))
            draw_matrix_label("Z","zz'","hi",outer_label_alpha*operator_label_alpha)

            translate(+4,0)
            draw_matrix_label("D","dd'","lm",outer_label_alpha*operator_label_alpha)

            translate(-2,0)

        if uniform_operator_label_alpha > 0:

            color(dark_blue,uniform_operator_label_alpha)

            if outer_stuff_visible:
                for x in xrange(-2,3):
                    if x == 0:
                        continue
                    else:
                        text(x,0,"H",font=fonts["big"],size=0.5)

            color(dark_blue,uniform_operator_label_alpha*center_circle_alpha)
            text(center_circle_x,0,"H",font=fonts["big"],size=0.5)

    pop()

    #@-node:Draw labels
    #@+node:Draw connection index labels
    def draw_connection_labels():
        for x, index in [
             (-1.5,"i"),
             (-0.5,"j"),
             (+0.5,"k"),
             (+1.5,"l"),
             ]:

            text(x,0,index,fonts["big"],anchor="cc",size=0.3)

        for x, index in [
             (-2,"z"),
             (-1,"a"),
             (-0,"b"),
             (+1,"c"),
             (+2,"d"),
             ]:

            text(x, 0.625,index,fonts["big"],anchor="cc",size=0.3)
            text(x,-0.625,index+"'",fonts["big"],anchor="cc",size=0.3)


    draw_with_shadows(0,-2.375,0.015,draw_connection_labels,Color(0.25,0.5,1),connection_index_label_alpha)
    #@-node:Draw connection index labels
    #@-node:Draw matrix labels
    #@+node:Draw environment
    if environment_alpha > 0:

        color(center_circle_color, environment_alpha)
        dot(0.375,center_circle_x,center_circle_y)

        border_color = Color(white,environment_alpha)
        fill_color = Color(grey,environment_alpha)

        thickness(0.01)
        draw_centered_rectangle(-1.5,-1,2,0.75,border_color,fill_color)
        draw_centered_rectangle(+1.5,-1,2,0.75,border_color,fill_color)

    #@+at
    #     color(grey, environment_alpha)
    #     rectangle(-2.5,-1.5,-0.5,-0.5)
    #     rectangle(+0.5,-1.5,+2.5,-0.5)
    # 
    #     color(white, environment_alpha)
    #     thickness(0.01)
    #     frame(-2.5,-1.5,-0.5,-0.5)
    #     frame(+0.5,-1.5,+2.5,-0.5)
    #@-at
    #@@c

        def draw_environment_labels():
            t = text(-1.5,0,"Left",fonts["big"],size=0.5,anchor="cc")
            text(+1.5,t['bottom'],"Right",fonts["big"],size=0.5,anchor="lc")

        draw_with_shadows(0,-1,0.025,draw_environment_labels,white,environment_alpha)
    #@-node:Draw environment
    #@+node:Draw state chains
    def draw_line_with_protrusion(x,y,thickness,height):
        push()
        translate(x,y)
        scale(thickness/2,height/2)
        polygon(0,0,-1,0,-1,1,0,1+1/3,1,1,1,0)
        pop()

    if state_chain_alpha > 0:

        push()

        translate(0,-2.375)

        connection_color = Color(white,state_chain_alpha)

        if outer_stuff_visible:

            for y in [+state_chain_height,-state_chain_height]:
                push()
                translate(0,y)
                color(connection_color)
                thickness(0.1)
                line(-2,0,2,0)
                faded_rectangle(connection_color,-2.375,-2.375-0.25,0,0.1)
                faded_rectangle(connection_color,+2.375,+2.375+0.25,0,0.1)
                pop()

        else:
            offset = 0.25*(1-index_merge_slider)+0.05*index_merge_slider
            x = 1.625*(1-environment_absorbtion_slider) + offset*environment_absorbtion_slider
            y = state_chain_height*(1-environment_absorbtion_slider)
            thickness(0.1)
            line(-offset,-state_chain_height,-x,-y)
            line(+offset,-state_chain_height,+x,-y)
            line(-offset,+state_chain_height,-x,+y)
            line(+offset,+state_chain_height,+x,+y)

        def draw_circle(direction,c):
            color(c,state_chain_alpha*center_circle_alpha)
            dot(0.375,0,0)
            color(dark_blue,state_chain_alpha*center_circle_alpha)
            if direction == -1:
                text(0,0,"S",fonts["big"],anchor="cc",size=0.5)
            else:
                text(0,0,"S*",fonts["big"],anchor="cc",size=0.5)
            color(white,state_chain_alpha*center_circle_alpha)
            circle(0.375,0,0)
            draw_line_with_protrusion(0,direction*0.375,0.1,direction*0.25)

        thickness(0.01)

        if outer_stuff_visible:
            for x in xrange(-2,3):
                if x == 0:
                    continue

                c = initial_five_colors[x+2]

                push()
                translate(x,+state_chain_height)
                draw_circle(-1,c)
                pop()

                push()
                translate(x,-state_chain_height)
                draw_circle(+1,c)
                pop()

        push()
        translate(center_circle_x,+state_chain_height)
        draw_circle(-1,center_circle_color)
        pop()

        push()
        translate(center_circle_x,-state_chain_height)
        draw_circle(+1,center_circle_color)
        pop()

        pop()

    #@-node:Draw state chains
    #@+node:Draw environmonster
    monster_width = 2
    monster_height = 3

    number_of_teeth = 4
    tooth_width = monster_width/(number_of_teeth+1)
    tooth_height = tooth_width*1.25

    dx = tooth_width/2
    x = tooth_width

    tooth_points = [0,0,dx,0]

    for i in xrange(number_of_teeth):
        tooth_points.extend([x,tooth_height])
        x += dx
        tooth_points.extend([x,0])
        x += dx

    tooth_points.extend([x,0])

    upper_jaw_points = [monster_width,monster_height/2,0,monster_height/2]
    lower_jaw_points = [monster_width,-monster_height/2,0,-monster_height/2]

    def draw_environmonster(x,jaw_angle,direction,alpha,color_):

        push()
        translate(x,-2.375)

        push()
        scale(direction,1)

        push()
        rotate(-jaw_angle)
        color(color_,alpha)
        polygon(*(tooth_points+lower_jaw_points))
        pop()

        push()
        rotate(jaw_angle)
        color(color_,alpha)
        polygon(*(tooth_points+upper_jaw_points))
        translate(monster_width*3/4,monster_height*1/4)
        rotate(-30)
        token = clip(Rect(-0.5,-0.5,0.5,0))
        push()
        scale(1,0.75)
        color(white,alpha)
        dot(0.5)
        pop()

        push()
        translate(0.2,0)
        color(black,alpha)
        dot(0.2)
        pop()
        unclip(token)
        pop()

        pop()

        pop()

    draw_environmonster(left_environmonster_position,left_environmonster_jaw_angle,+1,left_environmonster_alpha,left_environment_color)
    draw_environmonster(right_environmonster_position,right_environmonster_jaw_angle,-1,right_environmonster_alpha,right_environment_color)

    #@-node:Draw environmonster
    #@+node:Draw environment
    draw_centered_rectangle(
        -1.625*(1-environment_absorbtion_slider),
        -2.375,
        monster_width*(1-environment_absorbtion_slider) + 0.75*environment_absorbtion_slider,
        monster_height*(1-environment_absorbtion_slider) + 0.75*environment_absorbtion_slider,
        Color(white,left_environment_alpha),
        Color(left_environment_color,left_environment_alpha)
        )

    color(dark_blue,left_environment_alpha)
    t = text(-1.625*(1-environment_absorbtion_slider),-2.375,"Left",fonts["big"],size=0.5*(1-environment_absorbtion_slider)+0.2*environment_absorbtion_slider,anchor="cc")

    draw_centered_rectangle(
        +1.625*(1-environment_absorbtion_slider),
        -2.375,
        monster_width*(1-environment_absorbtion_slider) + 0.75*environment_absorbtion_slider,
        monster_height*(1-environment_absorbtion_slider) + 0.75*environment_absorbtion_slider,
        Color(white,right_environment_alpha),
        Color(right_environment_color,right_environment_alpha)
        )

    color(dark_blue,right_environment_alpha)
    text(1.625*(1-environment_absorbtion_slider),t['bottom'],"Right",fonts["big"],size=0.5*(1-environment_absorbtion_slider)+0.2*environment_absorbtion_slider,anchor="lc")

    #@-node:Draw environment
    #@+node:Draw optimization matrix box
    if not outer_stuff_visible:
        draw_centered_rectangle(0,-2.375,0.75,0.75,white,grey)
        color(dark_blue,1-environment_absorbtion_slider)
        text(0,-2.375,"H",font=fonts["big"],size=0.5)
        color(dark_blue,environment_absorbtion_slider)
        text(0,-2.375,"Optimization\nMatrix",font=fonts["big"],size=0.125,anchor="cc",justify=0.5)
    #@-node:Draw optimization matrix box
    #@-others
#@-node:W state wirings
#@+node:Vector
def draw_vector(top=(STRING,"1"),bottom=(STRING,"0")):
    set_camera(Rect(0,0,width=1,height=2))
    color(white)
    thickness(0.1)
    line(-0.25,1,-0.5,1,-0.5,-1,-0.25,-1)
    line(+0.25,1,+0.5,1,+0.5,-1,+0.25,-1)
    text(0, 0.5,top,fonts["big"],size=0.75)
    text(0,-0.5,bottom,fonts["big"],size=0.75)
#@-node:Vector
#@+node:Thermometer
tang = asin(0.5/0.75)/pi*180
thermometer_path = Path().moveto(0.5,0).arc(0,3,0,180,0.5).arc(0,-3.5,90+tang,90-tang,0.75).closepath()

mark = Path().moveto(0,0).rqcurveto(0,-0.125,0.6,-0.125)

meniscus = Path().moveto(-0.35,0).rqcurveto(0,-0.1,0.35,-0.1).rqcurveto(0.35,0,0.35,0.1).rlineto(0,0.1).rlineto(-0.7,0).closepath()

def draw_thermometer(temperature=(SCALAR,0,1,1)):

    set_camera(Rect(0,0,height=8.5,width=1.5))

    color(white)
    fill(thermometer_path)

    color(red)
    dot(0.6,0,-3.5)
    rectangle(-0.35,-3.5,0.35,-2.95+5.95*temperature)

    push()
    translate(0,-2.95+5.95*temperature)
    color(white)
    fill(meniscus)
    pop()

    push()
    color(black)
    translate(-0.5,3)
    widestroke(mark,0.05)
    for i in xrange(4):
        translate(0,-1.25)
        widestroke(mark,0.05)
    pop()

    color(black)
    widestroke(thermometer_path,0.1)


def test_thermometer():

    thermometer = Drawable(get_camera().left(0.2).top(0.3),draw_thermometer)

    start_animation(bg,thermometer)

    pause()

    linear(3,thermometer.temperature,0)

    return end_animation()

test_thermometer = test_thermometer()
#@-node:Thermometer
#@+node:Qubit
def draw_qubit(
        angle=(SCALAR,0,360,0),
        qubit_scale=(SCALAR,1,2,1),
    ):
    set_camera(Rect(0,0,width=1.75,height=1.75))

    def draw_qubit():
        push()
        rotate(-angle)
        scale(qubit_scale,qubit_scale)
        widestroke(qubit,0.1)
        arrow(qubit,(0,0.2,0.2),(1,0.2,0.2))
        pop()

    draw_with_shadows(0,0,0.05,draw_qubit,white)
#@-node:Qubit
#@+node:Connector
def draw_connector(
        angle=(SCALAR,0,360,0),
        width=(SCALAR,0,1,0.5)
    ):
    set_camera(Rect(0,0,width=4,height=1.5))

    angle = angle % 360
    if angle > 180:
        angle = 360 - angle

    color(hsv(angle/180*(1/3),1,1))
    thickness(width)
    line(-1.5,0,1.5,0)

    color(port_fill_color)
    dot(0.5,-1.5,0)
    dot(0.5,+1.5,0)
    thickness(0.05)
    color(black)
    circle(0.4,-1.5,0)
    circle(0.4,+1.5,0)

#@-node:Connector
#@+node:Visualizer
#@+node:matrix drawing routines
separation = -0.3
width_scale = 0.1

def draw_lines_in_list(draw_list):
    draw_list.sort(reverse=True)
    for thickness_, color_, coordinates in draw_list:
        thickness(thickness_)
        color(color_)
        line(*coordinates)

def draw_matrix(matrix,alpha=1):
    draw_list = []

    for i in xrange(matrix.shape[1]):
        for j in xrange(matrix.shape[2]):
            UP, DOWN = abs(matrix[:,i,j])
            draw_list.append((
                (UP**2+DOWN**2)*width_scale,
                Color(UP,DOWN,0,alpha),
                (0,i*separation,1,j*separation),
            ))
    draw_lines_in_list(draw_list)

def draw_left_twisted_matrix(matrix,left_y_heights,alpha=1):
    draw_list = []

    for i in xrange(matrix.shape[1]):
        for j in xrange(matrix.shape[2]):
            UP, DOWN = abs(matrix[:,i,j])
            draw_list.append((
                (UP**2+DOWN**2)*width_scale,
                Color(UP,DOWN,0,alpha),
                (0,left_y_heights[i],1,j*separation),
            ))
    draw_lines_in_list(draw_list)

def draw_right_twisted_matrix(matrix,right_y_heights,alpha=1):
    draw_list = []

    for i in xrange(matrix.shape[1]):
        for j in xrange(matrix.shape[2]):
            UP, DOWN = abs(matrix[:,i,j])
            draw_list.append((
                (UP**2+DOWN**2)*width_scale,
                Color(UP,DOWN,0,alpha),
                (0,i*separation,1,right_y_heights[j]),
            ))
    draw_lines_in_list(draw_list)


#@-node:matrix drawing routines
#@+node:port drawing routines


radius = 0.075

def draw_port(x,y,alpha=1):
    color(port_fill_color,alpha)
    dot(radius,x,y)
    color(black,alpha)
    thickness(radius*.1)
    circle(radius*.8,x,y)

def draw_ports(x,y_list,alpha=1):
    for y in y_list:
        draw_port(x,y,alpha)

def draw_ports_automatically_separated(number_of_ports,alpha=1):
    draw_ports(0,arange(number_of_ports)*separation,alpha)
#@-node:port drawing routines
#@+node:draw_matrix_blend
def draw_matrix_blend(old_matrix,new_matrix,draw_twisted_matrix,heights,slider,alpha=1):
    draw_twisted_matrix(old_matrix,heights,slider*alpha)
    draw_matrix(new_matrix,alpha=(1-slider)*alpha)
#@-node:draw_matrix_blend
#@+node:draw_port_blend
def draw_port_blend(number_of_ports,heights,slider,alpha=1):
    if slider > 0:
        draw_ports(0,heights,slider*alpha)
        draw_ports_automatically_separated(number_of_ports,(1-slider)*alpha)
    else:
        draw_ports_automatically_separated(number_of_ports,alpha)
#@-node:draw_port_blend
#@+node:draw_visualizer
S100 = (SCALAR,0,1,0)

def draw_visualizer(
        cache=(OBJECT,None),
        old_slider=S100,
        old_shuffled_ports=(OBJECT,None),
        unnormalized_slider=S100,
        unnormalized_shuffled_ports=(OBJECT,None),
        left_shift=S100,
        right_shift=S100,
        break_left=(BOOLEAN,True),
        middle_matrix_index=(INTEGER,0,100000,0),
        left_matrix_indices=(OBJECT,[]),
        right_matrix_indices=(OBJECT,[]),
        middle_matrix_alpha=(SCALAR,0,1),
        middle_port_alpha=(SCALAR,0,1,1),
        height=(SCALAR,1,5,1)
        ):
    set_camera(Rect(0,0,width=5,height=5*(3/4)*height,anchor="n"))
    #@    @+others
    #@+node:Compute sliding heights
    if old_slider > 0:
        old_slider_twisted_heights = (arange(len(old_shuffled_ports))*old_slider + old_shuffled_ports*(1-old_slider))*separation

    if unnormalized_slider > 0:
        unnormalized_slider_twisted_heights = (arange(len(unnormalized_shuffled_ports))*unnormalized_slider + unnormalized_shuffled_ports*(1-unnormalized_slider))*separation


    #@-node:Compute sliding heights
    #@+node:fetch_and_draw_matrix_blend
    def fetch_and_draw_old_matrix_blend(index,draw_twisted_matrix):
        if old_slider > 0:
            old_matrix = cache.get_old_matrix(index)
            new_matrix = cache.get_matrix(index)
            draw_matrix_blend(old_matrix,new_matrix,draw_twisted_matrix,old_slider_twisted_heights,old_slider)
        else:
            draw_matrix(cache.get_matrix(index))

    def fetch_and_draw_unnormalized_matrix_blend(index,draw_twisted_matrix,alpha=1):
        if unnormalized_slider > 0:
            old_matrix = cache.get_unnormalized_matrix(index)
            new_matrix = cache.get_matrix(index)
            draw_matrix_blend(old_matrix,new_matrix,draw_twisted_matrix,unnormalized_slider_twisted_heights,unnormalized_slider,alpha)
        else:
            draw_matrix(cache.get_matrix(index),alpha)
    #@-node:fetch_and_draw_matrix_blend
    #@+node:Draw sites
    #@+at
    # Draw left sites
    #@-at
    #@@c

    push()
    translate(-1.5-left_shift,-0.5)
    first = True
    for index in left_matrix_indices:
        if first:
            fetch_and_draw_old_matrix_blend(index,draw_right_twisted_matrix)
            first = False
        else:
            draw_matrix(cache.get_matrix(index))
        translate(-1,0)
    pop()

    #@+at
    # Draw right sites
    #@-at
    #@@c

    push()
    translate(0.5+right_shift,-0.5)
    first = True
    for index in right_matrix_indices:
        if first:
            fetch_and_draw_old_matrix_blend(index,draw_left_twisted_matrix)
            first = False
        else:
            draw_matrix(cache.get_matrix(index))
        translate(+1,0)
    pop()

    #@+at
    # Draw middle
    #@-at
    #@@c

    if middle_matrix_alpha > 0:
        push()
        if break_left:
            translate(-0.5-left_shift,-0.5)
            fetch_and_draw_unnormalized_matrix_blend(middle_matrix_index,draw_right_twisted_matrix,middle_matrix_alpha)
        else:
            translate(-0.5+right_shift,-0.5)
            fetch_and_draw_unnormalized_matrix_blend(middle_matrix_index,draw_left_twisted_matrix,middle_matrix_alpha)
        pop()

    #@-node:Draw sites
    #@+node:Draw ports
    #@+at
    # Draw left ports
    #@-at
    #@@c

    push()
    translate(-1.5-left_shift,-0.5)
    for index in left_matrix_indices:
        number_of_ports = cache.get_matrix(index).shape[1]
        draw_ports_automatically_separated(number_of_ports)
        translate(-1,0)
    pop()

    if len(left_matrix_indices) > 0:
        number_of_ports = cache.get_matrix(left_matrix_indices[0]).shape[2]
        push()
        translate(-0.5-left_shift,-0.5)
        if old_slider > 0:
            draw_port_blend(number_of_ports,old_slider_twisted_heights,old_slider)
        else:
            draw_ports_automatically_separated(number_of_ports)
        pop()


    #@+at
    # Draw right ports
    #@-at
    #@@c

    push()
    translate(+1.5+right_shift,-0.5)
    for index in right_matrix_indices:
        number_of_ports = cache.get_matrix(index).shape[2]
        draw_ports_automatically_separated(number_of_ports)
        translate(+1,0)
    pop()

    if len(right_matrix_indices) > 0:
        number_of_ports = cache.get_matrix(right_matrix_indices[0]).shape[1]
        push()
        translate(+0.5+right_shift,-0.5)
        if old_slider > 0:
            draw_port_blend(number_of_ports,old_slider_twisted_heights,old_slider)
        else:
            draw_ports_automatically_separated(number_of_ports)
        pop()

    #@+at
    # Draw middle ports
    #@-at
    #@@c

    if middle_port_alpha > 0:
        push()
        if break_left:
            translate(-0.5-left_shift,-0.5)
            left_number_of_ports, right_number_of_ports = cache.get_matrix(middle_matrix_index).shape[1:]
            draw_ports_automatically_separated(left_number_of_ports,middle_port_alpha)
            translate(1,0)
            if unnormalized_slider > 0:
                draw_port_blend(right_number_of_ports,unnormalized_slider_twisted_heights,unnormalized_slider,middle_port_alpha)
            else:
                draw_ports_automatically_separated(right_number_of_ports,middle_port_alpha)
        else:
            translate(-0.5+right_shift,-0.5)
            left_number_of_ports, right_number_of_ports = cache.get_matrix(middle_matrix_index).shape[1:]
            if unnormalized_slider > 0:
                draw_port_blend(left_number_of_ports,unnormalized_slider_twisted_heights,unnormalized_slider,middle_port_alpha)
            else:
                draw_ports_automatically_separated(left_number_of_ports,middle_port_alpha)
            translate(1,0)
            draw_ports_automatically_separated(right_number_of_ports,middle_port_alpha)
        pop()

    #@-node:Draw ports
    #@-others
#@-node:draw_visualizer
#@-node:Visualizer
#@+node:Meter
def draw_meter(
        fetcher=(OBJECT,None),
        old_index=(INTEGER,0,10000,0),
        new_index=(INTEGER,0,10000,1),
        slider=(SCALAR,0,1,0),
        label=(STRING,""),
    ):
    set_camera(Rect(0,0,width=2,height=2,anchor="s"))

    if slider == 1:
        value = fetcher(new_index)
    elif slider == 0:
        value = fetcher(old_index)
    else:
        value = fetcher(old_index)*(1-slider) + fetcher(new_index)*slider

    color(white)
    rectangle(-0.45,1,0.45,1+value/10)

    color(yellow)
    text(0,0.65,"%.2f"%value,fonts["big"],anchor="sc",size=0.25)
    text(0,0.40,"Digits",fonts["big"],anchor="nc",size=0.20)
    text(0,0.20,label,fonts["big"],anchor="nc",size=0.20)
#@-node:Meter
#@-others

test_objects(draw_W_state_wirings, clear_color=blue)
#@-node:Objects
#@-others

#@-node:@file objects.py
#@-leo
