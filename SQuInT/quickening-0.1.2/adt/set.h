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

#ifndef _set_h
#define _set_h

#include <string.h>
#include "nscommon.h"

struct set_struct;
struct set_element;
typedef struct set_struct *set;
typedef struct set_element *set_iterator;
typedef void *(*set_callback)(void *value, void *user);
typedef void *(*set_callback2)(void *value, void *user, void *user2);

set set_new(void (*free_value)(void *value), 
            int (*comparator)(const void *v1, const void *v2, size_t len), 
            size_t compare_len );
void set_delete(set deleteme);
void set_add(set s, void *value);
void *set_iterate(set s, set_callback cb, void *user);
void *set_iterate2(set s, set_callback2 cb, void *user, void *user2);
void set_remove(set s, void *value);
void set_destructive_iterate(set s, set_callback cb, void *user);

void *set_first(set s, set_iterator *si);
void *set_next(set_iterator *si);
set_iterator set_find(set s, const void *value);
unsigned int set_count(set s);
void *set_value_from_si(set_iterator si);

#endif
