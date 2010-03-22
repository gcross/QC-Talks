#include "nscommon.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdarg.h>

/* returns number of bytes written, like that matters, 
   or -1 on error. */
int /*@alt void@*/ socket_printf(int s, 
                                 const char *fmt, 
                                 ...) __attribute__((format(printf, 2,3)));
int /*@alt void@*/ socket_vprintf(int s, const char *fmt, va_list args);
int socket_connect(const char *hostname, unsigned short port);
int socket_connect_addr(in_addr_t addr, unsigned short port);

boolean socket_addr(const char *hostname, unsigned short port, 
                    /*@out@*/ struct sockaddr_in *sain, socklen_t *salt);
int socket_connect_in4( const struct sockaddr *addr, socklen_t len );
boolean socket_unsetnonblock(int fd);
boolean socket_setnonblock(int fd);
int socket_check_pending_error(int sd, /*@out@*/ int *err);
