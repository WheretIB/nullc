#include "stdafx.h"

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
	Init(errStr, apprPos ? apprPos : NULL);
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
		while((begin >= codeStart) && (*begin != '\n') && (*begin != '\r'))
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

const char *CompilerError::codeStart = NULL;

Compiler::Compiler()
{
	// Add types
	TypeInfo* info;
	info = new TypeInfo(typeInfo.size(), "void", 0, 0, 1, NULL, TypeInfo::TYPE_VOID);
	info->size = 0;
	typeVoid = info;
	typeInfo.push_back(info);

	info = new TypeInfo(typeInfo.size(), "double", 0, 0, 1, NULL, TypeInfo::TYPE_DOUBLE);
	info->alignBytes = 8;
	info->size = 8;
	typeDouble = info;
	typeInfo.push_back(info);

	info = new TypeInfo(typeInfo.size(), "float", 0, 0, 1, NULL, TypeInfo::TYPE_FLOAT);
	info->alignBytes = 4;
	info->size = 4;
	typeFloat = info;
	typeInfo.push_back(info);

	info = new TypeInfo(typeInfo.size(), "long", 0, 0, 1, NULL, TypeInfo::TYPE_LONG);
	info->size = 8;
	typeLong = info;
	typeInfo.push_back(info);

	info = new TypeInfo(typeInfo.size(), "int", 0, 0, 1, NULL, TypeInfo::TYPE_INT);
	info->alignBytes = 4;
	info->size = 4;
	typeInt = info;
	typeInfo.push_back(info);

	info = new TypeInfo(typeInfo.size(), "short", 0, 0, 1, NULL, TypeInfo::TYPE_SHORT);
	info->size = 2;
	typeShort = info;
	typeInfo.push_back(info);

	info = new TypeInfo(typeInfo.size(), "char", 0, 0, 1, NULL, TypeInfo::TYPE_CHAR);
	info->size = 1;
	typeChar = info;
	typeInfo.push_back(info);

	info = new TypeInfo(typeInfo.size(), "float2", 0, 0, 1, NULL, TypeInfo::TYPE_COMPLEX);
	info->alignBytes = 4;
	info->AddMemberVariable("x", typeFloat);
	info->AddMemberVariable("y", typeFloat);
	typeInfo.push_back(info);

	info = new TypeInfo(typeInfo.size(), "float3", 0, 0, 1, NULL, TypeInfo::TYPE_COMPLEX);
	info->alignBytes = 4;
	info->AddMemberVariable("x", typeFloat);
	info->AddMemberVariable("y", typeFloat);
	info->AddMemberVariable("z", typeFloat);
	typeInfo.push_back(info);

	info = new TypeInfo(typeInfo.size(), "float4", 0, 0, 1, NULL, TypeInfo::TYPE_COMPLEX);
	info->alignBytes = 4;
	info->AddMemberVariable("x", typeFloat);
	info->AddMemberVariable("y", typeFloat);
	info->AddMemberVariable("z", typeFloat);
	info->AddMemberVariable("w", typeFloat);
	typeInfo.push_back(info);

	TypeInfo *typeFloat4 = info;

	info = new TypeInfo(typeInfo.size(), "double2", 0, 0, 1, NULL, TypeInfo::TYPE_COMPLEX);
	info->alignBytes = 8;
	info->AddMemberVariable("x", typeDouble);
	info->AddMemberVariable("y", typeDouble);
	typeInfo.push_back(info);

	info = new TypeInfo(typeInfo.size(), "double3", 0, 0, 1, NULL, TypeInfo::TYPE_COMPLEX);
	info->alignBytes = 8;
	info->AddMemberVariable("x", typeDouble);
	info->AddMemberVariable("y", typeDouble);
	info->AddMemberVariable("z", typeDouble);
	typeInfo.push_back(info);

	info = new TypeInfo(typeInfo.size(), "double4", 0, 0, 1, NULL, TypeInfo::TYPE_COMPLEX);
	info->alignBytes = 8;
	info->AddMemberVariable("x", typeDouble);
	info->AddMemberVariable("y", typeDouble);
	info->AddMemberVariable("z", typeDouble);
	info->AddMemberVariable("w", typeDouble);
	typeInfo.push_back(info);

	info = new TypeInfo(typeInfo.size(), "float4x4", 0, 0, 1, NULL, TypeInfo::TYPE_COMPLEX);
	info->alignBytes = 4;
	info->AddMemberVariable("row1", typeFloat4);
	info->AddMemberVariable("row2", typeFloat4);
	info->AddMemberVariable("row3", typeFloat4);
	info->AddMemberVariable("row4", typeFloat4);
	typeInfo.push_back(info);

	info = new TypeInfo(typeInfo.size(), "file", 0, 0, 1, NULL, TypeInfo::TYPE_COMPLEX);
	info->size = 4;
	typeFile = info;
	typeInfo.push_back(info);

	// Add functions
	FunctionInfo	*fInfo;
	fInfo = new FunctionInfo("cos");
	fInfo->address = -1;
	fInfo->AddParameter(new VariableInfo(InplaceStr("deg"), GetStringHash("deg"), 0, typeDouble));
	fInfo->retType = typeDouble;
	fInfo->vTopSize = 1;
	fInfo->funcType = GetFunctionType(fInfo);
	funcInfo.push_back(fInfo);

	fInfo = new FunctionInfo("sin");
	fInfo->address = -1;
	fInfo->AddParameter(new VariableInfo(InplaceStr("deg"), GetStringHash("deg"), 0, typeDouble));
	fInfo->retType = typeDouble;
	fInfo->vTopSize = 1;
	fInfo->funcType = GetFunctionType(fInfo);
	funcInfo.push_back(fInfo);

	fInfo = new FunctionInfo("tan");
	fInfo->address = -1;
	fInfo->AddParameter(new VariableInfo(InplaceStr("deg"), GetStringHash("deg"), 0, typeDouble));
	fInfo->retType = typeDouble;
	fInfo->vTopSize = 1;
	fInfo->funcType = GetFunctionType(fInfo);
	funcInfo.push_back(fInfo);

	fInfo = new FunctionInfo("ctg");
	fInfo->address = -1;
	fInfo->AddParameter(new VariableInfo(InplaceStr("deg"), GetStringHash("deg"), 0, typeDouble));
	fInfo->retType = typeDouble;
	fInfo->vTopSize = 1;
	fInfo->funcType = GetFunctionType(fInfo);
	funcInfo.push_back(fInfo);

	fInfo = new FunctionInfo("ceil");
	fInfo->address = -1;
	fInfo->AddParameter(new VariableInfo(InplaceStr("deg"), GetStringHash("deg"), 0, typeDouble));
	fInfo->retType = typeDouble;
	fInfo->vTopSize = 1;
	fInfo->funcType = GetFunctionType(fInfo);
	funcInfo.push_back(fInfo);

	fInfo = new FunctionInfo("floor");
	fInfo->address = -1;
	fInfo->AddParameter(new VariableInfo(InplaceStr("deg"), GetStringHash("deg"), 0, typeDouble));
	fInfo->retType = typeDouble;
	fInfo->vTopSize = 1;
	fInfo->funcType = GetFunctionType(fInfo);
	funcInfo.push_back(fInfo);

	fInfo = new FunctionInfo("sqrt");
	fInfo->address = -1;
	fInfo->AddParameter(new VariableInfo(InplaceStr("deg"), GetStringHash("deg"), 0, typeDouble));
	fInfo->retType = typeDouble;
	fInfo->vTopSize = 1;
	fInfo->funcType = GetFunctionType(fInfo);
	funcInfo.push_back(fInfo);

	buildInTypes = (int)typeInfo.size();
	TypeInfo::SaveBuildinTop();
	VariableInfo::SaveBuildinTop();
	buildInFuncs = (int)funcInfo.size();

#ifdef NULLC_LOG_FILES
	compileLog = NULL;
#endif
}

Compiler::~Compiler()
{
	for(unsigned int i = 0; i < typeInfo.size(); i++)
		typeInfo[i]->fullName = NULL;
	for(unsigned int i = 0; i < funcInfo.size(); i++)
	{
		if(funcInfo[i]->address == -1 && funcInfo[i]->funcPtr != NULL)
			delete[] funcInfo[i]->name;
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

	for(unsigned int i = 0; i < buildInTypes; i++)
		if(typeInfo[i]->refType && typeInfo[i]->refType->typeIndex >= buildInTypes)
			typeInfo[i]->refType = NULL;

	for(unsigned int i = buildInTypes; i < typeInfo.size(); i++)
		typeInfo[i]->fullName = NULL;

	typeInfo.resize(buildInTypes);
	TypeInfo::DeleteTypeInformation();

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
//return true;
	funcInfo.back()->name = DuplicateString(funcInfo.back()->name);
	funcInfo.back()->address = -1;
	funcInfo.back()->funcPtr = (void*)ptr;

	buildInFuncs++;

	funcInfo.back()->funcType = CodeInfo::GetFunctionType(funcInfo.back());

	buildInTypes = (int)typeInfo.size();
	TypeInfo::SaveBuildinTop();
	VariableInfo::SaveBuildinTop();

	return true;
}

bool Compiler::Compile(const char *str)
{
	ClearState();

	cmdInfoList.Clear();
	cmdList.clear();

#ifdef NULLC_LOG_FILES
	FILE *fCode = fopen("code.txt", "wb");
	fwrite(str.c_str(), 1, str.length(), fCode);
	fclose(fCode);
#endif

#ifdef NULLC_LOG_FILES
	FILE *fTime = fopen("time.txt", "wb");
#endif

	CompilerError::codeStart = str;
	cmdInfoList.SetSourceStart(str);

	unsigned int t = clock();

	lexer.Lexify(str);
//return true;
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
//return true;
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
		size += sizeof(ExternTypeInfo);

	unsigned int offsetToVar = size;
	for(unsigned int i = 0; i < CodeInfo::varInfo.size(); i++)
		size += sizeof(ExternVarInfo);

	unsigned int offsetToFunc = size;
	for(unsigned int i = 0; i < CodeInfo::funcInfo.size(); i++)
	{
		size += sizeof(ExternFuncInfo);
		size += CodeInfo::funcInfo[i]->nameLength + 1;
		size += (unsigned int)CodeInfo::funcInfo[i]->paramCount * sizeof(unsigned int);
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
		tInfo->structSize = sizeof(ExternTypeInfo);

		tInfo->size = CodeInfo::typeInfo[i]->size;
		tInfo->type = (ExternTypeInfo::TypeCategory)CodeInfo::typeInfo[i]->type;
		tInfo->nameHash = CodeInfo::typeInfo[i]->GetFullNameHash();

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
		varInfo->structSize = sizeof(ExternVarInfo);

		varInfo->type = GetTypeIndexByPtr(CodeInfo::varInfo[i]->varType);
		varInfo->nameHash = GetStringHash(CodeInfo::varInfo[i]->name.begin, CodeInfo::varInfo[i]->name.end);

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

		fInfo->retType = GetTypeIndexByPtr(CodeInfo::funcInfo[i]->retType);
		fInfo->paramCount = (unsigned int)CodeInfo::funcInfo[i]->paramCount;
		fInfo->paramList = (unsigned int*)((char*)(&fInfo->nameHash) + sizeof(fInfo->nameHash));

		fInfo->structSize = sizeof(ExternFuncInfo) + fInfo->paramCount * sizeof(unsigned int);

		unsigned int n = 0;
		for(VariableInfo *curr = CodeInfo::funcInfo[i]->firstParam; curr; curr = curr->next, n++)
			fInfo->paramList[n] = curr->varType->typeIndex;

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

