#include "php_blacknc.h"
#include "zend_execute.h"

ZEND_FUNCTION(blacknc_init) {
	php_printf("blacknc extension init...\n");
}

ZEND_FUNCTION(blacknc_call) {
	zval *fname;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &fname) == FAILURE) {
		RETURN_NULL();
	}

	zend_function *fe;
	char *lfname;
	lfname = estrndup(Z_STRVAL_P(fname), Z_STRLEN_P(fname));
	zend_str_tolower(lfname, Z_STRLEN_P(fname));
	if (zend_hash_find(EG(function_table), lfname, 	Z_STRLEN_P(fname) + 1, (void **)&fe) == SUCCESS) {
		zend_execute(&fe->op_array TSRMLS_CC);
	} else {
		php_error_docref(NULL TSRMLS_CC, E_ERROR, "Call to undefined function: %s()", Z_STRVAL_P(fname));
	}
	efree(lfname);
}

ZEND_FUNCTION(blacknc_return_null) {
	ZVAL_NULL(return_value);
}
ZEND_FUNCTION(blacknc_return_bool) {
	ZVAL_BOOL(return_value, 1);
}
ZEND_FUNCTION(blacknc_return_false) {
	ZVAL_FALSE(return_value);
}
ZEND_FUNCTION(blacknc_return_long) {
	ZVAL_LONG(return_value, 43);
}
ZEND_FUNCTION(blacknc_return_double) {
	ZVAL_DOUBLE(return_value, 45.3);
}
ZEND_FUNCTION(blacknc_return_string) {
	ZVAL_STRING(return_value, "hello world", 1);
}
ZEND_FUNCTION(blacknc_return_stringl) {
	ZVAL_STRINGL(return_value, "HELLO WORLD", sizeof("HELLO WORLD") - 1, 1);
}
ZEND_FUNCTION(blacknc_isset) {
	zval *name;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &name) == FAILURE) {
		RETURN_FALSE;
	}
	zval **val;
	if (zend_hash_find(&EG(symbol_table), Z_STRVAL_P(name), Z_STRLEN_P(name) + 1, (void **)&val) == FAILURE) {
		RETURN_FALSE;
	}
	PHPWRITE(Z_STRVAL_P(name), Z_STRLEN_P(name));
	php_printf(" value:");
	convert_to_string(*val)
	PHPWRITE(Z_STRVAL_PP(val), Z_STRLEN_PP(val));
	php_printf("\n");
	php_printf("type: %s\n", Z_TYPE_PP(val) == IS_STRING ? "string" : "unknown");
	RETURN_TRUE;
}

zval *get_var_and_separate(char *varname, int varname_len TSRMLS_DC);

ZEND_FUNCTION(blacknc_get_args) {

	zval **foo;
	if (zend_get_parameters_ex(1, &foo) == FAILURE) {
		WRONG_PARAM_COUNT;
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Expected at leat 1 parameter.");
		RETURN_NULL();
	}

	PHPWRITE(Z_STRVAL_PP(foo), Z_STRLEN_PP(foo));
}

ZEND_FUNCTION(blacknc_get_parameters_array_ex) {

	int i, argc = ZEND_NUM_ARGS();
	zval ***foo;

	foo = (zval ***)safe_emalloc(argc, sizeof(zval **), 0);
	if (argc == 0 || zend_get_parameters_array_ex(argc, foo) == FAILURE) {
		efree(foo);
		WRONG_PARAM_COUNT;
	}

	for(i = 0; i < argc; i++) {
		php_var_dump(foo[i], 1 TSRMLS_CC);
	}
	efree(foo);
}


ZEND_FUNCTION(blacknc_test_separate) {

	zval *val;
	MAKE_STD_ZVAL(val);
	ZVAL_STRING(val, "hello", 1);
	zend_hash_add(EG(active_symbol_table), "foo", sizeof("foo"), &val, sizeof(zval **), NULL);
	Z_ADDREF_P(val);
	zend_hash_add(EG(active_symbol_table), "boo", sizeof("boo"), &val, sizeof(zval **), NULL);
	val = get_var_and_separate("foo", sizeof("foo") - 1 TSRMLS_CC);
}

zval *get_var_and_separate(char *varname, int varname_len TSRMLS_DC) {

    zval **varval, *varcopy;
    if (zend_hash_find(EG(active_symbol_table),varname, varname_len + 1, (void**)&varval) == FAILURE)
    {
        /* 如果在符号表里找不到这个变量则直接return */
        return NULL;
    }
 
    if ((*varval)->refcount__gc < 2)
    {   
        //如果这个变量的zval部分的refcount小于2，代表没有别的变量在用，return
        return *varval;
    }
     
    /* 否则，复制一份zval*的值 */
    MAKE_STD_ZVAL(varcopy);
    *varcopy = **varval;

    /* 复制任何在zval*内已分配的结构*/
    zval_copy_ctor(varcopy);

    /* 从符号表中删除原来的变量
     * 这将减少该过程中varval的refcount的值
     */
    zend_hash_del(EG(active_symbol_table), varname, varname_len + 1);
 
    /* 初始化新的zval的refcount，并在符号表中重新添加此变量信息，并将其值与我们的新zval相关联。*/
    varcopy->refcount__gc = 1;
    varcopy->is_ref__gc = 0;
    zend_hash_add(EG(active_symbol_table), varname, varname_len + 1,&varcopy, sizeof(zval*), NULL);
     
    /* 返回新zval的地址 */
    return varcopy;
}

#if (PHP_MAJOR_VERSION > 5) || (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION > 0)
	ZEND_BEGIN_ARG_INFO_EX(blacknc_return_by_ref_arginfo, 0, 1, 0)
	ZEND_END_ARG_INFO()

	ZEND_FUNCTION(blacknc_return_by_ref) {

		zval **a_ptr, *a;

		if (zend_hash_find(&EG(symbol_table), "a", sizeof("a"), (void **)&a_ptr) == FAILURE) {
			ALLOC_INIT_ZVAL(a);
			zend_hash_add(&EG(symbol_table), "a", sizeof("a"), &a, sizeof(zval *), NULL);
		} else {
			a = *a_ptr;
		}

		zval_ptr_dtor(return_value_ptr);
		if (!a->is_ref__gc && a->refcount__gc > 1) {

			zval *tmp;
			MAKE_STD_ZVAL(tmp);
			*tmp = *a;
			zval_copy_ctor(tmp);
			tmp->is_ref__gc = 0;
			tmp->refcount__gc = 1;
			zend_hash_update(&EG(symbol_table), "a", sizeof("a"), &tmp, sizeof(zval *), NULL);
			a = tmp;
		}
		a->is_ref__gc = 1;
		a->refcount__gc++;
		*return_value_ptr = a;
	}
#endif

void php_sample_print_hash_table(HashTable *htable TSRMLS_DC);
void php_sample_print_hash_table_desc(HashTable *htable TSRMLS_DC);
void php_sample_print_hash_table_values(HashTable *htable TSRMLS_DC);
void php_sample_print_hash_table_all(HashTable *htable TSRMLS_DC);
int php_sample_print_zvalpp(void *pDest TSRMLS_DC);
int php_sample_print_args(void *pDest TSRMLS_DC, int num_args, va_list args, zend_hash_key *hash_key);

ZEND_FUNCTION(blacknc_ht) {

	HashTable *htable;

	ALLOC_HASHTABLE(htable);

	/**
	 * int zend_hash_init(
	 * 	HashTable *ht,
	 *  uint nSize,
	 *  hash_func_t pHashFunction,
	 *  dtor_func_t pDestructor,
	 *  zend_bool persistent
	 * );
	 */
	if (zend_hash_init(htable, 50, NULL, ZVAL_PTR_DTOR, 0) == FAILURE) {
		FREE_HASHTABLE(htable);
		return;
	}

	int i;
	for (i = 0; i < 70; i++) {
		zval *zval_p;
		MAKE_STD_ZVAL(zval_p);
		ZVAL_LONG(zval_p, i);
		if (zend_hash_next_index_insert(htable, &zval_p, sizeof(zval_p), NULL) == FAILURE) {
			zval_ptr_dtor(zval_p);
			continue;
		}
	}
	//php_sample_print_hash_table(htable TSRMLS_CC);
	php_sample_print_hash_table_desc(htable TSRMLS_CC);
	//php_sample_print_hash_table_values(htable TSRMLS_CC);
	//php_sample_print_hash_table_all(htable TSRMLS_CC);

	zend_hash_destroy(htable);

	FREE_HASHTABLE(htable);
	return;
}

int php_sample_compare(Bucket **a, Bucket **b TSRMLS_DC) {

	zend_function *a_func = (*a)->pData;
	zend_function *b_func = (*b)->pData;
	return strcasecmp(a_func->common.function_name, b_func->common.function_name);
}

int php_sample_print_fname(void *pDest TSRMLS_DC) {
	php_printf("%s\n", ((zend_function *)pDest)->common.function_name);
	return ZEND_HASH_APPLY_KEEP;
}

void php_sample_zend_hash_apply_desc(HashTable *htable, apply_func_t apply_func TSRMLS_DC);
ZEND_FUNCTION(blacknc_loaded_functions) {

	zend_hash_sort(EG(function_table), zend_qsort, php_sample_compare, 0 TSRMLS_CC);
	//zend_hash_apply(EG(function_table), php_sample_print_fname TSRMLS_CC);
	php_sample_zend_hash_apply_desc(EG(function_table), php_sample_print_fname TSRMLS_CC);
}

ZEND_FUNCTION(blacknc_array) {
	zval *subarr;

	array_init(return_value);
	add_assoc_long(return_value, "life", 43);
	add_index_bool(return_value, 123, 1);
	add_next_index_double(return_value, 23.56);

	add_next_index_string(return_value, "hello world", 1);
	add_next_index_string(return_value, estrdup("hello world"), 0);

	MAKE_STD_ZVAL(subarr);
	array_init(subarr);

	add_assoc_long(subarr, "age", 100);
	add_index_long(subarr, 23, 294);
	add_index_long(subarr, 56, 300);
	add_index_zval(return_value, 234, subarr);
}

void php_sample_zend_hash_apply_desc(HashTable *htable, apply_func_t apply_func TSRMLS_DC) {

	HashPosition pos;

	int num_elements;
	for (
			zend_hash_internal_pointer_end_ex(htable, &pos),
			num_elements = zend_hash_num_elements(htable);
			//zend_hash_has_more_elements_ex(htable, &pos) == SUCCESS;
			num_elements > 0;
			num_elements--,
			zend_hash_move_backwards_ex(htable, &pos)
		) {

		char *key;
		uint keyLen;
		ulong index;
		void *pDest;

		int type = zend_hash_get_current_key_ex(htable, &key, &keyLen, &index, 0, &pos);
		if (zend_hash_get_current_data_ex(htable, (void **)&pDest, &pos) == FAILURE) {
			continue;
		}

		apply_func(pDest TSRMLS_CC);
	}
}

void php_sample_print_hash_table_all(HashTable *htable TSRMLS_DC) {

	zend_hash_apply_with_arguments(htable TSRMLS_CC, php_sample_print_args, 0);
}

void php_sample_print_hash_table_values(HashTable *htable TSRMLS_DC) {

	zend_hash_apply(htable, php_sample_print_zvalpp TSRMLS_CC);
}

int php_sample_print_args(void *pDest TSRMLS_DC, int num_args, va_list args, zend_hash_key *hash_key) {

	zval tmpcopy = **(zval **)pDest;
	zval_copy_ctor(&tmpcopy);
	INIT_PZVAL(&tmpcopy);
	convert_to_string(&tmpcopy);

	if (hash_key->nKeyLength) {
		PHPWRITE(hash_key->arKey, hash_key->nKeyLength);
	} else {
		php_printf("%d", hash_key->h);
	}
	php_printf(" => ");
	PHPWRITE(Z_STRVAL(tmpcopy), Z_STRLEN(tmpcopy));
	php_printf("\n");
	zval_dtor(&tmpcopy);
	return ZEND_HASH_APPLY_KEEP;
}

int php_sample_print_zvalpp(void *pDest TSRMLS_DC) {

	zval tmpcopy = **(zval **)pDest;
	zval_copy_ctor(&tmpcopy);
	INIT_PZVAL(&tmpcopy);

	convert_to_string(&tmpcopy);
	PHPWRITE(Z_STRVAL(tmpcopy), Z_STRLEN(tmpcopy));
	php_printf(" ");
	zval_dtor(&tmpcopy);
	return ZEND_HASH_APPLY_KEEP;
}

void php_sample_print_hash_table_desc(HashTable *htable TSRMLS_DC) {

	HashPosition pos;

	int num_elements;
	for (
			zend_hash_internal_pointer_end_ex(htable, &pos),
			num_elements = zend_hash_num_elements(htable);
			//zend_hash_has_more_elements_ex(htable, &pos) == SUCCESS;
			num_elements > 0;
			num_elements--,
			zend_hash_move_backwards_ex(htable, &pos)
		) {

		char *key;
		uint keyLen;
		ulong index;
		zval **zval_pp, tmpcopy;

		int type = zend_hash_get_current_key_ex(htable, &key, &keyLen, &index, 0, &pos);
		if (zend_hash_get_current_data_ex(htable, (void **)&zval_pp, &pos) == FAILURE) {
			continue;
		}

		tmpcopy = **zval_pp;
		zval_copy_ctor(&tmpcopy);
		INIT_PZVAL(&tmpcopy);

		convert_to_string(&tmpcopy);

		php_printf("The value of ");
		if (type == HASH_KEY_IS_STRING) {
			PHPWRITE(key, keyLen);
		} else {
			php_printf("%ld", index);
		}
		php_printf(" is: ");
		PHPWRITE(Z_STRVAL(tmpcopy), Z_STRLEN(tmpcopy));
		php_printf("\n");

		zval_dtor(&tmpcopy);
	}
}

void php_sample_print_hash_table(HashTable *htable TSRMLS_DC) {

	HashPosition pos;

	for (
			zend_hash_internal_pointer_reset_ex(htable, &pos);
			zend_hash_has_more_elements_ex(htable, &pos) == SUCCESS;
			zend_hash_move_forward_ex(htable, &pos)
		) {

		char *key;
		uint keyLen;
		ulong index;
		zval **zval_pp, tmpcopy;

		int type = zend_hash_get_current_key_ex(htable, &key, &keyLen, &index, 0, &pos);
		if (zend_hash_get_current_data_ex(htable, (void **)&zval_pp, &pos) == FAILURE) {
			continue;
		}

		tmpcopy = **zval_pp;
		zval_copy_ctor(&tmpcopy);
		INIT_PZVAL(&tmpcopy);

		convert_to_string(&tmpcopy);

		php_printf("The value of ");
		if (type == HASH_KEY_IS_STRING) {
			PHPWRITE(key, keyLen);
		} else {
			php_printf("%ld", index);
		}
		php_printf(" is: ");
		PHPWRITE(Z_STRVAL(tmpcopy), Z_STRLEN(tmpcopy));
		php_printf("\n");

		zval_dtor(&tmpcopy);
	}
}

static zend_function_entry blacknc_function_entry[] = {
	ZEND_FE(blacknc_init, NULL)
	ZEND_FE(blacknc_call, NULL)
	ZEND_FE(blacknc_return_null, NULL)
	ZEND_FE(blacknc_return_bool, NULL)
	ZEND_FE(blacknc_return_false, NULL)
	ZEND_FE(blacknc_return_long, NULL)
	ZEND_FE(blacknc_return_double, NULL)
	ZEND_FE(blacknc_return_string, NULL)
	ZEND_FE(blacknc_return_stringl, NULL)
	ZEND_FE(blacknc_isset, NULL)
	ZEND_FE(blacknc_array, NULL)
	ZEND_FE(blacknc_test_separate, NULL)
#if (PHP_MAJOR_VERSION > 5) || (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION > 0)
	ZEND_FE(blacknc_return_by_ref, blacknc_return_by_ref_arginfo)
#endif
	ZEND_FE(blacknc_get_args, NULL)
	ZEND_FE(blacknc_get_parameters_array_ex, NULL)
	ZEND_FE(blacknc_ht, NULL)
	ZEND_FE(blacknc_loaded_functions, NULL)
	{NULL, NULL, NULL}
};

zend_module_entry blacknc_module_entry = {
	STANDARD_MODULE_HEADER,
	"blacknc",
	blacknc_function_entry, //Function entry
	NULL, //Minit
	NULL, //Mshutdown
	NULL, //Rinit
	NULL, //Rshutdown
	NULL, //Minfo
	"0.0.1", //Version
	STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_BLACKNC
ZEND_GET_MODULE(blacknc)
#endif
