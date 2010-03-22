/* routines for running as a daemon.  It's possible that I've gotten
   this wrong in the past, by not reopening fd 0, 1, 2 to /dev/null
   and not calling setsid, so this refactoring should take care of
   that. */

/* will not return if it fails to become a daemon */
void  become_a_daemon_or_die(void);
