/* Copyright (c) 2002 Neil Spring and the University of Washington. *
 * Distributed under the terms of the GNU General Public License,   *
 * version 2.  See the file COPYING for more details.               *
 * This software comes with NO warranty.                            *
 * end license */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include "config.h"
#include <stdio.h>
#include "nscommon.h"
boolean want_verbose = FALSE;

#ifndef HAVE_CHECK_H
int main(void) { printf("Install check from sourceforge.net for tests to work"); return(0); };
#else
#include <check.h>
#include <stdlib.h>
#include <string.h>
#include "sashimitimer.h"
#include "sashimi.h"

#define fail_expect_int(result,expect,msg) do { \
  char buf[255]; \
  int r = result; \
  sprintf(buf, "%s (got %d, expected %d)", msg, (r), (expect)); \
  _fail_unless((r)==(expect),__FILE__,__LINE__,buf); \
} while(0)

#define fail_expect_voidp(result,expect,msg) do { \
  char buf[255]; \
  void *r = result; \
  sprintf(buf, "%s (got %p, expected %p)", msg, (r), (expect)); \
  _fail_unless((r)==((void *)expect),__FILE__,__LINE__,buf); \
} while(0)


void compare_times(void *desired_time) {
  struct timeval *expected = (struct timeval *)desired_time;
  struct timeval now;
  struct timeval diff;
  gettimeofday(&now, NULL);
  timersub(&now, expected, &diff);
  printf("%lu:%06lu %s\n", (unsigned long)diff.tv_sec, 
         (unsigned long)diff.tv_usec,
         ( diff.tv_sec == 0 && diff.tv_usec < 1000) ? "nice" : "passable");
}

void exit_as_cb(void *dummy __attribute__((unused))) {
  fish_finish();
  // exit(0);
}

START_TEST(test_basic)
{
  struct timeval eventage[11];
  int i;
  printf("starting basic test\n");
  for(i=0; i< 10; i++) {
    gettimeofday(&eventage[i], NULL);
    // eventage[i].tv_sec += i;
    eventage[i].tv_usec += i * 10000;
    fish_scheduleevent_absolute(&eventage[i], compare_times, 
                                &eventage[i], TRUE);
  }
  fish_scheduleevent(11 * 10, exit_as_cb, 
                     0, TRUE);
  fish_main(500);
}
END_TEST

START_TEST(test_sub_ten)
{
  struct timeval eventage[11];
  int i;
  printf("starting scheduler test (if your system supports 1ms quanta)\n");
  
  for(i=0; i< 10; i++) {
    gettimeofday(&eventage[i], NULL);
    // eventage[i].tv_sec += i;
    eventage[i].tv_usec += i * 1000;
    fish_scheduleevent_absolute(&eventage[i], compare_times, 
                                &eventage[i], TRUE);
  }
  fish_scheduleevent(11 * 1, exit_as_cb, 
                     0, TRUE);
  fish_main(500);
}
END_TEST

Suite *test_suite (void) 
{ 
  Suite *s = suite_create("sashimi"); 
  TCase *tc_core = tcase_create("bottom");
 
  suite_add_tcase (s, tc_core);
 
  tcase_add_test (tc_core, test_basic); 
  tcase_add_test (tc_core, test_sub_ten); 
  return s; 
}
 
int main (int argc __attribute__((unused)), char **argv __attribute__((unused))) 
{ 
  int nf;
  Suite *s = test_suite(); 
  SRunner *sr = srunner_create(s); 

  srunner_run_all (sr, CK_NORMAL); 
  nf = srunner_ntests_failed(sr); 
  srunner_free(sr); 
#ifdef HAVE_CHECK_SUITE_FREE
  suite_free(s); 
#endif

  return (nf == 0) ? EXIT_SUCCESS : EXIT_FAILURE; 
}

#endif
