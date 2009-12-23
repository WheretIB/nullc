#include "stdafx.h"

#include "CodeInfo.h"

#include "Bytecode.h"

#include "Compiler.h"

#include "Parser.h"
#include "Callbacks.h"

#include "StdLib.h"

#include "BinaryCache.h"

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
	info->size = 2;
	typeShort = info;
	CodeInfo::typeInfo.push_back(info);

	info = new TypeInfo(CodeInfo::typeInfo.size(), "char", 0, 0, 1, NULL, TypeInfo::TYPE_CHAR);
	info->size = 1;
	typeChar = info;
	CodeInfo::typeInfo.push_back(info);

	buildInTypes.resize(CodeInfo::typeInfo.size());
	memcpy(&buildInTypes[0], &CodeInfo::typeInfo[0], CodeInfo::typeInfo.size() * sizeof(TypeInfo*));

	CodeInfo::classCount = basicTypes = (int)CodeInfo::typeInfo.size();
	typeTop = TypeInfo::GetPoolTop();

	varTop = 0;
	funcTop = 0;

	// Add build-in functions
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

	CodeInfo::classCount = basicTypes;

	CodeInfo::typeInfo.resize(buildInTypes.size());
	memcpy(&CodeInfo::typeInfo[0], &buildInTypes[0], buildInTypes.size() * sizeof(TypeInfo*));
	for(unsigned int i = 0; i < buildInTypes.size(); i++)
	{
		CodeInfo::typeInfo[i]->typeIndex = i;
		if(CodeInfo::typeInfo[i]->refType && CodeInfo::typeInfo[i]->refType->typeIndex >= buildInTypes.size())
			CodeInfo::typeInfo[i]->refType = NULL;
	}

	for(unsigned int i = 0; i < CodeInfo::typeInfo.size(); i++)
		CodeInfo::typeInfo[i]->fullName = NULL;

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

	buildInTypes.resize(CodeInfo::typeInfo.size());
	memcpy(&buildInTypes[0], &CodeInfo::typeInfo[0], CodeInfo::typeInfo.size() * sizeof(TypeInfo*));

	basicTypes = CodeInfo::classCount;

	typeTop = TypeInfo::GetPoolTop();
	varTop = VariableInfo::GetPoolTop();
	funcTop = FunctionInfo::GetPoolTop();

	return true;
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
			path[i] = '\\';
	}
	SafeSprintf(cPath, 256 - int(cPath - path), ".nc");
	char *bytecode = BinaryCache::GetBytecode(path);
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
	
	fInfo += code->externalFunctionCount + code->moduleFunctionCount;
	for(unsigned int i = code->externalFunctionCount + code->moduleFunctionCount; i < code->functionCount; i++)
	{
		if(hash == fInfo->nameHash)
		{
			if(index == 0)
			{
				fInfo->address = -1;
				fInfo->funcPtr = ptr;
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

bool Compiler::ImportModule(char* bytecode, const char* pos)
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
			TypeInfo *newInfo = NULL, *tempInfo = NULL;
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
			case ExternTypeInfo::CAT_ARRAY:
				tempInfo = CodeInfo::typeInfo[typeRemap[tInfo->subType]];
				CodeInfo::typeInfo.push_back(new TypeInfo(CodeInfo::typeInfo.size(), NULL, 0, tempInfo->arrLevel + 1, tInfo->arrSize, tempInfo, TypeInfo::TYPE_COMPLEX));
				newInfo = CodeInfo::typeInfo.back();

				if(tInfo->arrSize == -1)
				{
					newInfo->size = 4;
					newInfo->AddMemberVariable("size", typeInt);
				}else{
					newInfo->size = tempInfo->size * tInfo->arrSize;
					if(newInfo->size % 4 != 0)
					{
						newInfo->paddingBytes = 4 - (newInfo->size % 4);
						newInfo->size += 4 - (newInfo->size % 4);
					}
				}
				break;
			case ExternTypeInfo::CAT_POINTER:
				tempInfo = CodeInfo::typeInfo[typeRemap[tInfo->subType]];
				CodeInfo::typeInfo.push_back(new TypeInfo(CodeInfo::typeInfo.size(), NULL, tempInfo->refLevel + 1, 0, 1, tempInfo, TypeInfo::TYPE_INT));
				newInfo = CodeInfo::typeInfo.back();
				newInfo->size = 4;

				// Save it for future use
				CodeInfo::typeInfo[typeRemap[tInfo->subType]]->refType = newInfo;
				break;
			case ExternTypeInfo::CAT_CLASS:
			{
				unsigned int strLength = (unsigned int)strlen(symbols + tInfo->offsetToName) + 1;
				const char *nameCopy = strcpy((char*)dupStrings.Allocate(strLength), symbols + tInfo->offsetToName);
				newInfo = new TypeInfo(CodeInfo::classCount, nameCopy, 0, 0, 1, NULL, TypeInfo::TYPE_COMPLEX);

				if(CodeInfo::classCount == CodeInfo::typeInfo.size())
				{
					CodeInfo::typeInfo.push_back(newInfo);
				}else{
					CodeInfo::typeInfo[CodeInfo::classCount]->typeIndex = CodeInfo::typeInfo.size();
					CodeInfo::typeInfo.push_back(CodeInfo::typeInfo[CodeInfo::classCount]);
					CodeInfo::typeInfo[CodeInfo::classCount] = newInfo;
				}
				
				typeRemap[CodeInfo::classCount] = typeRemap.back();
				typeRemap.back() = CodeInfo::classCount;

				CodeInfo::classCount++;

				const char *memberName = symbols + tInfo->offsetToName + strLength;
				for(unsigned int n = 0; n < tInfo->memberCount; n++)
				{
					strLength = (unsigned int)strlen(memberName) + 1;
					nameCopy = strcpy((char*)dupStrings.Allocate(strLength), memberName);
					memberName += strLength;
					newInfo->AddMemberVariable(nameCopy, CodeInfo::typeInfo[typeRemap[memberList[tInfo->memberOffset + n]]]);
				}
			}
				break;
			default:
				SafeSprintf(errBuf, 256, "ERROR: new type in module named %s unsupported", (symbols + tInfo->offsetToName));
				CodeInfo::lastError = CompilerError(errBuf, pos);
				return false;
			}
			newInfo->alignBytes = tInfo->defaultAlign;
		}else{
			// Type full check
			if(CodeInfo::typeInfo[index]->type != tInfo->type)
			{
				SafeSprintf(errBuf, 256, "ERROR: there already is a type named %s with a different structure", CodeInfo::typeInfo[index]->GetFullTypeName());
				CodeInfo::lastError = CompilerError(errBuf, pos);
				return false;
			}
			typeRemap.push_back(index);
		}

		tInfo++;
	}
/*
	// Import variables
	ExternVarInfo *vInfo = FindFirstVar(bCode);
	for(unsigned int i = 0; i< bCode->variableCount; i++)
	{
		SelectTypeByIndex(typeRemap[vInfo->type]);
		AddVariable(pos, InplaceStr(symbols + vInfo->offsetToName));
	}
*/
	// Import functions
	ExternFuncInfo *fInfo = FindFirstFunc(bCode);
	ExternLocalInfo *fLocals = (ExternLocalInfo*)((char*)(bCode) + bCode->offsetToLocals);

	unsigned int oldFuncCount = CodeInfo::funcInfo.size();
	fInfo += bCode->externalFunctionCount + bCode->moduleFunctionCount;
	for(unsigned int i = bCode->externalFunctionCount + bCode->moduleFunctionCount; i < bCode->functionCount; i++)
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
			const char *nameCopy = strcpy((char*)dupStrings.Allocate(strLength), symbols + fInfo->offsetToName);

			CodeInfo::funcInfo.push_back(new FunctionInfo(nameCopy));
			FunctionInfo* lastFunc = CodeInfo::funcInfo.back();

			lastFunc->retType = CodeInfo::typeInfo[typeRemap[fInfo->retType]];
			lastFunc->address = fInfo->funcPtr ? -1 : 0;
			lastFunc->funcPtr = fInfo->funcPtr;

			for(unsigned int n = 0; n < fInfo->paramCount; n++)
			{
				ExternLocalInfo &lInfo = fLocals[fInfo->offsetToFirstLocal + n];
				TypeInfo *currType = CodeInfo::typeInfo[typeRemap[lInfo.type]];
				lastFunc->AddParameter(new VariableInfo(InplaceStr(symbols + lInfo.offsetToName), GetStringHash(symbols + lInfo.offsetToName), 0, currType, false, false));
				lastFunc->allParamSize += currType->size < 4 ? 4 : currType->size;
			}
			lastFunc->implemented = true;
			lastFunc->type = strchr(lastFunc->name, ':') ? FunctionInfo::THISCALL : FunctionInfo::NORMAL;
			lastFunc->funcType = CodeInfo::typeInfo[typeRemap[fInfo->funcType]];
		}else{
			SafeSprintf(errBuf, 256, "ERROR: function %s (type %s) is already defined.", CodeInfo::funcInfo[index]->name, CodeInfo::funcInfo[index]->funcType->GetFullTypeName());
			CodeInfo::lastError = CompilerError(errBuf, pos);
			return false;
		}

		fInfo++;
	}

	return true;
}

char* Compiler::BuildModule(const char* file, const char* altFile)
{
	FILE *rawModule = fopen(file, "rb");
	bool failedImportPath = false;
	if(!rawModule && BinaryCache::GetImportPath())
	{
		failedImportPath = true;
		rawModule = fopen(altFile, "rb");
	}
	if(rawModule)
	{
		fseek(rawModule, 0, SEEK_END);
		unsigned int textSize = ftell(rawModule);
		fseek(rawModule, 0, SEEK_SET);
		char *fileContent = new char[textSize+1];
		fread(fileContent, 1, textSize, rawModule);
		fileContent[textSize] = 0;

		if(!Compile(fileContent, true))
		{
			SafeSprintf(CodeInfo::lastError.error, 256 - strlen(CodeInfo::lastError.error), "%s [in module %s]", CodeInfo::lastError.error, file);
			fclose(rawModule);
			return false;
		}
		fclose(rawModule);
		char *bytecode = NULL;
		GetBytecode(&bytecode);

		delete[] fileContent;

		BinaryCache::PutBytecode(failedImportPath ? altFile : file, bytecode);

		return bytecode;
	}
	return NULL;
}

bool Compiler::Compile(const char* str, bool noClear)
{
	unsigned int t = clock();

	if(!noClear)
	{
		lexer.Clear();
		moduleSource.clear();
	}
	unsigned int lexStreamStart = lexer.GetStreamSize();
	lexer.Lexify(str);

	char	*moduleName[32];
	char	*moduleData[32];
	unsigned int moduleCount = 0;

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
			cPath += SafeSprintf(cPath, 256 - int(cPath - path), "\\%.*s", start->length, start->pos);
			start++;
		}
		if(start->type != lex_semicolon)
		{
			CodeInfo::lastError = CompilerError("ERROR: ';' not found after import expression", name->pos);
			return false;
		}
		start++;
		SafeSprintf(cPath, 256 - int(cPath - path), ".nc");
		char *bytecode = BinaryCache::GetBytecode(path);
		if(!bytecode && importPath)
			bytecode = BinaryCache::GetBytecode(pathNoImport);

		if(moduleCount == 32)
		{
			CodeInfo::lastError = CompilerError("ERROR: temporary limit for 32 modules", name->pos);
			return false;
		}

		moduleName[moduleCount] = strcpy((char*)dupStrings.Allocate((unsigned int)strlen(pathNoImport) + 1), pathNoImport);
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
		if(!ImportModule(moduleData[i], moduleName[i]))
		{
			delete[] moduleData[i];
			return false;
		}
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
	size += CodeInfo::varInfo.size() * sizeof(ExternVarInfo);
	for(unsigned int i = 0; i < CodeInfo::varInfo.size(); i++)
	{
		VariableInfo *curr = CodeInfo::varInfo[i];

		symbolStorageSize += (unsigned int)(curr->name.end - curr->name.begin) + 1;
	}

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
	code->externalFunctionCount = buildInFuncs;
	code->moduleFunctionCount = functionsInModules;
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
		VariableInfo *refVar = CodeInfo::varInfo[i];

		varInfo.offsetToName = int(symbolPos - code->debugSymbols);
		memcpy(symbolPos, refVar->name.begin, refVar->name.end - refVar->name.begin + 1);
		symbolPos += refVar->name.end - refVar->name.begin;
		*symbolPos++ = 0;

		varInfo.nameHash = GetStringHash(refVar->name.begin, refVar->name.end);

		varInfo.type = refVar->varType->typeIndex;
		varInfo.size = refVar->varType->size;

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

		funcInfo.paramCount = refFunc->paramCount;

		ExternLocalInfo::LocalType paramType = refFunc->firstParam ? ExternLocalInfo::PARAMETER : ExternLocalInfo::LOCAL;
		if(refFunc->firstParam)
			refFunc->lastParam->next = refFunc->firstLocal;
		for(VariableInfo *curr = refFunc->firstParam ? refFunc->firstParam : refFunc->firstLocal; curr; curr = curr->next, localOffset++)
		{
			code->firstLocal[localOffset].paramType = paramType;
			code->firstLocal[localOffset].type = curr->varType->typeIndex;
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
			code->firstLocal[localOffset].type = curr->variable->varType->typeIndex;
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

