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

#ifndef _RED_TRANSITIVE_H
#define _RED_TRANSITIVE_H
#include "nscommon.h"

typedef enum rtm_color {
  black = 0, /* uncolored */
  red = 1,   /* transitive along */
  green = 2, /*  tranistive across red */
  purple = 3, /* not transitive */
  orange = 4, //this is for alias likelyhood
  aqua = 5,  //this is for non-alias likelyhood
  trans_green = 6,
  trans_red = 7,
  bright_orange = 8,
  bright_aqua = 9,
  mud = 10,
  yellow_on_black = 11,
  yellow_on_red = 12,
  yellow_on_green = 13,
  yellow_on_purple = 14,
  yellow_on_orange = 15,
  yellow_on_borange = 16, 
  yellow_on_aqua = 17, 
  yellow_on_baqua = 18,
  yellow_on_mud = 19, 
  yellow = 20
} rtm_color;

/* for molly: black=haven't tried, red=alias, green=not an alias,
   purple = tried and couldn't tell (unresponsive) */

typedef struct red_transitive_matrix *red_transitive;

rtm_color rtm_get_color(const struct red_transitive_matrix *rtm,
                        unsigned int node1, 
                        unsigned int node2) __attribute__((const));

const char *rtm_color_tostring(rtm_color c);

struct red_transitive_matrix *rtm_new(unsigned int dim);

/* sets and updates transitive relations */
void rtm_set_color(struct red_transitive_matrix *rtm, 
                   unsigned int node1, 
                   unsigned int node2, 
                   rtm_color color);


void rtm_set_color_non_transitive(struct red_transitive_matrix *rtm, 
                   unsigned int node1, 
                   unsigned int node2, 
                   rtm_color color);

/* returns true if this guy has any decent neighbors in the uid space beyond. */
boolean 
rtm_get_red_neighbors(struct red_transitive_matrix *rtm, 
                      unsigned int node, 
                      /*@out@*/unsigned int buf[], 
                      unsigned int *numneighbors);

boolean 
rtm_integrity_check(const struct red_transitive_matrix *rtm) __attribute__((const));
     
unsigned int 
rtm_get_dim(const struct red_transitive_matrix *rtm) __attribute__((const));
     
                                 
#endif
