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
#include<float.h>
#include<assert.h>

#include"nscommon.h"

/* gettimeofday */
#include <sys/time.h>
#include <unistd.h>

#include <string.h>

#include"progress.h"

/* snprintf */
#define _GNU_SOURCE
#include <stdio.h>
#ifdef HAVE_DMALLOC_H
#include<dmalloc.h>
#endif
#ifndef HAVE_SNPRINTF
int snprintf ( char *str, size_t n, const char *format, ... );
#endif

__inline static float diffTime(struct timeval bigger, struct timeval smaller) {
  long sec = bigger.tv_sec - smaller.tv_sec;
  long usec = bigger.tv_usec - smaller.tv_usec;
  return((float)sec + (float)usec/1000000.0);
}


static unsigned int next = 1;
static float fnext;
static char barstore[] =  
"==============================================================================";
static char fmtstring[60];
static const char *label =""; 
static boolean bAnnotate = FALSE;
static boolean bEnabled = TRUE;
static unsigned int spoonum;
static char ratestring[20];
static char timestring[20];

static void progress_timestring(float rate);
#define iBAR_WIDTHdefault 51
#define bBAR_WIDTHdefault 51
#define fBAR_WIDTHdefault ((float)iBAR_WIDTH)
#define bfBAR_WIDTHdefault ((float)bBAR_WIDTH)
 
static unsigned int iBAR_WIDTH = iBAR_WIDTHdefault;
static unsigned int bBAR_WIDTH = bBAR_WIDTHdefault;
static float fBAR_WIDTH = ((float)iBAR_WIDTHdefault);
static float bfBAR_WIDTH = ((float)bBAR_WIDTHdefault);

void progress_reset()  {
  next = 0;
  fnext = 0.0;
}
void progress_disable()  {
  bEnabled = FALSE;
}

void progress(const float f) {
  // fprintf(stderr,"%f\n",f);
  if(!bEnabled) return;
  if(f < FLT_EPSILON) {
    fprintf(stderr, "\r>");
    next = 1;
  } else {
    if(f >= ((float)(next))/fBAR_WIDTH || f >= (float)1.0) {
      next = (unsigned int)(f*fBAR_WIDTH)+1;
      if(bAnnotate) {
        snprintf(fmtstring, sizeof(fmtstring), "\r%s:%3.0f%%%% %%-%ds| %u", label, f*100.0, iBAR_WIDTH, spoonum);
      }
      else 
        snprintf(fmtstring, sizeof(fmtstring), "\r%s:%3.0f%%%% %%-%ds|", label, f*100.0, iBAR_WIDTH);
      if(next > 1 )barstore[next-2]='>';
      barstore[next-1]='\0';
      fprintf(stderr, fmtstring, barstore);
      //fprintf(stderr, "\r%s>", barstore);
      barstore[next-1]='=';
      if(next > 1 )barstore[next-2]='=';
    }
    if(f >= (float)1.0-FLT_EPSILON){
      fprintf(stderr,"\n");
      bAnnotate = FALSE;
    }
  }
}

void progress_label(const char *label_param) {
  if(strlen(label_param) < bBAR_WIDTHdefault - 5) {
    label = label_param;
    bBAR_WIDTH = min(1, (int) bBAR_WIDTHdefault - (int)strlen(label_param));
    iBAR_WIDTH = min(1, (int) iBAR_WIDTHdefault - (int)strlen(label_param));
  }
  fBAR_WIDTH = (float)iBAR_WIDTH;
  bfBAR_WIDTH = (float)bBAR_WIDTH;
}
void progress_annotate(unsigned int spoo) {
  bAnnotate = TRUE;
  spoonum = spoo;
}

static struct timeval start;

void progress_bytes_of_bytes(unsigned long int bytes, 
			     unsigned long int total ) {
  struct timeval now;
  if(bytes > total) return;
  if(!bEnabled) return;
  if(isatty(fileno(stderr)) == 0) return;
  if(bytes == 0) {
    gettimeofday(&start, NULL);
    fprintf(stderr, "\r>");
    fnext = 0.01;
  } else {
    float f = (float)bytes/(float)total;
    if(f >= fnext || bytes >= total) {
      float dt;
      unsigned int thisint = (unsigned int)(f*bfBAR_WIDTH);
      fnext = f+0.01;
      gettimeofday(&now, NULL);
      dt = diffTime(now,start);
      progress_ratestring((double)bytes / (double)dt, "B");
      progress_timestring(dt*((float)total-bytes)/((float)bytes));
      snprintf(fmtstring, sizeof(fmtstring), "\r%s:%3.0f%%%% %%-%ds| %9s eta %6s", 
	       label, f*100.0, bBAR_WIDTH, ratestring, timestring);
      if(thisint > 0 )barstore[thisint-1]='>';
      barstore[thisint]='\0';
      fprintf(stderr, fmtstring, barstore);
      //fprintf(stderr, "\r%s>", barstore);
      barstore[thisint]='=';
      if(thisint > 0 )barstore[thisint-1]='=';
    }
  }
  if(bytes == total){
    float dt = diffTime(now,start);
    fprintf(stderr,"\n");
    if(dt > 2) {
      progress_timestring(dt);
      fprintf(stderr, "%s process rate: %s for %s\n", label, ratestring, timestring);
    }
  }
}

void progress_n_of(unsigned int n, 
                   unsigned int total ) {
  struct timeval now;
  float f;
  assert( total != 0  );
  f = (float)n/(float)total;
  if(n > total) return;
  if(!bEnabled) return;
  if(isatty(fileno(stderr)) == 0) return;
  if(n == 0) {
    gettimeofday(&start, NULL);
    fprintf(stderr, "\r>");
    fnext = 0;
    progress_ratestring(0, "");
    progress_timestring(0);
  } 
  if(f >= fnext || n >= total) {
    float dt;
    unsigned int thisint = (unsigned int)(f*bfBAR_WIDTH);
    fnext = f+0.01;
    if(n != 0) {
      gettimeofday(&now, NULL);
      dt = diffTime(now,start);
      progress_ratestring((double)n / (double)dt, "");
      progress_timestring(dt*((float)total-n)/((float)n));
    }
    snprintf(fmtstring, sizeof(fmtstring), "\r%s:%3.0f%%%% %%-%ds| %9s eta %6s", 
             label, f*100.0, bBAR_WIDTH, ratestring, timestring);
    if(thisint != 0 ) barstore[thisint-1]='>';
    barstore[thisint]='\0';
    fprintf(stderr, fmtstring, barstore);
    //fprintf(stderr, "\r%s>", barstore);
    barstore[thisint]='=';
    if(thisint > 0 )barstore[thisint-1]='=';
  }
  if(n == total){
    float dt = diffTime(now,start);
    fprintf(stderr,"\n");
    if(dt > 2) {
      progress_timestring(dt);
      fprintf(stderr, "%s process rate: %s for %s\n", label, ratestring, timestring);
    }
  }
}

/* technique stolen from ncftp. */
#define KByte 1024u
#define MByte (1024u*KByte)
#define GByte (1024u*MByte)
/* haha */


static void progress_timestring(float timeInSeconds) {
  unsigned int hours=0;
  unsigned int minutes=0;
  unsigned int seconds=0;
  unsigned int deciseconds=0;
  hours = ((unsigned int)timeInSeconds) / 3600;
  timeInSeconds -= hours*3600;
  minutes = ((unsigned int)timeInSeconds) / 60;
  timeInSeconds -= minutes*60;
  seconds = ((unsigned int)timeInSeconds);
  timeInSeconds-=seconds;
  deciseconds = ((unsigned int)(timeInSeconds*10.0))%10;
  if(hours >0) {
    snprintf(timestring, sizeof(timestring), "%uh%02um", hours,minutes);
  } else if(minutes >0) {
    snprintf(timestring, sizeof(timestring), "%um%02us", minutes,seconds);
  } else {
    snprintf(timestring, sizeof(timestring), "%u.%01us", seconds, deciseconds);
  }
}

const char * progress_ratestring(float rate, const char *units) {
  char suff;
  if(rate > 999.5 * GByte) {
    suff = 'T';
    rate = rate / GByte / 1024.0;
  } else if(rate > 999.5 * MByte) {
    suff = 'G';
    rate /= GByte;
  } else if(rate > 999.5 * KByte) {
    suff = 'M';
    rate /= MByte;
    snprintf(ratestring, sizeof(ratestring), 
	     "%.2fM", (float)rate / (float)MByte);
  } else if(rate >= 999.5 ) {
    suff = 'K';
    rate /= KByte;;
  } else {
    suff = ' ';
    snprintf(ratestring, sizeof(ratestring), 
	     "%3.0f%s/s", rate, units);
  }
  if(rate >= 99.5)  { 
    snprintf(ratestring, sizeof(ratestring), 
	     "%3.0f%c%s/s", rate, suff, units);
  } else if(rate >= 9.5) {
    snprintf(ratestring, sizeof(ratestring), 
	     "%4.1f%c%s/s", rate, suff, units);
  } else {
    snprintf(ratestring, sizeof(ratestring), 
	     "%4.2f%c%s/s", rate, suff, units);
  }
  return(ratestring);
}

/*
vi:ts=2
*/
