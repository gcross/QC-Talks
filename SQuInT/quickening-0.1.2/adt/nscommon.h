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

#ifndef _NS_COMMON_H
#define _NS_COMMON_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#ifdef __cplusplus
# define NS_BEGIN_DECLS extern "C" {
# define NS_END_DECLS }
#else
# define NS_BEGIN_DECLS /* empty */
# define NS_END_DECLS /* empty */
#endif

NS_BEGIN_DECLS

#ifdef __LCLINT__
#define UNUSED(x) /*@unused@*/ ##x
typedef unsigned char u_int8_t;
typedef unsigned short u_int16_t;
typedef	unsigned int u_int32_t;
typedef	unsigned long long u_int64_t;
#else
#define UNUSED(x) x __attribute__((unused))
#endif

#ifdef __LCLINT__
#define __STDC__
char *rindex(const char *s, char c); /* not really true; it's an int */
/*@dependent@*/char *index(const char *s, char c); /* not really true; it's an int */
char *strdup(const char *s);  /* probably a good annotation here */
/*@printflike@*/
int /*@alt void@*/ snprintf(/*@out@*/char *s, size_t x, const char *fmt, ...);
#endif

typedef unsigned char boolean;

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE (!FALSE)
#endif

#ifdef __LCLINT__
/* lclint doesn't do typeof */
#define min(x,y) ((x > y) ? x : y)
#define max(x,y) ((x > y) ? x : y)
#else
/* from linux/kernel.h */
#define min(x,y) ({ \
        const typeof(x) _xi = (x);       \
        const typeof(y) _yi = (y);       \
        (void) (&_xi == &_yi);            \
        _xi < _yi ? _xi : _yi; })
#define max(x,y) ({ \
        const typeof(x) _xa = (x);       \
        const typeof(y) _ya = (y);       \
        (void) (&_xa == &_ya);            \
        _xa > _ya ? _xa : _ya; })
#endif

#define DEBUG_PRINT(x...) do { } while (0)
// #define DEBUG_PRINT(x) fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, x);
#define DEBUG_PRINTL(x) fprintf(stderr, "%s:%d: %s", __FILE__, __LINE__, x);

#ifdef HAVE_STDINT_H
/* a linux-y thing */
#include <stdint.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
/* kludgy fix for solaris. cygwin suggests u_int64_t is more portable,
 solaris suggests the opposite. */
typedef u_int64_t macaddress;

#ifdef VERBOSE
#define VERBAL printf
#else
#define VERBAL(X...) do { if(want_debug) fprintf(stderr, X); } while(0);

#if defined(__linux__)  && !defined(__cplusplus)
/*@unused@*/
static __inline void nullie(/*@unused@*/ const char *fmt __attribute__ ((unused)), ...) {
  return;
}
#else
#define nullie(x...) do {} while(0)
#endif

#endif

#include<stdlib.h>
#include<stdio.h>
/*@unused@*/
static __inline /*@out@*/ void *malloc_ordie(size_t len) {
  void *ret = malloc(len);
  if(ret == NULL) { 
    fprintf(stderr, "unable to allocate %d bytes\n", (int)len);
    abort();
  }
  return(ret);
}

#include <string.h>
/*@unused@*/
static __inline char *strdup_ordie(const char *str) /*@*/ {
  char *ret = strdup(str);
  if(ret == NULL) { 
    fprintf(stderr, "unable to duplicate %s\n", str);
    abort();
  }
  return(ret);
}

#define NEWPTR(type, name) type *name = (type *)malloc_ordie(sizeof(type))
#define NEWPTRX(type, name) type *name = (type *)malloc_ordie(sizeof(type))
#define NEWPTRZ(type, name) type *name = (type *)memset(malloc_ordie(sizeof(type)), 0, sizeof(type))

/* means nscommon must be included after sys/time */
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

NS_END_DECLS

#endif
/*
vi:ts=2
*/
