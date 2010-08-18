#include "TestBase.h"

const char	*testCoroutineAsIterator1 =
"coroutine int foo(){ for(int i = 1; i < 3000; i++) yield i; return 0; }\r\n\
int sum = 0;\r\n\
for(i in foo)\r\n\
	sum += i;\r\n\
return sum;";
TEST_RESULT("Coroutine as an iterator test 1", testCoroutineAsIterator1, "4498500");

const char	*testCoroutineAsIterator2 =
"auto _range(int start, end){ return coroutine auto(){ for(int i = start; i <= end; i++) yield i; return 0; }; }\r\n\
int sum = 0;\r\n\
for(i in _range(1, 2999))\r\n\
	sum += i;\r\n\
return sum;";
TEST_RESULT("Coroutine as an iterator test 2", testCoroutineAsIterator2, "4498500");

const char	*testCoroutineAsIterator3 =
"int sum = 0;\r\n\
for(i in coroutine auto(){ for(int i = 1; i < 3000; i++) yield i; return 0; })\r\n\
	sum += i;\r\n\
return sum;";
TEST_RESULT("Coroutine as an iterator test 3", testCoroutineAsIterator3, "4498500");
