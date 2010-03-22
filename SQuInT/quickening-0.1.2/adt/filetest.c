#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include "filetest.h"

boolean fileExists(const char *filename) {
  struct stat s; 
  if(stat(filename,&s)== 0) {
    if(S_ISREG(s.st_mode)) {
      return TRUE;
      /*     } else {
             printf("%s is not a regular file\n", filename); */
    }
  } 
  return FALSE;
}
boolean fileIsNewerThan(const char *filename, struct timeval *tv) {
  struct stat s; 
  if(stat(filename,&s)== 0) {
    // mac defines mtimespec, which is more precise, but doesn't 
    // appear universal.
    if(s.st_mtime >= tv->tv_sec) { 
      return TRUE;
    }
  } 
  return FALSE;
}

boolean dirExists(const char *dirname) {
  struct stat s; 
  if(stat(dirname,&s)== 0) {
    if(S_ISDIR(s.st_mode)) {
      return TRUE;
      /* } else {
         printf("%s is not a directory\n", dirname); */
    }
  } 
  return FALSE;
}

/* acts like execvp, with code inspired by it */
/* mustfree */
/*@null@*/
char *searchPath(/*@null@*/ const char *path,	
                 /*@notnull@ */ const char *find_me)
{
	char *buf;
	const char *p;
	int len, pathlen;
    if (path == NULL) return NULL;
	/* if (strchr(find_me, '/') != NULL) { */
	if (find_me[0] == '/' || find_me[0] == '.') {
		return (strdup_ordie(find_me));
	}
	pathlen = strlen(path);
	len = strlen(find_me) + 1;
	buf = malloc_ordie(pathlen + len + 1);
	memcpy(buf + pathlen + 1, find_me, len);
	buf[pathlen] = '/';

	for (p = path; p != NULL; path = p, path++) {
		char *startp;
		p = strchr(path, ':');
		if (p == NULL) {
			/* not found; p should point to the null char at the end */
			startp =
				memcpy(buf + pathlen - strlen(path), path, strlen(path));
		} else if (p == path) {
			/* double colon in a path apparently means try here */
			startp = &buf[pathlen + 1];
		} else {
			/* copy the part between the colons to the buffer */
			startp = memcpy(buf + pathlen - (p - path), path, p - path);
		}
		if (fileExists(startp) != 0) {
			char *ret = strdup_ordie(startp);
			free(buf);
			return (ret);
		}
	}
	free(buf);
	return (NULL);
}
