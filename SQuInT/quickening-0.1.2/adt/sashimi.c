/* Copyright (c) 2002 Neil Spring and the University of Washington. *
 * Distributed under the terms of the GNU General Public License,   *
 * version 2.  See the file COPYING for more details.               *
 * This software comes with NO warranty.                            *
 * end license */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <assert.h>
#include "sashimi.h"
#include "sashimitimer.h"
#include "nscommon.h"
#include "log.h"

/* routines and structures from fishguts */
static struct select_binding {
  struct select_binding *next;
  int sd;
  void (*handler)(int);
} /*@null@*/ *read_binding_list, *write_binding_list;

static int fish_constructmask(struct select_binding *psb, fd_set *prd);
static void fish_processbindings(struct select_binding *psb, fd_set *prd);

static int fish_done = FALSE;

void fish_finish(void) { 
  fish_done = TRUE;
}

void fish_main(unsigned int busy_wait_usec /* from configuration */) {
  while (!fish_done) {
    fd_set rd, wd;
    int max = 0;
    struct timeval tv_wait, *waitptr;
    int ok;
   
    FD_ZERO(&rd);
    max = fish_constructmask(read_binding_list, &rd);
    FD_ZERO(&wd);
    if( fish_time_to_next_event( &tv_wait ) ) {
      if ( tv_wait.tv_sec == 0 && (unsigned int)tv_wait.tv_usec < busy_wait_usec ) {
        /* busy wait mode, make select return immediately
           even if nothing is ready. */
        tv_wait.tv_sec = 0; 
        tv_wait.tv_usec = 0; 
      }
      waitptr = &tv_wait;
    } else {
      /* wait indefinitely mode */
      waitptr = NULL;
    }
    max = max(max,fish_constructmask(write_binding_list, &wd));
    ok = select(max+1, &rd, &wd, NULL, waitptr);
                
    if (ok < 0) {
      if(errno != EINTR) {
        if(errno == EBADF) {                                                   
          int i;                                                               
          log_print(LOG_DEBUG, "Bad file descriptor error from select()\n");
          for(i=0; i<max; i++) {                                               
            if(FD_ISSET(i, &rd)  && fcntl(i, F_GETFL) < 0) {                   
              log_print(LOG_NOTICE, 
                        " read file descriptor #%d is suspicious: %s\n",
                     i, strerror(errno));                                      
              fish_internal_removereadhook(i); /* pray */
            }                                                                  
            if(FD_ISSET(i, &wd)  && fcntl(i, F_GETFL) < 0) {                   
              log_print(LOG_NOTICE, 
                        " write file descriptor #%d is suspicious: %s\n",
                     i, strerror(errno));                                      
              fish_internal_removewritehook(i); /* pray */
            }                                                                  
          }                                                                    
        } else if(errno==EINVAL) {
          int i;
          log_print(LOG_DEBUG, "invalid argument? but max is %d\n", max);
          for(i=0; i<max; i++) {                                               
            if(FD_ISSET(i, &rd)  && fcntl(i, F_GETFL) < 0) {                   
              log_print(LOG_DEBUG, 
                        "  file descriptor #%d is suspicious: %s\n",
                     i, strerror(errno));                                      
              fish_internal_removereadhook(i); /* pray */
            }                                                                  
          }                                                                    
        } else {
          perror("select");
          abort();
        }
        log_print(LOG_DEBUG, "trying to recover.\n");
      } else {
        continue;
      }
    } /* end select() failure handling */
   
    if(ok > 0) {
      fish_processbindings(read_binding_list, &rd);
      fish_processbindings(write_binding_list, &wd);
    }
    fish_executepending();
  }
}

static struct select_binding *
new_sb(int sd, void (*fn)(int)) {
  NEWPTR(struct select_binding, psb);
  assert(psb != NULL);
  assert(sd >= 0);
  psb->sd = sd;
  psb->handler = fn;
  return(psb);
}

void fish_internal_readhook(int sd, void (*rdfn)(int)) {
  struct select_binding *psb = new_sb(sd, rdfn);
  psb->next = read_binding_list;
  read_binding_list = psb;
}

void fish_internal_writehook(int sd, void (*wrfn)(int)) {
  struct select_binding *psb = new_sb(sd, wrfn);
  psb->next = write_binding_list;
  write_binding_list = psb;
}

void fish_internal_removereadhook(int sd) {
  struct select_binding *psb, *qsb;
  for(psb = read_binding_list, qsb = NULL;
      psb != NULL && psb->sd != sd;
      qsb=psb, psb=psb->next);

  if(psb != NULL) { /* found it */
    if(qsb != NULL) { /* in the middle */
      qsb->next = psb->next;
    } else { /* at the beginning */
      read_binding_list = psb->next;
    }
    free(psb);
  }
}
void fish_internal_removewritehook(int sd) {
  struct select_binding *psb, *qsb;
  for(psb = write_binding_list, qsb = NULL;
      psb != NULL && psb->sd != sd;
      qsb=psb, psb=psb->next);

  if(psb != NULL) { /* found it */
    if(qsb != NULL) { /* in the middle */
      qsb->next = psb->next;
    } else { /* at the beginning */
      write_binding_list = psb->next;
    }
    free(psb);
  }
}

static int fish_constructmask(struct select_binding *psb, fd_set *prd) {
  int max=0;
  for( ; 
      psb != NULL;
      psb = psb->next) {
    if(psb->sd < 65000) {
      FD_SET(psb->sd, prd);
    } else {
      log_print(LOG_ERR, "constructmask failure!\n");
      abort();
    }
    max = max(psb->sd,max);
  }
  return(max);
}


static void fish_processbindings(struct select_binding *psb, fd_set *prd) {
  struct select_binding *next;
  for( ; psb != NULL; psb = next) {
    next = psb->next; /* grab this now, in case we remove
                         ourselves from the list */
    if(psb->sd >= 0 && psb->sd < 4096) {
      if(FD_ISSET(psb->sd, prd)) {
        psb->handler(psb->sd);
        // return;
      }
    } else {
      log_print(LOG_ERR, "(bug) something got corrupted\n");
    }
  }
}

