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
#include "link_matrix.h"
#include "nscommon.h"

typedef unsigned char arraytype;

struct link_matrix {
  /* could be just the upper triangle */
  arraytype *matrix;
  unsigned int dim;
};

unsigned int lm_get_dim(const struct link_matrix *lm) {
  return(lm->dim);
}

boolean lm_is_linked(const struct link_matrix *lm,
                     unsigned int node1, 
                     unsigned int node2) {
  return(lm->matrix[node1*lm->dim+node2] != 0 ? TRUE : FALSE);
}

void lm_link(struct link_matrix *lm,
             unsigned int node1, 
             unsigned int node2) {
  lm->matrix[node1*lm->dim+node2] = 1;
  lm->matrix[node2*lm->dim+node1] = 1;
}

void lm_get_neighbors(struct link_matrix *lm, 
                      unsigned int node, 
                      unsigned int buf[], 
                      unsigned int *numneighbors) {
  unsigned int i;
  unsigned int j;
  for(i=0, j=0; i<lm->dim && j<(*numneighbors); i++) {
    if(lm_is_linked(lm, node, i)) {
      buf[j++] = i;
    }
  }
  (*numneighbors) = j;
}

struct link_matrix *lm_new(unsigned int dim) {
  struct link_matrix *lm = 
    (struct link_matrix *)malloc(sizeof(struct link_matrix));
  lm->matrix = (arraytype*) malloc(dim*dim*sizeof(arraytype));
  if(lm->matrix == 0) {
    fprintf(stderr, 
            "Unable to allocate sufficient memory for link matrix.\n"
            "%d is too many routers for available memory.\n", dim);
  }
  memset(lm->matrix,0,dim*dim*sizeof(arraytype));
  lm->dim = dim;
  return(lm);
}
