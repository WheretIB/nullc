#include "TestBase.h"

const char *testSglStringFull =
"import std.string;\r\n\
\r\n\
int count = 0;\r\n\
int passed = 0;\r\n\
\r\n\
void test(bool value)\r\n\
{\r\n\
	count++;\r\n\
	passed += value;\r\n\
}\r\n\
void antitest(bool value)\r\n\
{\r\n\
	count++;\r\n\
	passed += !value;\r\n\
}\r\n\
\r\n\
test(string() == string(""));\r\n\
test(string() == string(0, 0));\r\n\
\r\n\
{\r\n\
	string s = \"\";\r\n\
	test(s == string());\r\n\
	test(s == string(""));\r\n\
}\r\n\
\r\n\
{\r\n\
	string s;\r\n\
	test(s == string());\r\n\
	test(s == string(\"\"));\r\n\
}\r\n\
\r\n\
test(string() == \"\");\r\n\
test(string(\"abc\") == \"abc\");\r\n\
test(string(15, 'a') == \"aaaaaaaaaaaaaaa\");\r\n\
\r\n\
\r\n\
test(string().size == 0);\r\n\
test(string(\"\").size == 0);\r\n\
test(string(\"abc\").size == 3);\r\n\
test(string(15, 'a').size == 15);\r\n\
\r\n\
\r\n\
test(string().length() == 0);\r\n\
test(string(\"\").length() == 0);\r\n\
test(string(\"abc\").length() == 3);\r\n\
test(string(15, 'a').length() == 15);\r\n\
\r\n\
\r\n\
test(string().empty());\r\n\
test(string(\"\").empty());\r\n\
test(!string(\"abc\").empty());\r\n\
\r\n\
\r\n\
test(string(\"abc\").insert(0, \"11\") == \"11abc\");\r\n\
test(string(\"abc\").insert(1, \"11\") == \"a11bc\");\r\n\
test(string(\"abc\").insert(2, \"11\") == \"ab11c\");\r\n\
test(string(\"abc\").insert(3, \"11\") == \"abc11\");\r\n\
\r\n\
test(string(\"abc\").insert(0, \"\") == \"abc\");\r\n\
test(string(\"abc\").insert(1, \"\") == \"abc\");\r\n\
test(string(\"abc\").insert(3, \"\") == \"abc\");\r\n\
\r\n\
test(string(\"\").insert(0, \"\") == \"\");\r\n\
test(string(\"\").insert(0, \"abc\") == \"abc\");\r\n\
\r\n\
\r\n\
test(string().erase(0) == \"\");\r\n\
test(string(\"\").erase(0) == \"\");\r\n\
test(string(\"abc\").erase(0) == \"\");\r\n\
test(string(\"abc\").erase(2) == \"ab\");\r\n\
test(string(\"abc\").erase(1, 1) == \"ac\");\r\n\
test(string(\"abc\").erase(0, 3) == \"\");\r\n\
test(string(\"abc\").erase(0, 35) == \"\");\r\n\
test(string(\"abc\").erase(1, 35) == \"a\");\r\n\
\r\n\
\r\n\
test(string(\"\").replace(0, 0, \"abc\") == \"abc\");\r\n\
test(string(\"\").replace(0, 2, \"abc\") == \"abc\");\r\n\
test(string(\"abc\").replace(0, 1, \"abc\") == \"abcbc\");\r\n\
test(string(\"abc\").replace(0, 3, \"111\") == \"111\");\r\n\
test(string(\"abc\").replace(2, 3, \"111\") == \"ab111\");\r\n\
test(string(\"abc\").replace(2, -1, \"111\") == \"ab111\");\r\n\
test(string(\"abcabc\").replace(1, 4, \"11\") == \"a11c\");\r\n\
\r\n\
{\r\n\
	string str = \"The sixth sick sheik's sixth sheep's sick.\";\r\n\
	string key = \"sixth\";\r\n\
	\r\n\
	test(str.replace(str.rfind(key), key.length(), \"seventh\") == \"The sixth sick sheik's seventh sheep's sick.\");\r\n\
}\r\n\
\r\n\
\r\n\
test((auto(){ auto x = string(\"abc\"); x.swap(\"def\"); return x; })() == \"def\");\r\n\
\r\n\
\r\n\
test(string().find(0) == -1);\r\n\
test(string(\"\").find(0) == -1);\r\n\
test(string(\"abc\").find(0) == -1);\r\n\
\r\n\
test(string().find(\"\") == 0);\r\n\
test(string(\"\").find(\"\") == 0);\r\n\
test(string(\"abc\").find(\"\") == 0);\r\n\
\r\n\
test(string(\"abcab\").find('a') == 0);\r\n\
test(string(\"abcab\").find('a', 1) == 3);\r\n\
test(string(\"abcab\").find('a', 10) == -1);\r\n\
test(string(\"abcab\").find('b') == 1);\r\n\
\r\n\
test(string(\"abcab\").find(\"ab\") == 0);\r\n\
test(string(\"abcab\").find(\"ab\", 1) == 3);\r\n\
test(string(\"abcab\").find(\"bc\") == 1);\r\n\
test(string(\"abcab\").find(\"bc\", 2) == -1);\r\n\
\r\n\
\r\n\
test(string().rfind(0) == -1);\r\n\
test(string(\"\").rfind(0) == -1);\r\n\
test(string(\"abc\").rfind(3) == -1);\r\n\
\r\n\
test(string().rfind(\"\") == 0);\r\n\
test(string(\"\").rfind(\"\") == 0);\r\n\
test(string(\"\").rfind(\"\", 5) == 0);\r\n\
test(string(\"abc\").rfind(\"\") == 3);\r\n\
test(string(\"abc\").rfind(\"\", 5) == 3);\r\n\
\r\n\
test(string(\"abcab\").rfind('a') == 3);\r\n\
test(string(\"abcab\").rfind('a', 1) == 0);\r\n\
test(string(\"abcab\").rfind('a', 0) == 0);\r\n\
test(string(\"abcab\").rfind('a', 10) == 3);\r\n\
test(string(\"abcab\").rfind('b') == 4);\r\n\
test(string(\"abcab\").rfind('c') == 2);\r\n\
\r\n\
test(string(\"abcab\").rfind(\"ab\") == 3);\r\n\
test(string(\"abcab\").rfind(\"ab\", 1) == 0);\r\n\
test(string(\"abcab\").rfind(\"bc\") == 1);\r\n\
test(string(\"abcab\").rfind(\"bc\", 2) == 1);\r\n\
test(string(\"abcabcab\").rfind(\"ab\") == 6);\r\n\
test(string(\"abcabcab\").rfind(\"ab\", 1) == 0);\r\n\
test(string(\"abcabcab\").rfind(\"ab\", 2) == 0);\r\n\
test(string(\"abcabcab\").rfind(\"ab\", 3) == 3);\r\n\
test(string(\"abcabcab\").rfind(\"ab\", 4) == 3);\r\n\
test(string(\"abcabcab\").rfind(\"ab\", 5) == 3);\r\n\
test(string(\"abcabcab\").rfind(\"ab\", 6) == 6);\r\n\
test(string(\"abcabcab\").rfind(\"ab\", 7) == 6);\r\n\
test(string(\"abcabcab\").rfind(\"ab\", 8) == 6);\r\n\
\r\n\
\r\n\
test(string().find_first_of(\"\") == -1);\r\n\
test(string(\"\").find_first_of(\"\") == -1);\r\n\
test(string(\"abc\").find_first_of(\"\") == -1);\r\n\
\r\n\
test(string(\"abcab\").find_first_of(\"a\") == 0);\r\n\
test(string(\"abcab\").find_first_of(\"a\", 1) == 3);\r\n\
test(string(\"abcab\").find_first_of(\"a\", 10) == -1);\r\n\
test(string(\"abcab\").find_first_of(\"b\") == 1);\r\n\
\r\n\
test(string(\"abcab\").find_first_of('a') == 0);\r\n\
test(string(\"abcab\").find_first_of('a', 1) == 3);\r\n\
test(string(\"abcab\").find_first_of('a', 10) == -1);\r\n\
test(string(\"abcab\").find_first_of('b') == 1);\r\n\
\r\n\
test(string(\"abcab\").find_first_of(\"cb\") == 1);\r\n\
test(string(\"abcab\").find_first_of(\"cb\", 1) == 1);\r\n\
test(string(\"abcab\").find_first_of(\"cb\", 3) == 4);\r\n\
test(string(\"abcab\").find_first_of(\"cb\", 10) == -1);\r\n\
test(string(\"abcab\").find_first_of(\"ab\") == 0);\r\n\
test(string(\"abcab\").find_first_of(\"ba\") == 0);\r\n\
test(string(\"abcab\").find_first_of(\"def\") == -1);\r\n\
\r\n\
\r\n\
test(string().find_last_of(\"\") == -1);\r\n\
test(string(\"\").find_last_of(\"\") == -1);\r\n\
test(string(\"\").find_last_of(\"\", 5) == -1);\r\n\
test(string(\"abc\").find_last_of(\"\") == -1);\r\n\
test(string(\"abc\").find_last_of(\"\", 5) == -1);\r\n\
\r\n\
test(string(\"abcab\").find_last_of(\"a\") == 3);\r\n\
test(string(\"abcab\").find_last_of(\"a\", 1) == 0);\r\n\
test(string(\"abcab\").find_last_of(\"a\", 0) == 0);\r\n\
test(string(\"abcab\").find_last_of(\"a\", 10) == 3);\r\n\
test(string(\"abcab\").find_last_of(\"b\") == 4);\r\n\
test(string(\"abcab\").find_last_of(\"c\") == 2);\r\n\
\r\n\
test(string(\"abcab\").find_last_of('a') == 3);\r\n\
test(string(\"abcab\").find_last_of('a', 1) == 0);\r\n\
test(string(\"abcab\").find_last_of('a', 0) == 0);\r\n\
test(string(\"abcab\").find_last_of('a', 10) == 3);\r\n\
test(string(\"abcab\").find_last_of('b') == 4);\r\n\
test(string(\"abcab\").find_last_of('c') == 2);\r\n\
\r\n\
test(string(\"abcab\").find_last_of(\"cb\") == 4);\r\n\
test(string(\"abcab\").find_last_of(\"cb\", 1) == 1);\r\n\
test(string(\"abcab\").find_last_of(\"cb\", 3) == 2);\r\n\
test(string(\"abcab\").find_last_of(\"cb\", 10) == 4);\r\n\
test(string(\"abcab\").find_last_of(\"ab\") == 4);\r\n\
test(string(\"abcab\").find_last_of(\"ba\") == 4);\r\n\
test(string(\"abcab\").find_last_of(\"def\") == -1);\r\n\
\r\n\
\r\n\
test(string().find_first_not_of(\"\") == -1);\r\n\
test(string(\"\").find_first_not_of(\"\") == -1);\r\n\
test(string(\"abc\").find_first_not_of(\"\") == 0);\r\n\
\r\n\
test(string(\"abcab\").find_first_not_of(\"a\") == 1);\r\n\
test(string(\"abcab\").find_first_not_of(\"a\", 1) == 1);\r\n\
test(string(\"abcab\").find_first_not_of(\"a\", 10) == -1);\r\n\
test(string(\"abcab\").find_first_not_of(\"b\") == 0);\r\n\
\r\n\
test(string(\"abcab\").find_first_not_of('a') == 1);\r\n\
test(string(\"abcab\").find_first_not_of('a', 1) == 1);\r\n\
test(string(\"abcab\").find_first_not_of('a', 10) == -1);\r\n\
test(string(\"abcab\").find_first_not_of('b') == 0);\r\n\
\r\n\
test(string(\"abcab\").find_first_not_of(\"cb\") == 0);\r\n\
test(string(\"abcab\").find_first_not_of(\"cb\", 1) == 3);\r\n\
test(string(\"abcab\").find_first_not_of(\"cb\", 3) == 3);\r\n\
test(string(\"abcab\").find_first_not_of(\"cb\", 10) == -1);\r\n\
test(string(\"abcab\").find_first_not_of(\"ab\") == 2);\r\n\
test(string(\"abcab\").find_first_not_of(\"ba\") == 2);\r\n\
test(string(\"abcab\").find_first_not_of(\"def\") == 0);\r\n\
\r\n\
\r\n\
test(string().find_last_not_of(\"\") == -1);\r\n\
test(string(\"\").find_last_not_of(\"\") == -1);\r\n\
test(string(\"\").find_last_not_of(\"\", 5) == -1);\r\n\
test(string(\"abc\").find_last_not_of(\"\") == 2);\r\n\
test(string(\"abc\").find_last_not_of(\"\", 2) == 2);\r\n\
test(string(\"abc\").find_last_not_of(\"\", 3) == 2);\r\n\
test(string(\"abc\").find_last_not_of(\"\", 4) == 2);\r\n\
test(string(\"abc\").find_last_not_of(\"\", 5) == 2);\r\n\
\r\n\
test(string(\"abcab\").find_last_not_of(\"a\") == 4);\r\n\
test(string(\"abcab\").find_last_not_of(\"a\", 1) == 1);\r\n\
test(string(\"abcab\").find_last_not_of(\"a\", 0) == -1);\r\n\
test(string(\"abcab\").find_last_not_of(\"a\", 10) == 4);\r\n\
test(string(\"abcab\").find_last_not_of(\"b\") == 3);\r\n\
test(string(\"abcab\").find_last_not_of(\"c\") == 4);\r\n\
\r\n\
test(string(\"abcab\").find_last_not_of('a') == 4);\r\n\
test(string(\"abcab\").find_last_not_of('a', 1) == 1);\r\n\
test(string(\"abcab\").find_last_not_of('a', 0) == -1);\r\n\
test(string(\"abcab\").find_last_not_of('a', 10) == 4);\r\n\
test(string(\"abcab\").find_last_not_of('b') == 3);\r\n\
test(string(\"abcab\").find_last_not_of('c') == 4);\r\n\
test(string(\"abcab\").find_last_not_of('f', 4) == 4);\r\n\
test(string(\"abcab\").find_last_not_of('f', 5) == 4);\r\n\
test(string(\"abcab\").find_last_not_of('f', 6) == 4);\r\n\
\r\n\
test(string(\"abcab\").find_last_not_of(\"cb\") == 3);\r\n\
test(string(\"abcab\").find_last_not_of(\"cb\", 1) == 0);\r\n\
test(string(\"abcab\").find_last_not_of(\"cb\", 3) == 3);\r\n\
test(string(\"abcab\").find_last_not_of(\"cb\", 10) == 3);\r\n\
test(string(\"abcab\").find_last_not_of(\"ab\") == 2);\r\n\
test(string(\"abcab\").find_last_not_of(\"ba\") == 2);\r\n\
test(string(\"abcab\").find_last_not_of(\"def\") == 4);\r\n\
\r\n\
\r\n\
test('a' in string(\"abcab\"));\r\n\
test(!('d' in string(\"abcab\")));\r\n\
test(!('\\0' in string(\"abcab\")));\r\n\
\r\n\
\r\n\
test(string().substr(0) == \"\");\r\n\
test(string(\"\").substr(0) == \"\");\r\n\
test(string(\"abcde\").substr(0) == \"abcde\");\r\n\
test(string(\"abcde\").substr(2) == \"cde\");\r\n\
test(string(\"abcde\").substr(4) == \"e\");\r\n\
test(string(\"abcde\").substr(5) == \"\");\r\n\
test(string(\"abcde\").substr(0, 3) == \"abc\");\r\n\
test(string(\"abcde\").substr(2, 3) == \"cde\");\r\n\
test(string(\"abcde\").substr(4, 3) == \"e\");\r\n\
test(string(\"abcde\").substr(2, 0) == \"\");\r\n\
test(string(\"abcde\").substr(2, 10) == \"cde\");\r\n\
\r\n\
\r\n\
test(string(\"abcde\")[0, 4] == \"abcde\");\r\n\
test(string(\"abcde\")[2, 4] == \"cde\");\r\n\
test(string(\"abcde\")[4, 4] == \"e\");\r\n\
test(string(\"abcde\")[0, 2] == \"abc\");\r\n\
\r\n\
\r\n\
test(string(\"\") + \"\" == \"\");\r\n\
test(string(\"\") + string(\"\") == \"\");\r\n\
test(string() + \"\" == \"\");\r\n\
test(string() + string() == \"\");\r\n\
test(string() + string() == string());\r\n\
\r\n\
test(string() + string(\"abc\") == \"abc\");\r\n\
test(string(\"abc\") + string(\"\") == \"abc\");\r\n\
test(string(\"ab\") + string(\"c\") == \"abc\");\r\n\
\r\n\
test(\"\" + string(\"abc\") == \"abc\");\r\n\
test(string(\"abc\") + \"\" == \"abc\");\r\n\
\r\n\
\r\n\
antitest(string() != string(\"\"));\r\n\
antitest(string() != string(0, 0));\r\n\
antitest(string() != \"\");\r\n\
antitest(string(\"abc\") != \"abc\");\r\n\
antitest(string(15, 'a') != \"aaaaaaaaaaaaaaa\");\r\n\
\r\n\
return passed / count;";
TEST_RESULT("std.string test (everything)", testSglStringFull, "1");

const char *testSglString1 =
"import std.string;\r\n\
\r\n\
string x = \"Hello\";\r\n\
\r\n\
x.clear();\r\n\
\r\n\
return x.length();";
TEST_RESULT("std.string test (clear)", testSglString1, "0");
