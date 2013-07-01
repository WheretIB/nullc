#include "stdafx.h"

#include "CodeInfo.h"

#include "Bytecode.h"

#include "Compiler.h"

#include "Parser.h"
#include "Callbacks.h"

#ifndef NULLC_NO_EXECUTOR
	#include "StdLib.h"
#endif

#include "BinaryCache.h"

#include "Executor_Common.h"

void ThrowError(const char* pos, const char* err, ...)
{
	char errorText[4096];

	va_list args;
	va_start(args, err);

	vsnprintf(errorText, 4096, err, args);
	errorText[4096-1] = '\0';

	CodeInfo::lastError = CompilerError(errorText, pos);
	longjmp(CodeInfo::errorHandler, 1);
}

jmp_buf CodeInfo::errorHandler;
//////////////////////////////////////////////////////////////////////////
//						Code gen ops
//////////////////////////////////////////////////////////////////////////

TypeInfo*	typeVoid = NULL;
TypeInfo*	typeChar = NULL;
TypeInfo*	typeShort = NULL;
TypeInfo*	typeInt = NULL;
TypeInfo*	typeFloat = NULL;
TypeInfo*	typeLong = NULL;
TypeInfo*	typeDouble = NULL;
TypeInfo*	typeObject = NULL;
TypeInfo*	typeTypeid = NULL;
TypeInfo*	typeAutoArray = NULL;
TypeInfo*	typeFunction = NULL;

TypeInfo*	typeGeneric = NULL;
TypeInfo*	typeBool = NULL;

namespace NULLC
{
	FastVector<Compiler::CodeRange>	*codeSourceRange = NULL;
}

CompilerError::CompilerError(const char* errStr, const char* apprPos)
{
	Init(errStr, apprPos);
}

void CompilerError::Init(const char* errStr, const char* apprPos)
{
	empty = 0;
	unsigned int len = (unsigned int)strlen(errStr) < NULLC_ERROR_BUFFER_SIZE ? (unsigned int)strlen(errStr) : NULLC_ERROR_BUFFER_SIZE - 1;
	memcpy(errLocal, errStr, len);
	errLocal[len] = 0;
	const char *intStart = codeStart, *intEnd = codeEnd;
	if(apprPos < intStart || apprPos > intEnd)
	{
		for(unsigned i = 1; i < NULLC::codeSourceRange->size(); i++)
		{
			if(apprPos >= (*NULLC::codeSourceRange)[i].start && apprPos <= (*NULLC::codeSourceRange)[i].end)
			{
				intStart = (*NULLC::codeSourceRange)[i].start;
				intEnd = (*NULLC::codeSourceRange)[i].end;
				break;
			}
		}
	}
	if(apprPos && apprPos >= intStart && apprPos <= intEnd)
	{
		const char *begin = apprPos;
		while((begin >= intStart) && (*begin != '\n') && (*begin != '\r'))
			begin--;
		if(begin < apprPos)
			begin++;

		lineNum = 1;
		const char *scan = intStart;
		while(*scan && scan < begin)
			if(*(scan++) == '\n')
				lineNum++;

		const char *end = apprPos;
		while((*end != '\r') && (*end != '\n') && (*end != 0))
			end++;
		len = (unsigned int)(end - begin) < 128 ? (unsigned int)(end - begin) : 127;
		memcpy(line, begin, len);
		for(unsigned int k = 0; k < len; k++)
			if((unsigned char)line[k] < 0x20)
				line[k] = ' ';
		line[len] = 0;
		shift = (unsigned int)(apprPos-begin);
		shift = shift > 128 ? 0 : shift;
	}else{
		line[0] = 0;
		shift = 0;
		lineNum = 0;
	}
}

const char *CompilerError::codeStart = NULL;
const char *CompilerError::codeEnd = NULL;

char* CompilerError::errLocal = NULL;
char* CompilerError::errGlobal = NULL;

const char *nullcBaseCode = "\
void assert(int val);\r\n\
void assert(int val, char[] message);\r\n\
\r\n\
int operator ==(char[] a, b);\r\n\
int operator !=(char[] a, b);\r\n\
char[] operator +(char[] a, b);\r\n\
char[] operator +=(char[] ref a, char[] b);\r\n\
\r\n\
bool bool(bool a);\r\n\
char char(char a);\r\n\
short short(short a);\r\n\
int int(int a);\r\n\
long long(long a);\r\n\
float float(float a);\r\n\
double double(double a);\r\n\
void bool:bool(bool a){ *this = a; }\r\n\
void char:char(char a){ *this = a; }\r\n\
void short:short(short a){ *this = a; }\r\n\
void int:int(int a){ *this = a; }\r\n\
void long:long(long a){ *this = a; }\r\n\
void float:float(float a){ *this = a; }\r\n\
void double:double(double a){ *this = a; }\r\n\
\r\n\
int as_unsigned(char a);\r\n\
int as_unsigned(short a);\r\n\
long as_unsigned(int a);\r\n\
\r\n\
short short(char[] str);\r\n\
void short:short(char[] str){ *this = short(str); }\r\n\
char[] short:str();\r\n\
int int(char[] str);\r\n\
void int:int(char[] str){ *this = int(str); }\r\n\
char[] int:str();\r\n\
long long(char[] str);\r\n\
void long:long(char[] str){ *this = long(str); }\r\n\
char[] long:str();\r\n\
float float(char[] str);\r\n\
void float:float(char[] str){ *this = float(str); }\r\n\
char[] float:str(int precision = 6, bool showExponent = false);\r\n\
double double(char[] str);\r\n\
void double:double(char[] str){ *this = double(str); }\r\n\
char[] double:str(int precision = 6, bool showExponent = false);\r\n\
\r\n\
void ref __newS(int size, int type);\r\n\
int[] __newA(int size, int count, int type);\r\n\
auto ref	duplicate(auto ref obj);\r\n\
void		__duplicate_array(auto[] ref dst, auto[] src);\r\n\
auto[]		duplicate(auto[] arr){ auto[] r; __duplicate_array(&r, arr); return r; }\r\n\
auto ref	replace(auto ref l, r);\r\n\
void		swap(auto ref l, r);\r\n\
int			equal(auto ref l, r);\r\n\
void		assign(explicit auto ref l, auto ref r);\r\n\
void		array_copy(auto[] l, r);\r\n\
void		array_copy(generic dst, int offsetDst, generic src, int offsetSrc, count)\r\n\
{\r\n\
	for({ int i = offsetDst, k = offsetSrc; }; i < offsetDst + count; {i++; k++; })\r\n\
		dst[i] = src[k];\r\n\
}\r\n\
\r\n\
void ref() __redirect(auto ref r, __function[] ref f);\r\n\
void ref() __redirect_ptr(auto ref r, __function[] ref f);\r\n\
// char inline array definition support\r\n\
auto operator=(char[] ref dst, int[] src)\r\n\
{\r\n\
	if(dst.size < src.size)\r\n\
		*dst = new char[src.size];\r\n\
	for(int i = 0; i < src.size; i++)\r\n\
		dst[i] = src[i];\r\n\
	return dst;\r\n\
}\r\n\
// short inline array definition support\r\n\
auto operator=(short[] ref dst, int[] src)\r\n\
{\r\n\
	if(dst.size < src.size)\r\n\
		*dst = new short[src.size];\r\n\
	for(int i = 0; i < src.size; i++)\r\n\
		dst[i] = src[i];\r\n\
	return dst;\r\n\
}\r\n\
// float inline array definition support\r\n\
auto operator=(float[] ref dst, double[] src)\r\n\
{\r\n\
	if(dst.size < src.size)\r\n\
		*dst = new float[src.size];\r\n\
	for(int i = 0; i < src.size; i++)\r\n\
		dst[i] = src[i];\r\n\
	return dst;\r\n\
}\r\n\
// typeid retrieval from auto ref\r\n\
typeid typeid(auto ref type);\r\n\
int typeid.size();\r\n\
// typeid comparison\r\n\
int operator==(typeid a, b);\r\n\
int operator!=(typeid a, b);\r\n\
\r\n\
int __rcomp(auto ref a, b);\r\n\
int __rncomp(auto ref a, b);\r\n\
\r\n\
bool operator<(explicit auto ref a, b);\n\r\
bool operator<=(explicit auto ref a, b);\n\r\
bool operator>(explicit auto ref a, b);\n\r\
bool operator>=(explicit auto ref a, b);\n\r\
\r\n\
int hash_value(explicit auto ref x);\r\n\
\r\n\
int __pcomp(void ref(int) a, void ref(int) b);\r\n\
int __pncomp(void ref(int) a, void ref(int) b);\r\n\
\r\n\
int __acomp(auto[] a, b);\r\n\
int __ancomp(auto[] a, b);\r\n\
\r\n\
int __typeCount();\r\n\
\r\n\
auto[] ref operator=(auto[] ref l, auto ref r);\r\n\
auto ref operator=(auto ref l, auto[] ref r);\r\n\
auto[] ref operator=(auto[] ref l, auto[] ref r);\r\n\
auto ref operator[](auto[] ref l, int index);\r\n\
// const string implementation\r\n\
class const_string\r\n\
{\r\n\
	char[] arr;\r\n\
	int size{ get{ return arr.size; } };\r\n\
}\r\n\
auto operator=(const_string ref l, char[] arr)\r\n\
{\r\n\
	l.arr = arr;\r\n\
	return l;\r\n\
}\r\n\
const_string const_string(char[] arr)\r\n\
{\r\n\
	const_string ret = arr;\r\n\
	return ret;\r\n\
}\r\n\
char operator[](const_string ref l, int index)\r\n\
{\r\n\
	return l.arr[index];\r\n\
}\r\n\
int operator==(const_string a, b){ return a.arr == b.arr; }\r\n\
int operator==(const_string a, char[] b){ return a.arr == b; }\r\n\
int operator==(char[] a, const_string b){ return a == b.arr; }\r\n\
int operator!=(const_string a, b){ return a.arr != b.arr; }\r\n\
int operator!=(const_string a, char[] b){ return a.arr != b; }\r\n\
int operator!=(char[] a, const_string b){ return a != b.arr; }\r\n\
const_string operator+(const_string a, b){ return const_string(a.arr + b.arr); }\r\n\
const_string operator+(const_string a, char[] b){ return const_string(a.arr + b); }\r\n\
const_string operator+(char[] a, const_string b){ return const_string(a + b.arr); }\r\n\
\r\n\
int isStackPointer(auto ref x);\r\n\
\r\n\
// list comprehension helper functions\r\n\
void auto_array_impl(auto[] ref arr, typeid type, int count);\r\n\
auto[] auto_array(typeid type, int count)\r\n\
{\r\n\
	auto[] res;\r\n\
	auto_array_impl(&res, type, count);\r\n\
	return res;\r\n\
}\r\n\
typedef auto[] auto_array;\r\n\
// function will set auto[] element to the specified one, with data reallocation if neccessary\r\n\
void auto_array:set(auto ref x, int pos);\r\n\
void __force_size(auto[] ref s, int size);\r\n\
\r\n\
int isCoroutineReset(auto ref f);\r\n\
void __assertCoroutine(auto ref f);\r\n\
auto ref[] __getFinalizeList();\r\n\
class __FinalizeProxy{ void finalize(){} }\r\n\
void	__finalizeObjects()\r\n\
{\r\n\
	auto l = __getFinalizeList();\r\n\
	for(i in l)\r\n\
		i.finalize();\r\n\
}\r\n\
bool operator in(generic x, typeof(x)[] arr)\r\n\
{\r\n\
	for(i in arr)\r\n\
		if(i == x)\r\n\
			return true;\r\n\
	return false;\r\n\
}\r\n\
void ref assert_derived_from_base(void ref derived, typeid base);\r\n\
auto __gen_list(@T ref() y)\r\n\
{\r\n\
	auto[] res = auto_array(T, 1);\r\n\
	int pos = 0;\r\n\
	for(T x in y)\r\n\
		res.set(&x, pos++);\r\n\
	__force_size(&res, pos);\r\n\
	\r\n\
	T[] r = res;\r\n\
	return r;\r\n\
}";

Compiler::Compiler()
{
	CompilerError::errLocal = (char*)NULLC::alloc(NULLC_ERROR_BUFFER_SIZE);
	CompilerError::errGlobal = (char*)NULLC::alloc(NULLC_ERROR_BUFFER_SIZE);

	buildInTypes.clear();
	buildInTypes.reserve(32);

	NULLC::codeSourceRange = &codeSourceRange;

	// Add basic types
	TypeInfo* info;
	info = new TypeInfo(CodeInfo::typeInfo.size(), "void", 0, 0, 1, NULL, TypeInfo::TYPE_VOID);
	info->size = 0;
	typeVoid = info;
	CodeInfo::typeInfo.push_back(info);

	info = new TypeInfo(CodeInfo::typeInfo.size(), "double", 0, 0, 1, NULL, TypeInfo::TYPE_DOUBLE);
	info->alignBytes = 8;
	info->size = 8;
	typeDouble = info;
	CodeInfo::typeInfo.push_back(info);

	info = new TypeInfo(CodeInfo::typeInfo.size(), "float", 0, 0, 1, NULL, TypeInfo::TYPE_FLOAT);
	info->alignBytes = 4;
	info->size = 4;
	typeFloat = info;
	CodeInfo::typeInfo.push_back(info);

	info = new TypeInfo(CodeInfo::typeInfo.size(), "long", 0, 0, 1, NULL, TypeInfo::TYPE_LONG);
	info->size = 8;
	typeLong = info;
	CodeInfo::typeInfo.push_back(info);

	info = new TypeInfo(CodeInfo::typeInfo.size(), "int", 0, 0, 1, NULL, TypeInfo::TYPE_INT);
	info->alignBytes = 4;
	info->size = 4;
	typeInt = info;
	CodeInfo::typeInfo.push_back(info);

	info = new TypeInfo(CodeInfo::typeInfo.size(), "short", 0, 0, 1, NULL, TypeInfo::TYPE_SHORT);
	info->alignBytes = 2;
	info->size = 2;
	typeShort = info;
	CodeInfo::typeInfo.push_back(info);

	info = new TypeInfo(CodeInfo::typeInfo.size(), "char", 0, 0, 1, NULL, TypeInfo::TYPE_CHAR);
	info->size = 1;
	typeChar = info;
	CodeInfo::typeInfo.push_back(info);

	info = new TypeInfo(CodeInfo::typeInfo.size(), "auto ref", 0, 0, 1, NULL, TypeInfo::TYPE_COMPLEX);
	typeObject = info;
	CodeInfo::typeInfo.push_back(info);

	info = new TypeInfo(CodeInfo::typeInfo.size(), "typeid", 0, 0, 1, NULL, TypeInfo::TYPE_COMPLEX);
	typeTypeid = info;
	CodeInfo::typeInfo.push_back(info);

	info->size = 4;

	typeObject->AddMemberVariable("type", typeTypeid);
	typeObject->AddMemberVariable("ptr", CodeInfo::GetReferenceType(typeVoid));
	typeObject->size = 4 + NULLC_PTR_SIZE;

	info = new TypeInfo(CodeInfo::typeInfo.size(), "auto[]", 0, 0, 1, NULL, TypeInfo::TYPE_COMPLEX);
	typeAutoArray = info;
	CodeInfo::typeInfo.push_back(info);

	typeAutoArray->AddMemberVariable("type", typeTypeid);
	typeAutoArray->AddMemberVariable("ptr", CodeInfo::GetReferenceType(typeVoid));
	typeAutoArray->AddMemberVariable("size", typeInt);
	typeAutoArray->size = 8 + NULLC_PTR_SIZE;

	info = new TypeInfo(CodeInfo::typeInfo.size(), "__function", 0, 0, 1, NULL, TypeInfo::TYPE_COMPLEX);
	info->size = 4;
	typeFunction = info;
	CodeInfo::typeInfo.push_back(info);

	info = new TypeInfo(CodeInfo::typeInfo.size(), "generic", 0, 0, 1, NULL, TypeInfo::TYPE_VOID);
	info->size = 0;
	info->dependsOnGeneric = true;
	typeGeneric = info;
	CodeInfo::typeInfo.push_back(info);

	info = new TypeInfo(CodeInfo::typeInfo.size(), "bool", 0, 0, 1, NULL, TypeInfo::TYPE_CHAR);
	info->size = 1;
	typeBool = info;
	CodeInfo::typeInfo.push_back(info);

	buildInTypes.resize(CodeInfo::typeInfo.size());
	memcpy(&buildInTypes[0], &CodeInfo::typeInfo[0], CodeInfo::typeInfo.size() * sizeof(TypeInfo*));

	typeTop = TypeInfo::GetPoolTop();

	typeMap.init();
	funcMap.init();

	realGlobalCount = 0;

	// Add base module with built-in functions
	baseModuleSize = 0;
	bool res = Compile(nullcBaseCode);
	(void)res;
	assert(res && "Failed to compile base NULLC module");
	baseModuleSize = lexer.GetStreamSize();

	char *bytecode = NULL;
	GetBytecode(&bytecode);
	BinaryCache::PutBytecode("$base$.nc", bytecode, NULL, 0);

#ifndef NULLC_NO_EXECUTOR
	AddModuleFunction("$base$", (void (*)())NULLC::Assert, "assert", 0);
	AddModuleFunction("$base$", (void (*)())NULLC::Assert2, "assert", 1);

	AddModuleFunction("$base$", (void (*)())NULLC::StrEqual, "==", 0);
	AddModuleFunction("$base$", (void (*)())NULLC::StrNEqual, "!=", 0);
	AddModuleFunction("$base$", (void (*)())NULLC::StrConcatenate, "+", 0);
	AddModuleFunction("$base$", (void (*)())NULLC::StrConcatenateAndSet, "+=", 0);

	AddModuleFunction("$base$", (void (*)())NULLC::Int, "bool", 0);
	AddModuleFunction("$base$", (void (*)())NULLC::Char, "char", 0);
	AddModuleFunction("$base$", (void (*)())NULLC::Short, "short", 0);
	AddModuleFunction("$base$", (void (*)())NULLC::Int, "int", 0);
	AddModuleFunction("$base$", (void (*)())NULLC::Long, "long", 0);
	AddModuleFunction("$base$", (void (*)())NULLC::Float, "float", 0);
	AddModuleFunction("$base$", (void (*)())NULLC::Double, "double", 0);

	AddModuleFunction("$base$", (void (*)())NULLC::UnsignedValueChar, "as_unsigned", 0);
	AddModuleFunction("$base$", (void (*)())NULLC::UnsignedValueShort, "as_unsigned", 1);
	AddModuleFunction("$base$", (void (*)())NULLC::UnsignedValueInt, "as_unsigned", 2);

	AddModuleFunction("$base$", (void (*)())NULLC::StrToShort, "short", 1);
	AddModuleFunction("$base$", (void (*)())NULLC::ShortToStr, "short::str", 0);
	AddModuleFunction("$base$", (void (*)())NULLC::StrToInt, "int", 1);
	AddModuleFunction("$base$", (void (*)())NULLC::IntToStr, "int::str", 0);
	AddModuleFunction("$base$", (void (*)())NULLC::StrToLong, "long", 1);
	AddModuleFunction("$base$", (void (*)())NULLC::LongToStr, "long::str", 0);
	AddModuleFunction("$base$", (void (*)())NULLC::StrToFloat, "float", 1);
	AddModuleFunction("$base$", (void (*)())NULLC::FloatToStr, "float::str", 0);
	AddModuleFunction("$base$", (void (*)())NULLC::StrToDouble, "double", 1);
	AddModuleFunction("$base$", (void (*)())NULLC::DoubleToStr, "double::str", 0);

	AddModuleFunction("$base$", (void (*)())NULLC::AllocObject, "__newS", 0);
	AddModuleFunction("$base$", (void (*)())NULLC::AllocArray, "__newA", 0);
	AddModuleFunction("$base$", (void (*)())NULLC::CopyObject, "duplicate", 0);
	AddModuleFunction("$base$", (void (*)())NULLC::CopyArray, "__duplicate_array", 0);
	AddModuleFunction("$base$", (void (*)())NULLC::ReplaceObject, "replace", 0);
	AddModuleFunction("$base$", (void (*)())NULLC::SwapObjects, "swap", 0);
	AddModuleFunction("$base$", (void (*)())NULLC::CompareObjects, "equal", 0);
	AddModuleFunction("$base$", (void (*)())NULLC::AssignObject, "assign", 0);

	AddModuleFunction("$base$", (void (*)())NULLC::ArrayCopy, "array_copy", 0);
	
	AddModuleFunction("$base$", (void (*)())NULLC::FunctionRedirect, "__redirect", 0);
	AddModuleFunction("$base$", (void (*)())NULLC::FunctionRedirectPtr, "__redirect_ptr", 0);

	AddModuleFunction("$base$", (void (*)())NULLC::Typeid, "typeid", 0);
	AddModuleFunction("$base$", (void (*)())NULLC::TypeSize, "typeid::size$", 0);
	AddModuleFunction("$base$", (void (*)())NULLC::TypesEqual, "==", 1);
	AddModuleFunction("$base$", (void (*)())NULLC::TypesNEqual, "!=", 1);

	AddModuleFunction("$base$", (void (*)())NULLC::RefCompare, "__rcomp", 0);
	AddModuleFunction("$base$", (void (*)())NULLC::RefNCompare, "__rncomp", 0);
	AddModuleFunction("$base$", (void (*)())NULLC::RefLCompare, "<", 0);
	AddModuleFunction("$base$", (void (*)())NULLC::RefLECompare, "<=", 0);
	AddModuleFunction("$base$", (void (*)())NULLC::RefGCompare, ">", 0);
	AddModuleFunction("$base$", (void (*)())NULLC::RefGECompare, ">=", 0);

	AddModuleFunction("$base$", (void (*)())NULLC::RefHash, "hash_value", 0);

	AddModuleFunction("$base$", (void (*)())NULLC::FuncCompare, "__pcomp", 0);
	AddModuleFunction("$base$", (void (*)())NULLC::FuncNCompare, "__pncomp", 0);

	AddModuleFunction("$base$", (void (*)())NULLC::ArrayCompare, "__acomp", 0);
	AddModuleFunction("$base$", (void (*)())NULLC::ArrayNCompare, "__ancomp", 0);

	AddModuleFunction("$base$", (void (*)())NULLC::TypeCount, "__typeCount", 0);

	AddModuleFunction("$base$", (void (*)())NULLC::AutoArrayAssign, "=", 3);
	AddModuleFunction("$base$", (void (*)())NULLC::AutoArrayAssignRev, "=", 4);
	AddModuleFunction("$base$", (void (*)())NULLC::AutoArrayAssignSelf, "=", 5);
	AddModuleFunction("$base$", (void (*)())NULLC::AutoArrayIndex, "[]", 0);

	AddModuleFunction("$base$", (void (*)())IsPointerUnmanaged, "isStackPointer", 0);

	AddModuleFunction("$base$", (void (*)())NULLC::AutoArray, "auto_array_impl", 0);
	AddModuleFunction("$base$", (void (*)())NULLC::AutoArraySet, "auto[]::set", 0);
	AddModuleFunction("$base$", (void (*)())NULLC::ShrinkAutoArray, "__force_size", 0);

	AddModuleFunction("$base$", (void (*)())NULLC::IsCoroutineReset, "isCoroutineReset", 0);
	AddModuleFunction("$base$", (void (*)())NULLC::AssertCoroutine, "__assertCoroutine", 0);

	AddModuleFunction("$base$", (void (*)())NULLC::GetFinalizationList, "__getFinalizeList", 0);

	AddModuleFunction("$base$", (void (*)())NULLC::AssertDerivedFromBase, "assert_derived_from_base", 0);
#endif
}

Compiler::~Compiler()
{
	ParseReset();

	CallbackReset();

	CodeInfo::varInfo.clear();
	CodeInfo::funcInfo.clear();
	CodeInfo::typeInfo.clear();

	NodeBreakOp::fixQueue.reset();
	NodeContinueOp::fixQueue.reset();
	NodeSwitchExpr::fixQueue.reset();

	NodeFuncCall::memoList.reset();
	NodeFuncCall::memoPool.~ChunkedStackPool();

	NodeZeroOP::ResetNodes();

	typeMap.reset();
	funcMap.reset();

	NULLC::dealloc(CompilerError::errLocal);
	NULLC::dealloc(CompilerError::errGlobal);
}

void Compiler::ClearState()
{
	CodeInfo::varInfo.clear();

	CodeInfo::typeInfo.resize(buildInTypes.size());
	memcpy(&CodeInfo::typeInfo[0], &buildInTypes[0], buildInTypes.size() * sizeof(TypeInfo*));
	for(unsigned int i = 0; i < buildInTypes.size(); i++)
	{
		assert(CodeInfo::typeInfo[i]->typeIndex == i);
		CodeInfo::typeInfo[i]->typeIndex = i;
		if(CodeInfo::typeInfo[i]->refType && CodeInfo::typeInfo[i]->refType->typeIndex >= buildInTypes.size())
			CodeInfo::typeInfo[i]->refType = NULL;
		if(CodeInfo::typeInfo[i]->unsizedType && CodeInfo::typeInfo[i]->unsizedType->typeIndex >= buildInTypes.size())
			CodeInfo::typeInfo[i]->unsizedType = NULL;
		CodeInfo::typeInfo[i]->arrayType = NULL;
	}

	for(unsigned int i = 0; i < CodeInfo::typeInfo.size(); i++)
		CodeInfo::typeInfo[i]->fullName = NULL;

	TypeInfo::SetPoolTop(typeTop);

	funcMap.clear();
	CodeInfo::funcInfo.resize(0);
	FunctionInfo::SetPoolTop(0);

	CodeInfo::nodeList.clear();

	ClearStringList();

	VariableInfo::SetPoolTop(0);

	CallbackInitialize();

	CodeInfo::lastError = CompilerError();

	CodeInfo::cmdInfoList.Clear();
	CodeInfo::cmdList.clear();
}

bool Compiler::AddModuleFunction(const char* module, void (NCDECL *ptr)(), const char* name, int index)
{
	char errBuf[256];

	// Find module
	const char *importPath = BinaryCache::GetImportPath();
	char path[256], *pathNoImport = path, *cPath = path;
	cPath += SafeSprintf(path, 256, "%s%.*s", importPath ? importPath : "", module, module);
	if(importPath)
		pathNoImport = path + strlen(importPath);

	for(unsigned int i = 0, e = (unsigned int)strlen(pathNoImport); i != e; i++)
	{
		if(pathNoImport[i] == '.')
			pathNoImport[i] = '/';
	}
	SafeSprintf(cPath, 256 - int(cPath - path), ".nc");
	const char *bytecode = BinaryCache::GetBytecode(path);
	if(!bytecode && importPath)
		bytecode = BinaryCache::GetBytecode(pathNoImport);

	// Create module if not found
	if(!bytecode)
		bytecode = BuildModule(path, pathNoImport);
	if(!bytecode)
	{
		SafeSprintf(errBuf, 256, "Failed to build module %s", module);
		CodeInfo::lastError.Init(errBuf, NULL);
		return false;
	}

	unsigned int hash = GetStringHash(name);
	ByteCode *code = (ByteCode*)bytecode;

	// Find function and set pointer
	ExternFuncInfo *fInfo = FindFirstFunc(code);
	
	unsigned int end = code->functionCount - code->moduleFunctionCount;
	for(unsigned int i = 0; i < end; i++)
	{
		if(hash == fInfo->nameHash)
		{
			if(index == 0)
			{
				fInfo->address = -1;
				fInfo->funcPtr = (void*)ptr;
				index--;
				break;
			}
			index--;
		}
		fInfo++;
	}
	if(index != -1)
	{
		SafeSprintf(errBuf, 256, "ERROR: function %s or one of it's overload is not found in module %s", name, module);
		CodeInfo::lastError.Init(errBuf, NULL);
		return false;
	}

	return true;
}

bool Compiler::ImportModuleNamespaces(const char* bytecode)
{
	ByteCode *bCode = (ByteCode*)bytecode;
	char *symbols = FindSymbols(bCode);

	// Import namespaces
	ExternNamespaceInfo *namespaceList = FindFirstNamespace(bCode);
	for(unsigned i = 0; i < bCode->namespaceCount; i++)
	{
		NamespaceInfo *parent = NULL;
		if(namespaceList->parentHash != ~0u)
		{
			for(unsigned k = 0; k < CodeInfo::namespaceInfo.size() && !parent; k++)
			{
				if(CodeInfo::namespaceInfo[k]->hash == namespaceList->parentHash)
					parent = CodeInfo::namespaceInfo[k];
			}
		}
		unsigned hash = parent ? StringHashContinue(StringHashContinue(namespaceList->parentHash, "."), symbols + namespaceList->offsetToName) : GetStringHash(symbols + namespaceList->offsetToName);
		CodeInfo::namespaceInfo.push_back(new NamespaceInfo(InplaceStr(symbols + namespaceList->offsetToName), hash, parent));
		namespaceList++;
	}

	return true;
}

bool Compiler::ImportModuleTypes(const char* bytecode, const char* pos)
{
	char errBuf[256];
	ByteCode *bCode = (ByteCode*)bytecode;
	char *symbols = FindSymbols(bCode);

	// Import types
	ExternTypeInfo *tInfo = FindFirstType(bCode);
	ExternTypeInfo *tInfoStart = tInfo;
	ExternMemberInfo *memberList = (ExternMemberInfo*)(tInfo + bCode->typeCount);

	unsigned int oldTypeCount = CodeInfo::typeInfo.size();

	typeMap.clear();
	for(unsigned int n = 0; n < oldTypeCount; n++)
		typeMap.insert(CodeInfo::typeInfo[n]->GetFullNameHash(), CodeInfo::typeInfo[n]);

	unsigned int lastTypeNum = CodeInfo::typeInfo.size();
	for(unsigned int i = 0; i < bCode->typeCount; i++, tInfo++)
	{
		TypeInfo **info = typeMap.find(tInfo->nameHash);
		if(info)
			typeRemap.push_back((*info)->typeIndex);
		else
			typeRemap.push_back(lastTypeNum++);
	}

	unsigned skipConstants = 0;
	tInfo = tInfoStart;
	for(unsigned int i = 0; i < bCode->typeCount; i++, tInfo++)
	{
		if(typeRemap[i] >= oldTypeCount)
		{
#ifdef IMPORT_VERBOSE_DEBUG_OUTPUT
			printf(" Importing type %s\r\n", symbols + tInfo->offsetToName);
#endif
			TypeInfo *newInfo = NULL, *tempInfo = NULL;
			switch(tInfo->subCat)
			{
			case ExternTypeInfo::CAT_FUNCTION:
				CodeInfo::typeInfo.push_back(new TypeInfo(CodeInfo::typeInfo.size(), NULL, 0, 0, 1, NULL, TypeInfo::TYPE_COMPLEX));
				newInfo = CodeInfo::typeInfo.back();
				CodeInfo::typeFunctions.push_back(newInfo);
				newInfo->CreateFunctionType(CodeInfo::typeInfo[typeRemap[memberList[tInfo->memberOffset].type]], tInfo->memberCount);

				newInfo->size = tInfo->size;
				newInfo->paddingBytes = tInfo->padding;

				newInfo->dependsOnGeneric = !!(tInfo->typeFlags & ExternTypeInfo::TYPE_DEPENDS_ON_GENERIC);

				for(unsigned int n = 0; n < tInfo->memberCount; n++)
					newInfo->funcType->paramType[n] = CodeInfo::typeInfo[typeRemap[memberList[tInfo->memberOffset + n + 1].type]];

				for(unsigned int n = 0; n < newInfo->funcType->paramCount; n++)
				{
					unsigned size = tInfoStart[memberList[tInfo->memberOffset + n + 1].type].size;
					newInfo->funcType->paramSize += size > 4 ? size : 4;
				}

#ifdef _DEBUG
				newInfo->AddMemberVariable("context", typeInt, false);
				newInfo->lastVariable->offset = 0;
				newInfo->AddMemberVariable("ptr", typeInt, false);
				newInfo->lastVariable->offset = 4;
#endif
				newInfo->hasPointers = true;
				break;
			case ExternTypeInfo::CAT_ARRAY:
				tempInfo = CodeInfo::typeInfo[typeRemap[tInfo->subType]];
				CodeInfo::typeInfo.push_back(new TypeInfo(CodeInfo::typeInfo.size(), NULL, 0, tempInfo->arrLevel + 1, tInfo->arrSize, tempInfo, TypeInfo::TYPE_COMPLEX));
				newInfo = CodeInfo::typeInfo.back();

				newInfo->size = tInfo->size;
				newInfo->paddingBytes = tInfo->padding;

				if(tInfo->arrSize == TypeInfo::UNSIZED_ARRAY)
				{
					newInfo->AddMemberVariable("size", typeInt, false);
					newInfo->lastVariable->offset = NULLC_PTR_SIZE;
				}

				newInfo->nextArrayType = tempInfo->arrayType;
				newInfo->dependsOnGeneric = !!(tInfo->typeFlags & ExternTypeInfo::TYPE_DEPENDS_ON_GENERIC);
				tempInfo->arrayType = newInfo;
				CodeInfo::typeArrays.push_back(newInfo);
				break;
			case ExternTypeInfo::CAT_POINTER:
				assert(typeRemap[tInfo->subType] < CodeInfo::typeInfo.size());
				tempInfo = CodeInfo::typeInfo[typeRemap[tInfo->subType]];
				CodeInfo::typeInfo.push_back(new TypeInfo(CodeInfo::typeInfo.size(), NULL, tempInfo->refLevel + 1, 0, 1, tempInfo, TypeInfo::NULLC_PTR_TYPE));
				newInfo = CodeInfo::typeInfo.back();

				newInfo->size = tInfo->size;
				newInfo->paddingBytes = tInfo->padding;

				newInfo->dependsOnGeneric = !!(tInfo->typeFlags & ExternTypeInfo::TYPE_DEPENDS_ON_GENERIC);

				// Save it for future use
				CodeInfo::typeInfo[typeRemap[tInfo->subType]]->refType = newInfo;
				break;
			case ExternTypeInfo::CAT_CLASS:
			{
				unsigned int strLength = (unsigned int)strlen(symbols + tInfo->offsetToName) + 1;
				const char *nameCopy = strcpy((char*)dupStringsModule.Allocate(strLength), symbols + tInfo->offsetToName);
				newInfo = new TypeInfo(CodeInfo::typeInfo.size(), nameCopy, 0, 0, 1, NULL, (TypeInfo::TypeCategory)tInfo->type);

				newInfo->size = tInfo->size;
				newInfo->paddingBytes = tInfo->padding;

				newInfo->hasFinalizer = tInfo->typeFlags & ExternTypeInfo::TYPE_HAS_FINALIZER;
				newInfo->dependsOnGeneric = !!(tInfo->typeFlags & ExternTypeInfo::TYPE_DEPENDS_ON_GENERIC);

				CodeInfo::typeInfo.push_back(newInfo);
				CodeInfo::classMap.insert(newInfo->GetFullNameHash(), newInfo);

				if(tInfo->namespaceHash != ~0u)
				{
					for(unsigned k = 0; k < CodeInfo::namespaceInfo.size() && !newInfo->parentNamespace; k++)
					{
						if(CodeInfo::namespaceInfo[k]->hash == tInfo->namespaceHash)
							newInfo->parentNamespace = CodeInfo::namespaceInfo[k];
					}
				}

				// This two pointers are used later to fill type member information
				newInfo->firstVariable = (TypeInfo::MemberVariable*)(symbols + tInfo->offsetToName + strLength);
				newInfo->lastVariable = (TypeInfo::MemberVariable*)tInfo;
				newInfo->definitionDepth = skipConstants;
				skipConstants += tInfo->constantCount;
				// If definitionOffset offset is set
				if(tInfo->definitionOffset != ~0u)
				{
					// It may be because it contains index of generic type base
					if(tInfo->definitionOffset & 0x80000000)
					{
						newInfo->genericBase = CodeInfo::typeInfo[typeRemap[tInfo->definitionOffset & ~0x80000000]];
					}else{ // Or because it is a generic type base
						newInfo->genericInfo = newInfo->CreateGenericContext(tInfo->definitionOffset + int(CodeInfo::lexFullStart - CodeInfo::lexStart));

						newInfo->genericInfo->aliasCount = tInfo->genericTypeCount;

						CodeInfo::nodeList.push_back(new NodeZeroOP());
						newInfo->definitionList = new NodeExpressionList();
						CodeInfo::funcDefList.push_back(newInfo->definitionList);
					}
				}
			}
				break;
			default:
				SafeSprintf(errBuf, 256, "ERROR: new type in module %s named %s unsupported", pos, symbols + tInfo->offsetToName);
				CodeInfo::lastError.Init(errBuf, NULL);
				return false;
			}
			newInfo->alignBytes = tInfo->defaultAlign;
		}else{
			skipConstants += tInfo->constantCount;
		}
	}

	ExternConstantInfo *constantList = FindFirstConstant(bCode);

	for(unsigned int i = oldTypeCount; i < CodeInfo::typeInfo.size(); i++)
	{
		TypeInfo *type = CodeInfo::typeInfo[i];
		if((type->type == TypeInfo::TYPE_COMPLEX || type->firstVariable) && !type->funcType && !type->subType)
		{
#ifdef IMPORT_VERBOSE_DEBUG_OUTPUT
			printf(" Type %s fixup\r\n", type->name);
#endif
			// first and last variable pointer contained pointer that allow to fill all information about members
			const char *memberName = (const char*)type->firstVariable;
			ExternTypeInfo *typeInfo = (ExternTypeInfo*)type->lastVariable;
			type->firstVariable = type->lastVariable = NULL;

			for(unsigned int n = 0; n < typeInfo->memberCount; n++)
			{
				unsigned int strLength = (unsigned int)strlen(memberName) + 1;
				const char *nameCopy = strcpy((char*)dupStringsModule.Allocate(strLength), memberName);
				memberName += strLength;

				TypeInfo *memberType = CodeInfo::typeInfo[typeRemap[memberList[typeInfo->memberOffset + n].type]];

				type->AddMemberVariable(nameCopy, memberType, false);
				type->lastVariable->offset = memberList[typeInfo->memberOffset + n].offset;
			}
			for(unsigned int n = 0; n < typeInfo->constantCount; n++)
			{
				TypeInfo *constantType = CodeInfo::typeInfo[typeRemap[constantList[type->definitionDepth + n].type]];
				NodeNumber *value = new NodeNumber(0, constantType);
				value->num.integer64 = constantList[type->definitionDepth + n].value;

				unsigned int strLength = (unsigned int)strlen(memberName) + 1;
				const char *nameCopy = strcpy((char*)dupStringsModule.Allocate(strLength), memberName);
				memberName += strLength;

				type->AddMemberVariable(nameCopy, typeVoid, false, value);
			}
			type->definitionDepth = 1;
		}
	}

#ifdef IMPORT_VERBOSE_DEBUG_OUTPUT
	tInfo = FindFirstType(bCode);
	for(unsigned int i = 0; i < typeRemap.size(); i++)
		printf("Type\r\n\t%s\r\nis remapped from index %d to index %d with\r\n\t%s\r\ntype\r\n", symbols + tInfo[i].offsetToName, i, typeRemap[i], CodeInfo::typeInfo[typeRemap[i]]->GetFullTypeName());
#endif

	return true;
}

bool Compiler::ImportModuleVariables(const char* bytecode, const char* pos, unsigned int number)
{
	ByteCode *bCode = (ByteCode*)bytecode;
	char *symbols = FindSymbols(bCode);

	ExternVarInfo *vInfo = FindFirstVar(bCode);

	if(!setjmp(CodeInfo::errorHandler))
	{
		// Import variables
		for(unsigned int i = 0; i < bCode->variableExportCount; i++, vInfo++)
		{
			// Exclude temporary variables from import
			if(memcmp(&symbols[vInfo->offsetToName], "$temp", 5) == 0)
				continue;
			SelectTypeByIndex(typeRemap[vInfo->type]);
			AddVariable(pos, InplaceStr(symbols + vInfo->offsetToName), true, false, memcmp(&symbols[vInfo->offsetToName], "$vtbl", 5) == 0);
			CodeInfo::varInfo.back()->parentModule = number;
			CodeInfo::varInfo.back()->pos = (number << 24) + (vInfo->offset & 0x00ffffff);
		}
	}else{
		return false;
	}

	return true;
}

bool Compiler::ImportModuleTypedefs(const char* bytecode)
{
	char errBuf[256];
	ByteCode *bCode = (ByteCode*)bytecode;
	char *symbols = FindSymbols(bCode);

	// Import type aliases
	ExternTypedefInfo *typedefInfo = FindFirstTypedef(bCode);
	for(unsigned i = 0; i < bCode->typedefCount; i++)
	{
		// Find is this alias is already defined
		unsigned int hash = GetStringHash(symbols + typedefInfo->offsetToName);
		TypeInfo **type = CodeInfo::classMap.find(hash);
		TypeInfo *targetType = CodeInfo::typeInfo[typeRemap[typedefInfo->targetType]];
		if(!type)
		{
			SelectTypeByIndex(typeRemap[typedefInfo->targetType]);
			if(typedefInfo->parentType != ~0u)
			{
				AliasInfo *info = TypeInfo::CreateAlias(InplaceStr(symbols + typedefInfo->offsetToName), GetSelectedType());
				TypeInfo *parent = CodeInfo::typeInfo[typeRemap[typedefInfo->parentType]];
				info->next = parent->childAlias;
				parent->childAlias = info;
			}else{
				AddAliasType(InplaceStr(symbols + typedefInfo->offsetToName));
			}
		}else{
			if((*type)->GetFullNameHash() == hash)
			{
				SafeSprintf(errBuf, 256, "ERROR: type '%s' alias '%s' is equal to previously imported class", targetType->GetFullTypeName(), symbols + typedefInfo->offsetToName);
				CodeInfo::lastError.Init(errBuf, NULL);
				return false;
			}else if((*type) != targetType){
				SafeSprintf(errBuf, 256, "ERROR: type '%s' alias '%s' is equal to previously imported alias", targetType->GetFullTypeName(), symbols + typedefInfo->offsetToName);
				CodeInfo::lastError.Init(errBuf, NULL);
				return false;
			}
		}
		typedefInfo++;
	}

	return true;
}

bool Compiler::ImportModuleFunctions(const char* bytecode, const char* pos)
{
	char errBuf[256];
	ByteCode *bCode = (ByteCode*)bytecode;
	char *symbols = FindSymbols(bCode);

	ExternVarInfo *vInfo = FindFirstVar(bCode);

	// Import functions
	ExternFuncInfo *fInfo = FindFirstFunc(bCode);
	ExternLocalInfo *fLocals = FindFirstLocal(bCode);

	unsigned int oldFuncCount = CodeInfo::funcInfo.size();
	unsigned int end = bCode->functionCount - bCode->moduleFunctionCount;
	for(unsigned int i = 0; i < end; i++)
	{
		const unsigned int INDEX_NONE = ~0u;

		unsigned int index = INDEX_NONE;

		TypeInfo* remappedType = CodeInfo::typeInfo[typeRemap[fInfo->funcType]];
		HashMap<unsigned int>::Node *curr = funcMap.first(fInfo->nameHash);
		while(curr)
		{
			if(curr->value < oldFuncCount && CodeInfo::funcInfo[curr->value]->funcType == remappedType)
			{
				index = curr->value;
				break;
			}
			curr = funcMap.next(curr);
		}
		if(index == INDEX_NONE)
		{
			unsigned int strLength = (unsigned int)strlen(symbols + fInfo->offsetToName) + 1;
			const char *nameCopy = strcpy((char*)dupStringsModule.Allocate(strLength), symbols + fInfo->offsetToName);

			unsigned int hashOriginal = fInfo->nameHash;
			const char *extendEnd = strstr(nameCopy, "::");
			if(extendEnd)
				hashOriginal = GetStringHash(extendEnd + 2);

			CodeInfo::funcInfo.push_back(new FunctionInfo(nameCopy, fInfo->nameHash, hashOriginal));
			funcMap.insert(CodeInfo::funcInfo.back()->nameHash, CodeInfo::funcInfo.size()-1);

			FunctionInfo* lastFunc = CodeInfo::funcInfo.back();

			static unsigned int hashNewS = GetStringHash("__newA");
			static unsigned int hashNewA = GetStringHash("__newS");
			if(lastFunc->nameHash == hashNewS || lastFunc->nameHash == hashNewA)
				lastFunc->visible = false;

			AddFunctionToSortedList(lastFunc);

			lastFunc->indexInArr = CodeInfo::funcInfo.size() - 1;
			lastFunc->address = fInfo->funcPtr ? ~0u : (fInfo->codeSize & 0x80000000 ? 0x80000000 : 0);
			lastFunc->funcPtr = fInfo->funcPtr;
			lastFunc->type = (FunctionInfo::FunctionCategory)fInfo->funcCat;

			if(fInfo->namespaceHash != ~0u)
			{
				for(unsigned k = 0; k < CodeInfo::namespaceInfo.size() && !lastFunc->parentNamespace; k++)
				{
					if(CodeInfo::namespaceInfo[k]->hash == fInfo->namespaceHash)
						lastFunc->parentNamespace = CodeInfo::namespaceInfo[k];
				}

				assert(lastFunc->parentNamespace);
			}

			for(unsigned int n = 0; n < fInfo->paramCount; n++)
			{
				ExternLocalInfo &lInfo = fLocals[fInfo->offsetToFirstLocal + n];
				TypeInfo *currType = CodeInfo::typeInfo[typeRemap[lInfo.type]];
				lastFunc->AddParameter(new VariableInfo(lastFunc, InplaceStr(symbols + lInfo.offsetToName), GetStringHash(symbols + lInfo.offsetToName), 0, currType, false));
				lastFunc->allParamSize += currType->size < 4 ? 4 : currType->size;
				lastFunc->lastParam->defaultValueFuncID = lInfo.defaultFuncId;
				lastFunc->lastParam->isExplicit = !!(lInfo.paramFlags & ExternLocalInfo::IS_EXPLICIT);
			}
			lastFunc->implemented = fInfo->codeSize & 0x80000000 ? false : true;
			lastFunc->funcType = CodeInfo::typeInfo[typeRemap[fInfo->funcType]];

			if(lastFunc->funcType == typeVoid)
			{
				lastFunc->generic = lastFunc->CreateGenericContext(fInfo->genericOffset + int(CodeInfo::lexFullStart - CodeInfo::lexStart));
				lastFunc->generic->parent = lastFunc;
				lastFunc->vTopSize = 1;

				lastFunc->retType = fInfo->genericReturnType != ~0u ? CodeInfo::typeInfo[typeRemap[fInfo->genericReturnType]] : NULL;
				lastFunc->funcType = CodeInfo::GetFunctionType(lastFunc->retType, lastFunc->firstParam, lastFunc->paramCount);
				if(lastFunc->type == FunctionInfo::COROUTINE)
				{
					CodeInfo::nodeList.push_back(new NodeZeroOP());
					lastFunc->afterNode = new NodeExpressionList();
					CodeInfo::funcDefList.push_back(lastFunc->afterNode);
				}
			}else{
				lastFunc->retType = lastFunc->funcType->funcType->retType;
			}
			lastFunc->genericInstance = !!fInfo->isGenericInstance;

			if(fInfo->parentType != ~0u)
			{
				lastFunc->parentClass = CodeInfo::typeInfo[typeRemap[fInfo->parentType]];
				lastFunc->extraParam = new VariableInfo(lastFunc, InplaceStr("this"), GetStringHash("this"), 0, CodeInfo::GetReferenceType(lastFunc->parentClass), false);
			}
			if(fInfo->contextType != ~0u)
			{
				lastFunc->contextType = CodeInfo::typeInfo[typeRemap[fInfo->contextType]];
			}

			assert(lastFunc->funcType->funcType->paramCount == lastFunc->paramCount);

			// Import function explicit type list
			AliasInfo *aliasEnd = NULL;

			for(unsigned k = 0; k < fInfo->explicitTypeCount; k++)
			{
				AliasInfo *info = TypeInfo::CreateAlias(InplaceStr(symbols + vInfo[k].offsetToName), CodeInfo::typeInfo[typeRemap[vInfo[k].type]]);

				if(!lastFunc->explicitTypes)
					lastFunc->explicitTypes = aliasEnd = info;
				else
					aliasEnd->next = info;
				aliasEnd = info;
			}

#ifdef IMPORT_VERBOSE_DEBUG_OUTPUT
			printf(" Importing function %s %s(", lastFunc->retType ? lastFunc->retType->GetFullTypeName() : "generic", lastFunc->name);
			VariableInfo *curr = lastFunc->firstParam;
			for(unsigned int n = 0; n < fInfo->paramCount; n++)
			{
				printf("%s%s %.*s", n == 0 ? "" : ", ", curr->varType->GetFullTypeName(), curr->name.end-curr->name.begin, curr->name.begin);
				curr = curr->next;
			}
			printf(")\r\n");
#endif
		}else if(CodeInfo::funcInfo[index]->name[0] == '$' || CodeInfo::funcInfo[index]->genericInstance){
			CodeInfo::funcInfo.push_back(CodeInfo::funcInfo[index]);
		}else{
			SafeSprintf(errBuf, 256, "ERROR: function %s (type %s) is already defined. While importing %s", CodeInfo::funcInfo[index]->name, CodeInfo::funcInfo[index]->funcType->GetFullTypeName(), pos);
			CodeInfo::lastError.Init(errBuf, NULL);
			return false;
		}

		vInfo += fInfo->explicitTypeCount;
		fInfo++;
	}

	fInfo = FindFirstFunc(bCode);
	for(unsigned int i = 0; i < end; i++)
	{
		FunctionInfo *func = CodeInfo::funcInfo[oldFuncCount + i];
		// Handle only global visible functions
		if(!func->visible || func->type == FunctionInfo::LOCAL || func->type == FunctionInfo::COROUTINE)
			continue;
		// Go through all function parameters
		VariableInfo *param = func->firstParam;
		while(param)
		{
			if(param->defaultValueFuncID != 0xffff)
			{
				FunctionInfo *targetFunc = CodeInfo::funcInfo[oldFuncCount + param->defaultValueFuncID - bCode->moduleFunctionCount];
				AddFunctionCallNode(NULL, targetFunc->name, 0, false);
				param->defaultValue = CodeInfo::nodeList.back();
				CodeInfo::nodeList.pop_back();
			}
			param = param->next;
		}
	}

	return true;
}

bool Compiler::ImportModule(const char* bytecode, const char* pos, unsigned int number)
{
	typeRemap.clear();

#ifdef IMPORT_VERBOSE_DEBUG_OUTPUT
	printf("Importing module %s\r\n", pos);
#endif

	if(!ImportModuleNamespaces(bytecode))
		return false;

	if(!ImportModuleTypes(bytecode, pos))
		return false;

	if(!ImportModuleVariables(bytecode, pos, number))
		return false;

	if(!ImportModuleTypedefs(bytecode))
		return false;

	if(!ImportModuleFunctions(bytecode, pos))
		return false;
	
	return true;
}

char* Compiler::BuildModule(const char* file, const char* altFile)
{
	unsigned int fileSize = 0;
	int needDelete = false;
	bool failedImportPath = false;

	char *fileContent = (char*)NULLC::fileLoad(file, &fileSize, &needDelete);
	if(!fileContent && BinaryCache::GetImportPath())
	{
		failedImportPath = true;
		fileContent = (char*)NULLC::fileLoad(altFile, &fileSize, &needDelete);
	}

	if(fileContent)
	{
		unsigned lexPos = lexer.GetStreamSize();
		if(!Compile(fileContent, true))
		{
			unsigned int currLen = (unsigned int)strlen(CodeInfo::lastError.errLocal);
			SafeSprintf(CodeInfo::lastError.errLocal + currLen, NULLC_ERROR_BUFFER_SIZE - currLen, " [in module %s]", file);

			if(needDelete)
				NULLC::dealloc(fileContent);
			return NULL;
		}
		char *bytecode = NULL;
#ifdef VERBOSE_DEBUG_OUTPUT
		printf("Bytecode for module %s. ", file);
#endif
		GetBytecode(&bytecode);

		if(needDelete)
		{
			const char *newStart = FindSource((ByteCode*)bytecode);
			// We have to fix lexeme positions to the code that is saved in bytecode
			for(Lexeme *c = lexer.GetStreamStart() + lexPos, *e = lexer.GetStreamStart() + lexer.GetStreamSize(); c != e; c++)
			{
				// Exit fix up if lexemes exited scope of the current file
				if(c->pos < fileContent || c->pos > (fileContent + fileSize))
					break;
				c->pos = (c->pos - fileContent) + newStart;
			}
			NULLC::dealloc(fileContent);
		}

		BinaryCache::PutBytecode(failedImportPath ? altFile : file, bytecode, lexer.GetStreamStart() + lexPos, lexer.GetStreamSize() - lexPos);

		return bytecode;
	}else{
		CodeInfo::lastError.Init("", NULL);
		SafeSprintf(CodeInfo::lastError.errLocal, NULLC_ERROR_BUFFER_SIZE, "ERROR: module %s not found", altFile);
	}
	return NULL;
}

#ifdef NULLC_LLVM_SUPPORT
const char *llvmBinary = NULL;
unsigned	llvmSize = 0;
#endif

void Compiler::RecursiveLexify(const char* bytecode)
{
	const char *importPath = BinaryCache::GetImportPath();

	ByteCode *bCode = (ByteCode*)bytecode;

	lexer.Lexify(bytecode + bCode->offsetToSource);

	ExternModuleInfo *mInfo = FindFirstModule(bCode);
	mInfo++;
	for(unsigned int i = 1; i < bCode->dependsCount; i++, mInfo++)
	{
		const char *path = FindSymbols(bCode) + mInfo->nameOffset;

		char fullPath[256];
		SafeSprintf(fullPath, 256, "%s%s", importPath ? importPath : "", path);

		const char *bytecode = BinaryCache::GetBytecode(fullPath);
		unsigned lexCount = 0;
		Lexeme *lexStream = BinaryCache::GetLexems(fullPath, lexCount);
		if(!bytecode)
		{
			bytecode = BinaryCache::GetBytecode(path);
			lexStream = BinaryCache::GetLexems(path, lexCount);
		}
		assert(bytecode);
		if(lexStream)
			lexer.Append(lexStream, lexCount);
		else
			Compiler::RecursiveLexify(bytecode);
	}
}

bool Compiler::Compile(const char* str, bool noClear)
{
	CodeInfo::nodeList.clear();
	NodeZeroOP::DeleteNodes();

	if(!noClear)
	{
		lexer.Clear(baseModuleSize);
		moduleSource.clear();
		dupStringsModule.Clear();
		funcMap.clear();
		importStack.clear();
		moduleStack.clear();
		codeSourceRange.clear();
	}
	unsigned int lexStreamStart = lexer.GetStreamSize();
	lexer.Lexify(str);
	unsigned int lexStreamEnd = lexer.GetStreamSize();

	unsigned moduleBase = moduleStack.size();

	if(BinaryCache::GetBytecode("$base$.nc"))
		moduleStack.push_back(ModuleInfo("$base$.nc", BinaryCache::GetBytecode("$base$.nc"), 0, CodeRange(lexer.GetStreamStart()[0].pos, lexer.GetStreamStart()[baseModuleSize - 1].pos)));

	const char *importPath = BinaryCache::GetImportPath();

	Lexeme *start = &lexer.GetStreamStart()[lexStreamStart];
	while(start->type == lex_import)
	{
		start++;
		char path[256], *cPath = path, *pathNoImport = path;
		Lexeme *name = start;
		if(start->type != lex_string)
		{
			CodeInfo::lastError.Init("ERROR: string expected after import", start->pos);
			return false;
		}
		cPath += SafeSprintf(cPath, 256, "%s%.*s", importPath ? importPath : "", start->length, start->pos);
		if(importPath)
			pathNoImport = path + strlen(importPath);

		start++;
		while(start->type == lex_point)
		{
			start++;
			if(start->type != lex_string)
			{
				CodeInfo::lastError.Init("ERROR: string expected after '.'", start->pos);
				return false;
			}
			cPath += SafeSprintf(cPath, 256 - int(cPath - path), "/%.*s", start->length, start->pos);
			start++;
		}
		if(start->type != lex_semicolon)
		{
			CodeInfo::lastError.Init("ERROR: ';' not found after import expression", name->pos);
			return false;
		}
		start++;
		SafeSprintf(cPath, 256 - int(cPath - path), ".nc");
		const char *bytecode = BinaryCache::GetBytecode(path);
		unsigned lexCount = 0;
		Lexeme *lexStream = BinaryCache::GetLexems(path, lexCount);
		if(!bytecode && importPath)
		{
			bytecode = BinaryCache::GetBytecode(pathNoImport);
			lexStream = BinaryCache::GetLexems(pathNoImport, lexCount);
		}

		moduleStack.push_back(ModuleInfo(strcpy((char*)dupStringsModule.Allocate((unsigned int)strlen(pathNoImport) + 1), pathNoImport), NULL, 0, CodeRange()));
		if(!bytecode)
		{
			unsigned pathHash = GetStringHash(pathNoImport);
			for(unsigned i = 0; i < importStack.size(); i++)
			{
				if(pathHash == importStack[i])
				{
					char buf[512];
					SafeSprintf(buf, 512, "ERROR: found cyclic dependency on module '%s'", pathNoImport);
					CodeInfo::lastError.Init(buf, name->pos);
					return false;
				}
			}
			unsigned int lexPos = (unsigned int)(start - &lexer.GetStreamStart()[lexStreamStart]);
			moduleStack.back().stream = lexer.GetStreamSize();
			importStack.push_back(pathHash);
			bytecode = BuildModule(path, pathNoImport);
			importStack.pop_back();
			start = &lexer.GetStreamStart()[lexStreamStart + lexPos];
			if(bytecode)
				moduleStack.back().range = CodeRange(lexer.GetStreamStart()[moduleStack.back().stream].pos, lexer.GetStreamStart()[moduleStack.back().stream].pos + ((ByteCode*)bytecode)->sourceSize);
		}else{
			unsigned int lexPos = (unsigned int)(start - &lexer.GetStreamStart()[lexStreamStart]);
			moduleStack.back().stream = lexer.GetStreamSize();

			if(lexStream)
				lexer.Append(lexStream, lexCount);
			else
				RecursiveLexify(bytecode);

			start = &lexer.GetStreamStart()[lexStreamStart + lexPos];
			moduleStack.back().range = CodeRange(lexer.GetStreamStart()[moduleStack.back().stream].pos, lexer.GetStreamStart()[lexer.GetStreamSize() - 1].pos);
		}
		if(!bytecode)
			return false;
		moduleStack.back().data = bytecode;
	}

	ClearState();

	activeModules.clear();
	CodeInfo::lexStart = lexer.GetStreamStart();

	codeSourceRange.resize(moduleStack.size() - moduleBase);
	for(unsigned int i = moduleBase; i < moduleStack.size(); i++)
	{
		activeModules.push_back();
		activeModules.back().name = moduleStack[i].name;
		activeModules.back().funcStart = CodeInfo::funcInfo.size();
		CodeInfo::lexFullStart = lexer.GetStreamStart() + moduleStack[i].stream;
		if(!ImportModule(moduleStack[i].data, moduleStack[i].name, i + 1 - moduleBase))
			return false;
		SetGlobalSize(0);
		activeModules.back().funcCount = CodeInfo::funcInfo.size() - activeModules.back().funcStart;
		codeSourceRange[i-moduleBase] = moduleStack[i].range;
	}
	moduleStack.shrink(moduleBase);

	CompilerError::codeStart = str;
	CompilerError::codeEnd = (lexer.GetStreamStart() + lexStreamEnd - 1)->pos;
	CodeInfo::cmdInfoList.SetSourceStart(CompilerError::codeStart, CompilerError::codeEnd);
	CodeInfo::lexFullStart = lexer.GetStreamStart() + lexStreamStart;

	RestoreRedirectionTables();

	bool res;
	if(!setjmp(CodeInfo::errorHandler))
	{
		res = ParseCode(&start);
		if(start->type != lex_none)
		{
			CodeInfo::lastError.Init("ERROR: unexpected symbol", start->pos);
			return false;
		}
	}else{
		return false;
	}
	if(!res)
	{
		CodeInfo::lastError.Init("ERROR: module contains no code", str + strlen(str));
		return false;
	}

#ifdef NULLC_LOG_FILES
	FILE *fGraph = fopen("graph.txt", "wb");
#endif

	if(!setjmp(CodeInfo::errorHandler))
	{
		// wrap all default function arguments in functions
		for(unsigned int i = 0; i < CodeInfo::funcInfo.size(); i++)
		{
			FunctionInfo *func = CodeInfo::funcInfo[i];
			// Handle only global visible functions
			if(!func->visible || func->type == FunctionInfo::LOCAL || func->type == FunctionInfo::COROUTINE)
				continue;
			// Go through all function parameters
			VariableInfo *param = func->firstParam;
			while(param)
			{
				if(param->defaultValue && param->defaultValueFuncID == ~0u)
				{
					// Wrap in function
					char	*functionName = (char*)AllocateString(func->nameLength + param->name.length() + 3 + 12);
					const char	*parentName = func->type == FunctionInfo::THISCALL ? strchr(func->name, ':') + 2 : func->name;
					SafeSprintf(functionName, func->nameLength + param->name.length() + 3 + 12, "$%s_%.*s_%d", parentName, param->name.length(), param->name.begin, CodeInfo::FindFunctionByPtr(func));

					SelectTypeByPointer(param->defaultValue->typeInfo);
					FunctionAdd(CodeInfo::lastKnownStartPos, functionName);
					FunctionStart(CodeInfo::lastKnownStartPos);
					CodeInfo::nodeList.push_back(param->defaultValue);
					AddReturnNode(CodeInfo::lastKnownStartPos);
					FunctionEnd(CodeInfo::lastKnownStartPos);

					param->defaultValueFuncID = CodeInfo::FindFunctionByPtr(CodeInfo::funcInfo.back());
					AddTwoExpressionNode(NULL);
				}
				param = param->next;
			}
		}
	}else{
#ifdef NULLC_LOG_FILES
		fclose(fGraph);
#endif
		return false;
	}

	CreateRedirectionTables();

	realGlobalCount = CodeInfo::varInfo.size();

	RestoreScopedGlobals();

#ifdef NULLC_LLVM_SUPPORT
	unsigned functionsInModules = 0;
	for(unsigned i = 0; i < activeModules.size(); i++)
		functionsInModules += activeModules[i].funcCount;
	StartLLVMGeneration(functionsInModules);
#endif

	CodeInfo::cmdList.push_back(VMCmd(cmdJmp));
	unsigned coroutineContext = 0;
	for(unsigned int i = 0; i < CodeInfo::funcDefList.size(); i++)
	{
		if(CodeInfo::funcDefList[i]->nodeType != typeNodeFuncDef)
		{
			coroutineContext++;
			continue;
		}
		CodeInfo::funcDefList[i]->Compile();
#ifdef NULLC_LOG_FILES
		CodeInfo::funcDefList[i]->LogToStream(fGraph);
#endif
#ifdef NULLC_LLVM_SUPPORT
		CodeInfo::funcDefList[i]->CompileLLVM();
#endif
		((NodeFuncDef*)CodeInfo::funcDefList[i])->Disable();
	}
	CodeInfo::cmdList[0].argument = CodeInfo::cmdList.size();
	for(unsigned int i = 0; i < coroutineContext; i++)
	{
		CodeInfo::funcDefList[i]->Compile();
#ifdef NULLC_LOG_FILES
		CodeInfo::funcDefList[i]->LogToStream(fGraph);
#endif
	}
	if(CodeInfo::nodeList.back())
	{
		CodeInfo::nodeList.back()->Compile();
#ifdef NULLC_LOG_FILES
		CodeInfo::nodeList.back()->LogToStream(fGraph);
#endif
#ifdef NULLC_LLVM_SUPPORT
		StartGlobalCode();
		CodeInfo::nodeList.back()->CompileLLVM();
#endif
	}
#ifdef NULLC_LOG_FILES
	fclose(fGraph);
#endif
#ifdef NULLC_LLVM_SUPPORT
	llvmBinary = GetLLVMIR(llvmSize);

	EndLLVMGeneration();
#endif
	if(CodeInfo::nodeList.size() != 1)
	{
		CodeInfo::lastError.Init("Compilation failed, AST contains more than one node", NULL);
		return false;
	}

	return true;
}

const char* Compiler::GetError()
{
	return CodeInfo::lastError.GetErrorString();
}

void Compiler::SaveListing(const char *fileName)
{
	FILE *compiledAsm = fopen(fileName, "wb");
	char instBuf[128];
	int line = 0, lastLine = ~0u;
	const char *lastSourcePos = CompilerError::codeStart;
	for(unsigned int i = 0; i < CodeInfo::cmdList.size(); i++)
	{
		while((line < (int)CodeInfo::cmdInfoList.sourceInfo.size() - 1) && (i >= CodeInfo::cmdInfoList.sourceInfo[line + 1].byteCodePos))
			line++;
		if(CodeInfo::cmdInfoList.sourceInfo.size() && line != lastLine)
		{
			lastLine = line;
			const char *codeStart = CodeInfo::cmdInfoList.sourceInfo[line].sourcePos;
			// Find beginning of the line
			while(codeStart != CodeInfo::cmdInfoList.sourceStart && *(codeStart-1) != '\n')
				codeStart--;
			// Skip whitespace
			while(*codeStart == ' ' || *codeStart == '\t')
				codeStart++;
			const char *codeEnd = codeStart;
			while(*codeEnd != '\0' && *codeEnd != '\r' && *codeEnd != '\n')
				codeEnd++;
			if(codeEnd > lastSourcePos)
			{
				fprintf(compiledAsm, "%.*s\r\n", codeEnd - lastSourcePos, lastSourcePos);
				lastSourcePos = codeEnd;
			}else{
				fprintf(compiledAsm, "%.*s\r\n", codeEnd - codeStart, codeStart);
			}
		}
		CodeInfo::cmdList[i].Decode(instBuf);
		if(CodeInfo::cmdList[i].cmd == cmdCall)
			fprintf(compiledAsm, "// %d %s (%s)\r\n", i, instBuf, CodeInfo::funcInfo[CodeInfo::cmdList[i].argument]->name);
		else
			fprintf(compiledAsm, "// %d %s\r\n", i, instBuf);
	}
	fclose(compiledAsm);
}

void Compiler::TranslateToC(const char* fileName, const char *mainName)
{
#ifdef NULLC_ENABLE_C_TRANSLATION
	FILE *fC = fopen(fileName, "wb");

	// Save all the types we need to translate
	translationTypes.clear();
	translationTypes.resize(CodeInfo::typeInfo.size());
	for(unsigned int i = 0; i < CodeInfo::typeInfo.size(); i++)
		translationTypes[i] = NULL;

	// Sort them in their original order of definition
	for(unsigned int i = buildInTypes.size(); i < CodeInfo::typeInfo.size(); i++)
		translationTypes[CodeInfo::typeInfo[i]->originalIndex] = CodeInfo::typeInfo[i];

	for(unsigned int k = 0; k < translationTypes.size(); k++)
	{
		TypeInfo *type = translationTypes[k];
		if(!type || type->arrSize == TypeInfo::UNSIZED_ARRAY || type->refLevel || type->funcType || type->arrLevel)
			continue;
		TypeInfo::MemberVariable *curr = type->firstVariable;
		for(; curr; curr = curr->next)
		{
			if(curr->type->originalIndex > type->originalIndex)
			{
				printf("%s (%d) depends on %s (%d), but the latter is defined later\n", type->GetFullTypeName(), type->originalIndex, curr->type->GetFullTypeName(), curr->type->originalIndex);
				unsigned index = curr->type->originalIndex;
				for(unsigned int i = type->originalIndex + 1; i <= curr->type->originalIndex; i++)
					translationTypes[i]->originalIndex--;
				type->originalIndex = index;
				printf("%s (%d) depends on %s (%d)\n", type->GetFullTypeName(), type->originalIndex, curr->type->GetFullTypeName(), curr->type->originalIndex);
			}
		}
	}
	for(unsigned int i = buildInTypes.size(); i < CodeInfo::typeInfo.size(); i++)
		translationTypes[CodeInfo::typeInfo[i]->originalIndex] = CodeInfo::typeInfo[i];

	ByteCode* runtimeModule = (ByteCode*)BinaryCache::GetBytecode("$base$.nc");

	fprintf(fC, "#include \"runtime.h\"\r\n");
	fprintf(fC, "// Typeid redirect table\r\n");
	fprintf(fC, "static unsigned __nullcTR[%d];\r\n", CodeInfo::typeInfo.size());
	fprintf(fC, "// Function pointer table\r\n");
	fprintf(fC, "static __nullcFunctionArray* __nullcFM;\r\n");
	fprintf(fC, "// Function pointer redirect table\r\n");
	fprintf(fC, "static unsigned __nullcFR[%d];\r\n", CodeInfo::funcInfo.size());
	fprintf(fC, "// Function pointers, arrays, classes\r\n");
	fprintf(fC, "#pragma pack(push, 4)\r\n");	
	for(unsigned int i = buildInTypes.size(); i < CodeInfo::typeInfo.size(); i++)
	{
		TypeInfo *type = translationTypes[i];
		if(type->funcType)
		{
			fprintf(fC, "struct __typeProxy_");
			type->WriteCNameEscaped(fC, type->GetFullTypeName());
			fprintf(fC, "{};\r\n");
			continue;
		}
		if(!type || type->arrSize == TypeInfo::UNSIZED_ARRAY || type->refLevel || i < runtimeModule->typeCount)
			continue;
		if(type->arrLevel)
		{
			fprintf(fC, "struct ");
			type->OutputCType(fC, "");
			fprintf(fC, "\r\n{\r\n\t");
			type->subType->OutputCType(fC, "ptr");
			fprintf(fC, "[%d];\r\n\t", type->arrSize + ((4 - (type->arrSize * type->subType->size % 4)) & 3));
			type->OutputCType(fC, "");
			fprintf(fC, "& set(unsigned index, ");
			if(type->subType->arrLevel && type->subType->arrSize != TypeInfo::UNSIZED_ARRAY && type->subType->subType == typeChar)
			{
				fprintf(fC, "const char* val){ memcpy(ptr, val, %d); return *this; }\r\n", type->subType->arrSize);
			}else{
				type->subType->OutputCType(fC, "const &");
				fprintf(fC, " val){ ptr[index] = val; return *this; }\r\n");
			}
			if(type->arrLevel && type->arrSize != TypeInfo::UNSIZED_ARRAY && type->subType == typeChar)
			{
				fprintf(fC, "\t");
				type->OutputCType(fC, "");
				fprintf(fC, "(){}\r\n\t");
				type->OutputCType(fC, "");
				fprintf(fC, "(const char* data){ memcpy(ptr, data, %d); }\r\n", type->arrSize);
			}
			fprintf(fC, "};\r\n");
		}else{
			fprintf(fC, "struct ");
			type->OutputCType(fC, "\r\n");
			unsigned countMembers = 0;
			if(type->name[0] == '_' && type->name[1] == '_' && 0 == memcmp("_cls", type->fullName + type->fullNameLength - 4, 5))
				countMembers = 1;
			fprintf(fC, "{\r\n");
			TypeInfo::MemberVariable *curr = type->firstVariable;
			for(; curr; curr = curr->next)
			{
				fprintf(fC, "\t");
				if(strchr(type->name, ':') && curr == type->firstVariable)
				{
					fprintf(fC, "void *%s", curr->name);
				}else{
					curr->type->OutputCType(fC, curr->name);
				}
				if(countMembers)
				{
					if(countMembers > 2)
						fprintf(fC, "%d", countMembers);
					countMembers++;
				}
				fprintf(fC, ";\r\n");
			}
			fprintf(fC, "};\r\n");
		}
	}
	fprintf(fC, "#pragma pack(pop)\r\n");

	unsigned int functionsInModules = 0;
	for(unsigned int i = 0; i < activeModules.size(); i++)
		functionsInModules += activeModules[i].funcCount;
	for(unsigned int i = functionsInModules; i < CodeInfo::funcInfo.size(); i++)
	{
		char name[NULLC_MAX_VARIABLE_NAME_LENGTH];
		FunctionInfo *info = CodeInfo::funcInfo[i];
		VariableInfo *local = info->firstParam;
		for(; local; local = local->next)
		{
			if(local->usedAsExternal)
			{
				const char *namePrefix = *local->name.begin == '$' ? "__" : "";
				unsigned int nameShift = *local->name.begin == '$' ? 1 : 0;
				sprintf(name, "%s%.*s_%d", namePrefix, local->name.length() - nameShift, local->name.begin + nameShift, local->pos);
				GetEscapedName(name);
				fprintf(fC, "__nullcUpvalue *__upvalue_%d_%s = 0;\r\n", CodeInfo::FindFunctionByPtr(local->parentFunction), name);
			}
		}
		if((info->type == FunctionInfo::LOCAL || info->type == FunctionInfo::COROUTINE) && info->closeUpvals)
		{
			fprintf(fC, "__nullcUpvalue *__upvalue_%d___", CodeInfo::FindFunctionByPtr(info));
			FunctionInfo::FunctionCategory cat = info->type;
			info->type = FunctionInfo::NORMAL;
			info->visible = true;
			OutputCFunctionName(fC, info);
			info->type = cat;
			info->visible = false;
			fprintf(fC, "_%d_ext_%d = 0;\r\n", CodeInfo::FindFunctionByPtr(info), info->allParamSize);
		}else if(info->type == FunctionInfo::THISCALL && info->closeUpvals){
			fprintf(fC, "__nullcUpvalue *__upvalue_%d___context = 0;\r\n", CodeInfo::FindFunctionByPtr(info));
		}
		local = info->firstLocal;
		for(; local; local = local->next)
		{
			if(local->usedAsExternal)
			{
				const char *namePrefix = *local->name.begin == '$' ? "__" : "";
				unsigned int nameShift = *local->name.begin == '$' ? 1 : 0;
				sprintf(name, "%s%.*s_%d", namePrefix, local->name.length() - nameShift, local->name.begin + nameShift, local->pos);
				GetEscapedName(name);
				if(info->name[0] == '$')
					fprintf(fC, "static ");
				fprintf(fC, "__nullcUpvalue *__upvalue_%d_%s = 0;\r\n", CodeInfo::FindFunctionByPtr(local->parentFunction), name);
			}
		}
	}
	for(unsigned int i = activeModules[0].funcCount; i < CodeInfo::funcInfo.size(); i++)
	{
		FunctionInfo *info = CodeInfo::funcInfo[i];
		if(i < functionsInModules && (info->type == FunctionInfo::LOCAL))
			continue;
		if(!info->retType)
			continue;
		if(i < functionsInModules)
			fprintf(fC, "extern ");
		else if(strcmp(info->name, "$gen_list") == 0 || memcmp(info->name, "$genl", 5) == 0)
			fprintf(fC, "static ");
		info->retType->OutputCType(fC, "");

		OutputCFunctionName(fC, info);
		fprintf(fC, "(");

		char name[NULLC_MAX_VARIABLE_NAME_LENGTH];
		VariableInfo *param = info->firstParam;
		for(; param; param = param->next)
		{
			sprintf(name, "%.*s", param->name.length(), param->name.begin);
			param->varType->OutputCType(fC, name);
			fprintf(fC, ", ");
		}
		if(info->type == FunctionInfo::THISCALL && info->parentClass)
		{
			info->parentClass->OutputCType(fC, "* __context");
		}else if((info->type == FunctionInfo::LOCAL || info->type == FunctionInfo::COROUTINE)){
			fprintf(fC, "void* __");
			OutputCFunctionName(fC, info);
			fprintf(fC, "_ext");
		}else{
			fprintf(fC, "void* unused");
		}
		fprintf(fC, ");\r\n");
	}
	for(unsigned int i = 0; i < CodeInfo::varInfo.size(); i++)
	{
		VariableInfo *varInfo = CodeInfo::varInfo[i];
		char vName[NULLC_MAX_VARIABLE_NAME_LENGTH];
		const char *namePrefix = varInfo->name.begin[0] == '$' ? (varInfo->name.begin[1] == '$' ? "___" : "__") : "";
		unsigned int nameShift = varInfo->name.begin[0] == '$' ? (varInfo->name.begin[1] == '$' ? 2 : 1) : 0;
		sprintf(vName, varInfo->blockDepth > 1 ? "%s%.*s_%d" : "%s%.*s", namePrefix, int(varInfo->name.end-varInfo->name.begin)-nameShift, varInfo->name.begin+nameShift, varInfo->pos);
		GetEscapedName(vName);
		if(varInfo->pos >> 24)
			fprintf(fC, "extern ");
		else if(*varInfo->name.begin == '$' && 0 != strcmp(varInfo->name.end-3, "ext") && 0 != memcmp(varInfo->name.begin, "$vtbl", 5))
			fprintf(fC, "static ");
		varInfo->varType->OutputCType(fC, vName);
		fprintf(fC, ";\r\n");
	}

	for(unsigned int i = 0; i < CodeInfo::funcDefList.size(); i++)
		((NodeFuncDef*)CodeInfo::funcDefList[i])->Enable();

	for(unsigned int i = 0; i < CodeInfo::funcDefList.size(); i++)
	{
		CodeInfo::funcDefList[i]->TranslateToC(fC);
		((NodeFuncDef*)CodeInfo::funcDefList[i])->Disable();
	}

	for(unsigned int i = 1; i < activeModules.size(); i++)
	{
		fprintf(fC, "extern int __init_");
		unsigned int len = (unsigned int)strlen(activeModules[i].name);
		for(unsigned int k = 0; k < len; k++)
		{
			if(activeModules[i].name[k] == '/' || activeModules[i].name[k] == '.')
				fprintf(fC, "_");
			else
				fprintf(fC, "%c", activeModules[i].name[k]);
		}
		fprintf(fC, "();\r\n");
	}
	if(CodeInfo::nodeList.back())
	{
		fprintf(fC, "int %s()\r\n{\r\n", mainName);
		fprintf(fC, "\tstatic int moduleInitialized = 0;\r\n\tif(moduleInitialized++)\r\n\t\treturn 0;\r\n");
		fprintf(fC, "\t__nullcFM = __nullcGetFunctionTable();\r\n");
		fprintf(fC, "\tint __local = 0;\r\n\t__nullcRegisterBase((void*)&__local);\r\n");
		fprintf(fC, "\t__nullcInitBaseModule();\r\n");
		for(unsigned int i = 1; i < activeModules.size(); i++)
		{
			fprintf(fC, "\t__init_");
			unsigned int len = (unsigned int)strlen(activeModules[i].name);
			for(unsigned int k = 0; k < len; k++)
			{
				if(activeModules[i].name[k] == '/' || activeModules[i].name[k] == '.')
					fprintf(fC, "_");
				else
					fprintf(fC, "%c", activeModules[i].name[k]);
			}
			fprintf(fC, "();\r\n");
		}
		for(unsigned int i = 0; i < CodeInfo::typeInfo.size(); i++)
		{
			TypeInfo *type = CodeInfo::typeInfo[i];
			fprintf(fC, "\t__nullcTR[%d] = __nullcRegisterType(", i);
			fprintf(fC, "%uu, ", type->GetFullNameHash());
			fprintf(fC, "\"%s\", ", type->GetFullTypeName());
			fprintf(fC, "%d, ", type->size);
			fprintf(fC, "__nullcTR[%d], ", type->subType ? type->subType->typeIndex : 0);
			if(type->arrLevel)
				fprintf(fC, "%d, NULLC_ARRAY);\r\n", type->arrSize);
			else if(type->refLevel)
				fprintf(fC, "%d, NULLC_POINTER);\r\n", 1);
			else if(type->funcType)
				fprintf(fC, "%d, NULLC_FUNCTION);\r\n", type->funcType->paramCount);
			else if(i >= 7) // 7 = { void, char, short, int, long, float, double }.length
				fprintf(fC, "%d, NULLC_CLASS);\r\n", type->memberCount);
			else
				fprintf(fC, "%d, 0);\r\n", type->memberCount);
		}
		for(unsigned int i = 0; i < CodeInfo::typeInfo.size(); i++)
		{
			TypeInfo *type = CodeInfo::typeInfo[i];
			if(type->arrLevel)
				continue;
			else if(type->refLevel)
				continue;
			else if(type->funcType)
				continue;
			else if(i >= 7){ // 7 = { void, char, short, int, long, float, double }.length
				fprintf(fC, "\t__nullcRegisterMembers(__nullcTR[%d], %d", i, type->memberCount);
				for(TypeInfo::MemberVariable *curr = type->firstVariable; curr; curr = curr->next)
				{
					fprintf(fC, ", __nullcTR[%d]", curr->type->typeIndex);
					fprintf(fC, ", %d", curr->offset);
				}
				fprintf(fC, ");\r\n");
			}
		}
		for(unsigned int i = 0; i < CodeInfo::varInfo.size(); i++)
		{
			VariableInfo *varInfo = CodeInfo::varInfo[i];
			if(varInfo->pos >> 24)
				continue;
			char vName[NULLC_MAX_VARIABLE_NAME_LENGTH];
			const char *namePrefix = varInfo->name.begin[0] == '$' ? "__" : "";
			unsigned int nameShift = varInfo->name.begin[0] == '$' ? 1 : 0;
			sprintf(vName, varInfo->blockDepth > 1 ? "%s%.*s_%d" : "%s%.*s", namePrefix, int(varInfo->name.end-varInfo->name.begin)-nameShift, varInfo->name.begin+nameShift, varInfo->pos);
			GetEscapedName(vName);
			fprintf(fC, "\t__nullcRegisterGlobal((void*)&%s, __nullcTR[%d]);\r\n", vName, varInfo->varType->typeIndex);
		}
		for(unsigned int i = 0; i < CodeInfo::funcInfo.size(); i++)
		{
			char fName[NULLC_MAX_VARIABLE_NAME_LENGTH + 32];
			GetCFunctionName(fName, NULLC_MAX_VARIABLE_NAME_LENGTH + 32, CodeInfo::funcInfo[i]);
			CodeInfo::funcInfo[i]->nameInCHash = GetStringHash(fName);
		}
		for(unsigned int i = 0; i < CodeInfo::funcInfo.size(); i++)
		{
			FunctionInfo *info = CodeInfo::funcInfo[i];
			if(i < functionsInModules && (info->type == FunctionInfo::LOCAL || info->type == FunctionInfo::COROUTINE))
				continue;
			bool duplicate = false;
			for(unsigned int l = 0; l < CodeInfo::funcInfo.size() && !duplicate; l++)
			{
				if(i == l)
					continue;
				bool functionIsPrototype = ((info->address & 0x80000000) && (info->address != -1));
				if(info->nameInCHash == CodeInfo::funcInfo[l]->nameInCHash && !((CodeInfo::funcInfo[l]->address & 0x80000000) && (CodeInfo::funcInfo[l]->address != -1)) && !functionIsPrototype)
					duplicate = true;
			}
			if(duplicate)
			{
				fprintf(fC, "\t__nullcFR[%d] = 0;\r\n", i);
			}else{
				fprintf(fC, "\t__nullcFR[%d] = __nullcRegisterFunction(\"", i);
				OutputCFunctionName(fC, info);
				fprintf(fC, "\", (void*)");
				OutputCFunctionName(fC, info);
				fprintf(fC, ", %uu, %d);\r\n", info->extraParam ? info->extraParam->varType->typeIndex : -1, (int)info->type);
			}
		}
		CodeInfo::nodeList.back()->TranslateToC(fC);
		fprintf(fC, "}\r\n");
	}
	fclose(fC);
#else
	(void)fileName;
	(void)mainName;
#endif
}

bool CreateExternalInfo(ExternFuncInfo &fInfo, FunctionInfo &refFunc)
{
	fInfo.bytesToPop = NULLC_PTR_SIZE;
	for(VariableInfo *curr = refFunc.firstParam; curr; curr = curr->next)
	{
		unsigned int paramSize = curr->varType->size > 4 ? curr->varType->size : 4;
		fInfo.bytesToPop += paramSize;
	}

	unsigned int rCount = 0, fCount = 0;
	unsigned int rMaxCount = sizeof(fInfo.rOffsets) / sizeof(fInfo.rOffsets[0]);
	unsigned int fMaxCount = sizeof(fInfo.fOffsets) / sizeof(fInfo.fOffsets[0]);

	// parse all parameters, fill offsets
	unsigned int offset = 0;

	fInfo.ps3Callable = 1;

	for(VariableInfo *curr = refFunc.firstParam; curr; curr = curr->next)
	{
		TypeInfo& type = *curr->varType;

		TypeInfo::TypeCategory oldCategory = type.type;
		if(type.type == TypeInfo::TYPE_COMPLEX)
		{
			if(type.size <= 4)
				type.type = TypeInfo::TYPE_INT;
			else if(type.size <= 8)
				type.type = TypeInfo::TYPE_LONG;
		}
		if(fCount >= fMaxCount || rCount >= rMaxCount) // too many f/r parameters
		{
			fInfo.ps3Callable = 0;
			break;
		}
		switch(type.type)
		{
		case TypeInfo::TYPE_CHAR:
		case TypeInfo::TYPE_SHORT:
		case TypeInfo::TYPE_INT:
		case TypeInfo::TYPE_LONG:
			if(type.refLevel)
				fInfo.rOffsets[rCount++] = offset | 2u << 30u;
			else
				fInfo.rOffsets[rCount++] = offset | (type.type == TypeInfo::TYPE_LONG ? 1u : 0u) << 30u;
			offset += type.type == TypeInfo::TYPE_LONG ? 2 : 1;
			break;

		case TypeInfo::TYPE_FLOAT:
		case TypeInfo::TYPE_DOUBLE:
			fInfo.rOffsets[rCount++] = offset;
			fInfo.fOffsets[fCount++] = offset | (type.type == TypeInfo::TYPE_FLOAT ? 1u : 0u) << 31u;
			offset += type.type == TypeInfo::TYPE_DOUBLE ? 2 : 1;
			break;

		default:
			if(rCount + type.size / 8 >= rMaxCount)
			{
				fInfo.ps3Callable = 0;
				break;
			}
			for(unsigned i = 0; i < type.size / 8; i++)
			{
				fInfo.rOffsets[rCount++] = offset | 1u << 30u;
				offset += 2;
			}
			break;
		}
		type.type = oldCategory;
	}
	if(refFunc.type != FunctionInfo::NORMAL)
	{
		if(fCount >= fMaxCount || rCount >= rMaxCount) // too many f/r parameters
		{
			fInfo.ps3Callable = 0;
		}else{
			fInfo.rOffsets[rCount++] = offset | 2u << 30u;
			offset += 1;
		}
	}

	// clear remaining offsets
	for(unsigned int i = rCount; i < rMaxCount; ++i)
		fInfo.rOffsets[i] = 0;
	for(unsigned int i = fCount; i < fMaxCount; ++i)
		fInfo.fOffsets[i] = 0;

	return true;
}

unsigned int Compiler::GetBytecode(char **bytecode)
{
	// find out the size of generated bytecode
	unsigned int size = sizeof(ByteCode);

	size += CodeInfo::typeInfo.size() * sizeof(ExternTypeInfo);

	unsigned symbolStorageSize = 0;
	unsigned allMemberCount = 0, allConstantCount = 0;
	unsigned typedefCount = 0;
	for(unsigned int i = 0; i < CodeInfo::typeInfo.size(); i++)
	{
		TypeInfo *type = CodeInfo::typeInfo[i];
		AliasInfo *typeAlias = type->childAlias;
		while(typeAlias)
		{
			typedefCount++;
			symbolStorageSize += typeAlias->name.length() + 1;
			typeAlias = typeAlias->next;
		}

		symbolStorageSize += type->GetFullNameLength() + 1;
		if((type->type == TypeInfo::TYPE_COMPLEX || type->firstVariable) && type->subType == NULL && type->funcType == NULL)
		{
			for(TypeInfo::MemberVariable *curr = type->firstVariable; curr; curr = curr->next)
			{
				symbolStorageSize += (unsigned int)strlen(curr->name) + 1;
				if(curr->type->hasPointers)
					allMemberCount += 2;
				if(curr->defaultValue)
					allConstantCount++;
			}
		}
		allMemberCount += type->funcType ? type->funcType->paramCount + 1 : type->memberCount;
	}
	size += allMemberCount * sizeof(ExternMemberInfo);

	unsigned int offsetToConstants = size;
	size += allConstantCount * sizeof(ExternConstantInfo);

	unsigned int functionsInModules = 0;
	unsigned int offsetToModule = size;
	size += activeModules.size() * sizeof(ExternModuleInfo);
	for(unsigned int i = 0; i < activeModules.size(); i++)
	{
		functionsInModules += activeModules[i].funcCount;
		symbolStorageSize += (unsigned int)strlen(activeModules[i].name) + 1;
	}

	unsigned int offsetToVar = size;
	unsigned int globalVarCount = 0, exportVarCount = 0;
	unsigned int externVariableInfoCount = 0;
	for(unsigned int i = 0; i < CodeInfo::varInfo.size(); i++)
	{
		VariableInfo *curr = CodeInfo::varInfo[i];

		if(curr->pos >> 24)
			continue;

		externVariableInfoCount++;

		globalVarCount++;
		if(i < realGlobalCount)
			exportVarCount++;

		symbolStorageSize += curr->name.length() + 1;
	}
	for(unsigned int i = 0; i < CodeInfo::funcInfo.size(); i++)
	{
		if(i < functionsInModules)
			continue;

		FunctionInfo *func = CodeInfo::funcInfo[i];

		AliasInfo *explicitType = func->explicitTypes;
		while(explicitType)
		{
			externVariableInfoCount++;

			symbolStorageSize += explicitType->name.length() + 1;

			explicitType = explicitType->next;
		}
	}
	size += externVariableInfoCount * sizeof(ExternVarInfo);

	unsigned int offsetToFunc = size;

	unsigned int clsListCount = 0;
	unsigned int localCount = 0;
	for(unsigned int i = 0; i < CodeInfo::funcInfo.size(); i++)
	{
		if(functionsInModules && i < functionsInModules)
			continue;
		
		FunctionInfo	*func = CodeInfo::funcInfo[i];
		symbolStorageSize += func->nameLength + 1;

		if(func->closeUpvals)
		{
			clsListCount += func->maxBlockDepth;
			assert(func->maxBlockDepth > 0);
		}

		localCount += (unsigned int)func->paramCount;
		for(VariableInfo *curr = func->firstParam; curr; curr = curr->next)
			symbolStorageSize += curr->name.length() + 1;

		localCount += (unsigned int)func->localCount;
		for(VariableInfo *curr = func->firstLocal; curr; curr = curr->next)
			symbolStorageSize += curr->name.length() + 1;

		localCount += (unsigned int)func->externalCount;
		for(FunctionInfo::ExternalInfo *curr = func->firstExternal; curr; curr = curr->next)
			symbolStorageSize += curr->variable->name.length() + 1;
	}
	size += (CodeInfo::funcInfo.size() - functionsInModules) * sizeof(ExternFuncInfo);

	unsigned int offsetToFirstLocal = size;

	size += localCount * sizeof(ExternLocalInfo);

	size += clsListCount * sizeof(unsigned int);

	AliasInfo *aliasInfo = CodeInfo::globalAliases;
	while(aliasInfo)
	{
		typedefCount++;
		symbolStorageSize += aliasInfo->name.length() + 1;
		aliasInfo = aliasInfo->next;
	}
	unsigned offsetToTypedef = size;
	size += typedefCount * sizeof(ExternTypedefInfo);

	for(unsigned i = 0; i < CodeInfo::namespaceInfo.size(); i++)
		symbolStorageSize += CodeInfo::namespaceInfo[i]->name.length() + 1;
	unsigned offsetToNamespace = size;
	size += CodeInfo::namespaceInfo.size() * sizeof(ExternNamespaceInfo);

	unsigned int offsetToCode = size;
	size += CodeInfo::cmdList.size() * sizeof(VMCmd);

	unsigned int offsetToInfo = size;
	size += sizeof(unsigned int) * 2 * CodeInfo::cmdInfoList.sourceInfo.size();

	unsigned int offsetToSymbols = size;
	size += symbolStorageSize;

	unsigned int sourceLength = (unsigned int)strlen(CodeInfo::cmdInfoList.sourceStart) + 1;
	unsigned int offsetToSource = size;
	size += sourceLength;

#ifdef NULLC_LLVM_SUPPORT
	unsigned int offsetToLLVM = size;
	size += llvmSize;
#endif
	
#ifdef VERBOSE_DEBUG_OUTPUT
	printf("Statistics. Overall: %d bytes\r\n", size);
	printf("Types: %db, ", offsetToModule - sizeof(ByteCode));
	printf("Modules: %db, ", offsetToVar - offsetToModule);
	printf("Variables: %db, ", offsetToFunc - offsetToVar);
	printf("Functions: %db\r\n", offsetToFirstLocal - offsetToFunc);
	printf("Locals: %db, ", offsetToCode - offsetToFirstLocal);
	printf("Code: %db, ", offsetToInfo - offsetToCode);
	printf("Code info: %db, ", offsetToSymbols - offsetToInfo);
	printf("Symbols: %db, ", offsetToSource - offsetToSymbols);
	printf("Source: %db\r\n\r\n", size - offsetToSource);
	printf("%d closure lists\r\n", clsListCount);
#endif
	*bytecode = new char[size];
	memset(*bytecode, 0, size);

	ByteCode	*code = (ByteCode*)(*bytecode);
	code->size = size;

	code->typeCount = (unsigned int)CodeInfo::typeInfo.size();

	code->dependsCount = activeModules.size();
	code->offsetToFirstModule = offsetToModule;

	code->globalVarSize = GetGlobalSize();
	// Make sure modules that are linked together will not break global variable alignment
	code->globalVarSize = (code->globalVarSize + 0xf) & ~0xf;

	code->variableCount = globalVarCount;
	code->variableExportCount = exportVarCount;
	code->offsetToFirstVar = offsetToVar;

	code->functionCount = (unsigned int)CodeInfo::funcInfo.size();
	code->moduleFunctionCount = functionsInModules;
	code->offsetToFirstFunc = offsetToFunc;

	code->localCount = localCount;
	code->offsetToLocals = offsetToFirstLocal;

	code->closureListCount = clsListCount;

	code->codeSize = CodeInfo::cmdList.size();
	code->offsetToCode = offsetToCode;

	code->symbolLength = symbolStorageSize;
	code->offsetToSymbols = offsetToSymbols;

	code->offsetToConstants = offsetToConstants;

	code->namespaceCount = CodeInfo::namespaceInfo.size();
	code->offsetToNamespaces = offsetToNamespace;

	char *debugSymbols = FindSymbols(code);
	char *symbolPos = debugSymbols;

	ExternTypeInfo *tInfo = FindFirstType(code);
	ExternMemberInfo *memberList = FindFirstMember(code);
	ExternConstantInfo *constantList = FindFirstConstant(code);

	unsigned int memberOffset = 0;
	for(unsigned int i = 0; i < CodeInfo::typeInfo.size(); i++)
	{
		ExternTypeInfo &typeInfo = *tInfo;
		TypeInfo &refType = *CodeInfo::typeInfo[i];

		typeInfo.offsetToName = int(symbolPos - debugSymbols);
		memcpy(symbolPos, refType.GetFullTypeName(), refType.GetFullNameLength() + 1);
		symbolPos += refType.GetFullNameLength() + 1;

		typeInfo.size = refType.size;
		typeInfo.padding = refType.paddingBytes;
		typeInfo.type = (ExternTypeInfo::TypeCategory)refType.type;
		typeInfo.nameHash = refType.GetFullNameHash();
		typeInfo.namespaceHash = refType.parentNamespace ? refType.parentNamespace->hash : ~0u;

		typeInfo.defaultAlign = (unsigned char)refType.alignBytes;
		typeInfo.typeFlags = refType.hasFinalizer ? ExternTypeInfo::TYPE_HAS_FINALIZER : 0;
		typeInfo.typeFlags |= refType.dependsOnGeneric ? ExternTypeInfo::TYPE_DEPENDS_ON_GENERIC : 0;
		typeInfo.typeFlags |= (refType.firstVariable && refType.firstVariable->nameHash == GetStringHash("$typeid")) ? ExternTypeInfo::TYPE_IS_EXTENDABLE : 0;

		typeInfo.constantCount = 0;

		typeInfo.pointerCount = refType.hasPointers;
		if(refType.funcType != 0)						// Function type
		{
			typeInfo.subCat = ExternTypeInfo::CAT_FUNCTION;
			typeInfo.memberCount = refType.funcType->paramCount;
			typeInfo.memberOffset = memberOffset;

			memberList[memberOffset].type = refType.funcType->retType->typeIndex;
			memberList[memberOffset].offset = 0;
			memberOffset++;

			for(unsigned int k = 0; k < refType.funcType->paramCount; k++)
			{
				memberList[memberOffset].type = refType.funcType->paramType[k]->typeIndex;
				memberList[memberOffset].offset = 0;
				memberOffset++;
			}
		}else if(refType.arrLevel != 0){				// Array type
			typeInfo.subCat = ExternTypeInfo::CAT_ARRAY;
			typeInfo.arrSize = refType.arrSize;
			typeInfo.subType = refType.subType->typeIndex;
		}else if(refType.refLevel != 0){				// Pointer type
			typeInfo.subCat = ExternTypeInfo::CAT_POINTER;
			typeInfo.subType = refType.subType->typeIndex;
			typeInfo.memberCount = 0;
		}else if(refType.type == TypeInfo::TYPE_COMPLEX || refType.firstVariable){	// Complex type
			typeInfo.subCat = ExternTypeInfo::CAT_CLASS;
			typeInfo.memberCount = refType.memberCount;
			typeInfo.memberOffset = memberOffset;
			// Export type members
			for(TypeInfo::MemberVariable *curr = refType.firstVariable; curr; curr = curr->next)
			{
				if(curr->defaultValue)
					continue;

				memberList[memberOffset].type = curr->type->typeIndex;
				memberList[memberOffset].offset = curr->offset;
				memberOffset++;

				memcpy(symbolPos, curr->name, strlen(curr->name) + 1);
				symbolPos += strlen(curr->name) + 1;
			}
			// Export type constants
			for(TypeInfo::MemberVariable *curr = refType.firstVariable; curr; curr = curr->next)
			{
				if(!curr->defaultValue)
					continue;
				typeInfo.constantCount++;
				assert(curr->defaultValue->nodeType == typeNodeNumber);
				constantList->type = curr->defaultValue->typeInfo->typeIndex;
				constantList->value = ((NodeNumber*)curr->defaultValue)->num.integer64;
				constantList++;
				memcpy(symbolPos, curr->name, strlen(curr->name) + 1);
				symbolPos += strlen(curr->name) + 1;
			}
			// Export type pointer members for GC
			typeInfo.pointerCount = 0;
			for(TypeInfo::MemberVariable *curr = refType.firstVariable; curr; curr = curr->next)
			{
				if(curr->type->hasPointers)
				{
					typeInfo.pointerCount++;
					memberList[memberOffset].type = curr->type->typeIndex;
					memberList[memberOffset].offset = curr->offset;
					memberOffset++;
				}
			}
		}else{
			typeInfo.subCat = ExternTypeInfo::CAT_NONE;
			typeInfo.subType = 0;
			typeInfo.memberCount = 0;
		}
		typeInfo.definitionOffset = refType.genericInfo ? refType.genericInfo->start - int(CodeInfo::lexFullStart - CodeInfo::lexStart) : ~0u;
		if(refType.genericBase)
			typeInfo.definitionOffset = 0x80000000 | refType.genericBase->typeIndex;

		typeInfo.genericTypeCount = refType.genericInfo ? refType.genericInfo->aliasCount : 0;

		typeInfo.baseType = refType.parentType ? refType.parentType->typeIndex : 0;

		// Fill up next
		tInfo++;
	}

	ExternModuleInfo *mInfo = FindFirstModule(code);
	for(unsigned int i = 0; i < activeModules.size(); i++)
	{
		mInfo->nameOffset = int(symbolPos - debugSymbols);
		memcpy(symbolPos, activeModules[i].name, strlen(activeModules[i].name) + 1);
		symbolPos += strlen(activeModules[i].name) + 1;

		mInfo->funcStart = activeModules[i].funcStart;
		mInfo->funcCount = activeModules[i].funcCount;

		mInfo->variableOffset = 0;
		mInfo->nameHash = ~0u;

		mInfo++;
	}

	unsigned int actualExternVariableInfoCount = 0;

	ExternVarInfo *vInfo = FindFirstVar(code);

	for(unsigned int i = 0; i < CodeInfo::varInfo.size(); i++)
	{
		ExternVarInfo &varInfo = *vInfo;
		VariableInfo *refVar = CodeInfo::varInfo[i];

		if(refVar->pos >> 24)
			continue;

		actualExternVariableInfoCount++;

		varInfo.offsetToName = int(symbolPos - debugSymbols);
		memcpy(symbolPos, refVar->name.begin, refVar->name.length() + 1);
		symbolPos += refVar->name.length();
		*symbolPos++ = 0;

		varInfo.nameHash = GetStringHash(refVar->name.begin, refVar->name.end);

		varInfo.type = refVar->varType->typeIndex;
		varInfo.offset = refVar->pos;

		// Fill up next
		vInfo++;
	}

	unsigned int localOffset = 0;
	unsigned int offsetToGlobal = 0;
	ExternFuncInfo *fInfo = FindFirstFunc(code);
	ExternLocalInfo *localInfo = FindFirstLocal(code);

	clsListCount = 0;
	for(unsigned int i = 0; i < CodeInfo::funcInfo.size(); i++)
	{
		if(functionsInModules && i < functionsInModules)
			continue;
		ExternFuncInfo &funcInfo = *fInfo;
		FunctionInfo *refFunc = CodeInfo::funcInfo[i];

		if(refFunc->codeSize == 0 && refFunc->address != -1 && (refFunc->address & 0x80000000))
		{
			funcInfo.address = CodeInfo::funcInfo[refFunc->address & ~0x80000000]->address;
			funcInfo.codeSize = refFunc->address | 0x80000000;
		}else{
			funcInfo.address = refFunc->address;
			funcInfo.codeSize = refFunc->codeSize;
		}
		funcInfo.funcPtr = refFunc->funcPtr;
		funcInfo.isVisible = refFunc->visible;

		offsetToGlobal += refFunc->codeSize;

		funcInfo.nameHash = refFunc->nameHash;
		funcInfo.namespaceHash = refFunc->parentNamespace ? refFunc->parentNamespace->hash : ~0u;

		funcInfo.funcCat = (unsigned char)refFunc->type;

		funcInfo.isGenericInstance = !!refFunc->genericBase || refFunc->genericInstance || (refFunc->parentClass ? refFunc->parentClass->genericBase : false);

		funcInfo.funcType = refFunc->funcType->typeIndex;
		funcInfo.parentType = refFunc->parentClass ? refFunc->parentClass->typeIndex : ~0u;
		funcInfo.contextType = refFunc->contextType ? refFunc->contextType->typeIndex : ~0u;
		funcInfo.offsetToFirstLocal = localOffset;
		funcInfo.paramCount = refFunc->paramCount;

		if(!refFunc->generic)
		{
			funcInfo.retType = ExternFuncInfo::RETURN_UNKNOWN;
			if(refFunc->retType->type == TypeInfo::TYPE_VOID)
				funcInfo.retType = ExternFuncInfo::RETURN_VOID;
			else if(refFunc->retType->type == TypeInfo::TYPE_FLOAT || refFunc->retType->type == TypeInfo::TYPE_DOUBLE)
				funcInfo.retType = ExternFuncInfo::RETURN_DOUBLE;
#if defined(NULLC_COMPLEX_RETURN) && defined(__linux)
			else if(refFunc->retType->type == TypeInfo::TYPE_CHAR ||
				refFunc->retType->type == TypeInfo::TYPE_SHORT ||
				refFunc->retType->type == TypeInfo::TYPE_INT)
				funcInfo.retType = ExternFuncInfo::RETURN_INT;
			else if(refFunc->retType->type == TypeInfo::TYPE_LONG)
				funcInfo.retType = ExternFuncInfo::RETURN_LONG;
#else
			else if(refFunc->retType->type == TypeInfo::TYPE_INT || refFunc->retType->size <= 4)
				funcInfo.retType = ExternFuncInfo::RETURN_INT;
			else if(refFunc->retType->type == TypeInfo::TYPE_LONG || refFunc->retType->size == 8)
				funcInfo.retType = ExternFuncInfo::RETURN_LONG;
#endif
			funcInfo.returnShift = (unsigned char)(refFunc->retType->size / 4);
			CreateExternalInfo(funcInfo, *refFunc);
		}else{
			funcInfo.address = ~0u;
			funcInfo.retType = ExternFuncInfo::RETURN_VOID;
			funcInfo.funcType = 0;
			memset(funcInfo.rOffsets, 0, 8 * sizeof(unsigned));
			memset(funcInfo.fOffsets, 0, 8 * sizeof(unsigned));
			funcInfo.ps3Callable = false;
			funcInfo.genericOffset = refFunc->generic->start - int(CodeInfo::lexFullStart - CodeInfo::lexStart);
			funcInfo.genericReturnType = refFunc->retType ? refFunc->retType->typeIndex : ~0u;
		}

		ExternLocalInfo::LocalType paramType = refFunc->firstParam ? ExternLocalInfo::PARAMETER : ExternLocalInfo::LOCAL;
		if(refFunc->firstParam)
			refFunc->lastParam->next = refFunc->firstLocal;
		for(VariableInfo *curr = refFunc->firstParam ? refFunc->firstParam : refFunc->firstLocal; curr; curr = curr->next, localOffset++)
		{
			localInfo[localOffset].paramType = (unsigned char)paramType;
			localInfo[localOffset].paramFlags = (unsigned char)(curr->isExplicit ? ExternLocalInfo::IS_EXPLICIT : 0);
			localInfo[localOffset].defaultFuncId = curr->defaultValue ? (unsigned short)curr->defaultValueFuncID : 0xffff;
			localInfo[localOffset].type = curr->varType ? curr->varType->typeIndex : 0;
			localInfo[localOffset].size = curr->varType ? curr->varType->size : 0;
			localInfo[localOffset].offset = curr->pos;
			localInfo[localOffset].closeListID = 0;
			localInfo[localOffset].offsetToName = int(symbolPos - debugSymbols);
			memcpy(symbolPos, curr->name.begin, curr->name.length() + 1);
			symbolPos += curr->name.length();
			*symbolPos++ = 0;
			if(curr->next == refFunc->firstLocal)
				paramType = ExternLocalInfo::LOCAL;
		}
		if(refFunc->firstParam)
			refFunc->lastParam->next = NULL;
		funcInfo.localCount = localOffset - funcInfo.offsetToFirstLocal;

		ExternLocalInfo *lInfo = &localInfo[localOffset];
		for(FunctionInfo::ExternalInfo *curr = refFunc->firstExternal; curr; curr = curr->next, lInfo++)
		{
			TypeInfo *vType = curr->variable->varType;
			InplaceStr vName = curr->variable->name;
			lInfo->paramType = ExternLocalInfo::EXTERNAL;
			lInfo->type = vType->typeIndex;
			lInfo->size = vType->size;
			lInfo->target = curr->targetVar ? curr->targetVar->pos : curr->targetPos;
			lInfo->closeListID = (curr->targetDepth + CodeInfo::funcInfo[curr->targetFunc]->closeListStart) | (curr->targetLocal ? 0x80000000 : 0);
			lInfo->offsetToName = int(symbolPos - debugSymbols);
			memcpy(symbolPos, vName.begin, vName.length() + 1);
			symbolPos += vName.length();
			*symbolPos++ = 0;
		}
		funcInfo.externCount = refFunc->externalCount;
		localOffset += refFunc->externalCount;

		funcInfo.offsetToName = int(symbolPos - debugSymbols);
		memcpy(symbolPos, refFunc->name, refFunc->nameLength + 1);
		symbolPos += refFunc->nameLength + 1;

		funcInfo.closeListStart = refFunc->closeListStart = clsListCount;
		if(refFunc->closeUpvals)
			clsListCount += refFunc->maxBlockDepth;

		AliasInfo *explicitType = refFunc->explicitTypes;
		while(explicitType)
		{
			funcInfo.explicitTypeCount++;

			ExternVarInfo &varInfo = *vInfo;

			actualExternVariableInfoCount++;

			varInfo.offsetToName = int(symbolPos - debugSymbols);
			memcpy(symbolPos, explicitType->name.begin, explicitType->name.length());
			symbolPos += explicitType->name.length();
			*symbolPos++ = 0;

			varInfo.nameHash = explicitType->nameHash;

			varInfo.type = explicitType->type->typeIndex;
			varInfo.offset = 0;

			explicitType = explicitType->next;

			vInfo++;
		}

		// Fill up next
		fInfo++;
	}

	code->offsetToInfo = offsetToInfo;
	code->offsetToSource = offsetToSource;

	unsigned int infoCount = CodeInfo::cmdInfoList.sourceInfo.size();
	code->infoSize = infoCount;
	unsigned int *infoArray = FindSourceInfo(code);
	for(unsigned int i = 0; i < infoCount; i++)
	{
		infoArray[i * 2 + 0] = CodeInfo::cmdInfoList.sourceInfo[i].byteCodePos;
		infoArray[i * 2 + 1] = (unsigned int)(CodeInfo::cmdInfoList.sourceInfo[i].sourcePos - CodeInfo::cmdInfoList.sourceStart);
	}
	char *sourceCode = (char*)code + offsetToSource;
	memcpy(sourceCode, CodeInfo::cmdInfoList.sourceStart, sourceLength);
	code->sourceSize = sourceLength;

	code->globalCodeStart = offsetToGlobal;
	memcpy(FindCode(code), &CodeInfo::cmdList[0], CodeInfo::cmdList.size() * sizeof(VMCmd));

#ifdef NULLC_LLVM_SUPPORT
	code->llvmSize = llvmSize;
	code->llvmOffset = offsetToLLVM;
	memcpy(((char*)(code) + code->llvmOffset), llvmBinary, llvmSize);
#endif

	code->typedefCount = typedefCount;
	code->offsetToTypedef = offsetToTypedef;
	aliasInfo = CodeInfo::globalAliases;
	ExternTypedefInfo *currAlias = FindFirstTypedef(code);
	while(aliasInfo)
	{
		currAlias->offsetToName = int(symbolPos - debugSymbols);
		memcpy(symbolPos, aliasInfo->name.begin, aliasInfo->name.length() + 1);
		symbolPos += aliasInfo->name.length();
		*symbolPos++ = 0;
		currAlias->targetType = aliasInfo->type->typeIndex;
		currAlias->parentType = ~0u;
		currAlias++;
		aliasInfo = aliasInfo->next;
	}
	for(unsigned int i = 0; i < CodeInfo::typeInfo.size(); i++)
	{
		TypeInfo *type = CodeInfo::typeInfo[i];
		aliasInfo = type->childAlias;
		while(aliasInfo)
		{
			currAlias->offsetToName = int(symbolPos - debugSymbols);
			memcpy(symbolPos, aliasInfo->name.begin, aliasInfo->name.length() + 1);
			symbolPos += aliasInfo->name.length();
			*symbolPos++ = 0;
			currAlias->targetType = aliasInfo->type->typeIndex;
			currAlias->parentType = type->typeIndex;
			currAlias++;
			aliasInfo = aliasInfo->next;
		}
	}

	ExternNamespaceInfo *namespaceList = FindFirstNamespace(code);
	for(unsigned i = 0; i < CodeInfo::namespaceInfo.size(); i++)
	{
		namespaceList->offsetToName = int(symbolPos - debugSymbols);
		memcpy(symbolPos, CodeInfo::namespaceInfo[i]->name.begin, CodeInfo::namespaceInfo[i]->name.length() + 1);
		symbolPos += CodeInfo::namespaceInfo[i]->name.length();
		*symbolPos++ = 0;

		namespaceList->parentHash = CodeInfo::namespaceInfo[i]->parent ? CodeInfo::namespaceInfo[i]->parent->hash : ~0u;

		namespaceList++;
	}

	assert(externVariableInfoCount == actualExternVariableInfoCount);

	return size;
}

