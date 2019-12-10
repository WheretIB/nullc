#include "TestBase.h"

#if defined(_MSC_VER)
	#pragma warning(disable: 4530)
#endif

#include "../NULLC/includes/file.h"
#include "../NULLC/includes/math.h"
#include "../NULLC/includes/vector.h"
#include "../NULLC/includes/random.h"
#include "../NULLC/includes/dynamic.h"
#include "../NULLC/includes/gc.h"
#include "../NULLC/includes/time.h"

#include "../NULLC/includes/canvas.h"
#include "../NULLC/includes/window.h"
#include "../NULLC/includes/io.h"

#include "../NULLC/includes/pugi.h"

struct TestEval : TestQueue
{
	virtual void Run()
	{
		nullcTerminate();
		nullcInit();
		nullcAddImportPath(MODULE_PATH_A);
		nullcAddImportPath(MODULE_PATH_B);
		nullcSetFileReadHandler(Tests::fileLoadFunc, Tests::fileFreeFunc);
		nullcSetEnableLogFiles(Tests::enableLogFiles, Tests::openStreamFunc, Tests::writeStreamFunc, Tests::closeStreamFunc);
		nullcSetEnableTimeTrace(Tests::enableTimeTrace);
		nullcInitDynamicModule();

		const char	*testEval =
"import std.dynamic;\r\n\
\r\n\
int a = 5;\r\n\
\r\n\
for(int i = 0; i < 200; i++)\r\n\
	eval(\"a += 3 * \" + i.str() + \";\");\r\n\
\r\n\
return a;";
		double evalStart = myGetPreciseTime();
		for(int t = 0; t < TEST_TARGET_COUNT; t++)
		{
			if(!Tests::testExecutor[t])
				continue;
			testsCount[t]++;
			if(Tests::RunCodeSimple(testEval, testTarget[t], "59705", "Dynamic code. eval()", false, ""))
				testsPassed[t]++;
			printf("Eval test finished in %f\n", myGetPreciseTime() - evalStart);
		}
		nullcTerminate();
		nullcInit();
		nullcAddImportPath(MODULE_PATH_A);
		nullcAddImportPath(MODULE_PATH_B);
		nullcSetFileReadHandler(Tests::fileLoadFunc, Tests::fileFreeFunc);
		nullcSetEnableLogFiles(Tests::enableLogFiles, Tests::openStreamFunc, Tests::writeStreamFunc, Tests::closeStreamFunc);
		nullcSetEnableTimeTrace(Tests::enableTimeTrace);
		nullcInitDynamicModule();
	}
};
TestEval testEval;

const char	*testCoroutineAndEval =
"import std.dynamic;\r\n\
coroutine int foo()\r\n\
{\r\n\
	int i = 0;\r\n\
	yield i++;\r\n\
	yield i++;\r\n\
	yield i++;\r\n\
	return i;\r\n\
}\r\n\
foo();\r\n\
int x = foo();\r\n\
int a = 5;\r\n\
\r\n\
for(int i = 0; i < 200; i++)\r\n\
	eval(\"a = 3 * \" + i.str() + \";\");\r\n\
int y = foo();\r\n\
return x * 10 + y;";
TEST_SIMPLE("Coroutine and dynamic code. eval()", testCoroutineAndEval, "12")
{
}

struct TestVariableImportCorrectness : TestQueue
{
	virtual void Run()
	{
		nullcTerminate();
		nullcInit();
		nullcAddImportPath(MODULE_PATH_A);
		nullcAddImportPath(MODULE_PATH_B);
		nullcSetFileReadHandler(Tests::fileLoadFunc, Tests::fileFreeFunc);
		nullcSetEnableLogFiles(Tests::enableLogFiles, Tests::openStreamFunc, Tests::writeStreamFunc, Tests::closeStreamFunc);
		nullcSetEnableTimeTrace(Tests::enableTimeTrace);
		nullcInitDynamicModule();

		const char	*testVariableImportCorrectness =
"import std.dynamic;\r\n\
int x = 2;\r\n\
eval(\"x = 4;\");\r\n\
{ int n, m; }\r\n\
int y = 3;\r\n\
eval(\"y = 8;\");\r\n\
return x * 10 + y;";
		for(int t = 0; t < TEST_TARGET_COUNT; t++)
		{
			if(!Tests::testExecutor[t])
				continue;

			testsCount[t]++;
			if(Tests::RunCodeSimple(testVariableImportCorrectness, testTarget[t], "48", "Variable import correctness", false, ""))
				testsPassed[t]++;
		}
	}
};
TestVariableImportCorrectness testVariableImportCorrectness;

struct TestGCGlobalLimit : TestQueue
{
	virtual void Run()
	{
		nullcTerminate();
		nullcInit();
		nullcAddImportPath(MODULE_PATH_A);
		nullcAddImportPath(MODULE_PATH_B);
		nullcSetFileReadHandler(Tests::fileLoadFunc, Tests::fileFreeFunc);
		nullcSetEnableLogFiles(Tests::enableLogFiles, Tests::openStreamFunc, Tests::writeStreamFunc, Tests::closeStreamFunc);
		nullcSetEnableTimeTrace(Tests::enableTimeTrace);
		nullcInitGCModule();
		nullcSetGlobalMemoryLimit(1024 * 1024);

		const char	*testGCGlobalLimit =
		"import std.gc;\r\n\
		int[] arr1 = new int[200000];\r\n\
		arr1 = new int[2];\r\n\
		int[] arr2 = new int[200000];\r\n\
		return arr2.size;";
		for(int t = 0; t < TEST_TARGET_COUNT; t++)
		{
			if(!Tests::testExecutor[t])
				continue;

			testsCount[t]++;
			if(Tests::RunCode(testGCGlobalLimit, testTarget[t], "200000", "GC collection before global limit exceeded error"))
				testsPassed[t]++;
		}
	}
};
TestGCGlobalLimit testGCGlobalLimit;

struct TestRestore : TestQueue
{
	virtual void Run()
	{
		nullcTerminate();
		nullcInit();
		nullcAddImportPath(MODULE_PATH_A);
		nullcAddImportPath(MODULE_PATH_B);
		nullcSetFileReadHandler(Tests::fileLoadFunc, Tests::fileFreeFunc);
		nullcSetEnableLogFiles(Tests::enableLogFiles, Tests::openStreamFunc, Tests::writeStreamFunc, Tests::closeStreamFunc);
		nullcSetEnableTimeTrace(Tests::enableTimeTrace);

		nullcInitTypeinfoModule();
		nullcInitFileModule();
		nullcInitMathModule();
		nullcInitVectorModule();
		nullcInitRandomModule();
		nullcInitDynamicModule();
		nullcInitGCModule();
		nullcInitIOModule();
		nullcInitCanvasModule();
#if defined(_MSC_VER)
		nullcInitWindowModule();
#endif
	}
};
TestRestore testRestore;
