#include "stdafx.h"

#include "SupSpi/SupSpi.h"
using namespace supspi;

#include "CodeInfo.h"
using namespace CodeInfo;

#include "Bytecode.h"

#include "Compiler.h"

#include "Parser.h"
#include "Callbacks.h"

#include <time.h>

jmp_buf CodeInfo::errorHandler;
//////////////////////////////////////////////////////////////////////////
//						Code gen ops
//////////////////////////////////////////////////////////////////////////
#ifdef NULLC_LOG_FILES
FILE*			compileLog;
#endif

TypeInfo*	typeVoid = NULL;
TypeInfo*	typeChar = NULL;
TypeInfo*	typeShort = NULL;
TypeInfo*	typeInt = NULL;
TypeInfo*	typeFloat = NULL;
TypeInfo*	typeLong = NULL;
TypeInfo*	typeDouble = NULL;
TypeInfo*	typeFile = NULL;

unsigned int buildInFuncs;
unsigned int buildInTypes;

CompilerError::CompilerError(const char* errStr, const char* apprPos)
{
	Init(errStr, apprPos ? (apprPos-codeStart)+codeStartOriginal : NULL);
}

void CompilerError::Init(const char* errStr, const char* apprPos)
{
	empty = 0;
	unsigned int len = (unsigned int)strlen(errStr) < 128 ? (unsigned int)strlen(errStr) : 127;
	memcpy(error, errStr, len);
	error[len] = 0;
	if(apprPos)
	{
		const char *begin = apprPos;
		while((begin > codeStart) && (*begin != '\n') && (*begin != '\r'))
			begin--;
		if(begin < apprPos)
			begin++;

		lineNum = 1;
		const char *scan = codeStart;
		while(scan < begin)
			if(*(scan++) == '\n')
				lineNum++;

		const char *end = apprPos;
		while((*end != '\r') && (*end != '\n') && (*end != 0))
			end++;
		len = (unsigned int)(end - begin) < 128 ? (unsigned int)(end - begin) : 127;
		memcpy(line, begin, len);
		line[len] = 0;
		shift = (unsigned int)(apprPos-begin);
	}else{
		line[0] = 0;
		shift = 0;
		lineNum = 0;
	}
}

const char *CompilerError::codeStartOriginal = NULL;
const char *CompilerError::codeStart = NULL;

Compiler::Compiler()
{
	// Add types
	TypeInfo* info;
	info = new TypeInfo(strdup("void"), 0, 0, 1, NULL);
	info->size = 0;
	info->type = TypeInfo::TYPE_VOID;
	typeVoid = info;
	typeInfo.push_back(info);

	info = new TypeInfo(strdup("double"), 0, 0, 1, NULL);
	info->alignBytes = 8;
	info->size = 8;
	info->type = TypeInfo::TYPE_DOUBLE;
	typeDouble = info;
	typeInfo.push_back(info);

	info = new TypeInfo(strdup("float"), 0, 0, 1, NULL);
	info->alignBytes = 4;
	info->size = 4;
	info->type = TypeInfo::TYPE_FLOAT;
	typeFloat = info;
	typeInfo.push_back(info);

	info = new TypeInfo(strdup("long"), 0, 0, 1, NULL);
	info->size = 8;
	info->type = TypeInfo::TYPE_LONG;
	typeLong = info;
	typeInfo.push_back(info);

	info = new TypeInfo(strdup("int"), 0, 0, 1, NULL);
	info->alignBytes = 4;
	info->size = 4;
	info->type = TypeInfo::TYPE_INT;
	typeInt = info;
	typeInfo.push_back(info);

	info = new TypeInfo(strdup("short"), 0, 0, 1, NULL);
	info->size = 2;
	info->type = TypeInfo::TYPE_SHORT;
	typeShort = info;
	typeInfo.push_back(info);

	info = new TypeInfo(strdup("char"), 0, 0, 1, NULL);
	info->size = 1;
	info->type = TypeInfo::TYPE_CHAR;
	typeChar = info;
	typeInfo.push_back(info);

	info = new TypeInfo(strdup("float2"), 0, 0, 1, NULL);
	info->alignBytes = 4;
	info->type = TypeInfo::TYPE_COMPLEX;
	info->AddMember("x", typeFloat);
	info->AddMember("y", typeFloat);
	typeInfo.push_back(info);

	info = new TypeInfo(strdup("float3"), 0, 0, 1, NULL);
	info->alignBytes = 4;
	info->type = TypeInfo::TYPE_COMPLEX;
	info->AddMember("x", typeFloat);
	info->AddMember("y", typeFloat);
	info->AddMember("z", typeFloat);
	typeInfo.push_back(info);

	info = new TypeInfo(strdup("float4"), 0, 0, 1, NULL);
	info->alignBytes = 4;
	info->type = TypeInfo::TYPE_COMPLEX;
	info->AddMember("x", typeFloat);
	info->AddMember("y", typeFloat);
	info->AddMember("z", typeFloat);
	info->AddMember("w", typeFloat);
	typeInfo.push_back(info);

	TypeInfo *typeFloat4 = info;

	info = new TypeInfo(strdup("double2"), 0, 0, 1, NULL);
	info->alignBytes = 8;
	info->type = TypeInfo::TYPE_COMPLEX;
	info->AddMember("x", typeDouble);
	info->AddMember("y", typeDouble);
	typeInfo.push_back(info);

	info = new TypeInfo(strdup("double3"), 0, 0, 1, NULL);
	info->alignBytes = 8;
	info->type = TypeInfo::TYPE_COMPLEX;
	info->AddMember("x", typeDouble);
	info->AddMember("y", typeDouble);
	info->AddMember("z", typeDouble);
	typeInfo.push_back(info);

	info = new TypeInfo(strdup("double4"), 0, 0, 1, NULL);
	info->alignBytes = 8;
	info->type = TypeInfo::TYPE_COMPLEX;
	info->AddMember("x", typeDouble);
	info->AddMember("y", typeDouble);
	info->AddMember("z", typeDouble);
	info->AddMember("w", typeDouble);
	typeInfo.push_back(info);

	info = new TypeInfo(strdup("float4x4"), 0, 0, 1, NULL);
	info->alignBytes = 4;
	info->type = TypeInfo::TYPE_COMPLEX;
	info->AddMember("row1", typeFloat4);
	info->AddMember("row2", typeFloat4);
	info->AddMember("row3", typeFloat4);
	info->AddMember("row4", typeFloat4);
	typeInfo.push_back(info);

	info = new TypeInfo(strdup("file"), 0, 0, 1, NULL);
	info->size = 4;
	info->type = TypeInfo::TYPE_COMPLEX;
	typeFile = info;
	typeInfo.push_back(info);

	// Add functions
	FunctionInfo	*fInfo;
	fInfo = new FunctionInfo();
	fInfo->address = -1;
	fInfo->name = strdup("cos");
	fInfo->nameHash = GetStringHash(fInfo->name);
	fInfo->params.push_back(VariableInfo("deg", 0, typeDouble));
	fInfo->retType = typeDouble;
	fInfo->vTopSize = 1;
	fInfo->funcType = GetFunctionType(fInfo);
	funcInfo.push_back(fInfo);

	fInfo = new FunctionInfo();
	fInfo->address = -1;
	fInfo->name = strdup("sin");
	fInfo->nameHash = GetStringHash(fInfo->name);
	fInfo->params.push_back(VariableInfo("deg", 0, typeDouble));
	fInfo->retType = typeDouble;
	fInfo->vTopSize = 1;
	fInfo->funcType = GetFunctionType(fInfo);
	funcInfo.push_back(fInfo);

	fInfo = new FunctionInfo();
	fInfo->address = -1;
	fInfo->name = strdup("tan");
	fInfo->nameHash = GetStringHash(fInfo->name);
	fInfo->params.push_back(VariableInfo("deg", 0, typeDouble));
	fInfo->retType = typeDouble;
	fInfo->vTopSize = 1;
	fInfo->funcType = GetFunctionType(fInfo);
	funcInfo.push_back(fInfo);

	fInfo = new FunctionInfo();
	fInfo->address = -1;
	fInfo->name = strdup("ctg");
	fInfo->nameHash = GetStringHash(fInfo->name);
	fInfo->params.push_back(VariableInfo("deg", 0, typeDouble));
	fInfo->retType = typeDouble;
	fInfo->vTopSize = 1;
	fInfo->funcType = GetFunctionType(fInfo);
	funcInfo.push_back(fInfo);

	fInfo = new FunctionInfo();
	fInfo->address = -1;
	fInfo->name = strdup("ceil");
	fInfo->nameHash = GetStringHash(fInfo->name);
	fInfo->params.push_back(VariableInfo("deg", 0, typeDouble));
	fInfo->retType = typeDouble;
	fInfo->vTopSize = 1;
	fInfo->funcType = GetFunctionType(fInfo);
	funcInfo.push_back(fInfo);

	fInfo = new FunctionInfo();
	fInfo->address = -1;
	fInfo->name = strdup("floor");
	fInfo->nameHash = GetStringHash(fInfo->name);
	fInfo->params.push_back(VariableInfo("deg", 0, typeDouble));
	fInfo->retType = typeDouble;
	fInfo->vTopSize = 1;
	fInfo->funcType = GetFunctionType(fInfo);
	funcInfo.push_back(fInfo);

	fInfo = new FunctionInfo();
	fInfo->address = -1;
	fInfo->name = strdup("sqrt");
	fInfo->nameHash = GetStringHash(fInfo->name);
	fInfo->params.push_back(VariableInfo("deg", 0, typeDouble));
	fInfo->retType = typeDouble;
	fInfo->vTopSize = 1;
	fInfo->funcType = GetFunctionType(fInfo);
	funcInfo.push_back(fInfo);

	buildInTypes = (int)typeInfo.size();
	buildInFuncs = (int)funcInfo.size();

#ifdef NULLC_LOG_FILES
	compileLog = NULL;
#endif
}

Compiler::~Compiler()
{
	for(unsigned int i = 0; i < typeInfo.size(); i++)
	{
		delete typeInfo[i]->funcType;
		delete[] typeInfo[i]->name;
		delete[] typeInfo[i]->fullName;
		delete typeInfo[i];
	}
	for(unsigned int i = 0; i < funcInfo.size(); i++)
	{
		delete[] funcInfo[i]->name;
		delete funcInfo[i];
	}

	CallbackDeinitialize();

	varInfo.clear();
	funcInfo.clear();
	typeInfo.clear();

#ifdef NULLC_LOG_FILES
	if(compileLog)
		fclose(compileLog);
#endif
}

void Compiler::ClearState()
{
	varInfo.clear();

	for(unsigned int i = buildInTypes; i < typeInfo.size(); i++)
	{
		delete typeInfo[i]->funcType;
		delete[] typeInfo[i]->name;
		delete[] typeInfo[i]->fullName;
		delete typeInfo[i];
	}
	for(unsigned int i = buildInFuncs; i < funcInfo.size(); i++)
	{
		delete[] funcInfo[i]->name;
		delete funcInfo[i];
	}

	typeInfo.resize(buildInTypes);
	funcInfo.resize(buildInFuncs);

	nodeList.clear();

	ClearStringList();

	CallbackInitialize();

#ifdef NULLC_LOG_FILES
	if(compileLog)
		fclose(compileLog);
	compileLog = fopen("compilelog.txt", "wb");
#endif
	lastError = CompilerError();
}

bool Compiler::AddExternalFunction(void (NCDECL *ptr)(), const char* prototype)
{
	ClearState();

	bool res;

	lexer.Lexify(prototype);

	if(!setjmp(errorHandler))
	{
		Lexeme *start = lexer.GetStreamStart();
		res = ParseFunctionPrototype(&start);
	}else{
		lastError = CompilerError("Parsing failed", NULL);
		return false;
	}
	if(!res)
		return false;

	funcInfo.back()->address = -1;
	funcInfo.back()->funcPtr = (void*)ptr;

	buildInFuncs++;

	FunctionInfo	&lastFunc = *funcInfo.back();
	// Find out the function type
	TypeInfo	*bestFit = NULL;
	// Search through active types
	for(unsigned int i = 0; i < typeInfo.size(); i++)
	{
		if(typeInfo[i]->funcType)
		{
			if(typeInfo[i]->funcType->retType != lastFunc.retType)
				continue;
			if(typeInfo[i]->funcType->paramType.size() != lastFunc.params.size())
				continue;
			bool good = true;
			for(unsigned int n = 0; n < lastFunc.params.size(); n++)
			{
				if(lastFunc.params[n].varType != typeInfo[i]->funcType->paramType[n])
				{
					good = false;
					break;
				}
			}
			if(good)
			{
				bestFit = typeInfo[i];
				break;
			}
		}
	}
	// If none found, create new
	if(!bestFit)
	{
		FunctionType *funcType = new FunctionType();
		funcType->retType = lastFunc.retType;
		for(unsigned int n = 0; n < lastFunc.params.size(); n++)
			funcType->paramType.push_back(lastFunc.params[n].varType);

		typeInfo.push_back(new TypeInfo(NULL, 0, 0, 1, NULL, funcType));
		typeInfo.back()->size = 8;

		bestFit = typeInfo.back();
		bestFit->type = TypeInfo::TYPE_COMPLEX;
	}
	lastFunc.funcType = bestFit;

	buildInTypes = (int)typeInfo.size();

	return true;
}

bool Compiler::Compile(const char *str)
{
	ClearState();

	cmdInfoList->Clear();
	cmdList.clear();

#ifdef NULLC_LOG_FILES
	FILE *fCode = fopen("code.txt", "wb");
	fwrite(str.c_str(), 1, str.length(), fCode);
	fclose(fCode);
#endif

	CompilerError::codeStartOriginal = str;

#ifdef NULLC_LOG_FILES
	FILE *fTime = fopen("time.txt", "wb");
#endif

	CompilerError::codeStart = str;

	unsigned int t = clock();

	lexer.Lexify(str);

	bool res;

	if(!setjmp(errorHandler))
	{
		Lexeme *start = lexer.GetStreamStart();
		res = ParseCode(&start);
	}else{
		return false;
	}
	if(!res)
	{
		lastError = CompilerError("Parsing failed", NULL);
		return false;
	}

	unsigned int tem = clock()-t;
#ifdef NULLC_LOG_FILES
	fprintf(fTime, "Parsing and AST tree gen. time: %d ms\r\n", tem * 1000 / CLOCKS_PER_SEC);
#endif

	// Emulate global block end
	CodeInfo::globalSize = varTop;

	t = clock();
	for(unsigned int i = 0; i < funcDefList.size(); i++)
	{
		funcDefList[i]->Compile();
		((NodeFuncDef*)funcDefList[i])->Disable();
	}
	if(nodeList.back())
		nodeList.back()->Compile();
	tem = clock()-t;
#ifdef NULLC_LOG_FILES
	fprintf(fTime, "Compile time: %d ms\r\n", tem * 1000 / CLOCKS_PER_SEC);
	fclose(fTime);
#endif

#ifdef NULLC_LOG_FILES
	FILE *fGraph = fopen("graph.txt", "wb");
	for(unsigned int i = 0; i < funcDefList.size(); i++)
		funcDefList[i]->LogToStream(fGraph);
	if(nodeList.back())
		nodeList.back()->LogToStream(fGraph);
	fclose(fGraph);
#endif

#ifdef NULLC_LOG_FILES
	fprintf(compileLog, "\r\n%s", warningLog.c_str());

	fprintf(compileLog, "\r\nActive types (%d):\r\n", typeInfo.size());
	for(unsigned int i = 0; i < typeInfo.size(); i++)
		fprintf(compileLog, "%s (%d bytes)\r\n", typeInfo[i]->GetTypeName().c_str(), typeInfo[i]->size);

	fprintf(compileLog, "\r\nActive functions (%d):\r\n", funcInfo.size());
	for(unsigned int i = 0; i < funcInfo.size(); i++)
	{
		FunctionInfo &currFunc = *funcInfo[i];
		fprintf(compileLog, "%s", currFunc.type == FunctionInfo::LOCAL ? "local " : (currFunc.type == FunctionInfo::NORMAL ? "global " : "thiscall "));
		fprintf(compileLog, "%s %s(", currFunc.retType->GetTypeName().c_str(), currFunc.name.c_str());

		for(unsigned int n = 0; n < currFunc.params.size(); n++)
			fprintf(compileLog, "%s %s%s", currFunc.params[n].varType->GetTypeName().c_str(), currFunc.params[n].name.c_str(), (n==currFunc.params.size()-1 ? "" :", "));
		
		fprintf(compileLog, ")\r\n");
		if(currFunc.type == FunctionInfo::LOCAL)
		{
			for(unsigned int n = 0; n < currFunc.external.size(); n++)
				fprintf(compileLog, "  external var: %s\r\n", currFunc.external[n].c_str());
		}
	}
	fflush(compileLog);
#endif

	if(nodeList.back())
		nodeList.pop_back();
	NodeZeroOP::DeleteNodes();

	if(nodeList.size() != 0)
	{
		lastError = CompilerError("Compilation failed, AST contains more than one node", NULL);
		return false;
	}

	return true;
}

const char* Compiler::GetError()
{
	return lastError.GetErrorString();
}

void Compiler::SaveListing(const char *fileName)
{
#ifdef NULLC_LOG_FILES
	FILE *compiledAsm = fopen(fileName, "wb");
	char instBuf[128];
	for(unsigned int i = 0; i < CodeInfo::cmdList.size(); i++)
	{
		CodeInfo::cmdList[i].Decode(instBuf);
		fprintf(compiledAsm, "%s\r\n", instBuf);
	}
	fclose(compiledAsm);
#else
	(void)fileName;
#endif
}

unsigned int GetTypeIndexByPtr(TypeInfo* type)
{
	for(unsigned int n = 0; n < CodeInfo::typeInfo.size(); n++)
		if(CodeInfo::typeInfo[n] == type)
			return n;
	assert(!"type not found");
	return ~0u;
}

unsigned int Compiler::GetBytecode(char **bytecode)
{
	// find out the size of generated bytecode
	unsigned int size = sizeof(ByteCode);

	for(unsigned int i = 0; i < CodeInfo::typeInfo.size(); i++)
	{
		size += sizeof(ExternTypeInfo);
		size += (int)CodeInfo::typeInfo[i]->GetFullNameLength() + 1;
	}

	unsigned int offsetToVar = size;
	for(unsigned int i = 0; i < CodeInfo::varInfo.size(); i++)
	{
		size += sizeof(ExternVarInfo);
		size += (int)strlen(CodeInfo::varInfo[i]->name)+1;
	}

	unsigned int offsetToFunc = size;
	for(unsigned int i = 0; i < CodeInfo::funcInfo.size(); i++)
	{
		size += sizeof(ExternFuncInfo);
		size += (int)strlen(CodeInfo::funcInfo[i]->name)+1;
		size += (unsigned int)CodeInfo::funcInfo[i]->params.size() * sizeof(unsigned int);
	}
	unsigned int offsetToCode = size;
	size += CodeInfo::cmdList.size() * sizeof(VMCmd);

	*bytecode = new char[size];

	ByteCode	*code = (ByteCode*)(*bytecode);
	code->size = size;

	code->typeCount = (unsigned int)CodeInfo::typeInfo.size();

	code->globalVarSize = varTop;
	code->variableCount = (unsigned int)CodeInfo::varInfo.size();
	code->offsetToFirstVar = offsetToVar;

	code->functionCount = (unsigned int)CodeInfo::funcInfo.size();
	code->offsetToFirstFunc = offsetToFunc;

	code->codeSize = CodeInfo::cmdList.size();
	code->offsetToCode = offsetToCode;

	ExternTypeInfo *tInfo = FindFirstType(code);
	code->firstType = tInfo;
	for(unsigned int i = 0; i < CodeInfo::typeInfo.size(); i++)
	{
		tInfo->size = CodeInfo::typeInfo[i]->size;
		tInfo->nameLength = (unsigned int)CodeInfo::typeInfo[i]->GetFullNameLength();
		tInfo->structSize = sizeof(ExternTypeInfo) + tInfo->nameLength + 1;

		tInfo->type = (ExternTypeInfo::TypeCategory)CodeInfo::typeInfo[i]->type;
		// ! write name after the pointer to name
		char *namePtr = (char*)(&tInfo->name) + sizeof(tInfo->name);
		memcpy(namePtr, CodeInfo::typeInfo[i]->GetFullTypeName(), tInfo->nameLength+1);
		tInfo->name = namePtr;

		if(i+1 == CodeInfo::typeInfo.size())
			tInfo->next = NULL;
		else
			tInfo->next = FindNextType(tInfo);

		// Fill up next
		tInfo = tInfo->next;
	}

	ExternVarInfo *varInfo = FindFirstVar(code);
	code->firstVar = varInfo;
	for(unsigned int i = 0; i < CodeInfo::varInfo.size(); i++)
	{
		varInfo->size = CodeInfo::varInfo[i]->varType->size;
		varInfo->nameLength = (unsigned int)strlen(CodeInfo::varInfo[i]->name);
		varInfo->structSize = sizeof(ExternVarInfo) + varInfo->nameLength + 1;

		varInfo->type = GetTypeIndexByPtr(CodeInfo::varInfo[i]->varType);

		// ! write name after the pointer to name
		char *namePtr = (char*)(&varInfo->name) + sizeof(varInfo->name);
		memcpy(namePtr, CodeInfo::varInfo[i]->name, varInfo->nameLength+1);
		varInfo->name = namePtr;

		if(i+1 == CodeInfo::varInfo.size())
			varInfo->next = NULL;
		else
			varInfo->next = FindNextVar(varInfo);

		// Fill up next
		varInfo = varInfo->next;
	}

	unsigned int offsetToGlobal = 0;
	ExternFuncInfo *fInfo = FindFirstFunc(code);
	code->firstFunc = fInfo;
	for(unsigned int i = 0; i < CodeInfo::funcInfo.size(); i++)
	{
		fInfo->oldAddress = fInfo->address = CodeInfo::funcInfo[i]->address;
		fInfo->codeSize = CodeInfo::funcInfo[i]->codeSize;
		fInfo->funcPtr = CodeInfo::funcInfo[i]->funcPtr;
		fInfo->isVisible = CodeInfo::funcInfo[i]->visible;
		fInfo->funcType = (ExternFuncInfo::FunctionType)CodeInfo::funcInfo[i]->type;

		offsetToGlobal += fInfo->codeSize;

		fInfo->nameHash = CodeInfo::funcInfo[i]->nameHash;
		fInfo->nameLength = (unsigned int)strlen(CodeInfo::funcInfo[i]->name);

		fInfo->retType = GetTypeIndexByPtr(CodeInfo::funcInfo[i]->retType);
		fInfo->paramCount = (unsigned int)CodeInfo::funcInfo[i]->params.size();
		fInfo->paramList = (unsigned int*)((char*)(&fInfo->name) + sizeof(fInfo->name) + fInfo->nameLength + 1);

		fInfo->structSize = sizeof(ExternFuncInfo) + fInfo->nameLength + 1 + fInfo->paramCount * sizeof(unsigned int);

		for(unsigned int n = 0; n < fInfo->paramCount; n++)
			fInfo->paramList[n] = GetTypeIndexByPtr(CodeInfo::funcInfo[i]->params[n].varType);

		// ! write name after the pointer to name
		char *namePtr = (char*)(&fInfo->name) + sizeof(fInfo->name);
		memcpy(namePtr, CodeInfo::funcInfo[i]->name, fInfo->nameLength+1);
		fInfo->name = namePtr;

		if(i+1 == CodeInfo::funcInfo.size())
			fInfo->next = NULL;
		else
			fInfo->next = FindNextFunc(fInfo);

		// Fill up next
		fInfo = fInfo->next;
	}

	code->code = FindCode(code);
	code->globalCodeStart = offsetToGlobal;
	memcpy(code->code, &CodeInfo::cmdList[0], CodeInfo::cmdList.size() * sizeof(VMCmd));

	return size;
}

