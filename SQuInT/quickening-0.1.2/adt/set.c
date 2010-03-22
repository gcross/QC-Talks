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

/* a kinda lame doubly linked list implementation of a set. */
/* should probably be renamed dll, as there aren't any set 
   operations */ 

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#ifdef HAVE_LIBPTHREAD
#include <pthread.h>
#endif
#include <stdlib.h>
#include <assert.h>
#include "set.h"
#ifdef  WITH_DMALLOC
#include <dmalloc.h>
#endif

/* implemented as a doubly linked list */
struct set_struct {
  struct set_element *first;
  struct set_element *last;
  void (*free_value)(void *value);
  int (*comparator)(const void *v1, const void *v2, size_t len);
  int len;  /* makes it easy to use memcmp as the comparator */
  pthread_mutex_t mutex; /* unimplemented */
};

struct set_element {
  void *value;
  struct set_element *prev, *next;
};

static boolean set_validate(const set s);
void *set_value_from_si(set_iterator si) {
  return((si)? si->value : NULL);
}
set set_new(void (*free_value)(void *value), 
            int (*comparator)(const void *v1, const void *v2, size_t len), 
            size_t len ) {
  set s = (set)malloc(sizeof(struct set_struct));
  s->first=s->last=NULL;
  s->free_value = free_value;
  s->comparator = comparator;
  s->len = len;
  assert(set_validate(s));
  return(s);
}

static void * set_free(void *value, void *user) {
  set s = (set)user;
  if(s->free_value) {
    s->free_value(value);
  }
  return (NULL);
}

void set_delete(set deleteme) {
  set_destructive_iterate(deleteme, set_free, deleteme);
}

void set_add(set s, void *value) {
  struct set_element *se = 
    (struct set_element *)malloc(sizeof(struct set_element));
  assert(s!=NULL);
  assert(se!=NULL);
  assert(set_validate(s));
  se->value = value;
  if(s->last == NULL) {
    s->last = se;
  }
  se->next = s->first;
  se->prev = NULL;
  if(s->first) 
    s->first->prev = se;
  s->first = se;
  assert(set_validate(s));
}

void *set_iterate(set s, set_callback cb, void *user) {
  struct set_element *se;
  void *r=NULL;
  assert(s!=NULL);
  assert(cb!=NULL);
  for(se = s->first; 
      se != NULL && ((r=cb(se->value,user))==NULL); 
      se = se->next);
  return(r);
}
/* callback with two void parameters, for convenience */
void *set_iterate2(set s, set_callback2 cb, void *user, void *user2) {
  struct set_element *se;
  void *r=NULL;
  assert(s!=NULL);
  assert(cb!=NULL);
  for(se = s->first; 
      se != NULL && ((r=cb(se->value,user,user2))==NULL); 
      se = se->next);
  return(r);
}
set_iterator set_find(set s, const void *value) {
  set_iterator se;
  for(se = s->first; 
      se != NULL && se->value != value; 
      se = se->next) {
    if(s->comparator != NULL && s->comparator(se->value, value, s->len)==0) 
      return(se);
  }
  return(se);
}
void set_remove(set s, void *value) {
  struct set_element *se;
  assert(s!=NULL);
  assert(set_validate(s));
  se = set_find(s, value);
  if(se!=NULL) {
    if(se->prev == NULL) {
      s->first = se->next;
    } else {
      se->prev->next = se->next;
    }
    if(se->next == NULL) {
      s->last = se->prev;
    } else {
      se->next->prev = se->prev;
    }
    free(se);
  }
}
/* caller is responsible for dealing with freeing value */
void set_destructive_iterate(set s, set_callback cb, void *user) {
  struct set_element *se;
  assert(s!=NULL);
  assert(cb!=NULL);
  for(se = s->first; 
      se != NULL; 
      se = s->first) {
    cb(se->value, user);
    s->first = se->next;
    free(se);
  }
  free(s);
}

static void *si_value_quux (set_iterator *si) {
  if((*si)!=NULL)
    return((*si)->value);
  else
    return(NULL);
}
void *set_first(set s, set_iterator *si){
  *si = s->first;
  return(si_value_quux(si));
}
void *set_next(set_iterator *si) {
  assert(si!=NULL);
  *si = (*si)->next;
  return(si_value_quux(si));
}
static void *counter_cb(void *value __attribute__ ((unused)), void *user) {
  unsigned int *counter = user;
  (*counter)++;
  return(NULL);
}
unsigned int set_count(set s){
  unsigned int counter=0;
  set_iterate(s, counter_cb, &counter);
  return(counter);
}

static boolean set_validate(const set s) {
  set_iterator se;
  for(se = s->first; 
      se != NULL; 
      se = se->next){
    if(se->prev && se->prev->next!=se) {
      return FALSE;
    }
    if(se->next && se->next->prev!=se) {
      return FALSE;
    }
  }
  return(TRUE);
}
