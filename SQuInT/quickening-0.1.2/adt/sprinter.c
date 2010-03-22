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
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/in_systm.h> /* needed before include of ip.h */
#include <netinet/ip.h>
#include <string.h>
#include <stdio.h>
#include "sprinter.h"
#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

//static char macbuffer[18];
char *sprintmac(macaddress mac, char *macbuffer) {
  unsigned int oct1 = (mac >> 40) & 0xffu;
  unsigned int oct2 = (mac >> 32) & 0xffu;
  unsigned int oct3 = (mac >> 24) & 0xffu;
  unsigned int oct4 = (mac >> 16) & 0xffu;
  unsigned int oct5 = (mac >> 8) & 0xffu;
  unsigned int oct6 = (mac >> 0) & 0xffu;
  sprintf(macbuffer, "%02x:%02x:%02x:%02x:%02x:%02x", 
          oct1, oct2, oct3, oct4, oct5, oct6);
  return(macbuffer);
}
static char ipbuffer[18];
char *sprintip(struct in_addr ip) {
  unsigned int oct1 = (ip.s_addr >> 24) & 0xffu;
  unsigned int oct2 = (ip.s_addr >> 16) & 0xffu;
  unsigned int oct3 = (ip.s_addr >> 8) & 0xffu;
  unsigned int oct4 = (ip.s_addr ) & 0xffu;
  sprintf(ipbuffer, "%u.%u.%u.%u", oct1, oct2, oct3, oct4);
  return(ipbuffer);
}

boolean sscanipstr(struct in_addr *pip, const char *string) {
  unsigned int quadary[4];
  if(sscanf(string, "%u.%u.%u.%u", 
            &quadary[0],
            &quadary[1],
            &quadary[2],
            &quadary[3]) == 4) {
    sscanip(pip, quadary);
    return(TRUE);
  } else {
    return(FALSE);
  }
    
}

void sscanip(struct in_addr *pip, const unsigned int *quadary) {
  int i;
  pip->s_addr = 0;
  for(i=0;i<4;i++) {
    pip->s_addr <<= 8;
    pip->s_addr += quadary[i];
  }
}

void sscanipc(struct in_addr *pip, const unsigned char *quadary) {
  int i;
  pip->s_addr = 0;
  for(i=0;i<4;i++) {
    pip->s_addr <<= 8;
    pip->s_addr += (unsigned int)(quadary[i]);
  }
}

void sscanmac(macaddress *pmac, const unsigned int *hex) {
  int i;
  *pmac = 0;
  for(i=0;i<6;i++) {
    *pmac <<= 8;
    *pmac += hex[i];
  }
}
void sscanmacc(macaddress *pmac, const unsigned char *hex) {
  int i;
  *pmac = 0;
  for(i=0;i<6;i++) {
    *pmac <<= 8;
    *pmac += (macaddress)(hex[i]);
  }
}
  
boolean sscanmacstr(macaddress *pmac, const char *string) {
  unsigned int hex[6];
  if(sscanf(string, "%x:%x:%x:%x:%x:%x", 
            &hex[0],
            &hex[1],
            &hex[2],
            &hex[3],
            &hex[4],
            &hex[5]) == 6){
    sscanmac(pmac, hex);
    return(TRUE);
  } else {
    *pmac = 0;
    return(FALSE);
  }
}
