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

/* interface to Eric Hoffman's hprop configuration tree */ 
/* yes, I know it's stupid to have a separate interface for
   it, since the interface is so simple and elegant, I just wanted 
   to hide stuff */

#ifndef _hpropinterface_h
#define _hpropinterface_h
#include "nscommon.h"
#include "hprop.h"
#include "buffer.h"
#include "table.h"
struct hprop_st;

struct hprop_handle_struct {
  hprop root;
  table handlers;
};

typedef struct hprop_handle_struct *hprop_handle;

hprop_handle hpropi_new(void);
boolean hpropi_loadf(hprop_handle hh, const char *filename);
const char *hpropi_callback(hprop_handle hi, 
                            const char *ev );

#endif
