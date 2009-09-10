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
		while(scan < begin)
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

	for(unsigned int i = 0; i < typeInfo.size(); i++)
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
	fwrite(str, 1, strlen(str), fCode);
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
	fprintf(compileLog, "\r\nActive types (%d):\r\n", typeInfo.size());
	for(unsigned int i = 0; i < typeInfo.size(); i++)
		fprintf(compileLog, "%s (%d bytes)\r\n", typeInfo[i]->GetFullTypeName(), typeInfo[i]->size);

	fprintf(compileLog, "\r\nActive functions (%d):\r\n", funcInfo.size());
	for(unsigned int i = 0; i < funcInfo.size(); i++)
	{
		FunctionInfo &currFunc = *funcInfo[i];
		fprintf(compileLog, "%s", currFunc.type == FunctionInfo::LOCAL ? "local " : (currFunc.type == FunctionInfo::NORMAL ? "global " : "thiscall "));
		fprintf(compileLog, "%s %s(", currFunc.retType->GetFullTypeName(), currFunc.name);

		for(VariableInfo *curr = currFunc.firstParam; curr; curr = curr->next)
			fprintf(compileLog, "%s %.*s%s", curr->varType->GetFullTypeName(), curr->name.end-curr->name.begin, curr->name.begin, (curr == currFunc.lastParam ? "" : ", "));
		
		fprintf(compileLog, ")\r\n");
		if(currFunc.type == FunctionInfo::LOCAL)
		{
			for(FunctionInfo::ExternalName *curr = currFunc.firstExternal; curr; curr = curr->next)
				fprintf(compileLog, "  external var: %.*s\r\n", curr->name.end-curr->name.begin, curr->name.begin);
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
		fprintf(compiledAsm, "%d %s\r\n", i, instBuf);
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

	unsigned int offsetToVar = size;
	size += CodeInfo::varInfo.size() * sizeof(ExternVarInfo);

	unsigned int offsetToFunc = size;
	size += CodeInfo::funcInfo.size() * sizeof(ExternFuncInfo);

	unsigned int offsetToFirstParameter = size;

	unsigned int parameterCount = 0;
	for(unsigned int i = 0; i < CodeInfo::funcInfo.size(); i++)
		parameterCount += (unsigned int)CodeInfo::funcInfo[i]->paramCount;
	size += parameterCount * sizeof(unsigned int);

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

	code->parameterCount = parameterCount;
	code->offsetToFirstParameter = offsetToFirstParameter;
	code->firstParameter = (unsigned int*)((char*)(code) + code->offsetToFirstParameter);

	code->codeSize = CodeInfo::cmdList.size();
	code->offsetToCode = offsetToCode;

	ExternTypeInfo *tInfo = FindFirstType(code);
	code->firstType = tInfo;
	for(unsigned int i = 0; i < CodeInfo::typeInfo.size(); i++)
	{
		ExternTypeInfo &typeInfo = *tInfo;

		typeInfo.structSize = sizeof(ExternTypeInfo);

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
		varInfo.structSize = sizeof(ExternVarInfo);

		varInfo.type = GetTypeIndexByPtr(CodeInfo::varInfo[i]->varType);
		varInfo.nameHash = GetStringHash(CodeInfo::varInfo[i]->name.begin, CodeInfo::varInfo[i]->name.end);

		// Fill up next
		vInfo++;
	}

	unsigned int parameterOffset = 0;
	unsigned int offsetToGlobal = 0;
	ExternFuncInfo *fInfo = FindFirstFunc(code);
	code->firstFunc = fInfo;
	for(unsigned int i = 0; i < CodeInfo::funcInfo.size(); i++)
	{
		ExternFuncInfo &funcInfo = *fInfo;
		FunctionInfo *refFunc = CodeInfo::funcInfo[i];

		funcInfo.oldAddress = funcInfo.address = refFunc->address;
		funcInfo.codeSize = refFunc->codeSize;
		funcInfo.funcPtr = refFunc->funcPtr;
		funcInfo.isVisible = refFunc->visible;

		offsetToGlobal += funcInfo.codeSize;

		funcInfo.nameHash = refFunc->nameHash;

		funcInfo.retSize = refFunc->retType->size;
		funcInfo.funcType = refFunc->funcType->typeIndex;

		CreateExternalInfo(funcInfo, *refFunc);

		funcInfo.structSize = sizeof(ExternFuncInfo);

		for(VariableInfo *curr = refFunc->firstParam; curr; curr = curr->next, parameterOffset++)
			code->firstParameter[parameterOffset] = curr->varType->typeIndex;

		// Fill up next
		fInfo++;
	}

	code->code = FindCode(code);
	code->globalCodeStart = offsetToGlobal;
	memcpy(code->code, &CodeInfo::cmdList[0], CodeInfo::cmdList.size() * sizeof(VMCmd));

	return size;
}

