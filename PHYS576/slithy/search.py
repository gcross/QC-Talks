import sys, os, os.path, cPickle, diafont, diaimage

class PathList(list):
    def append( self, item ):
        if item not in self:
            super( PathList, self ).append( item )

    def search( self, filename, origpath = None ):
        for i in self:
            p = os.path.join( i, filename )
	    print p
	    print os.access( p, os.R_OK );
            if os.access( p, os.R_OK ):
                return p
        if origpath:
            p = os.path.join( origpath, filename )
            if os.access( p, os.R_OK ):
                return p
        raise IndexError, "couldn't find '%s' in search path" % (filename,)
            
                

    def __getslice__( self, i, j ):
        l = super(PathList, self).__getslice__( i, j )
        result = PathList()
        result += l
        return result

            
fontpath = PathList()
imagepath = PathList()

add_extra_characters = diafont.add_extra_characters

def search_font( fn, size = None ):
    fn = fontpath.search( fn )
    return load_font( fn, size )

def load_font( fn, size = None ):
    if size is None:
        f = open( fn, 'rb' )
        t = cPickle.load( f )
        f.close()
        return diafont.new_font( t )
    else:
        return diafont.new_font( fn, size )

load_image = diaimage.get_image

def search_image( fn ):
    return diaimage.get_image( imagepath.search( fn ) )

def create_slithy_fonts( *items ):
    # if caller is not being run as script, return
    import inspect
    try:
        if inspect.stack()[1][0].f_globals['__name__'] != '__main__':
            return
    except (IndexError, KeyError, AttributeError):
        return

    import diafont, cPickle

    count = 0
    for infile, pixels, outfile in items:
        print "creating '%s' from '%s' (at %d pixels)" % (outfile, infile, pixels)
        try:
            font = search_font( infile, pixels )
        except diafont.FontError, msg:
            print "   error loading '%s' : %s" % (infile, msg)
            continue
            
        try:
            f = open( outfile, 'wb' )
            cPickle.dump( font.astuple(), f, 1 )
            f.close()
        except (diafont.FontError, IOError), msg:
            print "   error writing '%s' : %s" % (outfile, msg)
            continue

        count += 1
            

    print "%d slithy fonts created." % (count,)
    sys.exit(0)
    

    
        
