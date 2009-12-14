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
	typeTop = TypeInfo::GetPoolTop();

	varTop = 0;
	funcTop = 0;

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

	AddExternalFunction((void (*)())NULLC::IntToStr, "char[] int:str();");

	AddExternalFunction((void (*)())NULLC::AllocObject, "int __newS(int size);");
	AddExternalFunction((void (*)())NULLC::AllocArray, "int[] __newA(int size, int count);");

#ifdef NULLC_LOG_FILES
	compileLog = NULL;
#endif
}

Compiler::~Compiler()
{
	ParseReset();

	CallbackReset();

	CodeInfo::varInfo.clear();
	CodeInfo::funcInfo.clear();
	CodeInfo::typeInfo.clear();
	CodeInfo::aliasInfo.clear();

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
	TypeInfo::SetPoolTop(typeTop);
	CodeInfo::aliasInfo.clear();

	CodeInfo::funcInfo.resize(buildInFuncs);
	FunctionInfo::SetPoolTop(funcTop);

	CodeInfo::nodeList.clear();

	ClearStringList();

	VariableInfo::SetPoolTop(varTop);

	CallbackInitialize();

#ifdef NULLC_LOG_FILES
	if(compileLog)
		fclose(compileLog);
	compileLog = fopen("compilelog.txt", "wb");
#endif
	CodeInfo::lastError = CompilerError();

	CodeInfo::cmdInfoList.Clear();
	CodeInfo::cmdList.clear();
}

bool Compiler::AddExternalFunction(void (NCDECL *ptr)(), const char* prototype)
{
	ClearState();

	bool res;

	lexer.Clear();
	lexer.Lexify(prototype);

	if(!setjmp(CodeInfo::errorHandler))
	{
		Lexeme *start = lexer.GetStreamStart();
		res = ParseSelectType(&start);
		if(res)
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
	typeTop = TypeInfo::GetPoolTop();
	varTop = VariableInfo::GetPoolTop();
	funcTop = FunctionInfo::GetPoolTop();

	return true;
}

bool Compiler::AddType(const char* typedecl)
{
	ClearState();

	bool res;

	lexer.Clear();
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

	buildInTypes = (int)CodeInfo::typeInfo.size();
	typeTop = TypeInfo::GetPoolTop();
	varTop = VariableInfo::GetPoolTop();
	funcTop = FunctionInfo::GetPoolTop();

	return true;
}

bool Compiler::ImportModule(char* bytecode)
{
	char errBuf[256];
	ByteCode *bCode = (ByteCode*)bytecode;
	char *symbols = (char*)(bCode) + bCode->offsetToSymbols;

	FastVector<unsigned int>	typeRemap;

	// Import types
	ExternTypeInfo *tInfo = FindFirstType(bCode);
	unsigned int *memberList = (unsigned int*)(tInfo + bCode->typeCount);

	unsigned int oldTypeCount = CodeInfo::typeInfo.size();
	for(unsigned int i = 0; i < bCode->typeCount; i++)
	{
		const unsigned int INDEX_NONE = ~0u;

		unsigned int index = INDEX_NONE;
		for(unsigned int n = 0; n < oldTypeCount && index == INDEX_NONE; n++)
			if(CodeInfo::typeInfo[n]->GetFullNameHash() == tInfo->nameHash)
				index = n;

		if(index == INDEX_NONE)
		{
			typeRemap.push_back(CodeInfo::typeInfo.size());
			TypeInfo *newInfo = NULL;
			switch(tInfo->subCat)
			{
			case ExternTypeInfo::CAT_FUNCTION:
				CodeInfo::typeInfo.push_back(new TypeInfo(CodeInfo::typeInfo.size(), NULL, 0, 0, 1, NULL, TypeInfo::TYPE_COMPLEX));
				newInfo = CodeInfo::typeInfo.back();
				newInfo->CreateFunctionType(CodeInfo::typeInfo[typeRemap[memberList[tInfo->memberOffset]]], tInfo->memberCount);

				for(unsigned int n = 1; n < tInfo->memberCount + 1; n++)
					newInfo->funcType->paramType[n-1] = CodeInfo::typeInfo[typeRemap[memberList[tInfo->memberOffset + n]]];

#ifdef _DEBUG
				newInfo->AddMemberVariable("context", typeInt);
				newInfo->AddMemberVariable("ptr", typeInt);
#endif
				newInfo->size = 8;
				break;
			default:
				SafeSprintf(errBuf, 256, "ERROR: new type in module named %s unsupported", (symbols + tInfo->offsetToName));
				CodeInfo::lastError = CompilerError(errBuf, "Module link");
				return false;
			}
		}else{
			// Type full check
			if(CodeInfo::typeInfo[index]->type != tInfo->type)
			{
				SafeSprintf(errBuf, 256, "ERROR: there already is a type named %s with a different structure", CodeInfo::typeInfo[index]->GetFullTypeName());
				CodeInfo::lastError = CompilerError(errBuf, "Module link");
				return false;
			}
			typeRemap.push_back(index);
		}

		tInfo++;
	}

	// Import functions
	ExternFuncInfo *fInfo = FindFirstFunc(bCode);
	ExternLocalInfo *fLocals = (ExternLocalInfo*)((char*)(bCode) + bCode->offsetToLocals);

	unsigned int oldFuncCount = CodeInfo::funcInfo.size();
	fInfo += bCode->oldFunctionCount;
	for(unsigned int i = bCode->oldFunctionCount; i < bCode->functionCount; i++)
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
			CodeInfo::funcInfo.push_back(new FunctionInfo(symbols + fInfo->offsetToName));
			FunctionInfo* lastFunc = CodeInfo::funcInfo.back();

			lastFunc->retType = CodeInfo::typeInfo[typeRemap[fInfo->retType]];

			for(unsigned int n = 0; n < fInfo->localCount; n++)
			{
				ExternLocalInfo &lInfo = fLocals[fInfo->offsetToFirstLocal + n];
				TypeInfo *currType = CodeInfo::typeInfo[typeRemap[lInfo.type]];
				lastFunc->AddParameter(new VariableInfo(InplaceStr(symbols + lInfo.offsetToName), GetStringHash(symbols + lInfo.offsetToName), 0, currType, false, false));
				lastFunc->allParamSize += currType->size < 4 ? 4 : currType->size;
			}
			lastFunc->implemented = true;
			lastFunc->funcType = CodeInfo::typeInfo[typeRemap[fInfo->funcType]];
		}else{
			SafeSprintf(errBuf, 256, "ERROR: function %s (type %s) is already defined.", CodeInfo::funcInfo[index]->name, CodeInfo::funcInfo[index]->funcType->GetFullTypeName());
			CodeInfo::lastError = CompilerError(errBuf, "Module link");
			return false;
		}

		fInfo++;
	}

	return true;
}

bool Compiler::Compile(const char* str, bool noClear)
{
	unsigned int t = clock();

	if(!noClear)
		lexer.Clear();
	unsigned int lexStreamStart = lexer.GetStreamSize();
	lexer.Lexify(str);

	char	*moduleName[32];
	char	*moduleData[32];
	unsigned int moduleCount = 0;

	Lexeme *start = &lexer.GetStreamStart()[lexStreamStart];
	while(start->type == lex_import)
	{
		start++;
		char path[256], *cPath = path;
		Lexeme *name = start;
		if(start->type != lex_string)
		{
			CodeInfo::lastError = CompilerError("ERROR: string expected after import", start->pos);
			return false;
		}
		cPath += SafeSprintf(cPath, 256, "%.*s", start->length, start->pos);
		start++;
		while(start->type == lex_point)
		{
			start++;
			if(start->type != lex_string)
			{
				CodeInfo::lastError = CompilerError("ERROR: string expected after '.'", start->pos);
				return false;
			}
			cPath += SafeSprintf(cPath, 256 - int(cPath - path), "\\%.*s", start->length, start->pos);
			start++;
		}
		if(start->type != lex_semicolon)
		{
			CodeInfo::lastError = CompilerError("ERROR: ';' not found after import expression", name->pos);
			return false;
		}
		start++;
		/*SafeSprintf(cPath, 256 - int(cPath - path), ".ncm");
		if(FILE *module = fopen(path, "rb"))
		{
			fseek(module, 0, SEEK_END);
			unsigned int bcSize = ftell(module);
			fseek(module, 0, SEEK_SET);
			char *bytecode = new char[bcSize];
			fread(bytecode, 1, bcSize, module);
			fclose(module);

			if(!ImportModule(path, bytecode))
				return false;
		}else*/{
			SafeSprintf(cPath, 256 - int(cPath - path), ".nc");
			if(FILE *rawModule = fopen(path, "rb"))
			{
				fseek(rawModule, 0, SEEK_END);
				unsigned int textSize = ftell(rawModule);
				fseek(rawModule, 0, SEEK_SET);
				char *fileContent = new char[textSize+1];
				fread(fileContent, 1, textSize, rawModule);
				fileContent[textSize] = 0;

				unsigned int lexPos = (unsigned int)(start - &lexer.GetStreamStart()[lexStreamStart]);
				if(!Compile(fileContent, true))
				{
					SafeSprintf(CodeInfo::lastError.error, 256 - strlen(CodeInfo::lastError.error), "%s [in module %s]", CodeInfo::lastError.error, path);
					fclose(rawModule);
					return false;
				}
				start = &lexer.GetStreamStart()[lexStreamStart + lexPos];
				fclose(rawModule);
				char *bytecode = NULL;
				unsigned int bcSize = GetBytecode(&bytecode);

				SafeSprintf(cPath, 256 - int(cPath - path), ".ncm");
				FILE *module = fopen(path, "wb");
				fwrite(bytecode, 1, bcSize, module);
				fclose(module);

				if(moduleCount == 32)
				{
					CodeInfo::lastError = CompilerError("ERROR: temporary limit for 32 modules", name->pos);
					return false;
				}
				moduleName[moduleCount] = strcpy((char*)dupStrings.Allocate((unsigned int)strlen(path) + 1), path);
				moduleData[moduleCount++] = bytecode;
			}else{
				CodeInfo::lastError = CompilerError("ERROR: module or source file can't be found", name->pos);
				return false;
			}
		}
	}

	ClearState();

	activeModules.clear();

	for(unsigned int i = 0; i < moduleCount; i++)
	{
		activeModules.push_back();
		activeModules.back().name = moduleName[i];
		activeModules.back().funcStart = CodeInfo::funcInfo.size();
		if(!ImportModule(moduleData[i]))
			return false;
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
	fInfo.bytesToPop = refFunc.type == FunctionInfo::THISCALL ? 4 : 0;
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
	unsigned int allMemberCount = 0;
	for(unsigned int i = 0; i < CodeInfo::typeInfo.size(); i++)
	{
		TypeInfo *type = CodeInfo::typeInfo[i];

		symbolStorageSize += type->GetFullNameLength() + 1;
		allMemberCount += type->funcType ? type->funcType->paramCount + 1 : type->memberCount;
	}
	size += allMemberCount * sizeof(unsigned int);

	unsigned int offsetToModule = size;
	size += activeModules.size() * sizeof(ExternModuleInfo);
	for(unsigned int i = 0; i < activeModules.size(); i++)
	{
		symbolStorageSize += (unsigned int)strlen(activeModules[i].name) + 1;
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

	code->dependsCount = activeModules.size();
	code->offsetToFirstModule = offsetToModule;
	code->firstModule = (ExternModuleInfo*)((char*)(code) + code->offsetToFirstModule);

	code->globalVarSize = GetGlobalSize();
	code->variableCount = (unsigned int)CodeInfo::varInfo.size();
	code->offsetToFirstVar = offsetToVar;

	code->functionCount = (unsigned int)CodeInfo::funcInfo.size();
	code->oldFunctionCount = buildInFuncs;
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

		if(refType.funcType != 0)						// Function type
		{
			typeInfo.subCat = ExternTypeInfo::CAT_FUNCTION;
			typeInfo.memberCount = refType.funcType->paramCount;
			typeInfo.memberOffset = memberOffset;
			memberList[memberOffset++] = GetTypeIndexByPtr(refType.funcType->retType);
			for(unsigned int k = 0; k < refType.funcType->paramCount; k++)
				memberList[memberOffset++] = GetTypeIndexByPtr(refType.funcType->paramType[k]);
		}else if(refType.type == TypeInfo::TYPE_COMPLEX){	// Complex type
			typeInfo.subCat = ExternTypeInfo::CAT_CLASS;
			typeInfo.memberCount = refType.memberCount;
			typeInfo.memberOffset = memberOffset;
			for(TypeInfo::MemberVariable *curr = refType.firstVariable; curr; curr = curr->next)
				memberList[memberOffset++] = GetTypeIndexByPtr(curr->type);
		}else if(refType.arrLevel != 0){				// Array type
			typeInfo.subCat = ExternTypeInfo::CAT_ARRAY;
			typeInfo.arrSize = refType.arrSize;
			typeInfo.subType = GetTypeIndexByPtr(refType.subType);
		}else if(refType.refLevel != 0){				// Pointer type
			typeInfo.subCat = ExternTypeInfo::CAT_POINTER;
			typeInfo.subType = GetTypeIndexByPtr(refType.subType);
		}else{
			typeInfo.subCat = ExternTypeInfo::CAT_NONE;
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

		mInfo++;
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
			{
				funcInfo.paramCount = localOffset - funcInfo.offsetToFirstLocal;
				paramType = ExternLocalInfo::LOCAL;
			}
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

