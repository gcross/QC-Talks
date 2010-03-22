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

#ifndef E_TABLE
#define E_TABLE

#include "nscommon.h"
NS_BEGIN_DECLS

typedef struct table_entry_st {
  unsigned int key;
  void *value;
  struct table_entry_st *next;
} *table_entry;


typedef struct table_st {
  int size;
  int number_of_entries;
  table_entry *entries;
  /* returns true / 1 if match. */
  int (*compare_function)(const void *, const void *);
  unsigned int (*key_function)(const void *);
} *table;


unsigned int key_from_int(const void *int_i);
unsigned int key_from_string(const void * char_stars);
table create_table (int (*compare_function)(const void *, const void *),
		    unsigned int (*key_function)(const void *));
/*@dependent@*/void *table_find (const table t, const void *comparator);
void *random_table_entry(table t);
void table_insert (table t, /*@owned@*/void *value, /*@dependent@*/const void *comparator);
void table_remove (table t, void *comparator);
void iterate_table_entries(table t, 
                           void (*handler)(void *arg, void* value), 
                           void *arg);
void destroy_table(table t,void (*thunk)(void *value));

NS_END_DECLS
#endif
