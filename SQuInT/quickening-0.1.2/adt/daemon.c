/* routines for running as a daemon.  It's possible that I've gotten
   this wrong in the past, by not reopening fd 0, 1, 2 to /dev/null
   and not calling setsid, so this refactoring should take care of
   that. */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include "log.h"

/* does not return if it fails to become a daemon. */
void become_a_daemon_or_die(void) {
#ifdef HAVE_DAEMON
  if(daemon(FALSE, FALSE) < 0) {
    goto Failed;
  }
#else
  /* implement only the logic described for 
     daemon(FALSE, FALSE) in this block */
  switch ( fork() ) {
  case -1: 
    goto Failed;
  case 0: 
    if (setsid() < 0) goto Failed;
    if (chdir("/") < 0) goto Failed;
    freopen("/dev/null", "r", stdin);
    freopen("/dev/null", "w", stdout);
    freopen("/dev/null", "w", stderr);
    break;
  default:
    exit(EXIT_SUCCESS);
  }
#endif
  return;
 Failed:
  /* who knows if this will be seen? */
  log_print(LOG_ERR, "unable to become a daemon: %s\n", strerror(errno));
  exit(EXIT_FAILURE);
}
