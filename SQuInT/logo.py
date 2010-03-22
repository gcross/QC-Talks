from __future__ import division

from slithy.presentation import *
from slithy.library import *

from fonts import fonts

#def zero(width=(SCALAR,0,1)):
#    set_camera(Rect(0,0,width=2,height=2))
#    path = Path().moveto(0,1).curveto(-width,1,-width,-1,0,-1)
#    widestroke(path,0.1)

def unit_circle(circle_thickness=(SCALAR,0.05,2)):
    set_camera(Rect(0,0,width=2,height=2))
    thickness(circle_thickness)
    circle(1-circle_thickness/2)

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

    start_animation(bg,my_logo)

    pause()

    parallel()
    dt=1
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
    dt=0.6
    smooth(dt,my_logo.dot_radius,0.15,e=-1.5)

    serial()
    wait(dt/2)
    smooth(dt/2,my_logo.dot_color,Color(3/4,1/4,0,1),e=-1.5)
    end()
    
    end()

    wait(0.2)

    parallel()
    dt=0.3
    linear(dt,my_logo.dot_x,-0.25)
    linear(dt,my_logo.dot_y,-1)
    linear(dt,my_logo.line_4_x2,-0.25)
    linear(dt,my_logo.line_4_y2,-1)
    end()

    parallel()
    
    dt=0.6
    
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

    smooth(1,my_logo.bracket_position,2.5,e=-0.5)

    return end_animation()

animate_logo = animate_logo()

play(animate_logo)

run_presentation() 
