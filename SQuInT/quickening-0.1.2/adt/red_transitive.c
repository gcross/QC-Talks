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
#include <stdio.h>
#include "red_transitive.h"
#include "nscommon.h"

/* typedef rtm_color arraytype; */
typedef unsigned char arraytype;

struct red_transitive_matrix {
  /* could be just the upper triangle */
  arraytype *matrix;
  unsigned int dim;
};

unsigned int rtm_get_dim(const struct red_transitive_matrix *rtm) {
  return(rtm->dim);
}

rtm_color rtm_get_color(const struct red_transitive_matrix *rtm,
                        unsigned int node1, 
                        unsigned int node2) {
  return((rtm_color)rtm->matrix[node1*rtm->dim+node2]);
}

const char *rtm_color_tostring(rtm_color c) {
  switch (c) {
  case 0: return "black";
  case 1: return "red";
  case 2: return "green";
  case 3: return "purple";
  case 4: return "orange";
  case 5: return "aqua";
  case 6: return "trans_green";
  case 7: return "trans_red";
  case 8: return "bright_orange";
  case 9: return "bright_aqua";
  case 10: return "mud";

  default: return "unknown";
  }
}

struct red_transitive_matrix *rtm_new(unsigned int dim) {
  struct red_transitive_matrix *rtm = 
    (struct red_transitive_matrix *)malloc(sizeof(struct red_transitive_matrix));
  rtm->matrix = (arraytype*) malloc(dim*dim*sizeof(arraytype));
  if(rtm->matrix == 0) {
    fprintf(stderr, 
            "Unable to allocate sufficient memory for red transitive matrix.\n"
            "%d is too many routers for available memory.\n", dim);
  }
  memset(rtm->matrix,0,dim*dim*sizeof(arraytype));
  rtm->dim = dim;
  return(rtm);
}

static void 
rtm_color_edge(struct red_transitive_matrix *rtm, 
               unsigned int node1, 
               unsigned int node2, 
               rtm_color color) {
  if(node1 != node2) {
    /* fprintf(stderr, " m %d %d\n", node1, node2); */
    rtm->matrix[node1*rtm->dim+node2] = (arraytype) color;  
    rtm->matrix[node2*rtm->dim+node1] = (arraytype) color; 
  }
}

void rtm_set_color(struct red_transitive_matrix *rtm, 
                   unsigned int node1, 
                   unsigned int node2, 
                   rtm_color color) {
  unsigned int i, j;

  rtm_color_edge(rtm, node1, node2, color);
  if(color == purple) return; /* purple isn't transitive */

  /* we do a transitive closure of red */
  for(i=0; i<rtm->dim; i++) /* foreach neighbor of node1 */
    if(rtm_get_color(rtm, node1, i) == red && i!=node2) { /* if red edge */
      if(color == red)
	rtm_color_edge(rtm, i, node2, trans_red);
      if(color == green)
	rtm_color_edge(rtm, i, node2, trans_green);

      for(j=0; j<rtm->dim; j++) /* foreach neighbor of node2 */
        if(rtm_get_color(rtm, node2, j)  == red && j!=node1) {/* if red edge */
          if(color == red)
	    rtm_color_edge(rtm, i, j, trans_red);
	  if(color == green)
	    rtm_color_edge(rtm, i, j, trans_green);
	}
    }

  for(j=0; j<rtm->dim; j++) /* foreach neighbor of node2 */
    if(rtm_get_color(rtm, node2, j)  == red && j != node1){ /* if red edge */
      if(color == red)
	rtm_color_edge(rtm, node1, j, trans_red);
      if(color == green)
	rtm_color_edge(rtm, node1, j, trans_green);
    }
}

void rtm_set_color_non_transitive(struct red_transitive_matrix *rtm, 
				  unsigned int node1,
				  unsigned int node2,
				  rtm_color color) {

  rtm_color_edge(rtm, node1, node2, color);
  return;
}
			    

/* maybe? prefer unattached nodes? */
boolean rtm_get_uncolored_edge(/*@out@*/unsigned int * node1, 
                               /*@out@*/ unsigned int * node2);

/* overloaded for performance : returns true if this
   guy is responsive, false if it has no aliases 
   and doesn't respond */
boolean 
rtm_get_red_neighbors(struct red_transitive_matrix *rtm, 
                      unsigned int node, 
                      unsigned int buf[], unsigned int *numneighbors) {
  unsigned int i;
  unsigned int j;
  boolean ret = FALSE;
  if(node == 1) fprintf(stderr, "word?\n");
  for(i=0, j=0; i<rtm->dim && j<(*numneighbors); i++) {
    rtm_color c = rtm_get_color(rtm,node,i);
    if(c == red) {
      buf[j++] = i;
      ret = TRUE;
    } else if(!ret && c==green) {
      ret = TRUE;
      if(node == 1) {
        fprintf(stderr, "chill?\n");
      }
    }
  }
  (*numneighbors) = j;
  return ret;
}

boolean 
rtm_integrity_check(const struct red_transitive_matrix *rtm) {
  unsigned int i;
  unsigned int j;
  for(i=0; i<rtm->dim; i++) 
    for(j=i+1; j<rtm->dim; j++) 
      if(rtm_get_color(rtm, i, j) != rtm_get_color(rtm,j,i)) 
        return(FALSE);
  return(TRUE);
}
