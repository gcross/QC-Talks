from __future__ import generators

import Tkinter
import sys, error, cPickle

sys.excepthook = error.show_user_error

__all__ = []
all = __all__

import draw
from draw import *
all += filter( lambda x: x[0] != '_', dir(draw) )

import colors
from colors import *
all += filter( lambda x: x[0] != '_', dir(colors) )

from path import Path
all.append( 'Path' )


from parameters import ParameterError, SCALAR, STRING, BOOLEAN, INTEGER, OBJECT, COLOR
all += ['ParameterError', 'SCALAR', 'STRING', 'BOOLEAN', 'INTEGER', 'OBJECT', 'COLOR']

import anim
from anim import *
all += filter( lambda x: type(getattr(anim,x)) is types.FunctionType, dir(anim) )
all.append( 'stack' )

import objects
from objects import *
all += objects.exports

from transition import Transition
all.append( 'Transition' )

from undulate import Undulation
all.append( 'Undulation' )

import macros
from macros import *
all += macros.__all__

from rect import Rect
all.append( 'Rect' )


from search import *
all.extend( ('fontpath', 'imagepath', 'load_font', 'load_image',
             'search_font', 'search_image', 'create_slithy_fonts',
             'add_extra_characters' ) )


_images_to_preload = []
def preload_images( *im ):
    _images_to_preload.extend( im )
all.append( 'preload_images' )


import viewport
all.append( 'viewport' )

from controller import Controller
all.append( 'Controller' )


from main import test_objects
all.append( 'test_objects' )

class IDReader:
    def __init__( self ):
        self.doquery = None
        self.mode = 0
    
    def set_doquery( self, doquery ):
        self.doquery = doquery

    def query( self, *xy ):
        if self.mode:
            return draw._read_id( *xy )
        else:
            if self.doquery is None:
                return None
            return self.doquery( xy )

_reader = IDReader()
query_id = _reader.query
all.append( 'query_id' )


#all.sort()
#for i in all:
#    print i
    
