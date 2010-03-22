/* Copyright (c) 2002 Neil Spring and the University of Washington. *
 * Distributed under the terms of the GNU General Public License,   *
 * version 2.  See the file COPYING for more details.               *
 * This software comes with NO warranty.                            *
 * end license */

#include <syslog.h>
/* defines: 
   LOG_EMERG LOG_ALERT LOG_CRIT LOG_ERR 
   LOG_WARNING LOG_NOTICE LOG_INFO LOG_DEBUG 
   don't call syslog directly! let log_print decide if
   messages should go to the console. */
#include <stdarg.h>
#include "nscommon.h"

boolean log_open(const char *programname, const char *facilityname);
void log_set_verbose();
void log_print(int level, const char *format, ... ) __attribute__((format(printf, 2, 3)));
void log_vprint(int level, const char *format, va_list args );
void log_bug(const char *format, ... );
