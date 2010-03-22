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

#ifndef _typed_queue_h
#define _typed_queue_h
#include "queue.h"

#define DECLARE_TYPED_QUEUE(type, pr)                               \
struct pr##queue_struct;                                            \
typedef struct pr##queue_struct *pr##_queue;                        \
/*@unused@*/                                                        \
static __inline void                                                \
pr##_q_delete(/*@only@*/ pr##_queue d) {                            \
  q_delete((queue)d);                                               \
}                                                                   \
                                                                    \
/*@unused@*/                                                        \
static __inline pr##_queue                                          \
pr##_q_new(/*@null@*/ int (*cmp)(const type *v1,                    \
                      const type *v2),                              \
           /*@null@*/ void (*relse)(void *)) {                      \
  return((pr##_queue)q_new((q_comparator)cmp, relse));              \
}                                                                   \
                                                                    \
/*@unused@*/                                                        \
static __inline void                                                \
pr##_q_insert(pr##_queue h, /*@owned@*/ type *k) {                  \
  q_insert((queue)h, k);                                            \
}                                                                   \
/*@unused@*/                                                        \
static __inline void                                                \
pr##_q_append(pr##_queue h, /*@owned@*/ type *k) {                  \
  q_append((queue)h, k);                                            \
}                                                                   \
/*@unused@*/                                                        \
static __inline void                                                \
pr##_q_remove(pr##_queue h, type *k) {                              \
  q_remove((queue)h, k);                                            \
}                                                                   \
/*@unused@*/                                                        \
static __inline /*@null@*/ type *                                   \
pr##_q_pop(pr##_queue h) {                                          \
  return((type *)q_pop((queue)h));                                  \
}                                                                   \
/*@unused@*/                                                        \
static __inline /*@null@*/ /*@dependent@*/ type *                   \
pr##_q_top(pr##_queue h) {                                          \
  return((type *)q_top((queue)h));                                  \
}                                                                   \
/*@unused@*/                                                        \
static __inline boolean                                             \
pr##_q_iterate(pr##_queue h,                                        \
                boolean (*cb)(const type *keyval, void *user),      \
                void *user) {                                       \
  return(q_iterate((queue)h,(q_iterator)cb,user));                  \
}                                                                   \
/*@unused@*/                                                        \
static __inline boolean                                             \
pr##_q_iterate_nonconst(pr##_queue h,                               \
                boolean (*cb)(type *keyval, void *user),            \
                void *user) {                                       \
  return(q_iterate((queue)h,(q_iterator)cb,user));                  \
}                                                                   \
/*@unused@*/                                                        \
static __inline void *                                              \
pr##_q_find(pr##_queue q, const type *findme) {                     \
  return(q_find((queue)q,(const void *)findme));                    \
}                                                                   \
/*@unused@*/                                                        \
static __inline unsigned long                                       \
pr##_q_length(pr##_queue h) {                                       \
  return(q_length((queue)h));                                       \
}                                                                   
#endif
