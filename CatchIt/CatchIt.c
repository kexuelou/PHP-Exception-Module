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
            https://github.com/kexuelou/
  +----------------------------------------------------------------------+
*/

/* $Id$ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "common.h"

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_catchit.h"
#include "windows.h"
#include "dbghelp.h"
#include "tchar.h"
#include "detours.h"
//#include "eh.h"
//#include <signal.h>
//#include "logging.h"
/* If you declare any globals in php_catchit.h uncomment this:*/
//ZEND_DECLARE_MODULE_GLOBALS(catchit)


/* True global resources - no need for thread safety here */
static int le_catchit;
static LPTOP_LEVEL_EXCEPTION_FILTER Orig_UnhandledExceptionFilter;


/*Log Level, currently not used*/
#define LOG_DISABLED		0
#define LOG_WARNING			1
#define LOG_VERBOSE			2


/* {{{ PHP_INI
 */
/* Remove comments and fill if you need to have entries in php.ini*/
PHP_INI_BEGIN()
	PHP_INI_ENTRY("catchit.logLevel",				"2", PHP_INI_ALL, NULL)	
	PHP_INI_ENTRY("catchit.dumpType",				"0", PHP_INI_ALL, NULL)
	PHP_INI_ENTRY("catchit.dumpFolder",				"c:\\temp", PHP_INI_ALL, NULL)
	PHP_INI_ENTRY("catchit.usingExternalDebugger",	"0", PHP_INI_ALL, NULL)
	PHP_INI_ENTRY("catchit.externalDebugger",		"", PHP_INI_ALL, NULL)
PHP_INI_END()


/*
Debugging message, copied from WinCache
*/
void dprintmessage(char * format, va_list args)
{
	char debug_message[255];

	sprintf_s(debug_message, 255, "PHP_CatchIt : ");
	vsprintf_s(debug_message + 14, 245, format, args);

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
	{
		va_start(args, format);
		dprintmessage(format, args);
		va_end(args);
	}
}

void dprintwarning(char * format, ...)
{
    va_list args;
	{
		va_start(args, format);
		dprintmessage(format, args);
		va_end(args);
	}
}


/*
Log Active Requests -- the current PHP scritp, function and line number.
*/
void LogActiveRequests()
{
	php_error_docref(NULL TSRMLS_CC, E_NOTICE, "active function %s @ %s:%d", 
						get_active_function_name(TSRMLS_C),
						zend_get_executed_filename(TSRMLS_C),
						zend_get_executed_lineno(TSRMLS_C)
						);
}


/*
Generate the dump file name.
File Named in this way:
PHP-CGI-<sitename>-<dump type>-<utc time>-<PID>.dmp
PHP-CGI-wzhaotest-MiniDumpWithFullMemory-2015-3-17-8-26-42-253-UTC-99292

if dumpFolder is not specified, using TEMP folder.
*/

void GenerateDumpFileName(TCHAR* dumpFileName )
{
	//max lengh of site name is 60
	TCHAR siteName[120];
	TCHAR logFolder[20];
	DWORD pid;
	SYSTEMTIME stUTC;
	TCHAR currentTime[100];
	DWORD ret;
	char* typeStr;

	dprintverbose("%s", "Entering GenerateDumpFileName");
	
	//LogFile Folder
	if (NULL == INI_STR("catchit.dumpFolder"))
	{
		dprintverbose("%s", "logFolder is not defined, %TEMP% will be used");
		ret = GetEnvironmentVariable(_T("TEMP"), logFolder, 20);

		if (ret == 0)
			sprintf_s(logFolder, 20, _T("c:\\temp"));
	}
	
	//site name
	ret = GetEnvironmentVariable(_T("WEBSITE_SITE_NAME"), siteName, 120);
	
	if (ret == 0)
		sprintf_s(siteName, 120, "Unknow-Site");

	dprintverbose("siteName = %s", siteName);

		//dumpType
	switch (INI_INT("catchit.dumpType")) {
#define X(uc, lc) case uc: typeStr = #uc; break;
		DUMP_TYPE_MAP(X)
#undef X
default: typeStr = "unknown";
	}

	//Process ID
	pid = GetCurrentProcessId();
	dprintverbose("PID = %d", pid);

	//Current Time
    GetSystemTime(&stUTC);     
    sprintf_s(currentTime, 100, _T("%u-%u-%u-%u-%u-%u-%u-UTC"),
               stUTC.wYear, stUTC.wMonth, stUTC.wDay,  
               stUTC.wHour, stUTC.wMinute, stUTC.wSecond,  
               stUTC.wMilliseconds,stUTC.wDayOfWeek);  

	dprintverbose("currentTime = %s", currentTime);
	
	//Dump File Name
	if (NULL == INI_STR("catchit.dumpFolder"))
	{
		sprintf_s(dumpFileName, 256, "%s\\%s-%s-%s-%s-%d.dmp", logFolder, _T("PHP-CGI"), siteName, typeStr, currentTime, pid);
	}
	else 
	{
		sprintf_s(dumpFileName, 256, "%s\\%s-%s-%s-%s-%d.dmp", INI_STR("catchit.dumpFolder"), _T("PHP-CGI"), siteName, typeStr, currentTime, pid);
	}

	php_error_docref(NULL TSRMLS_CC, E_NOTICE, "memory dump will write to %s", dumpFileName);

	dprintverbose("dumpFileName = %s", dumpFileName);

	dprintverbose("%s", "Leaving GenerateDumpFileName");
	return;
}


/*
using specified debugger to write dump file when exception happened
ProcDump is the tool tested only
*/
LONG WINAPI WriteMiniDumpUsingDebugger(struct _EXCEPTION_POINTERS *ExceptionInfo)
{
	// EXCEPTION_CONTINUE_SEARCH means it continue to 
	// execute subsequent exception handlers.
	LONG rc = EXCEPTION_CONTINUE_SEARCH;

	HMODULE hDll = NULL;
	TCHAR szDumpFile[MAX_PATH];
	TCHAR dumpCommand[MAX_PATH];

	//launch debugger command
	STARTUPINFO si;
    PROCESS_INFORMATION pi;


	dprintverbose("%s", "Entering WriteMiniDumpUsingDebugger");

	GenerateDumpFileName(szDumpFile);
	dprintverbose("dump file name%s", szDumpFile);

	sprintf_s(dumpCommand, MAX_PATH, "%s %d %s", INI_STR("catchit.externalDebugger"), GetCurrentProcessId(), szDumpFile);

	dprintverbose("debugger command : %s", dumpCommand);

	//prepare the parameters 	
    ZeroMemory( &si, sizeof(si) );
    si.cb = sizeof(si);
    ZeroMemory( &pi, sizeof(pi) );
	
	// Start the child process. 
	if(CreateProcess( NULL,   // No module name (use command line)
		dumpCommand,        // Command line
		NULL,           // Process handle not inheritable
		NULL,           // Thread handle not inheritable
		FALSE,          // Set handle inheritance to FALSE
		0,              // No creation flags
		NULL,           // Use parent's environment block
		NULL,           // Use parent's starting directory 
		&si,            // Pointer to STARTUPINFO structure
		&pi )           // Pointer to PROCESS_INFORMATION structure
		) 
	{
		dprintverbose("%s", "Created Debugger Process and wait to finish.");
		php_error_docref(NULL TSRMLS_CC, E_NOTICE, "%s is running, wait to finish...", dumpCommand);
		// Wait until child process exits.
		WaitForSingleObject( pi.hProcess, INFINITE );

		// Close process and thread handles. 
		CloseHandle( pi.hProcess );
		CloseHandle( pi.hThread );
		dprintverbose("%s", "Debugger Process finished.");
	}
	else 
	{
		dprintwarning("%s with 0x%08lx", "Created Debugger Process failed", GetLastError());
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s with 0x%08lx", "Created Debugger Process failed", GetLastError());
	}

	dprintverbose("%s", "Leaving WriteMiniDumpUsingDebugger");
	return rc;
}

/*
This method is to write mini dump of current process using MiniDumpWriteDump when exception happened
*/
LONG WINAPI WriteMiniDump(struct _EXCEPTION_POINTERS *ExceptionInfo)
{
	// EXCEPTION_CONTINUE_SEARCH means it continue to 
	// execute subsequent exception handlers.
	LONG rc = EXCEPTION_CONTINUE_SEARCH;

	HMODULE hDll = NULL;
	TCHAR szDumpFile[MAX_PATH];
	HANDLE hDumpFile;

	dprintverbose("%s", "Entering WriteMiniDump");

	GenerateDumpFileName(szDumpFile);

	dprintverbose("Dump will write to %s", szDumpFile);
	//php_error_docref(NULL TSRMLS_CC, E_NOTICE, "Dump will write to %s\n", szDumpFile);
	php_error_docref(NULL TSRMLS_CC, E_NOTICE, "memory dump will write to %s", szDumpFile);

	hDumpFile = CreateFile(szDumpFile, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, 
		CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	if (hDumpFile != INVALID_HANDLE_VALUE)
	{
		MINIDUMP_EXCEPTION_INFORMATION ExInfo;

		dprintverbose("%s", "Start writing dump file");
		ExInfo.ThreadId = GetCurrentThreadId();
		ExInfo.ExceptionPointers = ExceptionInfo;
		ExInfo.ClientPointers = TRUE;

		// Write the information into the dump
		if (MiniDumpWriteDump(
			GetCurrentProcess(), // Handle of process
			GetCurrentProcessId(), // Process Id
			hDumpFile,    // Handle of dump file
			(MINIDUMP_TYPE)INI_INT("catchit.dumpType"),   // Dump Level: Mini
			&ExInfo,    // Exception information
			NULL,     // User stream parameter
			NULL))     // Callback Parameter
		{
			rc = EXCEPTION_CONTINUE_SEARCH;
		}
		else
		{
			dprintwarning("Failed to write dump file, MiniDumpWriteDump API failed with 0x%08lx", GetLastError());
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Failed to write dump file, MiniDumpWriteDump API failed with 0x%08lx\n", GetLastError());
		}

		CloseHandle(hDumpFile);
		dprintverbose("%s", "Finished writing dump file");
	}
	else
	{
		dprintwarning("Failed to open dump file to write, CreateFile API failed w/err 0x%08lx\n", GetLastError());
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Failed to open dump file to write, CreateFile API failed with 0x%08lx\n", GetLastError());
	}

	dprintverbose("%s", "Leaving WriteMiniDump");
	return rc;
}

/*
using the specifed way to create the dump file.
*/
LONG WINAPI DumpIt(struct _EXCEPTION_POINTERS *lpTopLevelExceptionFilter)
{
	dprintverbose("Writing dump now");

	//using external debugger
	if (INI_BOOL("catchit.usingExternalDebugger"))
	{
		dprintverbose("%s", "write dump using external debugger");
		php_error_docref(NULL TSRMLS_CC, E_NOTICE, "write dump using external debugger");
		WriteMiniDumpUsingDebugger(lpTopLevelExceptionFilter);
	}
	else 
	{
		dprintverbose("%s", "write dump using debugging API");
		php_error_docref(NULL TSRMLS_CC, E_NOTICE, "write dump using debugging API");
		WriteMiniDump(lpTopLevelExceptionFilter);
	}

	//active requests
	LogActiveRequests();

	dprintverbose("%s\n", "Leaving DumpIt");
	return EXCEPTION_CONTINUE_SEARCH;
}

/*
Detour Definitions:
Real_API == the original APIs
CatchIt_API == the detours function
*/
static LPTOP_LEVEL_EXCEPTION_FILTER (WINAPI *Real_SetUnhandledExceptionFilter)( _In_  LPTOP_LEVEL_EXCEPTION_FILTER lpTopLevelExceptionFilter) = SetUnhandledExceptionFilter;

LPTOP_LEVEL_EXCEPTION_FILTER WINAPI CatchIt_SetUnhandledExceptionFilter(LPTOP_LEVEL_EXCEPTION_FILTER lpTopLevelExceptionFilter)
{
    return NULL;
}


static BOOL (WINAPI *Real_TerminateProcess)(HANDLE hProcess, UINT uExitCode) = TerminateProcess;

BOOL WINAPI CatchIt_TerminateProcess(HANDLE hProcess, UINT uExitCode)
{
	dprintwarning("PHP_CatchIt -- Someone is invoking TerminateProcess");
	php_error_docref(NULL TSRMLS_CC, E_NOTICE, "PHP_CatchIt -- Someone is invoking TerminateProcess");
	DumpIt(NULL);
	dprintverbose("%s", "Calling Real_TerminateProcess");
	Real_TerminateProcess(hProcess, uExitCode);
	return TRUE;
}

static void (WINAPI *Real_ExitProcess)(UINT uExitCode) = ExitProcess;

void WINAPI CatchIt_ExitProcess(UINT uExitCode)
{
	dprintwarning("PHP_CatchIt -- Someone is invoking ExitProcess.");
	php_error_docref(NULL TSRMLS_CC, E_WARNING, "PHP_CatchIt -- Someone is invoking ExitProcess");
	DumpIt(NULL);
	dprintverbose("%s", "Calling Real_ExitProcess");
	Real_ExitProcess(uExitCode);
	return;
}
//
//static void (*Real_Terminate)(void) = terminate;
//void CatchIt_Terminate(void)
//{
//	dprintwarning("PHP_CatchIt -- Someone is invoking terminate.");
//	php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", "PHP_CatchIt -- Someone is invoking terminate");
//	DumpIt(NULL);
//	dprintverbose("%s", "Calling Real_Terminate");
//	Real_Terminate();
//	dprintverbose("%s", "Leaving CatchIt_Terminate");
//
//	return;
//}


static void (*Real_Abort)(void) = abort;
void CatchIt_Abort(void)
{
	dprintwarning("PHP_CatchIt -- Someone is invoking abort.");
	php_error_docref(NULL TSRMLS_CC, E_WARNING, "PHP_CatchIt -- Someone is invoking abort");
	DumpIt(NULL);
	dprintverbose("%s", "Calling Real_Abort");
	Real_Abort();
	dprintverbose("%s", "Leaving CatchIt_Abort");
	return;
}

static void (__cdecl *Real_Exit)(_In_ int status) = exit;
void __cdecl  CatchIt_Exit(_In_ int _Code)
{
	php_error_docref(NULL TSRMLS_CC, E_WARNING, "PHP_CatchIt -- Someone is invoking exit");
	dprintwarning("PHP_CatchIt -- Someone is invoking exit.");
	Real_Exit(_Code);
	DumpIt(NULL);
	return;
}

static void (__cdecl *Real__Exit)(_In_ int status) = _exit;
void __cdecl  CatchIt__Exit(_In_ int _Code)
{
	php_error_docref(NULL TSRMLS_CC, E_WARNING, "PHP_CatchIt -- Someone is invoking _exit");
	dprintwarning("PHP_CatchIt -- Someone is invoking _exit.");
	Real__Exit(_Code);
	DumpIt(NULL);
	return;
}

/*
These are copied from the Detour API samples
*/
PCHAR DetRealName(PCHAR psz)
{
	PCHAR pszBeg = psz;
	// Move to end of name.
	while (*psz) {
		psz++;
	}
	// Move back through A-Za-z0-9 names.
	while (psz > pszBeg &&
		((psz[-1] >= 'A' && psz[-1] <= 'Z') ||
		(psz[-1] >= 'a' && psz[-1] <= 'z') ||
		(psz[-1] >= '0' && psz[-1] <= '9'))) {
			psz--;
	}
	return psz;
}

VOID DetAttach(PVOID *ppbReal, PVOID pbMine, PCHAR psz)
{
	LONG l = DetourAttach(ppbReal, pbMine);
	if (l != 0) {
		dprintwarning("Detour Attach failed: `%s': error %d", DetRealName(psz), l);
		
	}
	dprintverbose("Detour DeAttach Success: `%s", DetRealName(psz));
}

VOID DetDetach(PVOID *ppbReal, PVOID pbMine, PCHAR psz)
{
	LONG l = DetourDetach(ppbReal, pbMine);
	if (l != 0) {
		dprintwarning("Detour DeAttach failed: `%s': error %d", DetRealName(psz), l);
	}
}

#define ATTACH(x)       DetAttach((PVOID*)&Real_##x,CatchIt_##x,#x)
#define DETACH(x)       DetDetach(&(PVOID&)Real_##x,CatchIt_##x,#x)

/*
Detour the APIs can result in process termination

*/
LONG DetourAPIs()
{
	long error = 0;

	php_error_docref(NULL TSRMLS_CC, E_NOTICE, "Start Detour APIs");

	//dprintverbose("%s", "Enter DetourAPIs");
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());

	//DetAttach((PVOID*)(&Real_TerminateProcess), CatchIt_TerminateProcess, "TerminateProcess");
	DetAttach((PVOID*)(&Real_ExitProcess), CatchIt_ExitProcess, "ExitProcess");
	DetAttach((PVOID*)(&Real_Abort), CatchIt_Abort, "Abort");
	DetAttach((PVOID*)(&Real_Exit), CatchIt_Exit, "Exit");
	DetAttach((PVOID*)(&Real__Exit), CatchIt__Exit, "_Exit");
	
	//Prevent others to run SetUnhandledExceptionFilter again
	DetAttach((PVOID*)(&Real_SetUnhandledExceptionFilter), CatchIt_SetUnhandledExceptionFilter, "SetUnhandledExceptionFilter"); 
	



	ATTACH(TerminateProcess); //Windows API TerminateProcess
	//ATTACH(ExitProcess); //Windows API ExitProcess
	////ATTACH(Terminate); // //c runtime API terminate
	//ATTACH(Abort);  //c runtime API abort
	//ATTACH(Exit); //c runtime API exit
	//ATTACH(_Exit); //c runtime API _exit

	//ATTACH(SetUnhandledExceptionFilter); //Prevent others to run SetUnhandledExceptionFilter again

	php_error_docref(NULL TSRMLS_CC, E_NOTICE, "Done Detour APIs");

	return DetourTransactionCommit();
}


//
//void SignalHandler(int signal)
//{
//    if (signal == SIGABRT) {
//        // abort signal handler code
//			php_error_docref(NULL TSRMLS_CC, E_WARNING, "SignalHandler called.");
//    } else {
//        // ...
//    }
//}
// 
//typedef void (*SignalHandlerPointer)(int);
// 
//



/*
The Unhandled Exception Filter
This API will be called when an exception was not handled.
-- Write an dump file
-- write the and function to php error log
*/
LONG WINAPI PHPUnhandledExceptionFilter(
    struct _EXCEPTION_POINTERS *lpTopLevelExceptionFilter)
  {
    // TODO: MiniDumpWriteDump
	
	dprintverbose("%s", "Enter PHPUnhandledExceptionFilter");
	//
	php_error_docref(NULL TSRMLS_CC, E_WARNING, "An unhandled excepton happened");
	if (lpTopLevelExceptionFilter)
	{
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "PHP_CatchIt -- Exception Code is : 0x%08lx", lpTopLevelExceptionFilter->ExceptionRecord->ExceptionCode);
		dprintwarning("PHP_CatchIt -- Exception Code is : 0x%08lx", lpTopLevelExceptionFilter->ExceptionRecord->ExceptionCode);
		//dprintverbose("Exception Code is : 0x%08lx", lpTopLevelExceptionFilter->ExceptionRecord->ExceptionCode);
	}
	DumpIt(lpTopLevelExceptionFilter);

	////Log the current request info
	//php_error_docref(NULL TSRMLS_CC, E_NOTICE, "active function %s @ %s:%d", 
	//					get_active_function_name(TSRMLS_C),
	//					zend_get_executed_filename(TSRMLS_C),
	//					zend_get_executed_lineno(TSRMLS_C)
	//					);

	dprintverbose("%s", "Leaving PHPUnhandledExceptionFilter");

    return EXCEPTION_EXECUTE_HANDLER;
  }


/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(catchit)
{
	long error;
	
	/* If you have INI entries, uncomment these lines 	*/
	REGISTER_INI_ENTRIES();
	
	Orig_UnhandledExceptionFilter = SetUnhandledExceptionFilter(PHPUnhandledExceptionFilter);
	dprintverbose("PHP_MINIT_FUNCTION catchit: %s", "catchit now monitor for un-handled exceptions");
	php_error_docref(NULL TSRMLS_CC, E_NOTICE, "catchit now monitor for un-handled exceptions");

	dprintverbose("%s", "Now DetourAPIs");

	error = DetourAPIs();
	if (error == ERROR_SUCCESS ) {
		php_error_docref(NULL TSRMLS_CC, E_NOTICE, "CatchIt now minitoring for APIs which can result in process termination.");
	}
	else 
	{
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "CatchIt failed to minitoring for APIs which can result in process termination.");
	}

	 
   /* SignalHandlerPointer previousHandler;
    previousHandler = signal(SIGABRT, SignalHandler);

	php_error_docref(NULL TSRMLS_CC, E_WARNING, "SignalHandler Registered.");*/

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(catchit)
{
	/* uncomment this line if you have INI entries 	*/
	UNREGISTER_INI_ENTRIES();

	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());

	DetDetach((PVOID*)(&Real_TerminateProcess), CatchIt_TerminateProcess, "TerminateProcess");
	DetDetach((PVOID*)(&Real_ExitProcess), CatchIt_ExitProcess, "ExitProcess");
	DetDetach((PVOID*)(&Real_Abort), CatchIt_Abort, "Abort");
	DetDetach((PVOID*)(&Real_Exit), CatchIt_Exit, "Exit");
	DetDetach((PVOID*)(&Real__Exit), CatchIt__Exit, "_Exit");

	DetDetach((PVOID*)(&Real_SetUnhandledExceptionFilter), CatchIt_SetUnhandledExceptionFilter, "SetUnhandledExceptionFilter"); //Prevent others to run SetUnhandledExceptionFilter again



	//DETACH(TerminateProcess); //Windows API TerminateProcess
	//DETACH(ExitProcess); //Windows API ExitProcess
	////DETACH(Terminate); 
	//// //c runtime API terminate
	//DETACH(Abort);  //c runtime API abort
	//DETACH(Exit); //c runtime API exit
	//DETACH(_Exit); //c runtime API _exit

	//DETACH(SetUnhandledExceptionFilter); //Prevent others to run SetUnhandledExceptionFilter again

	DetourTransactionCommit();

	//restore the origin handler
	if (NULL != Orig_UnhandledExceptionFilter)
		SetUnhandledExceptionFilter(Orig_UnhandledExceptionFilter);


	dprintverbose("%s", "SHUTDOWN php_catchit");
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(catchit)
{
	const char* typeStr;
	const char* logLevelStr;

	php_info_print_table_start();
	php_info_print_table_header(2, "Catchit", "Setting");

	php_info_print_table_row(2, "version", PHP_CATCHIT_VERSION);

	//Log level
	switch (INI_INT("catchit.logLevel")) {
#define X(uc, lc) case uc: logLevelStr = #lc; break;
		LOG_LEVEL_MAP(X)
#undef X
default: logLevelStr = "unknown";
	}

	php_info_print_table_row(2, "logLevel", logLevelStr);

	//dumpType
	switch (INI_INT("catchit.dumpType")) {
#define X(uc, lc) case uc: typeStr = #uc; break;
		DUMP_TYPE_MAP(X)
#undef X
default: typeStr = "<unknown>";
	}

	php_info_print_table_row(2, "dumpType", typeStr);

	//logFolder
	php_info_print_table_row(2, "dumpFolder", INI_STR("catchit.dumpFolder"));

	//externalDebugger
	if (INI_BOOL("catchit.usingExternalDebugger"))
	{
		php_info_print_table_row(2, "usingExternalDebugger", "Yes");
	}
	else 
	{
		php_info_print_table_row(2, "usingExternalDebugger", "No");
	}
	php_info_print_table_row(2, "externalDebugger", INI_STR("catchit.externalDebugger"));

	php_info_print_table_end();


	/* Remove comments if you have entries in php.ini */
	DISPLAY_INI_ENTRIES();
}
/* }}} */

/* {{{ catchit_functions[]
 *
 * Every user visible function must have an entry in catchit_functions[].
 */
const zend_function_entry catchit_functions[] = {
	//PHP_FE(confirm_catchit_compiled,	NULL)		/* For testing, remove later. */
	PHP_FE_END	/* Must be the last line in catchit_functions[] */
};
/* }}} */

/* {{{ catchit_module_entry
 */
zend_module_entry catchit_module_entry = {
	STANDARD_MODULE_HEADER,
	"catchit",
	catchit_functions,
	PHP_MINIT(catchit),
	PHP_MSHUTDOWN(catchit),
	NULL,		/* Replace with NULL if there's nothing to do at request start */
	NULL,	/* Replace with NULL if there's nothing to do at request end */
	//PHP_RINIT(catchit),		/* Replace with NULL if there's nothing to do at request start */
	//PHP_RSHUTDOWN(catchit),	/* Replace with NULL if there's nothing to do at request end */
	PHP_MINFO(catchit),
	PHP_CATCHIT_VERSION,
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_CATCHIT
ZEND_GET_MODULE(catchit)
#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */

