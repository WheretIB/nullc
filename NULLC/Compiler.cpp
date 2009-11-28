#include "stdafx.h"

#include "CodeInfo.h"

#include "Bytecode.h"

#include "Compiler.h"

#include "Parser.h"
#include "Callbacks.h"

#include "StdLib.h"

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

CompilerError::CompilerError(const char* errStr, const char* apprPos)
{
	Init(errStr, apprPos ? apprPos : NULL);
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
			if(line[k] < 0x20)
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

Compiler::Compiler()
{
	buildInFuncs = 0;
	buildInTypes = 0;

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
	info->size = 2;
	typeShort = info;
	CodeInfo::typeInfo.push_back(info);

	info = new TypeInfo(CodeInfo::typeInfo.size(), "char", 0, 0, 1, NULL, TypeInfo::TYPE_CHAR);
	info->size = 1;
	typeChar = info;
	CodeInfo::typeInfo.push_back(info);

	basicTypes = buildInTypes = (int)CodeInfo::typeInfo.size();
	TypeInfo::SaveBuildinTop();

	// Add complex types
	AddType("align(4) class float2{ float x, y; }");
	AddType("align(4) class float3{ float x, y, z; }");
	AddType("align(4) class float4{ float x, y, z, w; }");

	AddType("align(8) class double2{ double x, y; }");
	AddType("align(8) class double3{ double x, y, z; }");
	AddType("align(8) class double4{ double x, y, z, w; }");

	AddType("align(4) class float4x4{ float4 row1, row2, row3, row4; }");

	AddType("class file{ int id; }");

	AddExternalFunction((void (*)())NULLC::Assert, "void assert(int val);");

	AddExternalFunction((void (*)())NULLC::Cos, "double cos(double deg);");
	AddExternalFunction((void (*)())NULLC::Sin, "double sin(double deg);");
	AddExternalFunction((void (*)())NULLC::Tan, "double tan(double deg);");
	AddExternalFunction((void (*)())NULLC::Cosh, "double cosh(double deg);");
	AddExternalFunction((void (*)())NULLC::Sinh, "double sinh(double deg);");
	AddExternalFunction((void (*)())NULLC::Tanh, "double tanh(double deg);");
	AddExternalFunction((void (*)())NULLC::Acos, "double acos(double deg);");
	AddExternalFunction((void (*)())NULLC::Asin, "double asin(double deg);");
	AddExternalFunction((void (*)())NULLC::Atan, "double atan(double deg);");
	AddExternalFunction((void (*)())NULLC::Ctg, "double ctg(double deg);");

	AddExternalFunction((void (*)())NULLC::Ceil, "double ceil(double num);");
	AddExternalFunction((void (*)())NULLC::Floor, "double floor(double num);");
	AddExternalFunction((void (*)())NULLC::Exp, "double exp(double num);");
	AddExternalFunction((void (*)())NULLC::Log, "double log(double num);");

	AddExternalFunction((void (*)())NULLC::Sqrt, "double sqrt(double num);");

	AddExternalFunction((void (*)())NULLC::StrEqual, "int operator ==(char[] a, b);");
	AddExternalFunction((void (*)())NULLC::StrNEqual, "int operator !=(char[] a, b);");
	AddExternalFunction((void (*)())NULLC::StrConcatenate, "char[] operator +(char[] a, b);");
	AddExternalFunction((void (*)())NULLC::StrConcatenateAndSet, "char[] operator +=(char[] ref a, char[] b);");

	AddExternalFunction((void (*)())NULLC::Int, "char char(char a);");
	AddExternalFunction((void (*)())NULLC::Int, "short short(short a);");
	AddExternalFunction((void (*)())NULLC::Int, "int int(int a);");
	AddExternalFunction((void (*)())NULLC::Long, "long long(long a);");
	AddExternalFunction((void (*)())NULLC::Float, "float float(float a);");
	AddExternalFunction((void (*)())NULLC::Double, "double double(double a);");

	AddExternalFunction((void (*)())NULLC::AllocObject, "int __newS(int size);");
	AddExternalFunction((void (*)())NULLC::AllocArray, "int[] __newA(int size, int count);");

#ifdef NULLC_LOG_FILES
	compileLog = NULL;
#endif
}

Compiler::~Compiler()
{
	ParseReset();

	CallbackDeinitialize();
	CallbackReset();

	CodeInfo::varInfo.clear();
	CodeInfo::funcInfo.clear();
	CodeInfo::typeInfo.clear();

	NodeBreakOp::fixQueue.reset();
	NodeContinueOp::fixQueue.reset();
	NodeSwitchExpr::fixQueue.reset();

	NodeZeroOP::ResetNodes();

#ifdef NULLC_LOG_FILES
	if(compileLog)
		fclose(compileLog);
#endif
}

void Compiler::ClearState()
{
	CodeInfo::varInfo.clear();

	for(unsigned int i = 0; i < buildInTypes; i++)
	{
		if(CodeInfo::typeInfo[i]->refType && CodeInfo::typeInfo[i]->refType->typeIndex >= buildInTypes)
			CodeInfo::typeInfo[i]->refType = NULL;
	}

	for(unsigned int i = 0; i < CodeInfo::typeInfo.size(); i++)
		CodeInfo::typeInfo[i]->fullName = NULL;

	CodeInfo::typeInfo.resize(buildInTypes);
	TypeInfo::DeleteTypeInformation();

	CodeInfo::funcInfo.resize(buildInFuncs);
	FunctionInfo::DeleteFunctionInformation();

	CodeInfo::nodeList.clear();

	ClearStringList();

	CallbackInitialize();

#ifdef NULLC_LOG_FILES
	if(compileLog)
		fclose(compileLog);
	compileLog = fopen("compilelog.txt", "wb");
#endif
	CodeInfo::lastError = CompilerError();
}

bool Compiler::AddExternalFunction(void (NCDECL *ptr)(), const char* prototype)
{
	ClearState();

	bool res;

	lexer.Lexify(prototype);

	if(!setjmp(CodeInfo::errorHandler))
	{
		Lexeme *start = lexer.GetStreamStart();
		res = ParseFunctionDefinition(&start);
	}else{
		CodeInfo::lastError = CompilerError("Parsing failed", NULL);
		return false;
	}
	if(!res)
		return false;

	CodeInfo::funcInfo.back()->name = strcpy((char*)dupStrings.Allocate((unsigned int)strlen(CodeInfo::funcInfo.back()->name) + 1), CodeInfo::funcInfo.back()->name);
	CodeInfo::funcInfo.back()->address = -1;
	CodeInfo::funcInfo.back()->funcPtr = (void*)ptr;
	CodeInfo::funcInfo.back()->implemented = true;

	buildInFuncs = CodeInfo::funcInfo.size();
	buildInTypes = CodeInfo::typeInfo.size();
	TypeInfo::SaveBuildinTop();
	VariableInfo::SaveBuildinTop();
	FunctionInfo::SaveBuildinTop();

	return true;
}

bool Compiler::AddType(const char* typedecl)
{
	ClearState();

	bool res;

	lexer.Lexify(typedecl);

	if(!setjmp(CodeInfo::errorHandler))
	{
		Lexeme *start = lexer.GetStreamStart();
		res = ParseClassDefinition(&start);
	}else{
		CodeInfo::lastError = CompilerError("Parsing failed", NULL);
		return false;
	}
	if(!res)
		return false;

	TypeInfo *definedType = CodeInfo::typeInfo[buildInTypes];
	definedType->name = strcpy((char*)dupStrings.Allocate((unsigned int)strlen(definedType->name) + 1), definedType->name);
	TypeInfo::MemberVariable	*currV = definedType->firstVariable;
	while(currV)
	{
		currV->name = strcpy((char*)dupStrings.Allocate((unsigned int)strlen(currV->name) + 1), currV->name);
		currV = currV->next;
	}
	TypeInfo::MemberFunction	*currF = definedType->firstFunction;
	while(currF)
	{
		currF->func->name = strcpy((char*)dupStrings.Allocate((unsigned int)strlen(currF->func->name) + 1), currF->func->name);
		currF = currF->next;
	}

	buildInTypes = (int)CodeInfo::typeInfo.size();
	TypeInfo::SaveBuildinTop();
	VariableInfo::SaveBuildinTop();
	FunctionInfo::SaveBuildinTop();

	return true;
}

bool Compiler::Compile(const char *str)
{
	ClearState();

	CodeInfo::cmdInfoList.Clear();
	CodeInfo::cmdList.clear();

	CompilerError::codeStart = str;
	CodeInfo::cmdInfoList.SetSourceStart(str);

	unsigned int t = clock();

	lexer.Lexify(str);

	bool res;

	if(!setjmp(CodeInfo::errorHandler))
	{
		Lexeme *start = lexer.GetStreamStart();
		res = ParseCode(&start);
		if(start->type != lex_none)
		{
			CodeInfo::lastError = CompilerError("Unexpected symbol", start->pos);
			return false;
		}
	}else{
		return false;
	}
	if(!res)
	{
		CodeInfo::lastError = CompilerError("Parsing failed", NULL);
		return false;
	}

	unsigned int tem = clock()-t;
#ifdef NULLC_LOG_FILES
	FILE *fTime = fopen("time.txt", "wb");
	fprintf(fTime, "Parsing and AST tree gen. time: %d ms\r\n", tem * 1000 / CLOCKS_PER_SEC);
#endif

#ifdef NULLC_LOG_FILES
	FILE *fGraph = fopen("graph.txt", "wb");
#endif

	t = clock();
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
		CodeInfo::nodeList.back()->Compile();

	CodeInfo::cmdInfoList.FindLineNumbers();

	tem = clock()-t;
#ifdef NULLC_LOG_FILES
	fprintf(fTime, "Compile time: %d ms\r\n", tem * 1000 / CLOCKS_PER_SEC);
	fclose(fTime);
#endif

#ifdef NULLC_LOG_FILES
	//for(unsigned int i = 0; i < CodeInfo::funcDefList.size(); i++)
	//	CodeInfo::funcDefList[i]->LogToStream(fGraph);
	if(CodeInfo::nodeList.back())
		CodeInfo::nodeList.back()->LogToStream(fGraph);
	fclose(fGraph);
#endif

#ifdef NULLC_LOG_FILES
	fprintf(compileLog, "\r\nActive types (%d):\r\n", CodeInfo::typeInfo.size());
	for(unsigned int i = 0; i < CodeInfo::typeInfo.size(); i++)
		fprintf(compileLog, "%s (%d bytes)\r\n", CodeInfo::typeInfo[i]->GetFullTypeName(), CodeInfo::typeInfo[i]->size);

	fprintf(compileLog, "\r\nActive functions (%d):\r\n", CodeInfo::funcInfo.size());
	for(unsigned int i = 0; i < CodeInfo::funcInfo.size(); i++)
	{
		FunctionInfo &currFunc = *CodeInfo::funcInfo[i];
		fprintf(compileLog, "%s", currFunc.type == FunctionInfo::LOCAL ? "local " : (currFunc.type == FunctionInfo::NORMAL ? "global " : "thiscall "));
		fprintf(compileLog, "%s %s(", currFunc.retType->GetFullTypeName(), currFunc.name);

		for(VariableInfo *curr = currFunc.firstParam; curr; curr = curr->next)
			fprintf(compileLog, "%s %.*s%s", curr->varType->GetFullTypeName(), curr->name.end-curr->name.begin, curr->name.begin, (curr == currFunc.lastParam ? "" : ", "));
		
		fprintf(compileLog, ")\r\n");
		if(currFunc.type == FunctionInfo::LOCAL)
		{
			for(FunctionInfo::ExternalInfo *curr = currFunc.firstExternal; curr; curr = curr->next)
				fprintf(compileLog, "  external var: %.*s\r\n", curr->variable->name.end-curr->variable->name.begin, curr->variable->name.begin);
		}
	}
	fflush(compileLog);
#endif

	if(CodeInfo::nodeList.back())
		CodeInfo::nodeList.pop_back();
	NodeZeroOP::DeleteNodes();

	if(CodeInfo::nodeList.size() != 0)
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
			if(CodeInfo::cmdInfoList.sourceInfo[line].sourceEnd > lastSourcePos)
			{
				fprintf(compiledAsm, "%.*s\r\n", CodeInfo::cmdInfoList.sourceInfo[line].sourceEnd - lastSourcePos, lastSourcePos);
				lastSourcePos = CodeInfo::cmdInfoList.sourceInfo[line].sourceEnd;
			}else{
				fprintf(compiledAsm, "%.*s\r\n", CodeInfo::cmdInfoList.sourceInfo[line].sourceEnd - CodeInfo::cmdInfoList.sourceInfo[line].sourcePos, CodeInfo::cmdInfoList.sourceInfo[line].sourcePos);
			}
		}
		CodeInfo::cmdList[i].Decode(instBuf);
		if((CodeInfo::cmdList[i].cmd == cmdCall || CodeInfo::cmdList[i].cmd == cmdCallStd) && CodeInfo::cmdList[i].argument != -1)
			fprintf(compiledAsm, "// %d %s (%s)\r\n", i, instBuf, CodeInfo::funcInfo[CodeInfo::cmdList[i].argument]->name);
		else
			fprintf(compiledAsm, "// %d %s\r\n", i, instBuf);
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

bool CreateExternalInfo(ExternFuncInfo &fInfo, FunctionInfo &refFunc)
{
	fInfo.bytesToPop = 0;
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
		const TypeInfo& type = *curr->varType;

		switch(type.type)
		{
		case TypeInfo::TYPE_CHAR:
		case TypeInfo::TYPE_SHORT:
		case TypeInfo::TYPE_INT:
			if(rCount >= rMaxCount)	// too many r parameters
			{
				fInfo.ps3Callable = 0;
				return false;
			}
			fInfo.rOffsets[rCount++] = offset;
			offset++;
			break;

		case TypeInfo::TYPE_FLOAT:
		case TypeInfo::TYPE_DOUBLE:
			if(fCount >= fMaxCount || rCount >= rMaxCount) // too many f/r parameters
			{
				fInfo.ps3Callable = 0;
				return false;
			}
			fInfo.rOffsets[rCount++] = offset;
			fInfo.fOffsets[fCount++] = offset;
			offset += type.type == TypeInfo::TYPE_DOUBLE ? 2 : 1;
			break;

		default:
			fInfo.ps3Callable = 0; // unsupported type
			return false; 
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
	for(unsigned int i = 0; i < CodeInfo::typeInfo.size(); i++)
	{
		symbolStorageSize += CodeInfo::typeInfo[i]->GetFullNameLength() + 1;
	}

	unsigned int offsetToVar = size;
	size += CodeInfo::varInfo.size() * sizeof(ExternVarInfo);

	unsigned int offsetToFunc = size;
	size += CodeInfo::funcInfo.size() * sizeof(ExternFuncInfo);

	unsigned int offsetToFirstLocal = size;

	unsigned int localCount = 0;
	for(unsigned int i = 0; i < CodeInfo::funcInfo.size(); i++)
	{
		FunctionInfo	*func = CodeInfo::funcInfo[i];
		symbolStorageSize += func->nameLength + 1;

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

	unsigned int offsetToCode = size;
	size += CodeInfo::cmdList.size() * sizeof(VMCmd);

	unsigned int offsetToSymbols = size;
	size += symbolStorageSize;

	*bytecode = new char[size];

	ByteCode	*code = (ByteCode*)(*bytecode);
	code->size = size;

	code->typeCount = (unsigned int)CodeInfo::typeInfo.size();

	code->globalVarSize = GetGlobalSize();
	code->variableCount = (unsigned int)CodeInfo::varInfo.size();
	code->offsetToFirstVar = offsetToVar;

	code->functionCount = (unsigned int)CodeInfo::funcInfo.size();
	code->offsetToFirstFunc = offsetToFunc;

	code->localCount = localCount;
	code->offsetToLocals = offsetToFirstLocal;
	code->firstLocal = (ExternLocalInfo*)((char*)(code) + code->offsetToLocals);

	code->codeSize = CodeInfo::cmdList.size();
	code->offsetToCode = offsetToCode;

	code->symbolLength = symbolStorageSize;
	code->offsetToSymbols = offsetToSymbols;
	code->debugSymbols = (*bytecode) + offsetToSymbols;

	char*	symbolPos = code->debugSymbols;

	ExternTypeInfo *tInfo = FindFirstType(code);
	code->firstType = tInfo;
	for(unsigned int i = 0; i < CodeInfo::typeInfo.size(); i++)
	{
		ExternTypeInfo &typeInfo = *tInfo;
		TypeInfo &refType = *CodeInfo::typeInfo[i];

		typeInfo.offsetToName = int(symbolPos - code->debugSymbols);
		memcpy(symbolPos, refType.GetFullTypeName(), refType.GetFullNameLength() + 1);
		symbolPos += refType.GetFullNameLength() + 1;

		typeInfo.size = CodeInfo::typeInfo[i]->size;
		typeInfo.type = (ExternTypeInfo::TypeCategory)CodeInfo::typeInfo[i]->type;
		typeInfo.nameHash = CodeInfo::typeInfo[i]->GetFullNameHash();

		// Fill up next
		tInfo++;
	}

	ExternVarInfo *vInfo = FindFirstVar(code);
	code->firstVar = vInfo;
	for(unsigned int i = 0; i < CodeInfo::varInfo.size(); i++)
	{
		ExternVarInfo &varInfo = *vInfo;

		varInfo.size = CodeInfo::varInfo[i]->varType->size;
		varInfo.type = GetTypeIndexByPtr(CodeInfo::varInfo[i]->varType);
		varInfo.nameHash = GetStringHash(CodeInfo::varInfo[i]->name.begin, CodeInfo::varInfo[i]->name.end);

		// Fill up next
		vInfo++;
	}

	unsigned int localOffset = 0;
	unsigned int offsetToGlobal = 0;
	ExternFuncInfo *fInfo = FindFirstFunc(code);
	code->firstFunc = fInfo;
	for(unsigned int i = 0; i < CodeInfo::funcInfo.size(); i++)
	{
		ExternFuncInfo &funcInfo = *fInfo;
		FunctionInfo *refFunc = CodeInfo::funcInfo[i];

		if(refFunc->codeSize == 0 && refFunc->address != -1 && (refFunc->address & 0x80000000))
			funcInfo.oldAddress = funcInfo.address = CodeInfo::funcInfo[refFunc->address & ~0x80000000]->address;
		else
			funcInfo.oldAddress = funcInfo.address = refFunc->address;
		funcInfo.codeSize = refFunc->codeSize;
		funcInfo.funcPtr = refFunc->funcPtr;
		funcInfo.isVisible = refFunc->visible;

		offsetToGlobal += funcInfo.codeSize;

		funcInfo.nameHash = refFunc->nameHash;

		funcInfo.retType = ExternFuncInfo::RETURN_UNKNOWN;
		if(refFunc->retType->type == TypeInfo::TYPE_VOID)
			funcInfo.retType = ExternFuncInfo::RETURN_VOID;
		else if(refFunc->retType->type == TypeInfo::TYPE_FLOAT || refFunc->retType->type == TypeInfo::TYPE_DOUBLE)
			funcInfo.retType = ExternFuncInfo::RETURN_DOUBLE;
		else if(refFunc->retType->type == TypeInfo::TYPE_INT || refFunc->retType->size <= 4)
			funcInfo.retType = ExternFuncInfo::RETURN_INT;
		else if(refFunc->retType->type == TypeInfo::TYPE_LONG || refFunc->retType->size == 8)
			funcInfo.retType = ExternFuncInfo::RETURN_LONG;

		funcInfo.funcType = refFunc->funcType->typeIndex;

		CreateExternalInfo(funcInfo, *refFunc);

		funcInfo.offsetToFirstLocal = localOffset;

		ExternLocalInfo::LocalType paramType = refFunc->firstParam ? ExternLocalInfo::PARAMETER : ExternLocalInfo::LOCAL;
		if(refFunc->firstParam)
			refFunc->lastParam->next = refFunc->firstLocal;
		for(VariableInfo *curr = refFunc->firstParam ? refFunc->firstParam : refFunc->firstLocal; curr; curr = curr->next, localOffset++)
		{
			code->firstLocal[localOffset].paramType = paramType;
			code->firstLocal[localOffset].type = GetTypeIndexByPtr(curr->varType);
			code->firstLocal[localOffset].offset = curr->pos;
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

		for(FunctionInfo::ExternalInfo *curr = refFunc->firstExternal; curr; curr = curr->next, localOffset++)
		{
			code->firstLocal[localOffset].paramType = ExternLocalInfo::EXTERNAL;
			code->firstLocal[localOffset].type = GetTypeIndexByPtr(curr->variable->varType);
			code->firstLocal[localOffset].size = curr->variable->varType->size;
			code->firstLocal[localOffset].target = curr->targetPos;
			code->firstLocal[localOffset].closeFuncList = curr->targetFunc | (curr->targetLocal ? 0x80000000 : 0);
			code->firstLocal[localOffset].offsetToName = int(symbolPos - code->debugSymbols);
			memcpy(symbolPos, curr->variable->name.begin, curr->variable->name.end - curr->variable->name.begin + 1);
			symbolPos += curr->variable->name.end - curr->variable->name.begin;
			*symbolPos++ = 0;
		}
		funcInfo.externCount = refFunc->externalCount;

		funcInfo.offsetToName = int(symbolPos - code->debugSymbols);
		memcpy(symbolPos, refFunc->name, refFunc->nameLength + 1);
		symbolPos += refFunc->nameLength + 1;

		// Fill up next
		fInfo++;
	}

	code->code = FindCode(code);
	code->globalCodeStart = offsetToGlobal;
	memcpy(code->code, &CodeInfo::cmdList[0], CodeInfo::cmdList.size() * sizeof(VMCmd));

	return size;
}

