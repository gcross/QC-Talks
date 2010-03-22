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

#include "nscommon.h"
#include "adt/eventqueue.h"
#include <stdio.h>
#include <stdlib.h>

boolean count_cb(struct timeval *now __attribute__((unused)), 
                 struct timeval *scheduled __attribute__((unused)), 
                 void *user) {
  unsigned int *pcounter = user;
  /* yeah, I know this isn't thread safe in an 
     explicitly mt program.  bite me. */
  (*pcounter) ++;
  printf("%d...\n", *pcounter);
  return(*pcounter <10);
}

#ifdef HAVE_LIBPTHREAD
static double deltat(struct timeval *now, struct timeval *then) {
  return  (
           (double)now->tv_sec - then->tv_sec
           + ((double)now->tv_usec / 100000.0)
           - ((double)then->tv_usec / 100000.0)
           );
}
#endif

int main(int argc __attribute__((unused)), 
         char *argv[] __attribute__((unused))) {
#ifdef HAVE_LIBPTHREAD
  eventq eq;
  int counter=0;
  struct timeval t1, t2;
  
  eq = ev_start(1,1);
  printf("counting to ten in one second.\n");
  gettimeofday(&t1, NULL);
  ev_schedule(eq, 0, 100, count_cb, &counter);
  ev_join(eq);
  gettimeofday(&t2, NULL);
  printf("took %5.3f seconds\n", deltat(&t2, &t1));
  
  counter = 0;
  eq = ev_start(1,1);
  printf("counting to ten in one second.\n");
  gettimeofday(&t1, NULL);
  ev_schedule(eq, 0, 100, count_cb, &counter);
  ev_join(eq);
  gettimeofday(&t2, NULL);
  printf("took %5.3f seconds\n", deltat(&t2, &t1));
  

  //counter = 0;
  //eq = ev_start(1,1);
  //printf("repeating.\n");
  //ev_schedule(eq, 0, 1000, count_cb, &counter);
  //sleep(15);
  //ev_stop(eq);

  counter = 0;
  eq = ev_start(3,3);
  printf("repeating by threes ( to twelve ).\n");
  ev_schedule(eq, 0, 999, count_cb, &counter);
  ev_schedule(eq, 0, 999, count_cb, &counter);
  ev_schedule(eq, 0, 999, count_cb, &counter);
  sleep(5);
  ev_join(eq);

  counter = 0;
  eq = ev_start(3,3);
  printf("repeating by five ( overprovisioned, to 14. ).\n");
  ev_schedule(eq, 0, 999, count_cb, &counter);
  ev_schedule(eq, 0, 999, count_cb, &counter);
  ev_schedule(eq, 0, 999, count_cb, &counter);
  ev_schedule(eq, 0, 999, count_cb, &counter);
  ev_schedule(eq, 0, 999, count_cb, &counter);
  ev_join(eq);

#endif
  exit(0);
}
