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
#include "nscommon.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h> 
#include <sys/socket.h> /* AF_INET */
#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif
#include "hashes.h"

unsigned int mac_hash(const void *key) {
  unsigned int retval; // this is a pretty damn useless hash.
  const macaddress *mac1 = key;
  retval=(unsigned int)(*mac1*1109); // prime.
  return(retval);
}

// * uses key as a pointer * 
unsigned int ip_hash(const void *key) {
  const unsigned long *addr = key;
  return(*addr*12917);
}

// * uses key as an integer *
unsigned int port_hash(const void *key) {
  return((unsigned int)key);
}

unsigned int string_hash(const char *obj) {
  unsigned int h= 0;  // seed with the server.
  unsigned int i;
  assert(obj!=NULL);
  for(i=0; i<256 && obj[i]!='\0'; i++){
    h*=307;
    h+=(unsigned int) obj[i];
  }
  return h;
}
unsigned int pstring_hash(const hash_cptr *key) {
  return(string_hash(*key));
}

boolean mac_isequal(const void *v1, const void *v2) {
  const macaddress *mac1 = v1;
  const macaddress *mac2 = v2;
  return((*mac1==*mac2) ? TRUE : FALSE);
}
boolean ip_isequal(const void *v1, const void *v2) {
  const unsigned long *addr1 = v1;
  const unsigned long *addr2 = v2;
  return((*addr1==*addr2) ? TRUE : FALSE);
}
boolean port_isequal(const void *key1, const void *key2) {
  return((key1==key2) ? TRUE : FALSE);
}
boolean string_isequal(const char *v1, const char *v2) {
  return( (strcmp(v1,v2)==0) ? TRUE : FALSE);
}
boolean pstring_isequal(const hash_cptr *v1, const hash_cptr *v2) {
  return(string_isequal(*v1, *v2));
}

boolean sockaddr_in_isequal(const struct sockaddr_in *v1, const struct sockaddr_in *v2) {
  assert(v1->sin_family == AF_INET);
  assert(v2->sin_family == AF_INET);
  return ( v1->sin_port == v2->sin_port && memcmp(&v1->sin_addr, &v2->sin_addr, sizeof(struct in_addr)) == 0);
}

unsigned int sockaddr_in_hash(const struct sockaddr_in *v) {
  return(v->sin_addr.s_addr*12917 + v->sin_port);
}
