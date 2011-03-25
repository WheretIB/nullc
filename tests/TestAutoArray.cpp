#include "TestBase.h"

const char *testAutoArray1 = "auto str = \"Hello\"; auto[] arr = str; return char(arr[3]) - 'l';";
TEST_RESULT("Array to auto[] type conversion and access 1.", testAutoArray1, "0");
const char *testAutoArray2 = "char[6] str = \"Hello\"; auto[] arr = str; return char(arr[3]) - 'l';";
TEST_RESULT("Array to auto[] type conversion and access 2.", testAutoArray2, "0");
const char *testAutoArray3 = "auto str = \"Hello\"; auto[] arr = str; char[] str2 = arr; return str == str2;";
TEST_RESULT("auto[] type to array conversion 1", testAutoArray3, "1");
const char *testAutoArray4 = "auto str = \"Hello\"; auto[] arr = str; char[6] str2 = arr; return str == str2;";
TEST_RESULT("auto[] type to array conversion 2", testAutoArray4, "1");
const char *testAutoArray5 = "auto str = \"Hello\"; auto[] arr = str; auto[] arr2 = arr; char[] str2 = arr; char[] str3 = arr2; return str2 == str3;";
TEST_RESULT("auto[] type to auto[] assignment", testAutoArray5, "1");

const char *testAutoArrayImplicit =
"int arrSize(auto[] arr){ return arr.size; }\r\n\
char[] x = \"hello\";\r\n\
int a = arrSize(x);\r\n\
int b = arrSize({ 1, 2, 3, 4});\r\n\
int c = arrSize(new int[43]);\r\n\
return a * 100 + b * 10 + c;";
TEST_RESULT("Implicit conversion to auto[] in function arguments", testAutoArrayImplicit, "683");

const char *testDuplicateHasCorrectSelectionForArray =
"auto a = duplicate({1, 2, 3, 4});\r\n\
return int(a[2]);";
TEST_RESULT("duplicate(auto[]) is selected for typed array arguments", testDuplicateHasCorrectSelectionForArray, "3");
