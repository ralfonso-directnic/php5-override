/*
  +---------------------------------------------------------------------+
  | PHP Version 5 - Copyright (c) 1997-2007 The PHP Group		   |
  +---------------------------------------------------------------------+
  | Author: Simon Uyttendaele					   |
  +---------------------------------------------------------------------+
*/

/* $Id: header,v 1.16.2.1.2.1 2007/01/01 19:32:09 iliaa Exp $ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "SAPI.h"
#include "php_override.h"
#include <regex.h>

ZEND_DECLARE_MODULE_GLOBALS(override)
static int le_override;

zend_function_entry override_functions[] = {
	{NULL, NULL, NULL}
};

zend_module_entry override_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
	STANDARD_MODULE_HEADER,
#endif
	"override",
	override_functions,
	PHP_MINIT(override),
	PHP_MSHUTDOWN(override),
	PHP_RINIT(override),
	PHP_RSHUTDOWN(override),
	PHP_MINFO(override),
#if ZEND_MODULE_API_NO >= 20010901
	"1.0", /* module version number */
#endif
	STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_OVERRIDE
ZEND_GET_MODULE(override)
#endif

// ======================== PHP.ini ========================
PHP_INI_BEGIN()
	STD_PHP_INI_ENTRY("override.enabled", "1", PHP_INI_SYSTEM, OnUpdateLong, global_enabled, zend_override_globals, override_globals)
	STD_PHP_INI_ENTRY("override.config", "", PHP_INI_SYSTEM, OnUpdateString, global_config, zend_override_globals, override_globals)
PHP_INI_END()

static void php_override_init_globals(zend_override_globals *override_globals)
{
	override_globals->global_enabled = 1;
	override_globals->global_config = NULL;
}

// ======================== CUSTOM CONSTANTS ========================
typedef char* string;
#define false 0
#define true 1
int global_overridden = 0;
void load_config(char *filename);
HashTable _replaced;
void _replaced_destruct(void *pElement)
{
	efree(pElement);
}
static int processed = 0;

// ======================== MODULE START ========================
PHP_MINIT_FUNCTION(override)
{
	REGISTER_INI_ENTRIES();
	return SUCCESS;
}

// ======================== MODULE END ========================
PHP_MSHUTDOWN_FUNCTION(override)
{
	if( processed == 1 )
	{
		zend_hash_destroy(&_replaced);
		processed = 0;
	}

	UNREGISTER_INI_ENTRIES();
	return SUCCESS;
}

// ======================== REQUEST START ========================
PHP_RINIT_FUNCTION(override)
{
	if( processed == 1 )
		return SUCCESS;

	if( override_globals.global_enabled > 0 && override_globals.global_config != NULL && strlen(override_globals.global_config) > 0 )
	{
		zend_hash_init(&_replaced, 200, NULL, _replaced_destruct, 0);
		srand(time(NULL)%5000);
		load_config(override_globals.global_config);
		processed = 1;
	}
	return SUCCESS;
}

// ======================== REQUEST END ========================
PHP_RSHUTDOWN_FUNCTION(override)
{
	return SUCCESS;
}

// ======================== PHP INFO ========================
PHP_MINFO_FUNCTION(override)
{
	php_info_print_table_start();
	php_info_print_table_header(1, "The override PHP module has been developped explicitely for the Olympe Network. It allows to remove, rename or override any PHP core function.");
	php_info_print_table_end();

	php_info_print_table_start();
	php_info_print_table_header(2, "Override module", "Loaded");
	php_info_print_table_row(2, "Developper", "Simon Uyttendaele");
	php_info_print_table_end();

	DISPLAY_INI_ENTRIES();
}

// ***********************************************************
// ======================== REAL CODE ========================
// ***********************************************************

// ======================== UTILS ========================
void skipBOM(FILE *f)
{
	if( ftell(f) == 0 )
	{
		if( fgetc(f) != 0xef || fgetc(f) != 0xbb || fgetc(f) != 0xbf )
			fseek(f, 0, SEEK_SET);
	}
}

int strpos(string str, string search)
{
	string index = strstr(str, search);

	if( index == NULL )
		return -1;
	else
		return (index - str);
}

string str_replace(string str, string search, string replace)
{
	int index;
	int length = strlen(str) + (strlen(replace) - strlen(search)) + 1;
	string result;

	index = strpos(str, search);
	if( index < 0 )
		return str;

	result = emalloc(length);
	strncpy(result, str, index);

	result[index] = '\0';
	sprintf(result+index, "%s%s", replace, str+index+strlen(search));

	efree(str);

	return result;
}

string str_replace_all(string str, string search, string replace)
{
	string result = str;

	do
	{
		str = result;
		result = str_replace(str, search, replace);
	} while( str != result );

	return result;
}

// ======================== SPECIFIC ========================
string generate_random_function_name()
{
	string chars = "azertyuiopqsdfghjklmwxcvbn0987654321";
	string name = emalloc(34);
	int i = 0;

	sprintf(name, "__override_00000000000000000000__");

	do
	{
		for( i = 11; i < 30; i++ )
			name[i] = chars[rand()%strlen(chars)];
	}
	while( zend_hash_exists(&_replaced, name, strlen(name)) );

	return name;
}

regex_t* regex_prepare(string pattern)
{
	regex_t *r = emalloc(sizeof (*r));

	if( regcomp(r, pattern, REG_EXTENDED) == 0 )
		return r;
	else
	{
		regfree(r);
		return NULL;
	}
}

string regex_match(regex_t *regex, string subject)
{
	int result = 0;
	regmatch_t pmatch[2];
	string match;
	int start;
	int end;
	int size;

	if( regex == NULL || subject == NULL || regex->re_nsub == 0 )
		return NULL;

	result = regexec(regex, subject, 2, pmatch, 0);

	if( result == 0 )
	{
		start = pmatch[1].rm_so;
		end = pmatch[1].rm_eo;
		if( start < 0 || end < 0 || start >= end )
			return NULL;
		size = end - start;
		match = emalloc(size + 1);
		if( match != NULL )
		{
			strncpy(match, &subject[start], size);
			match[size] = '\0';
		}
		return match;
	}
	else
		return NULL;
}

string substitute_random_function_names(string code)
{
	regex_t *regex;
	string match;
	string replacement;

	regex = regex_prepare("(#[a-z_A-Z0-9:]*)\\s*\\(");
	if( regex == NULL || code == NULL )
		return code;

	while( (match = regex_match(regex, code)) != NULL )
	{
		if( zend_hash_find(&_replaced, match+1, strlen(match+1), (void**) &replacement) == SUCCESS )
			code = str_replace_all(code, match, replacement);
		efree(match);
	}

	regfree(regex);
	return code;
}

int zend_hash_rename(HashTable *ht, string original, string replacement)
{
	string key;

	if( ht == NULL || original == NULL || replacement == NULL )
		return FAILURE;

	zend_hash_internal_pointer_reset(ht);
	while( zend_hash_has_more_elements(ht) == SUCCESS )
	{
		zend_hash_get_current_key(ht, &key, NULL, 0);
		if( strcmp(key, original) == 0 )
			return zend_hash_update_current_key(ht, HASH_KEY_IS_STRING, replacement, strlen(replacement) + 1, 0);
		zend_hash_move_forward(ht);
	}

	// original key not found
	return FAILURE;
}

int zend_update_constructor(string class, zend_function *function)
{
	zend_class_entry **ce;

	if( class != NULL )
	{
		if(zend_hash_find(EG(class_table), class, strlen(class) + 1, (void **) &ce) == FAILURE)
			return FAILURE; // the class does not exist
		function->common.fn_flags |= ZEND_ACC_CTOR;
		function->common.fn_flags &= ~ZEND_ACC_ALLOW_STATIC;
		function->op_array.fn_flags |= ZEND_ACC_PUBLIC;
		function->common.function_name = "__construct";
		(*ce)->constructor = function;
		return SUCCESS;
	}
	else
		return FAILURE;
}

void parse_function_name(string function_name, string *class, string *method)
{
	regex_t* regex;
	
	// try class with "function "
	regex = regex_prepare("^\\s*function\\s+([_a-zA-Z0-9]+)::[_a-zA-Z0-9]+\\s*(\\(|$)");
	if( regex == NULL ) return;
	*class = regex_match(regex, function_name);
	regfree(regex);
	
	// try class without "function "
	if( *class == NULL )
	{
		regex = regex_prepare("^\\s*([_a-zA-Z0-9]+)::[_a-zA-Z0-9]+\\s*(\\(|$)");
		if( regex == NULL ) return;
		*class = regex_match(regex, function_name);
		regfree(regex);
	}

	// try method with "function " and with class
	regex = regex_prepare("^\\s*function\\s+[_a-zA-Z0-9]+::([_a-zA-Z0-9]+)\\s*(\\(|$)");
	if( regex == NULL ) return;
	*method = regex_match(regex, function_name);
	regfree(regex);

	// try method without "function " and with class
	if( *method == NULL )
	{
		regex = regex_prepare("^\\s*[_a-zA-Z0-9]+::([_a-zA-Z0-9]+)\\s*(\\(|$)");
		if( regex == NULL ) return;
		*method = regex_match(regex, function_name);
		regfree(regex);
	}
	
	// try method with "function " and without class
	if( *method == NULL )
	{
		regex = regex_prepare("^\\s*function\\s+([_a-zA-Z0-9]+)\\s*(\\(|$)");
		if( regex == NULL ) return;
		*method = regex_match(regex, function_name);
		regfree(regex);
	}

	// try method without "function " and without class
	if( *method == NULL )
	{
		regex = regex_prepare("^\\s*([_a-zA-Z0-9]+)\\s*(\\(|$)");
		if( regex == NULL ) return;
		*method = regex_match(regex, function_name);
		regfree(regex);
	}
}

HashTable* get_function_table(string class)
{
	zend_class_entry **class_code;

	if( class != NULL )
	{
		if(zend_hash_find(EG(class_table), class, strlen(class) + 1, (void **) &class_code) == FAILURE)
			return NULL; // the class does not exist
		else
			return &(*class_code)->function_table;
	}
	else
		return EG(function_table);
}

// ======================== OVERRIDE ========================
int method_exists(string class, string method)
{
	HashTable *method_table = get_function_table(class);

	if( method_table == NULL || method == NULL )
		return false;

	if(zend_hash_exists(method_table, method, strlen(method) + 1))
		return true; // the method exists
	else
	{
		// special case for old style constructor
		if( class != NULL && strcmp(method, "__construct") == 0 && zend_hash_exists(method_table, class, strlen(class) + 1) )
			return true;
		return false; // the method does not exist
	}
}

int override_delete(string class, string method)
{
	HashTable *method_table = get_function_table(class);

	if( method_table == NULL || method == NULL || method_exists(class, method) == false )
		return false; // the function does not exist

	if(zend_hash_del(method_table, method, strlen(method) + 1) == FAILURE)
	{
		// special case for old style constructor
		if( class != NULL && strcmp(method, "__construct") == 0 )
			return (zend_hash_del(method_table, class, strlen(class) + 1) == SUCCESS);
		return false; // failed to remove the function
	}

	return true; // function removed successfully
}

int override_add(string class, string method, zend_function *method_code)
{
	HashTable *method_table = get_function_table(class);

	if( method_table == NULL || method == NULL || method_code == NULL || method_exists(class, method) == true )
		return false;
	if(zend_hash_add(method_table, method, strlen(method) + 1, method_code, sizeof(zend_function), NULL) == FAILURE)
		return false; // failed to insert function
	else
	{
		// special case for old style constructor
		if( class != NULL && strcmp(method, "__construct") == 0 )
		{
			if( zend_hash_find(method_table, "__construct", sizeof("__construct"), (void **) &method_code) == FAILURE )
				return false;
			zend_update_constructor(class, method_code);
		}
		return true; // function added successfully
	}
}

int override_rename(string class, string method_old, string method_new)
{
	HashTable *method_table = get_function_table(class);

	if( method_table == NULL || method_old == NULL || method_new == NULL )
		return false;

	if( zend_hash_rename(method_table, method_old, method_new) == SUCCESS )
		return true;
	else
	{
		// special case for old style constructor
		if( class != NULL && strcmp(method_old, "__construct") == 0 )
			return (zend_hash_rename(method_table, class, method_new) == SUCCESS);
		return false;
	}
}

int override_eval(string code)
{
	string class = NULL;
	string method = NULL;
	string tmp_code = NULL;
	int index = 0;
	zend_function *zf;
	
	// caution : if we want to create a class method, we must attach it to the proper class function table
	parse_function_name(code, &class, &method);
	
	if( class == NULL )
	{
		if( method != NULL ) efree(method);
		return (zend_eval_string(code, NULL, "code overridden" TSRMLS_CC) == SUCCESS);
	}
	else
	{
		if( method == NULL ) { efree(class); return false; }
		
		index = strpos(code, "(");
		if( index < 0 ) { efree(method); efree(class); return false; }
		
		code = code + index; // strip the blabla before arguments
		tmp_code = emalloc(strlen("function __override_tmp__") + strlen(code) + 1);
		sprintf(tmp_code, "%s%s", "function __override_tmp__", code);
		
		// eval the function with tmp name
		if( zend_eval_string(tmp_code, NULL, "method overridden" TSRMLS_CC) == FAILURE ) { efree(method); efree(class); efree(tmp_code); return false; }
		
		// get the pointer of the function into zf
		if( zend_hash_find(EG(function_table), "__override_tmp__", sizeof("__override_tmp__"), (void **) &zf) == FAILURE ) { efree(method); efree(class); efree(tmp_code); return false; }
		
		// add reference so that code is not deleted at override_delete
		function_add_ref(zf);
		
		// add to the class method table
		override_add(class, method, zf);
		
		// remove the tmp name
		override_delete(NULL, "__override_tmp__");
		
		efree(method);
		efree(class);
		efree(tmp_code);
		return true;
	}
}

// ======================== LOAD CONFIG ========================
void load_config(char *filename)
{
	FILE *f;
	string line = NULL;
	string class = NULL;
	string method = NULL;
	string rename_method = NULL;
	string replaced_key = NULL;
	const char RENAME_FLAG = '#';
	const char APPEND_FLAG = '+';
	const char DELETE_FLAG = '-';
	const char COMMENT_FLAG = ';';

	f = fopen(filename, "r");
	if( f == NULL )
		return;
	skipBOM(f);

	do
	{
		line = emalloc(5000);
		if( fgets(line, 5000, f) == NULL )
		{
			efree(line);
			break;
		}

		// trim end line
		while( isspace(line[strlen(line)-1]) && strlen(line) > 0 )
			line[strlen(line)-1] = '\0';

		// empty line
		if( strlen(line) <= 0 )
		{
			efree(line);

			if( feof(f) )
				break; // end of file so exit
			else
				continue; // not end of file so get next line
		}

		if( line[0] == RENAME_FLAG )
		{
			parse_function_name(line+1, &class, &method);
			rename_method = generate_random_function_name();
			if( override_rename(class, method, rename_method) )
			{
				if( class != NULL ) { replaced_key = emalloc(strlen(class) + strlen(method) + 3); sprintf(replaced_key, "%s::%s", class, method); }
				else { replaced_key = emalloc(strlen(method) + 1); sprintf(replaced_key, "%s", method); }
				zend_hash_add(&_replaced, replaced_key, strlen(replaced_key), rename_method, strlen(rename_method) + 1, NULL);
				efree(replaced_key);
			}
		}
		else if( line[0] == DELETE_FLAG )
		{
			parse_function_name(line+1, &class, &method);
			override_delete(class, method);
		}
		else if( line[0] == APPEND_FLAG )
		{
			line = substitute_random_function_names(line);
			if( line != NULL )
				override_eval(line+1);
		}
		
		if( line != NULL ) { efree(line); line = NULL; }
		if( class != NULL ) { efree(class); class = NULL; }
		if( method != NULL ) { efree(method); method = NULL; }

	}while( !feof(f) );

	fclose(f);
}
