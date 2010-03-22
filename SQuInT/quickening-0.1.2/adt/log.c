/* Copyright (c) 2002 Neil Spring and the University of Washington. *
 * Distributed under the terms of the GNU General Public License,   *
 * version 2.  See the file COPYING for more details.               *
 * This software comes with NO warranty.                            *
 * end license */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <stdlib.h>
#include <syslog.h>
#include <string.h>
#include <sys/time.h>
#include <stdarg.h>
#include <time.h>
#include "log.h"

/* taken from /usr/include/sys/syslog.h on linux; 
   unfortunately defined without const char *. */

struct _code {
  /*@null@*/ const char	*c_name;
	int	c_val;
} facilitynames[] =
  {
    { "auth", LOG_AUTH },
#ifdef LOG_AUTHPRIV
    /* solaris lacks LOG_AUTHPRIV...  I don't care so much. */
    { "authpriv", LOG_AUTHPRIV },
#endif
    { "cron", LOG_CRON },
    { "daemon", LOG_DAEMON },
#ifdef LOG_FTP
    /* solaris lacks LOG_FTP...  (again) I don't care so much. */
    { "ftp", LOG_FTP },
#endif
    { "kern", LOG_KERN },
    { "lpr", LOG_LPR },
    { "mail", LOG_MAIL },
    /* { "mark", INTERNAL_MARK },		INTERNAL */
    { "news", LOG_NEWS },
    { "security", LOG_AUTH },		/* DEPRECATED */
    { "syslog", LOG_SYSLOG },
    { "user", LOG_USER },
    { "uucp", LOG_UUCP },
    { "local0", LOG_LOCAL0 },
    { "local1", LOG_LOCAL1 },
    { "local2", LOG_LOCAL2 },
    { "local3", LOG_LOCAL3 },
    { "local4", LOG_LOCAL4 },
    { "local5", LOG_LOCAL5 },
    { "local6", LOG_LOCAL6 },
    { "local7", LOG_LOCAL7 },
    { NULL, -1 }
  };

boolean use_syslog;
static boolean want_verbose;
/*@null@*/ /*@dependent@*/ FILE *fpFakeSyslog;

static const char *
/*@dependent@*/ time_now(void) {
  static char buf[16];
  struct timeval tv;
  (void) gettimeofday(&tv, NULL);
  (void) strftime (buf, 16, "%b %e %H:%M:%S", localtime((time_t *)&tv.tv_sec));
  buf[15] = '\0';
  return buf;
}

extern void log_set_verbose() {
  want_verbose = TRUE;
}

extern boolean log_open(const char *programname, const char *facilityname) {
  int facilitynum;

  if(facilityname[0] == '/')  {
    fpFakeSyslog = fopen(facilityname, "a");
    if(fpFakeSyslog != NULL) {
      fprintf(fpFakeSyslog, "log opened at %s\n", time_now());
      (void) fflush(fpFakeSyslog);
      return TRUE;
    } else {
      fprintf(stderr, "unable to open fake syslog %s\n", facilityname);
      (void) fflush(fpFakeSyslog);
      return FALSE;
    }
  } else {
    for(facilitynum = 0; facilitynames[facilitynum].c_name != NULL &&
          strcasecmp(facilitynames[facilitynum].c_name, facilityname) != 0;
        facilitynum++);
    if(facilitynames[facilitynum].c_name != NULL) {
      openlog(programname, 0, facilitynum);
      use_syslog = TRUE;
      return TRUE;
    } else {
      return FALSE;
    }
  }
}

void log_vprint(int level, const char *format, va_list ap ) {
  if(use_syslog) {
    vsyslog(level, format, ap);
  } else if(fpFakeSyslog != NULL)  {
    (void)fprintf(fpFakeSyslog, "%s ", time_now()); 
    (void)vfprintf(fpFakeSyslog, format, ap); 
    if(format[strlen(format)-1] != '\n') {
      (void)fprintf(fpFakeSyslog, "\n"); 
    }
    (void)fflush(fpFakeSyslog);
  } else {
    /* log messages don't need newlines, but if printed to the console, they should */
    (void)vfprintf(stderr, format, ap); 
  }
}

void log_print(int level, const char *format, ... ) {
  va_list ap;
  if(level < LOG_INFO || want_verbose) {
    va_start(ap,format);
    log_vprint(level, format, ap);
    va_end(ap);
  }
}

/* special version for printing bug information.  Because a
   bug's presence might be a surprise if in a recent
   version, try to print the version of the package first.
   Not sure this is right (more general to take the version
   as a parameter, but more convenient this way for now). */
#ifndef PACKAGE_VERSION
#define PACKAGE_VERSION ""
#warning "PACKAGE_VERSION undefined"
#endif
   
void log_bug(const char *format, ... ) {
  char newfmt[255];
  va_list ap;
  snprintf(newfmt, 255, "(bug! v" PACKAGE_VERSION ") %s", format);
  va_start(ap,format);
  log_vprint(LOG_ERR, newfmt, ap);
  va_end(ap);
}
