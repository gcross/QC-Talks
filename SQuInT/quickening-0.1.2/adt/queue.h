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

#ifndef _queue_h
#define _queue_h
#include "nscommon.h"
typedef struct queue_struct *queue;

/* q_comparator returns like strcmp. */
typedef int (*q_comparator)(const void *v1, const void *v2);
#define q_strcmp ((q_comparator)strcmp)
/*@only@*/ 
queue q_new(/*@null@*/ q_comparator compare, 
            /*@null@*/ void (*release)(void *v));
void q_delete(/*@only@*/ queue q);
void q_insert(queue q, /*@only@*/ const void *v);
void q_append(queue q, /*@only@*/ const void *v);
/* removeme is a pointer to the element in the queue, not
   something that compares equal. */
void q_remove(queue q, void * removeme);
/*@null@*/ /*@only@*/ const void *q_pop(queue q);
/*@null@*/ /*@dependent@*/ const void *q_top(const queue q);

/* returns TRUE if it wishes to go on */
typedef boolean (*q_iterator_q)(queue q, const void *value, void *user);
typedef boolean (*q_iterator)(const void *value, void *user);
/* q_iterate returns TRUE if it reached the end, FALSE if
   interrupted by iterator returning FALSE */
boolean q_iterate(queue q, 
                  q_iterator iterator,
                  /*@out@*/ void *user);
boolean q_iterate_q(queue q, 
                  q_iterator_q iterator,
                  /*@out@*/ void *user);

unsigned long q_length(queue q);
void *q_find(queue q, const void *findme);
#endif
