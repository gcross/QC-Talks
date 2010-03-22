/* Copyright (c) 2002 Neil Spring and the University of Washington. *
 * Distributed under the terms of the GNU General Public License,   *
 * version 2.  See the file COPYING for more details.               *
 * This software comes with NO warranty.                            *
 * end license */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <stdio.h>
#ifndef HAVE_CHECK_H
int main(void) { printf("Install check from sourceforge.net for tests to work"); return(0); };
#else
#include <check.h>
#include <string.h>
#include <stdlib.h>
#include "base64.h"
#include "nscommon.h"

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
  const char *strings[] = { "hello", "one", "k00l", 
                            "thegang", "n\000ll",  
                            "\002\133xx", NULL };
  struct { 
    const char *bindat;
    unsigned int len;
  } binaries[] = {
    { "n\000ll", 4 },
    { "En\000ll", 4 },
    { NULL, 0 } 
  };
  char plain[128];
  char out[128];
  int i;

  for(i=0; strings[i] != NULL; i++) {
    const char *s = strings[i];
    Encode_Base64(s, strlen(s), out);
    fail_unless(strlen(s) ==  Decode_Base64(out, plain), "wrong number of chars decoded");
    fail_unless(strcmp(plain, s) == 0, "didn't get back what we wanted");
  }

  for(i=0; binaries[i].bindat != NULL; i++) {
    const char *s = binaries[i].bindat;
    Encode_Base64(s, binaries[i].len, out);
    fail_unless(binaries[i].len == Decode_Base64(out, plain), "wrong number of chars decoded");
    fail_unless(memcmp(plain, s, binaries[i].len) == 0, "didn't get back what we wanted");
  }
}
END_TEST

Suite *test_suite (void) 
{ 
  Suite *s = suite_create("base64"); 
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
