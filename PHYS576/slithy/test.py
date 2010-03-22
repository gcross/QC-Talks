import diafont, cPickle

f = open( 'minion.slf', 'rb' )
y = cPickle.load(f)
f.close()

print y[0]

x = diafont.new_font( y )

print x.name
print x.pixelsize
