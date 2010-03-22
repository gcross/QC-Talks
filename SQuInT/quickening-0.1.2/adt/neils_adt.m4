dnl -*-Autoconf-*-
dnl macros used to setup all that adt requires.
dnl and macros I always use.

# from the autoconf archive.
AC_DEFUN([QEF_C_NORETURN],
[AC_REQUIRE([AC_PROG_CC])
AC_MSG_CHECKING(whether the C compiler (${CC-cc}) accepts noreturn attribute)
AC_CACHE_VAL(qef_cv_c_noreturn,
[qef_cv_c_noreturn=no
AC_TRY_COMPILE(
[#include <stdio.h>
void f (void) __attribute__ ((noreturn));
void f (void)
{
   exit (1);
}
], [
   f ();
],
[qef_cv_c_noreturn="yes";  FUNCATTR_NORETURN_VAL="__attribute__ ((noreturn))"],
[qef_cv_c_noreturn="no";   FUNCATTR_NORETURN_VAL="/* will not return */"])
])
AC_MSG_RESULT($qef_cv_c_noreturn)
AC_DEFINE_UNQUOTED(FUNCATTR_NORETURN, $FUNCATTR_NORETURN_VAL, "defined to be __attribute__((noreturn)) when it works")
if test "$qef_cv_c_noreturn" = "yes"; then
  AC_DEFINE(HAVE_ATTR_NORETURN, 1, [Define if __attribute__((noreturn)) exists and works])
fi
])dnl

dnl taken from curl http://curl.haxx.se/mail/archive-2001-08/0073.html
dnl fragment written by albert chin.
AC_DEFUN([CURL_CHECK_WORKING_GETADDRINFO],[
  AC_CACHE_CHECK(for working getaddrinfo, ac_cv_working_getaddrinfo,[
  AC_TRY_RUN( [
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>

void main(void) {
    struct addrinfo hints, *ai;
    int error;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    error = getaddrinfo("127.0.0.1", "8080", &hints, &ai);
    if (error) {
        exit(1);
    }
    else {
        exit(0);
    }
}
],[
  ac_cv_working_getaddrinfo="yes"
],[
  ac_cv_working_getaddrinfo="no"
],[
  ac_cv_working_getaddrinfo="yes"
])])
if test "$ac_cv_working_getaddrinfo" = "yes"; then
  AC_DEFINE(HAVE_GETADDRINFO, 1, [Define if getaddrinfo exists and works])
  AC_DEFINE(ENABLE_IPV6, 1, [Define if you want to enable IPv6 support])

  IPV6_ENABLED=1
  AC_SUBST(IPV6_ENABLED)
fi
])





AC_DEFUN([AC_NSPRING_CHECK_CACHE],
   [
    AC_MSG_CHECKING(cached information)
    hostcheck="$host"
    AC_CACHE_VAL(ac_cv_hostcheck, [ ac_cv_hostcheck="$hostcheck" ])
    if test "$ac_cv_hostcheck" != "$hostcheck"; then
      AC_MSG_RESULT(changed)
      AC_MSG_WARN(config.cache exists!)
      AC_MSG_ERROR(you must make distclean first to compile for different host)
    else
      AC_MSG_RESULT(ok)
    fi
    ])

AC_DEFUN([AC_NSPRING_PROG_CC],    [
    AC_PROG_CC
    if test -n "$GCC"; then
       # the following cleverness extracted from libcurl's autoconf script
       AC_MSG_CHECKING([gcc version])
       gccver=`$CC -dumpversion`
       num1=`echo $gccver | cut -d . -f1`
       num2=`echo $gccver | cut -d . -f2`
       gccnum=`(expr $num1 "*" 100 + $num2) 2>/dev/null`
       AC_MSG_RESULT($gccver)

      AC_MSG_RESULT(adding -Wall and friends to CFLAGS.)
      CFLAGS="$CFLAGS -W -Wall -Wshadow -Wpointer-arith -Wwrite-strings"
      
       if test "$gccnum" -ge "304"; then
           CFLAGS="$CFLAGS -Wmissing-noreturn"
       fi
      dnl 342 in FC2 screwed it up again, it seems, so 
      dnl require that it be greater than 3.4.x
       if test "$gccnum" -gt "304"; then
           CFLAGS="$CFLAGS -Wunreachable-code"
       fi

dnl  -Wstrict-prototypes -Wtraditional"
dnl  -Wredundant-decls"
    fi
    QEF_C_NORETURN
    ])


AC_DEFUN([AC_NSPRING_PATH_PROGS], [
         AC_PATH_PROG(LINT,  splint, "./missing splint")
         AC_PATH_PROG(DOT,   dot,    "./missing dot")
         AC_PATH_PROG(CVSCL, cvs2cl, "./missing cvs2cl")
         AC_SUBST(CVSCL) ])

AC_DEFUN([AC_NSPRING_ADT], [
         AC_NSPRING_PATH_PROGS
         dnl not needed with libtool AC_PROG_RANLIB
         AC_PROG_INSTALL
         AC_PROG_MAKE_SET
         AC_PROG_LIBTOOL

         AC_CHECK_FUNC(getopt_long, 
            GETOPT_OBJ="" GETOPT_SRC="" GETOPT_LIBTOOLOBJ="", \
            GETOPT_SRC="getopt.c getopt1.c" GETOPT_OBJ="getopt.o getopt1.o" GETOPT_LIBTOOLOBJ="getopt.lo getopt1.lo")
         AC_SUBST(GETOPT_SRC)
         AC_SUBST(GETOPT_OBJ)
         AC_SUBST(GETOPT_LIBTOOLOBJ)

         dnl solaris lacks scandir.
         AC_CHECK_FUNC(scandir, SCANDIR_LIBTOOLOBJ="" SCANDIR_OBJ="", 
                     [ SCANDIR_LIBTOOLOBJ="scandir.lo" SCANDIR_OBJ="scandir.o"
                       AC_DEFINE(MUST_INCL_SCANDIR_H, 1, [we need to include scandir.h as scandir is not provided by dirent.])
                       ] )
         AC_SUBST(SCANDIR_OBJ)
         AC_SUBST(SCANDIR_LIBTOOLOBJ)

         AC_CHECK_HEADERS(check.h)
         AC_CHECK_LIB(check,suite_create)
         if test $ac_cv_lib_check_suite_create = yes; then
           AC_CHECK_LIB(check,suite_free, 
                [ AC_DEFINE(HAVE_CHECK_SUITE_FREE, 1, [suite_free is supported (required) in this version of check ] ) ], 
                  AC_MSG_WARN( [ suite_free not present in this new version of check.  build may fail ]) )
         fi
                                        

         dnl stuff nscommon or the rest of adt needs
         AC_CHECK_HEADERS(stdint.h)

         dnl stuff to portably get u_int64_t;  taken from a patch for squid.
         AC_CHECK_SIZEOF(uint64_t)
         AC_CHECK_SIZEOF(long long)
         AC_CHECK_SIZEOF(long)
         AC_CHECK_SIZEOF(__int64)
         dnl u_int64_t
         if test "x$ac_cv_sizeof_uint64_t" = "x8"; then
            AC_CHECK_TYPE(u_int64_t,uint64_t)
         elif test "x$ac_cv_sizeof_long" = "x8"; then
              AC_CHECK_TYPE(u_int64_t,unsigned long)
         elif test "x$ac_cv_sizeof_long_long" = "x8"; then
              AC_CHECK_TYPE(u_int64_t,unsigned long long)
         elif test "x$ac_cv_sizeof___int64" = "x8"; then
              AC_CHECK_TYPE(int64_t,unsigned __int64)
         fi
         AC_HAVE_FUNCS(snprintf)

         dnl these are used by socket_printf.c, but require -lresolv -lsocket -lnsl on solaris?
         dnl herror is deprecated presumably because it isn't thread safe. hstrerror is okay on solaris.
         dnl both are discouraged on recent linux, presumably in favor of getaddrinfo(3), getnameinfo(3), gai_strerror(3).
         dnl and those aren't everywhere.  ugh.  
         AC_HAVE_FUNCS(herror hstrerror)

         dnl used by the new daemon.c if it's around, instead of using what I have.
         AC_HAVE_FUNCS(daemon)

         AC_TYPE_SOCKLEN_T
         CURL_CHECK_WORKING_GETADDRINFO
])

AC_DEFUN([AC_NSPRING_ADT_PGSQL], [
         AC_CHECK_HEADERS(postgresql/libpq-fe.h libpq-fe.h)
         AC_CHECK_LIB(pq, main)
])


AC_DEFUN([AC_NSPRING_NO_PTHREAD], [
         dnl Checks for libraries.
         AC_CHECK_TYPE(pthread_mutex_t,,
             AC_DEFINE(pthread_mutex_t, void *, [Define to 'void *' if we're not using pthread and it doesn't happen to exist]),  [ 
#include <sys/types.h>  
#include <stdio.h>  
  ] )
])

AC_DEFUN([AC_NSPRING_PTHREAD], [
         dnl Checks for libraries.
         AC_CHECK_LIB(pthread, pthread_cond_init)
         AC_DEFINE(_REENTRANT, 1, [actually use pthread.])
])


AC_DEFUN([AC_NSPRING_PROF_COV_DMALLOC_OPTIONS], [
  AC_ARG_ENABLE(prof, AC_HELP_STRING([--enable-prof], 
                                [compile for profiling with gprof]),
    CFLAGS="$CFLAGS -pg")
  AC_ARG_ENABLE(cov, AC_HELP_STRING([--enable-cov], 
                     [ compile for coverage with gcov]),
    CFLAGS="$CFLAGS -fprofile-arcs -ftest-coverage")
  dnl emulating AM_WITH_DMALLOC less foolishly.
  AC_ARG_WITH(dmalloc, AC_HELP_STRING([--with-dmalloc], [ compile for memory debug]), [
   AC_CHECK_LIB(dmallocth, malloc)
   AC_CHECK_HEADERS(dmalloc.h)
   AC_MSG_CHECKING(whether dmalloc is complete)
   if test "x$ac_cv_lib_dmallocth_malloc" = "xyes"; then
     if test "x$ac_cv_header_dmalloc_h" = "xyes"; then
       AC_MSG_RESULT(yes)
       AC_DEFINE(WITH_DMALLOC, 1, [use dmalloc])
     else
       AC_MSG_RESULT(no header)
     fi
   else
     AC_MSG_RESULT(no library)
   fi
  ])
])

AC_DEFUN([AC_NSPRING_APPEASE_SYSTEM_H], [
  AC_CHECK_HEADERS( sys/param.h sys/time.h time.h sys/mkdev.h sys/sysmacros.h string.h memory.h fcntl.h dirent.h sys/ndir.h ndir.h alloca.h locale.h )

  dnl AC_EGREP_HEADER(utimbuf, utime.h, AC_DEFINE(HAVE_STRUCT_UTIMBUF))
  dnl would miss ifdef'd out utimbuf.
  AC_CHECK_HEADERS(utime.h error.h)
  AC_MSG_CHECKING(for utime)
  AC_TRY_COMPILE([#include <sys/types.h>
  #include <utime.h>],
  [struct utimbuf x; x.actime = x.modtime = 0; utime ("/", &x);],
    [AC_MSG_RESULT(yes)
     AC_DEFINE(HAVE_STRUCT_UTIMBUF, 1, [has struct utimbuf in utime.h] )],
    [AC_MSG_RESULT(no)
     AC_CHECK_FUNCS(utimes)])

  AC_HEADER_MAJOR
  AC_FUNC_ALLOCA
  AC_STRUCT_TM
  AC_STRUCT_ST_BLOCKS
  AC_FUNC_CLOSEDIR_VOID
  AC_CHECK_FUNCS(mkfifo)
  AC_CHECK_FUNC(mknod)
  AC_CHECK_FUNC(vprintf)
  AC_CHECK_FUNC(doprnt)

  dnl ----- end   appease system.h ------
])


dnl http://www.hlrs.de/people/keller/configure_scripts.html
AC_DEFUN([AC_TYPE_SOCKLEN_T], [
  dnl Since the old-style (autoconf 2.13) macro AC_CHECK_TYPE(type, replacement)
  dnl only checks in <sys/types.h> we have to be more awkward:
  dnl We have to check for socklen_t in <sys/types.h> -- if it's not defined
  dnl there, we also look in <sys/socket.h> (a more common place)
  AC_CHECK_TYPE(socklen_t,,
    AC_DEFINE(socklen_t, int, [Define to `int' if neither <sys/types.h> nor <sys/socket.h> define.]),
    [
#include <sys/types.h>
#include <sys/socket.h>
  ])
])

AC_DEFUN([AC_NSPRING_WERROR], [
   AC_ARG_ENABLE(werror, 
                 AC_HELP_STRING([--enable-werror], 
                                [add -Werror to CFLAGS]),
                 dnl was specified
                 ac_cv_use_werror=$enableval, 
   dnl enable -Werror if user is uw1 or building in cs.washington.edu
   dnl can probably expand to test if a file exists...
   AC_MSG_CHECKING(whether to enable -Werror as default)
   LOCAL_DOMAIN_NAME=`hostname -d`
   if test "x$LOCAL_DOMAIN_NAME" = "xcs.washington.edu" || test "x$USER" = "xuw9"; then
      AC_MSG_RESULT(of course.)
      ac_cv_use_werror="yes"
   else
      AC_MSG_RESULT(nope.)
      ac_cv_use_werror="no"
   fi
   ) dnl enable
   if test "x$ac_cv_use_werror" = "xyes"; then
     WERROR="-Werror"
     AC_SUBST(WERROR) 
   fi
])


AC_DEFUN([AC_NSPRING_STATIC_LINK], [
  AC_MSG_CHECKING(whether -static might work)
  TMP_CFLAGS=$CFLAGS
  CFLAGS="$CFLAGS -static"
  AC_TRY_RUN( [int main(int argc, char **argv) { exit(0); } ],
    [AC_MSG_RESULT(yes)
     STATIC_LINK="-static"],
    [AC_MSG_RESULT(no)
     STATIC_LINK=""])
  AC_SUBST(STATIC_LINK)
  CFLAGS=$TMP_CFLAGS
])

