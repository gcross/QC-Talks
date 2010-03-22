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

#ifndef _string_bindings_h
#define _string_bindings_h
#include "nscommon.h"
#include <stdio.h>

NS_BEGIN_DECLS

typedef struct string_bindings_st *string_bindings;

string_bindings 
strb_new(unsigned short initial_size);

void 
strb_delete(/*@only@*/string_bindings sb);

unsigned short 
strb_bind(string_bindings sb, /*@owned@*/ const char *st);

unsigned short 
strb_lookup(string_bindings sb, const char *st);

/*@dependent@*/
/*@null@*/
const char *
strb_resolve(const string_bindings sb, unsigned short idx);

/* TODO: make this take strcmp instead; might
* not be possible */
void 
strb_sort_and_rebind(string_bindings sb, 
                     int (*indirect_string_compare)(const char **a, 
                                           const char **b));

void strb_iterate(string_bindings sb,
                  void (*callback)(const char *, unsigned short, void *user),
                  void *user);
 
void 
strb_print(string_bindings sb, FILE *fpout);

unsigned short strb_count(string_bindings sb);
void strb_integrity_check(string_bindings sb);

NS_END_DECLS
#endif
