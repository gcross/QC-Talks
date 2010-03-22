import types
import colors

class ParameterError:
    def __init__( self, errorlist ):
        self.errorlist = errorlist
    def __str__( self ):
        return str(self.errorlist)

SCALAR = 'scalar'
STRING = 'string'
BOOLEAN = 'boolean'
INTEGER = 'integer'
OBJECT = 'object'
COLOR = 'color'

def analyze_diagram_parameters( fn ):
    count = fn.func_code.co_argcount
    pnames = fn.func_code.co_varnames[:count]
    pvals = fn.func_defaults
    if pvals is None:
        pvals = ()

    errors = []

    lyst = []
    for n, v in zip( pnames, pvals ):
        if type(v) is not types.TupleType:
            v = (v,)
        l = len(v)
        if v[0] == SCALAR:
            if l == 3:
                lyst.append( (n, SCALAR, float(v[1]) ) )
            elif l == 4:
                lyst.append( (n, SCALAR, float(v[3]) ) )
            else:
                errors.append( '"%s" : SCALAR parameters must specify min and max' % n )
        elif v[0] == INTEGER:
            if l == 3:
                lyst.append( (n, INTEGER, int(v[1]) ) )
            elif l == 4:
                lyst.append( (n, INTEGER, int(v[3]) ) )
            else:
                errors.append( '"%s" : INTEGER parameters must specify min and max' % n )
        elif v[0] == STRING:
            if l == 1:
                lyst.append( (n, STRING, '') )
            else:
                lyst.append( (n, STRING, reduce(lambda x,y:x+y, map(str, v[1:]))))
        elif v[0] == BOOLEAN:
            if l == 1:
                lyst.append( (n, BOOLEAN, 0) )
            elif l == 2:
                lyst.append( (n, BOOLEAN, not not v[1]) )
            else:
                errors.append( '"%s" : too many defaults for BOOLEAN parameter' % n )
        elif v[0] == OBJECT:
            if l == 1:
                lyst.append( (n, OBJECT, None) )
            elif l == 2:
                lyst.append( (n, OBJECT, v[1]) )
            else:
                errors.append( '"%s" : too many defaults for OBJECT parameter' % n )
        elif v[0] == COLOR:
            if l == 1:
                lyst.append( (n, COLOR, colors.black) )
            elif l == 2:
                lyst.append( (n, COLOR, v[1]) )
            else:
                errors.append( '"%s" : too many defaults for COLOR parameter' % n )
        else:
            error.append( '"%s" : unknown parameter type "%s"' % (n, v[0]) )

    if len( errors ) > 0:
        raise ParameterError(errors)

    return lyst

def diagram_parameter_ranges( fn ):
    count = fn.func_code.co_argcount
    pnames = fn.func_code.co_varnames[:count]
    pvals = fn.func_defaults
    if pvals is None:
        pvals = ()

    errors = []

    d = {}
    
    for n, v in zip( pnames, pvals ):
        if type(v) is not types.TupleType:
            v = (v,)
        l = len(v)
        if v[0] == SCALAR:
            if l == 3 or l == 4:
                d[n] = (float(v[1]), float(v[2]))
            else:
                errors.append( '"%s" : SCALAR parameters must specify min and max' % n )
        elif v[0] == INTEGER:
            if l == 3 or l == 4:
                d[n] = (int(v[1]), int(v[2]))
            else:
                errors.append( '"%s" : INTEGER parameters must specify min and max' % n )

    if len( errors ) > 0:
        raise ParameterError(errors)

    return d

def gldiagram_parameter_ranges( obj ):
    d = {}
    for i in obj[1]:
        if i[1] in (SCALAR, INTEGER):
            d[i[0]] = (i[3], i[4])
    return d


# def check_parameters( p ):
#     used_names = ('x1', 'y1', 'x2', 'y2', 'alpha', 'draw', 'camera_ox', 'camera_oy',
#                   'camera_ulen', 'camera_utheta', 'camera_aspect')
#     bad = []

#     for i in p:
#         if i[0] in used_names:
#             bad.append( i[0] )

#     return bad

   
