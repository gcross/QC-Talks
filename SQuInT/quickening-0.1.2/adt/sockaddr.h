#include <sys/types.h>
#include <sys/socket.h>
#include "nscommon.h"

in_addr_t ipv4_name_lookup(const char *hostname);
boolean ipv4_reverse_name_lookup(in_addr_t address, 
                                 /*@out@*/ char *name, 
                                 int namebuflen );
unsigned int ipv4_addr_from_string(const char *x);
