#include "TestBase.h"

const char	*testCoroutine0 =
"coroutine int produce()\r\n\
{\r\n\
	int i;\r\n\
	for(i = 0; i < 2; i++)\r\n\
		yield i;\r\n\
	return 0;\r\n\
}\r\n\
int[5] arr;\r\n\
arr[0] = produce();\r\n\
arr[1] = produce();\r\n\
arr[2] = produce();\r\n\
arr[3] = produce();\r\n\
arr[4] = produce();\r\n\
return arr[0] * 10000 + arr[1] * 1000 + arr[2] * 100 + arr[3] * 10 + arr[4];";
TEST_RESULT("Coroutine simple 1.", testCoroutine0, "1001");

const char	*testCoroutine1 =
"coroutine int produce()\r\n\
{\r\n\
	for(int i = 0; i < 2; i++)\r\n\
		yield i;\r\n\
	return 0;\r\n\
}\r\n\
int[5] arr;\r\n\
arr[0] = produce();\r\n\
arr[1] = produce();\r\n\
arr[2] = produce();\r\n\
arr[3] = produce();\r\n\
arr[4] = produce();\r\n\
return arr[0] * 10000 + arr[1] * 1000 + arr[2] * 100 + arr[3] * 10 + arr[4];";
TEST_RESULT("Coroutine simple 2.", testCoroutine1, "1001");

const char	*testCoroutine4 =
"coroutine int foo()\r\n\
{\r\n\
	int i;\r\n\
	i = 0;\r\n\
	yield i++;\r\n\
	yield i++;\r\n\
	yield i++;\r\n\
	return i;\r\n\
}\r\n\
int[6] arr;\r\n\
for(i in arr)\r\n\
	i = foo();\r\n\
return arr[0] * 100000 + arr[1] * 10000 + arr[2] * 1000 + arr[3] * 100 + arr[4] * 10 + arr[5];";
TEST_RESULT("Coroutine simple 5.", testCoroutine4, "12301");

const char	*testCoroutine5 =
"coroutine int foo()\r\n\
{\r\n\
	int i = 0;\r\n\
	yield i++;\r\n\
	yield i++;\r\n\
	yield i++;\r\n\
	return i;\r\n\
}\r\n\
int[6] arr;\r\n\
for(i in arr)\r\n\
	i = foo();\r\n\
return arr[0] * 100000 + arr[1] * 10000 + arr[2] * 1000 + arr[3] * 100 + arr[4] * 10 + arr[5];";
TEST_RESULT("Coroutine simple 6.", testCoroutine5, "12301");

#if 0	// It is not clear if coroutine context should be cleared every time at the beginning of a function
const char	*testCoroutine6 =
"coroutine int foo()\r\n\
{\r\n\
	int i;\r\n\
	yield i++;\r\n\
	yield i++;\r\n\
	yield i++;\r\n\
	return i;\r\n\
}\r\n\
int[6] arr;\r\n\
for(i in arr)\r\n\
	i = foo();\r\n\
return arr[0] * 100000 + arr[1] * 10000 + arr[2] * 1000 + arr[3] * 100 + arr[4] * 10 + arr[5];";
TEST_RESULT("Coroutine simple 7.", testCoroutine6, "12301");
#endif

const char	*testCoroutine7 =
"coroutine int gen3base(int x)\r\n\
{\r\n\
	int i;\r\n\
	for(i = x; i < x + 3; i++)\r\n\
		yield i;\r\n\
	return -1;\r\n\
}\r\n\
return gen3base(2) + gen3base(2) + gen3base(2) + gen3base(2);";
TEST_RESULT("Coroutine simple 8 (with arguments).", testCoroutine7, "8");

const char	*testCoroutine8 =
"coroutine int gen3base(int x)\r\n\
{\r\n\
	for(int i = x; i < x + 3; i++)\r\n\
		yield i;\r\n\
	return -1;\r\n\
}\r\n\
return gen3base(2) + gen3base(2) + gen3base(2) + gen3base(2);";
TEST_RESULT("Coroutine simple 9 (with arguments).", testCoroutine8, "8");

const char	*testCoroutine9 =
"auto get3GenFrom(int x)\r\n\
{\r\n\
	coroutine int gen3from()\r\n\
	{\r\n\
		for(int i = x; i < x + 3; i++)\r\n\
			yield i;\r\n\
		return 0;\r\n\
	}\r\n\
	return gen3from;\r\n\
}\r\n\
auto gen3from5 = get3GenFrom(5);\r\n\
auto gen3from2 = get3GenFrom(2);\r\n\
\r\n\
int t1 = 0, t2 = 0, t3 = 0, t4 = 0;\r\n\
int[6] arr1;\r\n\
for(i in arr1)\r\n\
{\r\n\
	i = gen3from5();\r\n\
	t1 = t1 * 10 + i;\r\n\
}\r\n\
\r\n\
int[6] arr2;\r\n\
for(i in arr2)\r\n\
{\r\n\
	i = gen3from2();\r\n\
	t2 = t2 * 10 + i;\r\n\
}\r\n\
\r\n\
int[6] arr3, arr4;\r\n\
for(i in arr3, j in arr4)\r\n\
{\r\n\
	i = gen3from5();\r\n\
	j = gen3from2();\r\n\
	t3 = t3 * 10 + i;\r\n\
	t4 = t4 * 10 + j;\r\n\
}\r\n\
return (t1 == 567056) + (t2 == 234023) + (t3 == 705670) + (t4 == 402340);";
TEST_RESULT("Coroutine 10 (coroutine in local function).", testCoroutine9, "4");

const char	*testCoroutine10 =
"auto get3GenFrom(int x)\r\n\
{\r\n\
	coroutine int gen3from()\r\n\
	{\r\n\
		int ref() m;\r\n\
		for(int i = x; i < x + 3; i++)\r\n\
		{\r\n\
			int help()\r\n\
			{\r\n\
				return i;\r\n\
			}\r\n\
			yield help();\r\n\
			m = help;\r\n\
		}\r\n\
		return m();\r\n\
	}\r\n\
	return gen3from;\r\n\
}\r\n\
auto gen3from5 = get3GenFrom(5);\r\n\
auto gen3from2 = get3GenFrom(2);\r\n\
\r\n\
int t1 = 0, t2 = 0;\r\n\
int[6] arr1;\r\n\
for(i in arr1)\r\n\
{\r\n\
	i = gen3from5();\r\n\
	t1 = t1 * 10 + i;\r\n\
}\r\n\
\r\n\
int[6] arr2;\r\n\
for(i in arr2)\r\n\
{\r\n\
	i = gen3from2();\r\n\
	t2 = t2 * 10 + i;\r\n\
}\r\n\
return (t1 == 567756) + (t2 == 234423);";
TEST_RESULT("Coroutine 11 (coroutine in local function with local function inside).", testCoroutine10, "2");

const char	*testCoroutine11 =
"coroutine int gen3from(int xx)\r\n\
{\r\n\
	int x = 3;\r\n\
	int ref() m;\r\n\
	for(int i = 0; i < 3; i++)\r\n\
	{\r\n\
		int help()\r\n\
		{\r\n\
			return x + i;\r\n\
		}\r\n\
		yield help();\r\n\
		m = help;\r\n\
	}\r\n\
	return m();\r\n\
}\r\n\
auto gen3from5 = gen3from;\r\n\
auto gen3from2 = gen3from;\r\n\
\r\n\
int t1 = 0, t2 = 0;\r\n\
int[6] arr1;\r\n\
for(i in arr1)\r\n\
{\r\n\
	i = gen3from5(5);\r\n\
	t1 = t1 * 10 + i;\r\n\
}\r\n\
\r\n\
int[6] arr2;\r\n\
for(i in arr2)\r\n\
{\r\n\
	i = gen3from2(2);\r\n\
	t2 = t2 * 10 + i;\r\n\
}\r\n\
return (t1 == 345534) + (t2 == 553455);";
TEST_RESULT("Coroutine 12 (coroutine with local function inside).", testCoroutine11, "2");

const char	*testCoroutine12 =
"coroutine int gen3from(int x)\r\n\
{\r\n\
	int ref() m;\r\n\
	for(int i = 0; i < 3; i++)\r\n\
	{\r\n\
		int help()\r\n\
		{\r\n\
			return x + i;\r\n\
		}\r\n\
		yield help();\r\n\
		m = help;\r\n\
	}\r\n\
	return m();\r\n\
}\r\n\
auto gen3from5 = gen3from;\r\n\
auto gen3from2 = gen3from;\r\n\
\r\n\
int t1 = 0, t2 = 0;\r\n\
int[6] arr1;\r\n\
for(i in arr1)\r\n\
{\r\n\
	i = gen3from5(5);\r\n\
	t1 = t1 * 10 + i;\r\n\
}\r\n\
\r\n\
int[6] arr2;\r\n\
for(i in arr2)\r\n\
{\r\n\
	i = gen3from2(2);\r\n\
	t2 = t2 * 10 + i;\r\n\
}\r\n\
return (t1 == 567756) + (t2 == 442344);";
TEST_RESULT("Coroutine 13 (coroutine with local function inside, argument closure).", testCoroutine12, "2");

const char	*testCoroutineExampleA =
"import std.vector;\r\n\
auto forward_iterator(vector ref x)\r\n\
{\r\n\
	coroutine auto ref iterate()\r\n\
	{\r\n\
		for(int i = 0; i < x.size(); i++)\r\n\
			yield x[i];\r\n\
		return nullptr;\r\n\
	}\r\n\
	return iterate;\r\n\
}\r\n\
vector a = vector(int);\r\n\
a.push_back(4);\r\n\
a.push_back(5);\r\n\
\r\n\
auto i = forward_iterator(a);\r\n\
return int(i()) + int(i());";
TEST_RESULT("Coroutine example A.", testCoroutineExampleA, "9");

LOAD_MODULE(test_coroutine1, "test.coroutine1", "coroutine int foo(){ int i = 10; while(i) yield i++; }");
const char	*testCoroutineImport1 =
"import test.coroutine1;\r\n\
return foo() + foo();";
TEST_RESULT("Coroutine export and import 1", testCoroutineImport1, "21");

const char	*testInlineCoroutine =
"auto f = coroutine auto(){ yield 1; yield 2; };\r\n\
int a = f(), b = f();\r\n\
return a * 10 + b;";
TEST_RESULT("Inline coroutine", testInlineCoroutine, "12");

const char *testInlineFunctionCall = "return (auto(){ return 5; })();";
TEST_RESULT("Inline function definition and call", testInlineFunctionCall, "5");

const char *testCorountineIterator = 
"class list_node\r\n\
{\r\n\
	list_node ref next;\r\n\
	int value;\r\n\
}\r\n\
auto list_node:start()\r\n\
{\r\n\
	return coroutine auto(){ list_node ref c = this; while(c){ yield c.value; c = c.next; } return 0; };\r\n\
}\r\n\
list_node list;\r\n\
list.value = 2;\r\n\
list.next = new list_node;\r\n\
list.next.value = 5;\r\n\
int product = 1;\r\n\
for(i in list)\r\n\
product *= i;\r\n\
return product;";
TEST_RESULT("Object iterator is a coroutine", testCorountineIterator, "10");

const char *testCorountineScopes = 
"coroutine int foo()\r\n\
{\r\n\
	int current = 0;\r\n\
	{\r\n\
		int current = 1;\r\n\
	}\r\n\
	return current;\r\n\
}\r\n\
return foo();";
TEST_RESULT("Coroutine with multiple variables with the same name in different scopes", testCorountineScopes, "0");

const char *testCorountinePrototype = 
"coroutine int foo();\r\n\
int a = foo();\r\n\
coroutine int foo(){ yield 1; return 2; }\r\n\
return a + foo();";
TEST_RESULT("Coroutine prototype", testCorountinePrototype, "3");

const char *testCorountinePrototype2 = 
"coroutine int foo();\r\n\
int bar()\r\n\
{\r\n\
	return foo();\r\n\
}\r\n\
coroutine int foo()\r\n\
{\r\n\
	return 5;\r\n\
}\r\n\
return bar();";
TEST_RESULT("Coroutine prototype 2", testCorountinePrototype2, "5");

const char	*testCoroutineImport2 =
"import test.coroutine1;\r\n\
auto x = foo;\r\n\
return x() + x();";
TEST_RESULT("Coroutine export and import 2", testCoroutineImport2, "21");

const char	*testCoroutinePrototype3 =
"coroutine int foo(int x)\r\n\
{\r\n\
	coroutine int bar();\r\n\
	for(;1;)\r\n\
		yield bar();\r\n\
	coroutine int bar()\r\n\
	{\r\n\
		while(1)\r\n\
			yield x++;\r\n\
	}\r\n\
}\r\n\
int a = foo(4);\r\n\
int b = foo(4);\r\n\
return a + b + foo(4);";
TEST_RESULT("Coroutine prototype 3", testCoroutinePrototype3, "15");

const char	*testCoroutineRecursion =
"int u = 0;\r\n\
coroutine int foo(int x)\r\n\
{\r\n\
	u += x;\r\n\
	for(x++; x <= 4; x++)\r\n\
	{\r\n\
		foo(x);\r\n\
	}\r\n\
	return x;\r\n\
}\r\n\
foo(0);\r\n\
return u;";
TEST_RESULT("Coroutine recursion", testCoroutineRecursion, "49");
