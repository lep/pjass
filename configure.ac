AC_INIT(pjass, 1.03, cilibrar@gmail.com)
AC_CONFIG_SRCDIR([src/misc.c])
AM_INIT_AUTOMAKE
AM_CONFIG_HEADER([src/config.h])
#AC_CANONICAL_TARGET
AC_CANONICAL_HOST
#AC_CANONICAL_SYSTEM
printenv | grep -i host
echo "hostos is $host_os"
case $host_os in
 cygwin ) CYGOPTS=-mnocygwin ;;
      * ) CYGOPTS=;;
esac
echo "CYGOPTS is $CYGOPTS"
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
