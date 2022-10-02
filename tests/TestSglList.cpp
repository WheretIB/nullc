#include "TestBase.h"

/*// sgl.list

auto list_node.value();
auto list_node.value(T ref val);

void list::list();

void list::push_back(T ref elem);!
void list::push_front(T ref elem);!
void list::pop_back();!
void list::pop_front();!
void list::insert(list_node<T> ref it, T ref elem);
void list::erase(list_node<T> ref it);
void list::clear();!
auto list::back();!
auto list::front();!
auto list::begin();
auto list::end();
int list::empty();!

auto list_iterator::list_iterator(list_node<T> ref start);
auto list::start();!
auto list_iterator::next();!
auto list_iterator::hasnext();!
*/

const char *testSglList1 =
"import std.list;\r\n\
\r\n\
auto x = new list<int>;\r\n\
x.push_back(4);\r\n\
x.push_back(3);\r\n\
x.push_back(2);\r\n\
x.pop_back();\r\n\
x.push_back(1);\r\n\
x.push_back(0);\r\n\
x.pop_front();\r\n\
x.push_front(7);\r\n\
\r\n\
int a = 1;\r\n\
for(i in x, j in {7, 3, 1, 0})\r\n\
	a = a && i == j;\r\n\
assert(a);\r\n\
assert(*x.back() == 0);\r\n\
assert(*x.front() == 7);\r\n\
for(i in x)\r\n\
	i++;\r\n\
for(i in x, j in {8, 4, 2, 1})\r\n\
	a = a && i == j;\r\n\
assert(a);\r\n\
assert(*x.back() == 1);\r\n\
assert(*x.front() == 8);\r\n\
\r\n\
return 1;";
TEST_RESULT("sgl.list test (push_back, pop_back, push_front, pop_front, back, front, iteration)", testSglList1, "1");

const char *testSglList2 =
"import std.list;\r\n\
list<int> arr;\r\n\
arr.push_back(1);\r\n\
arr.push_back(2);\r\n\
arr.push_back(3);\r\n\
arr.push_back(4);\r\n\
int sum = 0;\r\n\
for(int i in arr)\r\n\
	sum += i;\r\n\
return sum;";
TEST_RESULT("sgl.list test (push_back, iteration)", testSglList2, "10");

const char *testSglList3 =
"import std.list;\r\n\
list<int> arr;\r\n\
arr.push_back(1);\r\n\
assert(!arr.empty());\r\n\
arr.clear();\r\n\
assert(arr.empty());\r\n\
return 1;";
TEST_RESULT("sgl.list test (clear, empty)", testSglList3, "1");

const char *testSglList4 =
"import std.math;\r\n\
import std.list;\r\n\
\r\n\
auto arr = new list<float3>;\r\n\
arr.push_back(float3(0, 1, 0));\r\n\
arr.push_back(float3(0, 5, 0));\r\n\
arr.push_back(float3(0, 10, 0));\r\n\
arr.push_back(float3(0, 20, 0));\r\n\
\r\n\
assert(36 == arr.sum(<x>{ x.y; }));\r\n\
assert(9 == arr.average(<x>{ x.y; }));\r\n\
assert(1 == arr.min_element(<x>{ x.y; }));\r\n\
assert(20 == arr.max_element(<x>{ x.y; }));\r\n\
\r\n\
assert(2 == arr.count_if(<x>{ x.y > 8; }));\r\n\
assert(0 == arr.count_if(<x>{ x.y > 100; }));\r\n\
\r\n\
assert(0 == arr.all(<x>{ x.y > 8; }));\r\n\
assert(1 == arr.all(<x>{ x.y >= 1; }));\r\n\
\r\n\
assert(1 == arr.any(<x>{ x.y > 8; }));\r\n\
assert(0 == arr.any(<x>{ x.y > 100; }));\r\n\
\r\n\
return 1;";
TEST_RESULT("sgl.list test (aggregation functions)", testSglList4, "1");

const char *testSglList4b =
"import std.math;\r\n\
import std.list;\r\n\
\r\n\
auto arr = new list<float3>;\r\n\
arr.push_back(float3(0, 1, 0));\r\n\
arr.push_back(float3(0, 5, 0));\r\n\
arr.push_back(float3(0, 10, 0));\r\n\
arr.push_back(float3(0, 20, 0));\r\n\
\r\n\
auto operator<(float3 ref a, b){ return a.y < b.y; }\r\n\
auto operator>(float3 ref a, b){ return a.y > b.y; }\r\n\
assert(1 == arr.min_element().y);\r\n\
assert(20 == arr.max_element().y);\r\n\
\r\n\
return 1;";
TEST_RESULT_SIMPLE("sgl.list test (aggregation functions) 2", testSglList4b, "1");

const char *testSglList5 =
"import std.math;\r\n\
import std.list;\r\n\
\r\n\
auto arr = new list<float3>;\r\n\
arr.push_back(float3(0, 5, 0));\r\n\
\r\n\
assert(5 == arr.sum(<x>{ x.y; }));\r\n\
assert(5 == arr.average(<x>{ x.y; }));\r\n\
assert(5 == arr.min_element(<x>{ x.y; }));\r\n\
assert(5 == arr.max_element(<x>{ x.y; }));\r\n\
\r\n\
assert(0 == arr.count_if(<x>{ x.y > 8; }));\r\n\
assert(1 == arr.count_if(<x>{ x.y > 4; }));\r\n\
\r\n\
assert(0 == arr.all(<x>{ x.y > 8; }));\r\n\
assert(1 == arr.all(<x>{ x.y > 1; }));\r\n\
\r\n\
assert(0 == arr.any(<x>{ x.y > 8; }));\r\n\
assert(1 == arr.any(<x>{ x.y > 1; }));\r\n\
\r\n\
return 1;";
TEST_RESULT("sgl.list test (aggregation functions) one element", testSglList5, "1");

const char *testSglList5b =
"import std.math;\r\n\
import std.list;\r\n\
\r\n\
auto arr = new list<float3>;\r\n\
arr.push_back(float3(0, 5, 0));\r\n\
\r\n\
auto operator<(float3 ref a, b){ return a.y < b.y; }\r\n\
auto operator>(float3 ref a, b){ return a.y > b.y; }\r\n\
assert(5 == arr.min_element().y);\r\n\
assert(5 == arr.max_element().y);\r\n\
\r\n\
return 1;";
TEST_RESULT_SIMPLE("sgl.list test (aggregation functions) one element 2", testSglList5b, "1");

const char *testSglList6 =
"import std.list;\r\n\
\r\n\
list<auto ref> arr;\r\n\
arr.push_back(new int(4));\r\n\
arr.push_front(new double(7));\r\n\
\r\n\
auto m = arr.begin();\r\n\
arr.insert(m, new float(3));\r\n\
\r\n\
auto ref[3] flat;\r\n\
for(i in flat, j in arr)\r\n\
i = j;\r\n\
\r\n\
assert(typeid(flat[0]) == double);\r\n\
assert(typeid(flat[1]) == float);\r\n\
assert(typeid(flat[2]) == int);\r\n\
assert(double(flat[0]) == 7.0);\r\n\
assert(float(flat[1]) == 3);\r\n\
assert(int(flat[2]) == 4);\r\n\
\r\n\
return 1;";
TEST_RESULT("sgl.list test (auto ref elements)", testSglList6, "1");

const char *testSglList7 =
"import std.list;\r\n\
\r\n\
list<int ref> arr;\r\n\
arr.push_back(new int(4));\r\n\
arr.push_front(new int(7));\r\n\
\r\n\
auto m = arr.begin();\r\n\
arr.insert(m, new int(3));\r\n\
\r\n\
int[3] flat;\r\n\
for(i in flat, j in arr)\r\n\
	i = *j;\r\n\
\r\n\
assert(flat[0] == 7);\r\n\
assert(flat[1] == 3);\r\n\
assert(flat[2] == 4);\r\n\
\r\n\
return 1;";
TEST_RESULT("sgl.list test (reference elements)", testSglList7, "1");

const char *testSglList8 =
"import std.list;\r\n\
\r\n\
auto x = new list<int>;\r\n\
x.push_back(4);\r\n\
x.push_back(3);\r\n\
x.push_back(2);\r\n\
x.push_back(1);\r\n\
\r\n\
return (2 in x) && !(10 in x);";
TEST_RESULT("sgl.list test (in)", testSglList8, "1");
