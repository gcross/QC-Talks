
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
#include <unistd.h>
#include <string.h>
#include <sys/time.h>

#ifdef HAVE_CHECK_H
#include <check.h>

#include "filetest.h"

START_TEST(test_filetest) {
  struct timeval way_back;
  struct timeval now;
  fail_unless(searchPath(".", "check_misc") != NULL, "can't find myself");
  fail_unless(searchPath("/bin", "sh") != NULL, "can't find sh");
  fail_unless(searchPath("/usr/bin:/bin", "sh") != NULL, "can't find sh part 2");
  fail_unless(searchPath("/usr/include:/usr/include/sys", "stdio.h") != NULL, "can't find stdio.h");
  way_back.tv_sec = 12;
  way_back.tv_usec = 0;

  fail_unless(fileIsNewerThan("/usr/include/stdio.h", &way_back), "usr/include/stdio.h too old?");
  gettimeofday(&now, NULL);
  fail_unless(!fileIsNewerThan("/usr/include/stdio.h", &now), "usr/include/stdio.h too new?");
}
END_TEST

Suite *ht_suite (void) 
{ 
  Suite *s = suite_create("miscellaneous"); 
  TCase *tc_core = tcase_create("bottom");
 
  suite_add_tcase (s, tc_core);
 
  tcase_add_test (tc_core, test_filetest); 
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
  return ((nf == 0) ? EXIT_SUCCESS : EXIT_FAILURE);
}

#else
#warning "Install check from sourceforge.net and autoreconf; configure"
int main(void) {
	return (EXIT_SUCCESS);
}


#endif
