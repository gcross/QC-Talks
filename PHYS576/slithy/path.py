import math
import draw
import types

class PathError(Exception):
    pass

def invalidate_all():
    for i in Path.instances:
        i.invalidate()
        

class Path:
    CLOSE = 0
    MOVE = 1
    LINE = 2
    CURVE = 3
    QCURVE = 4

    strings = ['closepath', 'moveto', 'lineto', 'curveto', 'qcurveto']
    instances = []

    invalid = []

    def __init__( self ):
        self.currpoint = None
        self.raw = []
        self._fill_list = None
        self._stroke_list = None
        self._widestroke_list = None
        self._bbox = None
        Path.instances.append( self )

    def __del__( self ):
        self.invalidate()

    def current_point( self ):
        return self.currpoint

    def erase_last( self ):
        self.raw.pop()
        self.invalidate()
        return self

    def invalidate( self ):
        if self._fill_list is not None:
            Path.invalid.append( self._fill_list )
            self._fill_list = None
        if self._stroke_list is not None:
            Path.invalid.append( self._stroke_list )
            self._stroke_list = None
        if self._widestroke_list is not None:
            Path.invalid.append( self._widestroke_list )
            self._widestroke_list = None
        self._bbox = None

    def clear( self ):
        self.__init__()

    def moveto( self, *xy ):
        if len(xy) == 1: xy = xy[0]
        self.raw.append( [Path.MOVE,xy[0],xy[1]] )
        self.currpoint = xy
        self.invalidate()
        return self

    def rmoveto( self, *xy ):
        if len(xy) == 1: xy = xy[0]
        if self.currpoint is None: raise PathError, "no current point"
        p = (self.currpoint[0] + xy[0], self.currpoint[1] + xy[1])
        self.raw.append( [Path.MOVE,p[0],p[1]] )
        self.currpoint = p
        self.invalidate()
        return self

    def lineto( self, *xy ):
        if len(xy) == 1: xy = xy[0]
        if self.currpoint is None: raise PathError, "no current point"
        self.raw.append( [Path.LINE,xy[0],xy[1]] )
        self.currpoint = xy
        self.invalidate()
        return self

    def rlineto( self, *xy ):
        if len(xy) == 1: xy = xy[0]
        if self.currpoint is None: raise PathError, "no current point"
        p = (self.currpoint[0] + xy[0], self.currpoint[1] + xy[1])
        self.raw.append( [Path.LINE,p[0],p[1]] )
        self.currpoint = p
        self.invalidate()
        return self

    def closepath( self ):
        self.raw.append( [Path.CLOSE,] )
        self.currpoint = None
        self.invalidate()
        return self

    def curveto( self, *p ):
        if len(p) == 3: p = p[0] + p[1] + p[2]
        if self.currpoint is None: raise PathError, "no current point"
        self.raw.append( [Path.CURVE,] + list(p) )
        self.currpoint = (p[4],p[5])
        self.invalidate()
        return self
    
    def rcurveto( self, *p ):
        if len(p) == 3: p = p[0] + p[1] + p[2]
        if self.currpoint is None: raise PathError, "no current point"
        x,y = self.currpoint
        self.raw.append( [Path.CURVE,p[0]+x,p[1]+y,p[2]+x,p[3]+y,p[4]+x,p[5]+y] )
        self.currpoint = (p[4]+x,p[5]+y)
        self.invalidate()
        return self
    
    def qcurveto( self, *p ):
        if len(p) == 2: p = p[0] + p[1]
        if self.currpoint is None: raise PathError, "no current point"
        self.raw.append( [Path.QCURVE,] + list(p) )
        self.currpoint = (p[2],p[3])
        self.invalidate()
        return self
    
    def rqcurveto( self, *p ):
        if len(p) == 2: p = p[0] + p[1]
        if self.currpoint is None: raise PathError, "no current point"
        x,y = self.currpoint
        self.raw.append( [Path.QCURVE,p[0]+x,p[1]+y,p[2]+x,p[3]+y] )
        self.currpoint = (p[2]+x,p[3]+y)
        self.invalidate()
        return self

    def arcn( self, *p ):
        if len(p) == 4:
            (x,y),beta,alpha,r = p
        elif len(p) == 5:
            x,y,beta,alpha,r = p
        else:
            raise PathError, "bad arguments to arc"

        if beta < alpha:
            circles = int((alpha - beta) / 360)
            beta += circles * 360
            while beta < alpha:
                beta += 360

        if alpha > 0:
            qa = int( alpha / 90 )
        else:
            qa = int( alpha / 90 ) - 1

        if beta > 0:
            qb = int( beta / 90 )
        else:
            qb = int( beta / 90 ) - 1

        arcs = []

        if qa == qb:
            arcs.append( arc( alpha, beta, x, y, r ) )
        else:
            if alpha != (qa+1)*90:
                arcs.append( arc( alpha, (qa+1)*90, x, y, r ) )
            for i in xrange(qa+1,qb):
                arcs.append( quarter_arc( i%4, x, y, r) )
            if beta != qb*90:
                arcs.append( arc( qb*90, beta, x, y, r ) )

        if not arcs:
            self.currpoint = (x + r * cos(beta * dtor), y + r * sin(beta * dtor) )
            return self

        if self.currpoint is None:
            self.raw.append( [Path.MOVE, arcs[-1][6], arcs[-1][7]] )
        else:
            self.raw.append( [Path.LINE, arcs[-1][6], arcs[-1][7]] )

        arcs.reverse()
        for i in arcs:
            j = flip_arc( i )
            self.raw.append( [Path.CURVE] + j[2:] )
        self.currpoint = tuple(arcs[0][:2])
        self.invalidate()
        return self

    def arc( self, *p ):
        if len(p) == 4:
            (x,y),alpha,beta,r = p
        elif len(p) == 5:
            x,y,alpha,beta,r = p
        else:
            raise PathError, "bad arguments to arc"

        if beta < alpha:
            circles = int((alpha - beta) / 360)
            beta += circles * 360
            while beta < alpha:
                beta += 360

        if alpha > 0:
            qa = int( alpha / 90 )
        else:
            qa = int( alpha / 90 ) - 1

        if beta > 0:
            qb = int( beta / 90 )
        else:
            qb = int( beta / 90 ) - 1

        arcs = []

        if qa == qb:
            arcs.append( arc( alpha, beta, x, y, r ) )
        else:
            if alpha != (qa+1)*90:
                arcs.append( arc( alpha, (qa+1)*90, x, y, r ) )
            for i in xrange(qa+1,qb):
                arcs.append( quarter_arc( i%4, x, y, r) )
            if beta != qb*90:
                arcs.append( arc( qb*90, beta, x, y, r ) )

        if not arcs:
            self.currpoint = (x + r * cos(beta * dtor), y + r * sin(beta * dtor) )
            return self

        if self.currpoint is None:
            self.raw.append( [Path.MOVE, arcs[0][0], arcs[0][1]] )
        else:
            self.raw.append( [Path.LINE, arcs[0][0], arcs[0][1]] )

        for i in arcs:
            self.raw.append( [Path.CURVE] + i[2:] )
        self.currpoint = tuple(arcs[-1][-2:])
        self.invalidate()
        return self
        
        
    
    def dump( self ):
        print 'path has', len(self.raw), 'elements:'
        for i in self.raw:
            print '  ', Path.strings[i[0]], i[1:]

    def bbox( self ):
        return draw.bbox( self )
            
    def translate( self, *p ):
        if len(p) == 2:
            x, y = p
        else:
            x, y = p[0], p[1]
        for e in self.raw:
            for i in range(1,len(e),2):
                e[i] += x
                e[i+1] += y
        self.invalidate()
                
    def rotate( self, *p ):
        if len(p) == 3:
            theta, x, y = p
        else:
            theta, (x, y) = p
        theta *= math.pi / 180.0
        c = math.cos( theta )
        s = math.sin( theta )
        for e in self.raw:
            for i in range(1,len(e),2):
                e[i], e[i+1] = x + c*(e[i]-x) - s*(e[i+1]-y), y + s*(e[i]-x) + c*(e[i+1]-y)
        self.invalidate()

    def scale( self, *p ):
        l = len(p)
        if l == 1:
            x = y = p[0]
            cx = cy = 0
        elif l == 2 and type(p[1]) is types.TupleType:
            x, (cx,cy) = p
            y = x
        elif l == 2:
            x, y = p
            cx = xy = 0
        elif l == 3:
            x, y, (cx,cy) = p
        elif l == 4:
            x, y, cx, cy = p

        for e in self.raw:
            for i in range(1,len(e),2):
                e[i], e[i+1] = cx+x*(e[i]-cx), cy+y*(e[i+1]-cy)
        self.invalidate()
            
            
            
            
            
            
dtor = math.pi / 180
arc_epsilon = 0.1 * dtor
quarter_offset = 4 * (math.sqrt(2)-1) / 3

def arc( alpha, beta, x, y, r ):
    alpha *= dtor
    beta *= dtor
    ca = math.cos( alpha )
    sa = math.sin( alpha )
    cb = math.cos( beta )
    sb = math.sin( beta )

    x0 = r * ca + x
    y0 = r * sa + y

    x3 = r * cb + x 
    y3 = r * sb + y

    if abs(alpha-beta) < arc_epsilon:
        return [x0, y0, x0, y0, x3, y3, x3, y3]

    a = r * (math.cos( (alpha + beta)/2 ) - (ca + cb) / 2) / (.375 * (sb - sa))
    
    x1 = x0 - a * sa
    y1 = y0 + a * ca

    x2 = x3 + a * sb
    y2 = y3 - a * cb

    return [x0, y0, x1, y1, x2, y2, x3, y3]

def quarter_arc( q, x, y, r ):
    rq = r * quarter_offset
    
    if q == 0:
        return [ x+r, y,
                 x+r, y+rq,
                 x+rq, y+r,
                 x, y+r ]
    elif q == 1:
        return [ x, y+r,
                 x-rq, y+r,
                 x-r, y+rq,
                 x-r, y ]
    elif q == 2:
        return [ x-r, y,
                 x-r, y-rq,
                 x-rq, y-r,
                 x, y-r ]
    else:
        return [ x, y-r,
                 x+rq, y-r,
                 x+r, y-rq,
                 x+r, y ]
    
def flip_arc( a ):
    return a[6:8] + a[4:6] + a[2:4] + a[0:2]

        
        
        
        

        
