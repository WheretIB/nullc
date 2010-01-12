#include "stdafx.h"
#include "UnitTests.h"
#include "NULLC/nullc.h"
#include "NULLC/ParseClass.h"

#include "Modules/includes/file.h"
#include "Modules/includes/math.h"

#include <stdio.h>

#ifndef _DEBUG
	#define FAILURE_TEST
	#define SPEED_TEST
#endif

double timeCompile;
double timeGetListing;
double timeGetBytecode;
double timeClean;
double timeLinkCode;
double timeRun;

char *varData = NULL;
unsigned int varCount = 0;
VariableInfo **varInfo = NULL;

bool lastFailed;

#define CHECK_DOUBLE(var, index, expected) if(fabs(((double*)FindVar(var))[index] - (expected)) > 1e-6){ printf(" Failed %s[%d] == %f (got %f)\r\n", #var, index, expected, ((double*)FindVar(var))[index]); lastFailed = true; }
#define CHECK_FLOAT(var, index, expected) if(((float*)FindVar(var))[index] != (expected)){ printf(" Failed %s[%d] == %f (got %f)\r\n", #var, index, expected, ((float*)FindVar(var))[index]); lastFailed = true; }
#define CHECK_LONG(var, index, expected) if(((long long*)FindVar(var))[index] != (expected)){ printf(" Failed %s[%d] == %I64d (got %I64d)\r\n", #var, index, expected, ((long long*)FindVar(var))[index]); lastFailed = true; }
#define CHECK_INT(var, index, expected) if(((int*)FindVar(var))[index] != (expected)){ printf(" Failed %s[%d] == %d (got %d)\r\n", #var, index, expected, ((int*)FindVar(var))[index]); lastFailed = true; }
#define CHECK_SHORT(var, index, expected) if(((short*)FindVar(var))[index] != (expected)){ printf(" Failed %s[%d] == %d (got %d)\r\n", #var, index, expected, ((short*)FindVar(var))[index]); lastFailed = true; }
#define CHECK_CHAR(var, index, expected) if(((char*)FindVar(var))[index] != (expected)){ printf(" Failed %s[%d] == %d (got %d)\r\n", #var, index, expected, ((char*)FindVar(var))[index]); lastFailed = true; }
#define CHECK_STR(var, index, expected) if(strcmp(((char*)FindVar(var)+index), (expected)) != 0){ printf(" Failed %s[%d] == %s (got %s)\r\n", #var, index, expected, ((char*)FindVar(var))+index); lastFailed = true; }

void*	FindVar(const char* name)
{
	unsigned int addressShift = 0;
	for(unsigned int i = 0; i < varCount; i++)
	{
		VariableInfo &currVar = *(*(varInfo+i));
		if(currVar.pos >> 24)
			addressShift += currVar.varType->size;
		if(strlen(name) == (unsigned int)(currVar.name.end - currVar.name.begin) && memcmp(currVar.name.begin, name, currVar.name.end - currVar.name.begin) == 0)
			return (void*)(varData + addressShift + currVar.pos);
	}
	return varData;
}

bool	RunCode(const char *code, unsigned int executor, const char* expected)
{
	//if(executor != NULLC_VM)
	//	return false;
	nullcSetExecutor(executor);

	char buf[256];
	sprintf(buf, "%s", executor == NULLC_VM ? "VM " : "X86");

	double time = myGetPreciseTime();
	nullres good = nullcCompile(code);
	timeCompile += myGetPreciseTime() - time;

	time = myGetPreciseTime();
	nullcSaveListing("asm.txt");
	timeGetListing += myGetPreciseTime() - time;

	if(!good)
	{
		printf("%s Compilation failed: %s", buf, nullcGetCompilationError());
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
		int linkgood = nullcLinkCode(bytecode, 1);
		timeLinkCode += myGetPreciseTime() - time;
		delete[] bytecode;

		if(!linkgood)
		{
			printf("%s Link failed: %s\r\n", buf, nullcGetCompilationError());
			return false;
		}

		varData = (char*)nullcGetVariableData();

		time = myGetPreciseTime();
		nullres goodRun = nullcRun();
		timeRun += myGetPreciseTime() - time;
		if(goodRun)
		{
			const char* val = nullcGetResult();
			varData = (char*)nullcGetVariableData();

			if(expected && strcmp(val, expected) != 0)
			{
				printf("%s Failed (%s != %s)\r\n", buf, val, expected);
				return false;
			}
		}else{
			printf("%s Execution failed: %s\r\n", buf, nullcGetRuntimeError());
			return false;
		}
	}
	varCount = 0;
	varInfo = (VariableInfo**)nullcGetVariableInfo(&varCount);
	return true;
}

const char *testIntOp =
"// Integer tests\r\n\
int[33] res;\r\n\
int a = 14, b = 3, c = 0;\r\n\
res[0] = a+b; // 17\r\n\
res[1] = a-b; // 11\r\n\
res[2] = -a; // -14\r\n\
res[3] = ~b; // -4\r\n\
res[4] = a*b; // 42\r\n\
res[5] = a/b; // 4\r\n\
res[6] = a%b; // 2\r\n\
res[7] = a**b; // 2744\r\n\
res[8] = a > b; // 1\r\n\
res[9] = a < b; // 0\r\n\
res[10] = a >= b; // 1\r\n\
res[11] = a <= b; // 0\r\n\
res[12] = a == b; // 0\r\n\
res[13] = a != b; // 1\r\n\
res[14] = a << b; // 112\r\n\
res[15] = a >> b; // 1\r\n\
res[16] = a & b; // 2\r\n\
res[17] = a | b; // 15\r\n\
res[18] = a ^ b; // 13\r\n\
int o = 0, i = 1;\r\n\
res[19] = o && o;\r\n\
res[20] = o && i;\r\n\
res[21] = i && o;\r\n\
res[22] = i && i;\r\n\
res[23] = o || o;\r\n\
res[24] = o || i;\r\n\
res[25] = i || o;\r\n\
res[26] = i || i;\r\n\
res[27] = o ^^ o;\r\n\
res[28] = o ^^ i;\r\n\
res[29] = i ^^ o;\r\n\
res[30] = i ^^ i;\r\n\
res[31] = !i; // 0\r\n\
res[32] = !o; // 1\r\n\
return a >> b;";

const char	*testDoubleOp = 
"// Floating point tests\r\n\
double[15] res;\r\n\
double a = 14.0, b = 3.0;\r\n\
res[0] = a+b;\r\n\
res[1] = a-b;\r\n\
res[2] = -a;\r\n\
res[3] = a*b;\r\n\
res[4] = a/b;\r\n\
res[5] = a%b;\r\n\
res[6] = a**b;\r\n\
res[7] = a > b;\r\n\
res[8] = a < b;\r\n\
res[9] = a >= b;\r\n\
res[10] = a <= b;\r\n\
res[11] = a == b;\r\n\
res[12] = a != b;\r\n\
return a+b;";

const char	*testLongOp = 
"// Long tests\r\n\
long[34] res;\r\n\
long a = 4494967296l, b = 594967296l, c = 3;\r\n\
res[0] = a+b; // 5089934592\r\n\
res[1] = a-b; // 3900000000\r\n\
res[2] = -a; // -4494967296\r\n\
res[3] = ~a; // -4494967297\r\n\
res[4] = a*b; // 2674358537709551616\r\n\
res[5] = a/b; // 7\r\n\
res[6] = a%b; // 330196224\r\n\
res[7] = 594967**c; // 210609828468829063\r\n\
res[8] = a > b; // 1\r\n\
res[9] = a < b; // 0\r\n\
res[10] = a >= b; // 1\r\n\
res[11] = a <= b; // 0\r\n\
res[12] = a == b; // 0\r\n\
res[13] = a != b; // 1\r\n\
res[14] = a << c; // 35959738368 \r\n\
res[15] = c << 45; // 105553116266496 \r\n\
res[16] = a >> c; // 561870912\r\n\
res[17] = a & b; // 56771072\r\n\
res[18] = a | b; // 5033163520\r\n\
res[19] = a ^ b; // 4976392448\r\n\
long o = 0, i = 1;\r\n\
res[20] = o && o;\r\n\
res[21] = o && i;\r\n\
res[22] = i && o;\r\n\
res[23] = i && i;\r\n\
res[24] = o || o;\r\n\
res[25] = o || i;\r\n\
res[26] = i || o;\r\n\
res[27] = i || i;\r\n\
res[28] = o ^^ o;\r\n\
res[29] = o ^^ i;\r\n\
res[30] = i ^^ o;\r\n\
res[31] = i ^^ i;\r\n\
res[32] = !i; // 0\r\n\
res[33] = !o; // 1\r\n\
\r\n\
return 1;";

const char	*tesIncDec =
"// Decrement and increment tests for all types\r\n\
double a1=5, b1=5, c1, d1, e1, f1;\r\n\
float a2=5, b2=5, c2, d2, e2, f2;\r\n\
long a3=5, b3=5, c3, d3, e3, f3;\r\n\
int a4=5, b4=5, c4, d4, e4, f4;\r\n\
short a5=5, b5=5, c5, d5, e5, f5;\r\n\
char a6=5, b6=5, c6, d6, e6, f6;\r\n\
c1 = a1++; a1++; e1 = ++a1; ++a1;\r\n\
d1 = b1--; b1--; f1 = --b1; --b1;\r\n\
\r\n\
c2 = a2++; a2++; e2 = ++a2; ++a2;\r\n\
d2 = b2--; b2--; f2 = --b2; --b2;\r\n\
\r\n\
c3 = a3++; a3++; e3 = ++a3; ++a3;\r\n\
d3 = b3--; b3--; f3 = --b3; --b3;\r\n\
\r\n\
c4 = a4++; a4++; e4 = ++a4; ++a4;\r\n\
d4 = b4--; b4--; f4 = --b4; --b4;\r\n\
\r\n\
c5 = a5++; a5++; e5 = ++a5; ++a5;\r\n\
d5 = b5--; b5--; f5 = --b5; --b5;\r\n\
\r\n\
c6 = a6++; a6++; e6 = ++a6; ++a6;\r\n\
d6 = b6--; b6--; f6 = --b6; --b6;\r\n\
return 1;";

const char	*testCmplxType1 = 
"// Complex type test (simple)\r\n\
import std.math;\r\n\
float f1;\r\n\
float2 f2;\r\n\
float3 f3;\r\n\
float4 f4;\r\n\
f1 = 1; // f1 = 1.0\r\n\
f2.x = 2.0; // f2.x = 2.0\r\n\
f2.y = 3l; // f2.y = 3.0\r\n\
f3.x = f2.y; // f3.x = 3.0\r\n\
f3.y = 4.0f; // f3.y = 4.0\r\n\
f3.z = f1*f3.x; // f3.z = 3.0\r\n\
f4.x = f3.y; // f4.x = 4.0\r\n\
f4.y = 6; // f4.y = 6.0\r\n\
f4.z = f3.z++; //f4.z = 3.0 f3.z = 4.0\r\n\
f4.w = 12; // f4.w = 12.0\r\n\
f3.x += f4.y++; // f3.x = 9.0 f4.y = 7.0\r\n\
f3.y -= f4.z--; // f3.y = 1.0 f4.z = 2.0\r\n\
f3.z++; // f3.z = 5.0\r\n\
++f4.x; // f4.x = 5.0\r\n\
f4.y--; // f4.y = 6.0\r\n\
--f4.z; // f4.z = 1.0\r\n\
f4.w *= f2.x += f3.z = 5; // f3.z = 5.0 f2.x = 7.0 f4.w = 84\r\n\
f2.x /= 0.5; // f2.x = 14.0\r\n\
f2.y **= 2.0; // f2.y = 9.0\r\n\
return 1;";

const char	*testCmplxType2 = 
"// Complex type test (array)\r\n\
import std.math;\r\n\
float3[10] fa;\r\n\
for(int i = 0; i < 10; i++)\r\n\
{\r\n\
	fa[i].x = i*8;\r\n\
	fa[i].y = fa[i].x++ - i*4;\r\n\
	fa[fa[(fa[i].x-1)*0.125].y*0.25].z = i+100;\r\n\
}\r\n\
return 1;";

char*	Format(const char *str, ...)
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

FILE *allocLog = NULL;
void* testAlloc(size_t size)
{
	if(!allocLog)
		allocLog = fopen("testAlloc.txt", "wb");
	static size_t overall = 0;
	static int allocs = 0;
	overall += size;
	allocs++;
	fprintf(allocLog, "%d Alloc of %u bytes (Total %u)\r\n", allocs, size, overall);
	fflush(allocLog);
	return malloc(size);
}
void testDealloc(void* ptr)
{
	free(ptr);
}

#ifdef SPEED_TEST
void speedTestStub(int x, int y, int width, int height, int color)
{
	(void)x; (void)y; (void)width; (void)height; (void)color;
}
#endif

int TestInt(int a)
{
	return a;
}

long long TestLong(long long a)
{
	return a;

}
float TestFloat(float a)
{
	return a;
}

double TestDouble(double a)
{
	return a;
}

void	RunTests()
{
	timeCompile = 0.0;
	timeGetListing = 0.0;
	timeGetBytecode = 0.0;
	timeClean = 0.0;
	timeLinkCode = 0.0;
	timeRun = 0.0;

	// Init NULLC
	nullcInit();
	//nullcInitCustomAlloc(testAlloc, testDealloc);
	nullcSetImportPath("Modules\\");

#ifdef SPEED_TEST
	nullcAddExternalFunction((void (*)())speedTestStub, "void draw_rect(int x, int y, int width, int height, int color);");
#endif

	nullcAddExternalFunction((void (*)())TestInt, "char char_(char a);");
	nullcAddExternalFunction((void (*)())TestInt, "short short_(short a);");
	nullcAddExternalFunction((void (*)())TestInt, "int int_(int a);");
	nullcAddExternalFunction((void (*)())TestLong, "long long_(long a);");
	nullcAddExternalFunction((void (*)())TestFloat, "float float_(float a);");
	nullcAddExternalFunction((void (*)())TestDouble, "double double_(double a);");

	nullcInitFileModule();
	nullcInitMathModule();

	int passed[] = { 0, 0, 0 };
	int testCount[] = { 0, 0, 0 };

	unsigned int	testTarget[] = { NULLC_VM, NULLC_X86, };

//////////////////////////////////////////////////////////////////////////
	printf("\r\nTwo bytecode merge test 1\r\n");

	const char *partA1 = "int a = 5;\r\nint test(int ref a, int b)\r\n{\r\n\treturn *a += b;\r\n}\r\ntest(&a, 4);\r\nvoid run(){ test(&a, 4); }\r\n";
	const char *partB1 = "int aa = 15;\r\nint testA(int ref a, int b)\r\n{\r\n\treturn *a += b + 1;\r\n}\r\ntestA(&aa, 5);\r\nvoid runA(){ testA(&aa, 5); }\r\nreturn aa;\r\n";

	char *bytecodeA, *bytecodeB;
	bytecodeA = NULL;
	bytecodeB = NULL;

	for(int t = 0; t < 2; t++)
	{
		testCount[t]++;
		nullcSetExecutor(testTarget[t]);

		nullres good = nullcCompile(partA1);
		nullcSaveListing("asm.txt");
		if(!good)
		{
			printf("Compilation failed: %s\r\n", nullcGetCompilationError());
			continue;
		}else{
			nullcGetBytecode(&bytecodeA);
		}

		good = nullcCompile(partB1);
		nullcSaveListing("asm.txt");
		if(!good)
		{
			printf("Compilation failed: %s\r\n", nullcGetCompilationError());
			continue;
		}else{
			nullcGetBytecode(&bytecodeB);
		}

		nullcClean();
		if(!nullcLinkCode(bytecodeA, 0))
		{
			printf("Compilation failed: %s\r\n", nullcGetRuntimeError());
			continue;
		}
		if(!nullcLinkCode(bytecodeB, 0))
		{
			printf("Compilation failed: %s\r\n", nullcGetRuntimeError());
			break;
		}
		delete[] bytecodeA;
		delete[] bytecodeB;

		if(!nullcRunFunction(NULL))
		{
			printf("Execution failed: %s\r\n", nullcGetRuntimeError());
		}else{
			if(!nullcRunFunction("run"))
			{
				printf("Execution failed: %s\r\n", nullcGetRuntimeError());
			}else{
				if(!nullcRunFunction("runA"))
				{
					printf("Execution failed: %s\r\n", nullcGetRuntimeError());
				}else{
					varData = (char*)nullcGetVariableData();

					varCount = 0;
					varInfo = (VariableInfo**)nullcGetVariableInfo(&varCount);
					if(varInfo)
					{
						bool lastFailed = false;
						CHECK_INT("ERROR", 0, 13);
						CHECK_INT("ERROR", 1, 27);
						if(!lastFailed)
							passed[t]++;
					}
				}
			}
		}
	}
//////////////////////////////////////////////////////////////////////////
	// Number operation test
	printf("\r\nInteger operation test\r\n");
	for(int t = 0; t < 2; t++)
	{
		testCount[t]++;
		if(RunCode(testIntOp, testTarget[t], "1"))
		{
			lastFailed = false;
			CHECK_INT("a", 0, 14);
			CHECK_INT("b", 0, 3);
			CHECK_INT("c", 0, 0);
			int resExp[] = { 17, 11, -14, -4, 42, 4, 2, 2744, 1, 0, 1, 0, 0, 1, 112, 1, 2, 15, 13, 0, 0, 0, 1, 0, 1, 1, 1, 1, 0, 0, 1, 0, 1 };
			for(int i = 0; i < 27; i++)
				CHECK_INT("res", i, resExp[i]);
			if(!lastFailed)
				passed[t]++;
		}
	}
	printf("\r\nDouble operation test\r\n");
	for(int t = 0; t < 2; t++)
	{
		testCount[t]++;
		if(RunCode(testDoubleOp, testTarget[t], "17.000000"))
		{
			lastFailed = false;
			CHECK_DOUBLE("a", 0, 14.0);
			CHECK_DOUBLE("b", 0, 3.0);
			double resExp[] = { 17.0, 11.0, -14.0, 42.0, 14.0/3.0, 2.0, 2744.0, 1.0, 0.0, 1.0, 0.0, 0.0, 1.0, 0.0};
			for(int i = 0; i < 14; i++)
				CHECK_DOUBLE("res", i, resExp[i]);
			if(!lastFailed)
				passed[t]++;
		}
	}
	printf("\r\nLong operation test\r\n");
	for(int t = 0; t < 2; t++)
	{
		testCount[t]++;
		if(RunCode(testLongOp, testTarget[t], "1"))
		{
			lastFailed = false;
			CHECK_LONG("a", 0, 4494967296ll);
			CHECK_LONG("b", 0, 594967296ll);
			CHECK_LONG("c", 0, 3ll);
			long long resExp[] = { 5089934592ll, 3900000000ll, -4494967296ll, -4494967297ll, 2674358537709551616ll, 7, 330196224, 210609828468829063ll, 1, 0, 1, 0, 0, 1,
				35959738368ll, 105553116266496ll, 561870912, 56771072, 5033163520ll, 4976392448ll, 0, 0, 0, 1, 0, 1, 1, 1, 1, 0, 0, 1, 0, 1 };
			for(int i = 0; i < 24; i++)
				CHECK_LONG("res", i, resExp[i]);
			if(!lastFailed)
				passed[t]++;
		}
	}
	printf("\r\nDecrement and increment tests for all types\r\n");
	for(int t = 0; t < 2; t++)
	{
		testCount[t]++;
		if(RunCode(tesIncDec, testTarget[t], "1"))
		{
			lastFailed = false;

			const char *name[] = { "a", "b", "c", "d", "e", "f" };
			int	value[] = { 9, 1, 5, 5, 8, 2 };

			int num = 1;
			for(int i = 0; i < 6; i++)
				CHECK_DOUBLE(Format("%s%d", name[i], num), 0, value[i]);
			num = 2;
			for(int i = 0; i < 6; i++)
				CHECK_FLOAT(Format("%s%d", name[i], num), 0, value[i]);
			num = 3;
			for(int i = 0; i < 6; i++)
				CHECK_LONG(Format("%s%d", name[i], num), 0, value[i]);
			num = 4;
			for(int i = 0; i < 6; i++)
				CHECK_INT(Format("%s%d", name[i], num), 0, value[i]);
			num = 5;
			for(int i = 0; i < 6; i++)
				CHECK_SHORT(Format("%s%d", name[i], num), 0, value[i]);
			num = 6;
			for(int i = 0; i < 6; i++)
				CHECK_CHAR(Format("%s%d", name[i], num), 0, value[i]);

			if(!lastFailed)
				passed[t]++;
		}
	}

	// Complex type tests
	printf("\r\nComplex type test (simple)\r\n");
	for(int t = 0; t < 2; t++)
	{
		testCount[t]++;
		if(RunCode(testCmplxType1, testTarget[t], "1"))
		{
			lastFailed = false;
			CHECK_FLOAT("f1", 0, 1);

			CHECK_FLOAT("f2", 0, 14);
			CHECK_FLOAT("f2", 1, 9);

			CHECK_FLOAT("f3", 0, 9);
			CHECK_FLOAT("f3", 1, 1);
			CHECK_FLOAT("f3", 2, 5);

			CHECK_FLOAT("f4", 0, 5);
			CHECK_FLOAT("f4", 1, 6);
			CHECK_FLOAT("f4", 2, 1);
			CHECK_FLOAT("f4", 3, 84);

			if(!lastFailed)
				passed[t]++;
		}
	}
	printf("\r\nComplex type test (complex)\r\n");
	for(int t = 0; t < 2; t++)
	{
		testCount[t]++;
		if(RunCode(testCmplxType2, testTarget[t], "1"))
		{
			lastFailed = false;

			float values[] = { 1, 0, 100, 9, 4, 101, 17, 8, 102, 25, 12, 103, 33, 16, 104, 41, 20, 105, 49, 24, 106, 57, 28, 107, 65, 32, 108, 73, 36, 109 };
			for(int i = 0; i < 30; i++)
				CHECK_FLOAT("fa", i, values[i]);

			if(!lastFailed)
				passed[t]++;
		}
	}


	const char	*testMislead = 
"// Compiler mislead test\r\n\
import std.math;\r\n\
float2 a;\r\n\
a/*[gg]*/.x = 2;\r\n\
a.y = a/*[gg]*/.x + 3;\r\n\
// Result:\r\n\
// a.x = 2; a.y = 5\r\n\
return a.x;";
	printf("\r\nCompiler mislead test\r\n");
	for(int t = 0; t < 2; t++)
	{
		testCount[t]++;
		if(RunCode(testMislead, testTarget[t], "2.000000"))
		{
			lastFailed = false;

			CHECK_FLOAT("a", 0, 2.0f);
			CHECK_FLOAT("a", 1, 5.0f);

			if(!lastFailed)
				passed[t]++;
		}
	}

const char	*testCmplx3 = 
"// Complex type test\r\n\
import std.math;\r\n\
float4x4 mat;\r\n\
mat.row1.y = 5;\r\n\
return 1;";
	printf("\r\nComplex type test\r\n");
	for(int t = 0; t < 2; t++)
	{
		testCount[t]++;
		if(RunCode(testCmplx3, testTarget[t], "1"))
		{
			lastFailed = false;

			CHECK_FLOAT("mat", 1, 5.0f);

			if(!lastFailed)
				passed[t]++;
		}
	}

const char	*testCycle = 
"int a=5;\r\n\
double[10] d=0.0;\r\n\
for(int i = 0; i < a; i++)\r\n\
d[i] = i*2 + i-2;\r\n\
return d[5];";
	printf("\r\nArray test\r\n");
	for(int t = 0; t < 2; t++)
	{
		testCount[t]++;
		if(RunCode(testCycle, testTarget[t], "0.000000"))
		{
			lastFailed = false;

			CHECK_INT("a", 0, 5);
			double values[] = { -2.0, 1.0, 4.0, 7.0, 10.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
			for(int i = 0; i < 10; i++)
				CHECK_DOUBLE("d", i, values[i]);

			if(!lastFailed)
				passed[t]++;
		}
	}

const char	*testFuncCall1 = 
"int test(int x, int y, int z){return x*y+z;}\r\n\
return 1+test(2, 3, 4);	// 11";
	printf("\r\nFunction call test 1\r\n");
	for(int t = 0; t < 2; t++)
	{
		testCount[t]++;
		if(RunCode(testFuncCall1, testTarget[t], "11"))
		{
			lastFailed = false;
			if(!lastFailed)
				passed[t]++;
		}
	}


const char	*testFuncCall2 = 
"int test(int x, int y, int z){return x*y+z;}\r\n\
int b = 1;\r\n\
if(7>5)\r\n\
b = 3;\r\n\
return b+test(2, 3, 4);";
	printf("\r\nFunction call test 2\r\n");
	for(int t = 0; t < 2; t++)
	{
		testCount[t]++;
		if(RunCode(testFuncCall2, testTarget[t], "13"))
		{
			lastFailed = false;
			if(!lastFailed)
				passed[t]++;
		}
	}


const char	*testFuncCall3 = 
"int fib(int n){ if(n<3) return 5; return 10; }\r\n\
return fib(1);";
	printf("\r\nFunction call test 3\r\n");
	for(int t = 0; t < 2; t++)
	{
		testCount[t]++;
		if(RunCode(testFuncCall3, testTarget[t], "5"))
		{
			lastFailed = false;
			if(!lastFailed)
				passed[t]++;
		}
	}


const char	*testRecursion = 
"int fib(int n){ if(n<3) return 1; return fib(n-2)+fib(n-1); }\r\n\
return fib(4);";
	printf("\r\nRecursion test\r\n");
	for(int t = 0; t < 2; t++)
	{
		testCount[t]++;
		if(RunCode(testRecursion, testTarget[t], "3"))
		{
			lastFailed = false;
			if(!lastFailed)
				passed[t]++;
		}
	}


const char	*testIndirection = 
"// Array indirection and optimization test\r\n\
int[5] res=1;\r\n\
res[1] = 13;\r\n\
res[2] = 3;\r\n\
res[res[2]] = 4;\r\n\
res[res[res[2]]] = 12;\r\n\
return 5;";
	printf("\r\nArray indirection and optimization test\r\n");
	for(int t = 0; t < 2; t++)
	{
		testCount[t]++;
		if(RunCode(testIndirection, testTarget[t], "5"))
		{
			lastFailed = false;

			CHECK_INT("res", 0, 1);
			CHECK_INT("res", 1, 13);
			CHECK_INT("res", 2, 3);
			CHECK_INT("res", 3, 4);
			CHECK_INT("res", 4, 12);

			if(!lastFailed)
				passed[t]++;
		}
	}


const char	*testAllInOne = 
"// Old all-in-one test\r\n\
double test(float x, float y){ /*teste*/return x**2*y; }\r\n\
int a=5;\r\n\
float b=1;\r\n\
float[3] c=14**2-134;\r\n\
double[10] d;\r\n\
for(int i = 0; i< 10; i++)\r\n\
d[i] = test(i*2, i-2);\r\n\
double n=1;\r\n\
while(1){ n*=2; if(n>1000) break; }\r\n\
return 2+test(2, 3)+a**b;";
	printf("\r\nOld all-in-one test\r\n");
	for(int t = 0; t < 2; t++)
	{
		testCount[t]++;
		if(RunCode(testAllInOne, testTarget[t], "19.000000"))
		{
			lastFailed = false;

			CHECK_INT("a", 0, 5);
			CHECK_FLOAT("b", 0, 1.0f);
			CHECK_FLOAT("c", 0, 62.0f);
			CHECK_FLOAT("c", 1, 62.0f);
			CHECK_FLOAT("c", 2, 62.0f);
			double values[] = { -0.0, -4.0, 0.0, 36.0, 128.0, 300.0, 576.0, 980.0, 1536.0, 2268.0 };
			for(int i = 0; i < 10; i++)
				CHECK_DOUBLE("d", i, values[i]);
			CHECK_DOUBLE("n", 0, 1024);

			if(!lastFailed)
				passed[t]++;
		}
	}


const char	*testLongSpeed = 
"//longPow speed test\r\n\
long a = 43l, b = 10l; \r\n\
long c;\r\n\
for(int i = 0; i < 1000; i++)\r\n\
  c = a**b;\r\n\
return 0;";
	printf("\r\nlongPow speed test\r\n");
	for(int t = 0; t < 2; t++)
	{
		testCount[t]++;
		if(RunCode(testLongSpeed, testTarget[t], "0"))
		{
			lastFailed = false;

			CHECK_LONG("a", 0, 43);
			CHECK_LONG("b", 0, 10);
			CHECK_LONG("c", 0, 21611482313284249ll);

			if(!lastFailed)
				passed[t]++;
		}
	}

const char	*testTypeConv = 
"// Type conversions\r\n\
int ia=3, ib, ic;\r\n\
double da=5.0, db, dc;\r\n\
long la=4l, lb, lc;\r\n\
ib = da;\r\n\
ic = la;\r\n\
db = ia;\r\n\
dc = la;\r\n\
lb = ia;\r\n\
lc = da;\r\n\
return 1;";
	printf("\r\nType conversions\r\n");
	for(int t = 0; t < 2; t++)
	{
		testCount[t]++;
		if(RunCode(testTypeConv, testTarget[t], "1"))
		{
			lastFailed = false;

			CHECK_INT("ia", 0, 3);
			CHECK_INT("ib", 0, 5);
			CHECK_INT("ic", 0, 4);
			CHECK_DOUBLE("da", 0, 5.0);
			CHECK_DOUBLE("db", 0, 3.0);
			CHECK_DOUBLE("dc", 0, 4.0);
			CHECK_LONG("la", 0, 4);
			CHECK_LONG("lb", 0, 3);
			CHECK_LONG("lc", 0, 5);

			if(!lastFailed)
				passed[t]++;
		}
	}


const char	*testArrayFill = 
"// Array fill test\r\n\
int[10] a=5;\r\n\
return 1;";
	printf("\r\nArray fill test\r\n");
	for(int t = 0; t < 2; t++)
	{
		testCount[t]++;
		if(RunCode(testArrayFill, testTarget[t], "1"))
		{
			lastFailed = false;

			for(int i = 0; i < 10; i++)
				CHECK_INT("a", i, 5);

			if(!lastFailed)
				passed[t]++;
		}
	}


const char	*testBuildinFunc = 
"// Build-In function checks\r\n\
import std.math;\r\n\
double Pi = 3.1415926583;\r\n\
double[27] res;\r\n\
res[0] = cos(0); // 1.0\r\n\
res[1] = cos(Pi/3.0); // 0.5\r\n\
res[2] = cos(Pi); // -1.0\r\n\
\r\n\
res[3] = sin(0); // 0.0\r\n\
res[4] = sin(Pi/6.0); // 0.5\r\n\
res[5] = sin(Pi); // 0.0\r\n\
\r\n\
res[6] = ceil(1.5); // 2.0\r\n\
res[7] = floor(1.5); // 1.0\r\n\
res[8] = ceil(-1.5); // -1.0\r\n\
res[9] = floor(-1.5); // -2.0\r\n\
\r\n\
res[10] = tan(0); // 0.0\r\n\
res[11] = tan(Pi/4.0); // 1.0\r\n\
res[12] = tan(Pi/2.0); // +inf\r\n\
\r\n\
res[13] = ctg(0); // +inf\r\n\
res[14] = ctg(Pi/4.0); // 1.0\r\n\
res[15] = ctg(Pi/2.0); // 0.0\r\n\
\r\n\
res[16] = sqrt(1.0); // 1.0\r\n\
res[17] = sqrt(0.0); // 0.0\r\n\
res[18] = sqrt(9.0); // 3.0\r\n\
\r\n\
res[19] = cosh(1.0); // \r\n\
res[20] = sinh(1.0); // \r\n\
res[21] = tanh(1.0); // \r\n\
\r\n\
res[22] = acos(0.5); // Pi/3.0\r\n\
res[23] = asin(0.5); // Pi/6.0\r\n\
res[24] = atan(1.0); // Pi/4.0\r\n\
\r\n\
res[25] = exp(1.0); // E\r\n\
res[26] = log(2.7182818284590452353602874713527); // 1.0\r\n\
\r\n\
return (\"hello\" == \"hello\") + (\"world\" != \"World\") + (\"world\" != \"worl\");";
	printf("\r\nBuild-In function checks\r\n");
	for(int t = 0; t < 2; t++)
	{
		testCount[t]++;
		if(RunCode(testBuildinFunc, testTarget[t], "3"))
		{
			lastFailed = false;

			double resExp[] = { 1.0, 0.5, -1.0, 0.0, 0.5, 0.0, 2.0, 1.0, -1.0, -2.0,
								0.0, 1.0, 0,0, 1.0, 0.0, 1.0, 0.0, 3.0, 
								1.5430806348152437784779056207571, 1.1752011936438014568823818505956, 0.76159415595576488811945828260479,
								1.0471975511965977461542144610932, 0.52359877559829887307710723054658, 0.78539816339744830961566084581988,
								2.7182818284590452353602874713527, 1.0 };
			for(int i = 0; i < 27; i++)
				if(i != 12 && i != 13)
					CHECK_DOUBLE("res", i, resExp[i]);

			if(!lastFailed)
				passed[t]++;
		}
	}


const char	*testDoublePow = 
"double a = 0.9;\r\n\
return a**2.0;";
	printf("\r\nDouble power\r\n");
	for(int t = 0; t < 2; t++)
	{
		testCount[t]++;
		if(RunCode(testDoublePow, testTarget[t], "0.810000"))
		{
			lastFailed = false;

			CHECK_DOUBLE("a", 0, 0.9);

			if(!lastFailed)
				passed[t]++;
		}
	}



const char	*testFuncCall4 = 
"double clamp(double a, double min, double max)\r\n\
{\r\n\
  if(a < min)\r\n\
    return min;\r\n\
  if(a > max)\r\n\
    return max;\r\n\
  return a;\r\n\
}\r\n\
double abs(double x)\r\n\
{\r\n\
  if(x < 0.0)\r\n\
    return -x;\r\n\
  return x;\r\n\
}\r\n\
return clamp(abs(-1.5), 0.0, 1.0);";
	printf("\r\nFunction call test 4\r\n");
	for(int t = 0; t < 2; t++)
	{
		testCount[t]++;
		if(RunCode(testFuncCall4, testTarget[t], "1.000000"))
		{
			lastFailed = false;

			if(!lastFailed)
				passed[t]++;
		}
	}


const char	*testFuncCall5 = 
"int test(int x, int y, int z){return x*y+z;}\r\n\
int res;\r\n\
{\r\n\
int x = 2;\r\n\
{\r\n\
res = 1+test(x, 3, 4);\r\n\
}\r\n\
}\r\n\
return res;";
	printf("\r\nFunction Call test 5\r\n");
	for(int t = 0; t < 2; t++)
	{
		testCount[t]++;
		if(RunCode(testFuncCall5, testTarget[t], "11"))
		{
			lastFailed = false;

			CHECK_INT("res", 0, 11);

			if(!lastFailed)
				passed[t]++;
		}
	}


const char	*testFuncCall6 = 
"double abs(double x)\r\n\
{\r\n\
  {\r\n\
    if(x < 0.0)\r\n\
      return -x;\r\n\
  }\r\n\
  return x;\r\n\
}\r\n\
return abs(-0.5);";
	printf("\r\nFunction call test 6\r\n");
	for(int t = 0; t < 2; t++)
	{
		testCount[t]++;
		if(RunCode(testFuncCall6, testTarget[t], "0.500000"))
		{
			lastFailed = false;

			if(!lastFailed)
				passed[t]++;
		}
	}

const char	*testIncDec = 
"int[5] test=0;\r\n\
for(int i = 0; i < 5; i++)\r\n\
{\r\n\
  test[i] = 1;\r\n\
  test[i] += 5;\r\n\
  test[i] = test[i]++;\r\n\
  test[i] = ++test[i];\r\n\
}\r\n\
return 1;";
	printf("\r\nInc dec test\r\n");
	for(int t = 0; t < 2; t++)
	{
		testCount[t]++;
		if(RunCode(testIncDec, testTarget[t], "1"))
		{
			lastFailed = false;

			CHECK_INT("test", 0, 7);
			CHECK_INT("test", 1, 7);
			CHECK_INT("test", 2, 7);
			CHECK_INT("test", 3, 7);
			CHECK_INT("test", 4, 7);

			if(!lastFailed)
				passed[t]++;
		}
	}


const char	*testPointers = 
"// Pointers!\r\n\
int testA(int ref v){ return *v * 5; }\r\n\
void testB(int ref v){ *v += 5; }\r\n\
int a = 5;\r\n\
int ref b = &a;\r\n\
int c = 2;\r\n\
c = *b;\r\n\
*b = 14;\r\n\
*b++;\r\n\
*b *= 4;\r\n\
testB(b);\r\n\
return testA(&a);";
	printf("\r\nPointers\r\n");
	for(int t = 0; t < 2; t++)
	{
		testCount[t]++;
		if(RunCode(testPointers, testTarget[t], "325"))
		{
			lastFailed = false;
			CHECK_INT("a", 0, 65);
			CHECK_INT("c", 0, 5);
			if(!lastFailed)
				passed[t]++;
		}
	}


const char	*testPointersCmplx = 
"// Pointers on complex!\r\n\
import std.math;\r\n\
double length(float4 ref v)\r\n\
{\r\n\
	return sqrt((v.x*v.x)+(v.y*v.y)+(v.z*v.z));\r\n\
}\r\n\
void normalize(float4 ref v)\r\n\
{\r\n\
	double len = length(v);\r\n\
	v.x /= len; v.y /= len; v.z /= len;\r\n\
}\r\n\
float4 a;\r\n\
a.x = 12.0;\r\n\
a.y = 4.0;\r\n\
a.z = 3.0;\r\n\
a.w = 1.0;\r\n\
float4 ref b = &a;\r\n\
normalize(&a);\r\n\
return length(b);";
	printf("\r\nPointers on complex\r\n");
	for(int t = 0; t < 2; t++)
	{
		testCount[t]++;
		if(RunCode(testPointersCmplx, testTarget[t], "1.000000"))
		{
			lastFailed = false;

			CHECK_FLOAT("a", 0, 12.0f/13.0f);
			CHECK_FLOAT("a", 1, 4.0f/13.0f);
			CHECK_FLOAT("a", 2, 3.0f/13.0f);
			CHECK_FLOAT("a", 3, 1.0f);
			if(!lastFailed)
				passed[t]++;
		}
	}


const char	*testPointersCmplx2 = 
"import std.math;\r\n\
float4 a;\r\n\
float4 ref b = &a;\r\n\
b.x = 5.0f;\r\n\
return b.x;";
	printf("\r\nPointers on complex 2\r\n");
	for(int t = 0; t < 2; t++)
	{
		testCount[t]++;
		if(RunCode(testPointersCmplx2, testTarget[t], "5.000000"))
		{
			lastFailed = false;
			CHECK_FLOAT("a", 0, 5.0);
			if(!lastFailed)
				passed[t]++;
		}
	}


const char	*testPointers2 = 
"import std.math;\r\n\
double testA(float4 ref v){ return v.x; }\r\n\
float4 a;\r\n\
float4 ref b = &a;\r\n\
a.x = 5.0f;\r\n\
return testA(b);";
	printf("\r\nPointers 2\r\n");
	for(int t = 0; t < 2; t++)
	{
		testCount[t]++;
		if(RunCode(testPointers2, testTarget[t], "5.000000"))
		{
			lastFailed = false;
			CHECK_FLOAT("a", 0, 5.0);
			if(!lastFailed)
				passed[t]++;
		}
	}


const char	*testOptiA = 
"int a = 12;\r\n\
int[6] res;\r\n\
res[0] = a + 0;\r\n\
res[1] = a * 0;\r\n\
res[2] = a * 1;\r\n\
res[3] = (a*1) +(a*0);\r\n\
res[4] = a*2+0;\r\n\
res[5] = a*3*1;\r\n\
return 1;";
	printf("\r\nSimple optimizations\r\n");
	for(int t = 0; t < 2; t++)
	{
		testCount[t]++;
		if(RunCode(testOptiA, testTarget[t], "1"))
		{
			lastFailed = false;
			CHECK_INT("a", 0, 12);
			CHECK_INT("res", 0, 12);
			CHECK_INT("res", 1, 0);
			CHECK_INT("res", 2, 12);
			CHECK_INT("res", 3, 12);
			CHECK_INT("res", 4, 24);
			CHECK_INT("res", 5, 36);
			if(!lastFailed)
				passed[t]++;
		}
	}


const char	*testPointers3 = 
"import std.math;\r\n\
int[5] arr;\r\n\
float4[4] arrF;\r\n\
int ref a = &arr[3];\r\n\
*a = 55;\r\n\
float4 ref b = &arrF[1];\r\n\
b.x = 85;\r\n\
float ref c = &arrF[1].y;\r\n\
*c = 125;\r\n\
{\r\n\
	int ref a = &arr[1];\r\n\
	*a = 5;\r\n\
	float4 ref b = &arrF[2];\r\n\
	b.x = 8;\r\n\
	float ref c = &arrF[2].y;\r\n\
	*c = 12;\r\n\
}\r\n\
return 1;";
	printf("\r\nPointers test 3\r\n");
	for(int t = 0; t < 2; t++)
	{
		testCount[t]++;
		if(RunCode(testPointers3, testTarget[t], "1"))
		{
			lastFailed = false;
			CHECK_INT("arr", 1, 5);
			CHECK_INT("arr", 3, 55);

			CHECK_FLOAT("arrF", 4, 85.0);
			CHECK_FLOAT("arrF", 5, 125.0);

			CHECK_FLOAT("arrF", 8, 8.0);
			CHECK_FLOAT("arrF", 9, 12.0);
			if(!lastFailed)
				passed[t]++;
		}
	}


const char	*testCalls = 
"int calltest = 0;\r\n\
int fib(int n, int ref calls)\r\n\
{\r\n\
	*calls++;\r\n\
	calltest++;\r\n\
	if(n < 3)\r\n\
		return 1;\r\n\
	return fib(n-2, calls) + fib(n-1, calls);\r\n\
}\r\n\
int calls = 0;\r\n\
return fib(/*40*/15, &calls);";
	printf("\r\nCall number test\r\n");
	for(int t = 0; t < 2; t++)
	{
		testCount[t]++;
		if(RunCode(testCalls, testTarget[t], "610"))
		{
			lastFailed = false;
			CHECK_INT("calltest", 0, 1219);
			CHECK_INT("calls", 0, 1219);
			if(!lastFailed)
				passed[t]++;
		}
	}



const char	*testNegate = 
"double neg(double a){ return -a; }\r\n\
double x = 5.0, nx;\r\n\
for(int i = 0; i < /*50000000*/1000; i++)\r\n\
nx = neg(x);\r\n\
return nx;";
	printf("\r\nNegate test\r\n");
	for(int t = 0; t < 2; t++)
	{
		testCount[t]++;
		if(RunCode(testNegate, testTarget[t], "-5.000000"))
		{
			lastFailed = false;
			CHECK_DOUBLE("x", 0, 5.0);
			CHECK_DOUBLE("nx", 0, -5.0);
			if(!lastFailed)
				passed[t]++;
		}
	}



const char	*testFuncOverload = 
"int fa(int i){ return i*2; }\r\n\
int fa(int i, double c){ return i*c; }\r\n\
int fa(float i){ return i*3.0f; }\r\n\
return fa(5.0f) * fa(2, 3.0);";
	printf("\r\nFunction overload test\r\n");
	for(int t = 0; t < 2; t++)
	{
		testCount[t]++;
		if(RunCode(testFuncOverload, testTarget[t], "90"))
			passed[t]++;
	}

const char	*testSwitch = 
"// Switch test!\r\n\
int u = 12;\r\n\
int a = 3, b = 0;\r\n\
{\r\n\
  switch(a)\r\n\
  {\r\n\
    case 1:\r\n\
  	  b = 5;\r\n\
	  break;\r\n\
    case 3:\r\n\
	  b = 7;\r\n\
	  break;\r\n\
	case 5:\r\n\
	  b = 18;\r\n\
  }\r\n\
}\r\n\
return u;";
	printf("\r\nSwitch test\r\n");
	for(int t = 0; t < 2; t++)
	{
		testCount[t]++;
		if(RunCode(testSwitch, testTarget[t], "12"))
		{
			lastFailed = false;

			CHECK_INT("u", 0, 12);
			CHECK_INT("a", 0, 3);
			CHECK_INT("b", 0, 7);

			if(!lastFailed)
				passed[t]++;
		}
	}


const char	*testClass1 = 
"// Class test\r\n\
import std.math;\r\n\
class One\r\n\
{\r\n\
  int a, b, c;\r\n\
  float e, f;\r\n\
}\r\n\
class Two\r\n\
{\r\n\
  One a, b;\r\n\
  float3 c;\r\n\
  int d;\r\n\
}\r\n\
One one;\r\n\
Two two;\r\n\
one.a = 3;\r\n\
one.e = 2;\r\n\
two.a.a = 14;\r\n\
two.c.x = 2;\r\n\
return 1;";
	printf("\r\nClass test\r\n");
	for(int t = 0; t < 2; t++)
	{
		testCount[t]++;
		if(RunCode(testClass1, testTarget[t], "1"))
		{
			lastFailed = false;

			CHECK_INT("one", 0, 3);
			CHECK_FLOAT("one", 3, 2.0f);
			CHECK_INT("two", 0, 14);
			CHECK_FLOAT("two", 10, 2.0f);

			if(!lastFailed)
				passed[t]++;
		}
	}

const char	*testVarMod = 
"// Variable modify test\r\n\
int slow(int how){ for(int i = 0; i < how; i++){ how = how-1; } return 2; }\r\n\
int index = 2;\r\n\
int[10] arr = 4;\r\n\
arr[slow(/*40000000*/1000)] += 16; // 330 ms total. target - 140ms\r\n\
return 3;";
	printf("\r\nVariable modify test\r\n");
	for(int t = 0; t < 2; t++)
	{
		testCount[t]++;
		if(RunCode(testVarMod, testTarget[t], "3"))
		{
			lastFailed = false;

			CHECK_INT("index", 0, 2);
			for(int i = 0; i < 10; i++)
			{
				if(i != 2)
				{
					CHECK_INT("arr", i, 4);
				}else{
					CHECK_INT("arr", i, 20);
				}
			}

			if(!lastFailed)
				passed[t]++;
		}
	}

	
const char	*testClass2 = 
"// Class test 2\r\n\
import std.math;\r\n\
class One\r\n\
{\r\n\
  int a, b, c;\r\n\
  float e, f;\r\n\
}\r\n\
class Two\r\n\
{\r\n\
  One a, b;\r\n\
  float3 c;\r\n\
  int d;\r\n\
}\r\n\
Two two, twonext;\r\n\
float3[2][4] fa;\r\n\
int[2][4] ia;\r\n\
double[8] da;\r\n\
char c = 66;\r\n\
short u = 15;\r\n\
long l = 45645l;\r\n\
l *= 4594454795l;\r\n\
float4x4 mat;\r\n\
\r\n\
two.a.a = 14;\r\n\
two.c.x = 2;\r\n\
two.d = 5;\r\n\
twonext = two;\r\n\
return 1;";
	printf("\r\nClass test 2\r\n");
	for(int t = 0; t < 2; t++)
	{
		testCount[t]++;
		if(RunCode(testClass2, testTarget[t], "1"))
		{
			lastFailed = false;

			CHECK_CHAR("c", 0, 66);
			CHECK_SHORT("u", 0, 15);
			CHECK_LONG("l", 0, 45645ll * 4594454795ll);
			
			CHECK_INT("two", 0, 14);
			CHECK_FLOAT("two", 10, 2);
			CHECK_INT("two", 13, 5);

			CHECK_INT("twonext", 0, 14);
			CHECK_FLOAT("twonext", 10, 2);
			CHECK_INT("twonext", 13, 5);

			if(!lastFailed)
				passed[t]++;
		}
	}

const char	*testCmplx4 = 
"//Complex types test #3\r\n\
import std.math;\r\n\
float test(float4 a, float4 b){ return a.x*b.x+a.y*b.y+a.z*b.z+a.w*b.w; }\r\n\
float4 test2(float4 u){ u.x += 5.0; return u; }\r\n\
float4 float4(float all){ float4 ret; ret.x = ret.y = ret.z = ret.w = all; return ret; }\r\n\
float sum(float[10] u){ float res = 0; for(int i = 0; i < 10; i++) res += u[i]; return res; }\r\n\
float[10] inc(float[10] v){ float[10] res; for(int i = 0; i < 10; i++) res[i] = v[i]+1.0f; return res; }\r\n\
float4 n, m;\r\n\
n.x = 6.0f;\r\n\
n.y = 3.0f;\r\n\
n.z = 5.0f;\r\n\
n.w = 0.0f;\r\n\
\r\n\
m.x = 2.0f;\r\n\
m.y = 3.0f;\r\n\
m.z = 7.0f;\r\n\
m.w = 0.0f;\r\n\
float3 k;\r\n\
k.x = 12.0;\r\n\
k.y = 4.7;\r\n\
k.z = 0;\r\n\
float4 u = test2(n), v = float4(2.5, 1.2, 5, 6.0), w = float4(5.9), q = float4(k, 2.0);\r\n\
float[10] arr;\r\n\
for(int i = 0; i < 10; i++)\r\n\
  arr[i] = i*1.5f;\r\n\
float arrSum = sum(arr);\r\n\
float[10] iArr = inc(arr);\r\n\
float iArrSum = sum(iArr);\r\n\
return test(n, m); // 56.0";
	printf("\r\nComplex types test #3\r\n");
	for(int t = 0; t < 2; t++)
	{
		testCount[t]++;
		if(RunCode(testCmplx4, testTarget[t], "56.000000"))
		{
			lastFailed = false;

			float values[] = { 6, 3, 5, 0, 2, 3, 7, 0, 12, 4.7f, 0, 11, 3, 5, 0, 2.5f, 1.2f, 5, 6, 5.9f, 5.9f, 5.9f, 5.9f, 12, 4.7f, 0, 2 };
			for(int i = 0; i < 4; i++)
				CHECK_FLOAT("n", i, values[i]);
			for(int i = 0; i < 4; i++)
				CHECK_FLOAT("m", i, values[i+4]);
			for(int i = 0; i < 3; i++)
				CHECK_FLOAT("k", i, values[i+8]);
			for(int i = 0; i < 4; i++)
				CHECK_FLOAT("u", i, values[i+11]);
			for(int i = 0; i < 4; i++)
				CHECK_FLOAT("v", i, values[i+15]);
			for(int i = 0; i < 4; i++)
				CHECK_FLOAT("w", i, values[i+19]);
			for(int i = 0; i < 4; i++)
				CHECK_FLOAT("q", i, values[i+23]);

			float values2[] = { 0.0, 1.5, 3.0, 4.5, 6.0, 7.5, 9.0, 10.5, 12.0, 13.5 };
			for(int i = 0; i < 10; i++)
				CHECK_FLOAT("arr", i, values2[i]);
			CHECK_FLOAT("arrSum", 0, 67.5f);
			for(int i = 0; i < 10; i++)
				CHECK_FLOAT("iArr", i, values2[i]+1.0f);
			CHECK_FLOAT("iArrSum", 0, 77.5f);

			if(!lastFailed)
				passed[t]++;
		}
	}

const char	*testSpeed = 
"// Speed tests\r\n\
import std.math;\r\n\
float4x4 mat;\r\n\
class Float{ float x; }\r\n\
Float f;\r\n\
float2 f2;\r\n\
float3 f3;\r\n\
float4 f4;\r\n\
float4x4 test(float4x4 p){ p.row1.y *= 2.0f; return p; }\r\n\
Float test(Float p){ p.x *= 2.0f; return p; }\r\n\
float2 test(float2 p){ p.y *= 2.0f; return p; }\r\n\
float3 test(float3 p){ p.y *= 2.0f; return p; }\r\n\
float4 test(float4 p){ p.y *= 2.0f; return p; }\r\n\
for(int i = 0; i < 100; i++)\r\n\
{\r\n\
  mat = test(mat);\r\n\
  f = test(f);\r\n\
  f2 = test(f2);\r\n\
  f3 = test(f3);\r\n\
  f4 = test(f4);\r\n\
}\r\n\
return int(mat.row1.y);";
	printf("\r\nSpeed tests\r\n");
	for(int t = 0; t < 2; t++)
	{
		testCount[t]++;
		if(RunCode(testSpeed, testTarget[t], "0"))
		{
			lastFailed = false;

			if(!lastFailed)
				passed[t]++;
		}
	}

const char	*testAuto = 
"//Auto type tests\r\n\
import std.math;\r\n\
float lengthSqr(float3 ref f){ return f.x*f.x+f.y*f.y+f.z*f.z; }\r\n\
float[10] tenArr(float n){ float[10] arr; for(int i = 0; i < 10; i++) arr[i] = n; return arr; }\r\n\
auto b = 15;\r\n\
auto c = 2*2.0;\r\n\
float3 m;\r\n\
m.x = 3.0;\r\n\
m.y = 0.0;\r\n\
m.z = 1;\r\n\
auto n = m, nd = lengthSqr(&n);\r\n\
auto k = c = 12;\r\n\
auto ar = tenArr(3.0);\r\n\
\r\n\
auto u = &b;\r\n\
return *u;";
	printf("\r\nAuto type tests\r\n");
	for(int t = 0; t < 2; t++)
	{
		testCount[t]++;
		if(RunCode(testAuto, testTarget[t], "15"))
		{
			lastFailed = false;

			CHECK_INT("b", 0, 15);
			CHECK_DOUBLE("c", 0, 12.0);

			CHECK_FLOAT("m", 0, 3.0f);
			CHECK_FLOAT("m", 1, 0.0f);
			CHECK_FLOAT("m", 2, 1.0f);

			CHECK_FLOAT("n", 0, 3.0f);
			CHECK_FLOAT("m", 1, 0.0f);
			CHECK_FLOAT("n", 2, 1.0f);

			CHECK_FLOAT("nd", 0, 10.0f);
			CHECK_DOUBLE("k", 0, 12.0f);

			for(int i = 0; i < 10; i++)
				CHECK_FLOAT("ar", i, 3.0);
			if(!lastFailed)
				passed[t]++;
		}
	}

const char	*testCharArr = 
"// Char array test\r\n\
auto str1 = \"\", str2 = \"a\", str3 = \"ab\";\r\n\
auto str4 = \"abc\", str5 = \"abcd\", string = \"Hello World!\";\r\n\
return 1;";
	printf("\r\nChar array test\r\n");
	for(int t = 0; t < 2; t++)
	{
		testCount[t]++;
		if(RunCode(testCharArr, testTarget[t], "1"))
		{
			lastFailed = false;

			CHECK_STR("str1", 0, "");
			CHECK_STR("str2", 0, "a");
			CHECK_STR("str3", 0, "ab");
			CHECK_STR("str4", 0, "abc");
			CHECK_STR("str5", 0, "abcd");
			CHECK_STR("string", 0, "Hello World!");

			if(!lastFailed)
				passed[t]++;
		}
	}


const char	*testCharArr2 = 
"char[2][5] arr;\r\n\
arr[0] = \"hehe\";\r\n\
arr[1] = \"haha\";\r\n\
auto un = \"buggy\";\r\n\
align(13) class sss{ int a; char[5] uhu; int bb; }\r\n\
sss kl;\r\n\
char p;\r\n\
noalign double c, d;\r\n\
double f;\r\n\
kl.uhu = \"tyty\";\r\n\
return 0;";
	printf("\r\nChar array test 2\r\n");
	for(int t = 0; t < 2; t++)
	{
		testCount[t]++;
		if(RunCode(testCharArr2, testTarget[t], "0"))
		{
			lastFailed = false;

			CHECK_STR("arr", 0, "hehe");
			CHECK_STR("arr", 8, "haha");

			CHECK_STR("un", 0, "buggy");

			CHECK_STR("kl", 4, "tyty");

			if(!lastFailed)
				passed[t]++;
		}
	}


const char	*testArrPtr = 
"// Implicit pointers to arrays\r\n\
int sum(int[] arr){ int res=0; for(int i=0;i<arr.size;i++) res+=arr[i]; return res; }\r\n\
void print(char[] str){ 1; } \r\n\
\r\n\
int[7] u=0;\r\n\
int[] k = u;\r\n\
int i = 3;\r\n\
\r\n\
u[1] = 5;\r\n\
k[2] = 2;\r\n\
k[i] = 5;\r\n\
\r\n\
int[7][3] uu;\r\n\
uu[2][1] = 100;\r\n\
int[][3] kk = uu;\r\n\
\r\n\
k[4] = kk[2][1];\r\n\
\r\n\
auto name = \"omfg\";\r\n\
print(name);\r\n\
print(\"does work\");\r\n\
\r\n\
return sum(u);";
	printf("\r\nImplicit pointers to arrays\r\n");
	for(int t = 0; t < 2; t++)
	{
		testCount[t]++;
		if(RunCode(testArrPtr, testTarget[t], "112"))
		{
			lastFailed = false;

			CHECK_INT("u", 1, 5);
			CHECK_INT("u", 2, 2);
			CHECK_INT("u", 3, 5);
			CHECK_INT("u", 4, 100);

			CHECK_INT("k", 1, 7);

			CHECK_INT("uu", 7, 100);

			CHECK_INT("kk", 1, 7);

			CHECK_STR("name", 0, "omfg");
			CHECK_STR("$temp1", 0, "does work");

			if(!lastFailed)
				passed[t]++;
		}
	}


const char	*testFile = 
"// File and something else test\r\n\
import std.file;\r\n\
int test(char[] eh, int u){ int b = 0; for(int i=0;i<u;i++)b+=eh[i]; return b; }\r\n\
\r\n\
auto uh = \"ehhhe\";\r\n\
int k = 5464321;\r\n\
File n = File(\"haha.txt\", \"wb\");\r\n\
auto text = \"Hello file!!!\";\r\n\
n.Write(text);\r\n\
\r\n\
n.Write(k);\r\n\
n.Close();\r\n\
return test(uh, 3);";
	printf("\r\nFile and something else test\r\n");
	for(int t = 0; t < 2; t++)
	{
		testCount[t]++;
		if(RunCode(testFile, testTarget[t], "309"))
		{
			lastFailed = false;
			CHECK_STR("uh", 0, "ehhhe");
			CHECK_STR("$temp1", 0, "haha.txt");
			CHECK_STR("$temp2", 0, "wb");
			CHECK_STR("text", 0, "Hello file!!!");

			CHECK_INT("k", 0, 5464321);
			if(!lastFailed)
				passed[t]++;
		}
	}


const char	*testFile2 = 
"//File test\r\n\
import std.file;\r\n\
auto name = \"extern.bin\";\r\n\
auto acc = \"wb\", acc2 = \"rb\";\r\n\
\r\n\
// Perform to write\r\n\
auto text = \"Hello again\";\r\n\
char ch = 45;\r\n\
short sh = 4*256+45;\r\n\
int num = 12568;\r\n\
long lnum = 4586564;\r\n\
\r\n\
// Write to file\r\n\
File test = File(name, acc);\r\n\
test.Write(text);\r\n\
test.Write(ch);\r\n\
test.Write(sh);\r\n\
test.Write(num);\r\n\
test.Write(lnum);\r\n\
test.Write(ch);\r\n\
test.Write(sh);\r\n\
test.Write(num);\r\n\
test.Write(lnum);\r\n\
test.Close();\r\n\
\r\n\
// Perform to read\r\n\
char[12] textR1;\r\n\
char chR1, chR2;\r\n\
short shR1, shR2;\r\n\
int numR1, numR2;\r\n\
long lnumR1, lnumR2;\r\n\
\r\n\
test.Open(name, acc2);\r\n\
test.Read(textR1);\r\n\
test.Read(&chR1);\r\n\
test.Read(&shR1);\r\n\
test.Read(&numR1);\r\n\
test.Read(&lnumR1);\r\n\
test.Read(&chR2);\r\n\
test.Read(&shR2);\r\n\
test.Read(&numR2);\r\n\
test.Read(&lnumR2);\r\n\
test.Close();\r\n\
return 1;";
	printf("\r\nFile test 2\r\n");
	for(int t = 0; t < 2; t++)
	{
		testCount[t]++;
		if(RunCode(testFile2, testTarget[t], "1"))
		{
			lastFailed = false;
			CHECK_STR("name", 0, "extern.bin");
			CHECK_STR("acc", 0, "wb");
			CHECK_STR("acc2", 0, "rb");
			CHECK_STR("text", 0, "Hello again");
			CHECK_STR("textR1", 0, "Hello again");

			CHECK_CHAR("ch", 0, 45);
			CHECK_SHORT("sh", 0, 1069);
			CHECK_INT("num", 0, 12568);
			CHECK_LONG("lnum", 0, 4586564);

			CHECK_CHAR("chR1", 0, 45);
			CHECK_SHORT("shR1", 0, 1069);
			CHECK_INT("numR1", 0, 12568);
			CHECK_LONG("lnumR1", 0, 4586564);

			CHECK_CHAR("chR2", 0, 45);
			CHECK_SHORT("shR2", 0, 1069);
			CHECK_INT("numR2", 0, 12568);
			CHECK_LONG("lnumR2", 0, 4586564);
			if(!lastFailed)
				passed[t]++;
		}
	}


const char	*testEscape = 
"//Escape sequences\r\n\
auto name = \"01n\";\r\n\
for(int i = 0; i < 10; i++)\r\n\
{\r\n\
  name[1] = i+'0';\r\n\
  name[2] = '\\n';\r\n\
}\r\n\
\r\n\
return 'n';";
	printf("\r\nEscape sequences\r\n");
	for(int t = 0; t < 2; t++)
	{
		testCount[t]++;
		if(RunCode(testEscape, testTarget[t], "110"))
		{
			lastFailed = false;
			CHECK_STR("name", 0, "09\n");
			if(!lastFailed)
				passed[t]++;
		}
	}


const char	*testPrint = 
"auto ts = \"Hello World!\\r\\noh\\toh\\r\\n\";\r\n\
char[] mm = \"hello\";\r\n\
char[] mn = \"world\";\r\n\
auto mo = \"!!!\";\r\n\
return 1;";
	printf("\r\nPrint test\r\n");
	for(int t = 0; t < 2; t++)
	{
		testCount[t]++;
		if(RunCode(testPrint, testTarget[t], "1"))
		{
			lastFailed = false;
			CHECK_STR("ts", 0, "Hello World!\r\noh\toh\r\n");
			CHECK_STR("$temp1", 0, "hello");
			CHECK_STR("$temp2", 0, "world");
			CHECK_STR("mo", 0, "!!!");
			if(!lastFailed)
				passed[t]++;
		}
	}



const char	*testVarGetSet1 = 
"import std.math;\r\n\
int[10] a=4;\r\n\
int[] b = a;\r\n\
float4 c;\r\n\
c.y = 5.0f;\r\n\
c.z = c.w;\r\n\
b[8] = 4;\r\n\
int t1 = b[8];\r\n\
int t2 = b.size;\r\n\
\r\n\
for(int i = 0; i < 3; i++)\r\n\
  b[5+i] += 3;\r\n\
b[3]++;\r\n\
b[6] = ++b[9];\r\n\
b[2] = b[8]++;\r\n\
b[b[8]]++;\r\n\
b[b[2]] = b[b[8]]++;\r\n\
b[4]--;\r\n\
return b[1];";
	printf("\r\nVariable get and set\r\n");
	for(int t = 0; t < 2; t++)
	{
		testCount[t]++;
		if(RunCode(testVarGetSet1, testTarget[t], "4"))
		{
			lastFailed = false;
			int val[] = { 4, 4, 4, 5, 7, 9, 5, 7, 5, 5, };
			for(int i = 0; i < 10; i++)
				CHECK_INT("a", i, val[i]);
			CHECK_INT("b", 1, 10);

			CHECK_FLOAT("c", 1, 5.0f);

			CHECK_INT("t1", 0, 4);
			CHECK_INT("t2", 0, 10);
			if(!lastFailed)
				passed[t]++;
		}
	}


const char	*testArrays = 
"import std.math;\r\n\
int test(int a, int b, int c){ return a*b+c; }\r\n\
float test2(float4 b){ return b.x-b.y; }\r\n\
int test3(char[] ch){ return ch.size; }\r\n\
\r\n\
int kl = test(2, 3, 4);\r\n\
float4 kl2; kl2.x = 5.0; kl2.y = 3.0;\r\n\
float res2 = test2(kl2);\r\n\
auto kl4 = \"kjskadjaskd\";\r\n\
int kl5 = test3(kl4);\r\n\
return 1;";
	printf("\r\nArrays test\r\n");
	for(int t = 0; t < 2; t++)
	{
		testCount[t]++;
		if(RunCode(testArrays, testTarget[t], "1"))
		{
			lastFailed = false;
			CHECK_INT("kl", 0, 10);
			CHECK_FLOAT("kl2", 0, 5.0f);
			CHECK_FLOAT("kl2", 1, 3.0f);

			CHECK_FLOAT("res2", 0, 2.0f);

			CHECK_INT("kl5", 0, 12);
			CHECK_STR("kl4", 0, "kjskadjaskd");
			if(!lastFailed)
				passed[t]++;
		}
	}



const char	*testArrays2 = 
"int test(int a, int b, int c){ return a*b+c; }\r\n\
void test4(float b){ return; }\r\n\
float test6(){ return 12; }\r\n\
\r\n\
int sum(int[] arr)\r\n\
{\r\n\
  int res = 0;\r\n\
  for(int i = 0; i < arr.size; i++)\r\n\
    res += arr[i];\r\n\
  return res;\r\n\
}\r\n\
short sh(int a){ return a; }\r\n\
auto n = { 10, 12, 11, 156 };\r\n\
auto m = \"hello?\";\r\n\
auto l = { sh(1), sh(2), sh(3) };\r\n\
\r\n\
int ns = sum(n), ts = sum({10, 12, 14, 16});\r\n\
return 1;";
	printf("\r\nArray test 2\r\n");
	for(int t = 0; t < 2; t++)
	{
		testCount[t]++;
		if(RunCode(testArrays2, testTarget[t], "1"))
		{
			lastFailed = false;
			CHECK_INT("n", 0, 10);
			CHECK_INT("n", 1, 12);
			CHECK_INT("n", 2, 11);
			CHECK_INT("n", 3, 156);

			CHECK_STR("m", 0, "hello?");

			CHECK_INT("l", 0, 1);
			CHECK_INT("l", 1, 2);
			CHECK_INT("l", 2, 3);

			CHECK_INT("ns", 0, 189);
			CHECK_INT("ts", 0, 52);

			CHECK_INT("$temp1", 0, 10);
			CHECK_INT("$temp1", 1, 12);
			CHECK_INT("$temp1", 2, 14);
			CHECK_INT("$temp1", 3, 16);
			if(!lastFailed)
				passed[t]++;
		}
	}



const char	*testVisibility = 
"// Function visibility test\r\n\
int u = 5, t1 = 2, t2 = 2;\r\n\
for({int i = 0;}; i < 4; {i++;})\r\n\
{\r\n\
  int b = 2;\r\n\
  {\r\n\
    int test(int x){ return x*b+u; }\r\n\
    t1 = test(2);\r\n\
  }\r\n\
  {\r\n\
    int test(int x){ return x*u+b; }\r\n\
    t2 = test(2);\r\n\
  }\r\n\
}\r\n\
\r\n\
return 4;";
	printf("\r\nFunction visibility test\r\n");
	for(int t = 0; t < 2; t++)
	{
		testCount[t]++;
		if(RunCode(testVisibility, testTarget[t], "4"))
		{
			lastFailed = false;
			CHECK_INT("u", 0, 5);
			CHECK_INT("t1", 0, 9);
			CHECK_INT("t2", 0, 12);
			if(!lastFailed)
				passed[t]++;
		}
	}


const char	*testLocalFunc1 = 
"// Local function context test\r\n\
int g1 = 3; // global variable (24)\r\n\
int r; // (28)\r\n\
{\r\n\
  int g2 = 5; // block variable (32)\r\n\
  int test(int x, int rek)\r\n\
  {\r\n\
    int g3 = 7; // rekursive function local variable (36?, 40?)\r\n\
    int help(int x)\r\n\
    {\r\n\
      return g3*x+g2*g1;\r\n\
    }\r\n\
    if(rek)\r\n\
      return help(x) * g2 + g1; // (7*3+15)*5+3 = 183\r\n\
    return help(x) * test(g1, 1); // (7*13+15) * 183\r\n\
  }\r\n\
  r = test(13, 0);\r\n\
}\r\n\
\r\n\
return r; // 19398";
	printf("\r\nLocal function context test\r\n");
	for(int t = 0; t < 2; t++)
	{
		testCount[t]++;
		if(RunCode(testLocalFunc1, testTarget[t], "19398"))
		{
			lastFailed = false;
			CHECK_INT("g1", 0, 3);
			CHECK_INT("r", 0, 19398);
			if(!lastFailed)
				passed[t]++;
		}
	}


const char	*testLocalFunc2 = 
"// Local function context test 2\r\n\
int g1 = 3; // global variable (24)\r\n\
int r; // (28)\r\n\
{\r\n\
  // test should get pointer to this variable\r\n\
  int g2 = 5; // block variable (32)\r\n\
  int test(int x, int rek)\r\n\
  {\r\n\
    // help should get pointer to this variable\r\n\
    int g3 = 7; // recursive function local variable (36?, 40?)\r\n\
    int help(int x)\r\n\
    {\r\n\
      int hell = 0;\r\n\
      if(x > 10)\r\n\
        hell = test(2, 0);\r\n\
      return g3*x + g2*g1 + hell;\r\n\
    }\r\n\
    if(rek)\r\n\
      return help(x) * g2 + g1; // (7*3+15)*5+3 = 183\r\n\
    return help(x) * test(g1, 1); // (7*13+15+(7*2+15+0)*((7*3+15+0)*5+3)) * 183\r\n\
  }\r\n\
  r = test(13, 0);\r\n\
}\r\n\
\r\n\
return r; // 990579";
	printf("\r\nLocal function context test 2\r\n");
	for(int t = 0; t < 2; t++)
	{
		testCount[t]++;
		if(RunCode(testLocalFunc2, testTarget[t], "990579"))
		{
			lastFailed = false;
			CHECK_INT("g1", 0, 3);
			CHECK_INT("r", 0, 990579);
			if(!lastFailed)
				passed[t]++;
		}
	}



const char	*testStrings = 
"int test(char[] text){ return 2; }\r\n\
int[10] arr;\r\n\
auto hm = \"World\\r\\n\";\r\n\
char[] nm = hm;\r\n\
char[] um = nm;\r\n\
arr[test(\"hello\\r\\n\")] += 3;\r\n\
return 1;";
	printf("\r\nStrings test\r\n");
	for(int t = 0; t < 2; t++)
	{
		testCount[t]++;
		if(RunCode(testStrings, testTarget[t], "1"))
		{
			lastFailed = false;
			CHECK_STR("hm", 0, "World\r\n");
			CHECK_STR("$temp1", 0, "hello\r\n");

			CHECK_INT("nm", 1, 8);

			CHECK_INT("um", 1, 8);
			if(!lastFailed)
				passed[t]++;
		}
	}


const char	*testMultiCtor = 
"// Multidimensional array constructor test\r\n\
import std.math;\r\n\
float3 float3(int[3] comp){ float3 ret; ret.x = comp[0]; ret.y = comp[1]; ret.z = comp[2]; return ret; }\r\n\
float3 float3(float[3] comp){ float3 ret; ret.x = comp[0]; ret.y = comp[1]; ret.z = comp[2]; return ret; }\r\n\
float3 float3(double[3] comp){ float3 ret; ret.x = comp[0]; ret.y = comp[1]; ret.z = comp[2]; return ret; }\r\n\
float3 g = float3({4.0, 6.0, 8.0}), h = float3({3.0f, 5.0f, 7.0f}), j = float3({11, 12, 13});\r\n\
\r\n\
auto a = { 3, 4, 5 };\r\n\
auto b = { {11, 12}, {14, 15} };\r\n\
\r\n\
auto c = { g, h, j };\r\n\
auto d = { { {g}, {h} }, { {h}, {j} } };\r\n\
\r\n\
auto f = { 1.0f, 2.0f };\r\n\
int[] k;\r\n\
k = { 1, 2, 3, 4 };\r\n\
return 1;";
	printf("\r\nMultidimensional array constructor test\r\n");
	for(int t = 0; t < 2; t++)
	{
		testCount[t]++;
		if(RunCode(testMultiCtor, testTarget[t], "1"))
		{
			lastFailed = false;

			CHECK_FLOAT("g", 0, 4.0f);
			CHECK_FLOAT("g", 1, 6.0f);
			CHECK_FLOAT("g", 2, 8.0f);

			CHECK_FLOAT("h", 0, 3.0f);
			CHECK_FLOAT("h", 1, 5.0f);
			CHECK_FLOAT("h", 2, 7.0f);

			CHECK_FLOAT("j", 0, 11.0f);
			CHECK_FLOAT("j", 1, 12.0f);
			CHECK_FLOAT("j", 2, 13.0f);

			CHECK_INT("a", 0, 3);
			CHECK_INT("a", 1, 4);
			CHECK_INT("a", 2, 5);

			CHECK_INT("b", 0, 11);
			CHECK_INT("b", 1, 12);
			CHECK_INT("b", 2, 14);
			CHECK_INT("b", 3, 15);

			CHECK_FLOAT("c", 0, 4.0f);
			CHECK_FLOAT("c", 1, 6.0f);
			CHECK_FLOAT("c", 2, 8.0f);
			CHECK_FLOAT("c", 3, 3.0f);
			CHECK_FLOAT("c", 4, 5.0f);
			CHECK_FLOAT("c", 5, 7.0f);
			CHECK_FLOAT("c", 6, 11.0f);
			CHECK_FLOAT("c", 7, 12.0f);
			CHECK_FLOAT("c", 8, 13.0f);

			CHECK_FLOAT("d", 0, 4.0f);
			CHECK_FLOAT("d", 1, 6.0f);
			CHECK_FLOAT("d", 2, 8.0f);
			CHECK_FLOAT("d", 3, 3.0f);
			CHECK_FLOAT("d", 4, 5.0f);
			CHECK_FLOAT("d", 5, 7.0f);
			CHECK_FLOAT("d", 6, 3.0f);
			CHECK_FLOAT("d", 7, 5.0f);
			CHECK_FLOAT("d", 8, 7.0f);
			CHECK_FLOAT("d", 9, 11.0f);
			CHECK_FLOAT("d", 10, 12.0f);
			CHECK_FLOAT("d", 11, 13.0f);

			CHECK_DOUBLE("f", 0, 1.0);
			CHECK_DOUBLE("f", 1, 2.0);

			CHECK_INT("k", 1, 4);

			if(!lastFailed)
				passed[t]++;
		}
	}


const char	*testHexConst = 
"//Hexademical constants\r\n\
auto a = 0xdeadbeef;\r\n\
auto b = 0xcafe;\r\n\
auto c = 0x7fffffffffffffff;\r\n\
return a;";
	printf("\r\nHexademical constants\r\n");
	for(int t = 0; t < 2; t++)
	{
		testCount[t]++;
		if(RunCode(testHexConst, testTarget[t], "-559038737"))
		{
			lastFailed = false;

			CHECK_INT("a", 0, -559038737);
			CHECK_INT("b", 0, 51966);
			CHECK_LONG("c", 0, 9223372036854775807ll);

			if(!lastFailed)
				passed[t]++;
		}
	}

const char	*testGetSet = 
"// New get and set functions test\r\n\
import std.math;\r\n\
auto s1 = 5;\r\n\
auto f1 = &s1;\r\n\
int d1 = 3;\r\n\
int ref df1 = &d1;\r\n\
\r\n\
auto k = { 1,4,8,64,7};\r\n\
auto kr = { &k[4], &k[3], &k[2], &k[1], &k[0] };\r\n\
auto krr = &kr;\r\n\
auto t1 = 5;\r\n\
float4 t2;\r\n\
t2.x = 6;\r\n\
t2.y = 4;\r\n\
float4[4] t3;\r\n\
t3[1].x = 5;\r\n\
t3[2].z = 14.0;\r\n\
auto t4 = &t1;\r\n\
auto t5 = &t2;\r\n\
auto t6 = &t3;\r\n\
float4 ref[4] t7;\r\n\
t7[0] = &t3[1];\r\n\
class Test{ int ref x, y; }\r\n\
Test t8;\r\n\
t8.x = &s1;\r\n\
t8.y = &t1;\r\n\
Test[3] t9;\r\n\
t9[0].x = &s1;t9[0].y = &s1;t9[1].x = &s1;t9[1].y = &s1;t9[2].x = &s1;\r\n\
t9[2].y = &t1;\r\n\
auto t10 = &t8;\r\n\
\r\n\
float er2, er1 = 2 * (er2 = t2.y);\r\n\
//gets\r\n\
int r1 = t1;	// 5\r\n\
int r2 = k[2];	// 8\r\n\
float r3s, r3 = r3s = t2.y;	// 4\r\n\
float r4 = t3[2].z;	// 14\r\n\
int r5 = *t4;	// 5\r\n\
int r6 = *kr[2];	// 8\r\n\
float r7 = t5.y;	// 4\r\n\
int r8 = *t8.y;	// 5\r\n\
int r9 = *t9[2].y;	// 5\r\n\
//float r10 = (*t7)[2].z;\r\n\
int r11 = *t10.y; // 5\r\n\
float4 t11 = *t7[0];\r\n\
\r\n\
//sets in reverse\r\n\
t1 = r1;\r\n\
k[2] = r2;\r\n\
t2.y = r3;\r\n\
t3[2].z = r4;\r\n\
*t4 = r5;\r\n\
*kr[2] = r6;\r\n\
t5.y = r7;\r\n\
*t8.y = r8;\r\n\
*t9[2].y = r9;\r\n\
*t10.y = r11;\r\n\
*t7[0] = t11;\r\n\
\r\n\
int i = 1, j = 2;\r\n\
auto spec = {{1,2,3},{4,5,6},{7,8,9}};\r\n\
int r2a = spec[2][1]; // 8\r\n\
int r2b = spec[j][1]; // 8\r\n\
int r2c = spec[1][j]; // 6\r\n\
int r2d = spec[i][j]; // 6\r\n\
{\r\n\
auto d2 = *f1;\r\n\
auto d3 = f1;\r\n\
d1 = d2;\r\n\
df1 = d3;\r\n\
}\r\n\
return 1;";
	printf("\r\nNew get and set functions test\r\n");
	for(int t = 0; t < 2; t++)
	{
		testCount[t]++;
		if(RunCode(testGetSet, testTarget[t], "1"))
		{
			lastFailed = false;

			CHECK_INT("s1", 0, 5);
			CHECK_INT("d1", 0, 5);

			CHECK_INT("k", 0, 1);
			CHECK_INT("k", 1, 4);
			CHECK_INT("k", 2, 8);
			CHECK_INT("k", 3, 64);
			CHECK_INT("k", 4, 7);

			CHECK_INT("t1", 0, 5);

			CHECK_FLOAT("t2", 0, 6.0f);
			CHECK_FLOAT("t2", 1, 4.0f);

			CHECK_FLOAT("t3", 4, 5.0f);
			CHECK_FLOAT("t3", 10, 14.0f);

			CHECK_FLOAT("er2", 0, 4.0f);
			CHECK_FLOAT("er1", 0, 8.0f);

			CHECK_INT("r1", 0, 5);
			CHECK_INT("r2", 0, 8);

			CHECK_FLOAT("r3s", 0, 4.0f);
			CHECK_FLOAT("r3", 0, 4.0f);
			CHECK_FLOAT("r4", 0, 14.0f);

			CHECK_INT("r5", 0, 5);
			CHECK_INT("r6", 0, 8);

			CHECK_FLOAT("r7", 0, 4.0f);

			CHECK_INT("r8", 0, 5);
			CHECK_INT("r9", 0, 5);
			CHECK_INT("r11", 0, 5);

			CHECK_FLOAT("t11", 0, 5.0f);

			CHECK_INT("i", 0, 1);
			CHECK_INT("j", 0, 2);

			CHECK_INT("spec", 0, 1);
			CHECK_INT("spec", 1, 2);
			CHECK_INT("spec", 2, 3);
			CHECK_INT("spec", 3, 4);
			CHECK_INT("spec", 4, 5);
			CHECK_INT("spec", 5, 6);
			CHECK_INT("spec", 6, 7);
			CHECK_INT("spec", 7, 8);
			CHECK_INT("spec", 8, 9);

			CHECK_INT("r2a", 0, 8);
			CHECK_INT("r2b", 0, 8);
			CHECK_INT("r2c", 0, 6);
			CHECK_INT("r2d", 0, 6);

			if(!lastFailed)
				passed[t]++;
		}
	}


const char	*testSizeof = 
"//sizeof tests\r\n\
import std.math;\r\n\
int t1 = sizeof(int); // 4\r\n\
int t2 = sizeof(float4); // 16\r\n\
int t3 = sizeof({4,5,5}); // 12\r\n\
int t4 = sizeof(t3); // 4\r\n\
int t5 = sizeof(t2*0.5); // 8\r\n\
return 1;";
	printf("\r\nsizeof tests\r\n");
	for(int t = 0; t < 2; t++)
	{
		testCount[t]++;
		if(RunCode(testSizeof, testTarget[t], "1"))
		{
			lastFailed = false;

			CHECK_INT("t1", 0, 4);
			CHECK_INT("t2", 0, 16);
			CHECK_INT("t3", 0, 12);
			CHECK_INT("t4", 0, 4);
			CHECK_INT("t5", 0, 8);

			if(!lastFailed)
				passed[t]++;
		}
	}


const char	*testTypeof = 
"//typeof tests\r\n\
int test(float a, double b){ return a+b; }\r\n\
int funcref(float a, double b, typeof(test) op){ return op(a, b); }\r\n\
\r\n\
int i=1;\r\n\
auto m = test; // int ref(float, double)\r\n\
typeof(i) i2 = 4.0f; // int\r\n\
typeof(m) n = m; // int ref(float, double)\r\n\
typeof(i*0.2) d = 3; // double\r\n\
typeof(funcref) fr = funcref; // int ref(float, double, int ref(float, double))\r\n\
int rr = sizeof(typeof(fr));\r\n\
\r\n\
return 1;";
	printf("\r\ntypeof tests\r\n");
	for(int t = 0; t < 2; t++)
	{
		testCount[t]++;
		if(RunCode(testTypeof, testTarget[t], "1"))
		{
			lastFailed = false;

			CHECK_INT("i", 0, 1);
			CHECK_INT("m", 0, 0);
			CHECK_INT("i2", 0, 4);
			CHECK_INT("n", 0, 0);

			CHECK_DOUBLE("d", 0, 3.0f);
			CHECK_INT("fr", 0, 0);
			CHECK_INT("rr", 0, 8);

			if(!lastFailed)
				passed[t]++;
		}
	}


const char	*testClassMethod = 
"// Class method test\r\n\
import std.math;\r\n\
class vec3\r\n\
{\r\n\
float x, y, z;\r\n\
void vec3(float xx, float yy, float zz){ x = xx; y = yy; z = zz; }\r\n\
\r\n\
float length(){ return sqrt(x*x+y*y+z*z); }\r\n\
float dot(vec3 ref v){ return x*v.x+y*v.y+z*v.z; }\r\n\
\r\n\
float w;\r\n\
}\r\n\
\r\n\
return 1;";
	printf("\r\nClass method test\r\n");
	for(int t = 0; t < 2; t++)
	{
		testCount[t]++;
		if(RunCode(testClassMethod, testTarget[t], "1"))
		{
			lastFailed = false;
			if(!lastFailed)
				passed[t]++;
		}
	}


const char	*testClosure = 
"// Closure test\r\n\
\r\n\
int global = 100;\r\n\
int test(int a, int c){ return global + a * c; }\r\n\
\r\n\
int proxy(int a, int c, typeof(test) foo){ return foo(a, c); }\r\n\
\r\n\
auto fp = test;\r\n\
int res1a, res1b, res1c, res1d;\r\n\
res1a = test(2, 3);\r\n\
res1b = fp(2, 4);\r\n\
res1c = proxy(2, 5, test);\r\n\
res1d = proxy(2, 6, fp);\r\n\
int res2a, res2b, res2c, res2d;\r\n\
{\r\n\
  int local = 200;\r\n\
  int localtest(int a, int c){ return local + a * c; }\r\n\
\r\n\
  auto fp2 = localtest;\r\n\
  res2a = localtest(2, 3);\r\n\
  res2b = fp2(2, 4);\r\n\
  res2c = proxy(2, 5, localtest);\r\n\
  res2d = proxy(2, 6, fp2);\r\n\
}\r\n\
\r\n\
class Del\r\n\
{\r\n\
  int classLocal;\r\n\
\r\n\
  int test(int a, int c){ return classLocal + a * c; }\r\n\
}\r\n\
Del cls;\r\n\
cls.classLocal = 300;\r\n\
auto fp3 = cls.test;\r\n\
int res3a, res3b, res3c, res3d;\r\n\
res3a = cls.test(2, 3);\r\n\
res3b = fp3(2, 4);\r\n\
res3c = proxy(2, 5, cls.test);\r\n\
res3d = proxy(2, 6, fp3);\r\n\
\r\n\
int foobar(int ref(int[3]) f) { int[3] x; x = {5, 6, 7}; return f(x); }\r\n\
int result;\r\n\
{\r\n\
  int local1 = 5, local2 = 2;\r\n\
  typeof(foobar) local3 = foobar;\r\n\
\r\n\
  int bar(int ref(int[3]) x){ return local3(x); }\r\n\
  int foo(int[3] x){ return local1 + x[local2]; }\r\n\
\r\n\
  result = bar(foo);\r\n\
}\r\n\
\r\n\
char ee;\r\n\
return 1;";
	printf("\r\nClosure test\r\n");
	for(int t = 0; t < 2; t++)
	{
		testCount[t]++;
		if(RunCode(testClosure, testTarget[t], "1"))
		{
			lastFailed = false;

			CHECK_INT("global", 0, 100);
			CHECK_INT("fp", 0, 0);
			CHECK_INT("res1a", 0, 106);
			CHECK_INT("res1b", 0, 108);
			CHECK_INT("res1c", 0, 110);
			CHECK_INT("res1d", 0, 112);

			CHECK_INT("res2a", 0, 206);
			CHECK_INT("res2b", 0, 208);
			CHECK_INT("res2c", 0, 210);
			CHECK_INT("res2d", 0, 212);

			//CHECK_INT("fp3", 0, 60);
			CHECK_INT("res3a", 0, 306);
			CHECK_INT("res3b", 0, 308);
			CHECK_INT("res3c", 0, 310);
			CHECK_INT("res3d", 0, 312);

			CHECK_INT("result", 0, 12);

			if(!lastFailed)
				passed[t]++;
		}
	}


const char	*testClosure2 = 
"int test(int n, int ref(int) ptr){ return ptr(n); }\r\n\
\r\n\
int n = 5;\r\n\
auto a = int lambda(int b){ return b + 5; };\r\n\
auto b = int lambda(int b){ return b + n; };\r\n\
\r\n\
int res1 = test(3, int lambda(int b){ return b+n; });\r\n\
\r\n\
int resA, resB, resC, resD, resE, resF;\r\n\
{\r\n\
  int d = 7;\r\n\
  int ref(int) a0, a1;\r\n\
  a0 = int lambda(int b){ return b + 2; };\r\n\
  a1 = int lambda(int b){ return b + d; };\r\n\
\r\n\
  resA = a0(3);\r\n\
  resB = a1(3);\r\n\
\r\n\
  auto b0 = int lambda(int b){ return b + 2; };\r\n\
  auto b1 = int lambda(int b){ return b + d; };\r\n\
\r\n\
  resC = b0(4);\r\n\
  resD = b1(4);\r\n\
\r\n\
  resE = test(5, int lambda(int b){ return b + 2; });\r\n\
  resF = test(5, int lambda(int b){ return b + d; });\r\n\
}\r\n\
int c=0;\r\n\
\r\n\
return 1;";
	printf("\r\nClosure test 2\r\n");
	for(int t = 0; t < 2; t++)
	{
		testCount[t]++;
		if(RunCode(testClosure2, testTarget[t], "1"))
		{
			lastFailed = false;

			CHECK_INT("n", 0, 5);
			CHECK_INT("a", 0, 0);
			CHECK_INT("b", 0, 0);

			CHECK_INT("res1", 0, 8);
			CHECK_INT("resA", 0, 5);
			CHECK_INT("resB", 0, 10);
			CHECK_INT("resC", 0, 6);

			CHECK_INT("resD", 0, 11);
			CHECK_INT("resE", 0, 7);
			CHECK_INT("resF", 0, 12);
			CHECK_INT("c", 0, 0);

			if(!lastFailed)
				passed[t]++;
		}
	}


const char	*testClosure3 = 
"int main()\r\n\
{\r\n\
  int test(int n, int ref(int) ptr){ return ptr(n); }\r\n\
\r\n\
  int n = 5;\r\n\
  int res1 = test(3, int lambda(int b){ return b+n; });\r\n\
  return res1;\r\n\
}\r\n\
\r\n\
return main();";
	printf("\r\nClosure test 3\r\n");
	for(int t = 0; t < 2; t++)
	{
		testCount[t]++;
		if(RunCode(testClosure3, testTarget[t], "8"))
		{
			lastFailed = false;

			if(!lastFailed)
				passed[t]++;
		}
	}

const char	*testClosure4 = 
"int a = 0;\r\n\
{\r\n\
int ff(int ref() f){ return f(); }\r\n\
a = 1 + ff(int f1(){ return 1 + ff(int f2(){ return 1; }); });\r\n\
}\r\n\
int ff(int ref() f){ return f(); }\r\n\
int b = 1 + ff(int f1(){ return 1 + ff(int f2(){ return 1 + ff(int f3(){ return 1; }); }); });\r\n\
return a+b;";
	printf("\r\nClosure test 4\r\n");
	for(int t = 0; t < 2; t++)
	{
		testCount[t]++;
		if(RunCode(testClosure4, testTarget[t], "7"))
		{
			lastFailed = false;

			if(!lastFailed)
				passed[t]++;
		}
	}

const char	*testClosure5 = 
"int r1, r2, r3, r4, r5;\r\n\
{\r\n\
int ff(int ref() f){ return f(); }\r\n\
int ref() a1 = int f1(){ return 1 + ff(int f2(){ return 1; }); };\r\n\
int ref() a2;\r\n\
a2 = int f3(){ return 1 + ff(int f4(){ return 1; }); };\r\n\
r1 = ff(int f5(){ return 1 + ff(int f6(){ return 1; }); });\r\n\
r2 = ff(a1);\r\n\
r3 = ff(a2);\r\n\
r4 = a1();\r\n\
r5 = a2();\r\n\
}\r\n\
return 1;";
	printf("\r\nClosure test 5\r\n");
	for(int t = 0; t < 2; t++)
	{
		testCount[t]++;
		if(RunCode(testClosure5, testTarget[t], "1"))
		{
			lastFailed = false;

			CHECK_INT("r1", 0, 2);
			CHECK_INT("r2", 0, 2);
			CHECK_INT("r3", 0, 2);
			CHECK_INT("r4", 0, 2);
			CHECK_INT("r5", 0, 2);

			if(!lastFailed)
				passed[t]++;
		}
	}

const char	*testPriority = 
"int func(){}\r\n\
int a = 13, b = 17, c = 14;\r\n\
int[10] res;\r\n\
res[0] = a + b * c;\r\n\
res[1] = a + b ** (c-10) * a;\r\n\
return 1;";
	printf("\r\nOperation priority test\r\n");
	for(int t = 0; t < 2; t++)
	{
		testCount[t]++;
		if(RunCode(testPriority, testTarget[t], "1"))
		{
			lastFailed = false;
			CHECK_INT("a", 0, 13);
			CHECK_INT("b", 0, 17);
			CHECK_INT("c", 0, 14);
			int resExp[] = { 251, 1085786, -14, -4, 42, 4, 2, 2744, 1, 0, 1, 0, 0, 1, 112, 1, 2, 15, 13, 1, 1, 0, 0, 1, 1, 0, 1 };
			for(int i = 0; i < 2; i++)
				CHECK_INT("res", i, resExp[i]);
			if(!lastFailed)
				passed[t]++;
		}
	}

const char	*testAutoReturn = 
"auto f1(){ }\r\n\
auto f2(){ return 3; }\r\n\
return f2();";
	printf("\r\nAuto return type tests\r\n");
	for(int t = 0; t < 2; t++)
	{
		testCount[t]++;
		if(RunCode(testAutoReturn, testTarget[t], "3"))
		{
			lastFailed = false;
			if(!lastFailed)
				passed[t]++;
		}
	}

const char	*testDepthBreakContinue = 
"int i, k = 0;\r\n\
for(i = 0; i < 4; i++)\r\n\
{\r\n\
	for(int j = 0;j < 4;j++)\r\n\
	{\r\n\
		if(j == 2 && i == 2)\r\n\
			break 2;\r\n\
		k++;\r\n\
	}\r\n\
}\r\n\
int a = k;\r\n\
k = 0;\r\n\
for(i = 0; i < 4;i ++)\r\n\
{\r\n\
	for(int j = 0; j < 4; j++)\r\n\
	{\r\n\
		if(j == 2 && i == 2)\r\n\
			continue 2;\r\n\
		k++;\r\n\
	}\r\n\
}\r\n\
int b = k;\r\n\
return a + b;";
	printf("\r\nMulti-depth break and continue\r\n");
	for(int t = 0; t < 2; t++)
	{
		testCount[t]++;
		if(RunCode(testDepthBreakContinue, testTarget[t], "24"))
		{
			lastFailed = false;
			CHECK_INT("a", 0, 10);
			CHECK_INT("b", 0, 14);
			if(!lastFailed)
				passed[t]++;
		}
	}

const char	*testMissingTests = 
"long a1 = 01, a2 = 0377, a3 = 01777777, a4 = 017777777777777777;\r\n\
long b1 = 0b, b2 = 1b, b3 = 1111111111100001010100101b, b4 = 101010101101010101011010101010011101011010010101110101010101001b;\r\n\
\r\n\
double c1 = 1e-3, c2 = 1e6, c3=123e2, c4=0.121e-4;\r\n\
\r\n\
char[] d1 = \"\\\\\\\'\\0\";\r\n\
\r\n\
int d2 = !4, d3 = ~5, d4 = -12;\r\n\
float e2 = -1.0f;\r\n\
double e3 = -3.0;\r\n\
long e4 = !324324234324234423l, e5 = ~89435763476541l, e6 = -1687313675313735l;\r\n\
int f1 = 2 << 4;\r\n\
long f2 = 3l << 12l;\r\n\
\r\n\
int f4 = 0 - f1;\r\n\
double f5 = 2 * 3.0, f6 = f1 - 0.0;\r\n\
\r\n\
return 1;";
	printf("\r\nGroup of tests\r\n");
	for(int t = 0; t < 2; t++)
	{
		testCount[t]++;
		if(RunCode(testMissingTests, testTarget[t], "1"))
		{
			lastFailed = false;
			CHECK_LONG("a1", 0, 1);
			CHECK_LONG("a2", 0, 255);
			CHECK_LONG("a3", 0, 524287);
			CHECK_LONG("a4", 0, 562949953421311ll);

			CHECK_LONG("b1", 0, 0);
			CHECK_LONG("b2", 0, 1);
			CHECK_LONG("b3", 0, 33538725);
			CHECK_LONG("b4", 0, 6154922420991617705ll);

			CHECK_DOUBLE("c1", 0, 1e-3);
			CHECK_DOUBLE("c2", 0, 1e6);
			CHECK_DOUBLE("c3", 0, 123e2);
			CHECK_DOUBLE("c4", 0, 0.121e-4);

			CHECK_INT("d1", 1, 4);

			CHECK_CHAR("$temp1", 0, '\\');
			CHECK_CHAR("$temp1", 1, '\'');
			CHECK_CHAR("$temp1", 2, 0);
			CHECK_CHAR("$temp1", 3, 0);

			CHECK_INT("d2", 0, !4);
			CHECK_INT("d3", 0, ~5);
			CHECK_INT("d4", 0, -12);

			CHECK_FLOAT("e2", 0, -1.0f);
			CHECK_DOUBLE("e3", 0, -3.0);

			CHECK_LONG("e4", 0, !324324234324234423ll);
			CHECK_LONG("e5", 0, ~89435763476541ll);
			CHECK_LONG("e6", 0, -1687313675313735ll);

			CHECK_INT("f1", 0, 2 << 4);
			CHECK_LONG("f2", 0, 3ll << 12ll);

			CHECK_INT("f4", 0, -(2 << 4));
			CHECK_DOUBLE("f5", 0, 6.0);
			CHECK_DOUBLE("f6", 0, (2 << 4));

			if(!lastFailed)
				passed[t]++;
		}
	}

const char	*testMissingTests2 =
"auto a = {1,2,3,4};\r\n\
int[] b = a;\r\n\
int[] ref c = &b;\r\n\
int d = c[2];\r\n\
int e = a.size;\r\n\
\r\n\
int[] f = *c;\r\n\
int[4] ref g = &a;\r\n\
int[] h = *g;\r\n\
int j = g.size;\r\n\
\r\n\
auto n1 = auto(int a){ return -a; };\r\n\
auto n2 = auto(){}, n4 = int ff(){};\r\n\
int ref(int) n1_ = n1;\r\n\
void ref() n2_ = n2;\r\n\
int ref(int) n11 = int n11f(int a){ return ~a; }, n33;\r\n\
void ref() n22 = void n22f(){}, n44;\r\n\
int k1 = n1(5), k2 = n1_(12), k3 = n11(7);\r\n\
n33 = auto(int n){ return n*1024; };\r\n\
n44 = auto(){};\r\n\
int k4 = n33(4);\r\n\
n44();\r\n\
return 1;";
	printf("\r\nGroup of tests 2\r\n");
	for(int t = 0; t < 2; t++)
	{
		testCount[t]++;
		if(RunCode(testMissingTests2, testTarget[t], "1"))
		{
			lastFailed = false;
			CHECK_INT("d", 0, 3);
			CHECK_INT("e", 0, 4);

			CHECK_INT("k1", 0, -5);
			CHECK_INT("k2", 0, -12);
			CHECK_INT("k3", 0, ~7);
			CHECK_INT("k4", 0, 4096);

			if(!lastFailed)
				passed[t]++;
		}
	}

const char	*testMissingTests3 =
"int max(int a, b, c){ return a > b ? (a > c ? a : c) : (b > c ? b : c); }\r\n\
int max(int a, b){ if(a > b) return a; else return b; }\r\n\
int max2(int a, b, c){ if(a > b) return max(a, c); else return max(b, c); }\r\n\
int a1 = max(1, 2, 3), a2 = max(1, 0, 2), a3 = max(7, 6, 5);\r\n\
int b1 = max2(1, 2, 3), b2 = max2(1, 0, 2), b3 = max2(7, 6, 5);\r\n\
\r\n\
int c1 = 1;\r\n\
do{\r\n\
c1 *= 2;\r\n\
}while(c1 < 500);\r\n\
\r\n\
class A{ char x, y; }\r\n\
\r\n\
int opt1 = ((12 / 3) < 8) != ((5 >= 7) == (5 <= 3)) + (18 >> 2) % (4 & 4 ^ 0 | 0xffff) + (5 && 1 || 3 ^^ 2);\r\n\
long opt2 = 18l >> 2l % 5l ^ 9l | 12l & 13l;\r\n\
long opt3 = 1l && 1l || 1l ^^ 1l;\r\n\
double opt4 = 8.0 % 3.0;\r\n\
return 1;";
	printf("\r\nGroup of tests 3\r\n");
	for(int t = 0; t < 2; t++)
	{
		testCount[t]++;
		if(RunCode(testMissingTests3, testTarget[t], "1"))
		{
			lastFailed = false;
			
			CHECK_INT("a1", 0, 3);
			CHECK_INT("a2", 0, 2);
			CHECK_INT("a3", 0, 7);

			CHECK_INT("b1", 0, 3);
			CHECK_INT("b2", 0, 2);
			CHECK_INT("b3", 0, 7);

			CHECK_INT("c1", 0, 512);

			CHECK_INT("opt1", 0, (1 != 1 + (18 >> 2) % (4 & 4 ^ 0 | 0xffff) + 1) ? 1 : 0);
			CHECK_LONG("opt2", 0, 18l >> 2l % 5l ^ 9l | 12l & 13l);
			CHECK_LONG("opt3", 0, 1);

			CHECK_DOUBLE("opt4", 0, 2.0);

			if(!lastFailed)
				passed[t]++;
		}
	}

const char	*testMissingTests4 =
"class matrix\r\n\
{\r\n\
    float[] arr;\r\n\
    int width, height;\r\n\
}\r\n\
matrix matrix(int width, height){ matrix ret; ret.arr = new float[width * height]; ret.width = width; ret.height = height; return ret; }\r\n\
class row\r\n\
{\r\n\
    matrix ref parent;\r\n\
    int x;\r\n\
}\r\n\
row operator[](matrix ref a, int x){ row ret; ret.x = x; ret.parent = a; return ret; }\r\n\
float ref operator[](row a, int y){ return &a.parent.arr[y * a.parent.width + a.x]; }\r\n\
\r\n\
matrix m = matrix(4, 4);\r\n\
m[1][3] = 4;\r\n\
\r\n\
int operator+(float a, int n){ return int(a) + n; }\r\n\
\r\n\
int min(int a, b){ return a < b ? a : b; }\r\n\
int ref rsize = new int;\r\n\
int[] operator+(int[] a, b){ int[] res = new int[min(a.size, b.size)]; for(int i = 0; i < res.size; i++) res[i] = a[i] + b[i]; *rsize = res.size; return res; }\r\n\
auto k = { 1, 2, 3, 4 } + { 4, 2, 5, 3 };\r\n\
int[] pass(int[] a){ return a; }\r\n\
auto i = pass({ 4, 7 });\r\n\
int k1 = k[0], k2 = k[1], k3 = k[2], k4 = k[3];\r\n\
return m[1][3] + 2;";
	printf("\r\nGroup of tests 4\r\n");
	for(int t = 0; t < 2; t++)
	{
		testCount[t]++;
		if(RunCode(testMissingTests4, testTarget[t], "6"))
		{
			lastFailed = false;

			CHECK_INT("$temp1", 0, 1);
			CHECK_INT("$temp1", 1, 2);
			CHECK_INT("$temp1", 2, 3);
			CHECK_INT("$temp1", 3, 4);

			CHECK_INT("$temp2", 0, 4);
			CHECK_INT("$temp2", 1, 2);
			CHECK_INT("$temp2", 2, 5);
			CHECK_INT("$temp2", 3, 3);

			CHECK_INT("$temp3", 0, 4);
			CHECK_INT("$temp3", 1, 7);

			CHECK_INT("k", 1, 4);
			CHECK_INT("i", 1, 2);

			CHECK_INT("k1", 0, 5);
			CHECK_INT("k2", 0, 4);
			CHECK_INT("k3", 0, 8);
			CHECK_INT("k4", 0, 7);

			if(!lastFailed)
				passed[t]++;
		}
	}

const char	*testMissingTests5 =
"long a = 3, a2 = 1;\r\n\
long b1 = a ** 27;\r\n\
long b2 = a2 ** 16884;\r\n\
long b3 = a ** -0;\r\n\
long b4 = a ** -1;\r\n\
char f;\r\n\
char ref k(){ align(16) char a; return &a; }\r\n\
char ref r(){ align(16) char a; return k(); }\r\n\
align(16) char ref i = r();\r\n\
char ref k2(){ align(16) char a; return &a; }\r\n\
char ref r2(){ align(16) char a; return k(); }\r\n\
char ref i2 = r2();\r\n\
return 1;";
	printf("\r\nGroup of tests 5\r\n");
	for(int t = 0; t < 2; t++)
	{
		testCount[t]++;
		if(RunCode(testMissingTests5, testTarget[t], "1"))
		{
			lastFailed = false;
			
			CHECK_LONG("b1", 0, 7625597484987ll);
			CHECK_LONG("b2", 0, 1);
			CHECK_LONG("b3", 0, 1);
			CHECK_LONG("b4", 0, 0);

			if(((int*)FindVar("i"))[0] % 16 != 0)
			{
				printf(" %s Failed. i unaligned\r\n", t == 0 ? "VM" : "X86");
				lastFailed = true;
			}
			if(((int*)FindVar("i2"))[0] % 16 != 0)
			{
				printf(" %s Failed. i2 unaligned\r\n", t == 0 ? "VM" : "X86");
				lastFailed = true;
			}

			if(!lastFailed)
				passed[t]++;
		}
	}

const char	*testIndirectPointers =
"int ref(int)[2] farr;\r\n\
\r\n\
farr[0] = auto(int c){ return -c; };\r\n\
farr[1] = auto(int d){ return 2*d; };\r\n\
\r\n\
int ref(int) func(int i){ return farr[i]; }\r\n\
typeof(func) farr2 = func;\r\n\
auto getarr(){ return { farr[1], farr[0] }; }\r\n\
\r\n\
int a1 = farr[0](12);\r\n\
int a2 = farr[1](12);\r\n\
int b1 = func(0)(12);\r\n\
int b2 = func(1)(12);\r\n\
int c1 = getarr()[0](12);\r\n\
int c2 = getarr()[1](12);\r\n\
int d1 = farr2(0)(12);\r\n\
int d2 = farr2(1)(12);\r\n\
\r\n\
class Del\r\n\
{\r\n\
  int classLocal;\r\n\
\r\n\
  int test(int a){ return classLocal + a; }\r\n\
}\r\n\
Del cls;\r\n\
cls.classLocal = 300;\r\n\
farr[0] = cls.test;\r\n\
\r\n\
return (farr[0])(12);";
	printf("\r\nIndirect function pointers\r\n");
	for(int t = 0; t < 2; t++)
	{
		testCount[t]++;
		if(RunCode(testIndirectPointers, testTarget[t], "312"))
		{
			lastFailed = false;
			
			CHECK_INT("a1", 0, -12);
			CHECK_INT("a2", 0, 24);
			CHECK_INT("b1", 0, -12);
			CHECK_INT("b2", 0, 24);
			CHECK_INT("c1", 0, 24);
			CHECK_INT("c2", 0, -12);
			CHECK_INT("d1", 0, -12);
			CHECK_INT("d2", 0, 24);

			if(!lastFailed)
				passed[t]++;
		}
	}

const char	*testArrayMemberAfterCall =
"import std.math;\r\n\
float2[2] f(){ return { float2(12,13), float2(14,15) }; }\r\n\
int x = (f())[0].x;\r\n\
int y = float2(45, 98).y;\r\n\
return int(f()[1].y);";
	printf("\r\nArray and class member access after function call\r\n");
	for(int t = 0; t < 2; t++)
	{
		testCount[t]++;
		if(RunCode(testArrayMemberAfterCall, testTarget[t], "15"))
		{
			lastFailed = false;

			CHECK_INT("x", 0, 12);
			CHECK_INT("y", 0, 98);

			if(!lastFailed)
				passed[t]++;
		}
	}

const char	*testUpvalues1 =
"auto generator(int i)\r\n\
{\r\n\
	return auto(){ return i++; };\r\n\
}\r\n\
auto a = generator(3);\r\n\
int i1 = a();	// 3\r\n\
auto b = generator(3);\r\n\
int i2 = a();	// 4\r\n\
int i3 = b();	// 3\r\n\
int i4 = a();	// 5\r\n\
return b();		// 4";
	printf("\r\nClosure with upvalues test 1\r\n");
	for(int t = 0; t < 2; t++)
	{
		testCount[t]++;
		if(RunCode(testUpvalues1, testTarget[t], "4"))
		{
			lastFailed = false;

			CHECK_INT("i1", 0, 3);
			CHECK_INT("i2", 0, 4);
			CHECK_INT("i3", 0, 3);
			CHECK_INT("i4", 0, 5);

			if(!lastFailed)
				passed[t]++;
		}
	}

const char	*testUpvalues2 =
"auto binder(int x, int ref(int, int) func)\r\n\
{\r\n\
	return auto(int y){ return func(x, y); };\r\n\
}\r\n\
\r\n\
int adder(int x, y){ return x + y; }\r\n\
auto add3 = binder(3, adder);\r\n\
auto add13 = binder(13, adder);\r\n\
\r\n\
int i1 = add3(5);	// 8\r\n\
int i2 = add13(7);	// 20\r\n\
return add3(add13(4));";
	printf("\r\nClosure with upvalues test 2\r\n");
	for(int t = 0; t < 2; t++)
	{
		testCount[t]++;
		if(RunCode(testUpvalues2, testTarget[t], "20"))
		{
			lastFailed = false;

			CHECK_INT("i1", 0, 8);
			CHECK_INT("i2", 0, 20);

			if(!lastFailed)
				passed[t]++;
		}
	}

	// Test checks if values of recursive function are captured correctly and that at the end of recursive function, it closes only upvalues that target it's stack frame
const char	*testUpvalues3 =
"typedef int ref(int ref) myFunc;\r\n\
myFunc f1, f2;\r\n\
\r\n\
int k1, k2, k3 = 0;\r\n\
int dh;\r\n\
\r\n\
int func(int k)\r\n\
{\r\n\
	int d = 2;\r\n\
	int func1(int r)\r\n\
	{\r\n\
		int b = k += 3;\r\n\
		\r\n\
		int func2(int ref dr)\r\n\
		{\r\n\
			d += 300;\r\n\
			b += 2;\r\n\
			*dr = d;\r\n\
			return b;\r\n\
		}\r\n\
		if(r)\r\n\
		{\r\n\
			k1 = func1(r-1);\r\n\
			f2 = func2;\r\n\
		}else{\r\n\
			f1 = func2;\r\n\
		}\r\n\
		k3 += func2(&dh);\r\n\
		return b;\r\n\
	}\r\n\
	k2 = func1(1);\r\n\
	return d;\r\n\
}\r\n\
int ddd = func(5);\r\n\
int aa, bb;\r\n\
int a = f1(&aa);\r\n\
int b = f2(&bb);\r\n\
return 1;";
	printf("\r\nClosure with upvalues test 3\r\n");
	for(int t = 0; t < 2; t++)
	{
		testCount[t]++;
		if(RunCode(testUpvalues3, testTarget[t], "1"))
		{
			lastFailed = false;

			CHECK_INT("k1", 0, 13);
			CHECK_INT("k2", 0, 10);
			CHECK_INT("k3", 0, 23);
			CHECK_INT("dh", 0, 602);
			CHECK_INT("ddd", 0, 602);
			CHECK_INT("aa", 0, 902);
			CHECK_INT("bb", 0, 902);
			CHECK_INT("a", 0, 15);
			CHECK_INT("b", 0, 12);

			if(!lastFailed)
				passed[t]++;
		}
	}

	// Partial upvalue list closure
const char	*testUpvalues4 =
"int ref()[4] arr;\r\n\
\r\n\
int func()\r\n\
{\r\n\
	int m = 8;\r\n\
	for(int i = 0; i < 2; i++)\r\n\
	{\r\n\
		int a = 5 * (i + 1);\r\n\
		arr[i*2+0] = auto(){ return a + m; };\r\n\
		int b = i - 3;\r\n\
		arr[i*2+1] = auto(){ return b + m; };\r\n\
	}\r\n\
	m = 12;\r\n\
	return 0;\r\n\
}\r\n\
int clear()\r\n\
{\r\n\
	int[20] clr = 0;\r\n\
	int ref a = &clr[0];\r\n\
	return *a;\r\n\
}\r\n\
func();\r\n\
clear();\r\n\
int i1 = arr[0]();\r\n\
int i2 = arr[1]();\r\n\
int i3 = arr[2]();\r\n\
int i4 = arr[3]();\r\n\
return 1;";
	printf("\r\nClosure with upvalues test 4\r\n");
	for(int t = 0; t < 2; t++)
	{
		testCount[t]++;
		if(RunCode(testUpvalues4, testTarget[t], "1"))
		{
			lastFailed = false;

			CHECK_INT("i1", 0, 17);
			CHECK_INT("i2", 0, 9);
			CHECK_INT("i3", 0, 22);
			CHECK_INT("i4", 0, 10);

			if(!lastFailed)
				passed[t]++;
		}
	}

const char	*testUpvalues5 =
"int func()\r\n\
{\r\n\
	auto f = auto init() { return 1; };\r\n\
	for (int i = 0; i < 10; ++i)\r\n\
	{\r\n\
		auto temp = f;\r\n\
		f = auto next() { return 1 + temp();  };\r\n\
	}\r\n\
	auto g = f();\r\n\
	return g;\r\n\
}\r\n\
return func();";
	printf("\r\nClosure with upvalues test 5\r\n");
	for(int t = 0; t < 2; t++)
	{
		testCount[t]++;
		if(RunCode(testUpvalues5, testTarget[t], "11"))
		{
			lastFailed = false;
			if(!lastFailed)
				passed[t]++;
		}
	}

const char	*testMemberFuncCallPostExpr =
"import std.math;\r\n\
class foo\r\n\
{\r\n\
	float2 v;\r\n\
	int[3] arr;\r\n\
	auto init(){ v.x = 4; v.y = 9; arr = { 12, 14, 17 }; }\r\n\
	auto vec(){ return v; }\r\n\
	auto array(){ return arr; }\r\n\
	auto func(){ return auto(){ return *this; }; }\r\n\
	auto func2(){ return auto(){ return this; }; }\r\n\
}\r\n\
foo f;\r\n\
f.init();\r\n\
float f1 = f.vec().x;\r\n\
float f2 = f.func()().v.y;\r\n\
int i1 = f.array()[0];\r\n\
int i2 = f.func()().arr[1];\r\n\
int i3 = f.func()().func()().arr[2];\r\n\
\r\n\
float f3 = f.func2()().v.y;\r\n\
int i4 = f.func2()().arr[0];\r\n\
int i5 = f.func2()().func2()().arr[1];\r\n\
int i6 = f.func()().func2()().arr[2];\r\n\
return 1;";
	printf("\r\nMember function call post expressions\r\n");
	for(int t = 0; t < 2; t++)
	{
		testCount[t]++;
		if(RunCode(testMemberFuncCallPostExpr, testTarget[t], "1"))
		{
			lastFailed = false;

			CHECK_FLOAT("f1", 0, 4);
			CHECK_FLOAT("f2", 0, 9);
			CHECK_FLOAT("f3", 0, 9);
			CHECK_INT("i1", 0, 12);
			CHECK_INT("i2", 0, 14);
			CHECK_INT("i3", 0, 17);
			CHECK_INT("i4", 0, 12);
			CHECK_INT("i5", 0, 14);
			CHECK_INT("i6", 0, 17);

			if(!lastFailed)
				passed[t]++;
		}
	}

const char	*testLeftValueExtends =
"int v = 0;\r\n\
int v2 = 12, v4 = 15;\r\n\
auto k = { 3, 4, 9 };\r\n\
int ref f(){ return &v; }\r\n\
auto g(){ return &k; }\r\n\
f() = 5;\r\n\
g()[0] = 12;\r\n\
g()[1] = 18;\r\n\
(v2 > 5 ? &v2 : &v4) = 5;\r\n\
return 1;";
	printf("\r\nL-value extended cases\r\n");
	for(int t = 0; t < 2; t++)
	{
		testCount[t]++;
		if(RunCode(testLeftValueExtends, testTarget[t], "1"))
		{
			lastFailed = false;

			CHECK_INT("v", 0, 5);
			CHECK_INT("k", 0, 12);
			CHECK_INT("k", 1, 18);
			CHECK_INT("v2", 0, 5);
			CHECK_INT("v4", 0, 15);

			if(!lastFailed)
				passed[t]++;
		}
	}

const char	*testClassFuncReturn =
"class Test\r\n\
{\r\n\
	int i;\r\n\
	int foo(){ return i; }\r\n\
	auto bar(){ return foo; }\r\n\
}\r\n\
Test a;\r\n\
a.i = 5;\r\n\
auto k = a.bar()();\r\n\
return 1;";
	printf("\r\nClass function return\r\n");
	for(int t = 0; t < 2; t++)
	{
		testCount[t]++;
		if(RunCode(testClassFuncReturn, testTarget[t], "1"))
		{
			lastFailed = false;

			CHECK_INT("k", 0, 5);

			if(!lastFailed)
				passed[t]++;
		}
	}

const char	*testClassExternalMethodInt =
"auto int:toString()\r\n\
{\r\n\
	int copy = *this, len = 0;\r\n\
	while(copy)\r\n\
	{\r\n\
		len++;\r\n\
		copy /= 10;\r\n\
	}\r\n\
	char[] val = new char[len+1];\r\n\
	copy = *this;\r\n\
	while(copy)\r\n\
	{\r\n\
		val[--len] = copy % 10 + '0';\r\n\
		copy /= 10;\r\n\
	}\r\n\
	return val;\r\n\
}\r\n\
int n = 19;\r\n\
auto nv = n.toString();\r\n\
return nv[0] + nv[1];";
	printf("\r\nClass externally defined method (int)\r\n");
	for(int t = 0; t < 2; t++)
	{
		testCount[t]++;
		if(RunCode(testClassExternalMethodInt, testTarget[t], "106"))
		{
			lastFailed = false;

			if(!lastFailed)
				passed[t]++;
		}
	}
	
const char	*testClassMethodString =
"typedef char[] string;\r\n\
string string:reverse()\r\n\
{\r\n\
	for(int i = 0; i < (this.size-1) / 2; i++)\r\n\
	{\r\n\
		char tmp = this[i];\r\n\
		this[i] = this[this.size-i-2];\r\n\
		this[this.size-i-2] = tmp;\r\n\
	}\r\n\
	return *this;\r\n\
}\r\n\
string a = \"hello\";\r\n\
string b = a.reverse();\r\n\
return b[0] - 'o';";
	printf("\r\nClass externally defined method (char[])\r\n");
	for(int t = 0; t < 2; t++)
	{
		testCount[t]++;
		if(RunCode(testClassMethodString, testTarget[t], "0"))
		{
			lastFailed = false;

			if(!lastFailed)
				passed[t]++;
		}
	}

const char	*testClassExternalMethod =
"class Foo\r\n\
{\r\n\
	int bar;\r\n\
}\r\n\
\r\n\
int Foo:GetBar(){ return bar; }\r\n\
Foo a;\r\n\
a.bar = 14;\r\n\
\r\n\
return a.GetBar();";
	printf("\r\nClass externally defined method (custom)\r\n");
	for(int t = 0; t < 2; t++)
	{
		testCount[t]++;
		if(RunCode(testClassExternalMethod, testTarget[t], "14"))
		{
			lastFailed = false;

			if(!lastFailed)
				passed[t]++;
		}
	}

const char	*testOverloadedOperator1 =
"import std.math;\r\n\
\r\n\
void operator= (float4 ref a, float b){ a.x = a.y = a.y = a.z = b; }\r\n\
float4 a, b = 16;\r\n\
a = 12;\r\n\
return int(a.x + b.z);";
	printf("\r\nOverloaded operator =\r\n");
	for(int t = 0; t < 2; t++)
	{
		testCount[t]++;
		if(RunCode(testOverloadedOperator1, testTarget[t], "28"))
		{
			lastFailed = false;

			if(!lastFailed)
				passed[t]++;
		}
	}

const char	*testOverloadedOperator2 =
"class string{ int len; }\r\n\
void operator=(string ref a, char[] str){ a.len = str.size; }\r\n\
string b = \"assa\";\r\n\
class funcholder{ int ref(int) ptr; }\r\n\
void operator=(funcholder ref a, int ref(int) func){ a.ptr = func; }\r\n\
int test(int a){ return -a; }\r\n\
funcholder c = test;\r\n\
return (c.ptr)(12);";
	printf("\r\nOverloaded operator = with arrays and functions\r\n");
	for(int t = 0; t < 2; t++)
	{
		testCount[t]++;
		if(RunCode(testOverloadedOperator2, testTarget[t], "-12"))
		{
			lastFailed = false;

			CHECK_INT("b", 0, 5);

			if(!lastFailed)
				passed[t]++;
		}
	}

const char	*testOverloadedOperator3 =
"import std.math;\r\n\
\r\n\
void operator= (float4 ref a, float b){ a.x = a.y = a.y = a.z = b; }\r\n\
void operator+= (float4 ref a, float b){ a.x += b; a.y += b; a.y += b; a.z += b; }\r\n\
float4 a, b = 16;\r\n\
a = 12;\r\n\
a += 3;\r\n\
return int(a.x + b.z);";
	printf("\r\nOverloaded operator =\r\n");
	for(int t = 0; t < 2; t++)
	{
		testCount[t]++;
		if(RunCode(testOverloadedOperator3, testTarget[t], "31"))
		{
			lastFailed = false;

			if(!lastFailed)
				passed[t]++;
		}
	}

const char	*testMemberFuncCallRef =
"typedef char[] string;\r\n\
\r\n\
void string:set(int a)\r\n\
{\r\n\
	this[0] = a;\r\n\
}\r\n\
\r\n\
string a = \"hello\";\r\n\
auto b = &a;\r\n\
b.set('m');\r\n\
return b[0];";
	printf("\r\nMember function call of a reference to a class\r\n");
	for(int t = 0; t < 2; t++)
	{
		testCount[t]++;
		if(RunCode(testMemberFuncCallRef, testTarget[t], "109"))
		{
			lastFailed = false;

			if(!lastFailed)
				passed[t]++;
		}
	}

const char	*testInplaceArrayDouble =
"double[] arr = { 12.0, 14, 18.0f };\r\n\
return int(arr[0]+arr[1]+arr[2]);";
	printf("\r\nInplace double array with integer elements\r\n");
	for(int t = 0; t < 2; t++)
	{
		testCount[t]++;
		if(RunCode(testInplaceArrayDouble, testTarget[t], "44"))
		{
			lastFailed = false;

			if(!lastFailed)
				passed[t]++;
		}
	}

const char	*testFunctionPrototypes =
"int func1();\r\n\
int func2(){ return func1(); }\r\n\
int func1(){ return 12; }\r\n\
return func2();";
	printf("\r\nFunction prototypes\r\n");
	for(int t = 0; t < 2; t++)
	{
		testCount[t]++;
		if(RunCode(testFunctionPrototypes, testTarget[t], "12"))
		{
			lastFailed = false;

			if(!lastFailed)
				passed[t]++;
		}
	}

const char	*testInternalMemberFunctionCall =
"class Test\r\n\
{\r\n\
	int i;\r\n\
	int foo(){ return i; }\r\n\
	auto bar(){ return foo(); }\r\n\
}\r\n\
Test a;\r\n\
a.i = 5;\r\n\
auto k = a.bar();\r\n\
return k;";
	printf("\r\nInternal member function call\r\n");
	for(int t = 0; t < 2; t++)
	{
		testCount[t]++;
		if(RunCode(testInternalMemberFunctionCall, testTarget[t], "5"))
		{
			lastFailed = false;

			if(!lastFailed)
				passed[t]++;
		}
	}

const char	*testSingleArrayIndexCalculation =
"int a = 0, b = 0;\r\n\
int func(int ref v, int index){ *v += 5; return index; }\r\n\
int[2] arr;\r\n\
arr[func(&a, 0)] += 4;\r\n\
arr[func(&b, 1)]++;\r\n\
return 0;";
	printf("\r\nSingle array index calculation\r\n");
	for(int t = 0; t < 2; t++)
	{
		testCount[t]++;
		if(RunCode(testSingleArrayIndexCalculation, testTarget[t], "0"))
		{
			lastFailed = false;

			CHECK_INT("a", 0, 5);
			CHECK_INT("b", 0, 5);

			CHECK_INT("arr", 0, 4);
			CHECK_INT("arr", 1, 1);

			if(!lastFailed)
				passed[t]++;
		}
	}

const char	*testAutoReference1 =
"int a = 17;\r\n\
double b = 14.0;\r\n\
auto ref c = &a, d = &a;\r\n\
auto m = &a;\r\n\
auto n = &b;\r\n\
c = &b;\r\n\
int ref l = d;\r\n\
int p1 = l == &a;\r\n\
return *l;";
	printf("\r\nAuto reference type\r\n");
	for(int t = 0; t < 2; t++)
	{
		testCount[t]++;
		if(RunCode(testAutoReference1, testTarget[t], "17"))
		{
			lastFailed = false;

			CHECK_INT("a", 0, 17);
			CHECK_DOUBLE("b", 0, 14.0);

			CHECK_INT("p1", 0, 1);

			if(!lastFailed)
				passed[t]++;
		}
	}

const char	*testAutoReference2 =
"typedef auto ref object;\r\n\
\r\n\
void func(object p){ int ref a = p; *a = 9; }\r\n\
int k = 4;\r\n\
\r\n\
func(&k);\r\n\
\r\n\
return k;";
	printf("\r\nAuto reference type\r\n");
	for(int t = 0; t < 2; t++)
	{
		testCount[t]++;
		if(RunCode(testAutoReference2, testTarget[t], "9"))
		{
			lastFailed = false;

			if(!lastFailed)
				passed[t]++;
		}
	}

const char	*testParametersExtraordinaire =
"char func(char a, b, c){ return a+b+c; }\r\n\
auto u = func;\r\n\
int i = u(1, 7, 18);\r\n\
return func(1,7,18);";
	printf("\r\nFunction parameters with different stack type\r\n");
	for(int t = 0; t < 2; t++)
	{
		testCount[t]++;
		if(RunCode(testParametersExtraordinaire, testTarget[t], "26"))
		{
			lastFailed = false;

			CHECK_INT("i", 0, 26);

			if(!lastFailed)
				passed[t]++;
		}
	}

const char	*testExternalFunctionPtr =
"import std.math;\r\n\
auto Sqrt = sqrt;\r\n\
auto Char = char_;\r\n\
auto Short = short_;\r\n\
auto Int = int_;\r\n\
auto Long = long_;\r\n\
auto Float = float_;\r\n\
auto Double = double_;\r\n\
auto t1 = Sqrt(9.0);\r\n\
auto t2 = Char(24);\r\n\
auto t3 = Short(57);\r\n\
auto t4 = Int(2458);\r\n\
auto t5 = Long(14841324198l);\r\n\
auto t6 = Float(3.0);\r\n\
auto t7 = Double(2.0);\r\n\
return 1;";
	printf("\r\nFunction parameters with different stack type\r\n");
	for(int t = 0; t < 2; t++)
	{
		testCount[t]++;
		if(RunCode(testExternalFunctionPtr, testTarget[t], "1"))
		{
			lastFailed = false;

			CHECK_DOUBLE("t1", 0, 3.0);
			CHECK_CHAR("t2", 0, 24);
			CHECK_SHORT("t3", 0, 57);
			CHECK_INT("t4", 0, 2458);
			CHECK_LONG("t5", 0, 14841324198ll);
			CHECK_FLOAT("t6", 0, 3.0);
			CHECK_DOUBLE("t7", 0, 2.0);

			if(!lastFailed)
				passed[t]++;
		}
	}

const char	*testDefaultFuncVars1 =
"int def = 4;\r\n\
int func(int a = 5.0, b = def){ return a + b; }\r\n\
\r\n\
int a = func();\r\n\
int b = func(7);\r\n\
def = 12;\r\n\
int a2 = func();\r\n\
int b2 = func(7);\r\n\
int c = func(12, 8);\r\n\
\r\n\
return a;";
	printf("\r\nDefault function parameter values\r\n");
	for(int t = 0; t < 2; t++)
	{
		testCount[t]++;
		if(RunCode(testDefaultFuncVars1, testTarget[t], "9"))
		{
			lastFailed = false;

			CHECK_INT("a", 0, 9);
			CHECK_INT("b", 0, 11);
			CHECK_INT("a2", 0, 17);
			CHECK_INT("b2", 0, 19);
			CHECK_INT("c", 0, 20);

			if(!lastFailed)
				passed[t]++;
		}
	}

const char	*testDefaultFuncVars2 =
"auto generator(int from = 0){ return auto(){ return from++; }; }\r\n\
\r\n\
auto genEater(auto gen = generator(5))\r\n\
{\r\n\
	int ret = 0;\r\n\
	for(int i = 0; i < 3; i++)\r\n\
		ret += gen();\r\n\
	return ret;\r\n\
}\r\n\
auto u = genEater();\r\n\
auto v = genEater(generator());\r\n\
auto w = genEater(generator(8));\r\n\
\r\n\
return 0;";
	printf("\r\nDefault function parameter values 2\r\n");
	for(int t = 0; t < 2; t++)
	{
		testCount[t]++;
		if(RunCode(testDefaultFuncVars2, testTarget[t], "0"))
		{
			lastFailed = false;

			CHECK_INT("u", 0, 18);
			CHECK_INT("v", 0, 3);
			CHECK_INT("w", 0, 27);

			if(!lastFailed)
				passed[t]++;
		}
	}

const char	*testSmallClass =
"class TestS{ short a; }\r\n\
TestS b;\r\n\
b.a = 6;\r\n\
class TestC{ char a; }\r\n\
TestC c;\r\n\
c.a = 3;\r\n\
int func(TestS a){ return a.a; }\r\n\
int func(TestC a){ return a.a; }\r\n\
return func(b) + func(c);";
	printf("\r\nClass with size smaller that 4 bytes\r\n");
	for(int t = 0; t < 2; t++)
	{
		testCount[t]++;
		if(RunCode(testSmallClass, testTarget[t], "9"))
		{
			lastFailed = false;

			if(!lastFailed)
				passed[t]++;
		}
	}

const char	*testExtraSmallClass =
"class Test{ }\r\n\
Test b;\r\n\
int func(Test a, int b){ return b; }\r\n\
return func(b, 4);";
	printf("\r\nClass with size of 0 bytes\r\n");
	for(int t = 0; t < 2; t++)
	{
		testCount[t]++;
		if(RunCode(testExtraSmallClass, testTarget[t], "4"))
		{
			lastFailed = false;

			if(!lastFailed)
				passed[t]++;
		}
	}

const char	*testDefaultFuncVars3 =
"int test(auto a = auto(int i){ return i++; }, int b = 5){ return a(3) + b; }\r\n\
return test() + test(auto(int l){ return l * 2; });";
	printf("\r\nDefault function parameter values 3\r\n");
	for(int t = 0; t < 2; t++)
	{
		testCount[t]++;
		if(RunCode(testDefaultFuncVars3, testTarget[t], "19"))
		{
			lastFailed = false;

			if(!lastFailed)
				passed[t]++;
		}
	}

const char	*testPostExpressions =
"typedef char[] string;\r\n\
\r\n\
int string:find(char a)\r\n\
{\r\n\
	int i = 0;\r\n\
	while(i < this.size && this[i] != a)\r\n\
		i++;\r\n\
	if(i == this.size)\r\n\
		i = -1;\r\n\
	return i;\r\n\
}\r\n\
\r\n\
int a = (\"hello\").size + \"me\".size;\r\n\
int b = (\"hi\" + \"me\").size;\r\n\
\r\n\
int l = (\"Pota\" + \"to\").find('a');\r\n\
int l2 = (\"Potato\").find('t');\r\n\
\r\n\
auto str = \"hello\";\r\n\
int l3 = str.find('o');\r\n\
char[] str2 = \"helloworld\";\r\n\
int l4 = str.find('3');\r\n\
\r\n\
int a2 = ({1, 2, 3}).size;\r\n\
int a3 = {1, 2, 3}.size;\r\n\
int a4 = \"as\".size;\r\n\
\r\n\
return 0;";
	printf("\r\nPost expressions on arrays and strings\r\n");
	for(int t = 0; t < 2; t++)
	{
		testCount[t]++;
		if(RunCode(testPostExpressions, testTarget[t], "0"))
		{
			lastFailed = false;

			CHECK_INT("a", 0, 9);
			CHECK_INT("b", 0, 5);
			CHECK_INT("l", 0, 3);
			CHECK_INT("l2", 0, 2);
			CHECK_INT("l3", 0, 4);
			CHECK_INT("l4", 0, -1);
			CHECK_INT("a2", 0, 3);
			CHECK_INT("a3", 0, 3);
			CHECK_INT("a4", 0, 3);

			if(!lastFailed)
				passed[t]++;
		}
	}

const char	*testLogicalAnd =
"int i = 0, m = 4;\r\n\
i && (m = 3);\r\n\
return m;";
	printf("\r\nLogical && special case\r\n");
	for(int t = 0; t < 2; t++)
	{
		testCount[t]++;
		if(RunCode(testLogicalAnd, testTarget[t], "4"))
		{
			lastFailed = false;

			if(!lastFailed)
				passed[t]++;
		}
	}

const char	*testLogicalOr =
"int i = 1, m = 4;\r\n\
i || (m = 3);\r\n\
return m;";
	printf("\r\nLogical || special case\r\n");
	for(int t = 0; t < 2; t++)
	{
		testCount[t]++;
		if(RunCode(testLogicalOr, testTarget[t], "4"))
		{
			lastFailed = false;

			if(!lastFailed)
				passed[t]++;
		}
	}

const char	*testImplicitToRef =
"class float2\r\n\
{\r\n\
float x, y;\r\n\
}\r\n\
float2 float2(float x, y){ float2 ret; ret.x = x; ret.y = y; return ret; }\r\n\
\r\n\
float2 operator+(float2 ref a, float2 ref b){ return float2(a.x+b.x, a.y+b.y); }\r\n\
float2 ref operator+=(float2 ref a, float2 ref b){ a.x += b.x; a.y += b.y; return a; }\r\n\
float2 ref operator=(float2 ref a, float2 ref b){ a.x = b.x; a.y = b.y; return a; }\r\n\
float2 a, b, c, d;\r\n\
\r\n\
a = float2(1, 3);\r\n\
b = float2(5, 10);\r\n\
c = a + b;\r\n\
d = c + float2(100, 14);\r\n\
\r\n\
float2 aa;\r\n\
aa += float2(1, 1);\r\n\
float2 bb = float2(3, 4) += float2(1, 4);\r\n\
return 0;";
	printf("\r\nImplicit type to type ref conversions\r\n");
	for(int t = 0; t < 2; t++)
	{
		testCount[t]++;
		if(RunCode(testImplicitToRef, testTarget[t], "0"))
		{
			lastFailed = false;

			CHECK_FLOAT("a", 0, 1);
			CHECK_FLOAT("a", 1, 3);
			CHECK_FLOAT("b", 0, 5);
			CHECK_FLOAT("b", 1, 10);
			CHECK_FLOAT("c", 0, 6);
			CHECK_FLOAT("c", 1, 13);
			CHECK_FLOAT("d", 0, 106);
			CHECK_FLOAT("d", 1, 27);
			CHECK_FLOAT("aa", 0, 1);
			CHECK_FLOAT("aa", 1, 1);
			CHECK_FLOAT("bb", 0, 4);
			CHECK_FLOAT("bb", 1, 8);

			if(!lastFailed)
				passed[t]++;
		}
	}

#ifdef FAILURE_TEST

const char	*testDivZero = 
"// Division by zero handling\r\n\
int a=5, b =0;\r\n\
return a/b;";
	printf("\r\n Division by zero handling\r\n");
	for(int t = 0; t < 2; t++)
	{
		testCount[t]++;
		if(!RunCode(testDivZero, testTarget[t], "ERROR: Integer division by zero"))
			passed[t]++;
		else
			printf("Should have failed");
	}

const char	*testFuncNoReturn = 
"// Function with no return handling\r\n\
int test(){ 1; } // temporary\r\n\
return test();";
	printf("\r\nFunction with no return handling\r\n");
	for(int t = 0; t < 2; t++)
	{
		testCount[t]++;
		if(!RunCode(testFuncNoReturn, testTarget[t], "1"))
			passed[t]++;
		else
			printf("Should have failed");
	}


const char	*testBounds1 = 
"// Array out of bound check \r\n\
int[4] n;\r\n\
int i = 4;\r\n\
n[i] = 3;\r\n\
return 1;";
	printf("\r\nArray out of bounds error check 1\r\n");
	for(int t = 0; t < 2; t++)
	{
		testCount[t]++;
		if(!RunCode(testBounds1, testTarget[t], "1"))
			passed[t]++;
		else
			printf("Should have failed");
	}

const char	*testBounds2 = 
"// Array out of bound check 2\r\n\
int[4] n;\r\n\
int[] nn = n;\r\n\
int i = 4;\r\n\
nn[i] = 3;\r\n\
return 1;";
	printf("\r\nArray out of bounds error check 2\r\n");
	for(int t = 0; t < 2; t++)
	{
		testCount[t]++;
		if(!RunCode(testBounds2, testTarget[t], "1"))
			passed[t]++;
		else
			printf("Should have failed");
	}

const char	*testBounds3 = 
"// Array out of bound check 3\r\n\
auto x0 = { 1, 2 };\r\n\
auto x1 = { 1, 2, 3 };\r\n\
auto x2 = { 1, 2, 3, 4 };\r\n\
int[3][] x;\r\n\
x[0] = x0;\r\n\
x[1] = x1;\r\n\
x[2] = x2;\r\n\
int[][] xr = x;\r\n\
return xr[1][3];";
	printf("\r\nArray out of bounds error check 3\r\n");
	for(int t = 0; t < 2; t++)
	{
		testCount[t]++;
		if(!RunCode(testBounds3, testTarget[t], "1"))
			passed[t]++;
		else
			printf("Should have failed");
	}

const char	*testAutoReferenceMismatch =
"int a = 17;\r\n\
auto ref d = &a;\r\n\
double ref ll = d;\r\n\
return *ll;";
	printf("\r\nAuto reference type\r\n");
	for(int t = 0; t < 2; t++)
	{
		testCount[t]++;
		if(!RunCode(testAutoReferenceMismatch, testTarget[t], "1"))
			passed[t]++;
		else
			printf("Should have failed");
	}

#endif

#define TEST_FOR_FAIL(name, str, error)\
{\
	testCount[2]++;\
	nullres good = nullcCompile(str);\
	if(!good)\
	{\
		char buf[512];\
		strcpy(buf, strstr(nullcGetCompilationError(), "ERROR:"));\
		*strchr(buf, '\r') = 0;\
		if(strcmp(error, buf) != 0)\
		{\
			printf("Failed %s but for wrong reason:\r\n    %s\r\nexpected:\r\n    %s\r\n", name, buf, error);\
		}else{\
			passed[2]++;\
		}\
	}else{\
		printf("Test \"%s\" failed to fail.\r\n", name);\
	}\
}

	TEST_FOR_FAIL("Number not allowed in this base", "return 09;", "ERROR: Digit 9 is not allowed in base 8");
	TEST_FOR_FAIL("Unknown escape sequence", "return '\\p';", "ERROR: unknown escape sequence");
	TEST_FOR_FAIL("Wrong alignment", "align(32) int a; return 0;", "ERROR: alignment must be less than 16 bytes");
	TEST_FOR_FAIL("Change of immutable value", "int i; return *i = 5;", "ERROR: cannot change immutable value of type int");
	TEST_FOR_FAIL("Hex overflow", "return 0xbeefbeefbeefbeefb;", "ERROR: Overflow in hexadecimal constant");
	TEST_FOR_FAIL("Oct overflow", "return 03333333333333333333333;", "ERROR: Overflow in octal constant");
	TEST_FOR_FAIL("Bin overflow", "return 10000000000000000000000000000000000000000000000000000000000000000b;", "ERROR: Overflow in binary constant");
	TEST_FOR_FAIL("Logical not on double", "return !0.5;", "ERROR: logical NOT is not available on floating-point numbers");
	TEST_FOR_FAIL("Binary not on double", "return ~0.4;", "ERROR: binary NOT is not available on floating-point numbers");

	TEST_FOR_FAIL("No << on float", "return 1.0 << 2.0;", "ERROR: << is illegal for floating-point numbers");
	TEST_FOR_FAIL("No >> on float", "return 1.0 >> 2.0;", "ERROR: >> is illegal for floating-point numbers");
	TEST_FOR_FAIL("No | on float", "return 1.0 | 2.0;", "ERROR: | is illegal for floating-point numbers");
	TEST_FOR_FAIL("No & on float", "return 1.0 & 2.0;", "ERROR: & is illegal for floating-point numbers");
	TEST_FOR_FAIL("No ^ on float", "return 1.0 ^ 2.0;", "ERROR: ^ is illegal for floating-point numbers");
	TEST_FOR_FAIL("No && on float", "return 1.0 && 2.0;", "ERROR: && is illegal for floating-point numbers");
	TEST_FOR_FAIL("No || on float", "return 1.0 || 2.0;", "ERROR: || is illegal for floating-point numbers");
	TEST_FOR_FAIL("No ^^ on float", "return 1.0 ^^ 2.0;", "ERROR: ^^ is illegal for floating-point numbers");

	TEST_FOR_FAIL("Wrong return", "int a(){ return {1,2};} return 1;", "ERROR: function returns int[2] but supposed to return int");
	TEST_FOR_FAIL("Shouldn't return anything", "void a(){ return 1; } return 1;", "ERROR: function returning a value");
	TEST_FOR_FAIL("Should return something", "int a(){ return; } return 1;", "ERROR: function should return int");
	TEST_FOR_FAIL("Global return doesn't accept void", "void a(){} return a();", "ERROR: global return cannot accept void");
	TEST_FOR_FAIL("Global return doesn't accept complex types", "void a(){} return a;", "ERROR: global return cannot accept complex types");

	TEST_FOR_FAIL("Break followed by trash", "int a; break a; return 1;", "ERROR: break must be followed by ';' or a constant");
	TEST_FOR_FAIL("Break with depth 0", "break 0; return 1;", "ERROR: break level cannot be 0");
	TEST_FOR_FAIL("Break with depth too big", "while(1){ break 2; } return 1;", "ERROR: break level is greater that loop depth");

	TEST_FOR_FAIL("continue followed by trash", "int a; continue a; return 1;", "ERROR: continue must be followed by ';' or a constant");
	TEST_FOR_FAIL("continue with depth 0", "continue 0; return 1;", "ERROR: continue level cannot be 0");
	TEST_FOR_FAIL("continue with depth too big", "while(1){ continue 2; } return 1;", "ERROR: continue level is greater that loop depth");

	TEST_FOR_FAIL("Variable redefinition", "int a, a; return 1;", "ERROR: Name 'a' is already taken for a variable in current scope");
	TEST_FOR_FAIL("Variable hides function", "void a(){} int a; return 1;", "ERROR: Name 'a' is already taken for a function");

	TEST_FOR_FAIL("Uninit auto", "auto a; return 1;", "ERROR: auto variable must be initialized in place of definition");
	TEST_FOR_FAIL("Array of auto", "auto[4] a; return 1;", "ERROR: cannot specify array size for auto variable");
	TEST_FOR_FAIL("sizeof auto", "return sizeof(auto);", "ERROR: sizeof(auto) is illegal");

	TEST_FOR_FAIL("Unknown function", "return b;", "ERROR: function 'b' is not defined");
	TEST_FOR_FAIL("Unclear decision", "void a(int b){} void a(float b){} return a;", "ERROR: there are more than one 'a' function, and the decision isn't clear");
	TEST_FOR_FAIL("Variable of unknown type used", "auto a = a + 1; return a;", "ERROR: variable 'a' is being used while its type is unknown");

	TEST_FOR_FAIL("Indexing not an array", "int a; return a[5];", "ERROR: indexing variable that is not an array");
	TEST_FOR_FAIL("Array underflow", "int[4] a; a[-1] = 2; return 1;", "ERROR: Array index cannot be negative");
	TEST_FOR_FAIL("Array overflow", "int[4] a; a[5] = 1; return 1;", "ERROR: Array index out of bounds");

	TEST_FOR_FAIL("No matching function", "int f(int a, b){} int f(int a, long b){} return f(1)'", "ERROR: can't find function 'f' with following parameters:");
	TEST_FOR_FAIL("No clear decision", "int f(int a, b){} int f(int a, long b){} int f(){} return f(1, 3.0)'", "ERROR: Ambiguity, there is more than one overloaded function available for the call.");

	TEST_FOR_FAIL("Array without member", "int[4] a; return a.m;", "ERROR: Array doesn't have member with this name");
	TEST_FOR_FAIL("No methods", "int[4] i; return i.ok();", "ERROR: function 'int[]::ok' is undefined");
	TEST_FOR_FAIL("void array", "void f(){} return { f(), f() };", "ERROR: array cannot be constructed from void type elements");
	TEST_FOR_FAIL("Name taken", "int a; void a(){} return 1;", "ERROR: Name 'a' is already taken for a variable in current scope");
	TEST_FOR_FAIL("Auto parameter", "auto(auto a){} return 1;", "ERROR: function parameter cannot be an auto type");
	TEST_FOR_FAIL("Function redefine", "int a(int b){} int a(int c){} return 1;", "ERROR: function 'a' is being defined with the same set of parameters");
	TEST_FOR_FAIL("Wrong overload", "int operator*(int a){} return 1;", "ERROR: binary operator definition or overload must accept exactly two arguments");
	TEST_FOR_FAIL("Overload in the wrong place", "int f(){ int operator+(int a, b){}} return 1;", "ERROR: binary operator definition or overload must be placed in global scope");
	TEST_FOR_FAIL("No member function", "int a; return a.ok();", "ERROR: function 'int::ok' is undefined");
	TEST_FOR_FAIL("Unclear decision - member function", "class test{ void a(int b){} void a(float b){} } test t; return t.a;", "ERROR: there are more than one 'a' function, and the decision isn't clear");
	TEST_FOR_FAIL("No function", "return k();", "ERROR: function 'k' is undefined");

	TEST_FOR_FAIL("void condition", "void f(){} if(f()){} return 1;", "ERROR: condition type cannot be void");
	TEST_FOR_FAIL("void condition", "void f(){} if(f()){}else{} return 1;", "ERROR: condition type cannot be void");
	TEST_FOR_FAIL("void condition", "void f(){} return f() ? 1 : 0;", "ERROR: condition type cannot be void");
	TEST_FOR_FAIL("void condition", "void f(){} for(int i = 0; f(); i++){} return 1;", "ERROR: condition type cannot be void");
	TEST_FOR_FAIL("void condition", "void f(){} while(f()){} return 1;", "ERROR: condition type cannot be void");
	TEST_FOR_FAIL("void condition", "void f(){} do{}while(f()); return 1;", "ERROR: condition type cannot be void");
	TEST_FOR_FAIL("void condition", "void f(){} switch(f()){ case 1: break; } return 1;", "ERROR: cannot switch by void type");
	TEST_FOR_FAIL("void case", "void f(){} switch(1){ case f(): break; } return 1;", "ERROR: case value type cannot be void");

	TEST_FOR_FAIL("class in class", "class test{ void f(){ class heh{ int h; } } } return 1;", "ERROR: Different type is being defined");
	TEST_FOR_FAIL("class wrong alignment", "align(32) class test{int a;} return 1;", "ERROR: alignment must be less than 16 bytes");
	TEST_FOR_FAIL("class member auto", "class test{ auto i; } return 1;", "ERROR: auto cannot be used for class members");
	TEST_FOR_FAIL("class is too big", "class nobiggy{ int[128][128][4] a; } return 1;", "ERROR: class size cannot exceed 65535 bytes");

	TEST_FOR_FAIL("array size not const", "import std.math; int[cos(12) * 16] a; return a[0];", "ERROR: Array size must be a constant expression");
	TEST_FOR_FAIL("array size not positive", "int[-16] a; return a[0];", "ERROR: Array size can't be negative or zero");
	TEST_FOR_FAIL("cannot dereference if not a reference", "int a; return *a;", "ERROR: cannot change immutable value of type int");

	TEST_FOR_FAIL("function parameter cannot be a void type", "int f(void a){ return 0; } return 1;", "ERROR: function parameter cannot be a void type");
	TEST_FOR_FAIL("function prototype with unresolved return type", "auto f(); return 1;", "ERROR: function prototype with unresolved return type");
	TEST_FOR_FAIL("Division by zero during constant folding", "return 5 / 0;", "ERROR: Division by zero during constant folding");
	TEST_FOR_FAIL("Modulus division by zero during constant folding 1", "return 5 % 0;", "ERROR: Modulus division by zero during constant folding");
	TEST_FOR_FAIL("Modulus division by zero during constant folding 2", "return 5l % 0l;", "ERROR: Modulus division by zero during constant folding");

	TEST_FOR_FAIL("Variable as a function", "int a = 5; return a(4);", "ERROR: variable is not a pointer to function");

	TEST_FOR_FAIL("Function pointer call with wrong argument count", "int f(int a){ return -a; } auto foo = f; auto b = foo(); return foo(1, 2);", "ERROR: function expects 1 argument(s), while 0 are supplied");
	TEST_FOR_FAIL("Function pointer call with wrong argument types", "import std.math; int f(int a){ return -a; } auto foo = f; float4 v; return foo(v);", "ERROR: there is no conversion from specified arguments and the ones that function accepts");

	TEST_FOR_FAIL("Indirect function pointer call with wrong argument count", "int f(int a){ return -a; } typeof(f)[2] foo = { f, f }; auto b = foo[0](); return foo(1, 2);", "ERROR: function expects 1 argument(s), while 0 are supplied");
	TEST_FOR_FAIL("Indirect function pointer call with wrong argument types", "import std.math; int f(int a){ return -a; } typeof(f)[2] foo = { f, f }; float4 v; return foo[0](v);", "ERROR: there is no conversion from specified arguments and the ones that function accepts");

	TEST_FOR_FAIL("Array element type mistmatch", "import std.math;\r\nauto err = { 1, float2(2, 3), 4 };\r\nreturn 1;", "ERROR: element 1 doesn't match the type of element 0 (int)");
	TEST_FOR_FAIL("Ternary operator complex type mistmatch", "import std.math;\r\nauto err = 1 ? 1 : float2(2, 3);\r\nreturn 1;", "ERROR: ternary operator ?: result types are not equal (int : float2)");

	TEST_FOR_FAIL("Indexing value that is not an array 2", "return (1)[1];", "ERROR: indexing variable that is not an array");
	TEST_FOR_FAIL("Illegal conversion from type[] ref to type[]", "int[] b = { 1, 2, 3 };int[] ref c = &b;int[] d = c;return 1;", "ERROR: Cannot convert 'int[] ref' to 'int[]'");
	TEST_FOR_FAIL("Type redefinition", "class int{ int a, b; } return 1;", "ERROR: 'int' is being redefined");

	TEST_FOR_FAIL("Illegal conversion 1", "import std.math; float3 a; a = 12.0; return 1;", "ERROR: Cannot convert 'double' to 'float3'");
	TEST_FOR_FAIL("Illegal conversion 2", "import std.math; float3 a; float4 b; b = a; return 1;", "ERROR: Cannot convert 'float3' to 'float4'");

	TEST_FOR_FAIL("For scope", "for(int i = 0; i < 1000; i++) i += 5; return i;", "ERROR: function 'i' is not defined");

	TEST_FOR_FAIL("Class function return unclear 1", "class Test{int i;int foo(){ return i; }int foo(int k){ return i; }auto bar(){ return foo; }}return 1;", "ERROR: there are more than one 'foo' function, and the decision isn't clear");
	TEST_FOR_FAIL("Class function return unclear 2", "int foo(){ return 2; }class Test{int i;int foo(){ return i; }auto bar(){ return foo; }}return 1;", "ERROR: there are more than one 'foo' function, and the decision isn't clear");

	TEST_FOR_FAIL("Class externally defined method 1", "int dontexist:do(){ return 0; } return 1;", "ERROR: class name expected before ':'");
	TEST_FOR_FAIL("Class externally defined method 2", "int int:(){ return *this; } return 1;", "ERROR: function name expected after ':'");

	TEST_FOR_FAIL("Member variable or function is not found", "int a; a.b; return 1;", "ERROR: member variable or function 'b' is not defined in class 'int'");

	TEST_FOR_FAIL("Inplace array element type mismatch", "auto a = { 12, 15.0 };", "ERROR: element 1 doesn't match the type of element 0 (int)");
	TEST_FOR_FAIL("Ternary operator void return type", "void f(){} return 1 ? f() : 0.0;", "ERROR: one of ternary operator ?: result type is void (void : double)");
	TEST_FOR_FAIL("Ternary operator return type difference", "import std.math; return 1 ? 12 : float2(3, 4);", "ERROR: ternary operator ?: result types are not equal (int : float2)");

	TEST_FOR_FAIL("Variable type is unknow", "int test(int a, typeof(test) ptr){ return ptr(a, ptr); }", "ERROR: variable type is unknown");

	TEST_FOR_FAIL("Illegal pointer operation 1", "int ref a; a += a;", "ERROR: There is no build-in operator for types 'int ref' and 'int ref'");
	TEST_FOR_FAIL("Illegal pointer operation 2", "int ref a; a++;", "ERROR: Increment is not supported on 'int ref'");
	TEST_FOR_FAIL("Illegal pointer operation 3", "int ref a; a = a * 5;", "ERROR: Operation * is not supported on 'int ref' and 'int'");
	TEST_FOR_FAIL("Illegal class operation", "import std.math; float2 v; v = ~v;", "ERROR: Unary operation '~' is not supported on 'float2'");

	TEST_FOR_FAIL("Default function parameter type mismatch", "import std.math;int f(int v = float3(3, 4, 5)){ return v; }return f();", "ERROR: Cannot convert from 'float3' to 'int'");
	
	TEST_FOR_FAIL("Undefined function call in function parameters", "int func(int a = func()){ return 0; } return 0;", "ERROR: function 'func' is undefined");
	
	//TEST_FOR_FAIL("parsing", "");

	TEST_FOR_FAIL("parsing", "return 0x;", "ERROR: '0x' must be followed by number");
	TEST_FOR_FAIL("parsing", "int[12 a;", "ERROR: Matching ']' not found");
	TEST_FOR_FAIL("parsing", "typeof 12 a;", "ERROR: typeof must be followed by '('");
	TEST_FOR_FAIL("parsing", "typeof(12 a;", "ERROR: ')' not found after expression in typeof");
	TEST_FOR_FAIL("parsing", "typeof() a;", "ERROR: expression not found after typeof(");
	TEST_FOR_FAIL("parsing", "class{}", "ERROR: class name expected");
	TEST_FOR_FAIL("parsing", "class Test int a;", "ERROR: '{' not found after class name");
	TEST_FOR_FAIL("parsing", "class Test{ int; }", "ERROR: class member name expected after type");
	TEST_FOR_FAIL("parsing", "class Test{ int a, ; }", "ERROR: member name expected after ','");
	TEST_FOR_FAIL("parsing", "class Test{ int a, b }", "ERROR: ';' not found after class member list");
	TEST_FOR_FAIL("parsing", "class Test{ int a; return 5;", "ERROR: '}' not found after class definition");
	TEST_FOR_FAIL("parsing", "auto(int a, ){}", "ERROR: variable name not found after type in function variable list");
	TEST_FOR_FAIL("parsing", "void f(int a, b){} return a(1, );", "ERROR: expression not found after ',' in function parameter list");
	TEST_FOR_FAIL("parsing", "void f(int a, b){} return a(1, 2;", "ERROR: ')' not found after function parameter list");
	TEST_FOR_FAIL("parsing", "int operator[(int a, b){ return a+b; }", "ERROR: ']' not found after '[' in operator definition");
	TEST_FOR_FAIL("parsing", "auto(int a, b{}", "ERROR: ')' not found after function variable list");
	TEST_FOR_FAIL("parsing", "auto(int a, b) return 0;", "ERROR: '{' not found after function header");
	TEST_FOR_FAIL("parsing", "auto(int a, b){ return a+b; return 1;", "ERROR: '}' not found after function body");
	TEST_FOR_FAIL("parsing", "int a[4];", "ERROR: array size must be specified after typename");
	TEST_FOR_FAIL("parsing", "int a=;", "ERROR: expression not found after '='");
	TEST_FOR_FAIL("parsing", "int = 3;", "ERROR: variable name not found after type name");
	TEST_FOR_FAIL("parsing", "int a, ;", "ERROR: next variable definition excepted after ','");
	TEST_FOR_FAIL("parsing", "align int a=2;", "ERROR: '(' expected after align");
	TEST_FOR_FAIL("parsing", "align() int a = 2;", "ERROR: alignment value not found after align(");
	TEST_FOR_FAIL("parsing", "align(2 int a;", "ERROR: ')' expected after alignment value");
	TEST_FOR_FAIL("parsing", "if", "ERROR: '(' not found after 'if'");
	TEST_FOR_FAIL("parsing", "if(", "ERROR: condition not found in 'if' statement");
	TEST_FOR_FAIL("parsing", "if(1", "ERROR: closing ')' not found after 'if' condition");
	TEST_FOR_FAIL("parsing", "if(1)", "ERROR: expression not found after 'if'");
	TEST_FOR_FAIL("parsing", "if(1) 1; else ", "ERROR: expression not found after 'else'");
	TEST_FOR_FAIL("parsing", "for", "ERROR: '(' not found after 'for'");
	TEST_FOR_FAIL("parsing", "for({})", "ERROR: ';' not found after initializer in 'for'");
	TEST_FOR_FAIL("parsing", "for(;1<2)", "ERROR: ';' not found after condition in 'for'");
	TEST_FOR_FAIL("parsing", "for({)", "ERROR: '}' not found after '{'");
	TEST_FOR_FAIL("parsing", "for(;;)", "ERROR: condition not found in 'for' statement");
	TEST_FOR_FAIL("parsing", "for(;1<2;{)", "ERROR: '}' not found after '{'");
	TEST_FOR_FAIL("parsing", "for(;1<2;{})", "ERROR: body not found after 'for' header");
	TEST_FOR_FAIL("parsing", "for(;1<2;)", "ERROR: body not found after 'for' header");
	TEST_FOR_FAIL("parsing", "for(;1<2", "ERROR: ';' not found after condition in 'for'");
	TEST_FOR_FAIL("parsing", "for(;1<2;{}", "ERROR: ')' not found after 'for' statement");
	TEST_FOR_FAIL("parsing", "while", "ERROR: '(' not found after 'while'");
	TEST_FOR_FAIL("parsing", "while(", "ERROR: expression expected after 'while('");
	TEST_FOR_FAIL("parsing", "while(1", "ERROR: closing ')' not found after expression in 'while' statement");
	TEST_FOR_FAIL("parsing", "while(1)", "ERROR: expression expected after 'while(...)'");
	TEST_FOR_FAIL("parsing", "do", "ERROR: expression expected after 'do'");
	TEST_FOR_FAIL("parsing", "do 1;", "ERROR: 'while' expected after 'do' statement");
	TEST_FOR_FAIL("parsing", "do 1; while", "ERROR: '(' not found after 'while'");
	TEST_FOR_FAIL("parsing", "do 1; while(", "ERROR: expression expected after 'while('");
	TEST_FOR_FAIL("parsing", "do 1; while(1", "ERROR: closing ')' not found after expression in 'while' statement");
	TEST_FOR_FAIL("parsing", "do 1; while(0)", "ERROR: while(...) should be followed by ';'");
	TEST_FOR_FAIL("parsing", "switch", "ERROR: '(' not found after 'switch'");
	TEST_FOR_FAIL("parsing", "switch(", "ERROR: expression not found after 'switch('");
	TEST_FOR_FAIL("parsing", "switch(2", "ERROR: closing ')' not found after expression in 'switch' statement");
	TEST_FOR_FAIL("parsing", "switch(2)", "ERROR: '{' not found after 'switch(...)'");
	TEST_FOR_FAIL("parsing", "switch(2){ case", "ERROR: expression expected after 'case' of 'default'");
	TEST_FOR_FAIL("parsing", "switch(2){ case 2", "ERROR: ':' expected");
	TEST_FOR_FAIL("parsing", "switch(2){ case 2:", "ERROR: '}' not found after 'switch' statement");
	TEST_FOR_FAIL("parsing", "switch(2){ default:", "ERROR: '}' not found after 'switch' statement");
	TEST_FOR_FAIL("parsing", "return", "ERROR: return must be followed by ';'");
	TEST_FOR_FAIL("parsing", "break", "ERROR: break must be followed by ';'");
	TEST_FOR_FAIL("parsing", "for(;1;) continue; continue", "ERROR: continue must be followed by ';'");
	TEST_FOR_FAIL("parsing", "(", "ERROR: expression not found after '('");
	TEST_FOR_FAIL("parsing", "(1", "ERROR: closing ')' not found after '('");
	TEST_FOR_FAIL("parsing", "*", "ERROR: variable name not found after '*'");
	TEST_FOR_FAIL("parsing", "int i; i.", "ERROR: member variable expected after '.'");
	TEST_FOR_FAIL("parsing", "int[4] i; i[", "ERROR: expression not found after '['");
	TEST_FOR_FAIL("parsing", "int[4] i; i[2", "ERROR: ']' not found after expression");
	TEST_FOR_FAIL("parsing", "&", "ERROR: variable not found after '&'");
	TEST_FOR_FAIL("parsing", "!", "ERROR: expression not found after '!'");
	TEST_FOR_FAIL("parsing", "~", "ERROR: expression not found after '~'");
	TEST_FOR_FAIL("parsing", "--", "ERROR: variable not found after '--'");
	TEST_FOR_FAIL("parsing", "++", "ERROR: variable not found after '++'");
	TEST_FOR_FAIL("parsing", "+", "ERROR: expression not found after '+'");
	TEST_FOR_FAIL("parsing", "-", "ERROR: expression not found after '-'");
	TEST_FOR_FAIL("parsing", "'aa'", "ERROR: only one character can be inside single quotes");
	TEST_FOR_FAIL("parsing", "sizeof", "ERROR: sizeof must be followed by '('");
	TEST_FOR_FAIL("parsing", "sizeof(", "ERROR: expression or type not found after sizeof(");
	TEST_FOR_FAIL("parsing", "sizeof(int", "ERROR: ')' not found after expression in sizeof");
	TEST_FOR_FAIL("parsing", "new", "ERROR: Type name expected after 'new'");
	TEST_FOR_FAIL("parsing", "new int[", "ERROR: expression not found after '['");
	TEST_FOR_FAIL("parsing", "new int[12", "ERROR: ']' not found after expression");
	TEST_FOR_FAIL("parsing", "auto a = {", "ERROR: value not found after '{'");
	TEST_FOR_FAIL("parsing", "auto a = {1,", "ERROR: value not found after ','");
	TEST_FOR_FAIL("parsing", "auto a = {1,2", "ERROR: '}' not found after inline array");
	TEST_FOR_FAIL("parsing", "return 1+;", "ERROR: terminal expression not found after binary operation");
	TEST_FOR_FAIL("parsing", "1?", "ERROR: expression not found after '?'");
	TEST_FOR_FAIL("parsing", "1?2", "ERROR: ':' not found after expression in ternary operator");
	TEST_FOR_FAIL("parsing", "1?2:", "ERROR: expression not found after ':'");
	TEST_FOR_FAIL("parsing", "int i; i+=;", "ERROR: expression not found after assignment operator");
	TEST_FOR_FAIL("parsing", "int i; i**=;", "ERROR: expression not found after '**='");
	TEST_FOR_FAIL("parsing", "int i", "ERROR: ';' not found after variable definition");
	TEST_FOR_FAIL("parsing", "int i; i = 5", "ERROR: ';' not found after expression");
	TEST_FOR_FAIL("parsing", "{", "ERROR: closing '}' not found");
	TEST_FOR_FAIL("parsing", "auto ref() a;", "ERROR: return type of a function type cannot be auto");
	TEST_FOR_FAIL("parsing", "int ref(int ref(int, auto), double) b;", "ERROR: parameter type of a function type cannot be auto");
	TEST_FOR_FAIL("parsing", "typedef", "ERROR: typename expected after typedef");
	TEST_FOR_FAIL("parsing", "typedef double", "ERROR: alias name expected after typename in typedef expression");
	TEST_FOR_FAIL("parsing", "typedef double somename", "ERROR: ';' not found after typedef");
	TEST_FOR_FAIL("parsing", "typedef double int;", "ERROR: there is already a type or an alias with the same name");
	TEST_FOR_FAIL("parsing", "typedef double somename; typedef int somename;", "ERROR: there is already a type or an alias with the same name");

	// Conclusion
	printf("VM passed %d of %d tests\r\n", passed[0], testCount[0]);
	printf("X86 passed %d of %d tests\r\n", passed[1], testCount[1]);
	printf("Failure passed %d of %d tests\r\n", passed[2], testCount[2]);
	printf("Passed %d of %d tests\r\n", passed[0]+passed[1]+passed[2], testCount[0]+testCount[1]+testCount[2]);

	printf("Compilation time: %f\r\n", timeCompile);
	printf("Get listing time: %f\r\n", timeGetListing);
	printf("Get bytecode time: %f\r\n", timeGetBytecode);
	printf("Clean time: %f\r\n", timeClean);
	printf("Link time: %f\r\n", timeLinkCode);
	printf("Run time: %f\r\n", timeRun);

#ifdef SPEED_TEST
/*const char	*testCompileSpeed =
"//import nullclib;\r\n\
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
return 0;";*/

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
int[] data = img.GetData();\r\n\
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
void render(float[] from, int[] to)\r\n\
{\r\n\
	for(auto x = 2; x < width - 2; x++)\r\n\
	{\r\n\
		for(auto y = 2; y < height - 2; y++)\r\n\
		{\r\n\
			float color = get(from, x, y);\r\n\
			\r\n\
			float progress = color / 256;\r\n\
			float rMin = 31, rMax = 168, \r\n\
			gMin = 57, gMax = 224, \r\n\
			bMin = 116, bMax = 237;\r\n\
			\r\n\
			auto rDelta = (rMax - rMin) / 2.0;\r\n\
			auto rValue = int(rMin + rDelta + rDelta * progress);\r\n\
			auto gDelta = (gMax - gMin) / 2.0;\r\n\
			auto gValue = int(gMin + gDelta + gDelta * progress);\r\n\
			auto bDelta = (bMax - bMin) / 2.0;\r\n\
			auto bValue = int(bMin + bDelta + bDelta * progress);\r\n\
			\r\n\
			to[x + y*width] = (rValue << 16) + (gValue << 8) + bValue;\r\n\
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

	nullcSetExecutor(NULLC_VM);

	double time = myGetPreciseTime();
	double compileTime = 0.0;
	double linkTime = 0.0;
	for(int i = 0; i < 30000; i++)
	{
		nullres good = nullcCompile(testCompileSpeed);
		compileTime += myGetPreciseTime() - time;

		if(good)
		{
			char *bytecode = NULL;
			nullcGetBytecode(&bytecode);
			nullcClean();
			nullcLinkCode(bytecode, 0);
			delete[] bytecode;
		}else{
			printf("Compilation failed: %s", nullcGetCompilationError());
			break;
		}
		linkTime += myGetPreciseTime() - time;
		time = myGetPreciseTime();
	}

	printf("Speed test compile time: %f Link time: %f\r\n", compileTime, linkTime - compileTime, compileTime / 30000.0);
	printf("Average compile time: %f Average link time: %f\r\n", compileTime / 30000.0, (linkTime - compileTime) / 30000.0);
	printf("Time: %f Average time: %f\r\n", linkTime, linkTime / 30000.0);
#endif

	nullcDeinitFileModule();
	nullcDeinitMathModule();

	// Deinit NULLC
	nullcDeinit();
}
