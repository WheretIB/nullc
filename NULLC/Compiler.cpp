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

CompilerError::CompilerError(const char* errStr, const char* apprPos)
{
	Init(errStr, apprPos);
}

void CompilerError::Init(const char* errStr, const char* apprPos)
{
	empty = 0;
	unsigned int len = (unsigned int)strlen(errStr) < ERROR_LENGTH ? (unsigned int)strlen(errStr) : ERROR_LENGTH - 1;
	memcpy(error, errStr, len);
	error[len] = 0;
	if(apprPos)
	{
		const char *begin = apprPos;
		while((begin >= codeStart) && (*begin != '\n') && (*begin != '\r'))
			begin--;
		if(begin < apprPos)
			begin++;

		lineNum = 1;
		const char *scan = codeStart;
		while(scan && scan < begin)
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
const char *nullcBaseCode = "\
void assert(int val);\r\n\
void assert(int val, char[] message);\r\n\
\r\n\
int operator ==(char[] a, b);\r\n\
int operator !=(char[] a, b);\r\n\
char[] operator +(char[] a, b);\r\n\
char[] operator +=(char[] ref a, char[] b);\r\n\
\r\n\
char char(char a);\r\n\
short short(short a);\r\n\
int int(int a);\r\n\
long long(long a);\r\n\
float float(float a);\r\n\
double double(double a);\r\n\
void char:char(char a = 0){ *this = a; }\r\n\
void short:short(short a = 0){ *this = a; }\r\n\
void int:int(int a = 0){ *this = a; }\r\n\
void long:long(long a = 0){ *this = a; }\r\n\
void float:float(float a = 0){ *this = a; }\r\n\
void double:double(double a = 0){ *this = a; }\r\n\
\r\n\
char[] int:str();\r\n\
char[] double:str(int precision = 6);\r\n\
\r\n\
void ref __newS(int size);\r\n\
int[] __newA(int size, int count);\r\n\
auto ref	duplicate(auto ref obj);\r\n\
void		__duplicate_array(auto[] ref dst, auto[] src);\r\n\
auto[]		duplicate(auto[] arr){ auto[] r; __duplicate_array(&r, arr); return r; }\r\n\
auto ref	replace(auto ref l, r);\r\n\
void		swap(auto ref l, r);\r\n\
int			equal(auto ref l, r);\r\n\
\r\n\
void ref() __redirect(auto ref r, int[] ref f);\r\n\
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
int __pcomp(void ref(int) a, void ref(int) b);\r\n\
int __pncomp(void ref(int) a, void ref(int) b);\r\n\
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
// list comprehension helper functions\r\n\
void auto_array_impl(auto[] ref arr, typeid type, int count);\r\n\
auto[] auto_array(typeid type, int count)\r\n\
{\r\n\
	auto[] res;\r\n\
	auto_array_impl(&res, type, count);\r\n\
	return res;\r\n\
}\r\n\
typedef auto[] auto_array;\r\n\
// function will set auto[] element to the specified one, will data reallocation is neccessary\r\n\
void auto_array:set(auto ref x, int pos);\r\n\
void __force_size(auto[] ref s, int size);\r\n\
\r\n\
int isCoroutineReset(auto ref f);\r\n\
void __assertCoroutine(auto ref f);";

Compiler::Compiler()
{
	buildInTypes.clear();
	buildInTypes.reserve(32);

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

	buildInTypes.resize(CodeInfo::typeInfo.size());
	memcpy(&buildInTypes[0], &CodeInfo::typeInfo[0], CodeInfo::typeInfo.size() * sizeof(TypeInfo*));

	typeTop = TypeInfo::GetPoolTop();

	typeMap.init();
	funcMap.init();

	realGlobalCount = 0;

	// Add base module with build-in functions
	bool res = Compile(nullcBaseCode);
	assert(res && "Failed to compile base NULLC module");

	char *bytecode = NULL;
	GetBytecode(&bytecode);
	BinaryCache::PutBytecode("$base$.nc", bytecode);

#ifndef NULLC_NO_EXECUTOR
	AddModuleFunction("$base$", (void (*)())NULLC::Assert, "assert", 0);
	AddModuleFunction("$base$", (void (*)())NULLC::Assert2, "assert", 1);

	AddModuleFunction("$base$", (void (*)())NULLC::StrEqual, "==", 0);
	AddModuleFunction("$base$", (void (*)())NULLC::StrNEqual, "!=", 0);
	AddModuleFunction("$base$", (void (*)())NULLC::StrConcatenate, "+", 0);
	AddModuleFunction("$base$", (void (*)())NULLC::StrConcatenateAndSet, "+=", 0);

	AddModuleFunction("$base$", (void (*)())NULLC::Int, "char", 0);
	AddModuleFunction("$base$", (void (*)())NULLC::Int, "short", 0);
	AddModuleFunction("$base$", (void (*)())NULLC::Int, "int", 0);
	AddModuleFunction("$base$", (void (*)())NULLC::Long, "long", 0);
	AddModuleFunction("$base$", (void (*)())NULLC::Float, "float", 0);
	AddModuleFunction("$base$", (void (*)())NULLC::Double, "double", 0);

	AddModuleFunction("$base$", (void (*)())NULLC::IntToStr, "int::str", 0);
	AddModuleFunction("$base$", (void (*)())NULLC::DoubleToStr, "double::str", 0);

	AddModuleFunction("$base$", (void (*)())NULLC::AllocObject, "__newS", 0);
	AddModuleFunction("$base$", (void (*)())NULLC::AllocArray, "__newA", 0);
	AddModuleFunction("$base$", (void (*)())NULLC::CopyObject, "duplicate", 0);
	AddModuleFunction("$base$", (void (*)())NULLC::CopyArray, "__duplicate_array", 0);
	AddModuleFunction("$base$", (void (*)())NULLC::ReplaceObject, "replace", 0);
	AddModuleFunction("$base$", (void (*)())NULLC::SwapObjects, "swap", 0);
	AddModuleFunction("$base$", (void (*)())NULLC::CompareObjects, "equal", 0);

	AddModuleFunction("$base$", (void (*)())NULLC::FunctionRedirect, "__redirect", 0);

	AddModuleFunction("$base$", (void (*)())NULLC::Typeid, "typeid", 0);
	AddModuleFunction("$base$", (void (*)())NULLC::TypeSize, "typeid::size$", 0);
	AddModuleFunction("$base$", (void (*)())NULLC::TypesEqual, "==", 1);
	AddModuleFunction("$base$", (void (*)())NULLC::TypesNEqual, "!=", 1);

	AddModuleFunction("$base$", (void (*)())NULLC::RefCompare, "__rcomp", 0);
	AddModuleFunction("$base$", (void (*)())NULLC::RefNCompare, "__rncomp", 0);

	AddModuleFunction("$base$", (void (*)())NULLC::FuncCompare, "__pcomp", 0);
	AddModuleFunction("$base$", (void (*)())NULLC::FuncNCompare, "__pncomp", 0);

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

	for(unsigned int i = 0, e = (unsigned int)strlen(path); i != e; i++)
	{
		if(path[i] == '.')
			path[i] = '/';
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

bool Compiler::ImportModule(const char* bytecode, const char* pos, unsigned int number)
{
	char errBuf[256];
	ByteCode *bCode = (ByteCode*)bytecode;
	char *symbols = (char*)(bCode) + bCode->offsetToSymbols;

	typeRemap.clear();

#ifdef IMPORT_VERBOSE_DEBUG_OUTPUT
	printf("Importing module %s\r\n", pos);
#endif
	// Import types
	ExternTypeInfo *tInfo = FindFirstType(bCode);
	unsigned int *memberList = (unsigned int*)(tInfo + bCode->typeCount);

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

	tInfo = FindFirstType(bCode);
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
				newInfo->CreateFunctionType(CodeInfo::typeInfo[typeRemap[memberList[tInfo->memberOffset]]], tInfo->memberCount);

				for(unsigned int n = 1; n < tInfo->memberCount + 1; n++)
				{
					newInfo->funcType->paramType[n-1] = CodeInfo::typeInfo[typeRemap[memberList[tInfo->memberOffset + n]]];
					newInfo->funcType->paramSize += newInfo->funcType->paramType[n-1]->size > 4 ? newInfo->funcType->paramType[n-1]->size : 4;
				}

#ifdef _DEBUG
				newInfo->AddMemberVariable("context", typeInt);
				newInfo->AddMemberVariable("ptr", typeInt);
#endif
				newInfo->size = 4 + NULLC_PTR_SIZE;
				newInfo->hasPointers = true;
				break;
			case ExternTypeInfo::CAT_ARRAY:
				tempInfo = CodeInfo::typeInfo[typeRemap[tInfo->subType]];
				CodeInfo::typeInfo.push_back(new TypeInfo(CodeInfo::typeInfo.size(), NULL, 0, tempInfo->arrLevel + 1, tInfo->arrSize, tempInfo, TypeInfo::TYPE_COMPLEX));
				newInfo = CodeInfo::typeInfo.back();

				if(tInfo->arrSize == TypeInfo::UNSIZED_ARRAY)
				{
					newInfo->size = NULLC_PTR_SIZE;
					newInfo->AddMemberVariable("size", typeInt);
				}else{
					newInfo->size = tempInfo->size * tInfo->arrSize;
					if(newInfo->size % 4 != 0)
					{
						newInfo->paddingBytes = 4 - (newInfo->size % 4);
						newInfo->size += 4 - (newInfo->size % 4);
					}
				}
				newInfo->nextArrayType = tempInfo->arrayType;
				tempInfo->arrayType = newInfo;
				CodeInfo::typeArrays.push_back(newInfo);
				break;
			case ExternTypeInfo::CAT_POINTER:
				assert(typeRemap[tInfo->subType] < CodeInfo::typeInfo.size());
				tempInfo = CodeInfo::typeInfo[typeRemap[tInfo->subType]];
				CodeInfo::typeInfo.push_back(new TypeInfo(CodeInfo::typeInfo.size(), NULL, tempInfo->refLevel + 1, 0, 1, tempInfo, TypeInfo::NULLC_PTR_TYPE));
				newInfo = CodeInfo::typeInfo.back();
				newInfo->size = NULLC_PTR_SIZE;

				// Save it for future use
				CodeInfo::typeInfo[typeRemap[tInfo->subType]]->refType = newInfo;
				break;
			case ExternTypeInfo::CAT_CLASS:
			{
				unsigned int strLength = (unsigned int)strlen(symbols + tInfo->offsetToName) + 1;
				const char *nameCopy = strcpy((char*)dupStringsModule.Allocate(strLength), symbols + tInfo->offsetToName);
				newInfo = new TypeInfo(CodeInfo::typeInfo.size(), nameCopy, 0, 0, 1, NULL, TypeInfo::TYPE_COMPLEX);

				CodeInfo::typeInfo.push_back(newInfo);
				CodeInfo::classMap.insert(newInfo->GetFullNameHash(), newInfo);

				// This two pointers are used later to fill type member information
				newInfo->firstVariable = (TypeInfo::MemberVariable*)(symbols + tInfo->offsetToName + strLength);
				newInfo->lastVariable = (TypeInfo::MemberVariable*)tInfo;
			}
				break;
			default:
				SafeSprintf(errBuf, 256, "ERROR: new type in module %s named %s unsupported", pos, (symbols + tInfo->offsetToName));
				CodeInfo::lastError.Init(errBuf, NULL);
				return false;
			}
			newInfo->alignBytes = tInfo->defaultAlign;
		}
	}
	for(unsigned int i = oldTypeCount; i < CodeInfo::typeInfo.size(); i++)
	{
		TypeInfo *type = CodeInfo::typeInfo[i];
		if(type->type == TypeInfo::TYPE_COMPLEX && !type->funcType && !type->subType)
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
				TypeInfo *memberType = CodeInfo::typeInfo[typeRemap[memberList[typeInfo->memberOffset + n]]];
				unsigned int alignment = memberType->alignBytes > 4 ? 4 : memberType->alignBytes;
				if(alignment && type->size % alignment != 0)
					type->size += alignment - (type->size % alignment);
				type->AddMemberVariable(nameCopy, memberType);
			}
			if(type->size % 4 != 0)
			{
				type->paddingBytes = 4 - (type->size % 4);
				type->size += 4 - (type->size % 4);
			}
		}
	}

#ifdef IMPORT_VERBOSE_DEBUG_OUTPUT
	tInfo = FindFirstType(bCode);
	for(unsigned int i = 0; i < typeRemap.size(); i++)
		printf("Type\r\n\t%s\r\nis remapped from index %d to index %d with\r\n\t%s\r\ntype\r\n", symbols + tInfo[i].offsetToName, i, typeRemap[i], CodeInfo::typeInfo[typeRemap[i]]->GetFullTypeName());
#endif

	if(!setjmp(CodeInfo::errorHandler))
	{
		// Import variables
		ExternVarInfo *vInfo = FindFirstVar(bCode);
		for(unsigned int i = 0; i < bCode->variableExportCount; i++, vInfo++)
		{
			// Exclude temporary variables from import
			if(memcmp(&symbols[vInfo->offsetToName], "$temp", 5) == 0)
				continue;
			SelectTypeByIndex(typeRemap[vInfo->type]);
			AddVariable(pos, InplaceStr(symbols + vInfo->offsetToName));
			CodeInfo::varInfo.back()->parentModule = number;
			CodeInfo::varInfo.back()->pos = (number << 24) + (vInfo->offset & 0x00ffffff);
		}
	}else{
		return false;
	}

	// Import functions
	ExternFuncInfo *fInfo = FindFirstFunc(bCode);
	ExternLocalInfo *fLocals = (ExternLocalInfo*)((char*)(bCode) + bCode->offsetToLocals);

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

			AddFunctionToSortedList(lastFunc);

			lastFunc->indexInArr = CodeInfo::funcInfo.size() - 1;
			lastFunc->address = fInfo->funcPtr ? -1 : 0;
			lastFunc->funcPtr = fInfo->funcPtr;
			lastFunc->type = (FunctionInfo::FunctionCategory)fInfo->funcCat;

			for(unsigned int n = 0; n < fInfo->paramCount; n++)
			{
				ExternLocalInfo &lInfo = fLocals[fInfo->offsetToFirstLocal + n];
				TypeInfo *currType = CodeInfo::typeInfo[typeRemap[lInfo.type]];
				lastFunc->AddParameter(new VariableInfo(lastFunc, InplaceStr(symbols + lInfo.offsetToName), GetStringHash(symbols + lInfo.offsetToName), 0, currType, false));
				lastFunc->allParamSize += currType->size < 4 ? 4 : currType->size;
				lastFunc->lastParam->defaultValueFuncID = lInfo.defaultFuncId;
			}
			lastFunc->implemented = true;
			lastFunc->funcType = CodeInfo::typeInfo[typeRemap[fInfo->funcType]];

			lastFunc->retType = lastFunc->funcType->funcType->retType;

			if(fInfo->parentType != ~0u && !fInfo->externCount)
			{
				lastFunc->parentClass = CodeInfo::typeInfo[typeRemap[fInfo->parentType]];
				lastFunc->extraParam = new VariableInfo(lastFunc, InplaceStr("this"), GetStringHash("this"), 0, CodeInfo::GetReferenceType(lastFunc->parentClass), false);
			}

			assert(lastFunc->funcType->funcType->paramCount == lastFunc->paramCount);

#ifdef IMPORT_VERBOSE_DEBUG_OUTPUT
			printf(" Importing function %s %s(", lastFunc->retType->GetFullTypeName(), lastFunc->name);
			VariableInfo *curr = lastFunc->firstParam;
			for(unsigned int n = 0; n < fInfo->paramCount; n++)
			{
				printf("%s%s %.*s", n == 0 ? "" : ", ", curr->varType->GetFullTypeName(), curr->name.end-curr->name.begin, curr->name.begin);
				curr = curr->next;
			}
			printf(")\r\n");
#endif
		}else if(CodeInfo::funcInfo[index]->name[0] == '$'){
			CodeInfo::funcInfo.push_back(CodeInfo::funcInfo[index]);
		}else{
			SafeSprintf(errBuf, 256, "ERROR: function %s (type %s) is already defined. While importing %s", CodeInfo::funcInfo[index]->name, CodeInfo::funcInfo[index]->funcType->GetFullTypeName(), pos);
			CodeInfo::lastError.Init(errBuf, NULL);
			return false;
		}

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
		if(!Compile(fileContent, true))
		{
			unsigned int currLen = (unsigned int)strlen(CodeInfo::lastError.error);
			SafeSprintf(CodeInfo::lastError.error+currLen, 256 - strlen(CodeInfo::lastError.error), " [in module %s]", file);

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
			NULLC::dealloc(fileContent);

		BinaryCache::PutBytecode(failedImportPath ? altFile : file, bytecode);

		return bytecode;
	}else{
		CodeInfo::lastError.Init("", NULL);
		SafeSprintf(CodeInfo::lastError.error, 256, "ERROR: module %s not found", altFile);
	}
	return NULL;
}

#ifdef NULLC_LLVM_SUPPORT
const char *llvmBinary = NULL;
unsigned	llvmSize = 0;
#endif

bool Compiler::Compile(const char* str, bool noClear)
{
	CodeInfo::nodeList.clear();
	NodeZeroOP::DeleteNodes();

	if(!noClear)
	{
		lexer.Clear();
		moduleSource.clear();
		dupStringsModule.Clear();
		funcMap.clear();
	}
	unsigned int lexStreamStart = lexer.GetStreamSize();
	lexer.Lexify(str);

	const char	*moduleName[32];
	const char	*moduleData[32];
	unsigned int moduleCount = 0;

	if(BinaryCache::GetBytecode("$base$.nc"))
	{
		moduleName[moduleCount] = "$base$.nc";
		moduleData[moduleCount++] = BinaryCache::GetBytecode("$base$.nc");
	}

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
		if(!bytecode && importPath)
			bytecode = BinaryCache::GetBytecode(pathNoImport);

		if(moduleCount == 32)
		{
			CodeInfo::lastError.Init("ERROR: temporary limit for 32 modules", name->pos);
			return false;
		}

		moduleName[moduleCount] = strcpy((char*)dupStringsModule.Allocate((unsigned int)strlen(pathNoImport) + 1), pathNoImport);
		if(!bytecode)
		{
			unsigned int lexPos = (unsigned int)(start - &lexer.GetStreamStart()[lexStreamStart]);
			bytecode = BuildModule(path, pathNoImport);
			start = &lexer.GetStreamStart()[lexStreamStart + lexPos];
		}
		if(!bytecode)
			return false;
		moduleData[moduleCount++] = bytecode;
	}

	ClearState();

	activeModules.clear();

	for(unsigned int i = 0; i < moduleCount; i++)
	{
		activeModules.push_back();
		activeModules.back().name = moduleName[i];
		activeModules.back().funcStart = CodeInfo::funcInfo.size();
		if(!ImportModule(moduleData[i], moduleName[i], i + 1))
			return false;
		SetGlobalSize(0);
		activeModules.back().funcCount = CodeInfo::funcInfo.size() - activeModules.back().funcStart;
	}

	CompilerError::codeStart = str;
	CodeInfo::cmdInfoList.SetSourceStart(str);

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
					char	*functionName = (char*)AllocateString(func->nameLength + int(param->name.end - param->name.begin) + 3 + 12);
					const char	*parentName = func->type == FunctionInfo::THISCALL ? strchr(func->name, ':') + 2 : func->name;
					SafeSprintf(functionName, func->nameLength + int(param->name.end - param->name.begin) + 3 + 12, "$%s_%.*s_%d", parentName, param->name.end - param->name.begin, param->name.begin, CodeInfo::FindFunctionByPtr(func));

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
	for(unsigned int i = 0; i < CodeInfo::funcDefList.size(); i++)
	{
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
#ifdef NULLC_LOG_FILES
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
#else
	(void)fileName;
#endif
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
	fprintf(fC, "// Array classes\r\n");
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
			fprintf(fC, "{\r\n");
			TypeInfo::MemberVariable *curr = type->firstVariable;
			for(; curr; curr = curr->next)
			{
				fprintf(fC, "\t");
				if(strchr(type->name, ':') && curr == type->firstVariable)
					fprintf(fC, "void *%s", curr->name);
				else
					curr->type->OutputCType(fC, curr->name);
				fprintf(fC, ";\r\n");
			}
			fprintf(fC, "};\r\n");
		}
	}

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
				sprintf(name, "%s%.*s_%d", namePrefix, int(local->name.end - local->name.begin)-nameShift, local->name.begin+nameShift, local->pos);
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
				sprintf(name, "%s%.*s_%d", namePrefix, int(local->name.end - local->name.begin)-nameShift, local->name.begin+nameShift, local->pos);
			
				fprintf(fC, "__nullcUpvalue *__upvalue_%d_%s = 0;\r\n", CodeInfo::FindFunctionByPtr(local->parentFunction), name);
			}
		}
	}
	for(unsigned int i = activeModules[0].funcCount; i < CodeInfo::funcInfo.size(); i++)
	{
		FunctionInfo *info = CodeInfo::funcInfo[i];
		if(i < functionsInModules && (info->type == FunctionInfo::LOCAL))
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
			sprintf(name, "%.*s", int(param->name.end - param->name.begin), param->name.begin);
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
		if(varInfo->pos >> 24)
			fprintf(fC, "extern ");
		else if(*varInfo->name.begin == '$' && 0 != strcmp(varInfo->name.end-3, "ext"))
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
			const char *namePrefix = varInfo->name.begin[0] == '$' ? (varInfo->name.begin[1] == '$' ? "___" : "__") : "";
			unsigned int nameShift = varInfo->name.begin[0] == '$' ? (varInfo->name.begin[1] == '$' ? 2 : 1) : 0;
			sprintf(vName, varInfo->blockDepth > 1 ? "%s%.*s_%d" : "%s%.*s", namePrefix, int(varInfo->name.end-varInfo->name.begin)-nameShift, varInfo->name.begin+nameShift, varInfo->pos);
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
				if(info->nameInCHash == CodeInfo::funcInfo[l]->nameInCHash)
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

	unsigned int symbolStorageSize = 0;
	unsigned int allMemberCount = 0;
	for(unsigned int i = 0; i < CodeInfo::typeInfo.size(); i++)
	{
		TypeInfo *type = CodeInfo::typeInfo[i];

		symbolStorageSize += type->GetFullNameLength() + 1;
		if(type->type == TypeInfo::TYPE_COMPLEX && type->subType == NULL && type->funcType == NULL)
		{
			for(TypeInfo::MemberVariable *curr = type->firstVariable; curr; curr = curr->next)
			{
				symbolStorageSize += (unsigned int)strlen(curr->name) + 1;
				if(curr->type->hasPointers)
					allMemberCount += 2;
			}
		}
		allMemberCount += type->funcType ? type->funcType->paramCount + 1 : type->memberCount;
	}
	size += allMemberCount * sizeof(unsigned int);

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
	size += CodeInfo::varInfo.size() * sizeof(ExternVarInfo);
	for(unsigned int i = 0; i < CodeInfo::varInfo.size(); i++)
	{
		VariableInfo *curr = CodeInfo::varInfo[i];
		if(curr->pos >> 24)
			continue;
		globalVarCount++;
		if(i < realGlobalCount)
			exportVarCount++;
		symbolStorageSize += (unsigned int)(curr->name.end - curr->name.begin) + 1;
	}

	unsigned int offsetToFunc = size;
	size += (CodeInfo::funcInfo.size() - functionsInModules) * sizeof(ExternFuncInfo);

	unsigned int offsetToFirstLocal = size;

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
			symbolStorageSize += (unsigned int)(curr->name.end - curr->name.begin) + 1;

		localCount += (unsigned int)func->localCount;
		for(VariableInfo *curr = func->firstLocal; curr; curr = curr->next)
			symbolStorageSize += (unsigned int)(curr->name.end - curr->name.begin) + 1;

		localCount += (unsigned int)func->externalCount;
		for(FunctionInfo::ExternalInfo *curr = func->firstExternal; curr; curr = curr->next)
			symbolStorageSize += (unsigned int)(curr->variable->name.end - curr->variable->name.begin) + 1;
	}
	size += localCount * sizeof(ExternLocalInfo);

	size += clsListCount * sizeof(unsigned int);

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

	ByteCode	*code = (ByteCode*)(*bytecode);
	code->size = size;

	code->typeCount = (unsigned int)CodeInfo::typeInfo.size();

	code->dependsCount = activeModules.size();
	code->offsetToFirstModule = offsetToModule;
	code->firstModule = (ExternModuleInfo*)((char*)(code) + code->offsetToFirstModule);

	code->globalVarSize = GetGlobalSize();
	code->variableCount = globalVarCount;
	code->variableExportCount = exportVarCount;
	code->offsetToFirstVar = offsetToVar;

	code->functionCount = (unsigned int)CodeInfo::funcInfo.size();
	code->moduleFunctionCount = functionsInModules;
	code->offsetToFirstFunc = offsetToFunc;

	code->localCount = localCount;
	code->offsetToLocals = offsetToFirstLocal;
	code->firstLocal = (ExternLocalInfo*)((char*)(code) + code->offsetToLocals);

	code->closureListCount = clsListCount;

	code->codeSize = CodeInfo::cmdList.size();
	code->offsetToCode = offsetToCode;

	code->symbolLength = symbolStorageSize;
	code->offsetToSymbols = offsetToSymbols;
	code->debugSymbols = (*bytecode) + offsetToSymbols;

	char*	symbolPos = code->debugSymbols;

	ExternTypeInfo *tInfo = FindFirstType(code);
	code->firstType = tInfo;
	unsigned int *memberList = (unsigned int*)(tInfo + CodeInfo::typeInfo.size());
	unsigned int memberOffset = 0;
	for(unsigned int i = 0; i < CodeInfo::typeInfo.size(); i++)
	{
		ExternTypeInfo &typeInfo = *tInfo;
		TypeInfo &refType = *CodeInfo::typeInfo[i];

		typeInfo.offsetToName = int(symbolPos - code->debugSymbols);
		memcpy(symbolPos, refType.GetFullTypeName(), refType.GetFullNameLength() + 1);
		symbolPos += refType.GetFullNameLength() + 1;

		typeInfo.size = refType.size;
		typeInfo.type = (ExternTypeInfo::TypeCategory)refType.type;
		typeInfo.nameHash = refType.GetFullNameHash();

		typeInfo.defaultAlign = (unsigned short)refType.alignBytes;

		typeInfo.pointerCount = refType.hasPointers;
		if(refType.funcType != 0)						// Function type
		{
			typeInfo.subCat = ExternTypeInfo::CAT_FUNCTION;
			typeInfo.memberCount = refType.funcType->paramCount;
			typeInfo.memberOffset = memberOffset;
			memberList[memberOffset++] = refType.funcType->retType->typeIndex;
			for(unsigned int k = 0; k < refType.funcType->paramCount; k++)
				memberList[memberOffset++] = refType.funcType->paramType[k]->typeIndex;
		}else if(refType.arrLevel != 0){				// Array type
			typeInfo.subCat = ExternTypeInfo::CAT_ARRAY;
			typeInfo.arrSize = refType.arrSize;
			typeInfo.subType = refType.subType->typeIndex;
		}else if(refType.refLevel != 0){				// Pointer type
			typeInfo.subCat = ExternTypeInfo::CAT_POINTER;
			typeInfo.subType = refType.subType->typeIndex;
			typeInfo.memberCount = 0;
		}else if(refType.type == TypeInfo::TYPE_COMPLEX){	// Complex type
			typeInfo.subCat = ExternTypeInfo::CAT_CLASS;
			typeInfo.memberCount = refType.memberCount;
			typeInfo.memberOffset = memberOffset;
			for(TypeInfo::MemberVariable *curr = refType.firstVariable; curr; curr = curr->next)
			{
				memberList[memberOffset++] = curr->type->typeIndex;
				memcpy(symbolPos, curr->name, strlen(curr->name) + 1);
				symbolPos += strlen(curr->name) + 1;
			}
			typeInfo.pointerCount = 0;
			for(TypeInfo::MemberVariable *curr = refType.firstVariable; curr; curr = curr->next)
			{
				if(curr->type->hasPointers)
				{
					typeInfo.pointerCount++;
					memberList[memberOffset++] = curr->type->typeIndex;
					memberList[memberOffset++] = curr->offset;
				}
			}
		}else{
			typeInfo.subCat = ExternTypeInfo::CAT_NONE;
			typeInfo.subType = 0;
			typeInfo.memberCount = 0;
		}

		// Fill up next
		tInfo++;
	}

	ExternModuleInfo *mInfo = code->firstModule;
	for(unsigned int i = 0; i < activeModules.size(); i++)
	{
		mInfo->nameOffset = int(symbolPos - code->debugSymbols);
		memcpy(symbolPos, activeModules[i].name, strlen(activeModules[i].name) + 1);
		symbolPos += strlen(activeModules[i].name) + 1;

		mInfo->funcStart = activeModules[i].funcStart;
		mInfo->funcCount = activeModules[i].funcCount;

		mInfo->variableOffset = 0;
		mInfo->nameHash = ~0u;

		mInfo++;
	}

	ExternVarInfo *vInfo = FindFirstVar(code);
	code->firstVar = vInfo;
	for(unsigned int i = 0; i < CodeInfo::varInfo.size(); i++)
	{
		ExternVarInfo &varInfo = *vInfo;
		VariableInfo *refVar = CodeInfo::varInfo[i];

		if(refVar->pos >> 24)
			continue;

		varInfo.offsetToName = int(symbolPos - code->debugSymbols);
		memcpy(symbolPos, refVar->name.begin, refVar->name.end - refVar->name.begin + 1);
		symbolPos += refVar->name.end - refVar->name.begin;
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
	code->firstFunc = fInfo;
	clsListCount = 0;
	for(unsigned int i = 0; i < CodeInfo::funcInfo.size(); i++)
	{
		if(functionsInModules && i < functionsInModules)
			continue;
		ExternFuncInfo &funcInfo = *fInfo;
		FunctionInfo *refFunc = CodeInfo::funcInfo[i];

		if(refFunc->codeSize == 0 && refFunc->address != -1 && (refFunc->address & 0x80000000))
			funcInfo.address = CodeInfo::funcInfo[refFunc->address & ~0x80000000]->address;
		else
			funcInfo.address = refFunc->address;
		funcInfo.codeSize = refFunc->codeSize;
		funcInfo.funcPtr = refFunc->funcPtr;
		funcInfo.isVisible = refFunc->visible;

		offsetToGlobal += funcInfo.codeSize;

		funcInfo.nameHash = refFunc->nameHash;

		funcInfo.isNormal = refFunc->type == FunctionInfo::NORMAL ? 1 : 0;
		funcInfo.funcCat = (unsigned char)refFunc->type;

		funcInfo.retType = ExternFuncInfo::RETURN_UNKNOWN;
		if(refFunc->retType->type == TypeInfo::TYPE_VOID)
			funcInfo.retType = ExternFuncInfo::RETURN_VOID;
		else if(refFunc->retType->type == TypeInfo::TYPE_FLOAT || refFunc->retType->type == TypeInfo::TYPE_DOUBLE)
			funcInfo.retType = ExternFuncInfo::RETURN_DOUBLE;
#ifdef NULLC_COMPLEX_RETURN
		else if(refFunc->retType->type == TypeInfo::TYPE_CHAR ||
			refFunc->retType->type == TypeInfo::TYPE_SHORT ||
			refFunc->retType->type == TypeInfo::TYPE_INT)
			funcInfo.retType = ExternFuncInfo::RETURN_INT;
		else if(refFunc->retType->type == TypeInfo::TYPE_LONG)
			funcInfo.retType = ExternFuncInfo::RETURN_LONG;
		funcInfo.returnShift = (unsigned char)(refFunc->retType->size / 4);
#else
		else if(refFunc->retType->type == TypeInfo::TYPE_INT || refFunc->retType->size <= 4)
			funcInfo.retType = ExternFuncInfo::RETURN_INT;
		else if(refFunc->retType->type == TypeInfo::TYPE_LONG || refFunc->retType->size == 8)
			funcInfo.retType = ExternFuncInfo::RETURN_LONG;
#endif

		funcInfo.funcType = refFunc->funcType->typeIndex;
		funcInfo.parentType = (refFunc->parentClass ? refFunc->parentClass->typeIndex : ~0u);

		CreateExternalInfo(funcInfo, *refFunc);

		funcInfo.offsetToFirstLocal = localOffset;

		funcInfo.paramCount = refFunc->paramCount;

		ExternLocalInfo::LocalType paramType = refFunc->firstParam ? ExternLocalInfo::PARAMETER : ExternLocalInfo::LOCAL;
		if(refFunc->firstParam)
			refFunc->lastParam->next = refFunc->firstLocal;
		for(VariableInfo *curr = refFunc->firstParam ? refFunc->firstParam : refFunc->firstLocal; curr; curr = curr->next, localOffset++)
		{
			code->firstLocal[localOffset].paramType = (unsigned short)paramType;
			code->firstLocal[localOffset].defaultFuncId = curr->defaultValue ? (unsigned short)curr->defaultValueFuncID : 0xffff;
			code->firstLocal[localOffset].type = curr->varType->typeIndex;
			code->firstLocal[localOffset].size = curr->varType->size;
			code->firstLocal[localOffset].offset = curr->pos;
			code->firstLocal[localOffset].closeListID = 0;
			code->firstLocal[localOffset].offsetToName = int(symbolPos - code->debugSymbols);
			memcpy(symbolPos, curr->name.begin, curr->name.end - curr->name.begin + 1);
			symbolPos += curr->name.end - curr->name.begin;
			*symbolPos++ = 0;
			if(curr->next == refFunc->firstLocal)
				paramType = ExternLocalInfo::LOCAL;
		}
		if(refFunc->firstParam)
			refFunc->lastParam->next = NULL;
		funcInfo.localCount = localOffset - funcInfo.offsetToFirstLocal;

		ExternLocalInfo *lInfo = &code->firstLocal[localOffset];
		for(FunctionInfo::ExternalInfo *curr = refFunc->firstExternal; curr; curr = curr->next, lInfo++)
		{
			TypeInfo *vType = curr->variable->varType;
			InplaceStr vName = curr->variable->name;
			lInfo->paramType = ExternLocalInfo::EXTERNAL;
			lInfo->type = vType->typeIndex;
			lInfo->size = vType->size;
			lInfo->target = curr->targetPos;
			lInfo->closeListID = (curr->targetDepth + CodeInfo::funcInfo[curr->targetFunc]->closeListStart) | (curr->targetLocal ? 0x80000000 : 0);
			lInfo->offsetToName = int(symbolPos - code->debugSymbols);
			memcpy(symbolPos, vName.begin, vName.end - vName.begin + 1);
			symbolPos += vName.end - vName.begin;
			*symbolPos++ = 0;
		}
		funcInfo.externCount = refFunc->externalCount;
		if(funcInfo.externCount)
		{
			assert(refFunc->type == FunctionInfo::LOCAL || refFunc->type == FunctionInfo::COROUTINE);
			funcInfo.parentType = refFunc->extraParam->varType->typeIndex;
		}
		localOffset += refFunc->externalCount;

		funcInfo.offsetToName = int(symbolPos - code->debugSymbols);
		memcpy(symbolPos, refFunc->name, refFunc->nameLength + 1);
		symbolPos += refFunc->nameLength + 1;

		funcInfo.closeListStart = refFunc->closeListStart = clsListCount;
		if(refFunc->closeUpvals)
			clsListCount += refFunc->maxBlockDepth;

		// Fill up next
		fInfo++;
	}

	code->offsetToInfo = offsetToInfo;
	code->offsetToSource = offsetToSource;

	unsigned int infoCount = CodeInfo::cmdInfoList.sourceInfo.size();
	code->infoSize = infoCount;
	unsigned int *infoArray = (unsigned int*)((char*)code + offsetToInfo);
	for(unsigned int i = 0; i < infoCount; i++)
	{
		infoArray[i * 2 + 0] = CodeInfo::cmdInfoList.sourceInfo[i].byteCodePos;
		infoArray[i * 2 + 1] = (unsigned int)(CodeInfo::cmdInfoList.sourceInfo[i].sourcePos - CodeInfo::cmdInfoList.sourceStart);
	}
	char *sourceCode = (char*)code + offsetToSource;
	memcpy(sourceCode, CodeInfo::cmdInfoList.sourceStart, sourceLength);
	code->sourceSize = sourceLength;

	code->code = FindCode(code);
	code->globalCodeStart = offsetToGlobal;
	memcpy(code->code, &CodeInfo::cmdList[0], CodeInfo::cmdList.size() * sizeof(VMCmd));

#ifdef NULLC_LLVM_SUPPORT
	code->llvmSize = llvmSize;
	code->llvmOffset = offsetToLLVM;
	memcpy(((char*)(code) + code->llvmOffset), llvmBinary, llvmSize);
#endif

	return size;
}

