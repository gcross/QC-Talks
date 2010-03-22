# rectangle( x1, y1, x2, y2 )
# rectangle( x, y, pos = <>, width = <>, height = <> )

import math
strpos = { 'c' : (0.5, 0.5),
           'n' : (0.5, 1.0),
           's' : (0.5, 0.0),
           'w' : (0.0, 0.5),
           'e' : (1.0, 0.5),
           'sw' : (0.0, 0.0),
           'nw' : (0.0, 1.0),
           'ne' : (1.0, 1.0),
           'se' : (1.0, 0.0) }

def cossin( theta ):
    rad = theta * math.pi / 180.0
    return math.cos( rad ), math.sin( rad )

class Rect(tuple):
    def __new__( cls, *p, **kw ):
        pcount = len(p)
        kwcount = len(kw)

        if pcount == 5 and kwcount == 0:
            return tuple.__new__( cls, p )
        
        elif pcount == 1 and kwcount == 0 and isinstance( p[0], tuple ) and len(p[0]) == 5:
            return tuple.__new__( cls, p[0] )

        elif pcount == 1 and kwcount == 0 and isinstance( p[0], list ) and len(p[0]) == 5:
            return tuple.__new__( cls, p[0] )

        elif pcount == 4 and kwcount == 0:
            x1, y1, x2, y2 = p
            return tuple.__new__( cls, (x1, y1, x2-x1, 0.0, (x2-x1)/float(y2-y1)) )

        elif pcount == 2:
            x, y = p
            if 'width' in kw and 'height' in kw:
                anchor = kw.get( 'anchor', 'c' )
                if anchor in strpos:
                    anchor = strpos[anchor]
                w = kw['width']
                h = kw['height']
                x1 = x - w * anchor[0]
                y1 = y - h * anchor[1]
                return tuple.__new__( cls, (x1, y1, w, 0.0, w / float(h)) )
            
        raise ValueError, "don't understand arguments to Rect()"

    def interp( self, other, frac ):
        if frac == 0.0:
            return self
        elif frac == 1.0:
            return other
        return Rect( self[0] + (other[0] - self[0]) * frac,
                     self[1] + (other[1] - self[1]) * frac,
                     self[2] + (other[2] - self[2]) * frac,
                     self[3] + (other[3] - self[3]) * frac,
                     self[4] + (other[4] - self[4]) * frac )

    def top( self, f ):
        if not 0.0 < f <= 1.0:
            raise ValueError, "frac must be in range (0.0, 1.0]"
        ox, oy, ulen, utheta, aspect = self
        d = (ulen / aspect) * (1.0 - f)
        c, s = cossin( utheta + 90 )
        return Rect( ox + c * d, oy + s * d, ulen, utheta, aspect / f )
    
    def bottom( self, f ):
        if not 0.0 < f <= 1.0:
            raise ValueError, "frac must be in range (0.0, 1.0]"
        ox, oy, ulen, utheta, aspect = self
        return Rect( ox, oy, ulen, utheta, aspect / f )

    def left( self, f ):
        if not 0.0 < f <= 1.0:
            raise ValueError, "frac must be in range (0.0, 1.0]"
        ox, oy, ulen, utheta, aspect = self
        return Rect( ox, oy, ulen * f, utheta, aspect * f )
        
    def right( self, f ):
        if not 0.0 < f <= 1.0:
            raise ValueError, "frac must be in range (0.0, 1.0]"
        ox, oy, ulen, utheta, aspect = self
        d = ulen * (1.0 - f)
        c, s = cossin( utheta )
        return Rect( ox + c * d, oy + s * d, ulen * f, utheta, aspect * f )

    def ysplit( self, *y, **g ):
        e = not not g.get( 'ends', 1 )
        g = g.get( 'gutter', 0 )
        tg = g * (len(y)-1+2*e)
        if tg > 1.0:
            raise ValueError, "gutter too big for height of rectangle"
        ty = 0
        for i in y:
            ty += i
        ty = (1.0-tg) / ty
        y = [i * ty for i in y]
        y.reverse()
        ox, oy, ulen, utheta, aspect = self
        
        result = []
        s = g * e
        h = ulen / aspect
        cos, sin = cossin( utheta + 90 )
        for i in y:
            result.append( Rect( ox + cos * h * s, oy + sin * h * s, ulen, utheta, aspect / i ) )
            s += i + g
        result.reverse()
        return result

    def xsplit( self, *x, **g ):
        e = not not g.get( 'ends', 1 )
        g = g.get( 'gutter', 0 )
        tg = g * (len(x)-1+2*e)
        if tg > 1.0:
            raise ValueError, "gutter too big for width of rectangle"
        tx = 0
        for i in x:
            tx += i
        tx = (1.0-tg) / tx
        x = [i * tx for i in x]
        ox, oy, ulen, utheta, aspect = self
        
        result = []
        s = g * e
        cos, sin = cossin( utheta )
        for i in x:
            result.append( Rect( ox + cos * ulen * s, oy + sin * ulen * s, ulen * i, utheta, aspect * i ) )
            s += i + g
        return result

            
            
            
        
        
                
            

    def move_right( self, d, abs = 0 ):
        ox, oy, ulen, utheta, aspect = self
        if not abs: d *= ulen
        c, s = cossin( utheta )
        return Rect( ox + c * d, oy + s * d, ulen, utheta, aspect )

    def move_left( self, d, abs = 0 ):
        return self.move_right( -d, abs )

    def move_up( self, d, abs = 0 ):
        ox, oy, ulen, utheta, aspect = self
        if not abs: d *= ulen / aspect
        c, s = cossin( utheta + 90 )
        return Rect( ox + c * d, oy + s * d, ulen, utheta, aspect )

    def move_down( self, d, abs = 0 ):
        return self.move_up( -d, abs )

    def restrict_aspect( self, newaspect ):
        aspect = self[4]
        if newaspect > aspect:
            frac = aspect / newaspect
            x = (1.0 - frac) / 2
            return self.inset( 0.0, x )
        else:
            frac = newaspect / aspect
            x = (1.0 - frac) / 2
            return self.inset( x, 0.0 )

    def inset( self, *d, **kw ):
        ox, oy, ulen, utheta, aspect = self
        height = ulen / aspect

        frac = 'abs' not in kw or not kw['abs']
        
        if len(d) == 1 and not frac:
            d = d[0]
            if ulen <= 2 * d or height <= 2 * d:
                raise ValueError, "inset rectangle is empty"
            c, s = cossin( utheta )
            return Rect( ox + c * d - s * d, oy + s * d + c * d, ulen - 2 * d, utheta,
                         (ulen - 2*d) / (height - 2*d) )
        
        if len(d) == 2:
            dw, dh = d
        else:
            dw = dh = d[0]
        if frac:
            dw *= ulen
            dh *= height
            
        if ulen <= 2 * dw or height <= 2 * dh:
            raise ValueError, "inset rectangle is empty"
        c, s = cossin( utheta )
        return Rect( ox + c * dw - s * dh, oy + s * dw + c * dh, ulen - 2 * dw, utheta,
                     (ulen - 2*dw) / (height - 2*dh) )

    def outset( self, *d, **kw ):
        ox, oy, ulen, utheta, aspect = self
        height = ulen / aspect

        frac = 'abs' not in kw or not kw['abs']
        
        if len(d) == 1 and not frac:
            d = d[0]
            if d < 0:
                raise ValueError, "outset amount must be nonnegative" 
            c, s = cossin( utheta )
            return Rect( ox - c * d + s * d, oy - s * d - c * d, ulen + 2 * d, utheta,
                         (ulen + 2*d) / (height + 2*d) )
        
        if len(d) == 2:
            dw, dh = d
        else:
            dw = dh = d[0]
        if frac:
            dw *= ulen
            dh *= height
            
        if dw < 0 or dh < 0:
            raise ValueError, "outset amount must be nonnegative" 
        c, s = cossin( utheta )
        return Rect( ox - c * dw + s * dh, oy - s * dw - c * dh, ulen + 2 * dw, utheta,
                     (ulen + 2*dw) / (height + 2*dh) )


    def width( self ):
        return self[2]

    def height( self ):
        return self[2] / self[4]
    
    def aspect( self ):
        return self[4]
    
    
    
        
    
                     
    
        

        

        
                         
    
        
