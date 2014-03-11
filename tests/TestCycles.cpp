#include "TestBase.h"

const char	*testSwitch = 
"// Switch test!\r\n\
int u = 12;\r\n\
int a = 3, b = 0;\r\n\
{\r\n\
  switch(a)\r\n\
  {\r\n\
    case 1:\r\n\
  	  b = 5;\r\n\
	  break;\r\n\
    case 3:\r\n\
	  b = 7;\r\n\
	  break;\r\n\
	case 5:\r\n\
	  b = 18;\r\n\
  }\r\n\
}\r\n\
return u;";
TEST("Switch test", testSwitch, "12")
{
	CHECK_INT("u", 0, 12);
	CHECK_INT("a", 0, 3);
	CHECK_INT("b", 0, 7);
}

const char	*testSwitch2 = 
"double a = 5.0;\r\n\
int i = 0;\r\n\
switch(a)\r\n\
{\r\n\
case 1.0:\r\n\
	i = 1;\r\n\
	break;\r\n\
case 5.0:\r\n\
	i = 2;\r\n\
	break;\r\n\
case 7.0:\r\n\
	i = 3;\r\n\
	break;\r\n\
default:\r\n\
	i = 4;\r\n\
}\r\n\
return i;";
TEST_RESULT("Switch test (double)", testSwitch2, "2")

const char	*testSwitch3 = 
"int a = 1;\r\n\
int b = 0;\r\n\
\r\n\
switch(a)\r\n\
{\r\n\
case 1.0f:\r\n\
	b = 2;\r\n\
	break;\r\n\
case 4l:\r\n\
	b = 5;\r\n\
	break;\r\n\
}\r\n\
\r\n\
return b;";
TEST_RESULT("Switch test (int switch, double case)", testSwitch3, "2")

const char	*testSwitch4 = 
"int a = 4;\r\n\
int b = 0;\r\n\
\r\n\
switch(a)\r\n\
{\r\n\
case 1.0f:\r\n\
	b = 2;\r\n\
	break;\r\n\
case 4l:\r\n\
	b = 5;\r\n\
	break;\r\n\
}\r\n\
\r\n\
return b;";
TEST_RESULT("Switch test (int switch, long case)", testSwitch4, "5")

const char	*testSwitch5 = 
"float a = 4.0f;\r\n\
int b = 0;\r\n\
\r\n\
switch(a)\r\n\
{\r\n\
case 1.0f:\r\n\
	b = 2;\r\n\
	break;\r\n\
case 4l:\r\n\
	b = 5;\r\n\
	break;\r\n\
}\r\n\
\r\n\
return b;";
TEST_RESULT("Switch test (float switch, long case)", testSwitch5, "5")

const char	*testDepthBreakContinue = 
"int i, k = 0;\r\n\
for(i = 0; i < 4; i++)\r\n\
{\r\n\
	for(int j = 0;j < 4;j++)\r\n\
	{\r\n\
		if(j == 2 && i == 2)\r\n\
			break 2;\r\n\
		k++;\r\n\
	}\r\n\
}\r\n\
int a = k;\r\n\
k = 0;\r\n\
for(i = 0; i < 4;i ++)\r\n\
{\r\n\
	for(int j = 0; j < 4; j++)\r\n\
	{\r\n\
		if(j == 2 && i == 2)\r\n\
			continue 2;\r\n\
		k++;\r\n\
	}\r\n\
}\r\n\
int b = k;\r\n\
return a + b;";
TEST("Multi-depth break and continue", testDepthBreakContinue, "24")
{
	CHECK_INT("a", 0, 10);
	CHECK_INT("b", 0, 14);
}

const char	*testBreakContinueTests =
"int hadThis = 1;\r\n\
\r\n\
for(int k = 0; k < 10; k++)\r\n\
{\r\n\
	if(hadThis)\r\n\
		continue;\r\n\
\r\n\
	for(int z = 1; z < 4; z++){}\r\n\
	break;\r\n\
}\r\n\
for(int k2 = 0; 1; k2++)\r\n\
{\r\n\
	if(hadThis)\r\n\
		break;\r\n\
\r\n\
	for(int z = 1; z < 4; z++){}\r\n\
}\r\n\
return 0;";
TEST("More break and continue tests", testBreakContinueTests, "0")
{
	CHECK_INT("k", 0, 10);
	CHECK_INT("k2", 0, 0);
}

const char	*testDoWhileScope =
"int i = 0;\r\n\
do\r\n\
{\r\n\
	bool wrong = ++i < 10;\r\n\
}while(wrong);\r\n\
return i;";
TEST_RESULT("do...while cycle variable scope test", testDoWhileScope, "10")

const char	*testDoWhileScope2 =
"int i;\r\n\
do\r\n\
{\r\n\
	int i;\r\n\
}while(0);\r\n\
return 1;";
TEST_RESULT("do...while cycle variable scope test 2", testDoWhileScope2, "1")
