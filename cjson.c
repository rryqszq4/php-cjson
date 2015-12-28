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
#include <float.h>
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

static zval *php_cjson_zval_null();
static zval *php_cjson_zval_boolean(int boolean_value);
static zval *php_cjson_zval_number(cJSON *item);
static zval *php_cjson_zval_string_ptr(const char *str);
static zval *php_cjson_zval_string(cJSON *item);
static zval *php_cjson_zval_array(cJSON *item, int depth TSRMLS_DC);
static zval *php_cjson_zval_object(cJSON *item, int depth TSRMLS_DC);
static zval *php_cjson_decode(cJSON *item, int depth TSRMLS_DC);

static void php_cjson_zval_null_r(zval *v);
static void php_cjson_zval_boolean_r(zval *v, int boolean_value);
static void php_cjson_zval_number_r(zval *v, cJSON *item);
static void php_cjson_zval_string_r(zval *v, cJSON *item);
static void php_cjson_zval_array_r(zval *v, cJSON *item, int depth TSRMLS_DC);
static void php_cjson_zval_object_r(zval *v, cJSON *item, int depth TSRMLS_DC);
static void php_cjson_decode_r(zval *v, cJSON *item, int depth TSRMLS_DC);

static void php_cjson_decode_return(zval *return_value, cJSON *item, int depth TSRMLS_DC);

PHP_FUNCTION(cjson_encode);
PHP_FUNCTION(cjson_decode);

ZEND_BEGIN_ARG_INFO_EX(arginfo_cjson_encode, 0, 0, 1)
	ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_cjson_decode, 0, 0, 1)
	ZEND_ARG_INFO(0, str)
ZEND_END_ARG_INFO()

/* {{{ cjson_functions[]
 *
 * Every user visible function must have an entry in cjson_functions[].
 */
const zend_function_entry cjson_functions[] = {
	PHP_FE(confirm_cjson_compiled,	NULL)		/* For testing, remove later. */
	PHP_FE(cjson_encode, arginfo_cjson_encode)
	PHP_FE(cjson_decode, arginfo_cjson_decode)
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
	php_info_print_table_row(2, "php cjson version", PHP_CJSON_VERSION);
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

static char* cjson_return_zval_type(zend_uchar type)
{
  switch (type) {
    case IS_NULL:
      return "IS_NULL";
      break;
    case IS_BOOL:
      return "IS_BOOL";
      break;
    case IS_LONG:
      return "IS_LONG";
      break;
    case IS_DOUBLE:
      return "IS_DOUBLE";
      break;
    case IS_STRING:
      return "IS_STRING";
      break;
    case IS_ARRAY:
      return "IS_ARRAY";
      break;
    case IS_OBJECT:
      return "IS_OBJECT";
      break;
    case IS_RESOURCE:
      return "IS_RESOURCE";
      break;
    default :
      return "unknown";
  }
}

void cjson_dump_zval(zval *data)
{
  php_printf("zval<%p> {\n", data);
  php_printf("  refcount__gc -> %d\n", data->refcount__gc);
  php_printf("  is_ref__gc -> %d\n", data->is_ref__gc);
  php_printf("  type -> %s\n", cjson_return_zval_type(data->type));
  php_printf("  zand_value<%p> {\n", data->value);

  php_printf("    lval -> %d\n", data->value.lval);
  php_printf("    dval -> %e\n", data->value.dval);
  if (Z_TYPE_P(data) == IS_STRING){
    php_printf("    str -> %s\n", Z_STRVAL_P(data));
  }
  php_printf("    *ht<%p> {\n", data->value.ht);
  php_printf("    }\n");
  php_printf("    obj<%p> {\n", data->value.obj);
  php_printf("    }\n");
  php_printf("  }\n");

  php_printf("}\n");
}

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

static zval *php_cjson_zval_null()
{
	zval *copy;

	MAKE_STD_ZVAL(copy);
	ZVAL_NULL(copy);
	
	return copy;
}

static zval *php_cjson_zval_boolean(int boolean_value)
{
	zval *copy;

	MAKE_STD_ZVAL(copy);
	ZVAL_BOOL(copy, boolean_value);

	return copy;
}

static zval *php_cjson_zval_number(cJSON *item)
{
	char *str=0;
	zval *copy;

	MAKE_STD_ZVAL(copy);

	double d=item->valuedouble;
	if (d==0)
	{
		ZVAL_STRINGL(copy, "0", sizeof("0") - 1, 0);
	}
	else if (fabs(((double)item->valueint)-d)<=DBL_EPSILON && d<=INT_MAX && d>=INT_MIN)
	{
		ZVAL_LONG(copy, item->valueint);
	}
	else
	{
		
		if (fabs(floor(d)-d)<=DBL_EPSILON && fabs(d)<1.0e60) {ZVAL_DOUBLE(copy, d);}
		else if (fabs(d)<1.0e-6 || fabs(d)>1.0e9)			 {ZVAL_DOUBLE(copy, d);}
		else												 {ZVAL_DOUBLE(copy, d);}
	}
	return copy;
}

static zval *php_cjson_zval_string_ptr(const char *str)
{
	zval *copy;

	MAKE_STD_ZVAL(copy);

	ZVAL_STRINGL(copy, str, strlen(str), 1);

	return copy;
}

static zval *php_cjson_zval_string(cJSON *item)
{
	return php_cjson_zval_string_ptr(item->valuestring);
}

static zval *php_cjson_zval_array(cJSON *item, int depth TSRMLS_DC)
{
	zval *copy; zval *ret;
	MAKE_STD_ZVAL(copy);
	array_init(copy);

	cJSON *child=item->child;
	int numentries=0;
	
	/* How many entries in the array? */
	while (child) numentries++,child=child->next;
	/* Explicitly handle numentries==0 */
	if (!numentries)
	{
		return copy;
	}

	
	/* Retrieve all the results: */
	child=item->child;
	while (child)
	{
		ret=php_cjson_decode(child,depth+1 TSRMLS_CC);
		add_next_index_zval(copy, ret);

		child=child->next;
	}

	return copy;	
}

static zval *php_cjson_zval_object(cJSON *item, int depth TSRMLS_DC)
{
	zval *copy; zval *ret;
	MAKE_STD_ZVAL(copy);
	array_init(copy);

	cJSON *child=item->child;
	int numentries=0;
	/* Count the number of entries. */
	while (child) numentries++,child=child->next;
	/* Explicitly handle empty object case */
	if (!numentries)
	{
		return copy;
	}

	/* Collect all the results into our arrays: */
	child=item->child;depth++;
	while (child)
	{
		ret=php_cjson_decode(child,depth+1 TSRMLS_CC);
		add_assoc_zval(copy, child->string, ret);
		child=child->next;
	}
		
	return copy;	
}

static zval *php_cjson_decode(cJSON *item, int depth TSRMLS_DC)
{
	zval *out=NULL;

	if (!item) {ZVAL_NULL(out); return out;}
	
	switch ((item->type)&255)
	{
		case cJSON_NULL:	out=php_cjson_zval_null();	break;
		case cJSON_False:	out=php_cjson_zval_boolean(0);break;
		case cJSON_True:	out=php_cjson_zval_boolean(1); break;
		case cJSON_Number:	out=php_cjson_zval_number(item);break;
		case cJSON_String:	out=php_cjson_zval_string(item);break;
		case cJSON_Array:	out=php_cjson_zval_array(item,depth TSRMLS_CC);break;
		case cJSON_Object:	out=php_cjson_zval_object(item,depth TSRMLS_CC);break;
	}

	return out;
}

static void php_cjson_zval_null_r(zval *v)
{
	ZVAL_NULL(v);
	
	return;
}

static void php_cjson_zval_boolean_r(zval *v, int boolean_value)
{

	ZVAL_BOOL(v, boolean_value);

	return;
}

static void php_cjson_zval_number_r(zval *v, cJSON *item)
{
	char *str=0;

	double d=item->valuedouble;
	if (d==0)
	{
		ZVAL_STRINGL(v, "0", sizeof("0") - 1, 0);
	}
	else if (fabs(((double)item->valueint)-d)<=DBL_EPSILON && d<=INT_MAX && d>=INT_MIN)
	{
		ZVAL_LONG(v, item->valueint);
	}
	else
	{
		
		if (fabs(floor(d)-d)<=DBL_EPSILON && fabs(d)<1.0e60) {ZVAL_DOUBLE(v, d);}
		else if (fabs(d)<1.0e-6 || fabs(d)>1.0e9)			 {ZVAL_DOUBLE(v, d);}
		else												 {ZVAL_DOUBLE(v, d);}
	}
	return;
}

static void php_cjson_zval_string_r(zval *v, cJSON *item)
{

	ZVAL_STRINGL(v, item->valuestring, strlen(item->valuestring), 1);

	return ;
}

static void php_cjson_zval_array_r(zval *v, cJSON *item, int depth TSRMLS_DC)
{
	zval *ret;
	array_init(v);

	cJSON *child=item->child;
	int numentries=0;
	
	/* How many entries in the array? */
	while (child) numentries++,child=child->next;
	/* Explicitly handle numentries==0 */
	if (!numentries)
	{
		return ;
	}

	
	/* Retrieve all the results: */
	child=item->child;
	while (child)
	{
		MAKE_STD_ZVAL(ret);
		php_cjson_decode_r(ret, child,depth+1 TSRMLS_CC);
		add_next_index_zval(v, ret);

		child=child->next;
	}

	return ;	
}

static void php_cjson_zval_object_r(zval *v, cJSON *item, int depth TSRMLS_DC)
{
	zval *ret;
	array_init(v);

	cJSON *child=item->child;
	int numentries=0;
	/* Count the number of entries. */
	while (child) numentries++,child=child->next;
	/* Explicitly handle empty object case */
	if (!numentries)
	{
		return ;
	}

	/* Collect all the results into our arrays: */
	child=item->child;depth++;
	while (child)
	{
		MAKE_STD_ZVAL(ret);
		php_cjson_decode_r(ret, child,depth+1 TSRMLS_CC);
		add_assoc_zval(v, child->string, ret);
		child=child->next;
	}
		
	return ;	
}

static void php_cjson_decode_r(zval *v, cJSON *item, int depth TSRMLS_DC)
{

	if (!item) {ZVAL_NULL(v); return ;}
	
	switch ((item->type)&255)
	{
		case cJSON_NULL:	php_cjson_zval_null_r(v);	break;
		case cJSON_False:	php_cjson_zval_boolean_r(v, 0);break;
		case cJSON_True:	php_cjson_zval_boolean_r(v, 1); break;
		case cJSON_Number:	php_cjson_zval_number_r(v, item);break;
		case cJSON_String:	php_cjson_zval_string_r(v, item);break;
		case cJSON_Array:	php_cjson_zval_array_r(v, item,depth TSRMLS_CC);break;
		case cJSON_Object:	php_cjson_zval_object_r(v, item,depth TSRMLS_CC);break;
	}

	return ;
}

static void php_cjson_decode_return(zval *return_value, cJSON *item, int depth TSRMLS_DC)
{
	double d;
	zval *ret;
	cJSON *child;
	int numentries;

	if (!item)
	{
		ZVAL_NULL(return_value);
		return ;
	}

	switch ((item->type)&255)
	{
		case cJSON_NULL:
				ZVAL_NULL(return_value);
			break;
		case cJSON_False:
				ZVAL_BOOL(return_value, 0);
			break;
		case cJSON_True:
				ZVAL_BOOL(return_value, 1);
			break;
		case cJSON_Number:
				d=item->valuedouble;
				if (d==0)
				{
					ZVAL_STRINGL(return_value, "0", sizeof("0") - 1, 0);
				}
				else if (fabs(((double)item->valueint)-d)<=DBL_EPSILON && d<=INT_MAX && d>=INT_MIN)
				{
					ZVAL_LONG(return_value, item->valueint);
				}
				else
				{
					if (fabs(floor(d)-d)<=DBL_EPSILON && fabs(d)<1.0e60) {ZVAL_DOUBLE(return_value, d);}
					else if (fabs(d)<1.0e-6 || fabs(d)>1.0e9)			 {ZVAL_DOUBLE(return_value, d);}
					else												 {ZVAL_DOUBLE(return_value, d);}
				}
			break;
		case cJSON_String:
				ZVAL_STRINGL(return_value, item->valuestring, strlen(item->valuestring), 1);
			break;
		case cJSON_Array:
				array_init(return_value);
				child=item->child;
				numentries=0;
				
				/* How many entries in the array? */
				while (child) numentries++,child=child->next;
				/* Explicitly handle numentries==0 */
				if (!numentries)
				{
					return ;
				}

				/* Retrieve all the results: */
				child=item->child;
				while (child)
				{
					MAKE_STD_ZVAL(ret);
					php_cjson_decode_r(ret, child,depth+1 TSRMLS_CC); 
					add_next_index_zval(return_value, ret);
					child=child->next;
				}
			break;
		case cJSON_Object:
				array_init(return_value);
				child=item->child;
				numentries=0;
				/* Count the number of entries. */
				while (child) numentries++,child=child->next;
				/* Explicitly handle empty object case */
				if (!numentries)
				{
					return ;
				}

				/* Collect all the results into our arrays: */
				child=item->child;depth++;
				while (child)
				{
					MAKE_STD_ZVAL(ret);
					php_cjson_decode_r(ret, child,depth+1 TSRMLS_CC);
					add_assoc_zval(return_value, child->string, ret);
					child=child->next;
				}
			break;
	}

	return ;
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

    ZVAL_STRINGL(return_value, buf, strlen(buf), 1);

    //printf("%s\n",buf);
    free(buf);

}

PHP_FUNCTION(cjson_decode)
{
	char *str;
	int str_len;
	cJSON *item;
	zval *node;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &str, &str_len) == FAILURE)
	{
		return ;
	}

	item = cJSON_Parse(str);
	if (!item) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Error before: [%s]", cJSON_GetErrorPtr());
		ZVAL_NULL(return_value);
	}
	else
	{
		//php_cjson_decode_return(return_value, item, 1 TSRMLS_CC);

		MAKE_STD_ZVAL(node);
		php_cjson_decode_return(node, item, 1 TSRMLS_CC);
		RETVAL_ZVAL(node, 1, 0);
	}

	cJSON_Delete(item);

}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
