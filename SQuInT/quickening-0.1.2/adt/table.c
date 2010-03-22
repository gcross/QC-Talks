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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdlib.h>
#include <string.h>
#include "table.h"

unsigned int key_from_int(const void *int_i)
{
  return((unsigned int)int_i);
}

unsigned int key_from_string(const void * char_stars)
{
  const unsigned char *s = (const unsigned char *)char_stars;
  unsigned int result=0;
  const unsigned char *n;
  int i;
  if (!s) return(1);
  for (n=s,i=0;*n;n++,i++) result^=(*n*57)^*n*i;
  return(result);
}


table create_table (int (*compare_function)(const void *k1, const void *k2),
		    unsigned int (*key_function)(const void *k))
{
  table new=(table)malloc(sizeof(struct table_st));
  memset(new, 0, sizeof(struct table_st));

  new->compare_function=compare_function;
  new->key_function=key_function;
  new->number_of_entries=0;
  new->size=4;
  new->entries=(table_entry *)malloc(sizeof(table_entry)*new->size);
  memset(new->entries,0,sizeof(table_entry)*new->size);
  return(new);
}

static table_entry *table_lookup (const table t,const void *comparator,
				  unsigned int k,
				  int (*compare_function)(const void *k1, const void *k2),
				  int *success)
{
  unsigned int key=k%t->size;
  table_entry *i;

  for (i=&(t->entries[key]);*i;i=&((*i)->next)){
    if (compare_function && ((*i)->key==k))
      if ((*t->compare_function)((*i)->value,comparator)){
	*success=1;
	return(i);
      }
  }
  *success=0;
  return(&(t->entries[key]));
}

void *table_find (const table t, const void *comparator)
{
  int success;
  table_entry* entry=table_lookup(t,comparator,
				  (*t->key_function)(comparator),
				  t->compare_function,
				  &success);
  if (success)  return((*entry)->value);
  return(0);
}


static void resize_table(table t, int size)
{
  int old_size=t->size;
  table_entry *old_entries=t->entries;
  int i; 
  table_entry j,n;
  table_entry *position;
  int success;
  
  t->size=size;
  t->entries=(table_entry *)malloc(sizeof(table_entry)*t->size);
  memset(t->entries,0,sizeof(table_entry)*t->size);

  for (i=0;i<old_size;i++)
    for (j=old_entries[i];j;j=n){
      n=j->next;
      position=table_lookup(t,0,j->key,0,&success);
      j->next= *position;
      *position=j;
    }
  free(old_entries);
}


void table_insert (table t, void *value, const void *comparator)
{
  int success;
  unsigned int k=(*t->key_function)(comparator);
  table_entry *position=table_lookup(t,comparator,k,
				     t->compare_function,&success);
  table_entry entry;

  if (success) {
    entry = *position;
  } else {
    entry = (table_entry)malloc(sizeof(struct table_entry_st));
    memset(entry, 0, sizeof(struct table_entry_st));
    entry->next= *position;
    *position=entry;
    t->number_of_entries++;
  }
  entry->value=value;
  entry->key=k;
  if (t->number_of_entries > t->size) resize_table(t,t->size*2);
}

void table_remove (table t, void *comparator)
{
  int success;
  table_entry temp;
  table_entry *position=table_lookup(t,comparator,
				     (*t->key_function)(comparator),
				     t->compare_function,&success);
  if(success) {
    temp=*position;
    *position=(*position)->next;
    free(temp); /* doesn't remove the value? */
    t->number_of_entries--;
  }
}

/* this really shouldn't be order n -- number generator?*/
void *random_table_entry(table t)
{
  int k;
  table_entry e=0;
  unsigned int idx=0;
  if (!t->number_of_entries) return(0);
#ifdef WIN32
  k=rand()%t->number_of_entries;
#else
  k=random()%t->number_of_entries;
#endif
  do {
    if (e) e=e->next;
    else e=t->entries[idx++];
    if (e) k--;
  } while (k>=0);
  return(e->value);
}

void iterate_table_entries(table t, 
                           void (*handler)(void *arg, void *value), 
                           void *arg)
{
  int i;
  table_entry *j,*next;
  
  for (i=0;i<t->size;i++)
    for (j=t->entries+i;*j;j=next){
      next=&((*j)->next);
      (*handler)(arg,(*j)->value);
    }
}

/* this will break if operations on the table occur in the handler*/
/* should take a retain/remove handler result*/
void filter_table_entries(table t, int (*handler)(void *a, void *value), 
                          void *arg)
{
  int i;
  table_entry *j,*next,v;
  
  for (i=0;i<t->size;i++)
    for (j=t->entries+i;*j;j=next){
      next=&((*j)->next);
      if (!(*handler)(arg,(*j)->value)){
        next=j;
        v=*j;
        *j=(*j)->next;
        t->number_of_entries--;
      }
    }
}

void destroy_table(table t,void (*thunk)(void *value))
{
  table_entry j,next;
  int i;
  for (i=0;i<t->size;i++)
    for (j=t->entries[i];j;j=next){
      next=j->next;
      if (thunk) (*thunk)(j->value);
      free(j);
    }
  free(t->entries);
  free(t);
}
