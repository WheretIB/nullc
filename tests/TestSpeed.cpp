#include "TestSpeed.h"
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

double speedTestTimeThreshold = 5000;	// how long, in ms, to run a speed test
#define RUN_GC_TESTS

void TestDrawRect(int, int, int, int, int)
{
}

void	SpeedTestText(const char* name, const char* text)
{
	nullcTerminate();
	nullcInit();
	nullcAddImportPath(MODULE_PATH_A);
	nullcAddImportPath(MODULE_PATH_B);
	nullcSetFileReadHandler(Tests::fileLoadFunc, Tests::fileFreeFunc);
	nullcSetEnableLogFiles(false, NULL, NULL, NULL);
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
	nullcInitTimeModule();
#if !defined(ANDROID)
	nullcInitPugiXMLModule();
#endif

	// exclusive for progressbar test
	nullcLoadModuleBySource("test.rect", "int draw_rect(int a, b, c, d, e);");
	nullcBindModuleFunctionHelper("test.rect", TestDrawRect, "draw_rect", 0);

#if defined(_MSC_VER)
	nullcInitWindowModule();
#endif

	nullcSetExecutor(NULLC_REG_VM);

	unsigned int runs = 0;
	double time = myGetPreciseTime();
	double compileTime = 0.0;
	double linkTime = 0.0;
	double coldStart = 0.0;
	while(linkTime < speedTestTimeThreshold)
	{
		runs++;
		nullres good = nullcCompile(text);
		compileTime += myGetPreciseTime() - time;

		if(good)
		{
			char *bytecode = NULL;
			nullcGetBytecode(&bytecode);
			nullcClean();
			if(!nullcLinkCode(bytecode))
				printf("Link failed: %s\r\n", nullcGetLastError());
			delete[] bytecode;
		}else{
			printf("Compilation failed: %s\r\n", nullcGetLastError());
			break;
		}
		linkTime += myGetPreciseTime() - time;
		time = myGetPreciseTime();
		if(runs == 1)
			coldStart = linkTime;
	}
	printf("Speed test (%s) managed to run %d times in %f ms\n", name, runs, linkTime);
	printf("Cold start: %f ms\n", coldStart);
	printf("Compile time: %f Link time: %f\n", compileTime, linkTime - compileTime);
	printf("Average compile time: %f Average link time: %f\n", compileTime / double(runs), (linkTime - compileTime) / double(runs));
	printf("Average time: %f Speed: %.3f Mb/sec\n\n", linkTime / double(runs), strlen(text) * (1000.0 / (linkTime / double(runs))) / 1024.0 / 1024.0);
}

void	SpeedTestFile(const char* file)
{
	char *blob = new char[1024 * 1024];
	FILE *euler = fopen(file, "rb");
	if(euler)
	{
		fseek(euler, 0, SEEK_END);
		unsigned int textSize = ftell(euler);
		assert(textSize < 1024 * 1024);
		fseek(euler, 0, SEEK_SET);
		fread(blob, 1, textSize, euler);
		blob[textSize] = 0;
		fclose(euler);

		SpeedTestText(file, blob);
	}

	delete[] blob;
}

void RunSpeedTests()
{
	#ifdef SPEED_TEST
#ifdef RUN_GC_TESTS
const char	*testGarbageCollection =
"import std.random;\r\n\
import std.io;\r\n\
import std.gc;\r\n\
int memStart = GC.UsedMemory();\r\n\
\r\n\
class A\r\n\
{\r\n\
    int a, b, c, d;\r\n\
    A ref ra, rb, rrt;\r\n\
}\r\n\
int count;\r\n\
long oldms;\r\n\
typedef A ref Aref;\r\n\
A ref[] arr;\r\n\
\r\n\
A ref Create(int level)\r\n\
{\r\n\
    if(level == 0)\r\n\
	{\r\n\
        return nullptr;\r\n\
    }else{\r\n\
        A ref a = new A;\r\n\
        arr[count] = a;\r\n\
        a.ra = Create(level - 1);\r\n\
        a.rb = Create(level - 1);\r\n\
        if (count > 0) {\r\n\
            a.rrt = arr[rand(count - 1)];\r\n\
        }\r\n\
        ++count;\r\n\
        return a;\r\n\
    }\r\n\
}\r\n\
double markTimeBegin = GC.MarkTime();\r\n\
double collectTimeBegin = GC.CollectTime();\r\n\
io.out << \"Started (\" << GC.UsedMemory() << \" bytes)\" << io.endl;\r\n\
int WS = 0;\r\n\
int ws = WS;\r\n"
#if defined(__CELLOS_LV2__)
"int d = 20;\r\n"
#else
"int d = 23;\r\n"
#endif
"arr = new Aref[1 << d];\r\n\
A ref a = Create(d);\r\n\
int minToCollect = (WS - ws) / 2;\r\n\
io.out << \"created \" << count << \" objects\" << io.endl;\r\n\
io.out << \"Used memory: (\" << GC.UsedMemory() << \" bytes)\" << io.endl;\r\n\
ws = WS;\r\n\
a = nullptr;\r\n\
arr = nullptr;\r\n\
GC.CollectMemory();\r\n\
io.out << \"destroyed \" << count << \" objects\" << io.endl;\r\n\
io.out << \"Used memory: (\" << GC.UsedMemory() << \" bytes)\" << io.endl;\r\n\
io.out << \"Marking time: (\" << GC.MarkTime() - markTimeBegin << \"sec) Collection time: \" << GC.CollectTime() - collectTimeBegin << \"sec)\" << io.endl;\r\n\
return GC.UsedMemory() - memStart;";

	printf("Garbage collection\r\n");
	for(int t = 0; t < TEST_TARGET_COUNT; t++)
	{
		if(!Tests::testExecutor[t])
			continue;

		testsCount[t]++;
		double tStart = myGetPreciseTime();
		if(Tests::RunCode(testGarbageCollection, testTarget[t], sizeof(void*) == 8 ? "400" : "296"))
			testsPassed[t]++;
		printf("%s finished in %f\r\n", testTarget[t] == NULLC_VM ? "VM" : "X86", myGetPreciseTime() - tStart);
	}

const char	*testGarbageCollection2 =
"import std.random;\r\n\
import std.io;\r\n\
import std.gc;\r\n\
int memStart = GC.UsedMemory();\r\n\
\r\n\
class A\r\n\
{\r\n\
    int a, b, c, d;\r\n\
    A ref ra, rb, rrt;\r\n\
	A ref[] rc;\r\n\
}\r\n\
int count;\r\n\
long oldms;\r\n\
typedef A ref Aref;\r\n\
A ref[] arr;\r\n\
\r\n\
A ref Create(int level)\r\n\
{\r\n\
    if(level == 0)\r\n\
	{\r\n\
        return nullptr;\r\n\
    }else{\r\n\
        A ref a = new A;\r\n\
        arr[count] = a;\r\n\
        a.ra = Create(level - 1);\r\n\
        a.rb = Create(level - 1);\r\n\
        if (count > 0) {\r\n\
            a.rrt = arr[rand(count - 1)];\r\n\
			a.rc = new A ref[2];\r\n\
			a.rc[0] = arr[rand(count - 1)];\r\n\
			a.rc[1] = arr[rand(count - 1)];\r\n\
        }\r\n\
        ++count;\r\n\
        return a;\r\n\
    }\r\n\
}\r\n\
double markTimeBegin = GC.MarkTime();\r\n\
double collectTimeBegin = GC.CollectTime();\r\n\
io.out << \"Started (\" << GC.UsedMemory() << \" bytes)\" << io.endl;\r\n\
int WS = 0;\r\n\
int ws = WS;\r\n"
#if defined(__CELLOS_LV2__)
"int d = 20;\r\n"
#else
"int d = 21;\r\n"
#endif
"arr = new Aref[1 << d];\r\n\
A ref a = Create(d);\r\n\
int minToCollect = (WS - ws) / 2;\r\n\
io.out << \"created \" << count << \" objects\" << io.endl;\r\n\
io.out << \"Used memory: (\" << GC.UsedMemory() << \" bytes)\" << io.endl;\r\n\
ws = WS;\r\n\
a = nullptr;\r\n\
arr = nullptr;\r\n\
GC.CollectMemory();\r\n\
io.out << \"destroyed \" << count << \" objects\" << io.endl;\r\n\
io.out << \"Used memory: (\" << GC.UsedMemory() << \" bytes)\" << io.endl;\r\n\
io.out << \"Marking time: (\" << GC.MarkTime() - markTimeBegin << \"sec) Collection time: \" << GC.CollectTime() - collectTimeBegin << \"sec)\" << io.endl;\r\n\
return GC.UsedMemory() - memStart;";

	printf("Garbage collection 2 \r\n");
	for(int t = 0; t < TEST_TARGET_COUNT; t++)
	{
		if(!Tests::testExecutor[t])
			continue;

		testsCount[t]++;
		double tStart = myGetPreciseTime();
		if(Tests::RunCode(testGarbageCollection2, testTarget[t], sizeof(void*) == 8 ? "400" : "296"))
			testsPassed[t]++;
		printf("%s finished in %f\r\n", testTarget[t] == NULLC_VM ? "VM" : "X86", myGetPreciseTime() - tStart);
	}

const char	*testGarbageCollection3 =
"import std.random;\r\n\
import std.io;\r\n\
import std.gc;\r\n\
import std.list;\r\n\
int memStart = GC.UsedMemory();\r\n\
\r\n\
list<int> arr;\r\n\
int count = 1 << 20;\r\n\
\r\n\
void Create()\r\n\
{\r\n\
   for(int i = 0; i < count; i++)\r\n\
      arr.push_back(i);\r\n\
}\r\n\
double markTimeBegin = GC.MarkTime();\r\n\
double collectTimeBegin = GC.CollectTime();\r\n\
io.out << \"Started (\" << GC.UsedMemory() << \" bytes)\" << io.endl;\r\n\
Create();\r\n\
GC.CollectMemory();\r\n\
io.out << \"created \" << count << \" objects\" << io.endl;\r\n\
io.out << \"Used memory: (\" << GC.UsedMemory() << \" bytes)\" << io.endl;\r\n\
arr.clear();\r\n\
GC.CollectMemory();\r\n\
io.out << \"destroyed \" << count << \" objects\" << io.endl;\r\n\
io.out << \"Used memory: (\" << GC.UsedMemory() << \" bytes)\" << io.endl;\r\n\
io.out << \"Marking time: (\" << GC.MarkTime() - markTimeBegin << \"sec) Collection time: \" << GC.CollectTime() - collectTimeBegin << \"sec)\" << io.endl;\r\n\
return GC.UsedMemory() - memStart;";

	printf("Garbage collection 3\r\n");
	for(int t = 0; t < TEST_TARGET_COUNT; t++)
	{
		if(!Tests::testExecutor[t])
			continue;

		testsCount[t]++;
		double tStart = myGetPreciseTime();
		if(Tests::RunCode(testGarbageCollection3, testTarget[t], sizeof(void*) == 8 ? "400" : "296"))
			testsPassed[t]++;
		printf("%s finished in %f\r\n", testTarget[t] == NULLC_VM ? "VM" : "X86", myGetPreciseTime() - tStart);
	}
#endif
#if defined(_MSC_VER)
	const char	*testCompileSpeed =
"import img.canvas;\r\n\
import win.window;\r\n\
import std.io;\r\n\
\r\n\
int width = 256;\r\n\
int height = 256;\r\n\
\r\n\
float[] a = new float[width*height];\r\n\
float[] b = new float[width*height];\r\n\
\r\n\
Canvas img = Canvas(width, height);\r\n\
\r\n\
float[] data = img.GetData();\r\n\
\r\n\
float get(float[] arr, int x, y){ return arr[x+y*width]; }\r\n\
void set(float[] arr, int x, y, float val){ arr[x+y*width] = val; }\r\n\
\r\n\
void process(float[] from, float[] to)\r\n\
{\r\n\
	auto damping = 0.01;\r\n\
	\r\n\
	for(auto x = 2; x < width - 2; x++)\r\n\
	{\r\n\
		for(auto y = 2; y < height - 2; y++)\r\n\
		{\r\n\
			double sum = get(from, x-2, y);\r\n\
			sum += get(from, x+2, y);\r\n\
			sum += get(from, x, y-2);\r\n\
			sum += get(from, x, y+2);\r\n\
			sum += get(from, x-1, y);\r\n\
			sum += get(from, x+1, y);\r\n\
			sum += get(from, x, y-1);\r\n\
			sum += get(from, x, y+1);\r\n\
			sum += get(from, x-1, y-1);\r\n\
			sum += get(from, x+1, y+1);\r\n\
			sum += get(from, x+1, y-1);\r\n\
			sum += get(from, x-1, y+1);\r\n\
			sum *= 1.0/6.0;\r\n\
			sum -= get(to, x, y);\r\n\
			\r\n\
			float val = sum - sum * damping;\r\n\
			val = val < 0.0 ? 0.0 : val;\r\n\
			val = val > 255.0 ? 255.0 : val;\r\n\
			set(to, x, y, val);\r\n\
		}\r\n\
	}\r\n\
}\r\n\
\r\n\
void render(float[] from, float[] to)\r\n\
{\r\n\
	float	rMin = 31 / 255.0, rMax = 168 / 255.0, \r\n\
			gMin = 57 / 255.0, gMax = 224 / 255.0, \r\n\
			bMin = 116 / 255.0, bMax = 237 / 255.0;\r\n\
	for(auto x = 2; x < width - 2; x++)\r\n\
	{\r\n\
		for(auto y = 2; y < height - 2; y++)\r\n\
		{\r\n\
			float color = get(from, x, y);\r\n\
\r\n\
			float progress = color / 255.0;\r\n\
\r\n\
			auto rDelta = (rMax - rMin) / 2.0;\r\n\
			auto rValue = (rMin + rDelta + rDelta * progress);\r\n\
			auto gDelta = (gMax - gMin) / 2.0;\r\n\
			auto gValue = (gMin + gDelta + gDelta * progress);\r\n\
			auto bDelta = (bMax - bMin) / 2.0;\r\n\
			auto bValue = (bMin + bDelta + bDelta * progress);\r\n\
\r\n\
			to[(x + y*width) * 4 + 0] = rValue;\r\n\
			to[(x + y*width) * 4 + 1] = gValue;\r\n\
			to[(x + y*width) * 4 + 2] = bValue;\r\n\
		}\r\n\
	}\r\n\
}\r\n\
\r\n\
float[] bufA = a, bufB = b, temp;\r\n\
\r\n\
Window main = Window(\"Test\", 400, 300, 260, 275);\r\n\
\r\n\
int seed = 10;\r\n\
int rand()\r\n\
{\r\n\
	seed = seed * 1103515245 + 12345;\r\n\
	return (seed / 65536) % 32768;\r\n\
}\r\n\
\r\n\
char[256] keys;\r\n\
do\r\n\
{\r\n\
	int randPosX = rand() % 200; randPosX = randPosX < 0 ? -randPosX : randPosX;\r\n\
	int randPosY = rand() % 200; randPosY = randPosY < 0 ? -randPosY : randPosY;\r\n\
	randPosX += 25;\r\n\
	randPosY += 25;\r\n\
	for(int x = randPosX-10; x < randPosX+10; x++)\r\n\
		for(int y = randPosY-10; y < randPosY+10; y++)\r\n\
			set(a, x, y, 255);\r\n\
\r\n\
	render(bufA, data);\r\n\
\r\n\
	main.DrawCanvas(&img, -1, -1);\r\n\
	\r\n\
	process(bufA, bufB);\r\n\
	temp = bufA;\r\n\
	bufA = bufB;\r\n\
	bufB = temp;\r\n\
\r\n\
	main.Update();\r\n\
	GetKeyboardState(keys);\r\n\
}while(!(keys[0x1B] & 0x80000000));\r\n\
main.Close();\r\n\
\r\n\
return 0;";

	SpeedTestText("ripples.nc inlined", testCompileSpeed);

#endif

const char	*testCompileSpeed2 =
"import test.rect;\r\n\
int progress_slider_position = 0;\r\n\
\r\n\
int clip(int value, int left, int right){	return value < left ? left : value > right ? right : value;}\r\n\
\r\n\
void draw_progress_text(int progress_x, int progress_y, int clip_left, int clip_right)\r\n\
{\r\n\
	// 60x8\r\n\
	auto pattern =\r\n\
	{\r\n\
		{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},\r\n\
		{0,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},\r\n\
		{0,1,0,0,1,0,1,1,0,0,1,1,0,0,1,1,0,0,1,0,0,0,1,0,1,1,0,0,1,0,0,0,1,0,0,0,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,0,1,1,0,0,1,1,0,0},\r\n\
		{0,1,0,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,0,0,0,0,1,0,1,0,1,1,0,0,1,1,0,0,0,0,1,0,1,0,1,0,1,0,1,0,1,0,0,0,1,0,1,0,1,0,1,0},\r\n\
		{0,1,0,0,1,0,1,1,1,0,1,1,1,0,1,0,1,0,1,0,0,0,1,0,1,0,1,0,1,0,1,0,1,0,0,0,0,0,1,1,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,1,1,0},\r\n\
		{0,1,0,0,1,0,1,0,0,0,1,0,0,0,1,1,1,0,1,0,0,0,1,0,1,1,1,0,1,0,1,0,1,0,0,0,0,0,1,0,0,0,1,0,1,0,1,1,1,0,1,0,1,0,1,0,1,0,0,0},\r\n\
		{0,1,1,1,1,0,1,1,1,0,1,1,1,0,1,0,0,0,1,1,1,0,1,0,0,0,1,0,1,0,1,0,1,1,0,0,0,0,1,1,1,0,1,0,1,0,0,0,1,0,1,0,1,0,1,0,1,1,1,0},\r\n\
		{0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,0,0,0,0,0,0,0,0,0,0,0}\r\n\
	};\r\n\
\r\n\
	auto pattern_x_offset = 80;\r\n\
	auto pattern_cell_width = 8;\r\n\
	auto pattern_cell_height = 8;\r\n\
	auto pattern_color = 0xff202020;\r\n\
\r\n\
	for (auto y = 0; y < 8; ++y)\r\n\
	{\r\n\
		for (auto x = 0; x < 60; ++x)\r\n\
		{\r\n\
			if (!pattern[y][x]) continue;\r\n\
\r\n\
			int left = progress_x + pattern_x_offset + pattern_cell_width * x;\r\n\
			int right = left + pattern_cell_width;\r\n\
\r\n\
			int clipped_left = clip(left, clip_left, clip_right);\r\n\
			int clipped_right = clip(right, clip_left, clip_right);\r\n\
\r\n\
			draw_rect(clipped_left, progress_y + pattern_cell_height * y, clipped_right - clipped_left, pattern_cell_height, pattern_color);\r\n\
		}\r\n\
	}\r\n\
}\r\n\
\r\n\
void draw_progress_bar_part(int progress_x, int progress_y, int x, int y, int width, int height)\r\n\
{\r\n\
	draw_rect(x, y, width, height, 0xffffffff);\r\n\
	draw_progress_text(progress_x, progress_y, x, x + width);\r\n\
}\r\n\
\r\n\
int draw_progress_bar()\r\n\
{\r\n\
	auto progress_width = 640;\r\n\
	auto progress_height = 64;\r\n\
	auto progress_margin = 8;\r\n\
	auto progress_slider_width = 128;\r\n\
	auto progress_slider_speed = 8;\r\n\
\r\n\
	// progress bar background\r\n\
	auto progress_x = 640 - progress_width/2;\r\n\
	auto progress_y = 360 - progress_height/2;\r\n\
\r\n\
	draw_rect(progress_x - progress_margin, progress_y - progress_margin, progress_width + progress_margin * 2, progress_height + progress_margin * 2, 0xff808080);\r\n\
\r\n\
	// progress slider\r\n\
	auto progress_slider_left = progress_slider_position % progress_width;\r\n\
	auto progress_slider_right = (progress_slider_position + progress_slider_width) % progress_width;\r\n\
\r\n\
	if (progress_slider_left < progress_slider_right)\r\n\
	{\r\n\
		draw_progress_bar_part(progress_x, progress_y, progress_x + progress_slider_left, progress_y, progress_slider_right - progress_slider_left, progress_height);\r\n\
	}\r\n\
	else\r\n\
	{\r\n\
		draw_progress_bar_part(progress_x, progress_y, progress_x, progress_y, progress_slider_right, progress_height);\r\n\
		draw_progress_bar_part(progress_x, progress_y, progress_x + progress_slider_left, progress_y, progress_width - progress_slider_left, progress_height);\r\n\
	}\r\n\
\r\n\
	progress_slider_position += progress_slider_speed;\r\n\
\r\n\
	return 0;\r\n\
}\r\n\
for(int i = 0; i < 10000; i++)\r\n\
	draw_progress_bar();\r\n\
return 0;";

	SpeedTestText("progressbar.nc inlined", testCompileSpeed2);

	for(int t = 0; t < TEST_TARGET_COUNT; t++)
	{
		if(!Tests::testExecutor[t])
			continue;

		testsCount[t]++;
		double tStart = myGetPreciseTime();
		if(Tests::RunCode(testCompileSpeed2, testTarget[t], "0"))
			testsPassed[t]++;
		printf("%s finished in %f (single run is %f)\r\n", testTarget[t] == NULLC_X86 ? "X86" : (testTarget[t] == NULLC_LLVM ? "LLVM" : "REGVM"), myGetPreciseTime() - tStart, (myGetPreciseTime() - tStart) / 10000.0);
	}

#if defined(_MSC_VER)
const char	*testCompileSpeed3 =
"import img.canvas;\r\n\
import win.window;\r\n\
import std.time;\r\n\
import std.io;\r\n\
import std.math;\r\n\
\r\n\
double time = clock();\r\n\
\r\n\
// parameters, you can modify them.\r\n\
int size = 300;\r\n\
double ambient = 0.1;\r\n\
double diffusion = 0.7;\r\n\
double specular = 0.6;\r\n\
double power = 12;\r\n\
\r\n\
int width = size;\r\n\
int height = size;\r\n\
\r\n\
Window main = Window(\"Raytrace example\", 400, 300, width + 4, height + 20);\r\n\
\r\n\
class Material\r\n\
{\r\n\
    int[513] lmap;\r\n\
    void Material(int col, double amb, double dif)\r\n\
	{\r\n\
        int i, r = col >> 16, g = (col >> 8) & 255, b = col & 255;\r\n\
		double a;\r\n\
        for(i = 0; i < 512; i++)\r\n\
		{\r\n\
            a = ((i < 256) ? amb : ((((i-256) * (dif - amb)) * 0.00390625) + amb)) * 2;\r\n\
            if (a<1) lmap[i] = (int(r*a)<<16)|(int(g*a)<<8)|(int(b*a)<<0);\r\n\
            else lmap[i] = (int(255-(255-r)*(2-a))<<16)|(int(255-(255-g)*(2-a))<<8)|(int((255-(255-b)*(2-a)))<<0);\r\n\
        }\r\n\
    }\r\n\
}\r\n\
\r\n\
class Sphere\r\n\
{\r\n\
    double x, y, z, r2;\r\n\
	Material ref mat;\r\n\
    double cx, cy, cz, cr, omg, pha;\r\n\
    void Sphere(double cx, cy, cz, cr, omg, pha, r, Material ref mat)\r\n\
	{\r\n\
        this.cx = cx;\r\n\
        this.cy = cy;\r\n\
        this.cz = cz;\r\n\
        this.cr = cr;\r\n\
        this.omg = omg;\r\n\
        this.pha = pha;\r\n\
        this.r2 = r * r;\r\n\
        this.mat = mat;\r\n\
    }\r\n\
    void update()\r\n\
	{\r\n\
        double ang = time * omg + pha;\r\n\
        x = cos(ang)*cr+cx;\r\n\
        y = cy;\r\n\
        z = sin(ang)*cr+cz;\r\n\
    }\r\n\
}\r\n\
\r\n\
class SphereNode\r\n\
{\r\n\
	Sphere ref sphere;\r\n\
	SphereNode ref next;\r\n\
}\r\n\
auto SphereNode:init(Sphere ref sphere)\r\n\
{\r\n\
	this.sphere = sphere;\r\n\
	return this;\r\n\
}\r\n\
auto SphereNode:add(Sphere ref sphere)\r\n\
{\r\n\
	next = new SphereNode;\r\n\
	next.init(sphere);\r\n\
	return next;\r\n\
}\r\n\
\r\n\
double focusZ = size;\r\n\
double floorY = 100;\r\n\
Canvas screen = Canvas(width, height);\r\n\
double[] initialDir = new double[size*size*3];\r\n\
SphereNode ref firstSphere, lastSphere;\r\n\
float3 ldir = float3(0, 0, 0);\r\n\
float3 light = float3(100,100,100);\r\n\
int[513] smap;\r\n\
int[1024] fcol;\r\n\
auto refs = {0, 1, 2, 3, 3};\r\n\
auto reff = {0xffffff, 0x7f7f7f, 0x3f3f3f, 0x1f1f1f, 0x1f1f1f};\r\n\
double mx, my, camX, camY, camZ, tcamX, tcamY, tcamZ;\r\n\
float[] pixels = screen.GetData();\r\n\
\r\n\
void setup()\r\n\
{\r\n\
    int i, j, idx;\r\n\
	double l;\r\n\
	int s;\r\n\
	double hs = (size - 1) * 0.5;\r\n\
    for({j=0;idx=0;}; j<size; j++)\r\n\
	{\r\n\
		for (i=0; i<size; i++)\r\n\
		{\r\n\
	        l = 1/sqrt((i - hs)*(i - hs) + (j - hs)*(j - hs) + focusZ*focusZ);\r\n\
	        initialDir[idx] = (i-hs) * l; idx++;\r\n\
	        initialDir[idx] = (j-hs) * l; idx++;\r\n\
	        initialDir[idx] = focusZ   * l; idx++;\r\n\
	    }\r\n\
	}\r\n\
    for (i=0; i<512; i++) {\r\n\
        s = (i<256) ? 64 : int((((i-256) * 0.00390625) ** power) * (power + 2) * specular * 0.15915494309189534 * 192 + 64);\r\n\
        if (s > 255) s = 255;\r\n\
        smap[i] = 0x10101 * s;\r\n\
        s = (i<256) ? (255-i) : (i-256);\r\n\
        fcol[i] = 0x10101 * (s - (s>>3) + 31);\r\n\
        fcol[i+512] = 0x10101 * ((s>>2) - (s>>5) + 31);\r\n\
    }\r\n\
	firstSphere = new SphereNode;\r\n\
	firstSphere.init(new Sphere(100, 40, 600, 200, 0.3, 1.0, 60, new Material(0x8080ff,ambient,diffusion)))\r\n\
	.add(new Sphere(  0, 50, 300, 100, 0.8, 0.8, 50, new Material(0x80ff80,ambient,diffusion)))\r\n\
	.add(new Sphere( 50, 60, 200, 200, 0.6, 2.0, 40, new Material(0xff8080,ambient,diffusion)))\r\n\
	.add(new Sphere(-50, 70, 500, 300, 0.4, 1.4, 30, new Material(0xc0c080,ambient,diffusion)))\r\n\
	.add(new Sphere(-90, 30, 600, 400, 0.2, 1.5, 70, new Material(0xc080c0,ambient,diffusion)))\r\n\
	.add(new Sphere( 70, 80, 400, 100, 0.7, 1.2, 20, new Material(0x80c0c0,ambient,diffusion)));\r\n\
	\r\n\
    camX = camY = camZ = tcamX = tcamY = tcamZ = 0;\r\n\
}\r\n\
\r\n\
void draw()\r\n\
{\r\n\
    int i, j, k, l, idx;\r\n\
	double t, tmin, n;\r\n\
	Sphere ref s;\r\n\
    int    ln, pixel;\r\n\
	Sphere ref hit;\r\n\
	int a, kmax;\r\n\
   double     ox, oy, oz, dx, dy, dz, nx, ny, nz, dsx, dsy, dsz, B, C, D;\r\n\
    light.x = cos(time*0.6)*100;\r\n\
    light.y = sin(time*1.1)*25+100;\r\n\
    light.z = sin(time*0.9)*100-100;\r\n\
    ldir.x = -light.x;\r\n\
    ldir.y = -light.y;\r\n\
    ldir.z = -light.z;\r\n\
    ldir.normalize();\r\n\
    \r\n\
    tcamX = mx * 400;\r\n\
    tcamY = my * 150 - 50;\r\n\
    tcamZ = my * 400 - 200;\r\n\
    camX += (tcamX - camX) * 0.02;\r\n\
    camY += (tcamY - camY) * 0.02;\r\n\
    camZ += (tcamZ - camZ) * 0.02;\r\n\
    \r\n\
    auto sphereNode = firstSphere;\r\n\
    while ( sphereNode ) {\r\n\
    		sphereNode.sphere.update();\r\n\
    		sphereNode = sphereNode.next;\r\n\
    	}\r\n\
    \r\n\
    int pos = 0;\r\n\
    for ({j=0;idx=0;}; j<size; ++j) {\r\n\
        for (i=0; i<size; ++i) {\r\n\
            ox = camX;\r\n\
            oy = camY;\r\n\
            oz = camZ;\r\n\
            dx = initialDir[idx]; idx++;\r\n\
            dy = initialDir[idx]; idx++;\r\n\
            dz = initialDir[idx]; idx++;\r\n\
            \r\n\
            pixel = 0;\r\n\
            for (l=1; l<5; l++) {\r\n\
                tmin = 99999;\r\n\
                hit = nullptr;\r\n\
                sphereNode = firstSphere;\r\n\
                while( sphereNode ) {\r\n\
                    s = sphereNode.sphere;\r\n\
                    dsx = ox - s.x;\r\n\
                    dsy = oy - s.y;\r\n\
                    dsz = oz - s.z;\r\n\
                    B = dsx * dx + dsy * dy + dsz * dz;\r\n\
                    C = dsx * dsx + dsy * dsy + dsz * dsz - s.r2;\r\n\
                    D = B * B - C;\r\n\
                    if (D > 0) {\r\n\
                        t = - B - sqrt(D);\r\n\
                        if ((t > 0) && (t < tmin)) {\r\n\
                            tmin = t;\r\n\
                            hit = s;\r\n\
                        }\r\n\
                    }\r\n\
                    sphereNode = sphereNode.next;\r\n\
                }\r\n\
\r\n\
                if (hit) {\r\n\
                    ox += dx * tmin;\r\n\
                    oy += dy * tmin;\r\n\
                    oz += dz * tmin;\r\n\
                    nx = ox - hit.x;\r\n\
                    ny = oy - hit.y;\r\n\
                    nz = oz - hit.z;\r\n\
                    n = 1 / sqrt(nx*nx + ny*ny + nz*nz);\r\n\
                    nx *= n;\r\n\
                    ny *= n;\r\n\
                    nz *= n;\r\n\
                    n = -(nx*dx + ny*dy + nz*dz) * 2;\r\n\
                    dx += nx * n;\r\n\
                    dy += ny * n;\r\n\
                    dz += nz * n;\r\n\
                    ln = int((ldir.x * nx + ldir.y * ny + ldir.z * nz) * 255) + 256;\r\n\
                    a = hit.mat.lmap[ln];\r\n\
                    a = a >> refs[l];\r\n\
                    a = a & reff[l];\r\n\
                    pixel += a;\r\n\
                } else {\r\n\
                    if (dy < 0) {\r\n\
                        ln = int((ldir.x * dx + ldir.y * dy + ldir.z * dz) * 255) + 256;\r\n\
                        a = smap[ln];\r\n\
                        ln = l - 1;\r\n\
                        a = a >> refs[ln];\r\n\
                        a = a & reff[ln];\r\n\
                        pixel += a;\r\n\
                        break;\r\n\
                    } else {\r\n\
                        tmin = (floorY-oy)/dy;\r\n\
                        ox += dx * tmin;\r\n\
                        oy += dy * tmin;\r\n\
                        oz += dz * tmin;\r\n\
                        dy = -dy;\r\n\
                        ln = dy * 256 + ((((int(ox+oz)>>7)+(int(ox-oz)>>7))&1)<<9) + 256;\r\n\
                        a = fcol[ln];\r\n\
                        a = a >> refs[l];\r\n\
                        a = a & reff[l];\r\n\
                        pixel += a;\r\n\
                    }\r\n\
                }\r\n\
            }\r\n\
            \r\n\
			pixels[pos * 4 + 0] = ((pixel >> 16) & 0xff) / 255.0;\r\n\
			pixels[pos * 4 + 1] = ((pixel >> 8) & 0xff) / 255.0;\r\n\
			pixels[pos * 4 + 2] = ((pixel) & 0xff) / 255.0;\r\n\
            ++pos;\r\n\
        }\r\n\
    }\r\n\
}\r\n\
\r\n\
setup();\r\n\
\r\n\
double lastTime = clock();\r\n\
\r\n\
char[256] keys;\r\n\
do\r\n\
{\r\n\
	// Draw\r\n\
	int mouseX, mouseY;\r\n\
	GetMouseState(mouseX, mouseY);\r\n\
	mx = (mouseX - 232.5) * 0.00390625;\r\n\
    my = (232.5 - mouseY) * 0.00390625;\r\n\
	time = clock() * 0.001;\r\n\
	draw();\r\n\
\r\n\
	main.DrawCanvas(&screen, -1, -1);\r\n\
	\r\n\
	double time = clock() - lastTime;\r\n\
	lastTime = clock();\r\n\
	int fps = 1000.0 / time;\r\n\
	main.SetTitle(\"FPS is \" + fps.str());\r\n\
\r\n\
	main.Update();\r\n\
	GetKeyboardState(keys);\r\n\
}while(!(keys[0x1B] & 0x80000000));	// While Escape is not pressed\r\n\
main.Close();\r\n\
\r\n\
return 0;";

	SpeedTestText("raytrace.nc inlined", testCompileSpeed3);
#endif

	speedTestTimeThreshold = 1000;
	char *tmp = new char[64*1024];

	const char *testCompileSpeed456 = "(((a + b) * a - a / b) * ((a + b) * a - a / b) + a + a + -b) + (((a + b) * a - a / b) * ((a + b) * a - a / b) + a + a + -b) + (((a + b) * a - a / b) * ((a + b) * a - a / b) + a + a + -b),\r\n";
	const char *testCompileSpeed456E = 
"	1\r\n\
};\r\n\
return arr.size;";

	const char *testCompileSpeed4S = 
"class complex<T>{ T re, im; }\r\n\
auto complex(generic re, im){ complex<typeof(re)> res; res.re = re; res.im = im; return res; }\r\n\
auto operator + (complex<generic> a, b){ return complex(a.re + b.re, a.im + b.im); }\r\n\
auto operator - (complex<generic> a, b){ return complex(a.re - b.re, a.im - b.im); }\r\n\
auto operator * (complex<generic> a, b){ return complex(a.re * b.re - a.im * b.im, a.im * b.re + a.re * b.im); }\r\n\
auto operator / (complex<generic> a, b){ double magn = b.re * b.re + b.im * b.im; return complex(typeof(a.re)((a.re * b.re + a.im * b.im) / magn), (a.im * b.re - a.re * b.im) / magn); }\r\n\
// Test how complex generic specialized operators slow down typical arithmetic operatior compilation\r\n\
double a, b;\r\n\
auto arr = \r\n\
{\r\n";
	strcpy(tmp, testCompileSpeed4S);
	for(unsigned i = 0; i < 32; i++)
		strcat(tmp, testCompileSpeed456);
	strcat(tmp, testCompileSpeed456E);
	SpeedTestText("generic operators slowdown (generic class)", tmp);

	const char *testCompileSpeed5S = 
"class complex{ double re, im; }\r\n\
auto complex(generic re, im){ complex res; res.re = re; res.im = im; return res; }\r\n\
auto operator + (complex a, b){ return complex(a.re + b.re, a.im + b.im); }\r\n\
auto operator - (complex a, b){ return complex(a.re - b.re, a.im - b.im); }\r\n\
auto operator * (complex a, b){ return complex(a.re * b.re - a.im * b.im, a.im * b.re + a.re * b.im); }\r\n\
auto operator / (complex a, b){ double magn = b.re * b.re + b.im * b.im; return complex(typeof(a.re)((a.re * b.re + a.im * b.im) / magn), (a.im * b.re - a.re * b.im) / magn); }\r\n\
double a, b;\r\n\
auto arr = \r\n\
{\r\n";
	strcpy(tmp, testCompileSpeed5S);
	for(unsigned i = 0; i < 32; i++)
		strcat(tmp, testCompileSpeed456);
	strcat(tmp, testCompileSpeed456E);
	SpeedTestText("generic operators slowdown (class)", tmp);

	const char *testCompileSpeed6S = 
"double a, b;\r\n\
auto arr = \r\n\
{\r\n";
	strcpy(tmp, testCompileSpeed6S);
	for(unsigned i = 0; i < 32; i++)
		strcat(tmp, testCompileSpeed456);
	strcat(tmp, testCompileSpeed456E);
	SpeedTestText("generic operators slowdown (none)", tmp);

	delete[] tmp;

#endif

#ifdef SPEED_TEST_EXTRA
	SpeedTestFile("blob.nc");

	SpeedTestFile("test_document.nc");

	SpeedTestFile("shapes.nc");

	SpeedTestFile("functional.nc");
#endif
}
