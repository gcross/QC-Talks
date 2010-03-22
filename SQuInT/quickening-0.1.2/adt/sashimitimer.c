/* Copyright (c) 2002 Neil Spring and the University of Washington. *
 * Distributed under the terms of the GNU General Public License,   *
 * version 2.  See the file COPYING for more details.               *
 * This software comes with NO warranty.                            *
 * end license */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <sys/time.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "typed_queue.h"
#include "nscommon.h"
#include "socket_printf.h"
#include "log.h"

#ifndef timersub
# define timersub(a, b, result)                   \
do {                                              \
  (result)->tv_sec = (a)->tv_sec - (b)->tv_sec;   \
  (result)->tv_usec = (a)->tv_usec - (b)->tv_usec;\
  if ((result)->tv_usec < 0) {                    \
    --(result)->tv_sec;                           \
    (result)->tv_usec += 1000000;                 \
  }                                               \
} while (0)
#endif

struct event {
  struct timeval when;
  void (*event_handler)(/*@only@*/ void *);
  /*@only@*/ void *event_handler_argument;
};

static struct timeval Now; /* cached value of current time for coarse or quick scheduling */

DECLARE_TYPED_QUEUE(struct event, ev);

/*@null@*/ ev_queue events;

int event_compare(const struct event *v1,
                  const struct event *v2) {
  if(v1->when.tv_sec < v2->when.tv_sec)    return -1;
  if(v1->when.tv_sec > v2->when.tv_sec)    return 1;
  if(v1->when.tv_usec < v2->when.tv_usec)  return -1;
  if(v1->when.tv_usec > v2->when.tv_usec)  return 1;
  return 0;
}

void insertevent(/*@owned@*/struct event *insertme);
/*@null@*//*@dependent@*/
struct event *event_find(/*@out@*/struct event *needle);

static void normalize_timeval ( struct timeval * when ) {  
  while(when->tv_usec > 1000000) {
    when->tv_usec -= 1000000;
    when->tv_sec += 1;
  }
  /* not sure if the next four are necessary, but it seems
     possible to overflow usec if now is really bad.... */
  while(when->tv_usec < 0) {
    when->tv_usec += 1000000;
    when->tv_sec -= 1;
  }
}
/* not really intendent for student consumption, but can be useful when
   fish_scheduleevent is inflexible */
/*@dependent@*/
struct event *fish_scheduleevent_absolute(struct timeval *event_time, 
                                          void (*event_handler)(/*@only@*/ void *),
                                          /*@owned@*/void *event_handler_argument,
                                          boolean abort_if_duplicate) {
  NEWPTR(struct event, newevent);
  if(events == NULL) events = ev_q_new(event_compare, free);
  assert(newevent != NULL);

  newevent->when.tv_sec = event_time->tv_sec;
  newevent->when.tv_usec = event_time->tv_usec;
  normalize_timeval(&newevent->when);
  newevent->event_handler = event_handler;
  newevent->event_handler_argument = event_handler_argument;

  if(abort_if_duplicate && event_find(newevent) != NULL) {
    log_print(LOG_ERR, "(bug) Scheduled duplicate event\n");
    abort();
  }
  insertevent(newevent);
  return(newevent);
}

void fish_update_now() {
  (void)gettimeofday(&Now, NULL);
}

/*@dependent@*/
extern struct event *fish_scheduleevent_coarse(unsigned int msec_delay,
                                 void (*event_handler)(/*@only@*/ void *),
                                 /*@owned@*/void *event_handler_argument,
                                 boolean abort_if_duplicate) {
  struct timeval when;

  fish_update_now();
  
  when = Now;

  when.tv_sec += (msec_delay / 1000);
  when.tv_usec += ((msec_delay % 1000) * 1000);

  return( fish_scheduleevent_absolute(&when, event_handler, 
                                      event_handler_argument, abort_if_duplicate) );
}


/* student-called routines */
/*@dependent@*/
extern struct event *fish_scheduleevent(unsigned int msec_delay,
                                 void (*event_handler)(/*@only@*/ void *),
                                 /*@owned@*/void *event_handler_argument,
                                 boolean abort_if_duplicate) {
  fish_update_now();
  return fish_scheduleevent_coarse(msec_delay, event_handler, event_handler_argument, abort_if_duplicate);
}


boolean event_isduplicate(const struct event *iterate, void *user) {
  const struct event **e = (const struct event **)user;
  if(iterate->event_handler == (*e)->event_handler &&
     iterate->event_handler_argument == (*e)->event_handler_argument) {
    *e = iterate;
    return FALSE;
  } else {
    return TRUE;
  }
}


/* needle as an input parameter is partially defined */
/*@null@*//*@dependent@*/
struct event *event_find(/*@out@*/ struct event *needle) {
  if(events) {
    /* if iteration was interrupted by finding the needle */
    if(!ev_q_iterate(events, event_isduplicate, &needle))
      return needle;
  }
  return NULL;
}


void insertevent(/*@owned@*/struct event *insertme) {
  assert(events != NULL);
  //log_print(LOG_INFO, "scheduled event for %lu:%06lu\n", insertme->when.tv_sec, insertme->when.tv_usec);
  ev_q_insert(events, insertme);
}

/*@only@*/ void *removeevent(/*@only@*/ struct event *removeme) {
  void *retval;
  assert(removeme != NULL);
  assert(events != NULL);
  retval = removeme->event_handler_argument;
  ev_q_remove(events, removeme);
  free(removeme);
  return(retval);
}

/* returns the event_handler_argument associated
   with the event; you may free the data (unlikely)
   or ignore the return value (more likely).  NULL
   is returned if the event was not found */
void *fish_cancelevent(/*@only@*/ struct event *event_handle) {
  return(removeevent(event_handle));
}

boolean event_same_argument(const struct event *iterate, void *user) {
  const struct event **e = (const struct event **)user;
  if(iterate->event_handler_argument == (*e)->event_handler_argument) {
    *e = iterate;
    return FALSE;
  } else {
    return TRUE;
  }
}


/*@null@*/
struct event *event_find_argument(void (*event_handler)(/*@only@*/ void *),
                                  /*@dependent@*/void * argument) {
  struct event arg_holder;
  struct event *needle = &arg_holder;
  arg_holder.event_handler = event_handler;
  arg_holder.event_handler_argument = argument;
  return(event_find(needle));
}

void *fish_cancelevent_by_sig(void (*event_handler)(void *), /*@dependent@*/ void *argument) {
  struct event *e = event_find_argument(event_handler, argument);
  if(e != NULL) {
    return(removeevent(e));
  } else {
    return NULL;
  }
}

void fish_executepending(void) {
  struct event *p;
  boolean did_something;
  do { /* loop tightly to execute pending events, so long as we ran something */
    did_something = FALSE;
    fish_update_now();
    while((events != NULL) &&
          ((p = ev_q_top(events)) != NULL) &&
          (timercmp(&(p->when), &Now, <))) {
      p=ev_q_pop(events);
      if(p != NULL /* this is just to assure splint */) {
        //log_print(LOG_INFO, "exec %lu:%06lu at %lu.%06lu\n", 
        //          p->when.tv_sec, p->when.tv_usec,
        //          now.tv_sec, now.tv_usec);
        (p->event_handler)(p->event_handler_argument);
        free(p);
        did_something = TRUE;
      }
    }
  } while(did_something);
}

/* returns whether timeout should be set at all. */
boolean fish_time_to_next_event(struct timeval *delta_t) {
  struct event *top;

  fish_update_now();
  if(events!=NULL && (top = ev_q_top(events)) != NULL) {
    if( timercmp(&top->when, &Now, <) ) {
      /* it's too late. return 0 time. */
      delta_t->tv_sec = 0;
      delta_t->tv_usec = 0;
    } else {
      /* defined in sys/time.h */
      timersub(&top->when, &Now, delta_t);
      if(delta_t->tv_sec < 0 || delta_t->tv_usec <0) {
        /* this appears possible... somehow printf("WARNING: detected negative time a little late!\n"); */
        delta_t->tv_sec = max((long)delta_t->tv_sec, (long)0);
        delta_t->tv_usec = max((long)0, (long)delta_t->tv_usec);
      }
    }
    return TRUE;
  } else {
    /* timeout should not be set at all, but just in case... */
    delta_t->tv_sec = -1;
    delta_t->tv_usec = -1;
    return FALSE;
  }
}

/* necessary for a full interface, but not for student use */
void *fish_eventdata(/*@notnull@*/ struct event *event_handle) {
  return ( event_handle -> event_handler_argument );
}

unsigned int fish_pending(void) {
  if(events) {
    return((unsigned int)ev_q_length(events));
  } else {
    return(0);
  }
}

struct event_dump_args {
  int sock;
  struct timeval now;
};

static boolean
event_dump(const struct event *iterate, void *user) {
  struct event_dump_args *peda = (struct event_dump_args *)user;
  struct timeval diff;
  diff.tv_sec = iterate->when.tv_sec - peda->now.tv_sec;
  diff.tv_usec = iterate->when.tv_usec - peda->now.tv_usec;
  while(diff.tv_usec < 0) {
    diff.tv_usec += 1000000;
    diff.tv_sec -= 1;
  }
  (void)socket_printf(peda->sock, "@now + %lu.%06lu %p(%p)\n",
                      (unsigned long)diff.tv_sec, (unsigned long)diff.tv_usec,
                      iterate->event_handler, iterate->event_handler_argument);
  return TRUE;
}

void fish_event_dump(int sock) {
  struct event_dump_args eda;
  fish_update_now();
  eda.now = Now;
  eda.sock=sock;
  (void)socket_printf(sock, "event queue:\n");
  if(events != NULL)
    (void)ev_q_iterate(events, event_dump, (void *)&eda);
}
