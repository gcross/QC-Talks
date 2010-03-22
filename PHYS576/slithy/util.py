import math
import types
from path import Path

def roundrect( x1, y1, x2, y2, r ):
    if x1 > x2:
        x1, x2 = x2, x1
    if y1 > y2:
        y1, y2 = y2, y1
        
    if (x2-x1) < 2 * r:
        r = (x2-x1) / 2.0
    if (y2-y1) < 2 * r:
        r = (y2-y1) / 2.0

    p = Path()
    p.arc( x1+r, y1+r, 180, 270, r )
    p.arc( x2-r, y1+r, 270, 360, r )
    p.arc( x2-r, y2-r, 0, 90, r )
    p.arc( x1+r, y2-r, 90, 180, r )
    p.closepath()

    return p

def interpolate( u, p1, p2 ):
    if type(p1) is types.ListType:
        return [(1.0-u)*x + u*y for x, y in zip(p1,p2)]
    elif type(p1) is types.TupleType:
        return tuple([(1.0-u)*x + u*y for x, y in zip(p1,p2)])
    else:
        return (1.0-u)*p1 + u*p2
    

def list_fraction( lyst, u ):
    frac, count = math.modf( u * len(lyst) )
    count = int(count)
    if count >= len(lyst):
        return lyst, None, None
    elif count < 0:
        return [], None, None
    else:
        return lyst[:count], lyst[count], frac
    
   
def split_sequence( n, u, v = 0.0 ):
    if u <= 0.0:
        return (0.0,) * n
    elif u >= 1.0:
        return (1.0,) * n
    
    l = 1.0 / ((n-1)*(1-v) + 1)
    f = l * (1-v)

    vec = []
    s = 0.0
    for i in range(n):
        if u < s:
            vec.append( 0.0 )
        elif u < s+l:
            vec.append( (u-s) / l )
        else:
            vec.append( 1.0 )
        s += f

    return tuple(vec)

def split_sequence_smooth( n, u, v = 0.0 ):
    if u <= 0.0:
        return (0.0,) * n
    elif u >= 1.0:
        return (1.0,) * n
    
    l = 1.0 / ((n-1)*(1-v) + 1)
    f = l * (1-v)

    vec = []
    s = 0.0
    for i in range(n):
        if u < s:
            vec.append( 0.0 )
        elif u < s+l:
            x = (u-s) / l
            vec.append( 3*x*x - 2*x*x*x )
        else:
            vec.append( 1.0 )
        s += f

    return tuple(vec)

def split_sequence_hat( n, u, v = 0.0 ):
    if u <= 0.0 or u >= 1.0:
        return (0.0,) * n
    
    l = 1.0 / ((n-1)*(1-v) + 1)
    f = l * (1-v)

    vec = []
    s = 0.0
    for i in range(n):
        if u < s:
            vec.append( 0.0 )
        elif u < s+l:
            vec.append( 1 - 2 * abs(0.5 - ((u-s) / l)) )
        else:
            vec.append( 0.0 )
        s += f

    return tuple(vec)

        
def interp_cameralist( c, cameras ):
    if c <= 0.0:
        return cameras[0]
    elif c >= len(cameras)-1:
        return cameras[-1]

    i = int(c)
    f = c - i
    return cameras[i].interp( cameras[i+1], f )
