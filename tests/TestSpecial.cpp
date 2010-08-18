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
vector masked = vector(int);\r\n\
masked.resize(201);\r\n\
masked[0] = masked[1] = 0;\r\n\
 \r\n\
void fill(vector ref curr, int depth)\r\n\
{\r\n\
    if(depth > 5)\r\n\
        return;\r\n\
 \r\n\
    vector added = vector(int);\r\n\
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
vector row = vector(int);\r\n\
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
typedef int ref(auto ref[]) pattern_matcher;\r\n\
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
	vector patterns;\r\n\
	pattern_callback elseFunction;\r\n\
	int binds;\r\n\
	Function ref bind_function(pattern_matcher pattern, pattern_callback function)\r\n\
	{\r\n\
		if(!binds)\r\n\
		{\r\n\
			patterns = vector(FunctionKV);\r\n\
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
list list(auto ref l)\r\n\
{\r\n\
	list ref u = l;\r\n\
	return *u;\r\n\
}\r\n\
auto ref list(arg_list args)\r\n\
{\r\n\
	auto r = new list;\r\n\
	*r = list();\r\n\
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
	auto r = new list;//();\r\n\
	*r = list();\r\n\
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
list l = map(bind_first(add, 8), list(1, 2, 3, 4));\r\n\
int[4] lX;\r\n\
for(i in l, j in lX)\r\n\
	j = int(i); // 9; 10; 11; 12\r\n\
\r\n\
list l2 = map(add, list(1, 2, 3, 4));\r\n\
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
