/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2014 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author:                                                              |
  +----------------------------------------------------------------------+
*/

/* $Id$ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

//#include "common.h"

#include "php.h"
//#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_crashit.h"


/* True global resources - no need for thread safety here */
static int le_crashit;


#define LOG_DISABLED		0
#define LOG_WARNING			1
#define LOG_VERBOSE			2


/* {{{ PHP_INI
 */
/* Remove comments and fill if you need to have entries in php.ini
PHP_INI_BEGIN()
	PHP_INI_ENTRY("crashit.logLevel",				"2", PHP_INI_ALL, NULL)	
	PHP_INI_ENTRY("crashit.dumpType",				"0", PHP_INI_ALL, NULL)
	PHP_INI_ENTRY("crashit.dumpFolder",				"c:\\temp", PHP_INI_ALL, NULL)
	PHP_INI_ENTRY("crashit.usingExternalDebugger",	"0", PHP_INI_ALL, NULL)
	PHP_INI_ENTRY("crashit.externalDebugger",		"", PHP_INI_ALL, NULL)
PHP_INI_END()
*/


//
//Debugging Utils
//Copied from WinCache module code
//

void dprintmessage(char * format, va_list args)
{
	char debug_message[255];

	sprintf_s(debug_message, 255, "crashit : ");
	vsprintf_s(debug_message + 10, 245, format, args);

	OutputDebugStringA(debug_message);
	if (IsDebuggerPresent())
	{
		OutputDebugStringA("\n");
	}

	return;
}

void dprintverbose(char * format, ...)
{
	va_list args;
	va_start(args, format);
	dprintmessage(format, args);
	va_end(args);
}

void dprintwarning(char * format, ...)
{
    va_list args;
	va_start(args, format);
	dprintmessage(format, args);
	va_end(args);
}



/* {{{ php_crashit_init_globals
 */
/* Uncomment this function if you have INI entries
static void php_crashit_init_globals(zend_crashit_globals *crashit_globals)
{
	dprintverbose("%s\n", "Entering php_crashit_init_globals");
	...
	dprintverbose("%s\n", "Leaving php_crashit_init_globals");
}
*/
/* }}} */


/* {{{ PHP_MINIT_FUNCTION
Module Initialization, actually nothing to do
 */
PHP_MINIT_FUNCTION(crashit)
{
	php_error_docref(NULL TSRMLS_CC, E_NOTICE, "PHP_MINIT_FUNCTION(crashit)");

	return SUCCESS;
}
/* }}} */


/* {{{ PHP_MSHUTDOWN_FUNCTION
Module shutdown -- actually, nothing to do
 */
PHP_MSHUTDOWN_FUNCTION(crashit)
{
	/* uncomment this line if you have INI entries 	*/
	//UNREGISTER_INI_ENTRIES();


	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request start */
/* {{{ PHP_RINIT_FUNCTION
 
PHP_RINIT_FUNCTION(crashit)
{
	php_error_docref(NULL TSRMLS_C, E_NOTICE, "%s", "PHP_RINIT_FUNCTION(crashit)");

	return SUCCESS;
}
 }}} */



/* Remove if there's nothing to do at request end */
/* {{{ PHP_RSHUTDOWN_FUNCTION
 
PHP_RSHUTDOWN_FUNCTION(crashit)
{
	return SUCCESS;
}
}}} */


/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(crashit)
{

	php_info_print_table_start();
	php_info_print_table_header(2, "Crashit", "Enabled");

	php_info_print_table_end();

	//dprintverbose("%s", "PHP_MINFO_FUNCTION(crashit)");

	/* Remove comments if you have entries in php.ini */
	//DISPLAY_INI_ENTRIES();
}
/* }}} */


/* 
Generate a 0xC0000005 exception which leads to process crash
*/
void crashit()
{
	int i = 0;
	int *iPtr = NULL;
	
	dprintverbose("%s", "crash it called");

	*iPtr = 1;
}

/*
Generate a stack over flow exception which leads to process crash
*/
void StackOverFlow()
{
	int violate[100000];
	int i = 0;
	//int *iPtr;
	
	
	dprintverbose("%s", "Create an StackOverFlow");
	
	for (; i < 10000; i++)
	{
		violate[i] = i;
		StackOverFlow();
	}

	//*iPtr = 1;
	dprintverbose("Leaving StackOverFlow");
}




/* {{{ crashit_functions[]
 *
 * Every user visible function must have an entry in crashit_functions[].
 */
const zend_function_entry crashit_functions[] = {
	//PHP_FE(confirm_crashit_compiled,	NULL)		/* For testing, remove later. */
	PHP_FE(av,	NULL)
	PHP_FE(sof,	NULL)
	PHP_FE(endapis,	NULL)
	PHP_FE_END	/* Must be the last line in crashit_functions[] */
};
/* }}} */

PHP_FUNCTION(av)
 {
     	php_error_docref(NULL TSRMLS_CC, E_NOTICE, "%s", "Generating a c0000005 exception now");
		crashit();
 }

PHP_FUNCTION(endapis)
 {
	 long type;

     if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &type) == FAILURE) {
         RETURN_NULL();
     }

	 php_error_docref(NULL TSRMLS_CC, E_NOTICE, "option is %d", type);
	 php_error_docref(NULL TSRMLS_CC, E_NOTICE, "%s", "Calling APIs to terminate the process now");
	 switch (type)
	 {
	 case 1:
		 abort();
		 break;
	 case 2:
		 exit(2);
		 break;
	 case 3:
		 _exit(3);
		 break;
	 case 4:
		 ExitProcess(4);
		 break;
	 default:
		 TerminateProcess(GetCurrentProcess(), 0);
		 break;
	 }
	 //TerminateProcess(); //Windows API TerminateProcess
	 //ExitProcess(0); //Windows API ExitProcess
	 //abort();  //c runtime API abort
	 //exit(0); //c runtime API exit
	 //_exit(0); //c runtime API _exit

	 RETURN_NULL();
 }

PHP_FUNCTION(sof)
 {
	 php_error_docref(NULL TSRMLS_CC, E_NOTICE, "%s", "Generating a stack overflow exception now");
	 StackOverFlow();
	 RETURN_NULL();
 }

/* {{{ crashit_module_entry
 */
zend_module_entry crashit_module_entry = {
	STANDARD_MODULE_HEADER,
	"crashit",
	crashit_functions,
	PHP_MINIT(crashit),
	PHP_MSHUTDOWN(crashit),
	NULL,
	NULL,
	//PHP_RINIT(crashit),		/* Replace with NULL if there's nothing to do at request start */
	//PHP_RSHUTDOWN(crashit),	/* Replace with NULL if there's nothing to do at request end */
	PHP_MINFO(crashit),
	PHP_CRASHIT_VERSION,
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_CRASHIT
ZEND_GET_MODULE(crashit)
#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */

