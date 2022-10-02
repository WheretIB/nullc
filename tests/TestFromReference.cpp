#include "TestBase.h"

const char	*testReference1 =
"class list_node{ list_node ref next; int value; }\r\n\
class list_iterator{    list_node ref curr; }\r\n\
auto list_node::start()\r\n\
{\r\n\
    list_iterator ret;\r\n\
    ret.curr = this;\r\n\
    return ret;\r\n\
}\r\n\
auto list_iterator::hasnext(){    return curr ? 1 : 0;}\r\n\
auto list_iterator::next()\r\n\
{\r\n\
    int ret = curr.value;\r\n\
    curr = curr.next;\r\n\
    return ret;\r\n\
}\r\n\
list_node list;\r\n\
list.value = 2;\r\n\
list.next = new list_node;\r\n\
list.next.value = 5;\r\n\
int product = 1;\r\n\
for(i in list)\r\n\
    product *= i;\r\n\
return product;";
TEST_RESULT("example of an iterator over elements of a single-linked list", testReference1, "10");

const char	*testReference2 =
"class NumberPair\r\n\
{\r\n\
    int a, b;\r\n\
    auto sum{ get{ return a + b; } };\r\n\
}\r\n\
NumberPair foo;\r\n\
foo.a = 5;\r\n\
foo.b = 10;\r\n\
return foo.sum;";
TEST_RESULT("example of a read-only accessor", testReference2, "15");

const char	*testReference3 =
"class vec2\r\n\
{\r\n\
	float x, y;\r\n\
	auto xy{get{return this;}set{x = r.x;y = r.y;}};\r\n\
	auto yx{get{vec2 tmp;tmp.x = y;tmp.y = x;return tmp;}set(value){y = value.x;x = value.y;}};\r\n\
}\r\n\
vec2 foo;foo.x = 1.5;foo.y = 2.5;\r\n\
vec2 bar;\r\n\
bar.yx = foo.xy;\r\n\
return int(100 * foo.x * bar.y);";
TEST_RESULT("example of a read-write accessor", testReference3, "225");

const char	*testReference4 =
"int sum(auto ref[] args)\r\n\
{\r\n\
    int result = 0;\r\n\
    for(i in args)\r\n\
        result += int(i);\r\n\
    return result;\r\n\
}\r\n\
return sum(1, 12, 201);";
TEST_RESULT("example of sum of integers using function with variable arguments", testReference4, "214");

const char	*testReference5 =
"class StdIO{} class Std{ StdIO out; } Std io; void operator<<(StdIO i, int x){} void operator<<(StdIO i, double x){} void operator<<(StdIO i, char[] x){}\r\n\
int println(auto ref[] args)\r\n\
{\r\n\
    int printedArgs = args.size;\r\n\
    for(i in args)\r\n\
    {\r\n\
        switch(typeid(i))\r\n\
        {\r\n\
        case int:\r\n\
            io.out << int(i);\r\n\
            break;\r\n\
        case double:\r\n\
            io.out << double(i);\r\n\
            break;\r\n\
        case char[]:\r\n\
            io.out << char[](i);\r\n\
            break;\r\n\
        default:\r\n\
            printedArgs--;\r\n\
        }\r\n\
    }\r\n\
    return printedArgs;\r\n\
}\r\n\
return println(2, \" \", 4, \" hello \", 5.0, 3.0f);";
TEST_RESULT("example of println function", testReference5, "5");

const char	*testReference6 =
"coroutine int generate_id()\r\n\
{\r\n\
    int ID = 0x23efdd67; // starting ID\r\n\
    // 'infinite' loop will return IDs\r\n\
    while(1)\r\n\
    {\r\n\
        yield ID; // return ID\r\n\
        ID++; // move to the next ID\r\n\
    }\r\n\
}\r\n\
int a = generate_id(); // 0x23efdd67\r\n\
int b = generate_id(); // 0x23efdd68\r\n\
return a+b;";
TEST_RESULT("example of an ID generator", testReference6, "1205844687");

const char	*testReference7 =
"coroutine int rand()\r\n\
{\r\n\
    int current = 1;\r\n\
    while(1)\r\n\
    {\r\n\
        current = current * 1103515245 + 12345;\r\n\
        yield (current >> 16) & 32767;\r\n\
    }\r\n\
}\r\n\
int[8] array;\r\n\
for(i in array)\r\n\
    i = rand();\r\n\
int sum = 0;\r\n\
for(i in array)\r\n\
    sum += i;\r\n\
return sum;";
TEST_RESULT("example of a random number generator", testReference7, "117331");

const char	*testReference8 =
"import std.vector;\r\n\
// Function will return forward iterator over vectors' values\r\n\
auto forward_iterator(vector<int> ref x)\r\n\
{\r\n\
    // Every time the coroutine is called, it will return vector element and advance to the next\r\n\
    coroutine auto iterate()\r\n\
    {\r\n\
        // Loop over all elements\r\n\
        for(int i = 0; i < x.size(); i++)\r\n\
            yield &x[i]; // and return them one after the other\r\n\
        // return statement can still be used in a coroutine.\r\n\
        return nullptr; // return null pointer to mark the end\r\n\
    }\r\n\
    // return iterator function\r\n\
    return iterate;\r\n\
}\r\n\
// create vector and add some numbers to it\r\n\
vector<int> a = vector<int>();\r\n\
a.push_back(4);\r\n\
a.push_back(5);\r\n\
a.push_back(40);\r\n\
\r\n\
// Create iterator\r\n\
auto it = forward_iterator(a);\r\n\
\r\n\
// Find sum of all vector elements\r\n\
int sum = 0;\r\n\
auto ref x; // variable to hold the pointer to current element\r\n\
while(x = it()) // iterate through all elements\r\n\
	sum += int(x); // and add them together\r\n\
return sum; // 49";
TEST_RESULT("example of a forward iterator over vector contents", testReference8, "49");

const char	*testReference9 =
"auto get_rng(int seed)\r\n\
{\r\n\
    // coroutine, that returns pseudo-random numbers in a range (0..32767)\r\n\
    coroutine int rand()\r\n\
    {\r\n\
        // starting seed\r\n\
        int current = seed;\r\n\
        // 'infinite' loop will return pseudo-random numbers\r\n\
        while(1)\r\n\
        {\r\n\
            current = current * 1103515245 + 12345;\r\n\
            yield (current >> 16) & 32767;\r\n\
        }\r\n\
    }\r\n\
    return rand;\r\n\
}\r\n\
// Create two pseudo-random number generators\r\n\
auto rngA = get_rng(1);\r\n\
auto rngB = get_rng(0xdeaf);\r\n\
\r\n\
// Generate an array of eight pseudo-random numbers \r\n\
int[8] array;\r\n\
for(int i = 0; i < array.size / 2; i++) // one half - using first RNG\r\n\
    array[i] = rngA();\r\n\
for(int i = array.size / 2; i < array.size; i++) // second half - using second RNG\r\n\
    array[i] = rngB();\r\n\
return 1;";
TEST("example of parallel random number generators", testReference9, "1")
{
	static const int arr[8] = { 16838, 5758, 10113, 17515, 28306, 25322, 1693, 12038 };
	for(unsigned i = 0; i < 8; i++)
		CHECK_INT("array", i, arr[i], lastFailed);
}
