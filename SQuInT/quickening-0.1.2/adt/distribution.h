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

#ifndef _distribution_h
#define _distribution_h
#include "nscommon.h"
#include <stdio.h>

typedef float dist_type;
typedef struct distribution_struct *distribution;

/*@only@*/ distribution dist_new(boolean storeCDF);
void dist_delete(/*@only@*/ distribution dist);

void dist_insert(distribution dist, dist_type to_insert);

boolean dist_iterate(distribution dist, 
                     boolean (*iterator)(int value, void *user), 
                     void *user);

/* eg mean */
float dist_extract_float(distribution dist, const char *tracked);
/* eg median, min, n */
dist_type dist_extract(distribution dist, const char *tracked);
boolean dist_extract_to_file(distribution dist, 
                             const char *tracked, 
                             const char *filename);
float dist_get_mean(distribution d);
dist_type dist_get_max(distribution d);
dist_type dist_get_min(distribution d);
float dist_get_median(distribution d);
float dist_get_sd(distribution d);
float dist_get_var(distribution d);
unsigned int dist_get_n(distribution d);
#endif
