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
#include "distribution.h"
#include "typed_hashtable.h"
#include <stdlib.h>
#include <math.h>
#include <string.h>
#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif


/* note: will stop working well if we need to store floats */
struct vpair {
  dist_type val;
  unsigned int count;
};

struct vpair *vpair_new(const dist_type val) {
  NEWPTR(struct vpair, ret);
  ret->val=val;
  ret->count=0;
  return(ret);
}

DECLARE_TYPED_HASHTABLE(struct vpair, vp);

struct distribution_struct {
  vp_hashtable cdf;
  unsigned int n;
  dist_type min, max, sum, ss;
};

distribution dist_new(boolean storeCDF) {
  NEWPTR(struct distribution_struct, dt);
  memset(dt,0,sizeof(struct distribution_struct));
  if(storeCDF) 
    dt->cdf=vp_ht_new(1024,(vp_hash_fn) hash_int, 
                  (vp_iseq_fn) isequal_int, 
                  (vp_ht_delete_cb)free);
  return(dt);
}
void dist_delete(distribution bye) {
  if(bye->cdf) 
    vp_ht_delete(bye->cdf,FALSE);
  free(bye);
}

void 
insertcdf(dist_type in, vp_hashtable cht) { 
  struct vpair *vp;
  vp = vp_ht_lookup_nofail(cht, (struct vpair *)&in, 
                           (vp_ht_constructor_cb)vpair_new); 
  if(vp==NULL) {
    vp = (struct vpair *)malloc(sizeof(struct vpair));
    vp->val = in;
    vp->count = 0;
    vp_ht_insert(cht, vp);
  }
  vp->count++;
}

void 
dist_insert(distribution d, dist_type in) {
  if(d->n == 0) {
    d->min=in;
    d->max=in;
  }
  d->n++;
  d->sum+=in;
  d->ss+=(in*in);
  if(in < d->min) { d->min = in; }
  if(in > d->max) { d->max = in; }
  if(d->cdf) {
    insertcdf(in,d->cdf);
  }
}

dist_type 
dist_get_min(distribution d) {
  return(d->min);
}
dist_type 
dist_get_max(distribution d) {
  return(d->max);
}
float 
dist_get_mean(distribution d) {
  return((float)d->sum / (float)d->n);
}

boolean vpair_copy(const struct vpair *ht_version, void *pcursor) {
  struct vpair *cursor = *((struct vpair **)pcursor);
  cursor->val = ht_version->val;
  cursor->count = ht_version->count;
  (*(struct vpair **)pcursor) += 1;
  return(TRUE);
}

static int disttype_comparator(const void *v1, const void *v2) {
  int a = *(int*)v1;
  int b = *(int*)v2;
  if(a<b) {
    return -1;
  } else if(a>b) {
    return 1;
  } else {
    return 0;
  }
}

float dist_get_median(distribution d) {
  struct vpair *linearized, *cursor;
  unsigned int n, i, count;
  unsigned int target;
  float median;
  if(d->cdf == NULL) {
    fprintf(stderr, "need cdf to calculate median\n");
    abort();
  }
  n = vp_ht_count(d->cdf);
  linearized = malloc(sizeof(struct vpair)*n);
  cursor = linearized;
  vp_ht_iterate(d->cdf, vpair_copy, &cursor);
  qsort(linearized, n, sizeof(struct vpair),  disttype_comparator);
  target = d->n/2;
  for(i=0, count=0; i<n && count<target; count++) {
    if(linearized[i].count + count > target) {
      median = linearized[i].val;
    } else if(linearized[i].count + count == target) {
      median = (linearized[i].val + linearized[i+1].val) / 2;
    }
    count += linearized[i].count;
  }
  free(linearized);
  return(0.0);
}

float dist_get_var(distribution d) {
  return((float)((double)d->ss - ((double)d->sum * d->sum / d->n)) / 
         (double)(d->n-1));
}
float dist_get_sd(distribution d) {
  return(sqrt((double)dist_get_var(d)));
}
unsigned int dist_get_n(distribution d) {
  return(d->n);
}

