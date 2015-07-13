# PHP Exception Handling Module
A PHP module to handle exceptions happened in php-cgi.exe running on Windows IIS(NTS).  


##Why need this module  
For a process crash issue, we used to capture a memory dump file for analysis the cause. However, this is very hard for PHP running on IIS. On IIS, PHP is configured to run as NTS(none-thread-safe). For a busy system, it is very easy to see hundreds of PHP-CGI.EXE processes running concurrently. Attaching a debugger to each PHP-CGI.exe instance is not feasible as this easily makes the whole system un-usable.  

This module did nothing on PHP runtime, but monitor for exceptions, so there is no additional overhead. When an un-handled exception happened, it will write a dump file.  


###Un-handled Exception
When a un-handled exception happens, this usually leads to process crash. This modules works as a top level exception handler by calling [SetUnhandledExceptionFilter](https://msdn.microsoft.com/en-us/library/windows/desktop/ms680634(v=vs.85).aspx). This module will be called when if an exception was not handled by anyone else. Then, it will write a dump before real crash the process.  


###APIs can terminate a process:
This module also monitors follow APIs, a dump will generated when those APIs are called.
-	TerminateProcess     //Windows API TerminateProcess  
-	ExitProcess          //Windows API ExitProcess  
-	terminate            //c runtime API terminate  
-	abort                //c runtime API abort  
-	exit                 //c runtime API exit  
-	_exit               //c runtime API _exit  


To achieve this, the [detour APIs](http://research.microsoft.com/en-us/projects/detours/) are used to detour related APIs.  


##How To Use
Like other PHP modules, simply configure this as a PHP extension. Follow is the configration of the module.

[catchit]
catchit.dumpType = 0  
catchit.dumpFolder = d:\home\logfiles  
catchit.usingExternalDebugger = 0  
catchit.externalDebugger = d:\home\site\dbg\procdump.exe -accepteula -ma  

Commonly used [dumpType](https://msdn.microsoft.com/en-us/library/windows/desktop/ms680519%28v=vs.85%29.aspx) type is 0x0(minidump) and 0x2(fulldump).  

**MiniDumpNormal(dumpType=0)**  
Include just the information necessary to capture stack traces for all existing threads in a process. This is the default option.  

**MiniDumpWithFullMemory(dumpType=2)**  
Include all accessible memory in the process. The raw memory data is included at the end, so that the initial structures can be mapped directly without the raw memory information. This option can result in a very large file.  


By default, the module write dump file using Windows API. This works almost when you want a **mini dump** contains very limited information. However, more than 90% times, this API fails when you want a **Full Memory Dump**. In the PHP error log, you will see this error:

[26-Mar-2015 02:38:06 America/Los_Angeles] PHP Notice:  Unknown: Failed to write dump file, *MiniDumpWriteDump* API failed with 0x8007012b   

This happens when the API tries to write dump of the process itself. For more information, read the **Remarks** section of [MiniDumpWriteDump](https://msdn.microsoft.com/en-us/library/windows/desktop/ms680360(v=vs.85).aspx) API document.

To resolve this, you can use the external debugger. I only tested with **procdump** with above syntax. The module only appends process ID and output folder to this command.



##Enable Log

Put these two lines in the .user.ini, the module will write some output to the php error log.

*error_reporting = E_ALL*   
*log_errors = on*  


**Startup Information:**  
[26-Mar-2015 22:52:20 America/Los_Angeles] PHP Notice:  PHP Startup: catchit now monitor for un-handled exceptions in Unknown on line 0  
[26-Mar-2015 22:52:20 America/Los_Angeles] PHP Notice:  PHP Startup: Start Detour APIs in Unknown on line 0  
[26-Mar-2015 22:52:20 America/Los_Angeles] PHP Notice:  PHP Startup: Done Detour APIs in Unknown on line 0  
[26-Mar-2015 22:52:20 America/Los_Angeles] PHP Notice:  PHP Startup: CatchIt now minitoring for APIs which can result in process termination.   


**Exception Information:**  
[26-Mar-2015 22:52:25 America/Los_Angeles] PHP Warning:  Unknown: An unhandled excepton happened in Unknown on line 0  
[26-Mar-2015 22:52:25 America/Los_Angeles] PHP Warning:  Unknown: PHP_CatchIt -- Exception Code is : 0xc0000005 in Unknown on line 0  

**APIs can terminate process:**  
[26-Mar-2015 20:49:17 America/Los_Angeles] PHP Notice:  Unknown: PHP_CatchIt -- Someone is invoking TerminateProcess in Unknown on line 0  

**Dump Writing Information:**  
[26-Mar-2015 20:49:17 America/Los_Angeles] PHP Notice:  Unknown: write dump using external debugger in Unknown on line 0  
[26-Mar-2015 20:49:17 America/Los_Angeles] PHP Notice:  Unknown: dump will write to d:\home\logfiles\PHP-CGI-wzhao-MiniDumpWithFullMemory-2015-3-27-3-49-17-887-UTC-2504.dmp in Unknown on line 0  
[26-Mar-2015 20:49:18 America/Los_Angeles] PHP Notice:  Unknown: d:\home\site\dbg\procdump.exe -accepteula -ma  2504 d:\home\logfiles\PHP-CGI-wzhao-MiniDumpWithFullMemory-2015-3-27-3-49-17-887-UTC-2504.dmp is running, wait to finish... in Unknown on line 0  

**Active Request Info(function name, file name and line number):**  

**Un-handled Exception:**  
[09-Apr-2015 00:21:51 America/Los_Angeles] PHP Warning:  av(): PHP_CatchIt -- Exception Code is : 0xc0000005 in D:\home\site\wwwroot\exception.php on line 10  
[09-Apr-2015 00:22:06 America/Los_Angeles] PHP Notice:  av(): active function av @ D:\home\site\wwwroot\exception.php:10  

**APIs result in process termination**  
[13-Apr-2015 02:02:14 America/Los_Angeles] PHP Warning:  endapis(): PHP_CatchIt -- Someone is invoking *abort* in D:\home\site\wwwroot\exception.php on line 11  
[13-Apr-2015 02:02:27 America/Los_Angeles] PHP Notice:  endapis(): active function endapis @ D:\home\site\wwwroot\exception.php:11  
