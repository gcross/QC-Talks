SUBDIRS=adt
bin_PROGRAMS=quickening
quickening_SOURCES=main.c 
ACLOCAL_AMFLAGS=-I adt
EXTRA_DIST=keynote-sprt.dat

quickening_LDFLAGS=-framework QuickTime -framework CoreServices -framework ApplicationServices 
quickening_LDADD=-Ladt -ladt
quickening_DEPENDENCIES = $(srcdir)/adt adt/libadt.a

quickening_CFLAGS= -Wall -g -fpascal-strings \
		-I/System/Library/Frameworks/QuickTime.framework/Headers 

# clean up the dependency.
adt/libadt.a:
	cd adt && $(MAKE)

# splint doesn't like validating the include header files
# because they do not have a good path. (CoreServices.h is
# not in a CoreServices directory)

splint:
	splint +posixlib -I/System/Library/Frameworks/QuickTime.framework/Headers  main.c

