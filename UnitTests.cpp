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

#include "NULLC/ParseClass.h"

#include "NULLC/includes/file.h"
#include "NULLC/includes/math.h"
#include "NULLC/includes/vector.h"
#include "NULLC/includes/random.h"
#include "NULLC/includes/dynamic.h"
#include "NULLC/includes/gc.h"
#include "NULLC/includes/time.h"

#include "NULLC/includes/canvas.h"
#include "NULLC/includes/window.h"
#include "NULLC/includes/io.h"

#include "NULLC/includes/pugi.h"

#include "tests/TestBase.h"
#include "tests/TestSpeed.h"
#include "tests/TestCompileFail.h"
#include "tests/TestParseFail.h"
#include "tests/TestInterface.h"

#pragma warning(disable: 4127)

FILE *allocLog = NULL;
void* testAlloc(size_t size)
{
	if(!allocLog)
		allocLog = fopen("testAlloc.txt", "wb");
	static size_t overall = 0;
	static int allocs = 0;
	overall += size;
	allocs++;
	fprintf(allocLog, "%d Alloc of %u bytes (Total %u)\r\n", allocs, (unsigned int)size, (unsigned int)overall);
	fflush(allocLog);
	return malloc(size);
}
void testDealloc(void* ptr)
{
	free(ptr);
}

nullres CompileFile(const char* fileName)
{
	char content[64 * 1024];

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

void	RunTests(bool verbose)
{
	Tests::messageVerbose = verbose;

	// Extra tests

	// Safe sprintf test
	{
		testsCount[3]++;

		char buf[8];
		char *pos = buf + SafeSprintf(buf, 8, "this ");
		pos += SafeSprintf(pos, 8 - int(pos - buf), "string is too long");
		if(memcmp(buf, "this st", 8) != 0)
			printf("Safe sprintf test failed: string is incorrect\n");
		else if(pos != buf + 8)
			printf("Safe sprintf test failed: iterator is incorrect\n");
		else
			testsPassed[3]++;
	}

/*
	unsigned int tStart = clock();
	for(unsigned int i = 0; i < 10000; i++)
	{
		nullcInit("Modules\\");
		nullcTerminate();
	}
	printf("Finished in %d\r\n", clock() - tStart);
*/

	// Init NULLC
	nullcInit(MODULE_PATH);
	//nullcInitCustomAlloc(testAlloc, testDealloc, "Modules\\");
	//nullcSetFileReadHandler(TestFileLoad);

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

	/*
	//SpeedTestFile("test_document.nc");
	//SpeedTestFile("shapes.nc");
	//SpeedTestFile("raytrace.nc");
	//SpeedTestFile("blob.nc");
	return;*/

	RunInterfaceTests();

#ifdef NULLC_ENABLE_C_TRANSLATION
	nullres bRes = CompileFile("Modules/std/math.nc");
	assert(bRes);
	nullcTranslateToC("NULLC\\translation\\std_math.cpp", "__init_std_math_nc");

	bRes = CompileFile("Modules/std/typeinfo.nc");
	assert(bRes);
	nullcTranslateToC("NULLC\\translation\\std_typeinfo.cpp", "__init_std_typeinfo_nc");

	bRes = CompileFile("Modules/std/file.nc");
	assert(bRes);
	nullcTranslateToC("NULLC\\translation\\std_file.cpp", "__init_std_file_nc");

	bRes = CompileFile("Modules/std/vector.nc");
	assert(bRes);
	nullcTranslateToC("NULLC\\translation\\std_vector.cpp", "__init_std_vector_nc");

	bRes = nullcCompile("import std.math; float4 a; a.x = 2;");
	assert(bRes);
	nullcTranslateToC("test_a.cpp", "__init_test_a_nc");

	bRes = nullcCompile("char[] arr2 = \" world\";{ int r = 5; }");
	assert(bRes);
	nullcTranslateToC("test_importhide.cpp", "__init_test_importhide_nc");

	bRes = nullcCompile("int func(int a, b = 6){ return a * b; }");
	assert(bRes);
	nullcTranslateToC("test_defargs.cpp", "__init_test_defargs_nc");

	bRes = nullcCompile("int func(int a, b = 6){ return a * b; } int func(int d, c, a, b = 4){ return d * c + a + b; }");
	assert(bRes);
	nullcTranslateToC("test_defargs2.cpp", "__init_test_defargs2_nc");

	bRes = nullcCompile("class Test{ int func(int a, b = 6){ return a * b; } }");
	assert(bRes);
	nullcTranslateToC("test_defargs3.cpp", "__init_test_defargs3_nc");

	bRes = nullcCompile("class Test{ int func(int a, b = 6); }");
	assert(bRes);
	nullcTranslateToC("test_defargs4.cpp", "__init_test_defargs4_nc");

	bRes = nullcCompile("int foo(int x, char[] a = \"xx\", int y = 0){return x + a[0] + a[1];}");
	assert(bRes);
	nullcTranslateToC("test_defargs5.cpp", "__init_test_defargs5_nc");

	bRes = nullcCompile("int x = 5; int foo(auto ref a = &x){return int(a);}");
	assert(bRes);
	nullcTranslateToC("test_defargs6.cpp", "__init_test_defargs6_nc");

	bRes = nullcCompile("int CheckAlignment(auto ref ptr, int alignment);");
	assert(bRes);
	nullcTranslateToC("test_alignment.cpp", "__init_test_alignment_nc");

	bRes = CompileFile("Modules/std/list.nc");
	assert(bRes);
	nullcTranslateToC("NULLC\\translation\\std_list.cpp", "__init_std_list_nc");

	bRes = CompileFile("Modules/std/range.nc");
	assert(bRes);
	nullcTranslateToC("NULLC\\translation\\std_range.cpp", "__init_std_range_nc");

	bRes = CompileFile("Modules/std/gc.nc");
	assert(bRes);
	nullcTranslateToC("NULLC\\translation\\std_gc.cpp", "__init_std_gc_nc");

	bRes = CompileFile("Modules/std/dynamic.nc");
	assert(bRes);
	nullcTranslateToC("NULLC\\translation\\std_dynamic.cpp", "__init_std_dynamic_nc");

	bRes = CompileFile("Modules/std/io.nc");
	assert(bRes);
	nullcTranslateToC("NULLC\\translation\\std_io.cpp", "__init_std_io_nc");
#endif

	RunCompileFailTests();
	RunParseFailTests();

	TestQueue queue;
	queue.RunTests();

	// Conclusion
	printf("VM passed %d of %d tests\r\n", testsPassed[0], testsCount[0]);
#ifdef NULLC_BUILD_X86_JIT
	printf("X86 passed %d of %d tests\r\n", testsPassed[1], testsCount[1]);
#else
	testsPassed[1] = 0;
	testsCount[1] = 0;
#endif
	printf("Failure tests: passed %d of %d tests\r\n", testsPassed[2], testsCount[2]);
	printf("Extra tests: passed %d of %d tests\r\n", testsPassed[3], testsCount[3]);
#ifdef NULLC_ENABLE_C_TRANSLATION
	printf("Translation tests: passed %d of %d tests\r\n", testsPassed[4], testsCount[4]);
#endif
	printf("Passed %d of %d tests\r\n", testsPassed[0]+testsPassed[1]+testsPassed[2]+testsPassed[3], testsCount[0]+testsCount[1]+testsCount[2]+testsCount[3]);

	printf("Compilation time: %f\r\n", Tests::timeCompile);
	printf("Get listing time: %f\r\n", Tests::timeGetListing);
	printf("Get bytecode time: %f\r\n", Tests::timeGetBytecode);
	printf("Clean time: %f\r\n", Tests::timeClean);
	printf("Link time: %f\r\n", Tests::timeLinkCode);
	printf("Run time: %f\r\n", Tests::timeRun);

	RunSpeedTests();

	// Terminate NULLC
	nullcTerminate();
}
