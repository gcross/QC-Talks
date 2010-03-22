/* Copyright (c) 2002 Neil Spring and the University of Washington. *
 * Distributed under the terms of the GNU General Public License,   *
 * version 2.  See the file COPYING for more details.               *
 * This software comes with NO warranty.                            *
 * end license */
#include <sys/types.h>
#include <sys/time.h>
#include "nscommon.h"

struct event;
struct event *fish_scheduleevent(unsigned int msec_delay,
                                 void (*event_handler)(void *),
                                 void *event_handler_argument,
                                 boolean abort_if_duplicate);
/* get close, but don't call gettimeofday.  don't call if not in
   a timer handler and _scheduleevent wasn't just called. */
struct event *fish_scheduleevent_coarse(unsigned int msec_delay,
                                 void (*event_handler)(void *),
                                 void *event_handler_argument,
                                 boolean abort_if_duplicate);

void fish_executepending(void);

boolean fish_time_to_next_event(/*@out@*/ struct timeval *delta_t);
unsigned int fish_pending(void);

void *fish_cancelevent_by_sig(void (*event_handler)(void *), void *argument);
void *fish_cancelevent(/*@only@*/struct event *event_handle);
void fish_event_dump(int sock);

/*@dependent@*/
struct event *fish_scheduleevent_absolute(struct timeval *event_time, 
                                          void (*event_handler)(/*@only@*/ void *),
                                          /*@owned@*/void *event_handler_argument,
                                          boolean abort_if_duplicate);

/* not really intended for use, but handy for quick fixes */
/*@null@*/
struct event *event_find_argument(void (*event_handler)(/*@only@*/ void *),
                                  /*@dependent@*/void * argument);
