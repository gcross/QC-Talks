class Color(tuple):
    def __new__( cls, *c ):
        try:
            l = len(c)
            if l == 1:
                c = c[0]
                if isinstance( c, tuple ) and 3 <= len(c) <= 4:
                    rgba = c
                else:
                    c = float( c )
                    rgba = (c, c, c, 1.0)
            elif l == 2:
                if isinstance( c[0], tuple ) and 3 <= len(c[0]) <= 4:
                    rgba = c[0][:3] + (float(c[1]),)
                else:
                    rgba = (float(c[0]),) * 3 + (float(c[1]),)
            elif l == 3:
                rgba = (float(c[0]), float(c[1]), float(c[2]), 1.0)
            elif l == 4:
                rgba = (float(c[0]), float(c[1]), float(c[2]), float(c[3]))
            elif l == 0:
                rgba = (0.0, 0.0, 0.0, 1.0)
            else:
                raise ValueError, "bad color specification '%s'" % (c,)

            return tuple.__new__( cls, rgba )

        except (ValueError, TypeError):
            raise ValueError, "bad color specification '%s'" % (c,)
        
    def __abs__( self ):
        return Color( self[0] * 0.299 + self[1] * 0.587 + self[2] * 0.114, self[3] )

    def __str__( self ):
        return '<color %f %f %f %f>' % tuple(self)

    def __repr__( self ):
        return 'Color(%r,%r,%r,%r)' % tuple(self)

    def interp( self, other, frac ):
        if not isinstance( other, tuple ) or len(other) != 4:
            raise ValueError, "interp target is not color"
        
        return Color( self[0] * (1.0-frac) + other[0] * frac,
                      self[1] * (1.0-frac) + other[1] * frac,
                      self[2] * (1.0-frac) + other[2] * frac,
                      self[3] * (1.0-frac) + other[3] * frac )

linen = Color( 250.0/255.0, 240.0/255.0, 230.0/255.0 )
red = Color( 1.0, 0.0, 0.0 )
orange = Color( 1.0, 0.7, 0.0 )
yellow = Color( 1.0, 1.0, 0.0 )
green = Color( 0.0, 0.6, 0.0 )
blue = Color( 0.0, 0.0, 1.0 )
purple = Color( 0.6, 0.0, 0.8 )
black = Color( 0.0 )
white = Color( 1.0 )
invisible = Color( 0.0, 0.0 )

try:
    f = open( '/usr/lib/X11/rgb.txt', 'r' )
    X = {}
    for i in f:
        if '!' not in i:
            stuff = i.split()
            name = stuff[-1].strip().replace(' ','').lower()
            c = tuple( [float(x)/255.0 for x in stuff[:3]] )
            X[name] = apply( Color, c )
    f.close()
    del f, i, stuff, name, c, x
except:
    pass

def hsv( h, s, v, a = 1 ):
    if s == 0:
        return Color( v, a )
    else:
        h = h - int(h)
        i = int(h * 6)
        f = (h*6) - i
        p = v * (1-s)
        q = v * (1-(s*f))
        t = v * (1-(s*(1-f)))

        if i == 0:
            return Color( v, t, p, a )
        elif i == 1:
            return Color( q, v, p, a )
        elif i == 2:
            return Color( p, v, t, a )
        elif i == 3:
            return Color( p, q, v, a )
        elif i == 4:
            return Color( t, p, v, a )
        else:
            return Color( v, p, q, a )
