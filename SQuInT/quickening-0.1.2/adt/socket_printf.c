#include "nscommon.h"
#include "socket_printf.h"
#include <unistd.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>

/* just a simple routine that takes a socket descriptor
   instead of a FILE *, then write()s the sprintf'd
   string. */
#define SOCK_PRINTF_BUFSIZE 4096
int socket_vprintf(int s, const char *fmt, va_list args) {
	int bytes;
	int written;

    char *buf = malloc_ordie(SOCK_PRINTF_BUFSIZE);

    memset(buf, 0, SOCK_PRINTF_BUFSIZE); /* valgrind appeasement */

	if (s < 0) {
		fprintf(stderr, "null connection to socket_printf\n");
		abort();
	}

	bytes = vsnprintf(buf, SOCK_PRINTF_BUFSIZE, fmt, args);

    /* make certain */
    buf[SOCK_PRINTF_BUFSIZE -1] = '\0';
    written = write(s, buf, bytes);
    /* let the caller deal with error and just shutup if
       errno is "Bad file descriptor" meaning the other side
       is dead. */
    if(written < 0 && errno != EBADF) {
      fprintf(stderr, "write failed on sd %d: (error %d) %s\n", s, errno, strerror(errno));
    }
    free(buf);
    return(written); /* may be -1. */
}

int socket_printf(int s, const char *fmt, ...) {
  	va_list args;
    int written;
	va_start(args, fmt);
	written = socket_vprintf(s, fmt, args);
	va_end(args);
    return(written);
}

/* returns the first of the gethostbyname-returned hostnames */ 
/* probably reasonable to obsolete this, and only use if
getaddrinfo is not present */
boolean socket_addr(const char *hostname, unsigned short port, 
                    /*@out@*/ struct sockaddr_in *sain, socklen_t *salt) {
	struct hostent *host;
    if(*salt < (socklen_t)sizeof(struct sockaddr_in)) {
      printf("we're pretty much doing ipv4 here\n");
      return FALSE;
    }

	host = gethostbyname(hostname);
	if (host == NULL) {
		printf("gethostbyname(%s) failed: %s\n", hostname, 
#ifdef HAVE_HSTRERROR
               hstrerror(h_errno)
#else
               ""
#endif
               );
		return FALSE;
	};

	sain->sin_family = AF_INET;
	sain->sin_addr.s_addr = *(u_long *) host->h_addr_list[0];
	sain->sin_port = htons(port);
    *salt = sizeof(struct sockaddr_in);
    return TRUE;
}

int socket_connect_in4( const struct sockaddr *addr, const socklen_t slen ) {
  int fd = socket(addr->sa_family, SOCK_STREAM, IPPROTO_TCP);
  int i;
  if (fd == -1) {
    perror("Error opening socket");
    printf("socket() failed.\n");
    return (-1);
  };

  i = connect(fd, addr, slen);
  if (i == -1) {
    int saved_errno = errno;
    perror("Error connecting");
    /*     printf("connect(%s:%d) failed: %s\n", inet_ntoa(addr->sin_addr),
           ntohs(addr->sin_port), strerror(saved_errno)); */
    close(fd);
    errno = saved_errno;
    return (-1);
  };
  return (fd);
}
/* this code from WMBIFF.  GPL'd. getaddrinfo code
 contributed to wmbiff by Junichiro Hagino (itojun) */
int socket_connect(const char *hostname, unsigned short port)
{
#ifdef HAVE_GETADDRINFO
	struct addrinfo hints, *res, *res0;
	int fd;
	char pbuf[NI_MAXSERV];
	int error;

	memset(&hints, 0, sizeof(hints));
	hints.ai_socktype = SOCK_STREAM;
	snprintf(pbuf, sizeof(pbuf), "%d", port);
	error = getaddrinfo(hostname, pbuf, &hints, &res0);
	if (error) {
		fprintf(stderr, "%s: %s\n", hostname, gai_strerror(error));
		return -1;
	}

	fd = -1;
	for (res = res0; res; res = res->ai_next) {
		fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
		if (fd < 0)
			continue;
		if (connect(fd, res->ai_addr, res->ai_addrlen) < 0) {
			close(fd);
			fd = -1;
			continue;
		}
		break;
	}
	freeaddrinfo(res0);
	if (fd < 0) {
		fprintf(stderr, "socket/connect to %s failed: %s\n", hostname,
			   strerror(errno));
		return -1;
	}
	return fd;
#else
#warning "This build will not support IPv6 sockets"
	struct sockaddr_in addr;
	socklen_t addr_len = sizeof(struct sockaddr_in);

    if(socket_addr(hostname, port, &addr, &addr_len) == FALSE) {
      return (-1);
    }

    return(socket_connect_in4((struct sockaddr *)&addr, addr_len));

#endif
}

int socket_connect_addr(in_addr_t addr, unsigned short port){
  struct sockaddr_in sain;

  memset(&sain, 0, sizeof(sain));
  sain.sin_family = AF_INET;
  sain.sin_addr.s_addr = addr;
  sain.sin_port = htons(port);
  return(socket_connect_in4((struct sockaddr *)&sain, sizeof(sain)));
}

boolean socket_setnonblock(int fd) {
  int flags;
  flags = fcntl(fd, F_GETFL, 0);
  if(flags != -1) {
    flags |= O_NONBLOCK;
    if( fcntl(fd, F_SETFL, flags) != -1 ) {
      return TRUE;
    }
  }
  return FALSE;
}
boolean socket_unsetnonblock(int fd) {
  int flags;
  flags = fcntl(fd, F_GETFL, 0);
  if(flags != -1) {
    flags &= ~O_NONBLOCK;
    if( fcntl(fd, F_SETFL, flags) != -1 ) {
      return TRUE;
    }
  }
  return FALSE;
}

int socket_check_pending_error(int sd, /*@out@*/ int *err) {
  int len = sizeof(*err);
  if ( getsockopt(sd, SOL_SOCKET, SO_ERROR, err, &len) < 0 ) {
    /* solaris */
    *err = errno;
    return -1;
  } else if(*err) {
    return -1;
  }
  return 0;
}

