import sys, traceback, os.path

debug = 1

def call( fn ):
    global system_path
    try:
        result = fn()
    except:
        if debug:
            raise
        else:
            show_user_error()
            raise ValueError

    return result

def show_user_error( t = None, v = None, tb = None ):
    if not t:
        t = sys.exc_type
        v = sys.exc_value
        tv = sys.exc_traceback

    if debug:
        traceback.print_exception( t, v, tb )
    else:
        lyst = traceback.extract_tb( tb )
        print >> sys.stderr, '%s: %s' % (str(t), str(v))
        print >> sys.stderr
        for i in lyst:
            if os.path.dirname( i[0] ) != system_path:
                print >> sys.stderr, '  %s:%d (in \'%s\'):\n    %s' % i
        print >> sys.stderr
    

def show_diagram_exc( type, value, tb ):
    if debug:
        traceback.print_exception( type, value, tb )
    else:
        lyst = traceback.extract_tb( sys.exc_traceback )
        print >> sys.stderr
        print >> sys.stderr, 'in diagram function:  %s: %s' % (str(type), str(value))
        print >> sys.stderr
        for i in lyst:
            if os.path.dirname( i[0] ) != system_path:
                print >> sys.stderr, '  %s:%d (in \'%s\'):\n    %s' % i
        print >> sys.stderr
    
    

system_path = os.path.dirname( call.func_code.co_filename )

