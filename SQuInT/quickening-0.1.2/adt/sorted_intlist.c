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
#include "sorted_intlist.h"
#include "queue.h"
#include <stdlib.h>
#include <assert.h>

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif

struct sorted_intlist_struct {
  queue q;
};

static int si_comparator(const void *v1, const void *v2) {
  int a = (int)v1;
  int b = (int)v2;
  if(a<b) {
    return -1;
  } else if(a>b) {
    return 1;
  } else {
    return 0;
  }
}

/*@only@*/ sorted_intlist si_new(void) {
  sorted_intlist si = (sorted_intlist) 
    malloc(sizeof(struct sorted_intlist_struct));
  si->q = q_new(si_comparator, NULL /* don't free */);
  return(si);
}

void si_delete(/*@only@*/ sorted_intlist si) {
  assert(si != NULL);
  assert(si->q != NULL);
  q_delete(si->q);
  free(si);
}

void si_insert(sorted_intlist si, int to_insert) {
  q_insert(si->q, (void *)to_insert);
}
void si_append(sorted_intlist si, int to_append) {
  q_append(si->q, (void *)to_append);
}

boolean si_iterate(sorted_intlist si, 
                   boolean (*iterator)(int value, void *user),
                   void *user) {
  if(si == NULL) {
    fprintf(stderr, "si_iterate called with a null intlist\n");
    abort();
  }
  return q_iterate(si->q, (q_iterator)iterator, user);
}

unsigned long si_length(sorted_intlist si) {
  return(q_length(si->q));
}

boolean si_member(sorted_intlist si, int value) {
  return(q_find(si->q, (void *)value) != 0);
}


static boolean intprint(const void *intvalue, void* file){ 
  FILE *fpout = (FILE *)file;
  int value = (int) intvalue;
  fprintf(fpout, "%d ", value);
  return TRUE;
}

void si_print(sorted_intlist si, FILE *fpout) {
  q_iterate(si->q, intprint, fpout);
  fprintf(fpout, "\n");
}

void si_remove(sorted_intlist si, int to_remove) {
  q_remove(si->q, (void *)to_remove);
}
