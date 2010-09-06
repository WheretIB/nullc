#include "TestInterface.h"

#include "TestBase.h"

void RunInterfaceTests()
{
	unsigned int	testTarget[] = { NULLC_VM, NULLC_X86, NULLC_LLVM };

	if(Tests::messageVerbose)
		printf("Two bytecode merge test 1\r\n");

	const char *partA1 = "int a = 5;\r\nint c = 8;\r\nint test(int ref a, int b)\r\n{\r\n\treturn *a += b;\r\n}\r\ntest(&a, 4);\r\nint run(){ test(&a, 4); return c; }\r\n";
	const char *partB1 = "int aa = 15;\r\nint testA(int ref a, int b)\r\n{\r\n\treturn *a += b + 1;\r\n}\r\ntestA(&aa, 5);\r\nvoid runA(){ testA(&aa, 5); }\r\nreturn aa;\r\n";

	char *bytecodeA, *bytecodeB;
	bytecodeA = NULL;
	bytecodeB = NULL;

	for(int t = 0; t < TEST_COUNT; t++)
	{
		if(!Tests::testExecutor[t])
			continue;
		testsCount[t]++;
		nullcSetExecutor(testTarget[t]);

		nullres good = nullcCompile(partA1);
		nullcSaveListing("asm.txt");
		if(!good)
		{
			if(!Tests::messageVerbose)
				printf("Two bytecode merge test 1\r\n");
			printf("Compilation failed: %s\r\n", nullcGetLastError());
			continue;
		}else{
			nullcGetBytecode(&bytecodeA);
		}

		good = nullcCompile(partB1);
		nullcSaveListing("asm.txt");
		if(!good)
		{
			if(!Tests::messageVerbose)
				printf("Two bytecode merge test 1\r\n");
			printf("Compilation failed: %s\r\n", nullcGetLastError());
			continue;
		}else{
			nullcGetBytecode(&bytecodeB);
		}

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
		}else{
			int* val = (int*)nullcGetGlobal("c");
			if(*val != 8)
			{
				if(!Tests::messageVerbose)
					printf("Two bytecode merge test 1\r\n");
				printf("nullcGetGlobal failed");
				continue;
			}
			int n = 45;
			nullcSetGlobal("c", &n);
			if(!nullcRunFunction("run"))
			{
				if(!Tests::messageVerbose)
					printf("Two bytecode merge test 1\r\n");
				printf("Execution failed: %s\r\n", nullcGetLastError());
			}else{
				if(nullcGetResultInt() != n)
				{
					if(!Tests::messageVerbose)
						printf("Two bytecode merge test 1\r\n");
					printf("nullcSetGlobal failed");
					continue;
				}
				if(!nullcRunFunction("runA"))
				{
					if(!Tests::messageVerbose)
						printf("Two bytecode merge test 1\r\n");
					printf("Execution failed: %s\r\n", nullcGetLastError());
				}else{
					Tests::varData = (char*)nullcGetVariableData(NULL);
					Tests::varInfo = nullcDebugVariableInfo(&Tests::variableCount);
					Tests::symbols = nullcDebugSymbols(NULL);

					if(Tests::varInfo)
					{
						bool lastFailed = false;
						CHECK_INT("a", 0, 13);
						CHECK_INT("aa", 0, 27);
						if(!lastFailed)
							testsPassed[t]++;
					}
				}
			}
		}
	}

//////////////////////////////////////////////////////////////////////////
	{
		if(Tests::messageVerbose)
			printf("Function update test\r\n");

		const char *partA = "int foo(){ return 15; }";
		const char *partB = "import __last; import std.dynamic; int new_foo(){ return 25; } void foo_update(){ override(foo, new_foo); }\r\n";
		
		int vmPassed = testsPassed[0], x86Passed = testsPassed[1];
		(void)x86Passed;
		for(int t = 0; t < TEST_COUNT; t++)
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
		if(vmPassed + 1 != testsPassed[0])
			printf("VM failed test: Function update test\r\n");
		if(TEST_COUNT >= 2 && x86Passed + 1 != testsPassed[1])
			printf("X86 failed test: Function update test\r\n");
	}

	{
		if(Tests::messageVerbose)
			printf("Function update test 2\r\n");

		const char *partA = "int foo(){ return 15; }";
		const char *partB = "int new_foo(){ return 25; }\r\n";

		int vmPassed = testsPassed[0], x86Passed = testsPassed[1];
		(void)x86Passed;
		for(int t = 0; t < TEST_COUNT; t++)
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
		if(vmPassed + 2 != testsPassed[0])
			printf("VM failed test: Function update test 2\r\n");
		if(TEST_COUNT >= 2 && x86Passed + 2 != testsPassed[1])
			printf("X86 failed test: Function update test 2\r\n");
	}

	{
		if(Tests::messageVerbose)
			printf("Value pass through nullcRunFunction\r\n");

		const char *code = "int Char(char x){ return -x*2; } int Short(short x){ return -x*3; } int Int(int x){ return -x*4; } int Long(long x){ return -x*5; } int Float(float x){ return -x*6; } int Double(double x){ return -x*7; } int Ptr(int ref x){ return -*x; }";

		int vmPassed = testsPassed[0], x86Passed = testsPassed[1];
		(void)x86Passed;
		for(int t = 0; t < TEST_COUNT; t++)
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
			if(!nullcRunFunction(name, arg)){ printf("nullcRunFunction("name") failed: %s\r\n", nullcGetLastError()); continue; }\
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
		if(vmPassed + 1 != testsPassed[0])
			printf("VM failed test: Value pass through nullcCallFunction\r\n");
		if(TEST_COUNT >= 2 && x86Passed + 1 != testsPassed[1])
			printf("X86 failed test: Value pass through nullcCallFunction\r\n");
	}
	{
		if(Tests::messageVerbose)
			printf("Structure pass through nullcRunFunction\r\n");

		const char *code = "int foo(int[] arr){ return arr[0] + arr.size; } int NULLCRef(auto ref x){ return -int(x); } int bar(){ return 127; } int NULLCFunc(int ref() x){ return x(); } int NULLCArray(auto[] arr){ return int(arr[0]); }";

		int vmPassed = testsPassed[0], x86Passed = testsPassed[1];
		(void)x86Passed;
		for(int t = 0; t < TEST_COUNT; t++)
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
		if(vmPassed + 1 != testsPassed[0])
			printf("VM failed test: Structure pass through nullcCallFunction\r\n");
		if(TEST_COUNT >= 2 && x86Passed + 1 != testsPassed[1])
			printf("X86 failed test: Structure pass through nullcCallFunction\r\n");
	}

	const char	*testLongRetrieval = "return 25l;";
	if(Tests::messageVerbose)
		printf("nullcGetResultLong test\r\n");
	for(int t = 0; t < TEST_COUNT; t++)
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
	for(int t = 0; t < TEST_COUNT; t++)
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
}
