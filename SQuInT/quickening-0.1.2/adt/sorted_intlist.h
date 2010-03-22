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

#ifndef _sorted_intlist_h
#define _sorted_intlist_h
#include <stdio.h>
#include "nscommon.h"

NS_BEGIN_DECLS

typedef struct sorted_intlist_struct *sorted_intlist;

/*@only@*/ sorted_intlist si_new(void);
void si_delete(/*@only@*/ sorted_intlist si);

void si_insert(sorted_intlist si, int to_insert);
void si_remove(sorted_intlist si, int to_remove);

/* when you know it's greater */
void si_append(sorted_intlist si, int to_append); 

/* si_iterate returns TRUE if it reached the end, FALSE if
   interrupted by iterator returning FALSE */
boolean si_iterate(sorted_intlist si, 
                   boolean (*iterator)(int value, void *user), void *user);

unsigned long si_length(sorted_intlist si);

boolean si_member(sorted_intlist si, int value);

void si_print(sorted_intlist si, FILE *fpout);
NS_END_DECLS
#endif
