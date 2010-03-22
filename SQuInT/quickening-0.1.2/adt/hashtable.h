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

#ifndef _hashtable_h
#define _hashtable_h

#include "nscommon.h"

NS_BEGIN_DECLS

struct hashtable_struct;
typedef struct hashtable_struct *hashtable;

void ht_delete(/*@only@*/ hashtable deleteme, boolean show_progress);

typedef unsigned int (*hash_cb)(const void *key);
typedef boolean (*isequal_cb)(const void *key1, const void *key2);
typedef void (*delete_cb)(void *key);
hashtable ht_new(unsigned int size, 
                 hash_cb hash,
                 isequal_cb isequal,
                 /*@null@*/ delete_cb free_keyval);
void ht_insert(hashtable ht, /*@owned@*/const void *keyval);
/* true if found and removed */
boolean ht_remove(hashtable ht, const void *key);
typedef boolean (*ht_delete_if_cb)(void *keyval, void *user);
unsigned int ht_delete_if(hashtable ht,
                          ht_delete_if_cb callback, 
                          /*@null@*/ void *user);
void ht_free_entry(hashtable ht, const void *keyval);
/*@dependent@*/ /*@null@*/ 
void *ht_lookup(hashtable ht, const void *key);

typedef void * (*ht_constructor_cb)(const void *key);
/*@dependent@*/ 
void *ht_lookup_nofail(hashtable ht, 
                       const void *key,
                       ht_constructor_cb constructor);

/* returns true if it should continue to be called */
typedef boolean (*ht_iterator_cb)(const void *keyval, void *user);
boolean ht_iterate(hashtable ht,
                   ht_iterator_cb callback, 
                   void *user);

/* not-const version of the above; differs only in type. */
typedef boolean (*ht_iterator_cb_nc)(void *keyval, void *user);
boolean ht_iterate_nc(hashtable ht,
                   ht_iterator_cb_nc callback, 
                   void *user);

unsigned long ht_count(hashtable ht);

void ht_occupancyjgr(const hashtable ht, const char *fname);

/* works if you're storing ints, or if the key is
   already generated and the first int in the structure */
unsigned int hash_8byte(const void *k);
boolean isequal_8byte(const void *ia, const void *ib);
unsigned int hash_k_int(const int *k);
boolean isequal_k_int(const int *ia, const int *ib);
struct in_addr;
unsigned int hash_k_saddr(const struct in_addr *k);
boolean isequal_k_saddr(const struct in_addr *ia, const struct in_addr *ib);
unsigned int hash_k_uint(const unsigned int *k);
boolean isequal_k_uint(const unsigned int *ia, const unsigned int *ib);
unsigned int hash_int(const void *k);
boolean isequal_int(const void *ia, const void *ib);
unsigned int hash_ushort(const void *k);
boolean isequal_ushort(const void *ia, const void *ib);

NS_END_DECLS
#endif
