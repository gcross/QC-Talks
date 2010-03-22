#include "nscommon.h"

boolean fileExists(const char *filename);
boolean fileIsNewerThan(const char *filename, struct timeval *tv);
boolean dirExists(const char *filename);
char *searchPath(/*@null@*/ const char *path,	
                 /*@notnull@ */ const char *find_me);
