#include "TestBase.h"

const char *testSglEventFull =
"import std.event;\r\n\
import std.list;\r\n\
\r\n\
int count = 0;\r\n\
int passed = 0;\r\n\
\r\n\
void test(bool value)\r\n\
{\r\n\
	count++;\r\n\
	passed += value;\r\n\
}\r\n\
\r\n\
{\r\n\
	event<void ref()> e;\r\n\
	\r\n\
	int x;\r\n\
	\r\n\
	void callback(){ x++; }\r\n\
	\r\n\
	e += callback;\r\n\
	e();\r\n\
	e += callback;\r\n\
	e();\r\n\
	e -= callback;\r\n\
	e();\r\n\
	e -= callback;\r\n\
	e();\r\n\
	\r\n\
	test(x == 2);\r\n\
}\r\n\
\r\n\
{\r\n\
	event<void ref()> e;\r\n\
	\r\n\
	int x;\r\n\
\r\n\
	e += auto(){ x++; };\r\n\
	e();\r\n\
	e += auto(){ x++; };\r\n\
	e();\r\n\
	e -= auto(){ x++; };\r\n\
	e();\r\n\
	e -= auto(){ x++; };\r\n\
	e();\r\n\
		\r\n\
	test(x == 7); // lambda functions are unique\r\n\
}\r\n\
\r\n\
{\r\n\
	event<void ref()> e;\r\n\
	\r\n\
	int x;\r\n\
	\r\n\
	void callback(){ x++; }\r\n\
	\r\n\
	e.attach(callback);\r\n\
	e();\r\n\
	e.attach(callback);\r\n\
	e();\r\n\
	e.detach(callback);\r\n\
	e();\r\n\
	e.detach(callback);\r\n\
	e();\r\n\
	\r\n\
	test(x == 2);\r\n\
}\r\n\
	\r\n\
{\r\n\
	event<void ref()> e;\r\n\
	\r\n\
	int x;\r\n\
	\r\n\
	e.attach(auto(){ x++; });\r\n\
	e();\r\n\
	e.attach(auto(){ x++; });\r\n\
	e();\r\n\
	e.detach(auto(){ x++; });\r\n\
	e();\r\n\
	e.detach(auto(){ x++; });\r\n\
	e();\r\n\
	\r\n\
	e.detach_all();\r\n\
	e();\r\n\
	\r\n\
	test(x == 7); // lambda functions are unique\r\n\
}\r\n\
\r\n\
{\r\n\
	event<void ref(int)> e;\r\n\
	\r\n\
	int x;\r\n\
	\r\n\
	e.attach(<y>{ x += y; });\r\n\
	\r\n\
	e(2);\r\n\
	e(8);\r\n\
	\r\n\
	e.detach_all();\r\n\
	e(4);\r\n\
	\r\n\
	test(x == 10);\r\n\
}\r\n\
\r\n\
{\r\n\
	event<void ref(int)> e;\r\n\
	\r\n\
	class Test\r\n\
	{\r\n\
		int y;\r\n\
		\r\n\
		void add(int x){ y += x; }\r\n\
	}\r\n\
	\r\n\
	Test x;\r\n\
	\r\n\
	test(!bool(e));\r\n\
		\r\n\
	e += x.add;\r\n\
	\r\n\
	test(bool(e));\r\n\
	\r\n\
	e(4);\r\n\
	e(8);\r\n\
\r\n\
	test(x.y == 12);\r\n\
}\r\n\
\r\n\
{\r\n\
	class Object\r\n\
	{\r\n\
		int y;\r\n\
		int z;\r\n\
		\r\n\
		event<EventHandler> onAdded;\r\n\
		\r\n\
		void ref() f;\r\n\
		\r\n\
		void add(int x)\r\n\
		{\r\n\
			y += x;\r\n\
			if(onAdded)\r\n\
				onAdded(this, y);\r\n\
		}\r\n\
	}\r\n\
	\r\n\
	class Listener\r\n\
	{\r\n\
		Object ref x;\r\n\
		\r\n\
		int lastY;\r\n\
		\r\n\
		void Listener(Object ref obj)\r\n\
		{\r\n\
			x = obj;\r\n\
			\r\n\
			x.onAdded.attach(<obj, args>{\r\n\
				test(typeid(obj) == Object);\r\n\
				test(Object ref(obj) == x);\r\n\
				test(int(args[0]) == x.y);\r\n\
				lastY = int(args[0]);\r\n\
			});\r\n\
		}\r\n\
	}\r\n\
	\r\n\
	auto object = new Object();\r\n\
	auto listener = new Listener(object);\r\n\
	\r\n\
	object.add(4);\r\n\
	object.add(5);\r\n\
	\r\n\
	test(object.y == 9);\r\n\
	test(listener.lastY == 9);\r\n\
}\r\n\
return passed / count;";
TEST_RESULT("std.event test (everything)", testSglEventFull, "1");
