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
#include "config.h"
#endif
#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_CHECK_H
#include <check.h>
#include "sorted_intlist.h"


#define fail_expect_int(result,expect,msg) do { \
  char buf[255]; \
  int r = result; \
  sprintf(buf, "%s (got %d, expected %d)", msg, (r), (expect)); \
  _fail_unless((r)==(expect),__FILE__,__LINE__,buf); \
} while(0)


START_TEST(test_basic)
{
  int i;
  sorted_intlist si = si_new();
  fail_unless(si!=NULL, "Constructor returned null");
  for(i=1; i<20; i++) {
  //for(i=1; i<223; i++) {
    si_append(si, i);
    fail_expect_int(si_length(si), i, "length is wrong");
  }
  si_delete(si);
}
END_TEST

START_TEST(test_find)
{
  int i,j;
  sorted_intlist si = si_new();
  fail_unless(si!=NULL, "Constructor returned null");
  for(i=1; i<10; i++) {
    si_append(si, i);
    fail_expect_int(si_length(si), i, "length is wrong");
    for(j=1; j<i; j++) {
      fail_expect_int(si_member(si,j), TRUE, "didn't find");
    }
  }
  si_delete(si);
}
END_TEST
START_TEST(test_insrev)
{
  int i,j;
  sorted_intlist si = si_new();
  fail_unless(si!=NULL, "Constructor returned null");
  for(i=20; i>0; i--) {
    si_insert(si, i);
  }
  // si_print(si, stderr);
  for(j=1; j<=20; j++) {
    fail_expect_int(si_member(si,j), TRUE, "didn't find");
  }
  si_delete(si);
}
END_TEST
START_TEST(test_remove)
{
  int i,j;
  sorted_intlist si = si_new();
  fail_unless(si!=NULL, "Constructor returned null");
  for(i=20; i>0; i--) {
    si_insert(si, i);
  }
  for(j=1; j<=20; j+=2) {
    si_remove(si, j);
  }
  for(j=1; j<=20; j+=3) {
    if(j%2 == 0)
      fail_expect_int(si_member(si,j), TRUE, "didn't find");
    else
      fail_expect_int(si_member(si,j), FALSE, "wasn't removed");
  }
  si_delete(si);
}
END_TEST


Suite *si_suite (void) 
{ 
  Suite *s = suite_create("sorted intlist"); 
  TCase *tc_core = tcase_create("bottom");
 
  suite_add_tcase (s, tc_core);
 
  tcase_add_test (tc_core, test_basic); 
  tcase_add_test (tc_core, test_find); 
  tcase_add_test (tc_core, test_insrev); 
  tcase_add_test (tc_core, test_remove); 
  return s; 
}
 
int main (void) 
{ 
  int nf; 
  Suite *s = si_suite(); 
  SRunner *sr = srunner_create(s); 
  srunner_run_all (sr, CK_NORMAL); 
  nf = srunner_ntests_failed(sr); 
  srunner_free(sr); 
#ifdef HAVE_CHECK_SUITE_FREE 
  suite_free(s); 
#endif
  return (nf == 0) ? EXIT_SUCCESS : EXIT_FAILURE; 
}

#else
#warning "Install check from sourceforge.net and autoreconf; configure"
int main(void) {
	exit(EXIT_SUCCESS);
}
#endif
