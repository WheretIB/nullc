#include "TestBase.h"


// function calls internal function, that perform a division
long long Recaller(int test, int testB)
{
	nullcRunFunction("inside", test, testB);
	return nullcGetResultInt();
}

// function calls an external function "Recaller"
int Recaller2(int testA, int testB)
{
	nullcRunFunction("Recaller", testA, testB);
	return (int)nullcGetResultLong();
}

// function calls internal function, that calls external function "Recaller"
int Recaller3(int testA, int testB)
{
	nullcRunFunction("inside2", testA, testB);
	return nullcGetResultInt();
}

// function calls function by NULLC pointer
int RecallerPtr(NULLCFuncPtr func)
{
	nullcCallFunction(func, 14);
	return nullcGetResultInt();
}

// sort array with comparator function inside NULLC
void BubbleSortArray(NULLCArray arr, NULLCFuncPtr comparator)
{
	int *elem = (int*)arr.ptr;

	for(unsigned int k = 0; k < arr.len; k++)
	{
		for(unsigned int l = arr.len-1; l > k; l--)
		{
			nullcCallFunction(comparator, elem[l], elem[l-1]);
			if(nullcGetResultInt())
			{
				int tmp = elem[l];
				elem[l] = elem[l-1];
				elem[l-1] = tmp;
			}
		}
	}
}

// function calls internal function
void RecallerCS(int x)
{
	nullcRunFunction("inside", x);
}

LOAD_MODULE_BIND(func_test, "func.test", "long Recaller(int testA, testB); int Recaller2(int testA, testB); int Recaller3(int testA, testB); int RecallerPtr(int ref(int) fPtr); void bubble(int[] arr, int ref(int, int) comp); void recall(int x);")
{
	nullcBindModuleFunction("func.test", (void(*)())Recaller, "Recaller", 0);
	nullcBindModuleFunction("func.test", (void(*)())Recaller2, "Recaller2", 0);
	nullcBindModuleFunction("func.test", (void(*)())Recaller3, "Recaller3", 0);
	nullcBindModuleFunction("func.test", (void(*)())RecallerPtr, "RecallerPtr", 0);
	nullcBindModuleFunction("func.test", (void(*)())BubbleSortArray, "bubble", 0);
	nullcBindModuleFunction("func.test", (void(*)())RecallerCS, "recall", 0);
}

#ifndef NULLC_ENABLE_C_TRANSLATION
const char	*testFunc1 =
"import func.test;\r\n\
int inside(int a, b){ return a / b; }\r\n\
int inside2(int a, b){ return Recaller(a, b); }\r\n\
int test(int i)\r\n\
{\r\n\
	return Recaller2(24, 2) * i;\r\n\
}\r\n\
return test(2);";
TEST_RESULT("NULLC function call externally test 1", testFunc1, "24");

const char	*testFunc1Ptr =
"import func.test;\r\n\
auto RecallerPtr_ = Recaller;\r\n\
auto Recaller2Ptr_ = Recaller2;\r\n\
int inside(int a, b){ return a / b; }\r\n\
int inside2(int a, b){ return RecallerPtr_(a, b); }\r\n\
int test(int i)\r\n\
{\r\n\
	return Recaller2Ptr_(24, 2) * i;\r\n\
}\r\n\
return test(2);";
TEST_RESULT("NULLC function call externally test 1 (with pointers to functions)", testFunc1Ptr, "24");

const char	*testFunc2 =
"import func.test;\r\n\
int inside(int a, b){ return a / b; }\r\n\
int inside2(int a, b){ return Recaller(a, b); }\r\n\
int test(int i)\r\n\
{\r\n\
	return Recaller3(24, 2) * i;\r\n\
}\r\n\
return test(2);";
TEST_RESULT("NULLC function call externally test 2", testFunc2, "24");

const char	*testFunc2Ptr =
"import func.test;\r\n\
auto RecallerPtr_ = Recaller;\r\n\
auto Recaller3Ptr_ = Recaller3;\r\n\
int inside(int a, b){ return a / b; }\r\n\
int inside2(int a, b){ return RecallerPtr_(a, b); }\r\n\
int test(int i)\r\n\
{\r\n\
	return Recaller3Ptr_(24, 2) * i;\r\n\
}\r\n\
return test(2);";
TEST_RESULT("NULLC function call externally test 2 (with pointers to functions)", testFunc2Ptr, "24");

const char	*testFunc3 =
"import func.test;\r\n\
return RecallerPtr(auto(int i){ return -i; });";
TEST_RESULT("NULLC function call externally test 3", testFunc3, "-14");

const char	*testFunc3b =
"import func.test;\r\n\
int a = 3;\r\n\
int foo(int x)\r\n\
{\r\n\
	assert(x == 5);\r\n\
	auto y = RecallerPtr(auto(int i){ return -i; });\r\n\
	assert(x == 5);\r\n\
	return y;\r\n\
}\r\n\
assert(a == 3);\r\n\
auto m = foo(5);\r\n\
assert(a == 3);\r\n\
return m;";
TEST_RESULT("NULLC function call externally test 3 b", testFunc3b, "-14");

const char	*testFunc3Ptr =
"import func.test;\r\n\
auto RecallerPtr_ = RecallerPtr;\r\n\
return RecallerPtr_(auto(int i){ return -i; });";
TEST_RESULT("NULLC function call externally test 3 (with pointers to functions)", testFunc3Ptr, "-14");

const char	*testFunc4 =
"import func.test;\r\n\
auto generator(int start)\r\n\
{\r\n\
	return auto(int u){ return ++start; };\r\n\
}\r\n\
return RecallerPtr(generator(7));";
TEST_RESULT("NULLC function call externally test 4", testFunc4, "8");

const char	*testFunc5 =
"import func.test;\r\n\
int seed = 5987;\r\n\
int[512] arr;\r\n\
for(int i = 0; i < 512; i++)\r\n\
	arr[i] = (((seed = seed * 214013 + 2531011) >> 16) & 0x7fff);\r\n\
bubble(arr, auto(int a, b){ return int(a > b); });\r\n\
return arr[8];";
TEST_RESULT("NULLC function call externally test 5", testFunc5, "32053");
#endif
