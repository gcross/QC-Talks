import sys
import os

for name in sys.argv[1:]:
    print "***",name,"***"
    if name[-4:] == ".svg":
        name = name[:-4]
    os.system('/Applications/Inkscape.app/Contents/Resources/bin/inkscape -f %s.svg -A %s.pdf --export-text-to-path --export-bbox-page' % ((name,)*2) )

    os.system('/Applications/Inkscape.app/Contents/Resources/bin/inkscape -f %s.svg -E %s.eps --export-text-to-path --export-bbox-page' % ((name,)*2) )

