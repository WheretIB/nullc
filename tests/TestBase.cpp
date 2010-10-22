#include "TestBase.h"

#pragma warning(disable: 4530)
#if defined(_MSC_VER)
	#include "../stdafx.h"
	#include <Windows.h>
#endif

#include "../NULLC/nullc_debug.h"

#if !defined(_MSC_VER)
#include <time.h>
	double myGetPreciseTime()
	{
		return (clock() / double(CLOCKS_PER_SEC)) * 1000.0;
	}
#endif

TestQueue* TestQueue::head = NULL;
TestQueue* TestQueue::tail = NULL;

int testsPassed[] = { 0, 0, 0, 0, 0, 0 };
int testsCount[] = { 0, 0, 0, 0, 0, 0 };
unsigned int	testTarget[] = { NULLC_VM, NULLC_X86, NULLC_LLVM };

namespace Tests
{
	bool messageVerbose = false;
	const char *lastMessage = NULL;

	double timeCompile = 0.0;
	double timeGetListing = 0.0;
	double timeGetBytecode = 0.0;
	double timeClean = 0.0;
	double timeLinkCode = 0.0;
	double timeRun = 0.0;

	const char		*varData = NULL;
	unsigned int	variableCount = 0;
	ExternVarInfo	*varInfo = NULL;
	const char		*symbols = NULL;

	bool doTranslation = true;

	bool	testExecutor[3] = {
		true,
#ifdef NULLC_BUILD_X86_JIT
		true,
#else
		false,
#endif
#ifdef NULLC_LLVM_SUPPORT
		true
#else
		false
#endif
	};
}

void*	Tests::FindVar(const char* name)
{
	for(unsigned int i = 0; i < variableCount; i++)
	{
		if(strcmp(name, symbols + varInfo[i].offsetToName) == 0)
			return (void*)(varData + varInfo[i].offset);
	}
	return (void*)varData;
}

bool	Tests::RunCode(const char *code, unsigned int executor, const char* expected, const char* message, bool execShouldFail)
{
	lastMessage = message;
#ifndef NULLC_BUILD_X86_JIT
	if(executor != NULLC_VM)
		return false;
#endif
	if(message && messageVerbose && executor == NULLC_VM)
		printf("%s\n", message);

	nullcSetExecutor(executor);

	char buf[256];
	sprintf(buf, "%s", executor == NULLC_VM ? "VM " : executor == NULLC_X86 ? "X86" : "LLVM");

	double time = myGetPreciseTime();
	nullres good = nullcCompile(code);
	timeCompile += myGetPreciseTime() - time;

	time = myGetPreciseTime();
	nullcSaveListing("asm.txt");
	timeGetListing += myGetPreciseTime() - time;

	if(!good)
	{
		if(message && !messageVerbose)
			printf("%s\n", message);
		printf("%s Compilation failed: %s\r\n", buf, nullcGetLastError());
		return false;
	}else{
		char *bytecode;
		time = myGetPreciseTime();
		nullcGetBytecode(&bytecode);
		timeGetBytecode += myGetPreciseTime() - time;
		time = myGetPreciseTime();
		nullcClean();
		timeClean += myGetPreciseTime() - time;
		time = myGetPreciseTime();
		int linkgood = nullcLinkCode(bytecode);
		timeLinkCode += myGetPreciseTime() - time;
		delete[] bytecode;

		if(!linkgood)
		{
			if(message && !messageVerbose)
				printf("%s\n", message);
			printf("%s Link failed: %s\r\n", buf, nullcGetLastError());
			return false;
		}

		time = myGetPreciseTime();
		nullres goodRun = nullcRun();
		timeRun += myGetPreciseTime() - time;
		if(goodRun)
		{
			if(execShouldFail)
			{
				if(message && !messageVerbose)
					printf("%s\n", message);
				printf("%s Execution should have failed with %s\r\n", buf, expected);
				return false;
			}
			const char* val = nullcGetResult();
			varData = (char*)nullcGetVariableData(NULL);

			nullcFinalize();

			if(expected && strcmp(val, expected) != 0)
			{
				if(message && !messageVerbose)
					printf("%s\n", message);
				printf("%s Failed (%s != %s)\r\n", buf, val, expected);
				return false;
			}
		}else{
			if(execShouldFail)
			{
				char buf[512];
				strcpy(buf, strstr(nullcGetLastError(), "ERROR:"));
				if(char *lineEnd = strchr(buf, '\r'))
					*lineEnd = 0;
				if(strcmp(expected, buf) != 0)
				{
					if(message && !messageVerbose)
						printf("%s\n", message);
					printf("Failed but for wrong reason:\r\n    %s\r\nexpected:\r\n    %s\r\n", buf, expected);
					return false;
				}
				return true;
			}
			
			if(message && !messageVerbose)
				printf("%s\n", message);
			printf("%s Execution failed: %s\r\n", buf, nullcGetLastError());
			return false;
		}
	}

	varData = (char*)nullcGetVariableData(NULL);
	varInfo = nullcDebugVariableInfo(&variableCount);
	symbols = nullcDebugSymbols(NULL);

#if defined(NULLC_ENABLE_C_TRANSLATION)
	if(executor == NULLC_VM && doTranslation)
	{
		testsCount[TEST_TRANSLATION_INDEX]++;
		nullcTranslateToC("1test.cpp", "main");

		char cmdLine[1024];
#if defined(_MSC_VER)
		STARTUPINFO stInfo;
		PROCESS_INFORMATION prInfo;
		memset(&stInfo, 0, sizeof(stInfo));
		stInfo.cb = sizeof(stInfo);
		memset(&prInfo, 0, sizeof(prInfo));
		strcpy(cmdLine, "gcc.exe -o runnable.exe");
#else
		strcpy(cmdLine, "gcc -o runnable");
#endif

		strcat(cmdLine, " 1test.cpp");
		strcat(cmdLine, " NULLC/translation/runtime.cpp -lstdc++");
		if(strstr(code, "std.math;"))
		{
			strcat(cmdLine, " NULLC/translation/std_math.cpp");
			strcat(cmdLine, " NULLC/translation/std_math_bind.cpp");
		}
		if(strstr(code, "std.typeinfo;"))
		{
			strcat(cmdLine, " NULLC/translation/std_typeinfo.cpp");
			strcat(cmdLine, " NULLC/translation/std_typeinfo_bind.cpp");
		}
		if(strstr(code, "std.file;"))
		{
			strcat(cmdLine, " NULLC/translation/std_file.cpp");
			strcat(cmdLine, " NULLC/translation/std_file_bind.cpp");
		}
		if(strstr(code, "std.vector;"))
		{
			strcat(cmdLine, " NULLC/translation/std_vector.cpp");
			strcat(cmdLine, " NULLC/translation/std_vector_bind.cpp");
			if(!strstr(code, "std.typeinfo;"))
			{
				strcat(cmdLine, " NULLC/translation/std_typeinfo.cpp");
				strcat(cmdLine, " NULLC/translation/std_typeinfo_bind.cpp");
			}
		}
		if(strstr(code, "std.list;"))
		{
			strcat(cmdLine, " NULLC/translation/std_list.cpp");
			if(!strstr(code, "std.typeinfo;") && !strstr(code, "std.vector;"))
			{
				strcat(cmdLine, " NULLC/translation/std_typeinfo.cpp");
				strcat(cmdLine, " NULLC/translation/std_typeinfo_bind.cpp");
			}
		}
		if(strstr(code, "std.range;"))
			strcat(cmdLine, " NULLC/translation/std_range.cpp");
		if(strstr(code, "test.a;"))
		{
			strcat(cmdLine, " tests/translation/test_a.cpp");
			if(!strstr(code, "std.math;"))
			{
				strcat(cmdLine, " NULLC/translation/std_math.cpp");
				strcat(cmdLine, " NULLC/translation/std_math_bind.cpp");
			}
		}
		if(strstr(code, "test.importhide;"))
			strcat(cmdLine, " tests/translation/test_importhide.cpp");
		if(strstr(code, "test.defargs;"))
			strcat(cmdLine, " tests/translation/test_defargs.cpp");
		if(strstr(code, "test.defargs2;"))
			strcat(cmdLine, " tests/translation/test_defargs2.cpp");
		if(strstr(code, "test.defargs3;"))
			strcat(cmdLine, " tests/translation/test_defargs3.cpp");
		if(strstr(code, "test.defargs4;"))
		{
			strcat(cmdLine, " tests/translation/test_defargs4.cpp");
			strcat(cmdLine, " tests/translation/test_defargs4_bind.cpp");
		}
		if(strstr(code, "test.defargs5;"))
			strcat(cmdLine, " tests/translation/test_defargs5.cpp");
		if(strstr(code, "test.defargs6;"))
			strcat(cmdLine, " tests/translation/test_defargs6.cpp");
		if(strstr(code, "test.coroutine1;"))
			strcat(cmdLine, " tests/translation/test_coroutine1.cpp");
		if(strstr(code, "test.autorefcall;"))
			strcat(cmdLine, " tests/translation/test_autorefcall.cpp");
		if(strstr(code, "test.class_typedef;"))
			strcat(cmdLine, " tests/translation/test_class_typedef.cpp");
		
		if(strstr(code, "test.list_comp1;"))
		{
			strcat(cmdLine, " NULLC/translation/std_range.cpp");
			strcat(cmdLine, " tests/translation/test_list_comp1.cpp");
		}
		if(strstr(code, "test.list_comp2;"))
		{
			if(!strstr(code, "test.list_comp1;"))
				strcat(cmdLine, " NULLC/translation/std_range.cpp");
			strcat(cmdLine, " tests/translation/test_list_comp2.cpp");
		}
		
		if(strstr(code, "test.alignment;"))
		{
			strcat(cmdLine, " tests/translation/test_alignment.cpp");
			strcat(cmdLine, " tests/translation/test_alignment_bind.cpp");
		}
		if(strstr(code, "std.gc;"))
		{
			strcat(cmdLine, " NULLC/translation/std_gc.cpp");
			strcat(cmdLine, " NULLC/translation/std_gc_bind.cpp");
		}
		if(strstr(code, "std.dynamic;"))
		{
			strcat(cmdLine, " NULLC/translation/std_dynamic.cpp");
			strcat(cmdLine, " NULLC/translation/std_dynamic_bind.cpp");
		}

#if defined(_MSC_VER)
		DWORD res = CreateProcess(NULL, cmdLine, NULL, NULL, false, 0, NULL, ".\\", &stInfo, &prInfo);
		res = GetLastError();
		WaitForSingleObject(prInfo.hProcess, 10000);
		DWORD retCode;
		GetExitCodeProcess(prInfo.hProcess, &retCode);
		CloseHandle(prInfo.hProcess);
		CloseHandle(prInfo.hThread);
#else
		int retCode = system(cmdLine);
#endif
		if(!retCode)
		{
#if defined(_MSC_VER)
			FILE *resPipe = _popen("runnable.exe", "rb");
#else
			FILE *resPipe = popen("./runnable", "r");
#endif
			fgets(buf, 256, resPipe);

			char expectedCopy[256];
			strcpy(expectedCopy, expected);
			if(strchr(expectedCopy, 'L'))
				*strchr(expectedCopy, 'L') = 0;
			if(strcmp(buf, expectedCopy))
			{
				if(message && !messageVerbose)
					printf("%s\n", message);
				printf("C++: failed, expected %s, got %s\r\n", expected, buf);
			}else{
				testsPassed[TEST_TRANSLATION_INDEX]++;
			}
		}else{
			if(message && !messageVerbose)
				printf("%s\n", message);
			printf("C++ compilation error (%d)\r\n", retCode);
		}
	}
#endif

	return true;
}

char*	Tests::Format(const char *str, ...)
{
	static char text[4096*16];
	static unsigned int section = 0;

	char* ptr = text + (section++ % 16) * 4096;

	va_list args;
	va_start(args, str);
	vsnprintf(ptr, 1023, str, args);
	va_end(args);
	return ptr;
}
