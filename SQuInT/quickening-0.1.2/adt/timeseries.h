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
#ifdef HAVE_STDINT_H
#include <stdint.h> 
#endif
#include "nscommon.h"
#include <stdio.h> 
/* timeseries representation of a normally monotonically 
   increasing counter. */

struct timeseriesEntry {
  unsigned long tv_sec_delta; // time measurement was taken
  unsigned long delta; // delta from previous value.
};

#define TS_LEN 100

typedef struct timeseries {
  struct timeseriesEntry history[TS_LEN];
  u_int64_t currentCount;
  unsigned long current_tv_sec;
  int circularIndex; // mod 100.
  /*@null@*/ FILE *dumpfile;
} *ts;

ts new_timeseries(u_int64_t startingCount, unsigned long timeval);
void append_timeseries(ts t, u_int64_t newcount, unsigned long timeval);
void dump_timeseries(ts t, const char *filename);
double getrate_timeseries(ts t);
  


  

