from slithy.library import *

fontpath.append( '.' )
fontpath.append( '/Users/cog/Documents/Homework/PHYS576' )

add_extra_characters( u'\u201c\u201d' )

create_slithy_fonts( ('times.ttf', 50, 'times.slf'),
                     ('ehrhardt.pfb', 50, 'ehrhardt.slf'),
                     ('mob.pfb', 50, 'minion.slf'),
                     ('mobi.pfb', 50, 'minion_i.slf'),
                     )
                     
fonts = { 'times' : search_font( 'times.slf' ),
          'ehrhardt' : search_font( 'ehrhardt.slf' ),
          'body' : search_font( 'minion.slf' ),
          'bodyi' : search_font( 'minion_i.slf' ),
          }

imagepath.append( '.' )
