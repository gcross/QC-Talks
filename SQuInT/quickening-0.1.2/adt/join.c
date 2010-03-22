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
#include "join.h"
#include <stdlib.h>
#include <stdio.h>

struct joinargs {
  set s;
  hashtable b;
};
boolean join_cb(const void *keyval, void *user) {
  struct joinargs *pj = (struct joinargs *)user;
  if(ht_lookup(pj->b, keyval)!=NULL) {
    set_add(pj->s, (void *)keyval); /* de-constify */
  }
  return(TRUE);
}
void * join_s_cb(void *value, void *user) {
  struct joinargs *pj = (struct joinargs *)user;
  if(ht_lookup(pj->b, value)!=NULL) {
    set_add(pj->s, value);
  }
  return(NULL);
}
boolean seek_duplicates_cb(const void *keyval, void *user) {
  hashtable a = (hashtable)user;
  if(ht_lookup(a, keyval) != keyval) {
    printf("duplicates in hashtable");
    return (FALSE);
  }
  return(TRUE);
}
set joinhh(hashtable a, hashtable b) {
  struct joinargs j;
  j.s = set_new(NULL, NULL, 0);
  j.b = b;
  ht_iterate(a, seek_duplicates_cb, a);
  ht_iterate(a, join_cb, &j);
  return j.s;
}

set joinsh(set a, hashtable b) {
  struct joinargs j;
  j.s = set_new(NULL, NULL, 0);
  j.b = b;
  set_iterate(a, join_s_cb, &j);
  return j.s;
}

