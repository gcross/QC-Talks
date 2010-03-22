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
#include <string.h>

#ifdef HAVE_CHECK_H
#include <check.h>

#define fail_expect_int(result,expect,msg) do { \
  char buf[255]; \
  int r = result; \
  sprintf(buf, "%s (got %d, expected %d)", msg, (r), (expect)); \
  _fail_unless((r)==(expect),__FILE__,__LINE__,buf); \
} while(0)


#include "string_bindings.h"


START_TEST(test_basic)
{
  unsigned short x;
  unsigned short y;
  string_bindings sb = strb_new(30);
  /* insert and check 1 */
  x = strb_bind(sb, strdup("x"));
  fail_expect_int(strb_lookup(sb, "x"), x, 
                  "didn't return expected");
  fail_unless(strcmp(strb_resolve(sb, x), "x") == 0, 
              "didn't resolve expected");
  /* failure */
  fail_unless(strb_resolve(sb, x+1) == NULL, 
              "didn't fail resolve with null");
  fail_expect_int(strb_lookup(sb, "y"), 0, 
                  "didn't fail lookup with zero");
  /* insert and check 2 */
  y = strb_bind(sb, strdup("y"));
  fail_expect_int(strb_lookup(sb, "y"), y, 
                  "didn't return expected y");
  fail_unless(strcmp(strb_resolve(sb, y), "y") == 0, 
              "didn't resolve expected y");
  /* verify 1 again */
  fail_expect_int(strb_lookup(sb, "x"), x, 
                  "didn't return expected");
  fail_unless(strcmp(strb_resolve(sb, x), "x") == 0, 
              "didn't resolve expected");
  /* done */
  strb_delete(sb);
}
END_TEST

Suite *ht_suite (void) 
{ 
  Suite *s = suite_create("string bindings"); 
  TCase *tc_core = tcase_create("bottom");
 
  suite_add_tcase (s, tc_core);
 
  tcase_add_test (tc_core, test_basic); 
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
