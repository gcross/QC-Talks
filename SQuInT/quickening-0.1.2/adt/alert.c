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
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
/* for fussing with getting a user id */
#include <pwd.h>
#include <sys/types.h>
#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

static char admin[200];

#undef REALLY_MAIL

#define SENDMAIL

static const char messageformat[] = 
#ifdef SENDMAIL
"To: %s\n"
"From: %s\n"
"Subject: Sentry Alert: %s\n\n"
#endif
"%s\n";
 

void alert_notifyadmin(const char *type, const char *fmt, ...) {
  char filename[200];
  char buf[512];
  FILE *fp;
#ifdef REALLY_MAIL
  pid_t pid;
#endif
  int w;
  va_list ap;
  snprintf(filename, 200, "/tmp/%d-msg%d", getpid(), rand()%1000);
  /* write message to a file */
#ifdef REALLY_MAIL
  fp = fopen(filename, "w");
#else
  fp = stdout;
#endif
  va_start(ap,fmt);
  w=vsnprintf(buf, 512, fmt, ap);
  fprintf(stderr, "message buf %d of 512 char\n", w);
  va_end(ap);
  fprintf(fp, messageformat, admin, "spring@pa.dec.com", type, buf);
  /* vfork and exec sendmail */
#ifdef REALLY_MAIL
  fclose(fp);
  
  if(admin[0] == '\0') {
    return;
  }
  pid=vfork();
  if(pid == 0) {
#ifdef SENDMAIL
    const char * const args [10] = {
      "/bin/sh",
      "-c",
      buf,
      NULL
    };
#else 
    const char * const args [10] = {
      "/bin/sh",
      "-c",
      "/bin/mail",
      "-n",
      "-s",
      buf,
      admin,
      "<",
      filename,
      NULL
    };
#endif
#ifdef SENDMAIL
    snprintf(buf, 512, "/usr/sbin/sendmail -t < %s", filename);
#else
    snprintf(buf, 512, "Sentry Alert: %s", type);
#endif
    /* pay no attention to the cast. */
    if(execv(args[0], (char **)args) == -1) {
      perror("couldn't exec:");
    }
  }
#endif
}

void alert_init(const char * adminemail) {
  if(adminemail) {
    strncpy(admin, adminemail, sizeof(admin));
  } else {
    const char *appendme = "@localhost";
    const char *username = getenv("USER");
    /* USER always seems to be defined... but if it isn't. maybe
       this would work? */
    if(!username) 
      username = getpwuid(getuid())->pw_name; 
    strncpy(admin, username, sizeof(admin) - strlen(appendme));
    strcat(admin, appendme);
  }
}
