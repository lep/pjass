AC_INIT(pjass, 1.03, cilibrar@gmail.com)
AC_CONFIG_SRCDIR([src/misc.c])
AM_INIT_AUTOMAKE
AM_CONFIG_HEADER([src/config.h])
AC_CANONICAL_HOST
case $host_os in
 cygwin ) CYGOPTS=-mno-cygwin ;;
      * ) CYGOPTS=;;
esac
AC_SUBST(CYGOPTS)
AC_LANG(C)
AC_PROG_CC
AM_PROG_LEX
AC_PROG_YACC
AC_PROG_INSTALL
AC_CONFIG_FILES([
Makefile
src/Makefile
])
AC_OUTPUT
