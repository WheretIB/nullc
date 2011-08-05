#include "TestBase.h"

const char	*testEuler90 =
"import std.math;\r\n\
int[210][7] arr;\r\n\
int count = 0;\r\n\
void gen()\r\n\
{\r\n\
	int[7] a;\r\n\
	for(a[0] = 0; a[0] < 9; a[0]++)\r\n\
		for(a[1] = a[0]+1; a[1] < 9; a[1]++)\r\n\
			for(a[2] = a[1]+1; a[2] < 9; a[2]++)\r\n\
				for(a[3] = a[2]+1; a[3] < 9; a[3]++)\r\n\
					for(a[4] = a[3]+1; a[4] < 9; a[4]++)\r\n\
						for(a[5] = a[4]+1; a[5] < 10; a[5]++)\r\n\
							arr[count++] = a;\r\n\
}\r\n\
gen();\r\n\
count = 0;\r\n\
void func(int[] numbers, int nn)\r\n\
{\r\n\
	for(int n = nn + 1; n < 210; n++)\r\n\
	{\r\n\
		int[] a = arr[n];\r\n\
		int[200] cubes = 0;\r\n\
		a[6] = 0;\r\n\
		if(a[0] == 6 || a[1] == 6 || a[2] == 6 || a[3] == 6 || a[4] == 6 || a[5] == 6)\r\n\
			a[6] = 9;\r\n\
		if(a[5] == 9)\r\n\
			a[6] = 6;\r\n\
		for(int i = 0; i < (numbers[6] == 0 ? 6 : 7); i++)\r\n\
		{\r\n\
			for(int j = 0; j < (a[6] == 0 ? 6 : 7); j++)\r\n\
			{\r\n\
				cubes[numbers[i] * 10 + a[j]] = 1;\r\n\
				cubes[numbers[i] + a[j] * 10] = 1;\r\n\
			}\r\n\
		}\r\n\
		if(cubes[1] && cubes[4] && cubes[9] && cubes[16] && cubes[25] && cubes[36] && cubes[49] && cubes[64] && cubes[81])\r\n\
			count++;\r\n\
	}\r\n\
}\r\n\
for(int n = 0; n < 10; n++)\r\n\
{\r\n\
	int[] a = arr[n];\r\n\
	a[6] = 0;\r\n\
	if(a[0] == 6 || a[1] == 6 || a[2] == 6 || a[3] == 6 || a[4] == 6 || a[5] == 6)\r\n\
		a[6] = 9;\r\n\
	if(a[5] == 9)\r\n\
		a[6] = 6;\r\n\
	func(a, n);\r\n\
}\r\n\
return count;";
TEST_RESULT("Euler 90 (with decreased N) set range check", testEuler90, "283");

const char	*testEuler122 =
"import std.vector;\r\n\
vector<int> masked;\r\n\
masked.resize(201);\r\n\
masked[0] = masked[1] = 0;\r\n\
 \r\n\
void fill(vector<int> ref curr, int depth)\r\n\
{\r\n\
    if(depth > 5)\r\n\
        return;\r\n\
 \r\n\
 vector<int> added;\r\n\
 \r\n\
    for(int i in curr)\r\n\
    {\r\n\
        for(int j in curr)\r\n\
        {\r\n\
            if(i+j < 201 && (!int(masked[i+j]) || depth <= int(masked[i+j])))\r\n\
            {\r\n\
                masked[i+j] = depth;\r\n\
                added.push_back(i+j);\r\n\
            }\r\n\
        }\r\n\
    }\r\n\
 \r\n\
    for(i in added)\r\n\
    {\r\n\
        curr.push_back(i);\r\n\
        fill(curr, depth + 1);\r\n\
        curr.pop_back();\r\n\
    }\r\n\
}\r\n\
vector<int> row;\r\n\
row.reserve(201);\r\n\
row.push_back(1);\r\n\
 \r\n\
fill(row, 1);\r\n\
int sum = 0;\r\n\
for(int i in masked)\r\n\
    sum += i;\r\n\
 \r\n\
return sum;";
TEST_RESULT("Euler 122 (small depth) vector test", testEuler122, "79");

const char	*testPatternMatching =
"import std.vector;\r\n\
\r\n\
typedef bool ref(auto ref[]) pattern_matcher;\r\n\
typedef auto ref ref(auto ref[]) pattern_callback;\r\n\
\r\n\
class FunctionKV\r\n\
{\r\n\
	pattern_matcher pattern;\r\n\
	pattern_callback function;\r\n\
}\r\n\
FunctionKV FunctionKV(pattern_matcher pattern, pattern_callback function)\r\n\
{\r\n\
	FunctionKV ret;\r\n\
	ret.pattern = pattern;\r\n\
	ret.function = function;\r\n\
	return ret;\r\n\
}\r\n\
class Function\r\n\
{\r\n\
	vector<FunctionKV> patterns;\r\n\
	pattern_callback elseFunction;\r\n\
	int binds;\r\n\
	Function ref bind_function(pattern_matcher pattern, pattern_callback function)\r\n\
	{\r\n\
		if(!binds)\r\n\
		{\r\n\
			binds = 1;\r\n\
		}\r\n\
		patterns.push_back(FunctionKV(pattern, function));\r\n\
		return this;\r\n\
	}\r\n\
	Function ref bind_else(pattern_callback function)\r\n\
	{\r\n\
		elseFunction = function;\r\n\
		return this;\r\n\
	}\r\n\
}\r\n\
\r\n\
auto ref operator()(Function ref f, auto ref[] args)\r\n\
{\r\n\
	for(FunctionKV i in f.patterns)\r\n\
	{\r\n\
		if(i.pattern(args))\r\n\
			return i.function(args);\r\n\
	}\r\n\
	return f.elseFunction(args);\r\n\
}\r\n\
\r\n\
Function fib;\r\n\
fib\r\n\
.bind_function(\r\n\
	auto(auto ref[] args)\r\n\
	{\r\n\
		return args[0].type == int && int(args[0]) == 1;\r\n\
	},\r\n\
	auto ref f(auto ref[] args)\r\n\
	{\r\n\
		return duplicate(1);\r\n\
	}\r\n\
).bind_function(\r\n\
	auto(auto ref[] args)\r\n\
	{\r\n\
		return args[0].type == int && int(args[0]) == 1;\r\n\
	},\r\n\
	auto ref f(auto ref[] args)\r\n\
	{\r\n\
		return duplicate(2);\r\n\
	}\r\n\
).bind_function(\r\n\
	auto(auto ref[] args)\r\n\
	{\r\n\
		return args[0].type == int && int(args[0]) > 0;\r\n\
	},\r\n\
	auto ref f(auto ref[] args)\r\n\
	{\r\n\
		return duplicate(int(fib(int(args[0]) - 1)) + int(fib(int(args[0]) - 2)));\r\n\
	}\r\n\
).bind_else(auto ref f(auto ref[] args){ return duplicate(0); });\r\n\
\r\n\
int d = int(fib(1));\r\n\
int c = int(fib(2));\r\n\
int b = int(fib(10));\r\n\
int a = int(fib(-10));\r\n\
return a*100**0 + b*100**1 + c*100**2 + d*100**3;";
TEST_RESULT("Pattern matching", testPatternMatching, "1015500");

const char	*testFunctional =
"import std.list;\r\n\
\r\n\
// make type aliases to simplify code\r\n\
typedef auto ref[] arg_list;\r\n\
typedef auto ref ref(arg_list) function;\r\n\
\r\n\
// constructor of an empty argument list\r\n\
arg_list arg_list()\r\n\
{\r\n\
	auto ref[] x;\r\n\
	return x;\r\n\
}\r\n\
// since 'function' cannot be called with no arguments, '_' will be a null-argument placeholder\r\n\
auto _ = arg_list();\r\n\
// argument list constructor from any number of arguments\r\n\
arg_list arg_list(arg_list x)\r\n\
{\r\n\
	return x;\r\n\
}\r\n\
// when variables are wrapped into an 'auto ref' pointer list, pointers to local variables are taken, and temporary 'auto ref[N]' array is also created on stack\r\n\
// this function duplicates argument list in a dynamic memory for save use\r\n\
arg_list arg_copy(arg_list x)\r\n\
{\r\n\
	arg_list ret = new auto ref[x.size];\r\n\
	for(l in ret, r in x)\r\n\
		l = duplicate(r);\r\n\
	return ret;\r\n\
}\r\n\
arg_list arg_splice(arg_list x, int start, int end)\r\n\
{\r\n\
	arg_list ret = new auto ref[end - start + 1];\r\n\
	for(int i = start; i <= end; i++)\r\n\
		ret[i-start] = x[i];\r\n\
	return ret;\r\n\
}\r\n\
// function concatenates multiple argument lists together\r\n\
arg_list operator+(arg_list a, b)\r\n\
{\r\n\
	arg_list ret = new auto ref[a.size + b.size];\r\n\
	for(int i = 0; i < a.size; i++)\r\n\
		ret[i] = a[i];\r\n\
	for(int i = 0; i < b.size; i++)\r\n\
		ret[i+a.size] = b[i];\r\n\
	return ret;\r\n\
}\r\n\
// if we have an 'auto ref' type which points to a function, we would've had to convert it to function before calling; this function enables us to call function immediately\r\n\
auto ref operator()(auto ref a, arg_list args)\r\n\
{\r\n\
	return function(a)(args);\r\n\
}\r\n\
list<auto ref> list(auto ref l)\r\n\
{\r\n\
	list<auto ref> ref u = l;\r\n\
	return *u;\r\n\
}\r\n\
auto ref list(arg_list args)\r\n\
{\r\n\
	auto r = new list<auto ref>;\r\n\
	//*r = list();\r\n\
	for(i in args)\r\n\
		r.push_back(i);\r\n\
	return r;\r\n\
}\r\n\
// this function binds first function argument to specified value (parameters after the second are ignored)\r\n\
auto ref bind_first(arg_list args)\r\n\
{\r\n\
	arg_list copy = arg_copy(args); // duplicate argument list internally\r\n\
	auto ref temp(arg_list argsLocal)\r\n\
	{\r\n\
		return copy[0](arg_splice(copy, 1, copy.size - 1) + argsLocal);\r\n\
	}\r\n\
	return duplicate(temp);	// return pointer to new function\r\n\
}\r\n\
// this function binds last function argument to specified value (parameters after the second are ignored)\r\n\
auto ref bind_last(arg_list args)\r\n\
{\r\n\
	arg_list copy = arg_copy(args); // duplicate argument list internally\r\n\
	auto ref temp(arg_list argsLocal)\r\n\
	{\r\n\
		return copy[0](argsLocal + arg_splice(copy, 1, copy.size - 1));\r\n\
	}\r\n\
	return duplicate(temp);\r\n\
}\r\n\
// \r\n\
auto ref map(arg_list args)\r\n\
{\r\n\
	auto r = new list<auto ref>;//();\r\n\
	//*r = list();\r\n\
	for(i in list(args[1]))\r\n\
		r.push_back(args[0](i));	// apply function to every element\r\n\
	return r; // return the new list\r\n\
}\r\n\
// foldl(function, first, list)\r\n\
auto ref foldl(arg_list args)\r\n\
{\r\n\
	auto x = list(args[2]).begin();\r\n\
	auto ref val = args[0](args[1], x.elem);\r\n\
	x = x.next;\r\n\
	while(x)\r\n\
	{\r\n\
		val = args[0](val, x.elem);\r\n\
		x = x.next;\r\n\
	}\r\n\
	return val;\r\n\
}\r\n\
// foldr(function, last, list)\r\n\
auto ref foldr(arg_list args)\r\n\
{\r\n\
	auto x = list(args[2]).end();\r\n\
	auto ref val = args[0](x.elem, args[1]);\r\n\
	x = x.prev;\r\n\
	while(x)\r\n\
	{\r\n\
		val = args[0](x.elem, val);\r\n\
		x = x.prev;\r\n\
	}\r\n\
	return val;\r\n\
}\r\n\
\r\n\
// function adds two numbers and supports partial application\r\n\
auto ref add(arg_list args)\r\n\
{\r\n\
	assert(args.size > 0 && args.size <= 2);\r\n\
	if(args.size == 1)	// if we have only one argument, return this function with first argument bind to it\r\n\
		return bind_first(add, args[0]);\r\n\
	return duplicate(int(args[0]) + int(args[1])); // otherwise, return sum\r\n\
}\r\n\
\r\n\
// use 'add' as the usual function\r\n\
int a = add(3, 4); // 7\r\n\
\r\n\
// bind first argument of 'add' function to 8\r\n\
auto f1 = bind_first(add, 8);\r\n\
int b = f1(9); // 17\r\n\
\r\n\
// bind last argument of bind_first to create a binder that binds first function argument to 5\r\n\
auto bar = bind_last(bind_first, 5);\r\n\
auto f2 = bar(add);\r\n\
int c = f2(20); // 25\r\n\
\r\n\
// bind both function parameters in a sequence\r\n\
auto f3 = bind_last(bind_first(add, 45), 10);\r\n\
int d = f3(_); // 55\r\n\
\r\n\
// partial application example\r\n\
int e = add(5)(3);\r\n\
auto f4 = add(12);\r\n\
int f = f4(4); // 16\r\n\
\r\n\
// bind both function parameters\r\n\
auto f5 = bind_first(add, 100, 1);\r\n\
int g1 = f5(_); // 101\r\n\
auto f6 = bind_last(add, 200, 2);\r\n\
int g2 = f6(_); // 202\r\n\
\r\n\
list<auto ref> l = map(bind_first(add, 8), list(1, 2, 3, 4));\r\n\
int[4] lX;\r\n\
for(i in l, j in lX)\r\n\
	j = int(i); // 9; 10; 11; 12\r\n\
\r\n\
list<auto ref> l2 = map(add, list(1, 2, 3, 4));\r\n\
int[4] l2X;\r\n\
for(i in l2, j in l2X)\r\n\
	j = int(i(4)); // 5; 6; 7; 8\r\n\
\r\n\
int x = foldl(add, 0, list(1, 7, 2, 3, 4, 5));\r\n\
int y = add(add(add(add(add(1, 7), 2), 3), 4), 5);\r\n\
\r\n\
int z = foldl(auto(arg_list args){ return duplicate(int(args[0]) / int(args[1])); }, 20, list(5, 2));\r\n\
\r\n\
int w = foldr(auto(arg_list args){ return duplicate(int(args[0]) / int(args[1])); }, 3, list(25, 15));\r\n\
\r\n\
return a == 7 && b == 17 && c == 25 && d == 55 && e == 8 && f == 16 &&\r\n\
g1 == 101 && g2 == 202 &&\r\n\
lX[0] == 9 && lX[1] == 10 && lX[2] == 11 && lX[3] == 12 &&\r\n\
l2X[0] == 5 && l2X[1] == 6 && l2X[2] == 7 && l2X[3] == 8 &&\r\n\
x == 22 && y == 22 && z == 2 && w == 5;";
TEST_RESULT("Functional primitives", testFunctional, "1");

const char	*testBind =
"class ph1{ } ph1 _1;\r\n\
class ph2{ } ph2 _2;\r\n\
\r\n\
auto bind(generic f, generic v0)\r\n\
{\r\n\
	@if(typeof(v0) == ph1)\r\n\
		return auto(typeof(f).argument[0] x){ return f(x); };\r\n\
	else\r\n\
		return auto(){ return f(v0); };\r\n\
}\r\n\
\r\n\
auto bind(generic f, generic v0, v1)\r\n\
{\r\n\
	@if((typeof(v0) == ph1 || typeof(v0) == ph2) && (typeof(v1) == ph1 || typeof(v1) == ph2))\r\n\
		return auto(typeof(f).argument[0] x, typeof(f).argument[0] y){ return f(typeof(v0) == ph1 ? x : y, typeof(v1) == ph1 ? x : y); };\r\n\
	else if(typeof(v0) == ph1)\r\n\
		return bind(auto(typeof(f).argument[0] x){ return f(x, v1); }, v0);\r\n\
	else if(typeof(v0) == ph2 || typeof(v1) == ph2)\r\n\
		return auto(typeof(f).argument[1] x, typeof(f).argument[0] y){ return f(typeof(v0) == ph2 ? y : v0, typeof(v1) == ph2 ? y : v1); };\r\n\
	else\r\n\
		return bind(auto(typeof(f).argument[0] x){ return f(v0, x); }, v1);\r\n\
}\r\n\
\r\n\
int bar(int x){ return -x; }\r\n\
int ken(int x, y){ return x - y; }\r\n\
\r\n\
auto x1 = bind(bar, 5);\r\n\
auto x2 = bind(bar, _1);\r\n\
\r\n\
auto y1 = bind(ken, 34, 14); // 34 - 14\r\n\
auto y2 = bind(ken, 5, _1); // 5 - x\r\n\
auto y3 = bind(ken, _1, 2); // x - 2\r\n\
auto y4 = bind(ken, _2, _1); // y - x\r\n\
auto y5 = bind(ken, _1, _1); // x - x\r\n\
auto y6 = bind(ken, _2, _2); // y - y\r\n\
auto y7 = bind(ken, _2, 5); // y - 5\r\n\
auto y8 = bind(ken, 10, _2); // 10 - y\r\n\
\r\n\
int r1 = y1(); // 20\r\n\
int r2 = y2(2); // 3\r\n\
int r3 = y3(12); // 10\r\n\
int r4 = y4(12, 3); // -9\r\n\
int r5 = y5(5, 3); // 0\r\n\
int r6 = y6(5, 3); // 0\r\n\
int r7 = y7(1000000, 7); // 2\r\n\
int r8 = y8(1000000, 7); // 3\r\n\
\r\n\
return 1;";
TEST("Function binding", testBind, "1")
{
	CHECK_INT("r1", 0, 20);
	CHECK_INT("r2", 0, 3);
	CHECK_INT("r3", 0, 10);
	CHECK_INT("r4", 0, -9);
	CHECK_INT("r5", 0, 0);
	CHECK_INT("r6", 0, 0);
	CHECK_INT("r7", 0, 2);
	CHECK_INT("r8", 0, 3);
}

const char	*testVectorSplice =
"class vector<T>\r\n\
{\r\n\
	T[]		data;\r\n\
	int		count;\r\n\
}\r\n\
void vector:push_back(generic val)\r\n\
{\r\n\
	if(count == data.size)\r\n\
		this.grow();\r\n\
	data[count++] = val;\r\n\
}\r\n\
void vector:pop_back()\r\n\
{\r\n\
	assert(count);\r\n\
	count--;\r\n\
}\r\n\
auto vector:back()\r\n\
{\r\n\
	assert(count);\r\n\
	return &data[count - 1];\r\n\
}\r\n\
auto vector:front()\r\n\
{\r\n\
	assert(count);\r\n\
	return &data[0];\r\n\
}\r\n\
auto operator[](vector<generic> ref v, int index)\r\n\
{\r\n\
	assert(index < v.count);\r\n\
	return &v.data[index];\r\n\
}\r\n\
void vector:grow()\r\n\
{\r\n\
	int nReserved = data.size + (data.size >> 1) + 1;\r\n\
	T[] nArr = new T[nReserved];\r\n\
	for(i in data, j in nArr)\r\n\
		j = i;\r\n\
	data = nArr;\r\n\
}\r\n\
auto vector:size()\r\n\
{\r\n\
	return count;\r\n\
}\r\n\
\r\n\
class vector_splice<T>\r\n\
{\r\n\
	vector<T> ref base;\r\n\
	int start, end;\r\n\
}\r\n\
auto operator[](vector<@T> ref a, int start, end)\r\n\
{\r\n\
	vector_splice<T> s;\r\n\
	s.base = a;\r\n\
	assert(start >= 0 && end >= 0 && start < a.size() && end >= start);\r\n\
	s.start = start;\r\n\
	s.end = end;\r\n\
	return s;\r\n\
}\r\n\
auto operator[](vector_splice<@T> ref a, int index)\r\n\
{\r\n\
	assert(index >= 0 && index <= (a.end - a.start));\r\n\
	return a.base[index + a.start];\r\n\
}\r\n\
auto arr = new vector<int>;\r\n\
arr.push_back(1); arr.push_back(2); arr.push_back(3); arr.push_back(4);\r\n\
\r\n\
auto sp1 = arr[1, 3];\r\n\
auto sp2 = arr[2, 2];\r\n\
return sp1[2] + sp2[0] + arr[0, 0][0];";
TEST_RESULT("Generic vector splice function", testVectorSplice, "8");

const char	*testAABBTransform =
"import std.math;\r\n\
import std.typeinfo;\r\n\
import std.range;\r\n\
\r\n\
class vector3\r\n\
{\r\n\
	float x, y, z;\r\n\
\r\n\
	void vector3(){}\r\n\
	void vector3(float x, y, z){ this.x = x; this.y = y; this.z = z; }\r\n\
}\r\n\
float min(float lhs, rhs)\r\n\
{\r\n\
	return lhs < rhs ? lhs : rhs;\r\n\
}\r\n\
\r\n\
float max(float lhs, rhs)\r\n\
{\r\n\
	return lhs > rhs ? lhs : rhs;\r\n\
}\r\n\
\r\n\
vector3 operator+(vector3 ref lhs, rhs)\r\n\
{\r\n\
	return vector3(lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z);\r\n\
}\r\n\
\r\n\
vector3 operator-(vector3 ref lhs, rhs)\r\n\
{\r\n\
	return vector3(lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z);\r\n\
}\r\n\
\r\n\
vector3 operator/(vector3 ref lhs, float rhs)\r\n\
{\r\n\
	return vector3(lhs.x / rhs, lhs.y / rhs, lhs.z / rhs);\r\n\
}\r\n\
\r\n\
vector3 minimize(vector3 ref lhs, rhs)\r\n\
{\r\n\
	return vector3(min(lhs.x, rhs.x), min(lhs.y, rhs.y), min(lhs.z, rhs.z));\r\n\
}\r\n\
\r\n\
vector3 maximize(vector3 ref lhs, rhs)\r\n\
{\r\n\
	return vector3(max(lhs.x, rhs.x), max(lhs.y, rhs.y), max(lhs.z, rhs.z));\r\n\
}\r\n\
\r\n\
class aabb\r\n\
{\r\n\
	vector3 min;\r\n\
	vector3 max;\r\n\
\r\n\
	void aabb() {}\r\n\
	void aabb(vector3 ref min, vector3 ref max){ this.min = *min; this.max = *max; }\r\n\
}\r\n\
\r\n\
aabb update_aabb(aabb ref b, vector3 ref p)\r\n\
{\r\n\
	return aabb(minimize(b.min, p), maximize(b.max, p));\r\n\
}\r\n\
\r\n\
class matrix43\r\n\
{\r\n\
	float m00, m01, m02, m03;\r\n\
	float m10, m11, m12, m13;\r\n\
	float m20, m21, m22, m23;\r\n\
}\r\n\
\r\n\
vector3 transform_point(vector3 ref v, matrix43 ref m)\r\n\
{\r\n\
	return vector3( v.x * m.m00 + v.y * m.m01 + v.z * m.m02 + m.m03,\r\n\
		v.x * m.m10 + v.y * m.m11 + v.z * m.m12 + m.m13,\r\n\
		v.x * m.m20 + v.y * m.m21 + v.z * m.m22 + m.m23);\r\n\
}\r\n\
\r\n\
vector3 transform_vector(vector3 ref v, matrix43 ref m)\r\n\
{\r\n\
	return vector3( v.x * m.m00 + v.y * m.m01 + v.z * m.m02,\r\n\
		v.x * m.m10 + v.y * m.m11 + v.z * m.m12,\r\n\
		v.x * m.m20 + v.y * m.m21 + v.z * m.m22);\r\n\
}\r\n\
\r\n\
aabb transform_aabb(aabb ref b, matrix43 ref m)\r\n\
{\r\n\
	auto corners =\r\n\
	{\r\n\
		vector3(b.max.x, b.min.y, b.min.z),\r\n\
		vector3(b.max.x, b.max.y, b.min.z),\r\n\
		vector3(b.min.x, b.max.y, b.min.z),\r\n\
		vector3(b.min.x, b.min.y, b.max.z),\r\n\
		vector3(b.max.x, b.min.y, b.max.z),\r\n\
		vector3(b.max.x, b.max.y, b.max.z),\r\n\
		vector3(b.min.x, b.max.y, b.max.z)\r\n\
	};\r\n\
\r\n\
	vector3 corner = transform_point(b.min, m);\r\n\
	aabb result = aabb(corner, corner);\r\n\
\r\n\
	for (int i = 0; i < 7; ++i)\r\n\
		result = update_aabb(result, transform_point(corners[i], m));\r\n\
\r\n\
	return result;\r\n\
}\r\n\
\r\n\
matrix43 matrix_abs(matrix43 ref m)\r\n\
{\r\n\
	matrix43 result;\r\n\
\r\n\
	result.m00 = abs(m.m00);\r\n\
	result.m01 = abs(m.m01);\r\n\
	result.m02 = abs(m.m02);\r\n\
	result.m03 = abs(m.m03);\r\n\
	result.m10 = abs(m.m10);\r\n\
	result.m11 = abs(m.m11);\r\n\
	result.m12 = abs(m.m12);\r\n\
	result.m13 = abs(m.m13);\r\n\
	result.m20 = abs(m.m20);\r\n\
	result.m21 = abs(m.m21);\r\n\
	result.m22 = abs(m.m22);\r\n\
	result.m23 = abs(m.m23);\r\n\
\r\n\
	return result;\r\n\
}\r\n\
\r\n\
aabb transform_aabb_fast(aabb ref b, matrix43 ref m)\r\n\
{\r\n\
	vector3 center = (b.min + b.max) / 2;\r\n\
	vector3 extent = (b.max - b.min) / 2;\r\n\
\r\n\
	vector3 new_center = transform_point(center, m);\r\n\
	vector3 new_extent = transform_vector(extent, matrix_abs(m));\r\n\
\r\n\
	return aabb(new_center - new_extent, new_center + new_extent);\r\n\
}\r\n\
\r\n\
aabb transform_aabb_ultra_fast(aabb ref aabb_, matrix43 ref matrix)\r\n\
{\r\n\
	auto cx = (aabb_.min.x + aabb_.max.x) * 0.5f;\r\n\
	auto cy = (aabb_.min.y + aabb_.max.y) * 0.5f;\r\n\
	auto cz = (aabb_.min.z + aabb_.max.z) * 0.5f;\r\n\
\r\n\
	auto ex = aabb_.max.x - cx;\r\n\
	auto ey = aabb_.max.y - cy;\r\n\
	auto ez = aabb_.max.z - cz;\r\n\
\r\n\
	auto ncx = cx * matrix.m00 + cy * matrix.m01 + cz * matrix.m02 + matrix.m03;\r\n\
	auto ncy = cx * matrix.m10 + cy * matrix.m11 + cz * matrix.m12 + matrix.m13;\r\n\
	auto ncz = cx * matrix.m20 + cy * matrix.m21 + cz * matrix.m22 + matrix.m23;\r\n\
\r\n\
	auto nex = ex * abs(matrix.m00) + ey * abs(matrix.m01) + ez * abs(matrix.m02);\r\n\
	auto ney = ex * abs(matrix.m10) + ey * abs(matrix.m11) + ez * abs(matrix.m12);\r\n\
	auto nez = ex * abs(matrix.m20) + ey * abs(matrix.m21) + ez * abs(matrix.m22);\r\n\
\r\n\
	return aabb(vector3(ncx - nex, ncy - ney, ncz - nez), vector3(ncx + nex, ncy + ney, ncz + nez));\r\n\
}\r\n\
\r\n\
aabb aabb_ = aabb(vector3(1, 2, 3), vector3(4, 9, 12));\r\n\
matrix43 matrix;\r\n\
for(i in range(1, 12), j in (matrix43).members(matrix))\r\n\
	(float ref(j.value)) = i;\r\n\
\r\n\
float r = 0;\r\n\
for (int i = 0; i < 100; ++i)\r\n\
	r += transform_aabb(aabb_, matrix).min.y;\r\n\
\r\n\
for (int i = 0; i < 100; ++i)\r\n\
	r += transform_aabb_fast(aabb_, matrix).min.y;\r\n\
\r\n\
for (int i = 0; i < 100; ++i)\r\n\
	r += transform_aabb_ultra_fast(aabb_, matrix).min.y;\r\n\
return int(r);";
TEST_RESULT("AABB transformations", testAABBTransform, "13800");

const char	*testReflection1 =
"import std.typeinfo;\r\n\
\r\n\
auto ref x = createInstanceByName(\"int\");\r\n\
\r\n\
typeid t = getType(\"int\");\r\n\
\r\n\
auto ref y = createInstanceByType(t);\r\n\
\r\n\
int ref a = x;\r\n\
*a = 5;\r\n\
int ref b = y;\r\n\
*b = 6;\r\n\
\r\n\
return *a + *b;";
TEST_RESULT("Reflection test 1", testReflection1, "11");

const char	*testReflection2 =
"import std.typeinfo;\r\n\
auto[] arr = createArrayByName(\"int\", 16);\r\n\
int[] arrI = arr;\r\n\
arrI[0] = 4;\r\n\
\r\n\
return arrI[0] + arrI.size;";
TEST_RESULT("Reflection test 2", testReflection2, "20");
