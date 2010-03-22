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
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#ifndef WIN32
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#ifndef __LCLINT__
#include <arpa/inet.h>
#endif
#else
#include "win32.h"
#endif
#include "nscommon.h"
#include "sockaddr.h"

in_addr_t ipv4_addr_from_string(const char *x)
{
  unsigned int a,b,c,d;
  unsigned int t;

  a=b=c=d=0;
  if (sscanf(x,"%u.%u.%u.%u",&a,&b,&c,&d)>3){
    return(htonl(a<<24|b<<16|c<<8|d));
  } else if( sscanf(x,"x%x",&t) > 0) {
    return(htonl(t));
  } else {
    return(0);
  }
}

struct sockaddr_in *make_numeric_sockaddr_in(unsigned int host,
					     unsigned short port)
{
  NEWPTRZ(struct sockaddr_in, new);
  // huh? host=host;
  memcpy (&new->sin_addr, &host, 4);
  new->sin_port = port; 
  new->sin_family = AF_INET;
  return(new);
}

in_addr_t ipv4_name_lookup(const char *hostname) {
  struct hostent *he;
  in_addr_t haddr=0;
  if (hostname){
    if (!(he=gethostbyname(hostname))){
      haddr=ipv4_addr_from_string(hostname);
    } else memcpy(&haddr, he->h_addr, 4);
  }
  return (haddr);
}

boolean ipv4_reverse_name_lookup(in_addr_t address, 
                                 /*@out@*/ char *name, 
                                 int namebuflen ) {
  struct hostent *he;
  struct in_addr in;
  in.s_addr = address;
  he = gethostbyaddr((const char *)&in, sizeof(in), AF_INET); 
  if(he != NULL) {
    /* fprintf(stderr, "reverse name successful for %s: %s\n", 
       inet_ntoa(in), he->h_name); */
    strncpy(name, he->h_name, namebuflen-1);
    name[namebuflen - 1]  = '\0'; /* force null term */
    return TRUE;
  } else {
    /* fprintf(stderr, "reverse name lookup failed for %s\n", inet_ntoa(in)); */
    return FALSE;
  }
}

struct sockaddr_in *make_sockaddr_in(char *hostname,char *service)
{
  in_addr_t haddr=0;
  unsigned short port;
  NEWPTRZ(struct sockaddr_in, new);

  haddr = ipv4_name_lookup(hostname);

#ifndef WIN32
  {
    struct servent *se;

    /* I hope these aren't important -ns */
    getservent(); 

    if (!(se=getservbyname(service,0))){
      port=(unsigned long)atol(service);
      if (port == 0) {
        free(new);
        return(NULL);
      }
    } else {
      port= se->s_port; 
    }
    endservent();  
  }
#else
  port=(unsigned short)atol(service);
#endif
  new->sin_family = AF_INET;
  /*  haddr=htonl(haddr);*/
  memcpy (&new->sin_addr, &haddr, 4);
  new->sin_port = htons(port); 
  return(new);
}

