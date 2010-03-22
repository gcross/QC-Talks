# per-machine configuration.
# version configs -- for linux, this simplifies 
# finding the include files and libraries
TKVER=8.4
# which of your pythons? (in case you have python2.3 or /usr/local)
PYTHON=/usr/bin/python
MAC=1   # uncomment this line if you're on a mac.

# -----
# it's entirely possible you won't have to touch stuff 
# below here 
# -----

CC = gcc

# build using make MAC=1 to enable the macintosh build; see 
# README.Apple for details of support software to grab.
ifdef MAC
LINK = 	$(CC) -flat_namespace -undefined suppress -bundle 
# the following may be easily changed to /opt/local/bin, but
# only if opendarwin's package has FT_New_Face
# FTCONFIG = /usr/local/bin/freetype-config
FTCONFIG = /usr/X11R6/bin/freetype-config
CFLAGS = -g -O2 -Wall \
	-I/System/Library/Frameworks/OpenGL.framework/Headers \
	-I/System/Library/Frameworks/AGL.framework/Headers \
	-I/System/Library/Frameworks/Tcl.framework/Headers \
	-I/Library/Frameworks/Tk.framework/Headers \
	-I/System/Library/Frameworks/Python.framework/Headers \
	-Wno-long-double \
	-DUSE_APPLE_AGL -DAPPLE_FULLSCREEN \
	`$(FTCONFIG) --cflags` 

# -Wall -Wno-unused 
LIBPATH = 
LIBS = -framework Tcl -framework Tk -framework OpenGL -framework AGL \
	`$(FTCONFIG) --libs` -ldl -lm  \
	-bundle_loader /System/Library/Frameworks/Python.framework/Versions/Current/Resources/Python.app/Contents/MacOS/Python  
DLEXT=so
else
# linux build
LINK = 	$(CC) -shared
FTCONFIG = freetype-config
CFLAGS = -g -O2 \
	-I`$(PYTHON) -c "import distutils.sysconfig; print distutils.sysconfig.get_python_inc()"` \
	-I/uns/include \
	-I/usr/X11R6/include \
	-I/usr/include/tcl$(TKVER) \
	`$(FTCONFIG) --cflags` 
LIBPATH = -L/usr/lib -L/uns/lib -L/usr/X11R6/lib
LIBS = -lGL -lGLU -ldl -lXext -lX11 -lm -ltcl$(TKVER) -ltk$(TKVER) `$(FTCONFIG) --libs`
DLEXT=so
endif

INSTALL_TO=`$(PYTHON) -c "import distutils.sysconfig; print distutils.sysconfig.get_python_lib()"`

TARGETS = dobj.$(DLEXT) draw.$(DLEXT) diafont.$(DLEXT) diaimage.$(DLEXT)

SOURCES = diafont.c diaimage.c \
	  dr_bezier.c dr_color.c dr_draw.c dr_grid.c dr_image.c dr_path.c dr_stack.c \
	  dr_text.c dr_wide.c dr_xform.c dr_mark.c \
	  ob_opengl.c ob_slsoggy.c

all : $(TARGETS)

install : $(TARGETS)
	mkdir -p $(INSTALL_TO)/slithy
	install -m644 $(TARGETS) $(INSTALL_TO)/slithy
	install -m644 *.py $(INSTALL_TO)/slithy

dobj.$(DLEXT) : ob_slsoggy.o ob_opengl.o
	$(LINK) -o $@ $^ $(LIBPATH) $(LIBS)

draw.$(DLEXT) : dr_draw.o dr_bezier.o dr_path.o dr_grid.o dr_text.o dr_color.o dr_image.o \
	  dr_stack.o dr_xform.o dr_wide.o dr_mark.o
	$(LINK) -o $@ $^ $(LIBPATH) $(LIBS)

diafont.$(DLEXT) : diafont.o
	$(LINK) -o $@ $^ $(LIBPATH) $(LIBS)

diaimage.$(DLEXT) : diaimage.o
	$(LINK) -o $@ $^ $(LIBPATH) $(LIBS)


clean :
	\rm -rf *.o *.pyc \#*\# *~ $(TARGETS) core

depend :
	makedepend -D_MAKEDEPEND $(INCPATH) $(SOURCES)


# DO NOT DELETE

diafont.o: diafont.h
diaimage.o: diaimage.h
dr_bezier.o: dr_draw.h
dr_color.o: dr_draw.h
dr_draw.o: dr_draw.h
dr_grid.o: dr_draw.h
dr_image.o: dr_draw.h diaimage.h
dr_path.o: dr_draw.h
dr_stack.o: dr_draw.h
dr_text.o: dr_draw.h diafont.h
dr_wide.o: dr_draw.h
dr_xform.o: dr_draw.h
dr_mark.o: dr_draw.h
