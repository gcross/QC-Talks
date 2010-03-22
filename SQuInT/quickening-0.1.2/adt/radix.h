#include "nscommon.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#ifndef __LCLINT__
#include <arpa/inet.h>
#endif
#include <stdio.h>

// #ifdef __linux__ 
// #define ADDR_T in_addr_t
// #else
#define ADDR_T u_int32_t
// #endif

typedef ADDR_T address;
typedef unsigned char prefixlen_t;
/* typedef struct patricia_entry *patricia_entry; */
/* typedef struct patricia_table *patricia_table; */

struct patricia_entry {
  address destination;
  prefixlen_t prefixlen;
  unsigned char position;
  void *data;
  struct patricia_entry *on,*off;
};

struct patricia_table {
 /*@null@*/ struct patricia_entry *routes;
 /*@null@*/ struct patricia_entry *default_route;
};

/* externally useful bits */
struct patricia_table *patricia_new(void);
void patricia_delete(/*@only@*/ struct patricia_table * p, void (*data_free)(void *));
void patricia_insert(struct patricia_table *pt, address a, 
                     unsigned char prefixlen, /*@owned@*/void *data);
/*@null@*//* just get the data *//*@dependent@*/
void *patricia_lookup(struct patricia_table *pt, address a); 
/* get it all */
boolean patricia_lookup_all(struct patricia_table * pt, 
                            address a, 
                            /*@out@*/ address *prefix, 
                            /*@out@*/ prefixlen_t *prefixlen, 
                            /*@out@*/ void **data);
void patricia_dump(struct patricia_table *pt, const char *filename);

/* slightly less useful */
/*@null@*//*@dependent@*/ 
struct patricia_entry *
radix_resolve_route(/*@null@*/
                    struct patricia_entry *top, 
                    address destination);
void *get_data(const struct patricia_entry *r);

/* somewhat useful bits */
void set_data(struct patricia_entry *r, void *data);

void radix_insert(struct patricia_entry **p, /*@owned@*//*@partial@*/struct patricia_entry *r);

void radix_remove_route(struct patricia_table *n,
                        address destination,
                        unsigned char prefixlen);

void radix_print_routing_table_text(/*@null@*/const struct patricia_entry *r, 
                                    FILE *fdout);
void radix_print_routing_table_dot(/*@null@*/const struct patricia_entry *r, 
                                   int fd);

address radix_mask(address addr, prefixlen_t prefix);
boolean radix_test_export(address addr, prefixlen_t x); // used by check_radix, real thing is static so as to be inlined.
