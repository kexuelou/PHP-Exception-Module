dnl $Id$
dnl config.m4 for extension CatchIt

dnl Comments in this file start with the string 'dnl'.
dnl Remove where necessary. This file will not work
dnl without editing.

dnl If your extension references something external, use with:

dnl PHP_ARG_WITH(CatchIt, for CatchIt support,
dnl Make sure that the comment is aligned:
dnl [  --with-CatchIt             Include CatchIt support])

dnl Otherwise use enable:

dnl PHP_ARG_ENABLE(CatchIt, whether to enable CatchIt support,
dnl Make sure that the comment is aligned:
dnl [  --enable-CatchIt           Enable CatchIt support])

if test "$PHP_CATCHIT" != "no"; then
  dnl Write more examples of tests here...

  dnl # --with-CatchIt -> check with-path
  dnl SEARCH_PATH="/usr/local /usr"     # you might want to change this
  dnl SEARCH_FOR="/include/CatchIt.h"  # you most likely want to change this
  dnl if test -r $PHP_CATCHIT/$SEARCH_FOR; then # path given as parameter
  dnl   CATCHIT_DIR=$PHP_CATCHIT
  dnl else # search default path list
  dnl   AC_MSG_CHECKING([for CatchIt files in default path])
  dnl   for i in $SEARCH_PATH ; do
  dnl     if test -r $i/$SEARCH_FOR; then
  dnl       CATCHIT_DIR=$i
  dnl       AC_MSG_RESULT(found in $i)
  dnl     fi
  dnl   done
  dnl fi
  dnl
  dnl if test -z "$CATCHIT_DIR"; then
  dnl   AC_MSG_RESULT([not found])
  dnl   AC_MSG_ERROR([Please reinstall the CatchIt distribution])
  dnl fi

  dnl # --with-CatchIt -> add include path
  dnl PHP_ADD_INCLUDE($CATCHIT_DIR/include)

  dnl # --with-CatchIt -> check for lib and symbol presence
  dnl LIBNAME=CatchIt # you may want to change this
  dnl LIBSYMBOL=CatchIt # you most likely want to change this 

  dnl PHP_CHECK_LIBRARY($LIBNAME,$LIBSYMBOL,
  dnl [
  dnl   PHP_ADD_LIBRARY_WITH_PATH($LIBNAME, $CATCHIT_DIR/$PHP_LIBDIR, CATCHIT_SHARED_LIBADD)
  dnl   AC_DEFINE(HAVE_CATCHITLIB,1,[ ])
  dnl ],[
  dnl   AC_MSG_ERROR([wrong CatchIt lib version or lib not found])
  dnl ],[
  dnl   -L$CATCHIT_DIR/$PHP_LIBDIR -lm
  dnl ])
  dnl
  dnl PHP_SUBST(CATCHIT_SHARED_LIBADD)

  PHP_NEW_EXTENSION(CatchIt, CatchIt.c, $ext_shared)
fi
