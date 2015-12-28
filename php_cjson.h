/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2013 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author: rryqszq4                                                     |
  +----------------------------------------------------------------------+
*/

/* $Id$ */

#ifndef PHP_CJSON_H
#define PHP_CJSON_H

extern zend_module_entry cjson_module_entry;
#define phpext_cjson_ptr &cjson_module_entry

#ifdef PHP_WIN32
#	define PHP_CJSON_API __declspec(dllexport)
#elif defined(__GNUC__) && __GNUC__ >= 4
#	define PHP_CJSON_API __attribute__ ((visibility("default")))
#else
#	define PHP_CJSON_API
#endif

#ifdef ZTS
#include "TSRM.h"
#endif

PHP_MINIT_FUNCTION(cjson);
PHP_MSHUTDOWN_FUNCTION(cjson);
PHP_RINIT_FUNCTION(cjson);
PHP_RSHUTDOWN_FUNCTION(cjson);
PHP_MINFO_FUNCTION(cjson);

PHP_FUNCTION(confirm_cjson_compiled);	/* For testing, remove later. */

/* 
  	Declare any global variables you may need between the BEGIN
	and END macros here:     

ZEND_BEGIN_MODULE_GLOBALS(cjson)
	long  global_value;
	char *global_string;
ZEND_END_MODULE_GLOBALS(cjson)
*/

/* In every utility function you add that needs to use variables 
   in php_cjson_globals, call TSRMLS_FETCH(); after declaring other 
   variables used by that function, or better yet, pass in TSRMLS_CC
   after the last function argument and declare your utility function
   with TSRMLS_DC after the last declared argument.  Always refer to
   the globals in your function as CJSON_G(variable).  You are 
   encouraged to rename these macros something shorter, see
   examples in any other php module directory.
*/

#ifdef ZTS
#define CJSON_G(v) TSRMG(cjson_globals_id, zend_cjson_globals *, v)
#else
#define CJSON_G(v) (cjson_globals.v)
#endif

#endif	/* PHP_CJSON_H */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
