#include "TestBase.h"

const char	*testForEachUserType =
"class array\r\n\
{\r\n\
	int[] arr;\r\n\
}\r\n\
class array_iterator\r\n\
{\r\n\
	array ref arr;\r\n\
	int pos;\r\n\
}\r\n\
auto array(int size)\r\n\
{\r\n\
	array ret;\r\n\
	ret.arr = new int[size];\r\n\
	return ret;\r\n\
}\r\n\
auto array::start()\r\n\
{\r\n\
	array_iterator iter;\r\n\
	iter.arr = this;\r\n\
	iter.pos = 0;\r\n\
	return iter;\r\n\
}\r\n\
int ref array_iterator::next()\r\n\
{\r\n\
	return &arr.arr[pos++];\r\n\
}\r\n\
int array_iterator::hasnext()\r\n\
{\r\n\
	return pos < arr.arr.size;\r\n\
}\r\n\
\r\n\
array arr = array(16);\r\n\
int u = 1;\r\n\
for(i in arr)\r\n\
	i = u++;\r\n\
for(i in arr)\r\n\
	i *= i;\r\n\
int sum = 0;\r\n\
for(i in arr)\r\n\
	sum += i;\r\n\
return sum;";
TEST_RESULT("For each on user type", testForEachUserType, "1496");

const char	*testForEach2 =
"import std.vector;\r\n\
auto a = vector<int>();\r\n\
a.push_back(4);\r\n\
a.push_back(8);\r\n\
a.push_back(14);\r\n\
\r\n\
int sum = 0;\r\n\
for(int i in a)\r\n\
{\r\n\
	sum += i;\r\n\
	i *= 2;\r\n\
}\r\n\
int sum2 = 0;\r\n\
for(int i in a)\r\n\
{\r\n\
	sum2 += i;\r\n\
}\r\n\
return sum + sum2;";
TEST_RESULT("For each with specified element type", testForEach2, "78");

const char	*testForEach2b =
"import std.vector;\r\n\
auto a = vector<int>();\r\n\
a.push_back(4);\r\n\
a.push_back(8);\r\n\
a.push_back(14);\r\n\
\r\n\
int sum = 0;\r\n\
for(auto i in a)\r\n\
{\r\n\
	sum += i;\r\n\
	i *= 2;\r\n\
}\r\n\
int sum2 = 0;\r\n\
for(auto i in a)\r\n\
{\r\n\
	sum2 += i;\r\n\
}\r\n\
return sum + sum2;";
TEST_RESULT("For each with specified element type 2", testForEach2b, "78");

const char	*testForEach3 =
"int[] arr1 = { 2, 6, 7 };\r\n\
int sum1 = 0;\r\n\
for(i in arr1)\r\n\
	sum1 += i;\r\n\
\r\n\
int[3] arr2 = { 2, 6, 7 };\r\n\
int sum2 = 0;\r\n\
for(i in arr2)\r\n\
	sum2 += i;\r\n\
\r\n\
auto arr3 = { { 2, 6, 7 }, { 1, 2, 3 } };\r\n\
int sum3 = 0;\r\n\
for(line in arr3)\r\n\
	for(num in line)\r\n\
		sum3 += num;\r\n\
\r\n\
auto func(){ return { 1, 6 }; }\r\n\
int sum4 = 0;\r\n\
for(i in func())\r\n\
	sum4 += i;\r\n\
\r\n\
int sum5 = 0;\r\n\
for(i in { 1, 2, 3 })\r\n\
	sum5 += i;\r\n\
return 0;";
TEST("For each for standard arrays", testForEach3, "0")
{
	CHECK_INT("sum1", 0, 15, lastFailed);
	CHECK_INT("sum2", 0, 15, lastFailed);
	CHECK_INT("sum3", 0, 21, lastFailed);
	CHECK_INT("sum4", 0, 7, lastFailed);
	CHECK_INT("sum5", 0, 6, lastFailed);
}

const char	*testForEach4 =
"int sum = 0;\r\n\
for(i in { 1, 2, 3 }, j in { 4, 5, 6, 7 })\r\n\
	sum += i + j;\r\n\
\r\n\
int[3] arr1 = { 2, 6, 7 };\r\n\
int[] arr2 = { 8, -4, 2 };\r\n\
for(i in arr1, j in arr2)\r\n\
	i += j;\r\n\
return sum;";
TEST("For each with multiple arrays", testForEach4, "21")
{
	CHECK_INT("arr1", 0, 10, lastFailed);
	CHECK_INT("arr1", 1, 2, lastFailed);
	CHECK_INT("arr1", 2, 9, lastFailed);
}

const char	*testRangeIterator =
"class range_iterator\r\n\
{\r\n\
	int pos;\r\n\
	int max;\r\n\
}\r\n\
auto range_iterator::start()\r\n\
{\r\n\
	return *this;\r\n\
}\r\n\
int range_iterator::next()\r\n\
{\r\n\
	pos++;\r\n\
	return pos - 1;\r\n\
}\r\n\
int range_iterator::hasnext()\r\n\
{\r\n\
	return pos != max;\r\n\
}\r\n\
auto range(int min, max)\r\n\
{\r\n\
	range_iterator r;\r\n\
	r.pos = min;\r\n\
	r.max = max + 1;\r\n\
	return r;\r\n\
}\r\n\
int factorial(int v)\r\n\
{\r\n\
	int fact = 1;\r\n\
	for(i in range(1, v))\r\n\
		fact *= i;\r\n\
	return fact;\r\n\
}\r\n\
return factorial(10);";
TEST_RESULT("Extra node wrapping in for each with function call in array part", testRangeIterator, "3628800");

const char	*testForEach5 =
"import std.vector;\r\n\
class Test\r\n\
{\r\n\
	vector<int> a;\r\n\
}\r\n\
auto a = new Test;\r\n\
a.a = vector<int>();\r\n\
a.a.push_back(10);\r\n\
a.a.push_back(105);\r\n\
int sum = 0;\r\n\
for(int i in a.a) sum += i;\r\n\
return sum;";
TEST_RESULT("For each on a member of a type that we had a reference to", testForEach5, "115");

const char	*testInOperatorOnArray =
"int[] arr = { 1, 2, 3, 4 };\r\n\
return (3 in arr) && !(6 in arr);";
TEST_RESULT("in operator on array", testInOperatorOnArray, "1");
