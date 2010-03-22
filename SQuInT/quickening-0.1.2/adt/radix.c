/* Copyright 1999-2002 Eric Hoffman, Neil Spring, 
   and the University of Washington */
#include "radix.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#ifdef WIN32
#include <fcntl.h>
#include <io.h>
#else
#include <netinet/in.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include "buffer.h"
#include "fcntl.h"
#include <assert.h>

#undef DEBUG_INSERTION
#define DEBUG_LOOKUP(x...) 

/* ugly global variable, which gets updated on entry from new 
   functions */
static struct patricia_table *current_table;
extern boolean want_debug;
boolean really_want_debug = FALSE;
boolean really_want_protection = FALSE;

#ifdef __ASSERT_VOID_CAST
/* we can, non-portably, define something that looks like assert and 
   works like assert, but aldo dumps the table */
#define radix_assert(expr) \
  do { if(!(expr) && current_table != NULL)   \
         patricia_dump(current_table, "assert.dot"); \
       (__ASSERT_VOID_CAST ((expr) ? 0 :					      \
		       (__assert_fail (__STRING(expr), __FILE__, __LINE__,    \
                               __ASSERT_FUNCTION), 0))); } while(0) 
#else 
#define radix_assert(expr) assert(expr)
#endif


static char * 
label_of_dest(address dest, prefixlen_t prefix, 
              /*@out@*//*@returned@*/char *buf);
static char * 
label_of_node(const struct patricia_entry *r, 
              /*@out@*//*@returned@*/char *buf);

void *get_data(const struct patricia_entry *r){
  return(r->data);
}
void set_data(struct patricia_entry *r, void *data) {
  r->data = data;
}

/* exported for testing */
address 
radix_mask(address addr, prefixlen_t prefix)
{
  address m;
  unsigned int shift = 32U - (unsigned int)prefix;
  if(prefix==(prefixlen_t)0) return 0;
  if(really_want_protection) assert(shift <= 32);
  m=(1<<(shift))-1;
  m=htonl(~m);
  return(addr&m);
}

static unsigned int testmasks[33];
void radix_init_testmasks(void) {
  unsigned int i;
  for(i=1; i<=32; i++) {
    testmasks[i] = htonl( 1 << (32U - i) );
  }
}

static inline boolean radix_test(address addr, prefixlen_t x) {
  return( x == (prefixlen_t) 0 || ((addr & testmasks[x]) != 0));
}

boolean radix_test_export(address addr, prefixlen_t x) {
  return radix_test(addr,x);
}


#if 0
/* test bit 1..32 */
/* if testing bit zero, return true; consider that an implicit match. */
unsigned int 
radix_test(address addr, prefixlen_t x)
{
  address g;
  unsigned int ret;
  unsigned int shift = 32U - (unsigned int)x;
  if((int)x==0) return 1;
  g=htonl(addr);
  ret = g&(1<<(shift));
  if(ret && !radix_test_fast(addr, x)) {
    exit(EXIT_FAILURE);
  } else if(!ret && radix_test_fast(addr, x)) {
    exit(EXIT_FAILURE);
  }
  return ret;
}
#endif


/*@null@*//*@dependent@*/ 
static struct patricia_entry *
find_route_internal(/*@null@*/struct patricia_entry *r, 
                    address destination, prefixlen_t prefix)
{
  if (!r) return(0);
  if ((r->destination==destination) && (r->prefixlen ==prefix))
    return(r);
  if(r->position <= 32) {
    if (radix_test(destination,r->position) != 0) {
      return(find_route_internal(r->on,destination,prefix));
    } else {
      return(find_route_internal(r->off,destination,prefix));
    }
  } else {
    return(NULL);
  }
}
 
/*@null@*//*@dependent@*/ 
struct patricia_entry *
radix_resolve_route(/*@null@*/ struct patricia_entry * top,
                    address destination) {
  struct patricia_entry * here=top;
  struct patricia_entry * best=NULL;
  prefixlen_t length=0;
  char la[25] __attribute__((unused));
  
  DEBUG_LOOKUP("looking for %s: ", label_of_dest(destination, 32, la));
  while(here){
    if ((here->prefixlen >= length)&&
        (radix_mask(destination,here->prefixlen) == here->destination)){
      best=here;
      length=here->prefixlen;
      DEBUG_LOOKUP("+%s ", label_of_node(here, la));
    } else {
      DEBUG_LOOKUP("-%s ", label_of_node(here, la));
    }
    if (here->position <= 32) 
      if( radix_test(destination,here->position) != 0)
        here=here->on;
      else 
        here=here->off;
    else 
      here=NULL;
  }
  DEBUG_LOOKUP("result: %s\n", best? label_of_node(best, la): "none");
  return(best);
}

static prefixlen_t find_difference(address a, address b, prefixlen_t maxbits){
  prefixlen_t difference;
  if(really_want_protection) assert(maxbits <= 33);
  for (difference=(prefixlen_t)1;
       (difference<maxbits &&
        (radix_test(a,difference) == 
         radix_test(b,difference))); 
       ((int)difference)++);
  return(difference);
}
#ifdef DEBUG_INSERTION
static void radix_insert_debug(struct patricia_entry * new, const char * as, 
                               struct patricia_entry * old) {
    char buf[50], buf2[50];
    VERBAL("radix_insert_debug %s %s %s-%d\n", 
           label_of_node(new, buf), 
           as,
           label_of_node(old, buf2), old->position); 
}
#else
static void radix_insert_debug(struct patricia_entry * new __attribute__((unused)), 
                               const char * as __attribute__((unused)), 
                               struct patricia_entry * old __attribute__((unused))) {
  return;
}
#endif

void radix_insert(struct patricia_entry **p, 
                  /*@owned@*//*@partial@*/struct patricia_entry *new)
{
  prefixlen_t difference;
  struct patricia_entry * old=*p;
  int commonprefixlen;

  /* a little piece of mind */
  if(really_want_protection) assert(new->position == 0);
  if(really_want_protection) assert(new->on == NULL);
  if(really_want_protection) assert(new->off == NULL);

  if(really_want_protection) radix_assert(old== NULL || old->on == NULL || 
               (old->on->on == NULL && old->on->off == NULL) ||
               old->position <= old->on->position);
  if(really_want_protection) radix_assert(old== NULL || old->off == NULL|| 
               (old->off->on == NULL && old->off->off == NULL) ||
               old->position <= old->off->position);

  /* an empty table or subtree */
  if (old == NULL) {
    new->position = new->prefixlen + 1;
    *p = new;
    radix_insert_debug (new, "in empty table", new);
    return;
  }

  /* a duplicate */
  if(old->destination == new->destination &&
     old->prefixlen == new->prefixlen) {
    /* NON-ROUTING-TABLE behavior: assume oldest, not newest, is right. */
    radix_insert_debug (new, "is duplicate of", old);
    return;
  }

  difference = find_difference(old->destination, new->destination,
                               min(new->prefixlen, old->prefixlen));
  commonprefixlen = max((int)0, ((int)old->position)-1);

  /* attempt to handle a strange case:
     we're inserting something that shouldn't go further down
     in the tree, because of prefix < position requirements. */
  if(difference >= old->position &&
     new->prefixlen <= old->position &&
     old->position < old->prefixlen) {
    struct patricia_entry * *child;
    new->position = old->position;
    new->on = old->on;
    new->off = old->off;
    old->on = old->off = NULL;
    *p = new;

    child = (radix_test(old->destination,new->position)!=0) 
      ? &new->on : &new->off;
    radix_insert_debug (old, "as pushdown of", new);
    old->position=0;
    radix_insert(child, old);

    return;
  }

  /* if new is contained in old/prefix; at least what old/position */
  if( difference >= old->position || 
      ((radix_mask(new->destination,commonprefixlen) == 
        radix_mask(old->destination,commonprefixlen) )
       && new->prefixlen > old->prefixlen )){

    struct patricia_entry * *child = (radix_test(new->destination,old->position)!=0) 
      ? &old->on : &old->off;
    radix_insert_debug (new, "as child of", old);
    radix_insert(child, new);
    if(really_want_protection) radix_assert(new->position > old->position || 
                 (new->position >= old->position && 
                  new->destination == old->destination));
    return;
  }

  /* if old is contained in new */
  /* if( (radix_mask(new->destination,old->prefixlen) == old->destination) 
     && new->prefixlen > old->prefixlen ){ */ 
  do {
    radix_insert_debug (new, "as parent of", old);
    if(difference < old->position-1) {
      if(radix_test(old->destination, difference) != 
         radix_test(new->destination, difference)) {
        new->position = difference;
      } else {
        new->position = difference + 1;
      }
    } else {
      new->position = old->position - 1; 
    }
    // new->position = min((int)difference, (int)old->position-1);
    if(radix_test(old->destination, new->position)!=0) {
      new->on = old;
    } else {
      new->off = old;
    }
    *p = new;
    return;
  } while(0);

  assert(0);
  
}

static char * 
label_of_dest(address dest, prefixlen_t prefixlen, /*@out@*//*@returned@*/char *buf) {
  address a=ntohl(dest);
  assert(prefixlen <= 32);
  sprintf(buf,"%u.%u.%u.%u/%u",
          a>>24, (a>>16)&255, (a>>8)&255, a&255,
          prefixlen);
  return(buf);
}

static char * 
label_of_node(const struct patricia_entry *r, 
              /*@out@*//*@returned@*/char *dest)
{
  return(label_of_dest(r->destination, r->prefixlen, dest));
}

static void 
print_routing_table_internal(const struct patricia_entry *r,
                             buffer b,
                             unsigned int *max)
{ 
  char here[255];

  if (r){
    if (r->position > *max) *max=r->position;
    (void)label_of_node(r,here);
    /*bprintf (b," {rank = same; %d ; \"%s\"}\n",r->position,here);*/
    bprintf (b," \"%s\" [label = \"{<h> %s | {<off> 0 |bit %d |<on> 1}}\"];\n",
             here, here, r->position);

    if (r->off){
      char off[255];
      bprintf (b,"  \"%s\":off -> \"%s\"[label=\"%d=0\"];\n",
               here,label_of_node(r->off,off),r->position);
      print_routing_table_internal(r->off,b,max);
    }
    if (r->on){
      char on[255];
      bprintf (b,"  \"%s\":on -> \"%s\"[label=\"%d=1\"];\n",
               here, label_of_node(r->on,on), r->position);
      print_routing_table_internal(r->on,b,max);
    }
  }
}

void radix_print_routing_table_dot(const struct patricia_entry *top, int fd)
{
  unsigned max=0;
  buffer b=create_buffer(100);
  buffer base=create_buffer(100);

  bprintf (base,"digraph radix {\n");
  bprintf (base,"  size=\"10,7.5\"; ratio=auto; ordering=out\n");
  bprintf (base,"  node [shape=record,width=.3,height=.3]\n");
  if (top) {
    // char z[90];
    // eh.     bprintf (b,"  root -> \"%s\";\n",label_of_node(top,z));
    print_routing_table_internal(top,b,&max);
  }
  (void) write (fd,base->contents,(size_t)base->fill);
  (void) write (fd,b->contents,(size_t)b->fill);
  (void) write (fd,"}\n",2);
  free_buffer(b);
  free_buffer(base);
}

static void remove_route_internal(struct patricia_entry * *place,
                                  address destination,
                                  unsigned int prefixlen)
{
  struct patricia_entry * r=*place;
  if (r){
    if ((r->destination!=destination) || (r->prefixlen!=prefixlen)){
      if (radix_test(destination,r->position) != 0)
        remove_route_internal(&r->on,destination,prefixlen);
      else 
        remove_route_internal(&r->off,destination,prefixlen);
    } else {
      /* found it */
      if (r->on) {
        struct patricia_entry * q=r->on;
        if (r->off) {
          /* the rotation case*/
          remove_route_internal(&r->on,q->destination,q->prefixlen);
          q->position=r->position;
          q->off=r->off;
          q->on=r->on;
        } 
        *place=q;
      } else *place=r->off;
    }
  }
}

void radix_remove_route(struct patricia_table * n,
                        address destination,
                        prefixlen_t prefixlen)
{
  if(n->routes) /* cheap check keeps lclint happy */
    remove_route_internal(&n->routes,destination,prefixlen);
}

static void 
print_routing_table_text_internal(const struct patricia_entry *r, 
                                  FILE *fdout) {
  if (r){
    char here[25];
    fprintf(fdout, " %s via --\n", label_of_node(r,here));
    print_routing_table_text_internal(r->off,fdout);
    print_routing_table_text_internal(r->on,fdout);
  }
}

void radix_print_routing_table_text(const struct patricia_entry *top, 
                                    FILE *fdout) {
  fprintf(fdout, "Routing table:\n");
  if(top)
    print_routing_table_text_internal(top, fdout);
}


struct patricia_table * patricia_new(void){
  NEWPTR(struct patricia_table, p);
  radix_init_testmasks(); /* can overwrite */
  p->routes = NULL;
  p->default_route = NULL;
  return(p);
}

static void recursive_delete(struct patricia_entry *e, 
                             /*@null@*/ void (*data_free)(void *)) {
  if(e != NULL) {
    if(e->on != NULL)     recursive_delete(e->on, data_free);
    if(e->off != NULL)    recursive_delete(e->off, data_free);
    if(data_free != NULL) data_free(e->data);
    free(e);
  }
}

void patricia_delete(struct patricia_table * p, 
                     /*@null@*/ void (*data_free)(void *)) {
  recursive_delete(p->routes, data_free);
  recursive_delete(p->default_route, data_free);
  p->routes = NULL;
  p->default_route = NULL;
  free(p);
}

void patricia_insert(struct patricia_table * pt, 
                     address a, 
                     prefixlen_t prefixlen, 
                     void *data) {
  /* really try to avoid duplication; I'd rather do that 
   in the radix_insert(), but that seems like a pain atm.*/
  // if(find_route_internal(pt->routes, a, prefixlen) == NULL) {
  NEWPTR(struct patricia_entry, pe);
  current_table = pt; /* icky */
  assert(pe != NULL);
  assert(prefixlen <=32);
  assert(pt != NULL);
  pe->destination = a;
  if(radix_mask(a, prefixlen) != a) {
    // VERBAL("dropping errant 0x%x/%u\n", a, prefixlen);
    return;
  }
  pe->prefixlen = prefixlen;
  pe->data = data;
  pe->position = 0;
  pe->on = NULL;
  pe->off = NULL;
  if(really_want_debug) {
    //      VERBAL("patricia_insert(pt, 0x%x, %u, (void *)0x%p);\n", a, prefixlen, data);
  }
  radix_insert(&pt->routes, pe);
  if(really_want_protection) assert(find_route_internal(pt->routes, a, prefixlen) != NULL);
  //}
}
/*@null@*/
void *patricia_lookup(struct patricia_table * pt, address a) {
  struct patricia_entry * pe;
  // char buf[256];
  current_table = pt;
  pe = radix_resolve_route(pt->routes, a);
  if(pe == NULL) {
    pe = pt->default_route;
  }
  if(pe == NULL) {
    return NULL;
  } 
  // printf("foundl %s\n", label_of_node(pe, buf));
  return(pe->data);
}

boolean 
patricia_lookup_all(struct patricia_table * pt, 
                    address a, 
                    /*@out@*/ address *prefix, 
                    /*@out@*/ prefixlen_t *prefixlen, 
                    /*@out@*/ void **data) {
  struct patricia_entry * pe;
  // char buf[256];
  current_table = pt;
  pe = radix_resolve_route(pt->routes, a);
  if(pe == NULL) {
    pe = pt->default_route;
  }
  if(pe == NULL) {
    return FALSE;
  } 
  // printf("found %s\n", label_of_node(pe, buf));
  *prefix = pe->destination;
  *prefixlen = pe->prefixlen;
  *data = pe->data;
  return TRUE;
}

void patricia_dump(struct patricia_table * pt, const char *filename) {
  int f = open(filename, O_CREAT | O_TRUNC | O_WRONLY, 0644);
  radix_print_routing_table_dot(pt->routes, f);
  close(f);
}
