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
#include <assert.h>
#include "hashtable.h"
#include "typed_queue.h"
#include "typed_hashtable.h"
#include "disjoint_set.h"

struct djs_elt {
  void *value;
  struct djs_elt *next;
  struct djs_elt *head;
};

DECLARE_TYPED_QUEUE(struct djs_elt, ds);

struct disjoint_sets {
  ds_queue st; /* list of sets */
  hashtable ht; /* value -> djs_elt */
};

disjoint_set djs_new( unsigned int elts, hash_cb hc, isequal_cb ic ) {
  struct disjoint_sets *djs = 
    (struct disjoint_sets *)malloc(sizeof(struct disjoint_sets));
  /* should change one of these nulls to a free to get cleanup behavior */
  djs->st = ds_q_new(NULL, NULL);
  djs->ht = ht_new( elts, hc, ic, NULL );
  return(djs);
}

struct djs_elt *
djs_find(struct disjoint_sets *djs, void *v) {
  struct djs_elt *de = (struct djs_elt *)ht_lookup(djs->ht, v);
  return(de);
}
struct djs_elt *
djs_insert(struct disjoint_sets *djs, void *v) {
  struct djs_elt *de = (struct djs_elt *) malloc(sizeof(struct djs_elt)); 
  de->value = v;
  de->next = NULL; 
  de->head = de;
  ht_insert(djs->ht, de);
  ds_q_insert(djs->st, de);
  return(de);
}

struct djs_elt *
djs_find_or_insert(struct disjoint_sets *djs, void *v) {
  struct djs_elt *de = (struct djs_elt *)ht_lookup(djs->ht, v);
  if(de == NULL) {
    de=djs_insert(djs,v);
  }
  return(de);
}

void 
djs_merge(struct disjoint_sets *djs, struct djs_elt *a, struct djs_elt *b) {
  struct djs_elt *a2, *e;
  assert(a);
  assert(b);
  /* cuz those are the only references that should be exposed */
  assert(a->head == a);
  assert(b->head == b);

  /* insert all of b after the first element of a,
     but before the rest.  This liberates us from
     having to traverse both lists to the end - one
     finding the end, the other reassigning head pointers */
  a2 = a->next;
  for( e = b; e->next != NULL; e=e->next ) {
    e->head = a;
  }
  e->next = a2;
  ds_q_remove(djs->st, b);
}


struct iteration {
  void (*each_set)(void *a);
  void (*each_value)(void *v, void *a);
  void *a;
};
  
boolean djs_iterate_helper(const struct djs_elt *head, void *iter) {
  struct iteration *i = (struct iteration *)iter;
  const struct djs_elt *p;
  i->each_set(i->a);
  for(p= head; p!= NULL; p=p->next) {
    i->each_value(p->value, i->a);
  }
  return TRUE;
}

void 
djs_iterate(struct disjoint_sets *djs, 
            void (*each_set)(void *a),
            void (*each_value)(void *v, void *a),
            void *a) {
  struct iteration i;
  i.each_set = each_set;
  i.each_value = each_value;
  i.a = a;
  ds_q_iterate(djs->st, djs_iterate_helper, &i);
}
  
