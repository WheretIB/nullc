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

#pragma warning(disable: 4127)

//#define ALLOC_TOP_DOWN
//#define NO_CUSTOM_ALLOCATOR

void* testAlloc(int size)
{
#ifdef ALLOC_TOP_DOWN
	return VirtualAlloc(NULL, size + 128, MEM_COMMIT | MEM_TOP_DOWN, PAGE_READWRITE);
#else
	return (char*)malloc(size + 128) + 128;
#endif
}
void testDealloc(void* ptr)
{
	if(!ptr)
		return;
#ifdef ALLOC_TOP_DOWN
	VirtualFree((char*)ptr - 128, 0, MEM_RELEASE);
#else
	free((char*)ptr - 128);
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

void	RunTests(bool verbose, const void* (NCDECL *fileLoadFunc)(const char*, unsigned int*, int*))
{
	Tests::messageVerbose = verbose;
	Tests::fileLoadFunc = fileLoadFunc;

	// Extra tests

	// Safe sprintf test
	{
		testsCount[3]++;

		char buf[8];
		char *pos = buf + NULLC::SafeSprintf(buf, 8, "this ");
		pos += NULLC::SafeSprintf(pos, 8 - int(pos - buf), "string is too long");
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
		nullcInit("Modules/");
		nullcTerminate();
	}
	printf("Finished in %d\r\n", clock() - tStart);
*/

	// Init NULLC
#ifdef NO_CUSTOM_ALLOCATOR
	nullcInit(MODULE_PATH);
#else
	nullcInitCustomAlloc(testAlloc, testDealloc, MODULE_PATH);
#endif
	nullcSetFileReadHandler(Tests::fileLoadFunc);

	nullcInitTypeinfoModule();
	nullcInitDynamicModule();
	RunInterfaceTests();

#ifdef NO_CUSTOM_ALLOCATOR
	nullcInit(MODULE_PATH);
#else
	nullcInitCustomAlloc(testAlloc, testDealloc, MODULE_PATH);
#endif
	nullcSetFileReadHandler(Tests::fileLoadFunc);

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
	return;*/

#ifdef NULLC_ENABLE_C_TRANSLATION
	if(!CompileFile("Modules/std/math.nc"))
		printf("ERROR: failed to compile std.math for translation\n");
	nullcTranslateToC("NULLC/translation/std_math.cpp", "__init_std_math_nc");

	if(!CompileFile("Modules/std/typeinfo.nc"))
		printf("ERROR: failed to compile std.typeinfo for translation\n");
	nullcTranslateToC("NULLC/translation/std_typeinfo.cpp", "__init_std_typeinfo_nc");

	if(!CompileFile("Modules/std/file.nc"))
		printf("ERROR: failed to compile std.file for translation\n");
	nullcTranslateToC("NULLC/translation/std_file.cpp", "__init_std_file_nc");

	if(!CompileFile("Modules/std/vector.nc"))
		printf("ERROR: failed to compile std.vector for translation\n");
	nullcTranslateToC("NULLC/translation/std_vector.cpp", "__init_std_vector_nc");

	if(!nullcCompile("import std.math; float4 a; a.x = 2;"))
		printf("ERROR: failed to compile test_a for translation\n");
	nullcTranslateToC("tests/translation/test_a.cpp", "__init_test_a_nc");

	if(!nullcCompile("char[] arr2 = \" world\";{ int r = 5; }"))
		printf("ERROR: failed to compile test_importhide for translation\n");
	nullcTranslateToC("tests/translation/test_importhide.cpp", "__init_test_importhide_nc");

	if(!nullcCompile("int func(int a, b = 6){ return a * b; }"))
		printf("ERROR: failed to compile test_defargs for translation\n");
	nullcTranslateToC("tests/translation/test_defargs.cpp", "__init_test_defargs_nc");

	if(!nullcCompile("int func(int a, b = 6){ return a * b; } int func(int d, c, a, b = 4){ return d * c + a + b; }"))
		printf("ERROR: failed to compile test_defargs2 for translation\n");
	nullcTranslateToC("tests/translation/test_defargs2.cpp", "__init_test_defargs2_nc");

	if(!nullcCompile("class Test{ int func(int a, b = 6){ return a * b; } }"))
		printf("ERROR: failed to compile test_defargs3 for translation\n");
	nullcTranslateToC("tests/translation/test_defargs3.cpp", "__init_test_defargs3_nc");

	if(!nullcCompile("class Test{ int func(int a, b = 6); }"))
		printf("ERROR: failed to compile test_defargs4 for translation\n");
	nullcTranslateToC("tests/translation/test_defargs4.cpp", "__init_test_defargs4_nc");

	if(!nullcCompile("int foo(int x, char[] a = \"xx\", int y = 0){return x + a[0] + a[1];}"))
		printf("ERROR: failed to compile test_defargs5 for translation\n");
	nullcTranslateToC("tests/translation/test_defargs5.cpp", "__init_test_defargs5_nc");

	if(!nullcCompile("int x = 5; int foo(auto ref a = &x){return int(a);}"))
		printf("ERROR: failed to compile test_defargs6 for translation\n");
	nullcTranslateToC("tests/translation/test_defargs6.cpp", "__init_test_defargs6_nc");

	if(!nullcCompile("int CheckAlignment(auto ref ptr, int alignment);"))
		printf("ERROR: failed to compile test_alignment for translation\n");
	nullcTranslateToC("tests/translation/test_alignment.cpp", "__init_test_alignment_nc");

	if(!CompileFile("Modules/std/list.nc"))
		printf("ERROR: failed to compile std.list for translation\n");
	nullcTranslateToC("NULLC/translation/std_list.cpp", "__init_std_list_nc");

	if(!CompileFile("Modules/std/range.nc"))
		printf("ERROR: failed to compile std.range for translation\n");
	nullcTranslateToC("NULLC/translation/std_range.cpp", "__init_std_range_nc");

	if(!CompileFile("Modules/std/gc.nc"))
		printf("ERROR: failed to compile std.gc for translation\n");
	nullcTranslateToC("NULLC/translation/std_gc.cpp", "__init_std_gc_nc");

	if(!CompileFile("Modules/std/dynamic.nc"))
		printf("ERROR: failed to compile std.dynamic for translation\n");
	nullcTranslateToC("NULLC/translation/std_dynamic.cpp", "__init_std_dynamic_nc");

	if(!CompileFile("Modules/std/io.nc"))
		printf("ERROR: failed to compile std.io for translation\n");
	nullcTranslateToC("NULLC/translation/std_io.cpp", "__init_std_io_nc");

	if(!nullcCompile("coroutine int foo(){ int i = 10; while(i) yield i++; }"))
		printf("ERROR: failed to compile test_coroutine1 for translation\n");
	nullcTranslateToC("tests/translation/test_coroutine1.cpp", "__init_test_coroutine1_nc");

	if(!nullcCompile("import std.range; auto a = { for(i in range(4, 10)) yield i; };"))
		printf("ERROR: failed to compile test_list_comp1 for translation\n");
	nullcTranslateToC("tests/translation/test_list_comp1.cpp", "__init_test_list_comp1_nc");

	if(!nullcCompile("import std.range; auto b = { for(i in range(2, 4)) yield i; };"))
		printf("ERROR: failed to compile test_list_comp2 for translation\n");
	nullcTranslateToC("tests/translation/test_list_comp2.cpp", "__init_test_list_comp2_nc");

	if(!CompileFile("Modules/std/time.nc"))
		printf("ERROR: failed to compile std.time for translation\n");
	nullcTranslateToC("NULLC/translation/std_time.cpp", "__init_std_time_nc");

	if(!CompileFile("Modules/img/canvas.nc"))
		printf("ERROR: failed to compile img.canvas for translation\n");
	nullcTranslateToC("NULLC/translation/img_canvas.cpp", "__init_img_canvas_nc");

	if(!CompileFile("Modules/win/window_ex.nc"))
		printf("ERROR: failed to compile win.window_ex for translation\n");
	nullcTranslateToC("NULLC/translation/win_window_ex.cpp", "__init_win_window_ex_nc");

	if(!CompileFile("Modules/win/window.nc"))
		printf("ERROR: failed to compile win.window for translation\n");
	nullcTranslateToC("NULLC/translation/win_window.cpp", "__init_win_window_nc");

	if(!nullcCompile("class Proxy{ int foo(int x){ return x; } } int rc(auto ref z){ return z.foo(5); }"))
		printf("ERROR: failed to compile test_autorefcall for translation\n");
	nullcTranslateToC("tests/translation/test_autorefcall.cpp", "__init_test_autorefcall_nc");

	if(!nullcCompile("class Foo { typedef int T; }"))
		printf("ERROR: failed to compile test_class_typedef for translation\n");
	nullcTranslateToC("tests/translation/test_class_typedef.cpp", "__init_test_class_typedef_nc");
#endif

	RunCompileFailTests();
	RunParseFailTests();

#if defined(NULLC_ENABLE_C_TRANSLATION)
	#ifdef __linux
		system("cp NULLC/translation/runtime.h runtime.h");
		system("cp NULLC/translation/runtime.h tests/translation/runtime.h");
	#else
		_popen("copy \"NULLC\\translation\\runtime.h\" \"runtime.h\"", "r");
		_popen("copy \"NULLC\\translation\\runtime.h\" \"tests\\translation\\runtime.h\"", "r");
	#endif
#endif

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
#ifdef NULLC_LLVM_SUPPORT
	printf("LLVM passed %d of %d tests\r\n", testsPassed[2], testsCount[2]);
#else
	testsPassed[2] = 0;
	testsCount[2] = 0;
#endif
	printf("Failure tests: passed %d of %d tests\r\n", testsPassed[TEST_FAILURE_INDEX], testsCount[TEST_FAILURE_INDEX]);
	printf("Extra tests: passed %d of %d tests\r\n", testsPassed[TEST_EXTRA_INDEX], testsCount[TEST_EXTRA_INDEX]);
#ifdef NULLC_ENABLE_C_TRANSLATION
	printf("Translation tests: passed %d of %d tests\r\n", testsPassed[TEST_TRANSLATION_INDEX], testsCount[TEST_TRANSLATION_INDEX]);
#endif
	unsigned allTests = 0;
	unsigned allPassed = 0;
	for(unsigned i = 0; i < 6; i++)
	{
		allTests += testsCount[i];
		allPassed += testsPassed[i];
	}
	printf("Passed %d of %d tests\r\n", allPassed, allTests);

	printf("Compilation time: %f\r\n", Tests::timeCompile);
	printf("Get listing time: %f\r\n", Tests::timeGetListing);
	printf("Get bytecode time: %f\r\n", Tests::timeGetBytecode);
	printf("Clean time: %f\r\n", Tests::timeClean);
	printf("Link time: %f\r\n", Tests::timeLinkCode);
	printf("Run time: %f\r\n", Tests::timeRun);

	RunSpeedTests();

#if defined(NULLC_ENABLE_C_TRANSLATION)
	#ifdef __linux
		system("rm runtime.h");
		system("rm tests/translation/runtime.h");
	#else
		_popen("del \"runtime.h\"", "r");
		_popen("del \"tests\\translation\\runtime.h\"", "r");
	#endif
#endif

	// Terminate NULLC
	nullcTerminate();
}
