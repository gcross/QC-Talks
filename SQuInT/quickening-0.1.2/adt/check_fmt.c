/* Copyright (c) 2002 Neil Spring and the University of Washington. *
 * Distributed under the terms of the GNU General Public License,   *
 * version 2.  See the file COPYING for more details.               *
 * This software comes with NO warranty.                            *
 * end license */
#include "config.h"
#include <stdio.h>
#ifndef HAVE_CHECK_H
int main(void) { printf("Install check from sourceforge.net for tests to work"); return(0); };
#else
#include <check.h>
#include <stdlib.h>
#include <string.h>
#include "nscommon.h"
#include "lamefmt.h"

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

START_TEST(test_basic)
{
  fail_expect_int(strcmp("hello my\nname is", fmt_buffer("hello my name is", 12)), 0, "twelve");
  fail_expect_int(strcmp("hello my name\nis", fmt_buffer("hello my name is", 15)), 0, "fifteen");
  fail_expect_int(strcmp("hellomyname\nis", fmt_buffer("hellomyname\nis", 6)), 0, "six");
  fail_expect_int(strcmp("hello my name is", fmt_buffer("hello my name is", 30)), 0, "thirty");
}
END_TEST

Suite *test_suite (void) 
{ 
  Suite *s = suite_create("fmt"); 
  TCase *tc_core = tcase_create("bottom");
 
  suite_add_tcase (s, tc_core);
 
  tcase_add_test (tc_core, test_basic); 
  return s; 
}
 
int main (int argc, char **argv __attribute__((unused))) 
{ 
  int nf; 
  Suite *s = test_suite(); 
  SRunner *sr = srunner_create(s); 
  if(argc > 1) { 
    srunner_set_fork_status (sr, CK_NOFORK);  
    //    want_debug = TRUE;
  }
  srunner_run_all (sr, CK_NORMAL); 
  nf = srunner_ntests_failed(sr); 
  srunner_free(sr); 
#ifdef HAVE_CHECK_SUITE_FREE
  suite_free(s); 
#endif
  return (nf == 0) ? EXIT_SUCCESS : EXIT_FAILURE; 
}


#endif
