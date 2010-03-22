class Timeline:
    def __init__( self, type, value = None ):
        self.type = type
        self.spans = [ (None, None, value) ]

    def add_span( self, start, end, value ):
        if start is None:
            startindex = 0
        else:
            startindex = self.find_span_index( start )
            
        if end is None:
            endindex = len(self.spans)-1
        else:
            endindex = self.find_span_index( end )

        replacement = []
        if start is not None and (self.spans[startindex][0] is None or self.spans[startindex][0] < start):
            replacement.append( (self.spans[startindex][0], start, self.spans[startindex][2]) )
        replacement.append( (start, end, value) )
        if end is not None and (self.spans[endindex][1] is None or self.spans[endindex][1] > end):
            replacement.append( (end, self.spans[endindex][1], self.spans[endindex][2]) )

        self.spans[startindex:endindex+1] = replacement

    def merge_span( self, start, end, modifier ):
        if start is None:
            startindex = 0
        else:
            startindex = self.find_span_index( start )
            
        if end is None:
            endindex = len(self.spans)-1
        else:
            endindex = self.find_span_index( end )

        replacement = []
        if start is not None and (self.spans[startindex][0] is None or self.spans[startindex][0] < start):
            replacement.append( (self.spans[startindex][0], start, self.spans[startindex][2]) )

        if startindex == endindex:
            replacement.append( (start, end, modifier( self.spans[startindex][2] )) )
        else:
            replacement.append( (start, self.spans[startindex][1], modifier( self.spans[startindex][2] )) )
            for i in self.spans[startindex+1:endindex]:
                replacement.append( (i[0], i[1], modifier(i[2])) )
            replacement.append( (self.spans[endindex][0], end, modifier( self.spans[endindex][2] )) )

        if end is not None and (self.spans[endindex][1] is None or self.spans[endindex][1] > end):
            replacement.append( (end, self.spans[endindex][1], self.spans[endindex][2]) )

        self.spans[startindex:endindex+1] = replacement

    def dump( self ):
        for i in self.spans:
            if i[0] is None:
                start = '      '
            else:
                start = '%6.3f' % (i[0],)
            if i[1] is None:
                end = '      '
            else:
                end = '%6.3f' % (i[1],)

            if isinstance( i[2], str ):
                print '%s - %s  [%s]' % (start, end, i[2])
            elif isinstance( i[2], tuple  ) or isinstance( i[2], list ):
                if len(i[2]) == 0:
                    print '%s - %s  empty' % (start, end)
                else:
                    print '%s - %s  %s' % (start, end, i[2][0])
                    for j in i[2][1:]:
                        print ' ' * 16, j
            else:
                print '%s - %s  %s' % (start, end, i[2])
                
            
        print '--- end ---'

    def find_span_index( self, search ):
        for i in range(len(self.spans)):
            start, end = self.spans[i][:2]
            if (start is None or start <= search) and (end is None or end > search):
                return i
        raise RuntimeError, 'find_span_index: should never reach this point'

    def eval( self, when ):
        span = self.spans[self.find_span_index( when )]
        if callable( span[2] ):
            return span[2]( when )
        else:
            return span[2]

    def is_trivial( self ):
        return len(self.spans) == 1

    def trivial_value( self ):
        return self.spans[0][2]

    def purge( self, t ):
        i = self.find_span_index( t )
        if i == 0:
            return
        self.spans[:i] = [(None, self.spans[i][0], self.spans[0][2])]
    
    

if __name__ == '__main__':
    x = Timeline( 'foo', 'a' )
    x.add_span( 0, None, 'b' )
    x.add_span( 2, None, 'c' )
    x.add_span( 5, None, 'd' )
    x.add_span( 8, None, 'e' )

    def add( x, y ):
        return y+x
    
    
            

