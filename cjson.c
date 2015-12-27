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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_cjson.h"
#include "cJSON/cJSON.h"

/* If you declare any globals in php_cjson.h uncomment this:
ZEND_DECLARE_MODULE_GLOBALS(cjson)
*/

/* True global resources - no need for thread safety here */
static int le_cjson;

#define PHP_CJSON_VERSION "0.0.1"

#define PHP_CJSON_OUTPUT_ARRAY 0
#define PHP_CJSON_OUTPUT_OBJECT 1

static int cjson_determine_array_type(zval **val TSRMLS_DC);
static cJSON *php_cjson_encode_array(zval *val TSRMLS_DC);
static cJSON *php_cjson_encode(zval *val TSRMLS_DC);

PHP_FUNCTION(cjson_encode);
PHP_FUNCTION(cjson_decode);

ZEND_BEGIN_ARG_INFO_EX(arginfo_cjson_encode, 0, 0, 1)
	ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

/* {{{ cjson_functions[]
 *
 * Every user visible function must have an entry in cjson_functions[].
 */
const zend_function_entry cjson_functions[] = {
	PHP_FE(confirm_cjson_compiled,	NULL)		/* For testing, remove later. */
	PHP_FE(cjson_encode, arginfo_cjson_encode)
	PHP_FE(cjson_decode, NULL)
	PHP_FE_END	/* Must be the last line in cjson_functions[] */
};
/* }}} */

/* {{{ cjson_module_entry
 */
zend_module_entry cjson_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
	STANDARD_MODULE_HEADER,
#endif
	"cjson",
	cjson_functions,
	PHP_MINIT(cjson),
	PHP_MSHUTDOWN(cjson),
	PHP_RINIT(cjson),		/* Replace with NULL if there's nothing to do at request start */
	PHP_RSHUTDOWN(cjson),	/* Replace with NULL if there's nothing to do at request end */
	PHP_MINFO(cjson),
#if ZEND_MODULE_API_NO >= 20010901
	"0.1", /* Replace with version number for your extension */
#endif
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_CJSON
ZEND_GET_MODULE(cjson)
#endif

/* {{{ PHP_INI
 */
/* Remove comments and fill if you need to have entries in php.ini
PHP_INI_BEGIN()
    STD_PHP_INI_ENTRY("cjson.global_value",      "42", PHP_INI_ALL, OnUpdateLong, global_value, zend_cjson_globals, cjson_globals)
    STD_PHP_INI_ENTRY("cjson.global_string", "foobar", PHP_INI_ALL, OnUpdateString, global_string, zend_cjson_globals, cjson_globals)
PHP_INI_END()
*/
/* }}} */

/* {{{ php_cjson_init_globals
 */
/* Uncomment this function if you have INI entries
static void php_cjson_init_globals(zend_cjson_globals *cjson_globals)
{
	cjson_globals->global_value = 0;
	cjson_globals->global_string = NULL;
}
*/
/* }}} */

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(cjson)
{
	/* If you have INI entries, uncomment these lines 
	REGISTER_INI_ENTRIES();
	*/
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(cjson)
{
	/* uncomment this line if you have INI entries
	UNREGISTER_INI_ENTRIES();
	*/
	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request start */
/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(cjson)
{
	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request end */
/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
PHP_RSHUTDOWN_FUNCTION(cjson)
{
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(cjson)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "cjson support", "enabled");
	php_info_print_table_end();

	/* Remove comments if you have entries in php.ini
	DISPLAY_INI_ENTRIES();
	*/
}
/* }}} */


/* Remove the following function when you have succesfully modified config.m4
   so that your module can be compiled into PHP, it exists only for testing
   purposes. */

/* Every user-visible function in PHP should document itself in the source */
/* {{{ proto string confirm_cjson_compiled(string arg)
   Return a string to confirm that the module is compiled in */
PHP_FUNCTION(confirm_cjson_compiled)
{
	char *arg = NULL;
	int arg_len, len;
	char *strg;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &arg, &arg_len) == FAILURE) {
		return;
	}

	len = spprintf(&strg, 0, "Congratulations! You have successfully modified ext/%.78s/config.m4. Module %.78s is now compiled into PHP.", "cjson", arg);
	RETURN_STRINGL(strg, len, 0);
}
/* }}} */
/* The previous line is meant for vim and emacs, so it can correctly fold and 
   unfold functions in source code. See the corresponding marks just before 
   function definition, where the functions purpose is also documented. Please 
   follow this convention for the convenience of others editing your code.
*/

static int cjson_determine_array_type(zval **val TSRMLS_DC)
{
	int i;
	HashTable *myht = HASH_OF(*val);

	i = myht ? zend_hash_num_elements(myht) : 0;
	if (i > 0) {
		char *key;
		ulong index, idx;
		uint key_len;
		HashPosition pos;

		zend_hash_internal_pointer_reset_ex(myht, &pos);
		idx = 0;
		for (;; zend_hash_move_forward_ex(myht, &pos)) {
			i = zend_hash_get_current_key_ex(myht, &key, &key_len, &index, 0, &pos);
			if (i == HASH_KEY_NON_EXISTANT) {
				break;
			}

			if (i == HASH_KEY_IS_STRING) {
				return 1;
			} else {
				if (index != idx) {
					return 1;
				}
			}
			idx++;
		}
	}

	return PHP_CJSON_OUTPUT_ARRAY;
}

static cJSON *php_cjson_encode_array(zval *val TSRMLS_DC)
{
	cJSON *root;
	HashTable *hash_table;
	int i, r;

	if (Z_TYPE_P(val) == IS_ARRAY)
	{
		hash_table = HASH_OF(val);
		r = cjson_determine_array_type(&val TSRMLS_CC);
		if (r == PHP_CJSON_OUTPUT_ARRAY){
			root = cJSON_CreateArray();
		}else {
			root = cJSON_CreateObject();
		}
	}else {
		hash_table = Z_OBJPROP_P(val);
		r = PHP_CJSON_OUTPUT_OBJECT;
		root = cJSON_CreateObject();
	}

	if (hash_table && hash_table->nApplyCount > 1) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "recursion detected");
		root = cJSON_CreateNull();
		return;
	}

	i = hash_table ? zend_hash_num_elements(hash_table) : 0;

	if (i > 0)
	{
		HashPosition pos;
		char *key;
		ulong index;
		uint key_len;
		zval **data;
		HashTable *tmp_ht;
		char key_index[10];

		zend_hash_internal_pointer_reset_ex(hash_table, &pos);
		for (;; zend_hash_move_forward_ex(hash_table, &pos))
		{
			i = zend_hash_get_current_key_ex(hash_table, &key, &key_len, &index, 0, &pos);
			if (i == HASH_KEY_NON_EXISTANT)
				break;

			if (zend_hash_get_current_data_ex(hash_table, (void **)&data, &pos) == SUCCESS)
			{
				tmp_ht = HASH_OF(*data);
				if (tmp_ht){
					tmp_ht->nApplyCount++;
				}

				if (r == PHP_CJSON_OUTPUT_ARRAY){
					cJSON_AddItemToArray(root, php_cjson_encode(*data TSRMLS_CC));
				}else if (r == PHP_CJSON_OUTPUT_OBJECT){
					if (i == HASH_KEY_IS_STRING){
						cJSON_AddItemToObject(root, key, php_cjson_encode(*data TSRMLS_CC));
					}else {
						sprintf(key_index, "%d", index);
						cJSON_AddItemToObject(root, key_index, php_cjson_encode(*data TSRMLS_CC));
					}
				}

				if (tmp_ht) {
					tmp_ht->nApplyCount--;
				}
			}
		}
	}

	return root;
}

static cJSON *php_cjson_encode(zval *val TSRMLS_DC)
{
	cJSON *root;

	switch(Z_TYPE_P(val))
	{
		case IS_NULL:
			root = cJSON_CreateNull();
			break;

		case IS_BOOL:
			if (Z_BVAL_P(val)){
				root = cJSON_CreateTrue();
			}else {
				root = cJSON_CreateFalse();
			}
			break;

		case IS_LONG:
			root = cJSON_CreateNumber(Z_LVAL_P(val));
			break;

		case IS_DOUBLE:
			root = cJSON_CreateNumber(Z_DVAL_P(val));
			break;

		case IS_STRING:
			root = cJSON_CreateString(Z_STRVAL_P(val));
			break;

		case IS_ARRAY:
		case IS_OBJECT:
			root = php_cjson_encode_array(val TSRMLS_CC);
			break;

		default:
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "type is unsupported, encode as null");
			root = cJSON_CreateNull();
			break;
	}

	return root;
}

PHP_FUNCTION(cjson_encode)
{
	zval *param;
	cJSON *root;
	char *buf;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &param) == FAILURE)
	{
		return ;
	}

    root = php_cjson_encode(param TSRMLS_CC);

    buf=cJSON_Print(root);
    cJSON_Delete(root);

    ZVAL_STRINGL(return_value, buf, strlen(buf)-1, 1);

    //printf("%s\n",buf);
    free(buf);

}

PHP_FUNCTION(cjson_decode)
{

}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
