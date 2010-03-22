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

#define fail_expect_int(result,expect,msg) do { \
  char buf[255]; \
  int r = result; \
  sprintf(buf, "%s (got %d, expected %d)", msg, (r), (expect)); \
  _fail_unless((r)==(expect),__FILE__,__LINE__,buf); \
} while(0)


#include "typed_hashtable.h"

DECLARE_TYPED_HASHTABLE(int, i);

unsigned int hash_i(const int *pi) {
  return ((unsigned int) *pi);
}
boolean isequal_i(const int *a, const int *b) {
  return (*a==*b);
}

START_TEST(test_basic)
{
  int i;
  int ary[20];
  i_hashtable ht = i_ht_new(20, hash_i, isequal_i, NULL);
  fail_unless(ht!=NULL, "Constructor returned null");
  for(i=1; i<20; i++) {
    ary[i] = i;
    i_ht_insert(ht, &ary[i]);
    fail_expect_int(i_ht_count(ht), i, "length is wrong");
  }
  i_ht_delete(ht, FALSE);
}
END_TEST

START_TEST(test_insrev)
{
  int i,j;
  int ary[20];
  i_hashtable ht = i_ht_new(20, hash_i, isequal_i, NULL);
  fail_unless(ht!=NULL, "Constructor returned null");
  for(i=20; i>0; i--) {
    ary[i]=i;
    i_ht_insert(ht, &ary[i]);
  }
  // ht_print(ht, stderr);
  for(j=1; j<=20; j++) {
    fail_unless(i_ht_lookup(ht,&j) == &ary[j], "didn't find");
  }
  i_ht_delete(ht, FALSE);
}
END_TEST

START_TEST(test_remove)
{
  int i,j;
  i_hashtable ht = i_ht_new(20, hash_i, isequal_i, NULL);
  int ary[20];
  fail_unless(ht!=NULL, "Constructor returned null");
  for(i=20; i>0; i--) {
    ary[i]=i;
    i_ht_insert(ht, &ary[i]);
  }
  for(j=1; j<=20; j+=2) {
    i_ht_remove(ht, &j);
  }
  for(j=1; j<=20; j+=3) {
    if(j%2 == 0)
      fail_unless(i_ht_lookup(ht,&j) == &ary[j], "wasn't found at the right place");
    else
      fail_unless(i_ht_lookup(ht,&j) == NULL, "wasn't removed");
  }
  i_ht_delete(ht, FALSE);
}
END_TEST

boolean deller(int *i, void *a __attribute__((unused))) {
  if((*i)%2 == 1) 
    return TRUE;
  else
    return FALSE;
}

START_TEST(test_delete_if)
{
  int i,j;
  i_hashtable ht = i_ht_new(20, hash_i, isequal_i, NULL);
  int ary[20];
  fail_unless(ht!=NULL, "Constructor returned null");
  for(i=20; i>0; i--) {
    ary[i]=i;
    i_ht_insert(ht, &ary[i]);
  }
  i_ht_delete_if(ht, deller, NULL);
  for(j=1; j<=20; j+=3) {
    if(j%2 == 0)
      fail_unless(i_ht_lookup(ht,&j) == &ary[j], "wasn't found at the right place");
    else
      fail_unless(i_ht_lookup(ht,&j) == NULL, "wasn't removed");
  }
  i_ht_delete(ht, FALSE);
}
END_TEST


Suite *ht_suite (void) 
{ 
  Suite *s = suite_create("typed hashtable"); 
  TCase *tc_core = tcase_create("bottom");
 
  suite_add_tcase (s, tc_core);
 
  tcase_add_test (tc_core, test_basic); 
  /* tcase_add_test (tc_core, test_find);  */
  tcase_add_test (tc_core, test_insrev); 
  tcase_add_test (tc_core, test_remove); 
  tcase_add_test (tc_core, test_delete_if); 
  return s; 
}
 
int main (void) 
{ 
  int nf; 
  Suite *s = ht_suite(); 
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
	return(EXIT_SUCCESS);
}


#endif
