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
#include "queue.h"
#include "eventqueue.h"
#include "nscommon.h"
#ifdef HAVE_LIBPTHREAD
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

struct event_queue_struct {
  queue q;
  boolean dying;
  pthread_mutex_t mutex;
  pthread_cond_t cond;
  pthread_t *workers;
  unsigned short min_workers, max_workers, current_workers, idle_workers;
};

typedef struct event_struct {
  struct timeval tv;
  unsigned int restartintervalms;
  ev_callback callback;
  void *user;
} *event;

int compare_timeval(const void *v1, 
                    const void *v2) {
  const struct timeval *tv1 = v1;
  const struct timeval *tv2 = v2;
  if(tv1->tv_sec < tv2->tv_sec)
    return -1;
  if(tv1->tv_sec > tv2->tv_sec) 
    return 1;
  if(tv1->tv_usec < tv2->tv_usec)
    return -1;
  if(tv1->tv_usec > tv2->tv_usec)
    return 1;
  return 0;
}

static /*@null@*/ void *ev_worker_thread(void *eventqvoid) {
  eventq eq = (eventq)eventqvoid;
  event ev;
  event etop;
  struct timeval now;
  DEBUG_PRINT(("worker started\n"));
  while(!eq->dying) {
    DEBUG_PRINT(("tid %ld locking\n", pthread_self()));
    assert(pthread_mutex_lock(&eq->mutex)==0);
    DEBUG_PRINT(("tid %ld locked\n", pthread_self()));
    while(!eq->dying && q_top(eq->q) == NULL) {
      DEBUG_PRINT(("tid %ld waiting\n", pthread_self()));
      pthread_cond_wait(&eq->cond, &eq->mutex);
      DEBUG_PRINT(("tid %ld woke up\n", pthread_self()));
    }
    gettimeofday(&now, NULL);
    /* TODO: likely race condition here - grabbing the top
       of the queue, then doing something with it without
       popping. */
    while(!eq->dying && 
          (etop = (event)q_top(eq->q)) != NULL && 
          compare_timeval(&(etop->tv), &now) == 1) {
      struct timespec until;
#define USE_ETOP
#ifdef USE_ETOP
      until.tv_sec = etop->tv.tv_sec;
      until.tv_nsec = etop->tv.tv_usec*1000;
#else
      until.tv_sec = ((event)q_top(eq->q))->tv.tv_sec;
      until.tv_nsec = ((event)q_top(eq->q))->tv.tv_usec*1000;
#endif
      DEBUG_PRINT(("tid %ld waiting, event in mind \n", 
             pthread_self()));
      (void)pthread_cond_timedwait(&eq->cond, &eq->mutex, &until);
      DEBUG_PRINT(("tid %ld woke up, event in mind %d\n", 
             pthread_self(),
             eq->dying));
      gettimeofday(&now, NULL);
    }
    if(eq->dying) goto bail;
    ev = (event)q_pop(eq->q);
    assert(pthread_mutex_unlock(&eq->mutex)==0);
    if(ev) {
      DEBUG_PRINT(("tid %ld executing callback\n", pthread_self()));
      if(ev->callback != NULL) {
        boolean retval = ev->callback(&now, &ev->tv, ev->user);
        if(retval && ev->restartintervalms != 0) {
          // reschedule interval away from scheduled time, not actual time.
          ev->tv.tv_sec  += ev->restartintervalms/1000;
          ev->tv.tv_usec += (ev->restartintervalms%1000)*1000;
          while(ev->tv.tv_usec > 1000000) {
            ev->tv.tv_usec -= 1000000;
            ev->tv.tv_sec ++;
          }

          assert(pthread_mutex_lock(&eq->mutex)==0);
          q_insert(eq->q, ev);
          // TODO: think about a cond_signal here...  
          // I think this thread shouldn't need one to make forward 
          // progress, though.
          assert(pthread_mutex_unlock(&eq->mutex)==0);
        } else {
          free(ev);
        }
      } else {
        printf("INTERNAL ERROR: no callback function for registered event\n");
        free(ev);
      }
    }
  }
 bail:
  assert(pthread_mutex_unlock(&eq->mutex)==0);
  DEBUG_PRINT(("tid %ld bailing\n", pthread_self()));
  return(NULL);
}

eventq ev_start(unsigned short min_workers, unsigned short max_workers) {
  unsigned short i;
  eventq eq = malloc(sizeof(struct event_queue_struct));
  assert(eq!=NULL);
  /* this memset is very important... somehow. */
  memset(eq, 0, sizeof(struct event_queue_struct));
  eq->q = q_new(compare_timeval, free);
  eq->min_workers = min_workers;
  eq->current_workers = min_workers;
  eq->max_workers = max_workers;
  eq->workers = (pthread_t *)malloc(sizeof(pthread_t)*max_workers);
  assert(eq->workers!=NULL);
  (void)pthread_mutex_init(&eq->mutex, NULL);
  assert(pthread_cond_init(&eq->cond, NULL)==0);
  for(i=0;i<min_workers;i++) {
    DEBUG_PRINT(("starting worker\n"));
    assert(pthread_create(&eq->workers[i], NULL, ev_worker_thread, eq)==0);
  }
  return(eq);
}

boolean ev_schedule(eventq eq, 
                    struct timeval *t, 
                    unsigned int restartintervalms, 
                    ev_callback callback,
                    void *user) {
  event e = (event)malloc(sizeof(struct event_struct));
  event f;
  assert(e!=NULL);
  e->restartintervalms  = restartintervalms;
  if(t != NULL) {
    e->tv.tv_sec = t->tv_sec;
    e->tv.tv_usec = t->tv_usec;
  } else {
    gettimeofday(&e->tv, NULL);
    e->tv.tv_sec +=  restartintervalms/1000;
    e->tv.tv_usec += (restartintervalms%1000)*1000;
    while(e->tv.tv_usec > 1000000) {
      e->tv.tv_usec -= 1000000;
      e->tv.tv_sec ++;
    }
  } 
  e->callback = callback;
  e->user = user;
  assert(pthread_mutex_lock(&eq->mutex)==0);
  f = (event)q_top(eq->q);
  q_insert(eq->q, e);
  if(f != q_top(eq->q)) {  
    // tells the service thread that a new first event has been
    // scheduled
    assert(pthread_mutex_unlock(&eq->mutex)==0);
    assert(pthread_cond_signal(&eq->cond)==0);
  } else {
    assert(pthread_mutex_unlock(&eq->mutex)==0);
  }
  return(TRUE); // forgot what I had in mind for the retval 
}

void ev_stop(/*@only@*/ eventq eq) {
  unsigned short i;
  eq->dying=TRUE;
  DEBUG_PRINT(("broadcast. \n"));
  assert(pthread_cond_broadcast(&eq->cond)==0);
  for(i=0;i<eq->current_workers;i++) 
    (void)pthread_join(eq->workers[i], NULL);
  free(eq->workers);
  (void)pthread_mutex_destroy(&eq->mutex);
  (void)pthread_cond_destroy(&eq->cond);
  q_delete(eq->q);
  eq->q=NULL;
  free(eq);
}

void ev_join(/*@only@*/ eventq eq) {
  assert(pthread_mutex_lock(&eq->mutex)==0);
  while(q_top(eq->q)) {
    assert(pthread_mutex_unlock(&eq->mutex)==0);
    assert(pthread_cond_signal(&eq->cond)==0); /* I'm waiting, dammit */
    (void)usleep(100);
    assert(pthread_mutex_lock(&eq->mutex)==0);
  }
  assert(pthread_mutex_unlock(&eq->mutex)==0);
  ev_stop(eq);
}
#endif
