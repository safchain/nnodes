AC_PREREQ([2.59])
AC_INIT([nnodes],[0.6],[safchain@gmail.com])
AC_CONFIG_SRCDIR([src])

AM_INIT_AUTOMAKE

AC_CONFIG_HEADERS([config.h])
AC_PROG_CC
AC_PROG_RANLIB

CFLAGS="$CFLAGS -Wall -W -fno-strict-aliasing"
AC_ARG_ENABLE(debug,
[  --enable-debug		Enable some extra expensive checks for developers],
    if test x$enableval = xyes; then
        AC_DEFINE(DEBUG,, Build with extra debugging checks)
	elif test x$enableval = xalloc; then
		AC_DEFINE(DEBUG_ALLOC,, Build with extra allocation debugging checks)
	elif test x$enableval = xstack; then
		AC_DEFINE(DEBUG_STACK,, Build with extra stack debugging checks)
	elif test x$enableval = "xalloc,stack"; then
		AC_DEFINE(DEBUG_ALLOC,, Build with extra allocation debugging checks)
		AC_DEFINE(DEBUG_STACK,, Build with extra stack debugging checks)
    fi)

dnl checks
AC_HEADER_STDC
AC_CHECK_HEADERS([linux/inotify.h pthread.h arpa/inet.h netinet/in.h stdlib.h string.h strings.h sys/socket.h fcntl.h unistd.h limits.h])

AC_C_INLINE
AC_FUNC_FORK
AC_FUNC_REALLOC
AC_TYPE_PID_T
AC_TYPE_UID_T

AC_CHECK_LIB(pthread, pthread_create) 

AC_C_CONST

AC_FUNC_MALLOC
AC_TYPE_SIGNAL
AC_CHECK_FUNCS([inet_ntoa atexit memset socket strcasecmp strerror strncasecmp strtol strdup dup2 strstr])

AC_CONFIG_FILES([Makefile \
	src/Makefile \
	src/libmm/Makefile \
	src/libhl/Makefile \
	src/libxml/Makefile \
	src/libmisc/Makefile \
	src/libdns/Makefile \
	src/libnet/Makefile \
	src/librdb/Makefile \
	src/libmon/Makefile \
	src/libtopt/Makefile \
	src/libnnodes/Makefile \
	src/libast/Makefile \
	src/liblog/Makefile \
	src/libvthttp/Makefile \
	src/service/Makefile \
	tests/Makefile \
	tests/fuzzing/Makefile])

dnl geoip
AC_ARG_ENABLE(geoip, [  --enable-geoip 		Enable geo ip localisation],
	[enable_geoip="$enableval"],[enable_geoip="no"])

AS_IF([test "x$enable_geoip" = "xyes"], [
	PKG_CHECK_MODULES([GEOIP], [geoip >= 1.4.7], AC_DEFINE([HAVE_GEOIP], [1], [Use GEOIP]))
	CFLAGS="$CFLAGS $GEOIP_CFLAGS"
	CPPFLAGS="$CPPFLAGS $GEOIP_CFLAGS"
	LDFLAGS="$LDFLAGS $GEOIP_LIBS"
])
AM_CONDITIONAL([HAVE_GEOIP], test x"$enable_geoip" = "xyes")

dnl unit test
AC_ARG_ENABLE(check, [  --enable-check 		Enable check unit testing],
	[enable_check="$enableval"],[enable_check="no"])

AS_IF([test "x$enable_check" = "xyes"], [
	PKG_CHECK_MODULES([check], [check >= 0.8.2], AC_DEFINE([HAVE_CHECK], [1], [Use CHECK]))
	CFLAGS="$CFLAGS $CHECK_CFLAGS"
	CPPFLAGS="$CPPFLAGS $CHECK_CFLAGS"
	LDFLAGS="$LDFLAGS $CHECK_LIBS"
])
AM_CONDITIONAL([HAVE_CHECK], test x"$enable_check" = "xyes")

AC_OUTPUT
