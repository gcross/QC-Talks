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
#include <string.h>
#include <assert.h>
#ifdef HAVE_LIBPTHREAD
#include <pthread.h>
#else 
// dealt with using AC_NSPRING_NO_PTHREAD typedef void *pthread_mutex_t;
#define pthread_mutex_lock(x) (0)
#define pthread_mutex_unlock(x) (0)
#define pthread_mutex_init(x,y) (0)
#define pthread_mutex_destroy(x) (0)
#endif
#include "queue.h"
#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

typedef struct queue_element_struct {
  const void *value;
  struct queue_element_struct *next;
  struct queue_element_struct *prev;
} *q_element;
  
struct queue_struct {
  q_element head, tail;
  int (*compare)(const void *v1, const void *v2);
  void (*release)(void *v); /* aka free */
  pthread_mutex_t mutex;
};

/*
#define LOCK { printf("^"); fflush(stdout); assert(pthread_mutex_lock(&q->mutex)==0); } 
#define UNLOCK { assert(pthread_mutex_unlock(&q->mutex)==0); printf("."); fflush(stdout); };
*/
#define LOCK assert(pthread_mutex_lock(&q->mutex)==0);
#define UNLOCK assert(pthread_mutex_unlock(&q->mutex)==0);

static inline void check_invariants(queue q) {
  assert(q->head == NULL || q->head->prev == NULL);
  assert(q->tail == NULL || q->tail->next == NULL);
}

/* compare has strcmp semantics */
queue q_new(int (*compare)(const void *v1, const void *v2),
            void (*release)(void *v)) {
  queue q = (queue)malloc(sizeof(struct queue_struct));
  assert(q!=NULL);
  memset(q, 0, sizeof(struct queue_struct));
  q->compare = compare;
  q->release = release;
  (void)pthread_mutex_init(&q->mutex, NULL);
  return(q);
}

void q_delete(/*@only@*/ queue q) {
  void *v;
  v = (void *)q_pop(q);
  while(v) {
    if(q->release) q->release(v); /* aka free */
    v=(void *)q_pop(q);
  }
  (void)pthread_mutex_destroy(&q->mutex);
  free(q);
}

void q_insert(queue q, const void *v) {
  assert(q!=NULL);
  if(q->compare == NULL) {
    fprintf(stderr, "you used q_insert instead of q_append on an unsorted queue\n");
    assert(q->compare!=NULL); /* insert is sorted; needs a comparator */
  }
  LOCK;
  if(!q->head) {
    q_element newelem = malloc(sizeof(struct queue_element_struct));
    assert(newelem!=NULL);
    newelem->value = v;
    assert(q->tail == NULL);
    newelem->prev = newelem->next = NULL;
    q->head = q->tail = newelem;
  } else {
    q_element iter = q->head;
    for(iter = q->head; 
        iter!=NULL && q->compare(iter->value, v) <= 0; 
        /* <= there should append equal things.  < should prepend equal things */
        iter = iter->next);
    if(iter) { // must insert in middle 
      q_element newelem = malloc(sizeof(struct queue_element_struct));
      assert(newelem!=NULL);
      newelem->value = v;
      newelem->next = iter;
      newelem->prev = iter->prev;
      iter->prev = newelem;
      if(newelem->prev) {
        newelem->prev->next = newelem;
      } else {
        q->head = newelem;
      }
    } else {
      UNLOCK; // argh.  TODO: clean this mess.
      q_append(q,v);
      LOCK;
    }
  }
  UNLOCK;
}
void q_append(queue q, const void *v) {
  q_element newelem = malloc(sizeof(struct queue_element_struct));
  assert(newelem!=NULL);
  assert(q!=NULL);
  newelem->value = v;
  newelem->next = NULL;
  LOCK;
  check_invariants(q);
  newelem->prev = q->tail;
  if(q->tail) {
    q->tail->next = newelem;
  } else {
    assert(q->head == NULL);
    q->head = newelem;
  }
  q->tail = newelem;
  check_invariants(q);
  UNLOCK;
}

static /*@null@*/ const void *q_top_gotlock(const queue q) {
  const void * retval = NULL;
  if(q->head) {
    retval = q->head->value;
  }
  return retval;
}

/*@null@*/ const void *q_pop(queue q) {
  const void *retval;
  q_element freeme;
  LOCK;
  check_invariants(q);
  freeme = q->head;
  retval = q_top_gotlock(q);
  if(freeme!=NULL) {
    q->head = q->head->next;
    if(q->head)
      q->head->prev=NULL;
    else
      q->tail=NULL;
    free(freeme);
  }
  check_invariants(q);
  UNLOCK;
  return(retval);
}
      
/*@null@*/ const void *q_top(const queue q) {
  const void * retval;
  LOCK;
  retval = q_top_gotlock(q);
  UNLOCK;
  return(retval);
}

/* returns true if not interrupted by a false callback return val */
boolean q_iterate(queue q, q_iterator iterator, void *user) {
  q_element i;
  boolean cont = TRUE;
  LOCK;
  check_invariants(q);
  for( i = q->head; i != NULL && cont; i = i->next ) {
    cont = iterator(i->value, user);
  }
  UNLOCK;
  return cont;
}

/* returns true if not interrupted by a false callback return val */
boolean q_iterate_q(queue q, q_iterator_q iterator, void *user) {
  q_element i;
  boolean cont = TRUE;
  LOCK;
  check_invariants(q);
  for( i = q->head; i != NULL && cont; i = i->next ) {
    cont = iterator(q, i->value, user);
  }
  UNLOCK;
  return cont;
}

static boolean counter(const void *v __attribute__ ((unused)),
                       void *count) {
  (*(unsigned long *)count)++;
  assert((*(unsigned long *)count) < 200000);
  return TRUE;
}
unsigned long q_length(queue q) {
  unsigned long count=0;
  (void)q_iterate(q, counter, &count);
  return count;
}

static boolean finder(queue q, const void *v, void *user) {
  void **found = (void **)user;
  if(q->compare(v, *found) == 0) {
    *found = (void *)v;
    return FALSE;
  }
  return TRUE;
}

void *q_find(queue q, const void *findme) {
  void *found;
  found = (void *)findme;
  if(q_iterate_q(q, finder, &found) == 0)
    return(found);
  else
    return(NULL);
}

void q_remove(queue q, void *removeme) {
  q_element prev = NULL;
  q_element p;  /* the one to remove */
  check_invariants(q);
  for(p=q->head; 
      p!=NULL && p->value != removeme;
      prev = p, p=p->next);
  if(p != NULL) {
    if(prev != NULL) {
      prev->next = p->next;
    } else {
      q->head = p->next;
    }
    if(p->next == NULL) {
      q->tail = prev;
    } else {
      p->next->prev = prev;
    }
    free(p);
  }
  check_invariants(q);
}
