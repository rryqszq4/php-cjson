dnl $Id$
dnl config.m4 for extension cjson

dnl Comments in this file start with the string 'dnl'.
dnl Remove where necessary. This file will not work
dnl without editing.

dnl If your extension references something external, use with:

dnl PHP_ARG_WITH(cjson, for cjson support,
dnl Make sure that the comment is aligned:
dnl [  --with-cjson             Include cjson support])

dnl Otherwise use enable:

dnl PHP_ARG_ENABLE(cjson, whether to enable cjson support,
dnl Make sure that the comment is aligned:
dnl [  --enable-cjson           Enable cjson support])

if test "$PHP_CJSON" != "no"; then
  dnl Write more examples of tests here...

  dnl # --with-cjson -> check with-path
  dnl SEARCH_PATH="/usr/local /usr"     # you might want to change this
  dnl SEARCH_FOR="/include/cjson.h"  # you most likely want to change this
  dnl if test -r $PHP_CJSON/$SEARCH_FOR; then # path given as parameter
  dnl   CJSON_DIR=$PHP_CJSON
  dnl else # search default path list
  dnl   AC_MSG_CHECKING([for cjson files in default path])
  dnl   for i in $SEARCH_PATH ; do
  dnl     if test -r $i/$SEARCH_FOR; then
  dnl       CJSON_DIR=$i
  dnl       AC_MSG_RESULT(found in $i)
  dnl     fi
  dnl   done
  dnl fi
  dnl
  dnl if test -z "$CJSON_DIR"; then
  dnl   AC_MSG_RESULT([not found])
  dnl   AC_MSG_ERROR([Please reinstall the cjson distribution])
  dnl fi

  dnl # --with-cjson -> add include path
  dnl PHP_ADD_INCLUDE($CJSON_DIR/include)

  dnl # --with-cjson -> check for lib and symbol presence
  dnl LIBNAME=cjson # you may want to change this
  dnl LIBSYMBOL=cjson # you most likely want to change this 

  dnl PHP_CHECK_LIBRARY($LIBNAME,$LIBSYMBOL,
  dnl [
  dnl   PHP_ADD_LIBRARY_WITH_PATH($LIBNAME, $CJSON_DIR/lib, CJSON_SHARED_LIBADD)
  dnl   AC_DEFINE(HAVE_CJSONLIB,1,[ ])
  dnl ],[
  dnl   AC_MSG_ERROR([wrong cjson lib version or lib not found])
  dnl ],[
  dnl   -L$CJSON_DIR/lib -lm
  dnl ])
  dnl
  dnl PHP_SUBST(CJSON_SHARED_LIBADD)

  PHP_NEW_EXTENSION(cjson, cjson.c, $ext_shared)
fi
