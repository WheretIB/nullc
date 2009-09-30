#include "stdafx.h"
#include "UnitTests.h"
#include "NULLC/nullc.h"
#include "NULLC/ParseClass.h"

#include <stdio.h>
#include <vadefs.h>

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

struct ArrayPtr{ char* ptr; int len; };
FILE* mFileOpen(ArrayPtr name, ArrayPtr access)
{
	return fopen(name.ptr, access.ptr);
}

void mFileWrite(FILE* file, ArrayPtr arr)
{
	fwrite(arr.ptr, 1, arr.len, file);
}

template<typename T>
void mFileWriteType(FILE* file, T val)
{
	fwrite(&val, sizeof(T), 1, file);
}

template<typename T>
void mFileWriteTypePtr(FILE* file, T* val)
{
	fwrite(val, sizeof(T), 1, file);
}

void mFileRead(FILE* file, ArrayPtr arr)
{
	fread(arr.ptr, 1, arr.len, file);
}

template<typename T>
void mFileReadTypePtr(FILE* file, T* val)
{
	fread(val, sizeof(T), 1, file);
}

void mFileClose(FILE* file)
{
	fclose(file);
}

void*	FindVar(const char* name)
{
	for(unsigned int i = 0; i < varCount; i++)
	{
		VariableInfo &currVar = *(*(varInfo+i));
		if(strlen(name) == (unsigned int)(currVar.name.end - currVar.name.begin) && memcmp(currVar.name.begin, name, currVar.name.end - currVar.name.begin) == 0)
			return (void*)(varData+currVar.pos);
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
		printf("%s Compilation failed: %s\r\n", buf, nullcGetCompilationError());
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
		nullcLinkCode(bytecode, 1);
		timeLinkCode += myGetPreciseTime() - time;
		delete[] bytecode;

		varData = (char*)nullcGetVariableData();

		time = myGetPreciseTime();
		nullres goodRun = nullcRun();
		timeRun += myGetPreciseTime() - time;
		if(goodRun)
		{
			const char* val = nullcGetResult();
			varData = (char*)nullcGetVariableData();

			if(strcmp(val, expected) != 0)
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

	va_list argptr;
	_crt_va_start(argptr, str);
	_vsnprintf(ptr, 1023, str, argptr);
	_crt_va_end(argptr);
	return ptr;
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

	nullcAddExternalFunction((void (*)())(mFileOpen), "file FileOpen(char[] name, char[] access);");
	nullcAddExternalFunction((void (*)())(mFileClose), "void FileClose(file fID);");
	nullcAddExternalFunction((void (*)())(mFileWrite), "void FileWrite(file fID, char[] arr);");
	nullcAddExternalFunction((void (*)())(mFileWriteTypePtr<char>), "void FileWrite(file fID, char ref data);");
	nullcAddExternalFunction((void (*)())(mFileWriteTypePtr<short>), "void FileWrite(file fID, short ref data);");
	nullcAddExternalFunction((void (*)())(mFileWriteTypePtr<int>), "void FileWrite(file fID, int ref data);");
	nullcAddExternalFunction((void (*)())(mFileWriteTypePtr<long long>), "void FileWrite(file fID, long ref data);");
	nullcAddExternalFunction((void (*)())(mFileWriteType<char>), "void FileWrite(file fID, char data);");
	nullcAddExternalFunction((void (*)())(mFileWriteType<short>), "void FileWrite(file fID, short data);");
	nullcAddExternalFunction((void (*)())(mFileWriteType<int>), "void FileWrite(file fID, int data);");
	nullcAddExternalFunction((void (*)())(mFileWriteType<long long>), "void FileWrite(file fID, long data);");

	nullcAddExternalFunction((void (*)())(mFileRead), "void FileRead(file fID, char[] arr);");
	nullcAddExternalFunction((void (*)())(mFileReadTypePtr<char>), "void FileRead(file fID, char ref data);");
	nullcAddExternalFunction((void (*)())(mFileReadTypePtr<short>), "void FileRead(file fID, short ref data);");
	nullcAddExternalFunction((void (*)())(mFileReadTypePtr<int>), "void FileRead(file fID, int ref data);");
	nullcAddExternalFunction((void (*)())(mFileReadTypePtr<long long>), "void FileRead(file fID, long ref data);");


	int passed[] = { 0, 0 };
	int testCount = 0;

	unsigned int	testTarget[] = { NULLC_VM, NULLC_X86, };

//////////////////////////////////////////////////////////////////////////
	printf("\r\nTwo bytecode merge test 1\r\n");
	testCount++;

	const char *partA1 = "int a = 5; int test(int b){ return a += b; } test(4);";
	const char *partB1 = "int aa = 15; int testA(int b){ return aa += b; } testA(5); return aa;";

	char *bytecodeA, *bytecodeB;
	bytecodeA = NULL;
	bytecodeB = NULL;

	for(int t = 0; t < 2; t++)
	{
		nullcSetExecutor(testTarget[t]);

		nullres good = nullcCompile(partA1);
		nullcSaveListing("asm.txt");
		if(!good)
			printf("Compilation failed: %s\r\n", nullcGetCompilationError());
		else
			nullcGetBytecode(&bytecodeA);

		good = nullcCompile(partB1);
		nullcSaveListing("asm.txt");
		if(!good)
			printf("Compilation failed: %s\r\n", nullcGetCompilationError());
		else
			nullcGetBytecode(&bytecodeB);

		nullcClean();
		nullcLinkCode(bytecodeA, 0);
		nullcLinkCode(bytecodeB, 0);

		nullres goodRun = nullcRun();
		if(goodRun)
		{
			const char* val = nullcGetResult();
			varData = (char*)nullcGetVariableData();

			if(strcmp(val, "20") != 0)
				printf("Failed (%s != %s)\r\n", val, "20");
			varCount = 0;
			varInfo = (VariableInfo**)nullcGetVariableInfo(&varCount);
			if(varInfo)
			{
				bool lastFailed = false;
				CHECK_INT("ERROR", 0, 9);
				CHECK_INT("ERROR", 1, 20);
				if(!lastFailed)
					passed[t]++;
			}
		}else{
			printf("Execution failed: %s\r\n", nullcGetRuntimeError());
		}
		delete[] bytecodeA;
		delete[] bytecodeB;
	}
//////////////////////////////////////////////////////////////////////////
	// Number operation test
	printf("\r\nInteger operation test\r\n");
	testCount++;
	for(int t = 0; t < 2; t++)
	{
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
	testCount++;
	for(int t = 0; t < 2; t++)
	{
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
	testCount++;
	for(int t = 0; t < 2; t++)
	{
		if(RunCode(testLongOp, testTarget[t], "1"))
		{
			lastFailed = false;
			CHECK_LONG("a", 0, 4494967296ll);
			CHECK_LONG("b", 0, 594967296ll);
			CHECK_LONG("c", 0, 3ll);
			long long resExp[] = { 5089934592ll, 3900000000ll, -4494967296ll, -4494967297ll, 2674358537709551616ll, 7, 330196224, 210609828468829063ll, 1, 0, 1, 0, 0, 1,
				35959738368, 105553116266496ll, 561870912, 56771072, 5033163520, 4976392448, 0, 0, 0, 1, 0, 1, 1, 1, 1, 0, 0, 1, 0, 1 };
			for(int i = 0; i < 24; i++)
				CHECK_LONG("res", i, resExp[i]);
			if(!lastFailed)
				passed[t]++;
		}
	}
	printf("\r\nDecrement and increment tests for all types\r\n");
	testCount++;
	for(int t = 0; t < 2; t++)
	{
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
	testCount++;
	for(int t = 0; t < 2; t++)
	{
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
	testCount++;
	for(int t = 0; t < 2; t++)
	{
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
float2 a;\r\n\
a/*[gg]*/.x = 2;\r\n\
a.y = a/*[gg]*/.x + 3;\r\n\
// Result:\r\n\
// a.x = 2; a.y = 5\r\n\
return a.x;";
	printf("\r\nCompiler mislead test\r\n");
	testCount++;
	for(int t = 0; t < 2; t++)
	{
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
float4x4 mat;\r\n\
mat.row1.y = 5;\r\n\
return 1;";
	printf("\r\nComplex type test\r\n");
	testCount++;
	for(int t = 0; t < 2; t++)
	{
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
	testCount++;
	for(int t = 0; t < 2; t++)
	{
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
	testCount++;
	for(int t = 0; t < 2; t++)
	{
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
	testCount++;
	for(int t = 0; t < 2; t++)
	{
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
	testCount++;
	for(int t = 0; t < 2; t++)
	{
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
	testCount++;
	for(int t = 0; t < 2; t++)
	{
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
	testCount++;
	for(int t = 0; t < 2; t++)
	{
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
	testCount++;
	for(int t = 0; t < 2; t++)
	{
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
return c;";
	printf("\r\nlongPow speed test\r\n");
	testCount++;
	for(int t = 0; t < 2; t++)
	{
		if(RunCode(testLongSpeed, testTarget[t], "21611482313284249L"))
		{
			lastFailed = false;

			CHECK_LONG("a", 0, 43);
			CHECK_LONG("b", 0, 10);
			CHECK_LONG("c", 0, 21611482313284249ll);

			if(!lastFailed)
				passed[t]++;
		}
	}


const char	*testDivZero = 
"// Division by zero handling\r\n\
int a=5, b =0;\r\n\
return a/b;";
	printf("\r\n Division by zero handling\r\n");
	testCount++;
	for(int t = 0; t < 2; t++)
	{
		if(!RunCode(testDivZero, testTarget[t], "ERROR: Integer division by zero"))
			passed[t]++;
		else
			printf("Should have failed");
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
	testCount++;
	for(int t = 0; t < 2; t++)
	{
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
	testCount++;
	for(int t = 0; t < 2; t++)
	{
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
double Pi = 3.1415926583;\r\n\
double[20] res;\r\n\
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
return 1;";
	printf("\r\nBuild-In function checks\r\n");
	testCount++;
	for(int t = 0; t < 2; t++)
	{
		if(RunCode(testBuildinFunc, testTarget[t], "1"))
		{
			lastFailed = false;

			double resExp[] = { 1.0, 0.5, -1.0, 0.0, 0.5, 0.0, 2.0, 1.0, -1.0, -2.0,
								0.0, 1.0, 0,0, 1.0, 0.0, 1.0, 0.0, 3.0 };
			for(int i = 0; i < 19; i++)
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
	testCount++;
	for(int t = 0; t < 2; t++)
	{
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
	testCount++;
	for(int t = 0; t < 2; t++)
	{
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
	testCount++;
	for(int t = 0; t < 2; t++)
	{
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
	testCount++;
	for(int t = 0; t < 2; t++)
	{
		if(RunCode(testFuncCall6, testTarget[t], "0.500000"))
		{
			lastFailed = false;

			if(!lastFailed)
				passed[t]++;
		}
	}


const char	*testCmplxFail = 
"//Some complexness %)\r\n\
float3 a;\r\n\
float4 b;\r\n\
a = 12.0; // should fail\r\n\
b = a; // should fail\r\n\
return 1;";
	printf("\r\nComplex fail test\r\n");
	testCount++;
	for(int t = 0; t < 2; t++)
	{
		if(!RunCode(testCmplxFail, testTarget[t], "1"))
			passed[t]++;
		else
			printf("Should have failed");
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
	testCount++;
	for(int t = 0; t < 2; t++)
	{
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
	testCount++;
	for(int t = 0; t < 2; t++)
	{
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
	testCount++;
	for(int t = 0; t < 2; t++)
	{
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
"float4 a;\r\n\
float4 ref b = &a;\r\n\
b.x = 5.0f;\r\n\
return b.x;";
	printf("\r\nPointers on complex 2\r\n");
	testCount++;
	for(int t = 0; t < 2; t++)
	{
		if(RunCode(testPointersCmplx2, testTarget[t], "5.000000"))
		{
			lastFailed = false;
			CHECK_FLOAT("a", 0, 5.0);
			if(!lastFailed)
				passed[t]++;
		}
	}


const char	*testPointers2 = 
"double testA(float4 ref v){ return v.x; }\r\n\
float4 a;\r\n\
float4 ref b = &a;\r\n\
a.x = 5.0f;\r\n\
return testA(b);";
	printf("\r\nPointers 2\r\n");
	testCount++;
	for(int t = 0; t < 2; t++)
	{
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
	testCount++;
	for(int t = 0; t < 2; t++)
	{
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
"int[5] arr;\r\n\
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
	testCount++;
	for(int t = 0; t < 2; t++)
	{
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
	testCount++;
	for(int t = 0; t < 2; t++)
	{
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
	testCount++;
	for(int t = 0; t < 2; t++)
	{
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
	testCount++;
	for(int t = 0; t < 2; t++)
	{
		if(RunCode(testFuncOverload, testTarget[t], "90"))
			passed[t]++;
	}


const char	*testFuncNoReturn = 
"// Function with no return handling\r\n\
int test(){ 1; } // temporary\r\n\
return test();";
	printf("\r\nFunction with no return handling\r\n");
	testCount++;
	for(int t = 0; t < 2; t++)
	{
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
	testCount++;
	for(int t = 0; t < 2; t++)
	{
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
	testCount++;
	for(int t = 0; t < 2; t++)
	{
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
	testCount++;
	for(int t = 0; t < 2; t++)
	{
		if(!RunCode(testBounds3, testTarget[t], "1"))
			passed[t]++;
		else
			printf("Should have failed");
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
	testCount++;
	for(int t = 0; t < 2; t++)
	{
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
	testCount++;
	for(int t = 0; t < 2; t++)
	{
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
	testCount++;
	for(int t = 0; t < 2; t++)
	{
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
	testCount++;
	for(int t = 0; t < 2; t++)
	{
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
float test(float4 a, float4 b){ return a.x*b.x+a.y*b.y+a.z*b.z+a.w*b.w; }\r\n\
float4 test2(float4 u){ u.x += 5.0; return u; }\r\n\
float4 float4(float x, float y, float z, float w){ float4 ret; ret.x = x; ret.y = y; ret.z = z; ret.w = w; return ret; }\r\n\
float4 float4(float all){ float4 ret; ret.x = ret.y = ret.z = ret.w = all; return ret; }\r\n\
float4 float4(float3 xyz, float w){ float4 ret; ret.x = xyz.x; ret.y = xyz.y; ret.z = xyz.z; ret.w = w; return ret; }\r\n\
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
	testCount++;
	for(int t = 0; t < 2; t++)
	{
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
for(int i = 0; i < /*10000000*/1000; i++)\r\n\
{\r\n\
  mat = test(mat);\r\n\
  f = test(f);\r\n\
  f2 = test(f2);\r\n\
  f3 = test(f3);\r\n\
  f4 = test(f4);\r\n\
}\r\n\
return mat.row1.y;";
	printf("\r\nSpeed tests\r\n");
	testCount++;
	for(int t = 0; t < 2; t++)
	{
		if(RunCode(testSpeed, testTarget[t], "1.#INF00"))
		{
			lastFailed = false;

			if(!lastFailed)
				passed[t]++;
		}
	}

const char	*testAuto = 
"//Auto type tests\r\n\
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
	testCount++;
	for(int t = 0; t < 2; t++)
	{
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
	testCount++;
	for(int t = 0; t < 2; t++)
	{
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
	testCount++;
	for(int t = 0; t < 2; t++)
	{
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
	testCount++;
	for(int t = 0; t < 2; t++)
	{
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
			CHECK_STR("$carr1", 0, "does work");

			if(!lastFailed)
				passed[t]++;
		}
	}


const char	*testFile = 
"// File and something else test\r\n\
int test(char[] eh, int u){ int b = 0; for(int i=0;i<u;i++)b+=eh[i]; return b; }\r\n\
\r\n\
auto uh = \"ehhhe\";\r\n\
int k = 5464321;\r\n\
file n = FileOpen(\"haha.txt\", \"wb\");\r\n\
auto text = \"Hello file!!!\";\r\n\
FileWrite(n, text);\r\n\
\r\n\
FileWrite(n, &k);\r\n\
FileClose(n);\r\n\
return test(uh, 3);";
	printf("\r\nFile and something else test\r\n");
	testCount++;
	for(int t = 0; t < 2; t++)
	{
		if(RunCode(testFile, testTarget[t], "309"))
		{
			lastFailed = false;
			CHECK_STR("uh", 0, "ehhhe");
			CHECK_STR("$carr1", 0, "haha.txt");
			CHECK_STR("$carr2", 0, "wb");
			CHECK_STR("text", 0, "Hello file!!!");

			CHECK_INT("k", 0, 5464321);
			if(!lastFailed)
				passed[t]++;
		}
	}


const char	*testFile2 = 
"//File test\r\n\
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
file test = FileOpen(name, acc);\r\n\
FileWrite(test, text);\r\n\
FileWrite(test, &ch);\r\n\
FileWrite(test, &sh);\r\n\
FileWrite(test, &num);\r\n\
FileWrite(test, &lnum);\r\n\
FileWrite(test, ch);\r\n\
FileWrite(test, sh);\r\n\
FileWrite(test, num);\r\n\
FileWrite(test, lnum);\r\n\
FileClose(test);\r\n\
\r\n\
// Perform to read\r\n\
char[12] textR1;\r\n\
char chR1, chR2;\r\n\
short shR1, shR2;\r\n\
int numR1, numR2;\r\n\
long lnumR1, lnumR2;\r\n\
\r\n\
test = FileOpen(name, acc2);\r\n\
FileRead(test, textR1);\r\n\
FileRead(test, &chR1);\r\n\
FileRead(test, &shR1);\r\n\
FileRead(test, &numR1);\r\n\
FileRead(test, &lnumR1);\r\n\
FileRead(test, &chR2);\r\n\
FileRead(test, &shR2);\r\n\
FileRead(test, &numR2);\r\n\
FileRead(test, &lnumR2);\r\n\
FileClose(test);\r\n\
return 1;";
	printf("\r\nFile test 2\r\n");
	testCount++;
	for(int t = 0; t < 2; t++)
	{
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
	testCount++;
	for(int t = 0; t < 2; t++)
	{
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
	testCount++;
	for(int t = 0; t < 2; t++)
	{
		if(RunCode(testPrint, testTarget[t], "1"))
		{
			lastFailed = false;
			CHECK_STR("ts", 0, "Hello World!\r\noh\toh\r\n");
			CHECK_STR("$carr1", 0, "hello");
			CHECK_STR("$carr2", 0, "world");
			CHECK_STR("mo", 0, "!!!");
			if(!lastFailed)
				passed[t]++;
		}
	}



const char	*testVarGetSet1 = 
"int[10] a=4;\r\n\
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
	testCount++;
	for(int t = 0; t < 2; t++)
	{
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
			CHECK_INT("i", 0, 3);
			if(!lastFailed)
				passed[t]++;
		}
	}


const char	*testArrays = 
"int test(int a, int b, int c){ return a*b+c; }\r\n\
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
	testCount++;
	for(int t = 0; t < 2; t++)
	{
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
	testCount++;
	for(int t = 0; t < 2; t++)
	{
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

			CHECK_INT("$carr1", 0, 10);
			CHECK_INT("$carr1", 1, 12);
			CHECK_INT("$carr1", 2, 14);
			CHECK_INT("$carr1", 3, 16);
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
	testCount++;
	for(int t = 0; t < 2; t++)
	{
		if(RunCode(testVisibility, testTarget[t], "4"))
		{
			lastFailed = false;
			CHECK_INT("u", 0, 5);
			CHECK_INT("t1", 0, 9);
			CHECK_INT("t2", 0, 12);
			CHECK_INT("i", 0, 4);
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
	testCount++;
	for(int t = 0; t < 2; t++)
	{
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
	testCount++;
	for(int t = 0; t < 2; t++)
	{
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
	testCount++;
	for(int t = 0; t < 2; t++)
	{
		if(RunCode(testStrings, testTarget[t], "1"))
		{
			lastFailed = false;
			CHECK_STR("hm", 0, "World\r\n");
			CHECK_STR("$carr1", 0, "hello\r\n");

			CHECK_INT("nm", 1, 8);

			CHECK_INT("um", 1, 8);
			if(!lastFailed)
				passed[t]++;
		}
	}


const char	*testMultiCtor = 
"// Multidimensional array constructor test\r\n\
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
return 1;";
	printf("\r\nMultidimensional array constructor test\r\n");
	testCount++;
	for(int t = 0; t < 2; t++)
	{
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
	testCount++;
	for(int t = 0; t < 2; t++)
	{
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
	testCount++;
	for(int t = 0; t < 2; t++)
	{
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
int t1 = sizeof(int); // 4\r\n\
int t2 = sizeof(float4); // 16\r\n\
int t3 = sizeof({4,5,5}); // 12\r\n\
int t4 = sizeof(t3); // 4\r\n\
int t5 = sizeof(t2*0.5); // 8\r\n\
return 1;";
	printf("\r\nsizeof tests\r\n");
	testCount++;
	for(int t = 0; t < 2; t++)
	{
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
	testCount++;
	for(int t = 0; t < 2; t++)
	{
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
	testCount++;
	for(int t = 0; t < 2; t++)
	{
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
int type(int[3] x){ return 0; }\r\n\
\r\n\
int foobar(typeof(type) f) { int[3] x; x = {5, 6, 7}; return f(x); }\r\n\
int result;\r\n\
{\r\n\
  int local1 = 5, local2 = 2;\r\n\
  typeof(foobar) local3 = foobar;\r\n\
\r\n\
  int bar(typeof(type) x){ return local3(x); }\r\n\
  int foo(int[3] x){ return local1 + x[local2]; }\r\n\
\r\n\
  result = bar(foo);\r\n\
}\r\n\
\r\n\
char ee;\r\n\
return 1;";
	printf("\r\nClosure test\r\n");
	testCount++;
	for(int t = 0; t < 2; t++)
	{
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
"int f(int a){ 1; }\r\n\
\r\n\
int test(int n, typeof(f) ptr){ return ptr(n); }\r\n\
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
  typeof(f) a0, a1;\r\n\
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
	testCount++;
	for(int t = 0; t < 2; t++)
	{
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
  int f(int a){ 1; }\r\n\
  int test(int n, typeof(f) ptr){ return ptr(n); }\r\n\
 \r\n\
  int n = 5;\r\n\
  int res1 = test(3, int lambda(int b){ return b+n; });\r\n\
  return res1;\r\n\
}\r\n\
\r\n\
return main();";
	printf("\r\nClosure test 3\r\n");
	testCount++;
	for(int t = 0; t < 2; t++)
	{
		if(RunCode(testClosure3, testTarget[t], "8"))
		{
			lastFailed = false;

			if(!lastFailed)
				passed[t]++;
		}
	}

const char	*testClosure4 = 
"int func(){}\r\n\
int a = 0;\r\n\
{\r\n\
int ff(typeof(func) f){ return f(); }\r\n\
a = 1 + ff(int f1(){ return 1 + ff(int f2(){ return 1; }); });\r\n\
}\r\n\
int ff(typeof(func) f){ return f(); }\r\n\
int b = 1 + ff(int f1(){ return 1 + ff(int f2(){ return 1 + ff(int f3(){ return 1; }); }); });\r\n\
return a+b;";
	printf("\r\nClosure test 4\r\n");
	testCount++;
	for(int t = 0; t < 2; t++)
	{
		if(RunCode(testClosure4, testTarget[t], "7"))
		{
			lastFailed = false;

			if(!lastFailed)
				passed[t]++;
		}
	}

const char	*testClosure5 = 
"int func(){}\r\n\
int r1, r2, r3, r4, r5;\r\n\
{\r\n\
int ff(typeof(func) f){ return f(); }\r\n\
typeof(func) a1 = int f1(){ return 1 + ff(int f2(){ return 1; }); };\r\n\
typeof(func) a2;\r\n\
a2 = int f3(){ return 1 + ff(int f4(){ return 1; }); };\r\n\
r1 = ff(int f5(){ return 1 + ff(int f6(){ return 1; }); });\r\n\
r2 = ff(a1);\r\n\
r3 = ff(a2);\r\n\
r4 = a1();\r\n\
r5 = a2();\r\n\
}\r\n\
return 1;";
	printf("\r\nClosure test 5\r\n");
	testCount++;
	for(int t = 0; t < 2; t++)
	{
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
	testCount++;
	for(int t = 0; t < 2; t++)
	{
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
	testCount++;
	for(int t = 0; t < 2; t++)
	{
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
	testCount++;
	for(int t = 0; t < 2; t++)
	{
		if(RunCode(testDepthBreakContinue, testTarget[t], "24"))
		{
			lastFailed = false;
			CHECK_INT("a", 0, 10);
			CHECK_INT("b", 0, 14);
			if(!lastFailed)
				passed[t]++;
		}
	}

	// Conclusion
	printf("VM passed %d of %d tests\r\n", passed[0], testCount);
	printf("X86 passed %d of %d tests\r\n", passed[1], testCount);
	printf("Passed %d of %d tests\r\n", passed[0]+passed[1], testCount*2);

	printf("Compilation time: %f\r\n", timeCompile);
	printf("Get listing time: %f\r\n", timeGetListing);
	printf("Get bytecode time: %f\r\n", timeGetBytecode);
	printf("Clean time: %f\r\n", timeClean);
	printf("Link time: %f\r\n", timeLinkCode);
	printf("Run time: %f\r\n", timeRun);

	// Deinit NULLC
	nullcDeinit();
}