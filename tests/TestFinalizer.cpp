#include "TestBase.h"

const char	*testFinalizerSimple =
"import std.gc;\r\n\
\r\n\
int z = 0;\r\n\
\r\n\
class Foo{ int a; }\r\n\
void Foo::finalize(){ z = a; }\r\n\
\r\n\
auto x = new Foo;\r\n\
(auto(){x.a = 10;\r\n\
x = nullptr;})();\r\n\
\r\n\
GC.CollectMemory();\r\n\
\r\n\
return z;";
TEST_RESULT_SIMPLE("Class finalize test 1", testFinalizerSimple, "10");

const char	*testFinalizerSelfreference =
"import std.gc;\r\n\
\r\n\
class Foo{ int a; }\r\n\
Foo ref m = nullptr;\r\n\
void Foo::finalize(){ m = this; }\r\n\
\r\n\
auto x = new Foo;\r\n\
(auto(){x.a = 10;\r\n\
x = nullptr;})();\r\n\
\r\n\
GC.CollectMemory();\r\n\
auto f = new Foo;\r\n\
f.a = 0;\r\n\
return m.a;";
TEST_RESULT_SIMPLE("Class finalize test 2", testFinalizerSelfreference, "10");

const char	*testFinalizerFullcollect =
"import std.gc;\r\n\
int start = GC.UsedMemory();\r\n\
class Foo\r\n\
{\r\n\
	int a;\r\n\
}\r\n\
Foo ref m;\r\n\
void Foo::finalize()\r\n\
{\r\n\
	m = this;\r\n\
}\r\n\
\r\n\
auto x = new Foo;\r\n\
(auto(){x.a = 10;\r\n\
x = nullptr;})();\r\n\
GC.CollectMemory();\r\n\
auto f = new int;\r\n\
int z = m.a;\r\n\
f = nullptr;\r\n\
m = nullptr;\r\n\
GC.CollectMemory();\r\n\
return GC.UsedMemory() - start;";
TEST_SIMPLE("Finalize should not prevent memory collection", testFinalizerFullcollect, "0")
{
	CHECK_INT("z", 0, 10, lastFailed);
}

const char	*testAfterExecution =
"int z = 0;\r\n\
class Foo{ int a; }\r\n\
void Foo::finalize(){ z = a; }\r\n\
auto x = new Foo;\r\n\
x.a = 10;\r\n\
x = nullptr;\r\n\
return z;";
TEST_SIMPLE("Finalize after program execution", testAfterExecution, "0")
{
	CHECK_INT("z", 0, 10, lastFailed);
}

const char	*testFinalizerOutOfPool =
"import std.gc;\r\n\
int z = 0;\r\n\
class Foo{ int a; int[4096] h; }\r\n\
void Foo::finalize(){ z = a; }\r\n\
auto x = new Foo;\r\n\
(auto(){x.a = 10;\r\n\
x = nullptr;})();\r\n\
GC.CollectMemory();\r\n\
return z;";
TEST_RESULT_SIMPLE("Class finalize test with big object", testFinalizerOutOfPool, "10");

const char	*testFinalizeArray =
"import std.gc;\r\n\
int z = 0;\r\n\
class Foo{ int a; }\r\n\
void Foo::Foo(int x){ a = x; }\r\n\
void Foo::finalize(){ z += a; }\r\n\
Foo[] x;\r\n\
(auto(){x = new Foo[4];\r\n\
x[0].a = 10;\r\n\
x[1].a = 8;\r\n\
x[2].a = 800;\r\n\
x[3].a = 2000;})();\r\n\
x = nullptr;\r\n\
GC.CollectMemory();\r\n\
return z;";
TEST_SIMPLE("Finalize for an array of objects", testFinalizeArray, "2818")
{
	CHECK_INT("z", 0, 2818, lastFailed);
}

const char	*testFinalizerRessurection1 =
"import std.gc;\r\n\
\r\n\
int ref global;\r\n\
\r\n\
class Scary\r\n\
{\r\n\
	int ref value = new int(4);\r\n\
}\r\n\
\r\n\
void Scary::finalize()\r\n\
{\r\n\
	global = value;\r\n\
}\r\n\
\r\n\
void test()\r\n\
{\r\n\
	new Scary();\r\n\
}\r\n\
\r\n\
test();\r\n\
\r\n\
GC.CollectMemory();\r\n\
\r\n\
int ref x = new int(7);\r\n\
\r\n\
return *global;";
TEST_RESULT_SIMPLE("Finalizer object ressurection test 1 (small object)", testFinalizerRessurection1, "4");

const char	*testFinalizerRessurection2 =
"import std.gc;\r\n\
\r\n\
int ref global;\r\n\
\r\n\
class Scary\r\n\
{\r\n\
	int ref value = new int(4);\r\n\
	int[2048] data;\r\n\
}\r\n\
\r\n\
void Scary::finalize()\r\n\
{\r\n\
	global = value;\r\n\
}\r\n\
\r\n\
void test()\r\n\
{\r\n\
	new Scary();\r\n\
}\r\n\
\r\n\
test();\r\n\
\r\n\
GC.CollectMemory();\r\n\
\r\n\
int ref x = new int(7);\r\n\
\r\n\
return *global;";
TEST_RESULT_SIMPLE("Finalizer object ressurection test 2 (large object)", testFinalizerRessurection2, "4");

const char	*testFinalizerRessurection3 =
"import std.gc;\r\n\
\r\n\
int ref global;\r\n\
int next = 4;\r\n\
\r\n\
class Scary\r\n\
{\r\n\
	int ref value = new int(next++);\r\n\
}\r\n\
\r\n\
void Scary::finalize()\r\n\
{\r\n\
	if(!global || *global < *value)\r\n\
		global = value;\r\n\
}\r\n\
\r\n\
void test()\r\n\
{\r\n\
	new Scary[10];\r\n\
}\r\n\
\r\n\
test();\r\n\
\r\n\
GC.CollectMemory();\r\n\
\r\n\
for(int i = 0; i < 10; i++) new int(7);\r\n\
\r\n\
return *global;";
TEST_RESULT_SIMPLE("Finalizer object ressurection test 3 (small array)", testFinalizerRessurection3, "13");

const char	*testFinalizerRessurection4 =
"import std.gc;\r\n\
\r\n\
int ref global;\r\n\
int next = 4;\r\n\
\r\n\
class Scary\r\n\
{\r\n\
	int ref value = new int(next++);\r\n\
}\r\n\
\r\n\
void Scary::finalize()\r\n\
{\r\n\
	if(!global || *global < *value)\r\n\
		global = value;\r\n\
}\r\n\
\r\n\
void test()\r\n\
{\r\n\
	new Scary[10];\r\n\
}\r\n\
\r\n\
test();\r\n\
\r\n\
GC.CollectMemory();\r\n\
\r\n\
for(int i = 0; i < 10; i++) new int(7);\r\n\
\r\n\
return *global;";
TEST_RESULT_SIMPLE("Finalizer object ressurection test 4 (large array)", testFinalizerRessurection4, "13");
