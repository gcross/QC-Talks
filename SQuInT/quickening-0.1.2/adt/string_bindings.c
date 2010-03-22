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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "typed_hashtable.h"
#include "string_bindings.h"

#ifndef __linux__
typedef int (*__compar_fn_t)(const void *, const void *);
#endif

/* kept in the hash table */
struct string_binding {
  const char *string;
  unsigned short idx;
};

static unsigned int hash_sb(const struct string_binding *sbs) {
  unsigned int ret;
  const char *p;
  for(ret=0, p=sbs->string; p!= NULL && *p!='\0'; p++) 
    ret += (unsigned int) *p;
  return ret;
}

static boolean isequal_sb(const struct string_binding *sbs1,
                   const struct string_binding *sbs2) {
  return( strcmp(sbs1->string, sbs2->string) == 0 );
}

DECLARE_TYPED_HASHTABLE(struct string_binding, sb);

struct string_bindings_st { 
  sb_hashtable ht; 
  const char **array;
  unsigned short array_mallocd;
  unsigned short next_index;
};

string_bindings strb_new(unsigned short initial_size) {
  string_bindings ret = malloc(sizeof(struct string_bindings_st));
  assert(ret != NULL);
  ret->ht = sb_ht_new(initial_size, hash_sb, isequal_sb, (sb_ht_delete_cb)free);
  ret->array = malloc(initial_size * sizeof(const char *));
  assert(ret->array != NULL);
  memset(ret->array, 0, initial_size * sizeof(const char *));
  ret->array_mallocd = initial_size;
  ret->next_index = 1;
  
  return(ret);
}
void strb_delete(/*@only@*/string_bindings sb) {
  sb_ht_delete(sb->ht, FALSE);
  free(sb->array);
  free(sb);
}

static void strb_bind_helper(string_bindings sb,
                             const char *st,
                             unsigned short idx) {
  struct string_binding *ps = malloc(sizeof(struct string_binding));
  assert(ps != NULL);
  ps->string = st;
  ps->idx = idx; 
  sb_ht_insert(sb->ht, ps);
}

unsigned short strb_bind(string_bindings sb, /*@owned@*/const char *st) {
  if(sb->next_index >= sb->array_mallocd) {
    sb->array_mallocd = min(sb->array_mallocd * 2, 65000);
    sb->array = realloc(sb->array, 
                        sb->array_mallocd * sizeof(const char *));
    assert(sb->array != NULL);
  }
  strb_bind_helper(sb, st, sb->next_index);
  sb->array[sb->next_index] = st;
  return( sb->next_index++ );
}

unsigned short strb_lookup(const string_bindings sb, const char *st) {
  struct string_binding *ps = 
    sb_ht_lookup(sb->ht, (struct string_binding *)&st);
  if(ps) {
    return ps->idx;
  } else {
    return 0;
  }
}

/*@dependent@*/
/*@null@*/
const char *strb_resolve(const string_bindings sb, unsigned short idx) {
  if(idx < sb-> next_index) {
    return(sb->array[idx]);
  } else {
    return NULL;
  }
}
static boolean 
verify_bindings(const struct string_binding *b, void *vsb) {
  string_bindings sb = (string_bindings) vsb;
  if(b->idx == 0) return FALSE;
  if(sb->array[b->idx] != b->string) {
    printf("ary[%d] = %s, b = %s\n", b->idx, sb->array[b->idx], b->string);
    return(FALSE); /* stop */
  }
  return(TRUE);
}

void strb_integrity_check(string_bindings sb) {
  if(sb_ht_iterate(sb->ht, verify_bindings, sb) == FALSE) {
    printf("string bindings integrity check failed\n");
    abort();
  }
}

void 
strb_sort_and_rebind(string_bindings sb, 
                     int (*ind_string_compare)(const char **a, 
                                               const char **b)) {
  unsigned short i;

  strb_integrity_check(sb);

  sb_ht_delete(sb->ht, FALSE);
  qsort(sb->array+1, sb->next_index-1, 
        sizeof(const char *), 
        (__compar_fn_t)ind_string_compare);
  sb->ht = sb_ht_new(sb->next_index, hash_sb, isequal_sb, (sb_ht_delete_cb)free);
  for(i=1; i< sb->next_index; i++) {
    strb_bind_helper(sb, sb->array[i], i);
  }
  strb_integrity_check(sb);
}

void 
strb_iterate(string_bindings sb,
             void (*callback)(const char *, 
                              unsigned short, 
                              void *user),
             void *user) {
  unsigned short i;
  for(i=0; i<sb->next_index; i++) {
    callback(sb->array[i], i, user);
  }
}

static void 
strb_print_cb(const char *str, unsigned short i, void *fpout) {
  FILE *fp = (FILE *)fpout;
  fprintf(fp, "%u %s\n", i, str);
}

void 
strb_print(string_bindings sb, FILE *fpout) {
  strb_iterate(sb, strb_print_cb, fpout);
}

unsigned short 
strb_count(string_bindings sb) {
  return(sb->next_index);
}
