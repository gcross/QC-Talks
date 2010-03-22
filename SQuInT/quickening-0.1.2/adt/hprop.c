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
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include "buffer.h"

#include "table.h"
#include "hprop.h"
#ifdef WIN32
#include "win32.h"
#else
/* inet_aton 
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
inet_aton */
#endif



char **hprop_string(const char *source)
{
  int avlen=10;
  char **argv=(char **)malloc(sizeof(char *)*avlen);
  int argc=0,alen=0,afill=0;
  const char *i;

  for (i=source;*i;i++){
    if (alen<=afill) 
      argv[argc]=(char *)(alen?realloc(argv[argc],alen+=64):
			  (char *)malloc(alen=64));
    if (*i==':'){
      *(argv[argc]+afill)=0;
      alen=afill=0;
      if (++argc >= (avlen-2))
	argv=(char **)realloc(argv,(avlen+=12)*sizeof(char *));
    } else {
      *(argv[argc]+afill++)=*i;
    }
  }
  if (afill){
    *(argv[argc++]+afill)=0;
  } else {
    argv[argc]=malloc(4);
    *(argv[argc++])='\0';  /* ns (from argv[argc++] = "") */
  }
  argv[argc++]=0;
  return(argv);
}

static int compare_children(const void *hprop_n, const void *char_starname)
{
  const hprop n = (const hprop)hprop_n;
  const char *name = (const char *)char_starname;
  return(!strcmp(n->name,name));
}


typedef struct iterate_children_state {
  void (*f)(void *a, const char **name, char *val);
  void *a;
  hprop n;
  int argc;
  const char *argv[15]; /*stupid*/
} *iterate_children_state;


static void hprop_internal(void *iterate_children_state_i, void *hprop_n)
{
  iterate_children_state i = (iterate_children_state) iterate_children_state_i;
  hprop n = (hprop) hprop_n;
  int k=i->argc;
  i->argv[i->argc++]=n->name;
  i->argv[i->argc]=0;
  /* this +1 should be moved back before the iterate*/
  if (n->value) (*i->f)(i->a,i->argv+1,n->value);
  iterate_table_entries(n->children,hprop_internal,i);
  i->argc=k;
}

void hprop_iterate_children(hprop n,void (*f)(void *a, const char **name, char *val), void *a)
{
  struct iterate_children_state c;
  c.argc=0;
  c.f=f;
  c.a=a;
  hprop_internal(&c,n);
}

typedef struct dump_state {
  char *buffer;
  int fill;
  int size;
  void (*f)(void *a, char *buf, int );
  void *a;
} *dump_state;


static void hprop_dump_internal(void *dump_state_d, void *hprop_h)
{
  dump_state d = (dump_state)dump_state_d;
  hprop h = (hprop)hprop_h;

  int n=strlen(h->name);
  int v=0,len;
  unsigned int old_fill=d->fill;

  if (h->value) v=strlen(h->value);
  if (d->fill+n+v+2 >= d->size) d->buffer=realloc(d->buffer,d->size+n+v);
  len=sprintf(d->buffer+d->fill,"%s %s\n",h->name,h->value);
  if (h->value) (*d->f)(d->a,d->buffer,len+d->fill);
  d->buffer[d->fill+=n]=':';
  d->fill++;
  iterate_table_entries(h->children,hprop_dump_internal,d);
  d->fill=old_fill;
}

void hprop_dump(hprop h,
                void (*f)(void *a, char *buffer, int ), 
                void *a)
{
  struct dump_state d;
  d.buffer=malloc(d.size=100);
  d.fill=0;
  d.f=f;
  d.a=a;
  iterate_table_entries(h->children,hprop_dump_internal,&d);
}

char *hprop_get(hprop n,const char *name)
{
  hprop c;
  if ((c=table_find(n->children,name)) != NULL)
    return(c->value);
  return(0);
}


double hprop_float(hprop config,const char *name)
{
  char *value;

  if ((value=hprop_get(config,name)) != NULL)
    return(atof(value));
  return(0);
}

unsigned int hprop_v4addr(hprop config,const char *name)
{
  char *value;

  if ((value=hprop_get(config,name)) != NULL){
    return(ipv4_addr_from_string(value));
  }
  return(0);
}

void hprop_increment(hprop c)
{
  if (!c->value){
    c->value=(char *)malloc(30);
    c->value[0]='0';
    c->value[1]=0;
  }
  sprintf(c->value,"%d",atoi(c->value)+1);
}

int hprop_int(hprop config,const char *name)
{
  char *value;

  if ((value=hprop_get(config,name)) != NULL)
    return(atoi(value));
  return(0);
}

typedef struct iterate_state {
  void (*f)(void *a, const char *name, const char *value, hprop h);
  void *a;
} *iterate_state;

static void each_iterate(void *iterate_state_i, void *hprop_h)
{
  iterate_state i = iterate_state_i;
  hprop h = hprop_h;
  (*i->f)(i->a,h->name,h->value,h);
}

hprop hprop_node(hprop p,const char *name)
{
  char **z=hprop_string(name),**i;
  hprop h;

  for (i=z,h=p;*i&&h;i++) {
    h=table_find(h->children,*i);
    free(*i);
  }
  free(z);
  return(h);
}


void hprop_iterate(hprop n, 
                   void (*f)(void *a, const char *name, const char *value, hprop h),
                   void *a)
{
  struct iterate_state i;

  if (n){
    i.f=f;
    i.a=a;
    iterate_table_entries(n->children,each_iterate,&i);
  }
}


hprop create_hprop(hprop p)
{
  hprop n;
  n=(hprop)malloc(sizeof(struct hprop_st));
  n->children=create_table(compare_children,key_from_string);
  n->name=0;
  n->value=0;  
  n->parent=p;
  return(n);
}

hprop hprop_node_create(hprop p,const char *name)
{
  hprop n,v;
  char **z=hprop_string(name),**i;

  for (i=z,n=p;*i;i++,n=v) {
    if ((v=table_find(n->children,*i)) != NULL) {
      free(*i);
    } else {
      v=create_hprop(p);
      v->name=*i;
      table_insert(n->children,v,*i);
    } 
  }
  free(z);
  return(n);
}


/*should be able to take varargs*/
void hprop_set_argv(hprop n,const char **argv,char *value)
{
  hprop c;

  if (*argv){
    if (!(c=table_find(n->children,argv[0]))) {
      c=create_hprop(n);
      c->name=argv[0];
      table_insert(n->children,c,c->name);
    }
    hprop_set_argv(c,argv+1,value);
  } else n->value=(char *)value;   /* const violation */
}

void hprop_set(hprop n,const char *name,const char *value)
{
  hprop c;

  if (!(c=table_find(n->children,name))){
    c=create_hprop(n);
    c->name=strdup(name);
    table_insert(n->children,c,c->name);
  }
  c->value=strdup(value);
}

void hprop_parse(hprop_parser p, const char *buf, int len)
{
  int i;
  for (i=0;i<len;i++){
    switch(p->state){
    case 0: /*beginning of line*/
      if (buf[i]=='\n') break;
      if (buf[i]=='#'){
	p->state=3;
	break;
      }
      p->state++;
    case 1: /* collect up the name*/
      if ((buf[i]==' ') || (buf[i]=='\n')){
	p->line[p->lfill]=0;
	p->name=(const char **)hprop_string(p->line);
	p->lfill=0;
	p->state++;
	if (buf[i]==' ') break;
      }
    case 2: /* look for the end of the value*/
      if (buf[i]=='\n'){
	p->line[p->lfill]=0;
	(*p->f)(p->a,p->name,strdup(p->line));
	p->state=p->lfill=0;
	break;
      }
    default:
      if (p->lsize < p->lfill+2) 
	p->line=realloc(p->line,p->lsize*=2);
      p->line[p->lfill++]=buf[i];
    case 3: /*comments*/
      if (buf[i]=='\n') p->state=0;
    }
  }
}

void shutdown_hprop_parser(hprop_parser p)
{
  hprop_parse(p,"\n",1);
}

void *create_hprop_parser(void (*f)(void *, const char **name, char *value), void *a)
{
  hprop_parser p=(hprop_parser)malloc(sizeof(struct hprop_parser_st));
  memset(p,0,sizeof(struct hprop_parser_st));
  p->f=f;
  p->a=a;
  p->line=malloc(p->lsize=512);
  return(p);
}

