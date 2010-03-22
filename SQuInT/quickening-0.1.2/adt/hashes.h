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

#ifndef _hashes_h
#define _hashes_h
#include <sys/types.h>
#include <netinet/in.h>
typedef char *hash_cptr;
unsigned int mac_hash(const void *key);   
unsigned int ip_hash(const void *key);    // treats void * as u_int *
unsigned int port_hash(const void *key);  // treats void * as u_int
unsigned int string_hash(const char *key);
unsigned int pstring_hash(const hash_cptr *key);
unsigned int sockaddr_in_hash(const struct sockaddr_in *v);

boolean mac_isequal(const void *v1, const void *v2);
boolean ip_isequal(const void *v1, const void *v2);
boolean port_isequal(const void *key1, const void *key2);
boolean string_isequal(const char *string1, const char *string2);
boolean pstring_isequal(const hash_cptr *string1, const hash_cptr *string2);
boolean sockaddr_in_isequal(const struct sockaddr_in *v1, const struct sockaddr_in *v2);
#endif
