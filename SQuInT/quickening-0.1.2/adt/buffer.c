/*
 * Copyright (c) 2002
 * Neil Spring and the University of Washington.
 * All rights reserved. 
 * Copyright (c) 1999 
 * Eric Hoffman
 * 
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
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <assert.h>
// #include "timer.h"
typedef unsigned int when;
#ifndef FISHTIME_HZ
#define FISHTIME_HZ ((when)1000)
#endif

/* open */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
/* end open */

#ifdef WIN32
#include "win32.h"
#else
#include <netinet/in.h> 
#endif
#include "buffer.h"

// 
// unsigned int ipv4_addr_from_string(const char *x)
// {
//   int a,b,c,d;
//   unsigned int t;
// 
//   a=b=c=d=0;
//   if (sscanf(x,"%d.%d.%d.%d",&a,&b,&c,&d)>0){
//     return(htonl(a<<24|b<<16|c<<8|d));
//   } else {
//     sscanf(x,"x%x",&t);
//     return(htonl(t));
//   }
// }

char *buffer_contents(buffer b)
{
  return(b->contents);
}

#define is_digit(c) ((char_to_digit(c) >= 0) && (char_to_digit(c) <= 9))
#define char_to_digit(c) (int)((c) - '0')

static char hex_digits[]="0123456789abcdef";

void buffer_concat(buffer b,buffer k)
{
  while ((b->length-b->fill)< k->fill)
    b->contents=(char *)realloc(b->contents,b->length+=k->fill+2);

  memcpy(b->contents+b->fill,k->contents,k->fill);
  b->contents[b->fill+=k->fill]=0;
}

void buffer_append(buffer b,const char *item,int len)
{
  while ((b->length-b->fill)< (len+1))
    b->contents=(char *)realloc(b->contents,b->length*=2);

  memcpy(b->contents+b->fill,item,len);
  b->contents[b->fill+=len]=0;
}

void vbprintf(buffer b,const char *fmt, va_list ap)
{
  const char *fc;     /* pointer to current format character */
  int qf;       /* flag to quit reading % format */ 
  int min_len, len;
  char *c;
  char type;
  unsigned int x,a1;
  int i, j,base, negative;
  int rfill;
  char reverse[20];

  for (fc=fmt;*fc;fc++){  
    if (*fc != '%') {
      buffer_append(b,fc,1);
    } else{ 
      min_len=0;
      qf=0;
      fc++;

      if (*fc=='0') { 
	fc++;
	while (is_digit(*fc) && *fc)
	  min_len=10*min_len + char_to_digit(*fc++);
      }
      
      type=*fc;
      base =0;
      negative=0;
      switch (type) {
        
      case '%':
        buffer_append(b,"%",1);
        break;

      case 't':
	buffer_append(b,reverse,
		      sprintf (reverse,"%f",
			       (float)(va_arg(ap,when))/(float)FISHTIME_HZ));
	break;

      case 'b':
	buffer_concat(b,va_arg(ap, buffer));
	break;

      case 'a':
        a1 = va_arg(ap, unsigned int);
	a1=htonl(a1);
	bprintf(b,"%d.",(a1>>24)&255);
	bprintf(b,"%d.",(a1>>16)&255);
	bprintf(b,"%d.",(a1>>8)&255);
	bprintf(b,"%d",a1&255);
	break;

      case 's':
        c = va_arg(ap, char *);
        if (!c) c= (char *)"(null)";
        len=strlen(c);
        for (i=0;i<min_len-len;i++)
            buffer_append(b," ",1);
        buffer_append(b,c,len);
        break;
        
      case 'x':
        base=16;
      case 'o':
        if (!base) base=8;
      case 'u':
        if (!base) base=10;
      case 'd':     case 'i':
        x = va_arg(ap, unsigned int );
        if (!base){
          base=10;
          if ((negative=((int)x <0))){
            buffer_append(b,"-",1); 
            x=(unsigned)(-1*(int)x);
          }
        }
        if (x){
          for ( i=0,rfill=19; i<12 && x; i++, x/=base) 
	    reverse[rfill--]=hex_digits[x%base];
	  for (j=i;j<min_len;j++) buffer_append(b,"0",1);
	  buffer_append(b,reverse+rfill+1,i);
        } else {
          buffer_append(b,"0",1);
          i=1;
        }
        break;
      default:
        break;
      }
    }
  }
}

void bprintf(buffer b,const char *fmt, ...)
{
  va_list ap;
  va_start(ap,fmt);
  vbprintf(b,fmt,ap);
  va_end(ap);
}

buffer create_buffer(int len)
{
  buffer b=(buffer)malloc(sizeof(struct buffer_st));
  assert(b!=NULL);
  b->contents=(char *)malloc(b->length=len);
  b->fill=0;
  b->contents[0]=0;
  return(b);
}

void free_buffer(buffer b)
{
  free(b->contents);
  free(b);
}


int writef(int (*f)(void *, char *, int),void *a,const char *format,...)
{
  buffer b=create_buffer(10);
  int result;
  va_list ap;
  va_start(ap,format);
  vbprintf(b,format,ap);
  result=(*f)(a,b->contents,b->fill);
  va_end(ap);
  free_buffer(b);
	 
  return(0);
}

int openf(int flags,char *format,...)
{
  buffer b=create_buffer(10);
  int result;
  va_list ap;
  va_start(ap,format);
  vbprintf(b,format,ap);
  result=open(b->contents,flags,0666);
  va_end(ap);
  free_buffer(b);

	return(0);
}
