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
#include <string.h>
#include "timeseries.h"
#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

ts new_timeseries(u_int64_t startingCount, 
                  unsigned long timeval) {
  ts retval =  
    (ts) malloc(sizeof(struct timeseries));
  assert(retval!=NULL);
  memset(retval, 0, sizeof(struct timeseries));
  retval -> currentCount = startingCount;
  retval -> circularIndex = 0;
  retval -> history[0].tv_sec_delta = 0;
  retval -> current_tv_sec = timeval;
  retval -> history[0].delta = 0;
  retval -> dumpfile = 0;
  return(retval);
}

void append_timeseries(ts t, u_int64_t newcount, 
                       unsigned long timeval) {
  t->circularIndex++;
  t->circularIndex%=TS_LEN;
  if(timeval > t->current_tv_sec) 
    t->history[t->circularIndex].tv_sec_delta = timeval - t->current_tv_sec;
  else 
    t->history[t->circularIndex].tv_sec_delta = 0;
  if(newcount > t->currentCount) {
     t->history[t->circularIndex].delta = 
       (unsigned long int)(newcount - t->currentCount);
  } else {
    t->history[t->circularIndex].delta = 0;
  }
  t->currentCount = newcount;
  t->current_tv_sec = timeval;
  if(t->dumpfile) {
    fprintf(t->dumpfile, "%lu %qu\n", timeval, newcount);
  }
}
  
void dump_timeseries(ts t, const char *filename) {
  t->dumpfile = fopen(filename, "w");
}

double getrate_timeseries(ts t) {
  /* crude, considering background */
  assert(t!=NULL);
  if(t->history[t->circularIndex].tv_sec_delta != 0) {
    
  return((double)t->history[t->circularIndex].delta/
    ((double)t->history[t->circularIndex].tv_sec_delta));
  } else {
    return(0.0);
  }
}

