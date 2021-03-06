AC_INIT(
    [the_silver_searcher],
    [2.2.0],
    [https://github.com/ggreer/the_silver_searcher/issues],
    [the_silver_searcher],
    [https://github.com/ggreer/the_silver_searcher])

AM_INIT_AUTOMAKE([no-define foreign subdir-objects])

AC_PROG_CC
AC_PROG_CC_C99
AM_PROG_CC_C_O
AC_PREREQ([2.59])
AC_PROG_GREP

m4_ifdef(
    [AM_SILENT_RULES],
    [AM_SILENT_RULES([yes])])

m4_include([m4/ax_pthread.m4])
AX_PTHREAD(
    [AC_CHECK_HEADERS([pthread.h])],
    [AC_MSG_WARN([No pthread support. Ag will be slower due to running single-threaded.])]
)

# Run CFLAGS="-pg" ./configure if you want debug symbols
if test "`$GREP -c '\b-O' >/dev/null ; echo $?`" = "0" ; then
    CFLAGS="$CFLAGS -O2"
fi

CFLAGS="$CFLAGS $PTHREAD_CFLAGS $PCRE_CFLAGS -Wall -Wextra -Wformat=2 -Wno-format-nonliteral -Wshadow"
CFLAGS="$CFLAGS -Wpointer-arith -Wcast-qual -Wmissing-prototypes -Wno-missing-braces -D_GNU_SOURCE"
LDFLAGS="$LDFLAGS"

# https://github.com/ggreer/the_silver_searcher/pull/1266
AC_CANONICAL_HOST
is_windows=
case ${host_os} in
*mingw*)
    AC_CHECK_LIB(shlwapi, main,, AC_MSG_ERROR(libshlwapi missing))
    is_windows=yes
;;
*cygwin*)
    is_windows=yes
;;
esac

AM_CONDITIONAL([WINDOWS], [test "x$is_windows" = "xyes"])

LIBS="$PTHREAD_LIBS $LIBS"

AC_ARG_WITH([pcre2],
    [AS_HELP_STRING([--with-pcre2], [Enable experimental support for libpcre2])])

AS_IF([test "x$with_pcre2" = "xyes"], [
      PKG_CHECK_MODULES([PCRE2], [libpcre2-8], [
                        AC_DEFINE([HAVE_PCRE2], [], [Use libpcre2])
                        AC_DEFINE([PCRE2_CODE_UNIT_WIDTH], [8], [8-byte characters for libpcre2])
                        AC_DEFINE([USE_PCRE_JIT], [], [Use PCRE JIT])
                        CFLAGS="$CFLAGS $PCRE2_CFLAGS"
                        ])
      ], [
      PKG_CHECK_MODULES([PCRE], [libpcre], [
                        AC_DEFINE([HAVE_PCRE], [], [Use libpcre])
                        CFLAGS="$CFLAGS $PCRE_CFLAGS"
                        AC_CHECK_DECL([PCRE_CONFIG_JIT], [AC_DEFINE([USE_PCRE_JIT], [], [Use PCRE JIT])],
                                      [], [#include <pcre.h>])
                        ])
])
AM_CONDITIONAL([HAVE_PCRE2], [test "x$with_pcre2" = "xyes"])

AC_ARG_ENABLE([zlib],
    AS_HELP_STRING([--disable-zlib], [Disable zlib compressed search support]))

AS_IF([test "x$enable_zlib" != "xno"], [
    AC_CHECK_HEADERS([zlib.h])
    AC_SEARCH_LIBS([inflate], [zlib, z])
])

AC_ARG_ENABLE([lzma],
    AS_HELP_STRING([--disable-lzma], [Disable lzma compressed search support]))

AS_IF([test "x$enable_lzma" != "xno"], [
    AC_CHECK_HEADERS([lzma.h])
    PKG_CHECK_MODULES([LZMA], [liblzma])
])

AC_CHECK_DECL([CPU_ZERO, CPU_SET], [AC_DEFINE([USE_CPU_SET], [], [Use CPU_SET macros])] , [], [#include <sched.h>])
AC_CHECK_HEADERS([sys/cpuset.h err.h])

AC_CHECK_MEMBER([struct dirent.d_type], [AC_DEFINE([HAVE_DIRENT_DTYPE], [], [Have dirent struct member d_type])], [], [[#include <dirent.h>]])
AC_CHECK_MEMBER([struct dirent.d_namlen], [AC_DEFINE([HAVE_DIRENT_DNAMLEN], [], [Have dirent struct member d_namlen])], [], [[#include <dirent.h>]])

#gcflymoto fopencookie breaks compression tests
#AC_CHECK_FUNCS(fgetln fopencookie getline realpath strlcpy strndup vasprintf madvise posix_fadvise pthread_setaffinity_np pledge)
AC_CHECK_FUNCS(fgetln getline realpath strlcpy strndup vasprintf madvise posix_fadvise pthread_setaffinity_np pledge)

AC_CONFIG_FILES([Makefile the_silver_searcher.spec])
AC_CONFIG_HEADERS([src/config.h])

AC_CHECK_PROGS(
    [CLANG_FORMAT],
    [clang-format-3.8 clang-format-3.7 clang-format-3.6 clang-format],
    [no]
)
AM_CONDITIONAL([HAS_CLANG_FORMAT], [test x$CLANG_FORMAT != xno])
AM_COND_IF(
    [HAS_CLANG_FORMAT],
    [AC_MSG_NOTICE([clang-format found. 'make test' will detect improperly-formatted files.])],
    [AC_MSG_WARN([clang-format not found. 'make test' will not detect improperly-formatted files.])]
)

AC_OUTPUT
