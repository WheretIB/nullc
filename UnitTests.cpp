#if defined(_MSC_VER)
	#include "stdafx.h"
#endif

#include "UnitTests.h"

#include "NULLC/nullc.h"
#include "NULLC/nullc_debug.h"

// Check that remote debug module compiles correctly
#if defined(_MSC_VER)
	#include "NULLC/nullc_remote.h"
#endif

#include "NULLC/includes/file.h"
#include "NULLC/includes/math.h"
#include "NULLC/includes/vector.h"
#include "NULLC/includes/random.h"
#include "NULLC/includes/dynamic.h"
#include "NULLC/includes/gc.h"
#include "NULLC/includes/time.h"
#include "NULLC/includes/string.h"

#include "NULLC/includes/canvas.h"
#include "NULLC/includes/window.h"
#include "NULLC/includes/io.h"

#include "NULLC/includes/pugi.h"

#include "tests/TestBase.h"
#include "tests/TestSpeed.h"
#include "tests/TestCompileFail.h"
#include "tests/TestParseFail.h"
#include "tests/TestInterface.h"

#if defined(_MSC_VER)
#pragma warning(disable: 4127 4996)
#endif

//#define ALLOC_TOP_DOWN
//#define NO_CUSTOM_ALLOCATOR

void* testAlloc(int size)
{
#ifdef ALLOC_TOP_DOWN
	return VirtualAlloc(NULL, size + 128, MEM_COMMIT | MEM_TOP_DOWN, PAGE_READWRITE);
#else
	char *ptr = new char[size + 128];
	memset(ptr, 0xee, 128);
	return ptr + 128;
#endif
}
void testDealloc(void* ptr)
{
	if(!ptr)
		return;
#ifdef ALLOC_TOP_DOWN
	VirtualFree((char*)ptr - 128, 0, MEM_RELEASE);
#else
	ptr = (char*)ptr - 128;
	for(unsigned i = 0; i < 128; i++)
		assert(((unsigned char*)ptr)[i] == 0xee);
	delete[] (char*)ptr;
#endif
}

nullres CompileFile(const char* fileName)
{
	static char content[64 * 1024];

	FILE *euler = fopen(fileName, "rb");
	fseek(euler, 0, SEEK_END);
	unsigned int textSize = ftell(euler);
	assert(textSize < 64 * 1024);
	fseek(euler, 0, SEEK_SET);
	fread(content, 1, textSize, euler);
	content[textSize] = 0;
	fclose(euler);

	return nullcCompile(content);
}

int RunTests(bool verbose, const void* (*fileLoadFunc)(const char*, unsigned int*, int*), bool runSpeedTests, bool testOutput, bool testTranslationSave)
{
	Tests::messageVerbose = verbose;
	Tests::fileLoadFunc = fileLoadFunc;

	// Extra tests

	// Safe sprintf test
	{
		testsCount[TEST_TYPE_EXTRA]++;

		char buf[8];
		char *pos = buf + NULLC::SafeSprintf(buf, 8, "this ");
		pos += NULLC::SafeSprintf(pos, 8 - int(pos - buf), "string is too long");
		if(memcmp(buf, "this st", 8) != 0)
			printf("Safe sprintf test failed: string is incorrect\n");
		else if(pos != buf + 8)
			printf("Safe sprintf test failed: iterator is incorrect\n");
		else
			testsPassed[TEST_TYPE_EXTRA]++;
	}

/*
	unsigned int tStart = clock();
	for(unsigned int i = 0; i < 10000; i++)
	{
		nullcInit("Modules/");
		nullcTerminate();
	}
	printf("Finished in %d\r\n", clock() - tStart);
*/

	Tests::enableLogFiles = testOutput;
	Tests::doSaveTranslation = testTranslationSave;

	// To enable real log files during tests
	/*
	Tests::openStreamFunc = 0;
	Tests::writeStreamFunc = 0;
	Tests::closeStreamFunc = 0;
	*/

	// Init NULLC
#ifdef NO_CUSTOM_ALLOCATOR
	nullcInit(MODULE_PATH);
#else
	nullcInitCustomAlloc(testAlloc, testDealloc, MODULE_PATH);
#endif
	nullcSetFileReadHandler(Tests::fileLoadFunc);
	nullcSetEnableLogFiles(Tests::enableLogFiles, Tests::openStreamFunc, Tests::writeStreamFunc, Tests::closeStreamFunc);

	nullcInitTypeinfoModule();
	nullcInitDynamicModule();
	RunInterfaceTests();

#ifdef NO_CUSTOM_ALLOCATOR
	nullcInit(MODULE_PATH);
#else
	nullcInitCustomAlloc(testAlloc, testDealloc, MODULE_PATH);
#endif
	nullcSetFileReadHandler(Tests::fileLoadFunc);
	nullcSetEnableLogFiles(Tests::enableLogFiles, Tests::openStreamFunc, Tests::writeStreamFunc, Tests::closeStreamFunc);

	nullcInitTypeinfoModule();
	nullcInitFileModule();
	nullcInitMathModule();
	nullcInitVectorModule();
	nullcInitRandomModule();
	nullcInitDynamicModule();
	nullcInitGCModule();
	nullcInitStringModule();

	nullcInitIOModule();
	nullcInitCanvasModule();

#if defined(_MSC_VER)
	nullcInitWindowModule();
#endif

	/*
	//SpeedTestFile("test_document.nc");
	//SpeedTestFile("shapes.nc");
	//SpeedTestFile("raytrace.nc");
	//SpeedTestFile("blob.nc");
	return 0;*/

	RunParseFailTests();
	RunCompileFailTests();

#ifdef __linux
	system("cp NULLC/translation/runtime.h runtime.h");
	system("cp NULLC/translation/runtime.h tests/translation/runtime.h");
#else
	_popen("copy \"NULLC\\translation\\runtime.h\" \"runtime.h\"", "r");
	_popen("copy \"NULLC\\translation\\runtime.h\" \"tests\\translation\\runtime.h\"", "r");
#endif

	TestQueue queue;
	queue.RunTests();

	// Conclusion 
	printf("VM passed %d of %d tests\n", testsPassed[TEST_TYPE_VM], testsCount[TEST_TYPE_VM]);

	printf("Expr Evaluated %d of %d tests\n", testsPassed[TEST_TYPE_EXPR_EVALUATION], testsCount[TEST_TYPE_EXPR_EVALUATION]);
	printf("Inst Evaluated %d of %d tests\n", testsPassed[TEST_TYPE_INST_EVALUATION], testsCount[TEST_TYPE_INST_EVALUATION]);

#ifdef NULLC_BUILD_X86_JIT
	printf("X86 passed %d of %d tests\n", testsPassed[TEST_TYPE_X86], testsCount[TEST_TYPE_X86]);
#else
	testsPassed[TEST_TYPE_X86] = 0;
	testsCount[TEST_TYPE_X86] = 0;
#endif

#ifdef NULLC_LLVM_SUPPORT
	printf("LLVM passed %d of %d tests\n", testsPassed[TEST_TYPE_LLVM], testsCount[TEST_TYPE_LLVM]);
#else
	testsPassed[TEST_TYPE_LLVM] = 0;
	testsCount[TEST_TYPE_LLVM] = 0;
#endif

	printf("Failure tests: passed %d of %d tests\n", testsPassed[TEST_TYPE_FAILURE], testsCount[TEST_TYPE_FAILURE]);
	printf("Extra tests: passed %d of %d tests\n", testsPassed[TEST_TYPE_EXTRA], testsCount[TEST_TYPE_EXTRA]);
	printf("Translation tests: passed %d of %d tests\n", testsPassed[TEST_TYPE_TRANSLATION], testsCount[TEST_TYPE_TRANSLATION]);

	unsigned allTests = 0;
	unsigned allPassed = 0;
	for(unsigned i = 0; i < TEST_TYPE_COUNT; i++)
	{
		allTests += testsCount[i];
		allPassed += testsPassed[i];
	}

	printf("Compilation time: %f\n", Tests::timeCompile);
	printf("Get bytecode time: %f\n", Tests::timeGetBytecode);
	printf("Expression evaluation time: %f\n", Tests::timeExprEvaluate);
	printf("Instruction evaluation time: %f\n", Tests::timeInstEvaluate);
	printf("Translation time: %f\n", Tests::timeTranslate);
	printf("Clean time: %f\n", Tests::timeClean);
	printf("Link time: %f\n", Tests::timeLinkCode);
	printf("Run time: %f\n", Tests::timeRun);

	printf("Total log output: %lld\n", Tests::totalOutput);

	printf("Passed %d of %d tests\n", allPassed, allTests);

	if(runSpeedTests)
		RunSpeedTests();

#ifdef __linux
	system("rm runtime.h");
	system("rm tests/translation/runtime.h");
#else
	_popen("del \"runtime.h\"", "r");
	_popen("del \"tests\\translation\\runtime.h\"", "r");
#endif

	// Terminate NULLC
	nullcTerminate();

	return allPassed == allTests ? 0 : 1;
}
