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
  | Author: cts-wzhao@live.com
   https://github.com/kexuelou/                                           |
  +----------------------------------------------------------------------+
*/

/* $Id$ */

#ifndef PHP_CATCHIT_H
#define PHP_CATCHIT_H


extern zend_module_entry catchit_module_entry;
#define phpext_catchit_ptr &catchit_module_entry

#define PHP_CATCHIT_VERSION "1.0" /* Replace with version number for your extension */

#ifdef PHP_WIN32
#	define PHP_CATCHIT_API __declspec(dllexport)
#elif defined(__GNUC__) && __GNUC__ >= 4
#	define PHP_CATCHIT_API __attribute__ ((visibility("default")))
#else
#	define PHP_CATCHIT_API
#endif

#ifdef ZTS
#include "TSRM.h"
#endif

/* 
  	Declare any global variables you may need between the BEGIN
	and END macros here:     

ZEND_BEGIN_MODULE_GLOBALS(catchit)
	//enable this feature or disable, default is 0
	//long enable;
	
	//memory dump type, default is 0x0;
    //https://msdn.microsoft.com/en-us/library/windows/desktop/ms680519%28v=vs.85%29.aspx

	long dumpType;
	//default is %temp%
	TCHAR *dumpFolder;
	zend_bool usingExternalDebugger;
	TCHAR *externalDebugger;
ZEND_END_MODULE_GLOBALS(catchit)
*/
/*
	typedef enum _MINIDUMP_TYPE { 
		MiniDumpNormal                          = 0x00000000,
		MiniDumpWithDataSegs                    = 0x00000001,
		MiniDumpWithFullMemory                  = 0x00000002,
		MiniDumpWithHandleData                  = 0x00000004,
		MiniDumpFilterMemory                    = 0x00000008,
		MiniDumpScanMemory                      = 0x00000010,
		MiniDumpWithUnloadedModules             = 0x00000020,
		MiniDumpWithIndirectlyReferencedMemory  = 0x00000040,
		MiniDumpFilterModulePaths               = 0x00000080,
		MiniDumpWithProcessThreadData           = 0x00000100,
		MiniDumpWithPrivateReadWriteMemory      = 0x00000200,
		MiniDumpWithoutOptionalData             = 0x00000400,
		MiniDumpWithFullMemoryInfo              = 0x00000800,
		MiniDumpWithThreadInfo                  = 0x00001000,
		MiniDumpWithCodeSegs                    = 0x00002000,
		MiniDumpWithoutAuxiliaryState           = 0x00004000,
		MiniDumpWithFullAuxiliaryState          = 0x00008000,
		MiniDumpWithPrivateWriteCopyMemory      = 0x00010000,
		MiniDumpIgnoreInaccessibleMemory        = 0x00020000,
		MiniDumpWithTokenInformation            = 0x00040000,
		MiniDumpWithModuleHeaders               = 0x00080000,
		MiniDumpFilterTriage                    = 0x00100000,
		MiniDumpValidTypeFlags                  = 0x001fffff
	} MINIDUMP_TYPE;
*/

/* In every utility function you add that needs to use variables 
   in php_catchit_globals, call TSRMLS_FETCH(); after declaring other 
   variables used by that function, or better yet, pass in TSRMLS_CC
   after the last function argument and declare your utility function
   with TSRMLS_DC after the last declared argument.  Always refer to
   the globals in your function as CATCHIT_G(variable).  You are 
   encouraged to rename these macros something shorter, see
   examples in any other php module directory.
*/

#ifdef ZTS
#define CATCHIT_G(v) TSRMG(catchit_globals_id, zend_catchit_globals *, v)
#else
#define CATCHIT_G(v) (catchit_globals.v)
#endif

#endif	/* PHP_CATCHIT_H */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */


 #define DUMP_TYPE_MAP(XX)																\
  XX(MiniDumpNormal, MiniDumpNormal)													\
  XX(MiniDumpWithDataSegs, MiniDumpWithDataSegs)										\
  XX(MiniDumpWithFullMemory, MiniDumpWithFullMemory)                                    \
  XX(MiniDumpWithHandleData, MiniDumpWithHandleData)                                    \
  XX(MiniDumpFilterMemory, MiniDumpFilterMemory)                                        \
  XX(MiniDumpScanMemory, MiniDumpScanMemory)                                            \
  XX(MiniDumpWithUnloadedModules, MiniDumpWithUnloadedModules)                          \
  XX(MiniDumpWithIndirectlyReferencedMemory, MiniDumpWithIndirectlyReferencedMemory)    \
  XX(MiniDumpFilterModulePaths, MiniDumpFilterModulePaths)                              \
  XX(MiniDumpWithProcessThreadData, MiniDumpWithProcessThreadData)                      \
  XX(MiniDumpWithPrivateReadWriteMemory, MiniDumpWithPrivateReadWriteMemory)            \
  XX(MiniDumpWithoutOptionalData, MiniDumpWithoutOptionalData)                          \
  XX(MiniDumpWithFullMemoryInfo, MiniDumpWithFullMemoryInfo)                            \
  XX(MiniDumpWithThreadInfo, MiniDumpWithThreadInfo)                                    \
  XX(MiniDumpWithCodeSegs, MiniDumpWithCodeSegs)                                        \
  XX(MiniDumpWithoutAuxiliaryState, MiniDumpWithoutAuxiliaryState)                      \
  XX(MiniDumpWithFullAuxiliaryState, MiniDumpWithFullAuxiliaryState)                    \
  XX(MiniDumpWithPrivateWriteCopyMemory, MiniDumpWithPrivateWriteCopyMemory)            \
  XX(MiniDumpIgnoreInaccessibleMemory, MiniDumpIgnoreInaccessibleMemory)                \
  XX(MiniDumpWithTokenInformation, MiniDumpWithTokenInformation)                        \
  XX(MiniDumpWithModuleHeaders, MiniDumpWithModuleHeaders)                              \
  XX(MiniDumpFilterTriage, MiniDumpFilterTriage)                                        \
  XX(MiniDumpValidTypeFlags, MiniDumpValidTypeFlags)                                    \




  #define LOG_LEVEL_MAP(XX)																\
  XX(LOG_DISABLED, DISABLED)									\
  XX(LOG_WARNING, WARNING)										\
  XX(LOG_VERBOSE, VERBOSE)                                    \
  

 