#include "TestBase.h"

const char *testSglHashmap1 =
"import sgl.hashmap;\r\n\
\r\n\
hashmap<char[], int> map = hashmap<char[], int>();\r\n\
\r\n\
// tests\r\n\
map[\"Tom\"] = 1;\r\n\
map[\"Tom\"] = 10;\r\n\
map[\"Mike\"] = 2;\r\n\
map[\"Grey\"] = 3;\r\n\
\r\n\
auto x = map.find(\"Tom\");\r\n\
assert(!!x);\r\n\
assert(*x == 10);\r\n\
\r\n\
auto z1 = map.find(\"Mike\");\r\n\
assert(!!z1);\r\n\
assert(*z1 == 2);\r\n\
map.remove(\"Mike\");\r\n\
auto z2 = map.find(\"Mike\");\r\n\
assert(!z2);\r\n\
\r\n\
map[\"hello\"] = 4;\r\n\
map[\"hello\"] += 4;\r\n\
\r\n\
auto w = map[\"hello\"];\r\n\
assert(w == 8);\r\n\
\r\n\
map.clear();\r\n\
assert(!map.find(\"hello\"));\r\n\
assert(!map.find(\"Tom\"));\r\n\
assert(!map.find(\"Mike\"));\r\n\
assert(!map.find(\"Grey\"));\r\n\
\r\n\
map[\"hello\"] = 10;\r\n\
assert(!!map.find(\"hello\"));\r\n\
assert(map[\"hello\"] == 10);\r\n\
\r\n\
return 1;";
TEST_RESULT("sgl.hashmap test ([] read/write, find, remove, clear)", testSglHashmap1, "1");

const char *testSglHashmap2 =
"import sgl.hashmap;\r\n\
\r\n\
int hash_value(int v) { return v; }\r\n\
hashmap<int, float> hh;\r\n\
\r\n\
hh[short(1)] = 6;\r\n\
return int(hh[short(1)]);";
TEST_RESULT("sgl.hashmap test (key is not an array)", testSglHashmap2, "6");
