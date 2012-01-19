/*
  +---------------------------------------------------------------------+
  | PHP Version 5 - Copyright (c) 1997-2007 The PHP Group               |
  +---------------------------------------------------------------------+
  | Author: Simon Uyttendaele                                           |
  +---------------------------------------------------------------------+
*/

/* $Id: header,v 1.16.2.1.2.1 2007/01/01 19:32:09 iliaa Exp $ */

#ifndef PHP_OVERRIDE_H
#define PHP_OVERRIDE_H

extern zend_module_entry override_module_entry;
#define phpext_override_ptr &override_module_entry

#ifdef PHP_WIN32
#define PHP_OVERRIDE_API __declspec(dllexport)
#else
#define PHP_OVERRIDE_API
#endif

#ifdef ZTS
#include "TSRM.h"
#endif

// PHP MODULE START/STOP functions
PHP_MINIT_FUNCTION(override);
PHP_MSHUTDOWN_FUNCTION(override);
// PHP REQUEST START/STOP functions
PHP_RINIT_FUNCTION(override);
PHP_RSHUTDOWN_FUNCTION(override);
// PHP INFO
PHP_MINFO_FUNCTION(override);

// CUSTOM PHP functions
//PHP_FUNCTION(my_function_name);

// PHP.ini variables
ZEND_BEGIN_MODULE_GLOBALS(override)
	long  global_enabled;
	char *global_config;
ZEND_END_MODULE_GLOBALS(override)

#endif	/* PHP_OVERRIDE_H */

