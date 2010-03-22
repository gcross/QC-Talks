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

#ifdef HAVE_CHECK_H
#include <check.h>

#include "socket_printf.h"
#include "sockaddr.h"

START_TEST(test_basic) {
  char *template;
  int fd;
  FILE *reader;
  char buf[512];
  template = strdup("/tmp/check_socket_printfXXXXXX");
  fd = mkstemp(template);
  socket_printf(fd, "hello %d", 1);
  close(fd);
  reader = fopen(template, "r");
  fgets(buf, 512, reader);
  fail_unless(strcmp(buf, "hello 1") == 0, "wrong contents");
  unlink(template);
}
END_TEST

START_TEST(test_ipv4addr) {
  fail_unless(  ipv4_addr_from_string("0.0.0.0") == 0, "zero not zero");
  fail_unless(  ipv4_addr_from_string("1.2.3.4") == htonl(0x01020304), "1234 not 1234");
  fail_unless(  ipv4_addr_from_string("1.2") == 0, "malformed not zero");
  fail_unless(  ipv4_addr_from_string("hello world") == 0, "string not zero");
}
END_TEST

Suite *ht_suite (void) 
{ 
  Suite *s = suite_create("socket printf"); 
  TCase *tc_core = tcase_create("bottom");
 
  suite_add_tcase (s, tc_core);
 
  tcase_add_test (tc_core, test_basic); 
  tcase_add_test (tc_core, test_ipv4addr); 
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
	return (EXIT_SUCCESS);
}


#endif
