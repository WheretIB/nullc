#include "TestBase.h"

const char *testSglVector1 =
"import sgl.vector;\r\n\
\r\n\
auto x = new vector<int>;\r\n\
auto y = new vector<float>(14);\r\n\
\r\n\
return x.capacity() + y.capacity();";
TEST_RESULT("sgl.vector test (constructors, capacity)", testSglVector1, "14");

const char *testSglVector2 =
"import sgl.vector;\r\n\
\r\n\
auto x = new vector<int>;\r\n\
x.push_back(4);\r\n\
x.push_back(3);\r\n\
x.push_back(2);\r\n\
x.pop_back();\r\n\
x.push_back(1);\r\n\
x.push_back(0);\r\n\
\r\n\
assert(x.size() == 4);\r\n\
assert(x[0] == 4);\r\n\
assert(x[1] == 3);\r\n\
assert(x[2] == 1);\r\n\
assert(x[3] == 0);\r\n\
assert(*x.back() == 0);\r\n\
assert(*x.front() == 4);\r\n\
\r\n\
return 1;";
TEST_RESULT("sgl.vector test (push_back, pop_back, size, read: [], back, front)", testSglVector2, "1");

const char *testSglVector3 =
"import sgl.vector;\r\n\
\r\n\
auto x = new vector<int>;\r\n\
x.resize(4);\r\n\
x[0] = 4;\r\n\
x[1] = 3.2;\r\n\
x[2] = 2;\r\n\
x[3] = 1;\r\n\
x.front() = 40;\r\n\
x.back() = 10;\r\n\
\r\n\
assert(x.size() == 4);\r\n\
assert(x[0] == 40);\r\n\
assert(x[1] == 3);\r\n\
assert(x[2] == 2);\r\n\
assert(x[3] == 10);\r\n\
assert(*x.back() == 10);\r\n\
assert(*x.front() == 40);\r\n\
\r\n\
return 1;";
TEST_RESULT("sgl.vector test (resize, write: [], back, front)", testSglVector3, "1");

const char *testSglVector4 =
"import sgl.vector;\r\n\
\r\n\
auto x = new vector<int>;\r\n\
x.reserve(42);\r\n\
assert(x.size() == 0);\r\n\
assert(x.capacity() >= 42);\r\n\
\r\n\
x.resize(16);\r\n\
assert(x.size() == 16);\r\n\
assert(x.capacity() == 16);\r\n\
\r\n\
x.clear();\r\n\
assert(x.size() == 0);\r\n\
assert(x.capacity() == 16);\r\n\
\r\n\
x.destroy();\r\n\
assert(x.size() == 0);\r\n\
assert(x.capacity() == 0);\r\n\
\r\n\
return 1;";
TEST_RESULT("sgl.vector test (reserve, resize, capacity, clear, destroy)", testSglVector4, "1");

const char *testSglVector5 =
"import sgl.vector;\r\n\
\r\n\
auto x = new vector<int>;\r\n\
x.push_back(4);\r\n\
x.push_back(3);\r\n\
x.push_back(2);\r\n\
x.push_back(1);\r\n\
\r\n\
int sum = 0;\r\n\
for(i in x)\r\n\
	sum += i;\r\n\
return sum;";
TEST_RESULT("sgl.vector test (iteration)", testSglVector5, "10");

const char *testSglVector6 =
"import sgl.vector;\r\n\
\r\n\
auto arr = new vector<int>;\r\n\
arr.push_back(1);\r\n\
arr.push_back(2);\r\n\
arr.push_back(3);\r\n\
arr.push_back(4);\r\n\
\r\n\
auto sp1 = arr[1, 3];\r\n\
auto sp2 = arr[2, 2];\r\n\
return sp1[2] + sp2[0] + arr[0, 0][0];";
TEST_RESULT("sgl.vector test (array splices)", testSglVector6, "8");

const char *testSglVector7 =
"import std.math;\r\n\
import sgl.vector;\r\n\
import std.algorithm;\r\n\
\r\n\
auto arr = new vector<float3>;\r\n\
arr.push_back(float3(0, 1, 0));\r\n\
arr.push_back(float3(0, 5, 0));\r\n\
arr.push_back(float3(0, 10, 0));\r\n\
arr.push_back(float3(0, 20, 0));\r\n\
\r\n\
assert(36 == arr.sum(<x>{ x.y; }));\r\n\
assert(9 == arr.average(<x>{ x.y; }));\r\n\
assert(1 == arr.min_element(<x>{ x.y; }));\r\n\
assert(20 == arr.max_element(<x>{ x.y; }));\r\n\
{ // make operators local to this block\r\n\
	auto operator<(float3 ref a, b){ return a.y < b.y; }\r\n\
	auto operator>(float3 ref a, b){ return a.y > b.y; }\r\n\
	assert(1 == arr.min_element().y);\r\n\
	assert(20 == arr.max_element().y);\r\n\
}\r\n\
assert(2 == arr.count_if(<x>{ x.y > 8; }));\r\n\
assert(0 == arr.count_if(<x>{ x.y > 100; }));\r\n\
\r\n\
assert(0 == arr.all(<x>{ x.y > 8; }));\r\n\
assert(1 == arr.all(<x>{ x.y >= 1; }));\r\n\
\r\n\
assert(1 == arr.any(<x>{ x.y > 8; }));\r\n\
assert(0 == arr.any(<x>{ x.y > 100; }));\r\n\
\r\n\
arr.sort(<x, y>{ x.y > y.y; });\r\n\
assert(arr[0].y == 20 && arr[1].y == 10 && arr[2].y == 5 && arr[3].y == 1);\r\n\
arr.sort(<x, y>{ x.y < y.y; });\r\n\
assert(arr[0].y == 1 && arr[1].y == 5 && arr[2].y == 10 && arr[3].y == 20);\r\n\
\r\n\
return 1;";
TEST_RESULT("sgl.vector test (aggregation functions)", testSglVector7, "1");

const char *testSglVector8 =
"import sgl.vector;\r\n\
\r\n\
auto x = new vector<int>;\r\n\
x.push_back(4);\r\n\
x.push_back(3);\r\n\
x.push_back(2);\r\n\
x.push_back(1);\r\n\
\r\n\
for(i in x)\r\n\
	i += 2;\r\n\
int sum = 0;\r\n\
for(i in x)\r\n\
	sum += i;\r\n\
return sum;";
TEST_RESULT("sgl.vector test (iteration) 2", testSglVector8, "18");

const char *testSglVector9 =
"import std.math;\r\n\
import sgl.vector;\r\n\
import std.algorithm;\r\n\
\r\n\
auto arr = new vector<float3>;\r\n\
arr.push_back(float3(0, 5, 0));\r\n\
\r\n\
assert(5 == arr.sum(<x>{ x.y; }));\r\n\
assert(5 == arr.average(<x>{ x.y; }));\r\n\
assert(5 == arr.min_element(<x>{ x.y; }));\r\n\
assert(5 == arr.max_element(<x>{ x.y; }));\r\n\
{ // make operators local to this block\r\n\
	auto operator<(float3 ref a, b){ return a.y < b.y; }\r\n\
	auto operator>(float3 ref a, b){ return a.y > b.y; }\r\n\
	assert(5 == arr.min_element().y);\r\n\
	assert(5 == arr.max_element().y);\r\n\
}\r\n\
assert(0 == arr.count_if(<x>{ x.y > 8; }));\r\n\
assert(1 == arr.count_if(<x>{ x.y > 4; }));\r\n\
\r\n\
assert(0 == arr.all(<x>{ x.y > 8; }));\r\n\
assert(1 == arr.all(<x>{ x.y > 1; }));\r\n\
\r\n\
assert(0 == arr.any(<x>{ x.y > 8; }));\r\n\
assert(1 == arr.any(<x>{ x.y > 1; }));\r\n\
\r\n\
arr.sort(<x, y>{ x.y > y.y; });\r\n\
assert(arr[0].y == 5);\r\n\
arr.sort(<x, y>{ x.y < y.y; });\r\n\
assert(arr[0].y == 5);\r\n\
\r\n\
return 1;";
TEST_RESULT("sgl.vector test (aggregation functions) one element", testSglVector9, "1");
