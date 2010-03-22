/*
 * Copyright (c) 2002
 * Neil Spring and the University of Washington.
 * Copyright (c) 1999 
 * Eric Hoffman
 * 
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

#ifndef _buffer_h
#define _buffer_h
#include "nscommon.h"


NS_BEGIN_DECLS

#include <stdarg.h>

typedef struct buffer_st {
  int length;
  int fill;
  char *contents;
} *buffer;

/*@notnull@*/ /*@only@*/
buffer create_buffer(int len);
void free_buffer(/*@null@*/ /*@out@*/ /*@only@*/buffer b)  /*@modifies b@*/;

void vbprintf(buffer b,const char *fmt, va_list ap);
void bprintf(buffer b,const char *fmt, ...);
void buffer_append(buffer b,const char *item,int len);
int writef(int (*f)(void *, char *, int),void *a,const char *format,...);
unsigned int ipv4_addr_from_string(const char *x);

NS_END_DECLS
#endif
