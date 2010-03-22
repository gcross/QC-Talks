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
#include <config.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include "recursively.h"
#include "commando.h"

int depth;
char *directory;

struct commandos commandos[] = {
  { "progressbar depth",
      "progress-depth", 'p', required_argument, commando_int, &depth },
  { "root directory",
      "directory", 'd', required_argument, commando_strdup, &directory },
  { NULL, NULL, '\0', 0, NULL, NULL }
}; 

void readit(const char *fname, void *dummy __attribute__((unused))) {
  FILE *f=fopen(fname, "r");
  char buf[4096];
  if(f!=NULL) {
    while(fread(buf,1,4096,f));
    fclose(f);
  }
}

int main(int argc, char *argv[]) {
  commando_parse(argc, argv, commandos);
  if(directory == NULL) {
    commando_usage(NULL, commandos);
    exit(EXIT_FAILURE);
  }
  recursively(directory, readit, NULL, NULL, depth);
  return(EXIT_SUCCESS);
}
