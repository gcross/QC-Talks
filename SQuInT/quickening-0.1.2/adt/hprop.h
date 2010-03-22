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

#ifndef E_HPROP
#define E_HPROP
#include "table.h"

typedef struct hprop_parser_st {
  char *line;
  int lsize,ncount,state;
  int lfill;
  const char **name;
  void (*f)(void *, const char **, char *);
  void *a;
} *hprop_parser;


typedef struct hprop_st  {
  table children;
  char *value;
  const char *name;
  struct hprop_st *parent;
} *hprop;


char *hprop_get(hprop n,const char *name);
void hprop_set(hprop n,const char *name,const char *value);
void hprop_set_argv(hprop n,const char **argv,char *value);
hprop create_hprop(hprop p); 
char **hprop_string(const char *source);
hprop hprop_node(hprop p,const char *name);
double hprop_float(hprop config,const char *name);
unsigned int hprop_v4addr(hprop config,const char *name);
int hprop_int(hprop config,const char *name);

typedef void (*hprop_parser_cb)(void *, const char **name, char *value);
void *create_hprop_parser(hprop_parser_cb f, void *a);
void hprop_parse(hprop_parser p, const char *buf, int len);
void hprop_increment(hprop c);
void shutdown_hprop_parser(hprop_parser p);
void hprop_iterate(hprop n, 
                   void (*f)(void *a, const char *name, const char *value, hprop h),
                   void *a);
void hprop_iterate_children(hprop n,void (*f)(void *a, const char **name, char *val), void *a);
hprop hprop_node_create(hprop p,const char *name);
#endif
