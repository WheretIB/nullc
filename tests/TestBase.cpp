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

#if defined(NULLC_ENABLE_C_TRANSLATION) && defined(_MSC_VER)
	if(executor == NULLC_X86 && doTranslation)
	{
		testsCount[TEST_TRANSLATION_INDEX]++;
		nullcTranslateToC("1test.cpp", "main");

		STARTUPINFO stInfo;
		PROCESS_INFORMATION prInfo;
		memset(&stInfo, 0, sizeof(stInfo));
		stInfo.cb = sizeof(stInfo);

		memset(&prInfo, 0, sizeof(prInfo));
		char cmdLine[1024];
		strcpy(cmdLine, "gcc.exe -o runnable.exe");
		strcat(cmdLine, " 1test.cpp");
		strcat(cmdLine, " NULLC\\translation\\runtime.cpp -lstdc++");
		if(strstr(code, "std.math;"))
		{
			strcat(cmdLine, " NULLC\\translation\\std_math.cpp");
			strcat(cmdLine, " NULLC\\translation\\std_math_bind.cpp");
		}
		if(strstr(code, "std.typeinfo;"))
		{
			strcat(cmdLine, " NULLC\\translation\\std_typeinfo.cpp");
			strcat(cmdLine, " NULLC\\translation\\std_typeinfo_bind.cpp");
		}
		if(strstr(code, "std.file;"))
		{
			strcat(cmdLine, " NULLC\\translation\\std_file.cpp");
			strcat(cmdLine, " NULLC\\translation\\std_file_bind.cpp");
		}
		if(strstr(code, "std.vector;"))
		{
			strcat(cmdLine, " NULLC\\translation\\std_vector.cpp");
			strcat(cmdLine, " NULLC\\translation\\std_vector_bind.cpp");
			if(!strstr(code, "std.typeinfo;"))
			{
				strcat(cmdLine, " NULLC\\translation\\std_typeinfo.cpp");
				strcat(cmdLine, " NULLC\\translation\\std_typeinfo_bind.cpp");
			}
		}
		if(strstr(code, "std.list;"))
		{
			strcat(cmdLine, " NULLC\\translation\\std_list.cpp");
			if(!strstr(code, "std.typeinfo;") && !strstr(code, "std.vector;"))
			{
				strcat(cmdLine, " NULLC\\translation\\std_typeinfo.cpp");
				strcat(cmdLine, " NULLC\\translation\\std_typeinfo_bind.cpp");
			}
		}
		if(strstr(code, "std.range;"))
		{
			strcat(cmdLine, " NULLC\\translation\\std_range.cpp");
		}
		if(strstr(code, "test.a;"))
		{
			strcat(cmdLine, " test_a.cpp");
			if(!strstr(code, "std.math;"))
			{
				strcat(cmdLine, " NULLC\\translation\\std_math.cpp");
				strcat(cmdLine, " NULLC\\translation\\std_math_bind.cpp");
			}
		}
		if(strstr(code, "test.importhide;"))
			strcat(cmdLine, " test_importhide.cpp");
		if(strstr(code, "test.defargs;"))
			strcat(cmdLine, " test_defargs.cpp");
		if(strstr(code, "test.defargs2;"))
			strcat(cmdLine, " test_defargs2.cpp");
		if(strstr(code, "test.defargs3;"))
			strcat(cmdLine, " test_defargs3.cpp");
		if(strstr(code, "test.defargs4;"))
		{
			strcat(cmdLine, " test_defargs4.cpp");
			strcat(cmdLine, " test_defargs4_bind.cpp");
		}
		if(strstr(code, "test.defargs5;"))
			strcat(cmdLine, " test_defargs5.cpp");
		if(strstr(code, "test.defargs6;"))
			strcat(cmdLine, " test_defargs6.cpp");
		if(strstr(code, "test.alignment;"))
		{
			strcat(cmdLine, " test_alignment.cpp");
			strcat(cmdLine, " test_alignment_bind.cpp");
		}
		if(strstr(code, "std.gc;"))
		{
			strcat(cmdLine, " NULLC\\translation\\std_gc.cpp");
			strcat(cmdLine, " NULLC\\translation\\std_gc_bind.cpp");
		}
		if(strstr(code, "std.dynamic;"))
		{
			strcat(cmdLine, " NULLC\\translation\\std_dynamic.cpp");
			strcat(cmdLine, " NULLC\\translation\\std_dynamic_bind.cpp");
		}
		
		DWORD res = CreateProcess(NULL, cmdLine, NULL, NULL, false, 0, NULL, ".\\", &stInfo, &prInfo);
		res = GetLastError();
		WaitForSingleObject(prInfo.hProcess, 10000);
		DWORD retCode;
		GetExitCodeProcess(prInfo.hProcess, &retCode);
		CloseHandle(prInfo.hProcess);
		CloseHandle(prInfo.hThread);
		if(!retCode)
		{
			memset(&stInfo, 0, sizeof(stInfo));
			stInfo.cb = sizeof(stInfo);

			memset(&prInfo, 0, sizeof(prInfo));
			DWORD res = CreateProcess("runnable.exe", "runnable.exe", NULL, NULL, false, 0, NULL, ".\\", &stInfo, &prInfo);
			res = GetLastError();
			WaitForSingleObject(prInfo.hProcess, 10000);
			GetExitCodeProcess(prInfo.hProcess, &retCode);
			if(atoi(expected) != (int)retCode)
			{
				if(message && !messageVerbose)
					printf("%s\n", message);
				printf("C++: failed, expected %s, got %d\r\n", expected, retCode);
			}else{
				testsPassed[TEST_TRANSLATION_INDEX]++;
			}
			CloseHandle(prInfo.hProcess);
			CloseHandle(prInfo.hThread);
		}else{
			if(message && !messageVerbose)
				printf("%s\n", message);
			printf("C++ compilation error\r\n", expected, retCode);
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
