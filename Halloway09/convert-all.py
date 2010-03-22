import os, os.path

for filename in os.listdir('.'):
    name, ext = os.path.splitext(filename)

    if ext == '.svg':
        print name
        os.system('/Applications/Inkscape.app/Contents/Resources/bin/inkscape -f %s.svg -A %s.pdf --export-text-to-path --export-bbox-page' % ((name,)*2) )
