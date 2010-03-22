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
#include "nscommon.h"
#include <stdio.h>
#include "set.h"
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#ifdef  WITH_DMALLOC
#include <dmalloc.h>
#endif

void * pcb(void *val, void *user __attribute__((unused))) {
  printf("%d ", *(int *)val);
  return (NULL);
}
int main(int argc __attribute__((unused)), 
         char *argv[] __attribute__((unused))) {
  set s;
  set_iterator si;
  int *v;
  int vars[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
  int vars2[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };

  s = set_new(NULL, memcmp, sizeof(int));
  set_add(s, &vars[0]);
  printf("0 \n");
  set_iterate(s, pcb, NULL);
  set_delete(s);
  printf("\n===\n");

  s = set_new(NULL, memcmp, sizeof(int));
  set_add(s, &vars[0]);
  set_add(s, &vars[1]);
  set_add(s, &vars[2]);
  printf("2 1 0 \n");
  set_iterate(s, pcb, NULL);


  printf("\n");
  for(v=set_first(s, &si);
      v!=NULL;
      v=set_next(&si)) {
    printf("%d ", *v);
  } 
  assert(set_find(s,&vars[0]));
  assert(set_find(s,&vars[1]));
  assert(set_find(s,&vars[2]));
  assert(set_find(s,&vars[3])==0);
  assert(set_find(s,&vars[4])==0);
  assert(set_find(s,&vars[5])==0);

  assert(set_find(s,&vars2[0]));
  assert(set_find(s,&vars2[1]));
  assert(set_find(s,&vars2[2]));
  assert(set_find(s,&vars2[3])==0);
  assert(set_find(s,&vars2[4])==0);
  assert(set_find(s,&vars2[5])==0);
                  
                  
  set_delete(s);
  printf("\n===\n");

  exit(0);

}

