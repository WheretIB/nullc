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

CompilerError::CompilerError(const char* errStr, const char* apprPos)
{
	Init(errStr, apprPos);
}

void CompilerError::Init(const char* errStr, const char* apprPos)
{
	empty = 0;
	unsigned int len = (unsigned int)strlen(errStr) < 256 ? (unsigned int)strlen(errStr) : 255;
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
\r\n\
char[] int:str();\r\n\
\r\n\
void ref __newS(int size);\r\n\
int[] __newA(int size, int count);\r\n\
auto ref duplicate(auto ref obj);\r\n\
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
// typeid comparison\r\n\
int operator==(typeid a, b);\r\n\
int operator!=(typeid a, b);\r\n\
";

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

	info->AddMemberVariable("id", typeInt);
	info->size = 4;

	typeObject->AddMemberVariable("type", typeInt);
	typeObject->AddMemberVariable("ptr", CodeInfo::GetReferenceType(typeVoid));
	typeObject->size = 4 + NULLC_PTR_SIZE;

	buildInTypes.resize(CodeInfo::typeInfo.size());
	memcpy(&buildInTypes[0], &CodeInfo::typeInfo[0], CodeInfo::typeInfo.size() * sizeof(TypeInfo*));

	typeTop = TypeInfo::GetPoolTop();

	typeMap.init();

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

	AddModuleFunction("$base$", (void (*)())NULLC::AllocObject, "__newS", 0);
	AddModuleFunction("$base$", (void (*)())NULLC::AllocArray, "__newA", 0);
	AddModuleFunction("$base$", (void (*)())NULLC::CopyObject, "duplicate", 0);

	AddModuleFunction("$base$", (void (*)())NULLC::FunctionRedirect, "__redirect", 0);

	AddModuleFunction("$base$", (void (*)())NULLC::Typeid, "typeid", 0);
	AddModuleFunction("$base$", (void (*)())NULLC::TypesEqual, "==", 1);
	AddModuleFunction("$base$", (void (*)())NULLC::TypesNEqual, "!=", 1);
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

	NodeZeroOP::ResetNodes();

	typeMap.reset();
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
		CodeInfo::lastError = CompilerError(errBuf, NULL);
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
		CodeInfo::lastError = CompilerError(errBuf, NULL);
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
				CodeInfo::lastError = CompilerError(errBuf, NULL);
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
			if(symbols[vInfo->offsetToName] == '$')
				continue;
			SelectTypeByIndex(typeRemap[vInfo->type]);
			AddVariable(pos, InplaceStr(symbols + vInfo->offsetToName));
			CodeInfo::varInfo.back()->parentModule = number;
			CodeInfo::varInfo.back()->pos += (number << 24);
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
		for(unsigned int n = 0; n < oldFuncCount && index == INDEX_NONE; n++)
		{
			if(CodeInfo::funcInfo[n]->nameHash == fInfo->nameHash && CodeInfo::funcInfo[n]->funcType == CodeInfo::typeInfo[typeRemap[fInfo->funcType]])
				index = n;
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
			FunctionInfo* lastFunc = CodeInfo::funcInfo.back();

			AddFunctionToSortedList(lastFunc);

			lastFunc->address = fInfo->funcPtr ? -1 : 0;
			lastFunc->funcPtr = fInfo->funcPtr;

			for(unsigned int n = 0; n < fInfo->paramCount; n++)
			{
				ExternLocalInfo &lInfo = fLocals[fInfo->offsetToFirstLocal + n];
				TypeInfo *currType = CodeInfo::typeInfo[typeRemap[lInfo.type]];
				lastFunc->AddParameter(new VariableInfo(lastFunc, InplaceStr(symbols + lInfo.offsetToName), GetStringHash(symbols + lInfo.offsetToName), 0, currType, false));
				lastFunc->allParamSize += currType->size < 4 ? 4 : currType->size;
				lastFunc->lastParam->defaultValueFuncID = lInfo.defaultFuncId;
			}
			lastFunc->implemented = true;
			lastFunc->type = strchr(lastFunc->name, ':') ? FunctionInfo::THISCALL : FunctionInfo::NORMAL;
			lastFunc->funcType = CodeInfo::typeInfo[typeRemap[fInfo->funcType]];

			lastFunc->retType = lastFunc->funcType->funcType->retType;

			if(fInfo->parentType != ~0u)
				lastFunc->parentClass = CodeInfo::typeInfo[typeRemap[fInfo->parentType]];

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
		}else{
			SafeSprintf(errBuf, 256, "ERROR: function %s (type %s) is already defined. While importing %s", CodeInfo::funcInfo[index]->name, CodeInfo::funcInfo[index]->funcType->GetFullTypeName(), pos);
			CodeInfo::lastError = CompilerError(errBuf, NULL);
			return false;
		}

		fInfo++;
	}

	fInfo = FindFirstFunc(bCode);
	for(unsigned int i = 0; i < end; i++)
	{
		FunctionInfo *func = CodeInfo::funcInfo[oldFuncCount + i];
		// Handle only global visible functions
		if(!func->visible || func->type == FunctionInfo::LOCAL)
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
		CodeInfo::lastError = CompilerError("", NULL);
		SafeSprintf(CodeInfo::lastError.error, 256, "ERROR: module %s not found", altFile);
	}
	return NULL;
}

bool Compiler::Compile(const char* str, bool noClear)
{
	CodeInfo::nodeList.clear();
	NodeZeroOP::DeleteNodes();

	if(!noClear)
	{
		lexer.Clear();
		moduleSource.clear();
		dupStringsModule.Clear();
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
			CodeInfo::lastError = CompilerError("ERROR: string expected after import", start->pos);
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
				CodeInfo::lastError = CompilerError("ERROR: string expected after '.'", start->pos);
				return false;
			}
			cPath += SafeSprintf(cPath, 256 - int(cPath - path), "/%.*s", start->length, start->pos);
			start++;
		}
		if(start->type != lex_semicolon)
		{
			CodeInfo::lastError = CompilerError("ERROR: ';' not found after import expression", name->pos);
			return false;
		}
		start++;
		SafeSprintf(cPath, 256 - int(cPath - path), ".nc");
		const char *bytecode = BinaryCache::GetBytecode(path);
		if(!bytecode && importPath)
			bytecode = BinaryCache::GetBytecode(pathNoImport);

		if(moduleCount == 32)
		{
			CodeInfo::lastError = CompilerError("ERROR: temporary limit for 32 modules", name->pos);
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
			CodeInfo::lastError = CompilerError("ERROR: unexpected symbol", start->pos);
			return false;
		}
	}else{
		return false;
	}
	if(!res)
	{
		CodeInfo::lastError = CompilerError("ERROR: module contains no code", str + strlen(str));
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
			if(!func->visible || func->type == FunctionInfo::LOCAL)
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

					SelectTypeByPointer(NULL);
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
		return false;
	}

	CreateRedirectionTables();

	realGlobalCount = CodeInfo::varInfo.size();

	RestoreScopedGlobals();

	CodeInfo::cmdList.push_back(VMCmd(cmdJmp));
	for(unsigned int i = 0; i < CodeInfo::funcDefList.size(); i++)
	{
		CodeInfo::funcDefList[i]->Compile();
#ifdef NULLC_LOG_FILES
		CodeInfo::funcDefList[i]->LogToStream(fGraph);
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
	}
#ifdef NULLC_LOG_FILES
	fclose(fGraph);
#endif

	if(CodeInfo::nodeList.size() != 1)
	{
		CodeInfo::lastError = CompilerError("Compilation failed, AST contains more than one node", NULL);
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
	unsigned int line = 0, lastLine = ~0u;
	const char *lastSourcePos = CompilerError::codeStart;
	for(unsigned int i = 0; i < CodeInfo::cmdList.size(); i++)
	{
		while((line < CodeInfo::cmdInfoList.sourceInfo.size() - 1) && (i >= CodeInfo::cmdInfoList.sourceInfo[line + 1].byteCodePos))
			line++;
		if(line != lastLine)
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

	fprintf(fC, "#include \"runtime.h\"\r\n");
	fprintf(fC, "// Typeid redirect table\r\n");
	fprintf(fC, "static unsigned __nullcTR[%d];\r\n", CodeInfo::typeInfo.size());
	fprintf(fC, "// Array classes\r\n");
	for(unsigned int i = buildInTypes.size(); i < CodeInfo::typeInfo.size(); i++)
	{
		TypeInfo *type = translationTypes[i];
		if(!type || type->arrSize == TypeInfo::UNSIZED_ARRAY || type->refLevel || type->funcType)
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
			fprintf(fC, "};\r\n");
		}else{
			fprintf(fC, "typedef struct\r\n{\r\n");
			TypeInfo::MemberVariable *curr = type->firstVariable;
			for(; curr; curr = curr->next)
			{
				fprintf(fC, "\t");
				curr->type->OutputCType(fC, curr->name);
				fprintf(fC, ";\r\n");
			}
			fprintf(fC, "} %s;\r\n", type->name);
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
				sprintf(name, "%s%.*s_%d", namePrefix, local->name.end - local->name.begin-nameShift, local->name.begin+nameShift, local->pos);
			
				fprintf(fC, "__nullcUpvalue *__upvalue_%d_%s = 0;\r\n", CodeInfo::FindFunctionByPtr(local->parentFunction), name);
			}
		}
		if(info->type == FunctionInfo::LOCAL && info->closeUpvals)
		{
			fprintf(fC, "__nullcUpvalue *__upvalue_%d___", CodeInfo::FindFunctionByPtr(info));
			info->type = FunctionInfo::NORMAL;
			info->visible = true;
			OutputCFunctionName(fC, info);
			info->type = FunctionInfo::LOCAL;
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
				sprintf(name, "%s%.*s_%d", namePrefix, local->name.end - local->name.begin-nameShift, local->name.begin+nameShift, local->pos);
			
				fprintf(fC, "__nullcUpvalue *__upvalue_%d_%s = 0;\r\n", CodeInfo::FindFunctionByPtr(local->parentFunction), name);
			}
		}
	}
	for(unsigned int i = activeModules[0].funcCount; i < CodeInfo::funcInfo.size(); i++)
	{
		FunctionInfo *info = CodeInfo::funcInfo[i];
		if(i < functionsInModules)
			fprintf(fC, "extern ");
		info->retType->OutputCType(fC, "");

		OutputCFunctionName(fC, info);
		fprintf(fC, "(");

		char name[NULLC_MAX_VARIABLE_NAME_LENGTH];
		VariableInfo *param = info->firstParam;
		for(; param; param = param->next)
		{
			sprintf(name, "%.*s", param->name.end - param->name.begin, param->name.begin);
			param->varType->OutputCType(fC, name);
			fprintf(fC, ", ");
		}
		if(info->type == FunctionInfo::THISCALL)
		{
			info->parentClass->OutputCType(fC, "* __context");
		}else if(info->type == FunctionInfo::LOCAL){
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
		const char *namePrefix = *varInfo->name.begin == '$' ? "__" : "";
		unsigned int nameShift = *varInfo->name.begin == '$' ? 1 : 0;
		sprintf(vName, varInfo->blockDepth > 1 ? "%s%.*s_%d" : "%s%.*s", namePrefix, varInfo->name.end-varInfo->name.begin-nameShift, varInfo->name.begin+nameShift, varInfo->pos);
		if(varInfo->pos >> 24)
			fprintf(fC, "extern ");
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

	if(CodeInfo::nodeList.back())
	{
		fprintf(fC, "int %s()\r\n{\r\n", mainName);
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
			else
				fprintf(fC, "%d, NULLC_CLASS);\r\n", type->memberCount);
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
	fInfo.bytesToPop = 4;
#ifdef _M_X64
	fInfo.bytesToPop += 4;
#endif

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
			fInfo.rOffsets[rCount++] = offset | 3u << 30u;
			offset += type.size / 4;
			break;
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
				symbolStorageSize += (unsigned int)strlen(curr->name) + 1;
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

		typeInfo.defaultAlign = refType.alignBytes;

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

	return size;
}

