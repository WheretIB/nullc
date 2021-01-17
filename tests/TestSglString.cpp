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

const char *testSglStringRawOps =
"import std.string;\r\n\
\r\n\
assert(strlen(\"\") == 0);\r\n\
assert(strlen(\"test\") == 4);\r\n\
assert(strlen(\"te\\0st\") == 2);\r\n\
assert(strlen(\"test\\0\") == 4);\r\n\
\r\n\
assert(strstr(\"\", \"\") == 0);\r\n\
assert(strstr(\"\", \"test\") == -1);\r\n\
assert(strstr(\"test\", \"st\") == 2);\r\n\
assert(strstr(\"test\", \"test\") == 0);\r\n\
assert(strstr(\"test\", \"es\\0zz\") == 1);\r\n\
assert(strstr(\"te\\0st\", \"st\") == -1);\r\n\
\r\n\
assert(strchr(\"test\", 'z') == -1);\r\n\
assert(strchr(\"test\", 's') == 2);\r\n\
assert(strchr(\"test\", '\\0') == 4);\r\n\
assert(strchr(\"\", '\\0') == 0);\r\n\
assert(strchr(\"te\\0st\", 's') == -1);\r\n\
\r\n\
assert(strcmp(\"test\", \"none\") == 1);\r\n\
assert(strcmp(\"none\", \"test\") == -1);\r\n\
assert(strcmp(\"test\", \"zz\") == -1);\r\n\
assert(strcmp(\"zz\", \"test\") == 1);\r\n\
assert(strcmp(\"test\", \"test\") == 0);\r\n\
assert(strcmp(\"\", \"\") == 0);\r\n\
assert(strcmp(\"te\\0st\", \"te\") == 0);\r\n\
assert(strcmp(\"te\", \"te\\0st\") == 0);\r\n\
\r\n\
assert(strcpy(new char[16], \"test\") == 4);\r\n\
assert(strcpy(new char[16], \"\") == 0);\r\n\
assert(strcpy(new char[16], \"te\\0st\") == 2);\r\n\
\r\n\
return 1;";
TEST_RESULT("std.string test (raw character array operation)", testSglStringRawOps, "1");

const char *testSglStringRawOpsErrors =
"import std.string;\r\n\
import std.error;\r\n\
\r\n\
assert(try(<> strlen(nullptr)).message == \"string is null\");\r\n\
assert(try(<> strlen({ 't', 'e', 's', 't' })).message == \"string is not null-terminated\");\r\n\
\r\n\
assert(try(<> strstr(nullptr, \"test\")).message == \"string is null\");\r\n\
assert(try(<> strstr(\"test\", nullptr)).message == \"substring is null\");\r\n\
assert(try(<> strstr({ 't', 'e', 's', 't' }, \"st\")).message == \"string is not null-terminated\");\r\n\
assert(try(<> strstr(\"test\", { 's', 't' })).message == \"substring is not null-terminated\");\r\n\
\r\n\
assert(try(<> strchr(nullptr, 't')).message == \"string is null\");\r\n\
assert(try(<> strchr({ 't', 'e', 's', 't' }, 'e')).message == \"string is not null-terminated\");\r\n\
\r\n\
assert(try(<> strcmp(nullptr, \"test\")).message == \"first string is null\");\r\n\
assert(try(<> strcmp(\"test\", nullptr)).message == \"second string is null\");\r\n\
assert(try(<> strcmp({ 't', 'e', 's', 't' }, \"test\")).message == \"first string is not null-terminated\");\r\n\
assert(try(<> strcmp(\"test\", { 't', 'e', 's', 't' })).message == \"second string is not null-terminated\");\r\n\
\r\n\
assert(try(<> strcpy(nullptr, \"test\")).message == \"destination string is null\");\r\n\
assert(try(<> strcpy(new char[16], nullptr)).message == \"source string is null\");\r\n\
assert(try(<> strcpy(new char[16], { 't', 'e', 's', 't' })).message == \"string is not null-terminated\");\r\n\
assert(try(<> strcpy(new char[1], \"test\")).message == \"buffer overflow\");\r\n\
assert(try(<> strcpy(new char[4], \"test\")).message == \"buffer overflow\");\r\n\
\r\n\
return 1;";
TEST_RESULT("std.string test (raw character array errors)", testSglStringRawOpsErrors, "1");

const char *testSglStringCharacterArrayInterop =
"import std.string;\r\n\
\r\n\
assert(string(\"test\") == \"test\");\r\n\
assert(string(\"te\\0st\") == \"te\");\r\n\
assert(string(\"\\0test\") == \"\");\r\n\
assert(string({ 't', 'e', 's', 't' }) == \"test\");\r\n\
assert(string({ '\\0' }) == \"\");\r\n\
assert(string(char[](nullptr)) == \"\");\r\n\
\r\n\
assert(!string(\"test\").empty());\r\n\
assert(!string(\"te\\0st\").empty());\r\n\
assert(string(\"\\0test\").empty());\r\n\
assert(!string({ 't', 'e', 's', 't' }).empty());\r\n\
assert(string({ '\\0' }).empty());\r\n\
assert(string(char[](nullptr)).empty());\r\n\
assert(string().empty());\r\n\
\r\n\
assert(string(\"test\").length() == 4);\r\n\
assert(string(\"te\\0st\").length() == 2);\r\n\
assert(string(\"\\0test\").length() == 0);\r\n\
assert(string({ 't', 'e', 's', 't' }).length() == 4);\r\n\
assert(string({ '\\0' }).length() == 0);\r\n\
assert(string(char[](nullptr)).length() == 0);\r\n\
assert(string().length() == 0);\r\n\
\r\n\
string s;\r\n\
assert((s = \"test\") == \"test\");\r\n\
assert((s = \"te\\0st\") == \"te\");\r\n\
assert((s = \"\\0test\") == \"\");\r\n\
assert((s = { 't', 'e', 's', 't' }) == \"test\");\r\n\
assert((s = { '\\0' }) == \"\");\r\n\
assert((s = char[](nullptr)) == \"\");\r\n\
\r\n\
assert((s = \"test\").length() == 4);\r\n\
assert((s = \"te\\0st\").length() == 2);\r\n\
assert((s = \"\\0test\").length() == 0);\r\n\
assert((s = { 't', 'e', 's', 't' }).length() == 4);\r\n\
assert((s = { '\\0' }).length() == 0);\r\n\
assert((s = char[](nullptr)).length() == 0);\r\n\
\r\n\
assert(\"te\\0st\" + string(\"test\") == \"tetest\");\r\n\
assert({ 'z', 'e', 's', 't' } + string(\"test\") == \"zesttest\");\r\n\
assert(\"\\0test\" + string(\"test\") == \"test\");\r\n\
assert(\"\\0\" + string(\"test\") == \"test\");\r\n\
assert(\"\" + string(\"test\") == \"test\");\r\n\
\r\n\
assert(string(\"test\") + \"te\\0st\"  == \"testte\");\r\n\
assert(string(\"test\") + { 'z', 'e', 's', 't' }  == \"testzest\");\r\n\
assert(string(\"test\") + \"\\0test\" == \"test\");\r\n\
assert(string(\"test\") + \"\\0\" == \"test\");\r\n\
assert(string(\"test\") + \"\" == \"test\");\r\n\
\r\n\
assert(\"te\\0st\" + string(\"test\") == \"tetest\\0zzz\");\r\n\
assert({ 'z', 'e', 's', 't' } + string(\"test\") == \"zesttest\\0zzz\");\r\n\
assert(\"\\0test\" + string(\"test\") == \"test\\0zzz\");\r\n\
assert(\"\\0\" + string(\"test\") == \"test\\0zzz\");\r\n\
assert(\"\" + string(\"test\") == \"test\\0zzz\");\r\n\
\r\n\
assert(\"tetest\\0zzz\" == \"te\\0st\" + string(\"test\"));\r\n\
assert(\"zesttest\\0zzz\" == { 'z', 'e', 's', 't' } + string(\"test\"));\r\n\
assert(\"test\\0zzz\" == \"\\0test\" + string(\"test\"));\r\n\
assert(\"test\\0zzz\" == \"\\0\" + string(\"test\"));\r\n\
assert(\"test\\0zzz\" == \"\" + string(\"test\"));\r\n\
\r\n\
assert(string(\"test\") + \"te\\0st\"  != \"zestte\");\r\n\
assert(string(\"test\") + { 'z', 'e', 's', 't' }  != \"zestzest\");\r\n\
assert(string(\"test\") + \"\\0test\" != \"zest\");\r\n\
\r\n\
return 1;";
TEST_RESULT("std.string test (interaction with character arrays)", testSglStringCharacterArrayInterop, "1");
