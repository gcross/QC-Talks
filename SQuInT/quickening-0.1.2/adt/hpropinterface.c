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
#include "hpropinterface.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>


typedef struct demux {
  char *name;
  void (*handler)(void *, void *, void *);
} *demux;

int demux_compare(const void *vd,const void *vx)
{
  const demux d = (const demux)vd;
  const char *x = (const char *)vx;
  return(!strcmp(d->name,x));
}

void control_demux(hprop_handle hh,const char **name, char *value)
{
  demux d;
  hprop_set_argv(hh->root,name,value);
  if ((d=(demux)table_find(hh->handlers,name[0]))!=NULL)
    (*d->handler)(hh->root,name,value);
}

hprop_handle hpropi_new(void) {
  hprop_handle hh = (hprop_handle)malloc(sizeof(struct hprop_handle_struct));
  hh->root =create_hprop(NULL);
  hh->handlers=create_table(demux_compare,key_from_string);
  return(hh);
}
boolean hpropi_loadf(hprop_handle hh, const char *filename) {
  void *z;
  int fd;
  int result;
  char bufferx[200];
  if ((fd=open(filename,O_RDONLY))<0) {
    printf ("couldn't open %s for reading\n",filename);
    return (FALSE);
  }
  z=(void *)create_hprop_parser((hprop_parser_cb)control_demux,hh);
  while ((result=read(fd,bufferx,sizeof(bufferx)))>0)
    hprop_parse(z,bufferx,result);
  shutdown_hprop_parser(z);
  return(TRUE);
}

void hpropi_add_handler(hprop_handle hh, char *name, void (*f)(void *, void *, void *))
{
  demux d=(demux)malloc(sizeof(struct demux));
  d->name=name;
  d->handler=f;
  table_insert(hh->handlers,d,d->name);
}

