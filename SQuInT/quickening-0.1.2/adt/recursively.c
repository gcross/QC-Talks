/*
 * Copyright (c) 2002
 * Neil Spring and the University of Washington.
 * All rights reserved. 
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author(s) may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.  
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR(S) BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif
#include <dirent.h>
#ifdef MUST_INCL_SCANDIR_H
#include "scandir.h"
#endif
#include "recursively.h"
#include "progress.h"
#include "nscommon.h"

/* TODO: avoid loops. */
#ifdef __LCLINT__
int scandir(const char *dir, /*@out@*/struct dirent ***namelist,
              int (*select)(const struct dirent *),
              int (*compar)(const void *, const void *));
int alphasort(const void *a, const void *b);
#endif


#if defined(__linux__) || defined(MUST_INCL_SCANDIR_H)
typedef int (*scandir_cb)(const struct dirent *);
#else
typedef int (*scandir_cb)(struct dirent *);
#endif

/* returns 1 if the file's name is not . or .. */
/* i.e. if not-equal to . and not equal to .. */
static int scandir_select_default( const struct dirent *de ) {
  boolean retval = (strcmp(de->d_name, ".") != 0) && 
    (strcmp(de->d_name, "..") != 0) &&
    (strcmp(de->d_name, "CVS") != 0) &&
    (strcmp(de->d_name, "core") != 0);
  return (int)retval;
}

void recursively(const char *f_or_dname, 
                 void (*callback)(const char * filename, 
                                  void *a), 
                 void *a,
                 int (*scandir_select)(const struct dirent *),
                 int progressbardepth) {

  struct stat st;
  if(stat(f_or_dname, &st) != 0) {
    printf("unable to stat '%s'\n", f_or_dname);
    return;
  } 

  if(S_ISDIR(st.st_mode)) {
    if(scandir_select == NULL) scandir_select = scandir_select_default;
    if(progressbardepth > 0) {
      struct dirent **namelist;
      int scandir_ret;
      scandir_ret = scandir(f_or_dname, &namelist, (scandir_cb)scandir_select, alphasort);
      if(scandir_ret<0) perror("scandir");
      else {
        unsigned int i;
        unsigned int n = (unsigned int)scandir_ret;
        for(i=0; i<n; i++) {
          char *fname = malloc(strlen(f_or_dname) + strlen(namelist[i]->d_name) + 2);
          sprintf(fname, "%s/%s", f_or_dname, namelist[i]->d_name);
          if(progressbardepth > 0) { 
            progress_label(namelist[i]->d_name);
            progress_n_of(i,n);
            /*             fprintf(stderr, "%d/%d\r", i,n); */
            /* if(progressbardepth > 1) fprintf(stderr,"\n"); */
          }
          recursively(fname, callback, a, scandir_select, progressbardepth - 1);
          free(fname);
        }
        if(progressbardepth > 0) progress_n_of(n,n);
      }
    } else {
      DIR *D = opendir(f_or_dname);
      struct dirent *de;
      if(D==NULL) {
        printf("couldn't open '%s' as a directory\n", f_or_dname);
        return;
      }
      fprintf(stderr, "\rworking on %s...\r", f_or_dname);
      while((de = readdir(D)) != NULL) {
        if(scandir_select(de) != 0) {
          char *fname = malloc(strlen(f_or_dname) + strlen(de->d_name) + 2);
          sprintf(fname, "%s/%s", f_or_dname, de->d_name);
          recursively(fname, callback, a, scandir_select, 0);
          free(fname); 
        }
      }
      (void)closedir(D);
    }
  } else {
    /* tis a file. operate */
    (*callback)(f_or_dname, a);
  }
}
