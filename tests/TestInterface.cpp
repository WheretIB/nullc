#include "TestInterface.h"

#include "TestBase.h"
#include "../NULLC/nullc_debug.h"

bool	initialized;

#define TEST_COMPARE(test, result)\
	testsCount[TEST_TYPE_EXTRA]++;\
	if((test) != result)\
	{\
		printf("\"%s\" didn't return '%s'.\r\n", #test, #result);\
	}else{\
		testsPassed[TEST_TYPE_EXTRA]++;\
	}

#define TEST_COMPARES(test, result)\
	testsCount[TEST_TYPE_EXTRA]++;\
	if(strcmp((test), result) != 0)\
	{\
		printf("\"%s\" didn't return '%s'.\r\n", #test, result);\
	}else{\
		testsPassed[TEST_TYPE_EXTRA]++;\
	}

void RunInterfaceTests()
{
	if(Tests::messageVerbose)
		printf("Two bytecode merge test 1\r\n");

	const char *partA1 = "int a = 5;\r\nint c = 8;\r\nint test(int ref a, int b)\r\n{\r\n\treturn *a += b;\r\n}\r\ntest(&a, 4);\r\nint run(){ test(&a, 4); return c; }\r\n";
	const char *partB1 = "int aa = 15;\r\nint testA(int ref a, int b)\r\n{\r\n\treturn *a += b + 1;\r\n}\r\ntestA(&aa, 5);\r\nvoid runA(){ testA(&aa, 5); }\r\nreturn aa;\r\n";

	{
		char *bytecodeA, *bytecodeB;
		bytecodeA = NULL;
		bytecodeB = NULL;

		for(int t = 0; t < TEST_TARGET_COUNT; t++)
		{
			if(!Tests::testExecutor[t])
				continue;
			testsCount[t]++;
			nullcSetExecutor(testTarget[t]);

			nullres good = nullcCompile(partA1);
			nullcSaveListing(FILE_PATH "asm.txt");
			if(!good)
			{
				if(!Tests::messageVerbose)
					printf("Two bytecode merge test 1\r\n");
				printf("Compilation failed: %s\r\n", nullcGetLastError());
				continue;
			}
			else
			{
				nullcGetBytecode(&bytecodeA);
			}

			good = nullcCompile(partB1);
			nullcSaveListing(FILE_PATH "asm.txt");
			if(!good)
			{
				if(!Tests::messageVerbose)
					printf("Two bytecode merge test 1\r\n");
				printf("Compilation failed: %s\r\n", nullcGetLastError());
				continue;
			}
			else
			{
				nullcGetBytecode(&bytecodeB);
			}

			remove(FILE_PATH "asm.txt");

			nullcClean();
			if(!nullcLinkCode(bytecodeA))
			{
				if(!Tests::messageVerbose)
					printf("Two bytecode merge test 1\r\n");
				printf("Compilation failed: %s\r\n", nullcGetLastError());
				continue;
			}
			if(!nullcLinkCode(bytecodeB))
			{
				if(!Tests::messageVerbose)
					printf("Two bytecode merge test 1\r\n");
				printf("Compilation failed: %s\r\n", nullcGetLastError());
				break;
			}
			delete[] bytecodeA;
			delete[] bytecodeB;

			if(!nullcRunFunction(NULL))
			{
				if(!Tests::messageVerbose)
					printf("Two bytecode merge test 1\r\n");
				printf("Execution failed: %s\r\n", nullcGetLastError());
			}
			else
			{
				int* val = (int*)nullcGetGlobal("c");
				if(*val != 8)
				{
					if(!Tests::messageVerbose)
						printf("Two bytecode merge test 1\r\n");
					printf("nullcGetGlobal failed\r\n");
					continue;
				}
				int n = 45;
				nullcSetGlobal("c", &n);
				if(!nullcRunFunction("run"))
				{
					if(!Tests::messageVerbose)
						printf("Two bytecode merge test 1\r\n");
					printf("Execution failed: %s\r\n", nullcGetLastError());
				}
				else
				{
					if(nullcGetResultInt() != n)
					{
						if(!Tests::messageVerbose)
							printf("Two bytecode merge test 1\r\n");
						printf("nullcSetGlobal failed\r\n");
						continue;
					}
					if(!nullcRunFunction("runA"))
					{
						if(!Tests::messageVerbose)
							printf("Two bytecode merge test 1\r\n");
						printf("Execution failed: %s\r\n", nullcGetLastError());
					}
					else
					{
						Tests::varData = (char*)nullcGetVariableData(NULL);
						Tests::varInfo = nullcDebugVariableInfo(&Tests::variableCount);
						Tests::symbols = nullcDebugSymbols(NULL);

						if(Tests::varInfo)
						{
							bool lastFailed = false;
							CHECK_INT("a", 0, 13, lastFailed);
							CHECK_INT("aa", 0, 27, lastFailed);
							if(!lastFailed)
								testsPassed[t]++;
						}
					}
				}
			}
		}
	}

	{
		if(Tests::messageVerbose)
			printf("Function update test\r\n");

		const char *partA = "int foo(){ return 15; }";
		const char *partB = "import __last; import std.dynamic; int new_foo(){ return 25; } void foo_update(){ override(foo, new_foo); }\r\n";
		
		int regVmPassed = testsPassed[TEST_TYPE_REGVM], x86Passed = testsPassed[TEST_TYPE_X86];
		(void)x86Passed;
		for(int t = 0; t < TEST_TARGET_COUNT; t++)
		{
			if(!Tests::testExecutor[t])
				continue;
			testsCount[t]++;
			nullcSetExecutor(testTarget[t]);

			char *bytecodeA = NULL, *bytecodeB = NULL;

			nullres good = nullcCompile(partA);
			if(!good)
			{
				printf("Compilation failed: %s\r\n", nullcGetLastError());
				continue;
			}else{
				nullcGetBytecode(&bytecodeA);
			}

			nullcClean();
			if(!nullcLinkCode(bytecodeA))
			{
				printf("Compilation failed: %s\r\n", nullcGetLastError());
				continue;
			}
			delete[] bytecodeA;

			if(!nullcRunFunction(NULL))
			{
				printf("Execution failed: %s\r\n", nullcGetLastError());
				continue;
			}
			if(!nullcRunFunction("foo"))
			{
				printf("Execution failed: %s\r\n", nullcGetLastError());
				continue;
			}
			if(nullcGetResultInt() != 15)
			{
				printf("original foo failed to return 15\r\n");
				continue;
			}

			if(!nullcCompile(partB))
			{
				printf("%s", nullcGetLastError());
				continue;
			}
			nullcGetBytecodeNoCache(&bytecodeB);
			if(!nullcLinkCode(bytecodeB))
			{
				delete[] bytecodeB;
				printf("%s", nullcGetLastError());
				continue;
			}
			delete[] bytecodeB;

			if(!nullcRunFunction("foo_update"))
			{
				printf("foo_update Execution failed: %s\r\n", nullcGetLastError());
				continue;
			}

			if(!nullcRunFunction("foo"))
			{
				printf("foo Execution failed: %s\r\n", nullcGetLastError());
				continue;
			}
			if(nullcGetResultInt() != 25)
			{
				printf("new foo failed to return 25\r\n");
				continue;
			}
			testsPassed[t]++;
		}
		if(regVmPassed + 1 != testsPassed[NULLC_REG_VM])
			printf("REGVM failed test: Function update test\r\n");
		if(Tests::testExecutor[NULLC_X86] && x86Passed + 1 != testsPassed[NULLC_X86])
			printf("X86 failed test: Function update test\r\n");
	}

	{
		if(Tests::messageVerbose)
			printf("Function update test 2\r\n");

		const char *partA = "int foo(){ return 15; }";
		const char *partB = "int new_foo(){ return 25; }\r\n";

		int regVmPassed = testsPassed[TEST_TYPE_REGVM], x86Passed = testsPassed[TEST_TYPE_X86];
		(void)x86Passed;
		for(int t = 0; t < TEST_TARGET_COUNT; t++)
		{
			if(!Tests::testExecutor[t])
				continue;
			testsCount[t]++;
			nullcSetExecutor(testTarget[t]);

			char *bytecodeA = NULL, *bytecodeB = NULL;

			nullres good = nullcCompile(partA);
			if(!good)
			{
				printf("Compilation failed: %s\r\n", nullcGetLastError());
				continue;
			}else{
				nullcGetBytecode(&bytecodeA);
			}

			nullcClean();
			if(!nullcLinkCode(bytecodeA))
			{
				printf("Compilation failed: %s\r\n", nullcGetLastError());
				continue;
			}
			delete[] bytecodeA;

			if(!nullcRunFunction(NULL))
			{
				printf("Execution failed: %s\r\n", nullcGetLastError());
				continue;
			}
			if(!nullcRunFunction("foo"))
			{
				printf("Execution failed: %s\r\n", nullcGetLastError());
				continue;
			}
			if(nullcGetResultInt() != 15)
			{
				printf("original foo failed to return 15\r\n");
				continue;
			}

			if(!nullcCompile(partB))
			{
				printf("%s", nullcGetLastError());
				continue;
			}
			nullcGetBytecode(&bytecodeB);
			if(!nullcLinkCode(bytecodeB))
			{
				delete[] bytecodeB;
				printf("%s", nullcGetLastError());
				continue;
			}
			delete[] bytecodeB;

			NULLCFuncPtr fooOld;
			NULLCFuncPtr fooNew;
			if(!nullcGetFunction("foo", &fooOld))
			{
				printf("nullcGetFunction(foo) failed: %s\r\n", nullcGetLastError());
				continue;
			}
			if(!nullcGetFunction("new_foo", &fooNew))
			{
				printf("nullcGetFunction(new_foo) failed: %s\r\n", nullcGetLastError());
				continue;
			}
			if(!nullcSetFunction("foo", fooNew))
			{
				printf("nullcSetFunction(foo) failed: %s\r\n", nullcGetLastError());
				continue;
			}

			if(!nullcRunFunction("foo"))
			{
				printf("foo Execution failed: %s\r\n", nullcGetLastError());
				continue;
			}
			if(nullcGetResultInt() != 25)
			{
				printf("new foo failed to return 25\r\n");
				continue;
			}
			testsPassed[t]++;
			testsCount[t]++;
			if(!nullcCallFunction(fooOld))
			{
				printf("foo Execution failed: %s\r\n", nullcGetLastError());
				continue;
			}
			if(nullcGetResultInt() != 25)
			{
				printf("new foo failed to return 25\r\n");
				continue;
			}
			testsPassed[t]++;
		}
		if(regVmPassed + 2 != testsPassed[NULLC_REG_VM])
			printf("REGVM failed test: Function update test 2\r\n");
		if(Tests::testExecutor[NULLC_X86] && x86Passed + 2 != testsPassed[NULLC_X86])
			printf("X86 failed test: Function update test 2\r\n");
	}

	{
		if(Tests::messageVerbose)
			printf("Value pass through nullcRunFunction\r\n");

		const char *code = "int Char(char x){ return -x*2; } int Short(short x){ return -x*3; } int Int(int x){ return -x*4; } int Long(long x){ return -x*5; } int Float(float x){ return -x*6; } int Double(double x){ return -x*7; } int Ptr(int ref x){ return -*x; }";

		int regVmPassed = testsPassed[TEST_TYPE_REGVM], x86Passed = testsPassed[TEST_TYPE_X86];
		(void)x86Passed;
		for(int t = 0; t < TEST_TARGET_COUNT; t++)
		{
			if(!Tests::testExecutor[t])
				continue;
			testsCount[t]++;
			nullcSetExecutor(testTarget[t]);

			nullres good = nullcBuild(code);
			if(!good)
			{
				printf("Build failed: %s\r\n", nullcGetLastError());
				continue;
			}
#define TEST_CALL(name, arg, expected)\
			if(!nullcRunFunction(name, arg)){ printf("nullcRunFunction(" name ") failed: %s\r\n", nullcGetLastError()); continue; }\
			if(nullcGetResultInt() != expected){ printf("nullcGetResultInt failed to return " #expected " result\r\n"); continue; }

			TEST_CALL("Char", 12, -12*2);
			TEST_CALL("Short", 13, -13*3);
			TEST_CALL("Int", 14, -14*4);
			TEST_CALL("Long", 15LL, -15*5LL);
			TEST_CALL("Float", 4.0f, -4.0f*6.0f);
			TEST_CALL("Double", 5.0f, -5.0*7.0);

			int ptrTest = 88;
			TEST_CALL("Ptr", &ptrTest, -88);

			testsPassed[t]++;
		}
		if(regVmPassed + 1 != testsPassed[TEST_TYPE_REGVM])
			printf("REGVM failed test: Value pass through nullcCallFunction\r\n");
		if(Tests::testExecutor[NULLC_X86] && x86Passed + 1 != testsPassed[NULLC_X86])
			printf("X86 failed test: Value pass through nullcCallFunction\r\n");
	}
	{
		if(Tests::messageVerbose)
			printf("Structure pass through nullcRunFunction\r\n");

		const char *code = "int foo(int[] arr){ return arr[0] + arr.size; } int NULLCRef(auto ref x){ return -int(x); } int bar(){ return 127; } int NULLCFunc(int ref() x){ return x(); } int NULLCArray(auto[] arr){ return int(arr[0]); }";

		int regVmPassed = testsPassed[TEST_TYPE_REGVM], x86Passed = testsPassed[TEST_TYPE_X86];
		(void)x86Passed;
		for(int t = 0; t < TEST_TARGET_COUNT; t++)
		{
			if(!Tests::testExecutor[t])
				continue;
			testsCount[t]++;
			nullcSetExecutor(testTarget[t]);

			nullres good = nullcBuild(code);
			if(!good)
			{
				printf("Build failed: %s\r\n", nullcGetLastError());
				continue;
			}

			int x = 50;
			NULLCArray arr = { (char*)&x, 1 };
			TEST_CALL("foo", arr, 51);

			x = 14;
			NULLCRef ref = { 4, (char*)&x };
			TEST_CALL("NULLCRef", ref, -14);

			NULLCFuncPtr fPtr;
			if(!nullcGetFunction("bar", &fPtr))
			{
				printf("Unable to get 'bar' function: %s\r\n", nullcGetLastError());
				continue;
			}
			TEST_CALL("NULLCFunc", fPtr, 127);

			x = 77;
			NULLCAutoArray autoArr = { 4, (char*)&x, 1 };
			TEST_CALL("NULLCArray", autoArr, 77);
			
			testsPassed[t]++;
		}
		if(regVmPassed + 1 != testsPassed[TEST_TYPE_REGVM])
			printf("REGVM failed test: Structure pass through nullcCallFunction\r\n");
		if(Tests::testExecutor[NULLC_X86] && x86Passed + 1 != testsPassed[NULLC_X86])
			printf("X86 failed test: Structure pass through nullcCallFunction\r\n");
	}

	const char	*testLongRetrieval = "return 25l;";
	if(Tests::messageVerbose)
		printf("nullcGetResultLong test\r\n");
	for(int t = 0; t < TEST_TARGET_COUNT; t++)
	{
		if(!Tests::testExecutor[t])
			continue;
		testsCount[t]++;
		nullcSetExecutor(testTarget[t]);
		nullres r = nullcBuild(testLongRetrieval);
		if(!r)
		{
			if(!Tests::messageVerbose)
				printf("nullcGetResultLong test\r\n");
			printf("Build failed:%s\r\n", nullcGetLastError());
			continue;
		}
		if(!nullcRun())
		{
			if(!Tests::messageVerbose)
				printf("nullcGetResultLong test\r\n");
			printf("Execution failed:%s\r\n", nullcGetLastError());
			continue;
		}
		if(nullcGetResultLong() == 25ll)
		{
			testsPassed[t]++;
		}else{
			if(!Tests::messageVerbose)
				printf("nullcGetResultLong test\r\n");
			printf("Incorrect result: %s", nullcGetResult());
		}
	}

	const char	*testDoubleRetrieval = "return 25.0;";
	if(Tests::messageVerbose)
		printf("nullcGetResultDouble test\r\n");
	for(int t = 0; t < TEST_TARGET_COUNT; t++)
	{
		if(!Tests::testExecutor[t])
			continue;
		testsCount[t]++;
		nullcSetExecutor(testTarget[t]);
		nullres r = nullcBuild(testDoubleRetrieval);
		if(!r)
		{
			if(!Tests::messageVerbose)
				printf("nullcGetResultDouble test\r\n");
			printf("Build failed:%s\r\n", nullcGetLastError());
			continue;
		}
		if(!nullcRun())
		{
			if(!Tests::messageVerbose)
				printf("nullcGetResultDouble test\r\n");
			printf("Execution failed:%s\r\n", nullcGetLastError());
			continue;
		}
		if(nullcGetResultDouble() == 25.0)
		{
			testsPassed[t]++;
		}else{
			if(!Tests::messageVerbose)
				printf("nullcGetResultDouble test\r\n");
			printf("Incorrect result: %s", nullcGetResult());
		}
	}

	if(Tests::messageVerbose)
		printf("nullcRunFunction test\r\n");

	for(int t = 0; t < TEST_TARGET_COUNT; t++)
	{
		if(!Tests::testExecutor[t])
			continue;
		testsCount[t]++;
		nullcSetExecutor(testTarget[t]);
		if(!nullcBuild("int main(){return 2;}"))
		{
			printf("Build failed:%s\n", nullcGetLastError());
		}else{
			if(!nullcRunFunction("main"))
			{
				printf("Run failed: %s\n", nullcGetLastError());
			}else{
				if(memcmp(nullcGetResult(), "2", 1))
					printf("Return value != 2\n");
				else
					testsPassed[t]++;
			}
		}
	}

	if(Tests::messageVerbose)
		printf("Type constant check\r\n");

	for(int t = 0; t < TEST_TARGET_COUNT; t++)
	{
		if(!Tests::testExecutor[t])
			continue;
		testsCount[t]++;
		nullcSetExecutor(testTarget[t]);

		nullres good = nullcCompile("return 1;");
		if(!good)
		{
			printf("Compilation failed: %s\r\n", nullcGetLastError());
			continue;
		}else{
			char *bytecode = NULL;
			nullcGetBytecode(&bytecode);

			ExternTypeInfo *types = FindFirstType((ByteCode*)bytecode);
			const char *symbols = FindSymbols((ByteCode*)bytecode);

			TEST_COMPARES(types[NULLC_TYPE_VOID].offsetToName + symbols, "void");
			TEST_COMPARES(types[NULLC_TYPE_BOOL].offsetToName + symbols, "bool");
			TEST_COMPARES(types[NULLC_TYPE_CHAR].offsetToName + symbols, "char");
			TEST_COMPARES(types[NULLC_TYPE_SHORT].offsetToName + symbols, "short");
			TEST_COMPARES(types[NULLC_TYPE_INT].offsetToName + symbols, "int");
			TEST_COMPARES(types[NULLC_TYPE_LONG].offsetToName + symbols, "long");
			TEST_COMPARES(types[NULLC_TYPE_FLOAT].offsetToName + symbols, "float");
			TEST_COMPARES(types[NULLC_TYPE_DOUBLE].offsetToName + symbols, "double");
			TEST_COMPARES(types[NULLC_TYPE_TYPEID].offsetToName + symbols, "typeid");
			TEST_COMPARES(types[NULLC_TYPE_FUNCTION].offsetToName + symbols, "__function");
			TEST_COMPARES(types[NULLC_TYPE_NULLPTR].offsetToName + symbols, "__nullptr");
			TEST_COMPARES(types[NULLC_TYPE_GENERIC].offsetToName + symbols, "generic");
			TEST_COMPARES(types[NULLC_TYPE_AUTO].offsetToName + symbols, "auto");
			TEST_COMPARES(types[NULLC_TYPE_AUTO_REF].offsetToName + symbols, "auto ref");
			TEST_COMPARES(types[NULLC_TYPE_VOID_REF].offsetToName + symbols, "void ref");
			TEST_COMPARES(types[NULLC_TYPE_AUTO_ARRAY].offsetToName + symbols, "auto[]");

			delete[] bytecode;

			testsPassed[t]++;
		}
	}

	nullcBuild("coroutine int main(){ yield 1; yield 2; }");
	TEST_COMPARE(nullcRunFunction("main"), 0);
	TEST_COMPARES(nullcGetLastError(), "ERROR: function uses context, which is unavailable");
	
	nullcBuild("int main(){ int foo(){ return 5; } return foo(); } int foo(){ return 6; }");
	TEST_COMPARE(nullcRunFunction("foo"), 1);
	TEST_COMPARE(nullcGetResultInt(), 6);

	nullcTerminate();
	TEST_COMPARES(nullcGetLastError(), "");

	TEST_COMPARE(nullcBuild("return 1;"), 0);
	TEST_COMPARES(nullcGetLastError(), "ERROR: NULLC is not initialized");

	// double termination handling
	nullcTerminate();
	TEST_COMPARES(nullcGetLastError(), "");

	TEST_COMPARE(nullcRun(), 0);
	TEST_COMPARES(nullcGetLastError(), "ERROR: NULLC is not initialized");
	TEST_COMPARE(nullcRunFunction("test", 1, 3), 0);
	TEST_COMPARES(nullcGetLastError(), "ERROR: NULLC is not initialized");

	TEST_COMPARES(nullcGetResult(), "");
	TEST_COMPARES(nullcGetLastError(), "ERROR: NULLC is not initialized");
	TEST_COMPARE(nullcGetResultInt(), 0);
	TEST_COMPARES(nullcGetLastError(), "ERROR: NULLC is not initialized");
	TEST_COMPARE(nullcGetResultDouble(), 0.0);
	TEST_COMPARES(nullcGetLastError(), "ERROR: NULLC is not initialized");
	TEST_COMPARE(nullcGetResultLong(), 0);
	TEST_COMPARES(nullcGetLastError(), "ERROR: NULLC is not initialized");

	TEST_COMPARE(nullcAllocate(1024), NULL);
	TEST_COMPARES(nullcGetLastError(), "ERROR: NULLC is not initialized");
	nullcThrowError("ERROR: shouldn't throw");
	TEST_COMPARES(nullcGetLastError(), "ERROR: NULLC is not initialized");
	NULLCFuncPtr fPtr = { 0, 0 };
	TEST_COMPARE(nullcCallFunction(fPtr, 1, 2), false);
	TEST_COMPARES(nullcGetLastError(), "ERROR: NULLC is not initialized");
	TEST_COMPARE(nullcGetGlobal("test"), NULL);
	TEST_COMPARES(nullcGetLastError(), "ERROR: NULLC is not initialized");
	TEST_COMPARE(nullcSetGlobal("test", NULL), false);
	TEST_COMPARES(nullcGetLastError(), "ERROR: NULLC is not initialized");
	TEST_COMPARE(nullcGetFunction("test", &fPtr), false);
	TEST_COMPARES(nullcGetLastError(), "ERROR: NULLC is not initialized");
	TEST_COMPARE(nullcSetFunction("test", fPtr), false);
	TEST_COMPARES(nullcGetLastError(), "ERROR: NULLC is not initialized");
	TEST_COMPARE(nullcIsStackPointer(NULL), false);
	TEST_COMPARE(nullcIsManagedPointer(NULL), false);

	TEST_COMPARE(nullcCompile("return 1"), false);
	TEST_COMPARES(nullcGetLastError(), "ERROR: NULLC is not initialized");
	char *temp = NULL;
	TEST_COMPARE(nullcGetBytecode(&temp), 0);
	TEST_COMPARES(nullcGetLastError(), "ERROR: NULLC is not initialized");
	TEST_COMPARE(nullcGetBytecodeNoCache(&temp), 0);
	TEST_COMPARES(nullcGetLastError(), "ERROR: NULLC is not initialized");
	nullcSaveListing("\\/\\/\\/\\/\\\\/\\");
	TEST_COMPARES(nullcGetLastError(), "ERROR: NULLC is not initialized");
	nullcTranslateToC("\\/\\/\\/\\/\\\\/\\", "main", NULL);
	TEST_COMPARES(nullcGetLastError(), "ERROR: NULLC is not initialized");
	nullcClean();
	TEST_COMPARES(nullcGetLastError(), "ERROR: NULLC is not initialized");
	TEST_COMPARE(nullcLinkCode(temp), false);
	TEST_COMPARES(nullcGetLastError(), "ERROR: NULLC is not initialized");

#ifdef NULLC_BUILD_X86_JIT
	TEST_COMPARE(nullcSetExecutorStackSize(1024), false);
	TEST_COMPARES(nullcGetLastError(), "ERROR: NULLC is not initialized");
#endif

#if !defined(NULLC_NO_RAW_EXTERNAL_CALL)
	TEST_COMPARE(nullcBindModuleFunction("std.test", NULL, "test", 0), false);
#endif

	TEST_COMPARE(nullcBindModuleFunctionWrapper("std.test", NULL, NULL, "test", 0), false);
	TEST_COMPARES(nullcGetLastError(), "ERROR: NULLC is not initialized");
	TEST_COMPARE(nullcLoadModuleBySource("std.test", "return 1;"), false);
	TEST_COMPARES(nullcGetLastError(), "ERROR: NULLC is not initialized");
	TEST_COMPARE(nullcLoadModuleByBinary("std.test", NULL), false);
	TEST_COMPARES(nullcGetLastError(), "ERROR: NULLC is not initialized");

	// double initialization check
	nullcInit();
	TEST_COMPARES(nullcGetLastError(), "");
	nullcInit();
	TEST_COMPARES(nullcGetLastError(), "ERROR: NULLC is already initialized");
	nullcTerminate();
	TEST_COMPARES(nullcGetLastError(), "");
}
