#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#ifndef HAVE_CHECK_H
#warning "Install check from sourceforge.net"
int main(void) { return(0); };
#else
#include <check.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <stdlib.h>
#include "nscommon.h"
#include "radix.h"

boolean want_debug = FALSE;

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

START_TEST(test_new)
{
  struct patricia_table * rt = patricia_new();
  fail_unless(rt!=NULL, "Constructor returned null");
  patricia_delete(rt, NULL);
}
END_TEST

START_TEST(test_insert)
{
  struct patricia_table * rt = patricia_new();
  fail_unless(rt!=NULL, "Constructor returned null");
  patricia_insert(rt, 1, 32, (void *)2);
  fail_unless(patricia_lookup(rt, 1) == (void *)2, "Lookup returned wrong interface.");

  /* on duplicates, only the oldest NOT THE NEWEST is kept */
  /* if you want to remove the old one, implement removal from the
     trie. ;) */
  patricia_insert(rt, 1, 32, (void *)2);
  patricia_insert(rt, 1, 32, (void *)3);
  patricia_insert(rt, 1, 32, (void *)4);
  patricia_insert(rt, 1, 32, (void *)5);
  patricia_insert(rt, 1, 32, (void *)6);
  fail_unless(patricia_lookup(rt, 1) == (void *)2, "Lookup of replaced returned wrong interface.");

  patricia_delete(rt, NULL);
}
END_TEST

START_TEST(test_nonexistent)
{
  struct patricia_table * rt = patricia_new();
  fail_unless(rt!=NULL, "Constructor returned null");
  patricia_insert(rt, 1, 32, (void*)2);
  fail_unless(patricia_lookup(rt, 2) == 0, "Lookup returned an interface when it shouldn't.");
  patricia_delete(rt, NULL);
}
END_TEST

START_TEST(test_mask) 
{
  fail_unless(radix_mask(htonl(1), 24) == 0, "/24 is wrong");
  fail_unless(radix_mask(htonl(1), 31) == 0, "/31 is wrong");
  fail_unless(radix_mask(htonl(1), 32) == htonl(1), "/32 is wrong");
}
END_TEST

START_TEST(test_test) 
{
  free(patricia_new()); /* get the initializer called */
  fail_unless(radix_test_export(-1, 32) != 0, "32 is wrong (should match the last bit)");
  //fail_unless(radix_test(-1, 0)  != 0, "0 is wrong (should be an implicit match)");
  fail_unless(radix_test_export(-1, 1)  != 0, "1 is wrong (should match the first bit)");
  fail_unless(radix_test_export(-1, 31) != 0, "31 is wrong (should match the penultimate bit)");
}
END_TEST

START_TEST(test_prefix)
{
  struct patricia_table * rt = patricia_new();
  fail_unless(rt!=NULL, "Constructor returned null");
  patricia_insert(rt, ntohl(0), 31, (void *)1);
  patricia_insert(rt, ntohl(2), 31, (void *)2);
  //printf("\n");
  //route_print_table_text(rt, stderr);
  //route_print_table_dot(rt, stderr);
  fail_unless(patricia_lookup(rt, ntohl(0)) == (void *)1, "In subnet failed (0)");
  fail_unless(patricia_lookup(rt, ntohl(1)) == (void *)1, "In subnet failed (1)");
  fail_unless(patricia_lookup(rt, ntohl(2)) == (void *)2, "In subnet failed (2)");
  // printf("foible %d\n", patricia_lookup(rt, ntohl(3)));
  fail_unless(patricia_lookup(rt, ntohl(3)) == (void *)2, "In subnet failed (3)");
  fail_unless(patricia_lookup(rt, ntohl(4)) == (void *)0, "out of subnet failed");
  patricia_delete(rt, NULL);
}
END_TEST

START_TEST(test_default) 
{
  struct patricia_table * rt = patricia_new();
  fail_unless(rt!=NULL, "Constructor returned null");
  patricia_insert(rt, htonl(0), 0, (void *)10);
  patricia_insert(rt, htonl(0), 31, (void *)1);
  patricia_insert(rt, htonl(2), 31, (void *)2);
  fail_unless(patricia_lookup(rt, htonl(0)) == (void *)1, "In subnet failed");
  fail_unless(patricia_lookup(rt, htonl(1)) == (void *)1, "In subnet failed");
  fail_unless(patricia_lookup(rt, htonl(2)) == (void *)2, "In subnet failed");
  fail_unless(patricia_lookup(rt, htonl(3)) == (void *)2, "In subnet failed");
  // printf("\n");
  // route_print_table_text(rt, stderr);
  // route_print_table_dot(rt, stderr);
  // printf("fraggle rock %d\n", patricia_lookup(rt, htonl(4)));
  fail_unless(patricia_lookup(rt, htonl(4)) == (void *)10, "default failed");
  fail_unless(patricia_lookup(rt, htonl(5)) == (void *)10, "default failed");
  fail_unless(patricia_lookup(rt, htonl(6)) == (void *)10, "default failed");
  fail_unless(patricia_lookup(rt, htonl(10000)) == (void *)10, "default failed");
  patricia_delete(rt, NULL);
}
END_TEST

START_TEST(test_yuri_one)
{
  struct patricia_table * rt = patricia_new();
  fail_unless(rt!=NULL, "Constructor returned null");
  patricia_insert(rt, htonl(1), 32, (void *)1);
  patricia_insert(rt, htonl(0), 31, (void *)3);
  patricia_insert(rt, htonl(0), 24, (void *)2);
  fail_expect_voidp(patricia_lookup(rt, htonl(1)), (void *)1, "wrong interface");
  fail_expect_voidp(patricia_lookup(rt, htonl(2)), (void *)2, "wrong interface");
  fail_expect_voidp(patricia_lookup(rt, htonl(0)), (void *)3, "wrong interface");
  fail_expect_voidp(patricia_lookup(rt, htonl(5)), (void *)2, "wrong interface (reported)");
  patricia_delete(rt, NULL);
}
END_TEST

START_TEST(test_yuri_two)
{
  struct patricia_table * rt = patricia_new();
  fail_unless(rt!=NULL, "Constructor returned null");
  patricia_insert(rt, htonl(0), 28, (void *)3);
  patricia_insert(rt, htonl(4), 30, (void *)2);
  patricia_insert(rt, htonl(6), 32, (void *)4);
  fail_expect_voidp(patricia_lookup(rt, htonl(1)), (void *)3, "wrong interface");
  fail_expect_voidp(patricia_lookup(rt, htonl(2)), (void *)3, "wrong interface");
  fail_expect_voidp(patricia_lookup(rt, htonl(0)), (void *)3, "wrong interface");
  fail_expect_voidp(patricia_lookup(rt, htonl(5)), (void *)2, "wrong interface");
  fail_expect_voidp(patricia_lookup(rt, htonl(7)), (void *)2, "wrong interface (reported)");
  patricia_delete(rt, NULL);
}
END_TEST

START_TEST(test_specific_first)
{
  struct patricia_table * rt = patricia_new();
  fail_unless(rt!=NULL, "Constructor returned null");
  patricia_insert(rt, htonl(1), 32, (void *)1);
  patricia_insert(rt, htonl(0), 28, (void *)14);
  fail_unless(patricia_lookup(rt, htonl(1)) == (void *)1, "wrong interface");
  fail_unless(patricia_lookup(rt, htonl(2)) == (void *)14, "wrong interface");
  fail_unless(patricia_lookup(rt, htonl(0)) == (void *)14, "wrong interface");
  fail_unless(patricia_lookup(rt, htonl(5)) == (void *)14, "wrong interface");
  patricia_delete(rt, NULL);
}
END_TEST

START_TEST(test_specific_last)
{
  struct patricia_table * rt = patricia_new();
  fail_unless(rt!=NULL, "Constructor returned null");
  patricia_insert(rt, htonl(0), 28, (void *)14);
  patricia_insert(rt, htonl(1), 32, (void *)1);
  fail_expect_voidp(patricia_lookup(rt, htonl(1)), (void *)1, "wrong interface");
  fail_expect_voidp(patricia_lookup(rt, htonl(2)), (void *)14, "didn't match less specific");
  fail_expect_voidp(patricia_lookup(rt, htonl(0)), (void *)14, "didn't match less specific");
  fail_expect_voidp(patricia_lookup(rt, htonl(5)), (void *)14, "didn't match less specific");
  patricia_delete(rt, NULL);
}
END_TEST

/* I don't think this actually tests anything, and I'm not sure what I was 
   trying to do */
START_TEST(test_specific_last2)
{
  struct patricia_table * rt = patricia_new();
  fail_unless(rt!=NULL, "Constructor returned null");
  patricia_insert(rt, htonl(0), 8, (void *)8);
  patricia_insert(rt, htonl(0), 30, (void *)14);
  patricia_insert(rt, htonl(32<<24), 24, (void *)1);
  fail_expect_voidp(patricia_lookup(rt, htonl(1)), (void *)14, "wrong interface");
  fail_expect_voidp(patricia_lookup(rt, htonl(2)), (void *)14, "didn't match less specific");
  fail_expect_voidp(patricia_lookup(rt, htonl(0)), (void *)14, "didn't match less specific");
  fail_expect_voidp(patricia_lookup(rt, htonl(5)), (void *)8, "didn't match less specific");
  patricia_delete(rt, NULL);
}
END_TEST

START_TEST(test_heavy)
{
  struct patricia_table * rt = patricia_new();
  fail_unless(rt!=NULL, "Constructor returned null");
  patricia_insert(rt, htonl(1), 32, (void *)1);
  patricia_insert(rt, htonl(2), 32, (void *)2);
  patricia_insert(rt, htonl(3), 32, (void *)3);
  patricia_insert(rt, htonl(4), 32, (void *)4);
  patricia_insert(rt, htonl(5), 32, (void *)5);
  mark_point(); patricia_insert(rt, htonl(6), 32, (void *)6); 
  /* may cause assertion failures */
  mark_point(); patricia_insert(rt, htonl(4), 30, (void *)12); 
  mark_point(); patricia_insert(rt, htonl(0), 28, (void *)14);
  mark_point(); patricia_insert(rt, htonl(0), 29, (void *)14);

  // patricia_dump(rt, "heavy.dot");
  
  fail_unless(patricia_lookup(rt, htonl(1)) == (void *)1, "wrong interface"); 
  fail_unless(patricia_lookup(rt, htonl(2)) == (void *)2, "wrong interface");
  fail_unless(patricia_lookup(rt, htonl(3)) == (void *)3, "wrong interface");
  fail_unless(patricia_lookup(rt, htonl(4)) == (void *)4, "wrong interface");
  fail_unless(patricia_lookup(rt, htonl(5)) == (void *)5, "wrong interface");
  fail_unless(patricia_lookup(rt, htonl(6)) == (void *)6, "wrong interface");
  fail_unless(patricia_lookup(rt, htonl(7)) == (void *)12, "wrong interface");
  fail_unless(patricia_lookup(rt, htonl(8)) == (void *)14, "wrong interface");
  fail_unless(patricia_lookup(rt, htonl(9)) == (void *)14, "wrong interface");
  fail_unless(patricia_lookup(rt, htonl(10)) ==(void *) 14, "wrong interface");
  fail_unless(patricia_lookup(rt, htonl(11)) ==(void *) 14, "wrong interface");
  fail_unless(patricia_lookup(rt, htonl(12)) ==(void *) 14, "wrong interface");
  fail_unless(patricia_lookup(rt, htonl(13)) ==(void *) 14, "wrong interface");
  patricia_delete(rt, NULL);
}
END_TEST

START_TEST(test_specificity)
{
  struct patricia_table * rt = patricia_new();
  fail_unless(rt!=NULL, "Constructor returned null");
  patricia_insert(rt, htonl(0), 28, (void *)4);
  patricia_insert(rt, htonl(0), 27, (void *)5);
  patricia_insert(rt, htonl(0), 26, (void *)6);
  patricia_insert(rt, htonl(0), 25, (void *)7);
  patricia_insert(rt, htonl(0), 29, (void *)3);
  patricia_insert(rt, htonl(0), 30, (void *)2);
  patricia_insert(rt, htonl(0), 31, (void *)1);
  // patricia_dump(rt, "spec.dot");
  fail_expect_voidp(patricia_lookup(rt, htonl(1)), (void *)1, "wrong interface");
  fail_expect_voidp(patricia_lookup(rt, htonl(2)), (void *)2, "wrong interface");
  fail_expect_voidp(patricia_lookup(rt, htonl(4)), (void *)3, "wrong interface");
  fail_expect_voidp(patricia_lookup(rt, htonl(8)), (void *)4, "wrong interface");
  fail_expect_voidp(patricia_lookup(rt, htonl(16)), (void *)5, "wrong interface");
  fail_expect_voidp(patricia_lookup(rt, htonl(32)), (void *)6, "wrong interface");
  fail_expect_voidp(patricia_lookup(rt, htonl(64)), (void *)7, "wrong interface");
  patricia_delete(rt, NULL);
}
END_TEST

START_TEST(test_subtle)
{
  struct patricia_table * rt = patricia_new();
  fail_unless(rt!=NULL, "Constructor returned null");
  patricia_insert(rt, htonl(0), 8, (void *)4);
  patricia_insert(rt, htonl(0), 24, (void *)5);
  patricia_insert(rt, htonl(0), 25, (void *)6);
  patricia_insert(rt, htonl(32<<24), 24, (void *)7);
  patricia_delete(rt, NULL);
}
END_TEST

START_TEST(test_subtle2)
{
  struct patricia_table * rt = patricia_new();
  fail_unless(rt!=NULL, "Constructor returned null");
  patricia_insert(rt, htonl(0), 8, (void *)4);
  patricia_insert(rt, htonl(0), 24, (void *)5);
  patricia_insert(rt, htonl(0), 25, (void *)6);
  patricia_insert(rt, htonl(32<<24), 24, (void *)7);
  patricia_insert(rt, htonl(128<<24), 24, (void *)8);
  patricia_insert(rt, htonl(0), 20, (void *)9);
  patricia_insert(rt, htonl(0), 16, (void *)10);
  patricia_insert(rt, htonl(0), 22, (void *)11);
  patricia_insert(rt, htonl(0), 18, (void *)12);
  patricia_insert(rt, htonl(0), 19, (void *)3);
  // patricia_dump(rt, "sub2.dot");
  patricia_delete(rt, NULL);
}
END_TEST

START_TEST(test_subtle3)
{
  struct patricia_table * pt = patricia_new();
  fail_unless(pt!=NULL, "Constructor returned null");

  patricia_insert(pt, 0x0, 8, (void *)0xd);
  patricia_insert(pt, 0x0, 24, (void *)0x156b);
  patricia_insert(pt, 0x0, 25, (void *)0x2bd);
  patricia_insert(pt, 0x20, 24, (void *)0x4db0);
  patricia_insert(pt, 0x80, 24, (void *)0x4db0);
  patricia_insert(pt, 0x0, 20, (void *)0xddd);
  patricia_insert(pt, 0x0, 16, (void *)0x5af);
  patricia_insert(pt, 0x0, 22, (void *)0x5af);
  patricia_insert(pt, 0x0, 18, (void*)0x5af);
  patricia_insert(pt, 0x0, 19, (void*)0x5af);
  patricia_insert(pt, 0x6, 20, (void*)0x5af);
  patricia_insert(pt, 0xc, 20, (void*)0x5af);
  patricia_insert(pt, 0x18, 15, (void*)0x5af);
  patricia_insert(pt, 0x80, 15, (void*)0x5af);
  patricia_insert(pt, 0x9, 20, (void*)0xeca);
  patricia_insert(pt, 0x24, 20, (void*)0xeca);
  patricia_insert(pt, 0x0, 32, (void*)0x26d6);
  patricia_insert(pt, 0x80, 25, (void*)0xfa6);
  patricia_insert(pt, 0x0, 23, (void*)0x421f);
  patricia_insert(pt, 0x0, 21, (void*)0x2f83);
  patricia_insert(pt, 0x30, 25, (void*)0x4d89);
  patricia_insert(pt, 0xc00, 24, (void*)0x2ab9);
  patricia_insert(pt, 0xc000, 24, (void*)0x1fcf);
  patricia_insert(pt, 0x8001, 24, (void*)0x578f);
  patricia_insert(pt, 0x800100, 24, (void*)0x1b6a);
  patricia_insert(pt, 0x600, 24, (void*)0x1b6a);
  patricia_insert(pt, 0x60, 24, (void*)0x4897);
  patricia_insert(pt, 0x300, 23, (void*)0x2d28);
  patricia_insert(pt, 0x600000, 24, (void*)0x1b6a);
  patricia_insert(pt, 0x3000, 21, (void*)0x2ac3);
  patricia_insert(pt, 0xc, 24, (void*)0x2cf4);
  patricia_insert(pt, 0x300000, 24, (void*)0x1dd1);
  patricia_insert(pt, 0x6000, 24, (void*)0x421c);
  patricia_insert(pt, 0x3, 23, (void*)0x4aed);
  patricia_insert(pt, 0x300000, 23, (void*)0x1b6a);
  patricia_insert(pt, 0x300, 24, (void*)0x1b6a);
  patricia_insert(pt, 0x1800, 24, (void*)0x1b6a);
  patricia_insert(pt, 0x3000, 23, (void*)0x1b6a);
  patricia_insert(pt, 0x80, 23, (void*)0x1921);
  patricia_insert(pt, 0x3000, 24, (void*)0x37bb);
  patricia_insert(pt, 0x6, 24, (void*)0x1b6a);
  patricia_insert(pt, 0xc0, 17, (void*)0x18a4);
  patricia_insert(pt, 0xc0, 18, (void*)0x18a4);
  patricia_insert(pt, 0xc00000, 22, (void*)0x18a4);
  patricia_insert(pt, 0x3, 24, (void*)0x43c4);
  patricia_insert(pt, 0x1800, 22, (void*)0x19b4);
  patricia_insert(pt, 0x80, 21, (void*)0xc4);
  patricia_insert(pt, 0x80, 19, (void*)0xc4);
  patricia_insert(pt, 0x800100, 22, (void*)0xc4);
  patricia_insert(pt, 0x1800, 23, (void*)0xc4);
  patricia_insert(pt, 0xc00, 23, (void*)0x2c77);
  patricia_insert(pt, 0x3000, 22, (void*)0x2c77);
  patricia_insert(pt, 0x3, 21, (void*)0x2c77);
  patricia_insert(pt, 0xc0, 23, (void*)0x39ee);
  patricia_insert(pt, 0x8001, 23, (void*)0x2e47);
  patricia_insert(pt, 0xc0, 24, (void*)0x2d0c);
  patricia_insert(pt, 0x18, 24, (void*)0x952);
  patricia_insert(pt, 0x30, 24, (void*)0x952);
  patricia_insert(pt, 0x18, 23, (void*)0x1b6a);
  patricia_insert(pt, 0x1800, 20, (void*)0x1b6a);
  patricia_insert(pt, 0x30, 23, (void*)0x1b6a);
  patricia_insert(pt, 0xc000, 23, (void*)0x1b6a);
  patricia_insert(pt, 0x3, 22, (void*)0x2ddd);
  patricia_insert(pt, 0xc000, 22, (void*)0x295c);
  patricia_insert(pt, 0xc, 22, (void*)0x952);
  patricia_insert(pt, 0x6, 23, (void*)0x2b52);
  patricia_insert(pt, 0x60, 23, (void*)0x2b52);
  patricia_insert(pt, 0xc, 23, (void*)0x3a28);
  patricia_insert(pt, 0x80, 26, (void*)0x4f6d);
  patricia_insert(pt, 0xc0, 22, (void*)0x1b6a);
  patricia_insert(pt, 0x0, 26, (void*)0x40c8);
  patricia_insert(pt, 0xc00000, 24, (void*)0x3b2b);
  patricia_insert(pt, 0x180000, 24, (void*)0x54e6);
  patricia_insert(pt, 0x300000, 22, (void*)0x3bc9);
  patricia_insert(pt, 0x600, 23, (void*)0x4a38);
  patricia_insert(pt, 0x60, 20, (void*)0x4147);
  patricia_insert(pt, 0x600000, 20, (void*)0x4147);
  patricia_insert(pt, 0x800100, 21, (void*)0x952);
  patricia_insert(pt, 0x8001, 22, (void*)0x952);
  patricia_insert(pt, 0x300, 21, (void*)0x1b6a);
  patricia_insert(pt, 0x60, 22, (void*)0x5570);
  patricia_insert(pt, 0x30, 22, (void*)0x1b6a);
  patricia_insert(pt, 0x600, 22, (void*)0x4d03);
  patricia_insert(pt, 0x600000, 22, (void*)0x2e82);
  patricia_insert(pt, 0x6, 22, (void*)0x3817);
  patricia_insert(pt, 0xc00, 22, (void*)0x4885);
  patricia_insert(pt, 0x80, 22, (void*)0x4c9a);
  patricia_insert(pt, 0x18, 21, (void*)0x1b6a);
  patricia_insert(pt, 0x0, 30, (void*)0x7f9);
  patricia_insert(pt, 0x18, 22, (void*)0x5811);
  patricia_insert(pt, 0x1a, 24, (void*)0x1825);
  patricia_insert(pt, 0xa001, 24, (void*)0x1825);
  patricia_insert(pt, 0xf, 19, (void*)0x761);
  patricia_insert(pt, 0x78, 18, (void*)0x761);
  patricia_insert(pt, 0xf0, 19, (void*)0x761);
  patricia_insert(pt, 0x0, 17, (void*)0x97);
  // patricia_dump(pt, "sub3.dot");
  patricia_delete(pt, NULL);
}
END_TEST

Suite *rt_suite (void) 
{ 
  Suite *s = suite_create("Routing Table"); 
  TCase *tc_core = tcase_create("Core");
 
  suite_add_tcase (s, tc_core);
 
  tcase_add_test (tc_core, test_new); 
  tcase_add_test (tc_core, test_insert); 
  tcase_add_test (tc_core, test_nonexistent); 
  tcase_add_test (tc_core, test_specific_first); 
  tcase_add_test (tc_core, test_prefix); 
  tcase_add_test (tc_core, test_specific_last); 
  tcase_add_test (tc_core, test_specific_last2); 
  tcase_add_test (tc_core, test_yuri_one); 
  tcase_add_test (tc_core, test_yuri_two); 
  tcase_add_test (tc_core, test_default); 
  tcase_add_test (tc_core, test_heavy); 
  tcase_add_test (tc_core, test_specificity); 
  tcase_add_test (tc_core, test_subtle); 
  tcase_add_test (tc_core, test_subtle2); 
  tcase_add_test (tc_core, test_subtle3); 
  return s; 
}
 
Suite *radix_suite (void) 
{ 
  Suite *s = suite_create("Radix"); 
  TCase *tc_core = tcase_create("bottom");
 
  suite_add_tcase (s, tc_core);
 
  tcase_add_test (tc_core, test_mask); 
  tcase_add_test (tc_core, test_test); 
  return s; 
}
 
int main (int argc, char **argv __attribute__((unused))) 
{ 
  int nf; 
  Suite *s = radix_suite(); 
  Suite *s2 = rt_suite(); 
  SRunner *sr = srunner_create(s); 
  srunner_add_suite(sr, s2); 
  if(argc > 1) { 
    srunner_set_fork_status (sr, CK_NOFORK);  
    want_debug = TRUE;
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
