#include "stdafx.h"
#ifdef NULLC_BUILD_X86_JIT

#include "Executor_X86.h"
#include "StdLib_X86.h"
#include "Translator_X86.h"
#include "Optimizer_x86.h"

#if defined(_MSC_VER)
bool ExecutorX86::CreateExternalInfo(ExternFuncInfo *fInfo, ExecutorX86::ExternalFunctionInfo& externalInfo)
{
	externalInfo.bytesToPop = 0;
	for(unsigned int i = 0; i < fInfo->paramCount; i++)
	{
		unsigned int paramSize = exTypes[fInfo->paramList[i]]->size > 4 ? exTypes[fInfo->paramList[i]]->size : 4;
		externalInfo.bytesToPop += paramSize;
	}
	
	return true;
}
#elif defined(__CELLOS_LV2__)
bool ExecutorX86::CreateExternalInfo(ExternFuncInfo *fInfo, ExecutorX86::ExternalFunctionInfo& externalInfo)
{
    unsigned int rCount = 0, fCount = 0;
    unsigned int rMaxCount = sizeof(externalInfo.rOffsets) / sizeof(externalInfo.rOffsets[0]);
    unsigned int fMaxCount = sizeof(externalInfo.fOffsets) / sizeof(externalInfo.fOffsets[0]);
    
    // parse all parameters, fill offsets
    unsigned int offset = 0;
    
	for (unsigned int i = 0; i < fInfo->paramCount; i++)
	{
	    const ExternTypeInfo& type = *exTypes[fInfo->paramList[i]];
	    
	    switch (type.type)
	    {
	    case ExternTypeInfo::TYPE_CHAR:
	    case ExternTypeInfo::TYPE_SHORT:
	    case ExternTypeInfo::TYPE_INT:
	        if (rCount >= rMaxCount) return false; // too many r parameters
	        externalInfo.rOffsets[rCount++] = offset;
	        offset++;
	        break;
	    
	    case ExternTypeInfo::TYPE_FLOAT:
	    case ExternTypeInfo::TYPE_DOUBLE:
	        if (fCount >= fMaxCount || rCount >= rMaxCount) return false; // too many f/r parameters
	        externalInfo.rOffsets[rCount++] = offset;
	        externalInfo.fOffsets[fCount++] = offset;
	        offset += 2;
	        break;
	        
	    default:
	        return false; // unsupported type
	    }
    }
    
    // clear remaining offsets
    for (unsigned int i = rCount; i < rMaxCount; ++i) externalInfo.rOffsets[i] = 0;
    for (unsigned int i = fCount; i < fMaxCount; ++i) externalInfo.fOffsets[i] = 0;
    
    return true;
}
#endif

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

unsigned int paramDataBase;
unsigned int reservedStack;
unsigned int commitedStack;
unsigned int stackGrowSize;
unsigned int stackGrowCommit;

ExecutorX86::ExecutorX86(): exTypes(50), exVariables(50), exFunctions(50), exFuncInfo(50)
{
	globalVarSize = 0;
	offsetToGlobalCode = 0;

	stackGrowSize = 128*4096;
	stackGrowCommit = 64*4096;
	// Request memory at address
	if(NULL == (paramData = (char*)VirtualAlloc(reinterpret_cast<void*>(0x20000000), stackGrowSize, MEM_RESERVE, PAGE_NOACCESS)))
		throw std::string("ERROR: Failed to reserve memory");
	if(!VirtualAlloc(reinterpret_cast<void*>(0x20000000), stackGrowCommit, MEM_COMMIT, PAGE_READWRITE))
		throw std::string("ERROR: Failed to commit memory");

	reservedStack = stackGrowSize;
	commitedStack = stackGrowCommit;
	
	paramDataBase = paramBase = static_cast<unsigned int>(reinterpret_cast<long long>(paramData));

	binCode = new unsigned char[200000];
	memset(binCode, 0x90, 20);
	binCodeStart = static_cast<unsigned int>(reinterpret_cast<long long>(&binCode[20]));
	binCodeSize = 0;
}
ExecutorX86::~ExecutorX86()
{
	VirtualFree(reinterpret_cast<void*>(0x20000000), 0, MEM_RELEASE);

	CleanCode();

	delete[] binCode;
}

void ExecutorX86::CleanCode()
{
	exFuncInfo.clear();
	for(unsigned int i = 0; i < exTypes.size(); i++)
		delete[] (char*)exTypes[i];
	exTypes.clear();
	for(unsigned int i = 0; i < exVariables.size(); i++)
		delete[] (char*)exVariables[i];
	exVariables.clear();
	for(unsigned int i = 0; i < exFunctions.size(); i++)
		delete[] (char*)exFunctions[i];
	exFunctions.clear();
	exCode.clear();

	globalVarSize = 0;
	offsetToGlobalCode = 0;
}

bool ExecutorX86::LinkCode(const char *code, int redefinitions)
{
	execError[0] = 0;

	FastVector<unsigned int>	typeRemap(50);
	FastVector<unsigned int>	funcRemap(50);

	ByteCode *bCode = (ByteCode*)code;

	typeRemap.clear();
	// Add all types from bytecode to the list
	ExternTypeInfo *tInfo = FindFirstType(bCode);
	for(unsigned int i = 0; i < bCode->typeCount; i++)
	{
	    const unsigned int index_none = ~0u;
	    
		unsigned int index = index_none;
		for(unsigned int n = 0; n < exTypes.size() && index == index_none; n++)
			if(strcmp(exTypes[n]->name, tInfo->name) == 0)
				index = n;

		if(index != index_none && exTypes[index]->size != tInfo->size)
		{
			sprintf(execError, "Link Error: type '%s' is redefined with a different size", tInfo->name);
			return false;
		}
		if(index == index_none)
		{
			typeRemap.push_back(exTypes.size());
			exTypes.push_back((ExternTypeInfo*)(new char[tInfo->structSize]));
			memcpy(exTypes.back(), tInfo, tInfo->structSize);
			exTypes.back()->name = (char*)(&exTypes.back()->name) + sizeof(exTypes.back()->name);
			exTypes.back()->next = NULL;	// no one cares
		}else{
			typeRemap.push_back(index);
		}

		tInfo = FindNextType(tInfo);
	}

	// Add all global variables
	ExternVarInfo *vInfo = FindFirstVar(bCode);
	for(unsigned int i = 0; i < bCode->variableCount; i++)
	{
		exVariables.push_back((ExternVarInfo*)(new char[vInfo->structSize]));
		memcpy(exVariables.back(), vInfo, vInfo->structSize);

		exVariables.back()->name = (char*)(&exVariables.back()->name) + sizeof(exVariables.back()->name);
		exVariables.back()->next = NULL;	// no one cares
		// Type index have to be updated
		exVariables.back()->type = typeRemap[vInfo->type];

		vInfo = FindNextVar(vInfo);
	}

	unsigned int oldGlobalSize = globalVarSize;
	globalVarSize += bCode->globalVarSize;

	unsigned int	oldExFuncSize = exFunctions.size();
	unsigned int	oldOffsetToGlobalCode = offsetToGlobalCode;
	funcRemap.clear();
	unsigned int	allFunctionSize = 0;
	// Add new functions
	ExternFuncInfo *fInfo = FindFirstFunc(bCode);
	for(unsigned int i = 0; i < bCode->functionCount; i++, fInfo = FindNextFunc(fInfo))
	{
		allFunctionSize += fInfo->codeSize;
		
        const unsigned int index_none = ~0u;
        
		unsigned int index = index_none;
		for(unsigned int n = 0; n < exFunctions.size() && index == index_none; n++)
			if(strcmp(exFunctions[n]->name, fInfo->name) == 0)
				index = n;

		// Suppose, this is an overload function
		bool isOverload = true;
		if(index != index_none)
		{
			for(unsigned int n = 0; n < exFunctions.size() && isOverload == true; n++)
			{
				if(strcmp(exFunctions[n]->name, fInfo->name) == 0)
				{
					if(fInfo->paramCount == exFunctions[n]->paramCount)
					{
						// If the parameter count is equal, test parameters
						unsigned int *paramList = (unsigned int*)((char*)(&fInfo->name) + sizeof(fInfo->name) + fInfo->nameLength + 1);
						unsigned int k;
						for(k = 0; k < fInfo->paramCount; k++)
							if(typeRemap[paramList[k]] != exFunctions[n]->paramList[k])
								break;
						if(k == fInfo->paramCount)
						{
							isOverload = false;
							index = n;
						}
					}
				}
			}
			if(!exFunctions[index]->isVisible)
				isOverload = true;
		}
		// If the function exists and is build-in or external, skip
		if(index != index_none && !isOverload && exFunctions[index]->address == -1)
		{
			funcRemap.push_back(index);
			continue;
		}
		// If the function exists and is internal, check if redefinition is allowed
		if(index != index_none && !isOverload)
		{
			if(redefinitions)
			{
				sprintf(execError, "Warning: function '%s' is redefined", fInfo->name);
			}else{
				sprintf(execError, "Link Error: function '%s' is redefined", fInfo->name);
				return false;
			}
		}
		if(index == index_none || isOverload)
		{
			funcRemap.push_back(exFunctions.size());

			exFunctions.push_back((ExternFuncInfo*)(new char[fInfo->structSize]));
			memcpy(exFunctions.back(), fInfo, fInfo->structSize);

			exFunctions.back()->name = (char*)(&exFunctions.back()->name) + sizeof(exFunctions.back()->name);
			exFunctions.back()->next = NULL;	// no one cares
			exFunctions.back()->paramList = (unsigned int*)((char*)(&exFunctions.back()->name) + sizeof(fInfo->name) + fInfo->nameLength + 1);

			// Type index have to be updated
			exFunctions.back()->retType = typeRemap[exFunctions.back()->retType];
			if(exFunctions.back()->funcPtr != NULL && exTypes[exFunctions.back()->retType]->size > 8 && exTypes[exFunctions.back()->retType]->type != ExternTypeInfo::TYPE_DOUBLE)
			{
				strcpy(execError, "ERROR: user functions with return type size larger than 4 bytes are not supported");
				return false;
			}
			for(unsigned int n = 0; n < exFunctions.back()->paramCount; n++)
				exFunctions.back()->paramList[n] = typeRemap[((unsigned int*)((char*)(&fInfo->name) + sizeof(fInfo->name) + fInfo->nameLength + 1))[n]];

			if(fInfo->funcType == ExternFuncInfo::LOCAL)
			{
				exFunctions.back()->address -= bCode->globalCodeStart;
				allFunctionSize -= fInfo->codeSize;
				continue;
			}
			// Add function code to the bytecode,
			// shifting global code forward,
			// fixing all jump addresses in global code

			if(exFunctions.back()->address != -1)
			{
				unsigned int shift = fInfo->codeSize;
				unsigned int oldCodeSize = exCode.size();
				// Expand bytecode
				exCode.resize(exCode.size() + shift);
				// Move global code
				memmove(&exCode[offsetToGlobalCode] + shift, &exCode[offsetToGlobalCode], oldCodeSize-offsetToGlobalCode);
				// Insert function code
				memcpy(&exCode[offsetToGlobalCode], FindCode(bCode) + fInfo->address, shift);
				// Update function position
			
				exFunctions.back()->address = offsetToGlobalCode;
				// Fix cmdJmp*, cmdCall, cmdCallStd and commands with absolute addressing in function code
				int pos = exFunctions.back()->address;
				while(pos < exFunctions.back()->address + exFunctions.back()->codeSize)
				{
					CmdID cmd = *(CmdID*)(&exCode[pos]);
					CmdFlag cFlag = *(CmdFlag*)(&exCode[pos+2]);
					switch(cmd)
					{
					case cmdPushCharAbs:
					case cmdPushShortAbs:
					case cmdPushIntAbs:
					case cmdPushFloatAbs:
					case cmdPushDorLAbs:
					case cmdPushCmplxAbs:
					case cmdPush:
					case cmdMov:
						*(unsigned int*)(&exCode[pos+4]) += oldGlobalSize;
						break;
					case cmdJmp:
						*(unsigned int*)(&exCode[pos+2]) += exFunctions.back()->address - fInfo->address;
						break;
					case cmdJmpZ:
					case cmdJmpNZ:
						*(unsigned int*)(&exCode[pos+3]) += exFunctions.back()->address - fInfo->address;
						break;
					}
					pos += CommandList::GetCommandLength(cmd, cFlag);
				}
				// Update global code start
				offsetToGlobalCode += shift;
			}
		}else{
			funcRemap.push_back(index);
			assert(!"No function rewrite at the moment");
		}
	}

	for(unsigned int n = oldExFuncSize; n < exFunctions.size(); n++)
	{
		if(exFunctions[n]->funcType == ExternFuncInfo::LOCAL)
			exFunctions[n]->address += offsetToGlobalCode;
		if(exFunctions.back()->address != -1)
		{
			// Fix cmdCall in function code
			int pos = exFunctions.back()->address;
			while(pos < exFunctions.back()->address + exFunctions.back()->codeSize)
			{
				CmdID cmd = *(CmdID*)(&exCode[pos]);
				CmdFlag cFlag = *(CmdFlag*)(&exCode[pos+2]);
				switch(cmd)
				{
				case cmdCall:
					if(*(unsigned int*)(&exCode[pos+2]) != CALL_BY_POINTER)
					{
                        const unsigned int index_none = ~0u;
                        
						unsigned int index = index_none;
						ExternFuncInfo *fInfo = FindFirstFunc(bCode);
						for(unsigned int n = 0; n < bCode->functionCount && index == index_none; n++, fInfo = FindNextFunc(fInfo))
							if(*(int*)(&exCode[pos+2]) == fInfo->oldAddress)
								index = n;
						assert(index != index_none);
						*(unsigned int*)(&exCode[pos+2]) = exFunctions[funcRemap[index]]->address;
					}
					break;
				case cmdCallStd:
					*(unsigned int*)(&exCode[pos+2]) = funcRemap[*(unsigned int*)(&exCode[pos+2])];
					break;
				case cmdFuncAddr:
					*(unsigned int*)(&exCode[pos+2]) = funcRemap[*(unsigned int*)(&exCode[pos+2])];
					break;
				}
				pos += CommandList::GetCommandLength(cmd, cFlag);
			}
		}
	}

	unsigned int oldCodeSize = exCode.size();
	// Fix cmdJmp* in old code
	unsigned int pos = offsetToGlobalCode;
	while(pos < oldCodeSize)
	{
		CmdID cmd = *(CmdID*)(&exCode[pos]);
		CmdFlag cFlag = *(CmdFlag*)(&exCode[pos+2]);
		switch(cmd)
		{
		case cmdJmp:
			*(unsigned int*)(&exCode[pos+2]) += offsetToGlobalCode - oldOffsetToGlobalCode;
			break;
		case cmdJmpZ:
		case cmdJmpNZ:
			*(unsigned int*)(&exCode[pos+3]) += offsetToGlobalCode - oldOffsetToGlobalCode;
			break;
		}
		pos += CommandList::GetCommandLength(cmd, cFlag);
	}

	// Add new global code

	// Expand bytecode
	exCode.resize(exCode.size() + bCode->codeSize - allFunctionSize);
	// Insert function code
	memcpy(&exCode[oldCodeSize], FindCode(bCode) + allFunctionSize, bCode->codeSize - allFunctionSize);

	// Fix cmdJmp*, cmdCall, cmdCallStd and commands with absolute addressing in new code
	pos = oldCodeSize;
	while(pos < exCode.size())
	{
		CmdID cmd = *(CmdID*)(&exCode[pos]);
		CmdFlag cFlag = *(CmdFlag*)(&exCode[pos+2]);
		switch(cmd)
		{
		case cmdPushCharAbs:
		case cmdPushShortAbs:
		case cmdPushIntAbs:
		case cmdPushFloatAbs:
		case cmdPushDorLAbs:
		case cmdPushCmplxAbs:
		case cmdPush:
		case cmdMov:
			*(unsigned int*)(&exCode[pos+4]) += oldGlobalSize;
			break;
		case cmdJmp:
			*(unsigned int*)(&exCode[pos+2]) += oldCodeSize - allFunctionSize;
			break;
		case cmdJmpZ:
		case cmdJmpNZ:
			*(unsigned int*)(&exCode[pos+3]) += oldCodeSize - allFunctionSize;
			break;
		case cmdCall:
			if(*(unsigned int*)(&exCode[pos+2]) != CALL_BY_POINTER)
			{
                const unsigned int index_none = ~0u;
                
				unsigned int index = index_none;
				ExternFuncInfo *fInfo = FindFirstFunc(bCode);
				for(unsigned int n = 0; n < bCode->functionCount && index == index_none; n++, fInfo = FindNextFunc(fInfo))
					if(*(int*)(&exCode[pos+2]) == fInfo->oldAddress)
						index = n;
				if(index != index_none)
					*(unsigned int*)(&exCode[pos+2]) = exFunctions[funcRemap[index]]->address;
				else
					*(unsigned int*)(&exCode[pos+2]) += oldCodeSize - allFunctionSize;
			}
			break;
		case cmdCallStd:
			*(unsigned int*)(&exCode[pos+2]) = funcRemap[*(unsigned int*)(&exCode[pos+2])];
			break;
		case cmdFuncAddr:
			*(unsigned int*)(&exCode[pos+2]) = funcRemap[*(unsigned int*)(&exCode[pos+2])];
			break;
		}
		pos += CommandList::GetCommandLength(cmd, cFlag);
	}

	exFuncInfo.resize(exFunctions.size());
	for(unsigned int n = 0; n < exFunctions.size(); n++)
	{
		ExternFuncInfo* funcInfoPtr = exFunctions[n];

		if(funcInfoPtr->funcPtr && !CreateExternalInfo(funcInfoPtr, exFuncInfo[n]))
		{
			sprintf(execError, "Link Error: External function info failed for '%s'", funcInfoPtr->name);
			return false;
		}
	}

#ifdef NULLC_LOG_FILES
	ostringstream logASM;
	logASM.str("");
	CommandList::PrintCommandListing(&logASM, &exCode[0], &exCode[exCode.size()]);

	ofstream m_FileStream("link.txt", std::ios::binary);
	m_FileStream << logASM.str();
	m_FileStream.flush();
#endif

	return true;
}

int runResult = 0;
int runResult2 = 0;
OperFlag runResultType = OTYPE_DOUBLE;

unsigned int stackReallocs;

unsigned int expCodePublic;
unsigned int expAllocCode;
unsigned int expECXstate;
DWORD CanWeHandleSEH(unsigned int expCode, _EXCEPTION_POINTERS* expInfo)
{
	expECXstate = expInfo->ContextRecord->Ecx;
	expCodePublic = expCode;
	if(expCode == EXCEPTION_INT_DIVIDE_BY_ZERO || expCode == EXCEPTION_BREAKPOINT || expCode == EXCEPTION_STACK_OVERFLOW)
		return EXCEPTION_EXECUTE_HANDLER;
	if(expCode == EXCEPTION_ACCESS_VIOLATION)
	{
		if(expInfo->ExceptionRecord->ExceptionInformation[1] > paramDataBase &&
			expInfo->ExceptionRecord->ExceptionInformation[1] < expInfo->ContextRecord->Edi+paramDataBase+64*1024)
		{
			// ��������, �� �������� �� �� ����� ��������� ������
			if(reservedStack > 512*1024*1024)
			{
				expAllocCode = 4;
				return EXCEPTION_EXECUTE_HANDLER;
			}
			// �������� ������������� ��������� �������� ����������������� ������
			if(!VirtualAlloc(reinterpret_cast<void*>(long long(paramDataBase+commitedStack)), stackGrowSize-stackGrowCommit, MEM_COMMIT, PAGE_READWRITE))
			{
				expAllocCode = 1; // failed to commit all old memory
				return EXCEPTION_EXECUTE_HANDLER;
			}
			// ������������� ��� ������ ����� ����� ����������� �����
			if(!VirtualAlloc(reinterpret_cast<void*>(long long(paramDataBase+reservedStack)), stackGrowSize, MEM_RESERVE, PAGE_NOACCESS))
			{
				expAllocCode = 2; // failed to reserve new memory
				return EXCEPTION_EXECUTE_HANDLER;
			}
			// �������� ������������� ���� ����������������� ������ ����� ��������� ��������
			if(!VirtualAlloc(reinterpret_cast<void*>(long long(paramDataBase+reservedStack)), stackGrowCommit, MEM_COMMIT, PAGE_READWRITE))
			{
				expAllocCode = 3; // failed to commit new memory
				return EXCEPTION_EXECUTE_HANDLER;
			}
			// ������� ����������
			commitedStack = reservedStack;
			reservedStack += stackGrowSize;
			commitedStack += stackGrowCommit;
			stackReallocs++;

			return (DWORD)EXCEPTION_CONTINUE_EXECUTION;
		}
	}

	return (DWORD)EXCEPTION_CONTINUE_SEARCH;
}

#pragma warning(disable: 4731)
void ExecutorX86::Run(const char* funcName) throw()
{
	if(!exCode.size())
	{
		strcpy(execError, "ERROR: no code to run");
		return;
	}

	execError[0] = 0;

	stackReallocs = 0;

	*(double*)(paramData) = 0.0;
	*(double*)(paramData+8) = 3.1415926535897932384626433832795;
	*(double*)(paramData+16) = 2.7182818284590452353602874713527;

	unsigned int binCodeStart = static_cast<unsigned int>(reinterpret_cast<long long>(&binCode[20]));

	if(funcName)
	{
		unsigned int funcPos = (unsigned int)-1;
		unsigned int fnameHash = GetStringHash(funcName);
		for(int i = (int)exFunctions.size()-1; i >= 0; i--)
		{
			if(exFunctions[i]->nameHash == fnameHash)
			{
				funcPos = exFuncInfo[i].startInByteCode;
				break;
			}
		}
		if(funcPos == -1)
		{
			strcpy(execError, "Cannot find starting function");
			return;
		}
		binCodeStart += funcPos;
	}else{
		binCodeStart += globalStartInBytecode;
	}

	unsigned int varSize = globalVarSize;

	unsigned int res1 = 0;
	unsigned int res2 = 0;
	unsigned int resT = 0;
	__try 
	{
		__asm
		{
			pusha ; // �������� ��� ��������
			mov eax, binCodeStart ;
			
			// ����������� ���� �� ������� 8 ����
			lea ebx, [esp+8];
			and ebx, 0fh;
			mov ecx, 16;
			sub ecx, ebx;
			sub esp, ecx;

			push ecx; // �������� �� ������� �������� ����
			push ebp; // �������� ���� ����� (� ������� ����������� �� popa)

			mov edi, varSize ;
			mov ebp, 0h ;

			call eax ; // � ebx ��� ������������ ��������

			pop ebp; // ���������� ���� �����
			pop ecx;
			add esp, ecx;

			mov dword ptr [res1], eax;
			mov dword ptr [res2], edx;
			mov dword ptr [resT], ebx;

			popa ;
		}
	}__except(CanWeHandleSEH(GetExceptionCode(), GetExceptionInformation())){
		if(expCodePublic == EXCEPTION_INT_DIVIDE_BY_ZERO)
			strcpy(execError, "ERROR: integer division by zero");
		if(expCodePublic == EXCEPTION_BREAKPOINT && expECXstate != 0xFFFFFFFF)
			strcpy(execError, "ERROR: array index out of bounds");
		if(expCodePublic == EXCEPTION_BREAKPOINT && expECXstate == 0xFFFFFFFF)
			strcpy(execError, "ERROR: function didn't return a value");
		if(expCodePublic == EXCEPTION_STACK_OVERFLOW)
			strcpy(execError, "ERROR: stack overflow");
		if(expCodePublic == EXCEPTION_ACCESS_VIOLATION)
		{
			if(expAllocCode == 1)
				strcpy(execError, "ERROR: Failed to commit old stack memory");
			if(expAllocCode == 2)
				strcpy(execError, "ERROR: Failed to reserve new stack memory");
			if(expAllocCode == 3)
				strcpy(execError, "ERROR: Failed to commit new stack memory");
			if(expAllocCode == 4)
				strcpy(execError, "ERROR: No more memory (512Mb maximum exceeded)");
		}
	}

	runResult = res1;
	runResult2 = res2;
	runResultType = (OperFlag)resT;
}
#pragma warning(default: 4731)

void ExecutorX86::GenListing()
{
	logASM.str("");

	unsigned int pos = 0, pos2 = 0;
	CmdID	cmd, cmdNext;
	unsigned int	valind, valind2;

	CmdFlag cFlag;
	OperFlag oFlag;
	asmStackType st;
	asmDataType dt;

	vector<unsigned int> instrNeedLabel;	// ����� �� ����� ����������� ����� �����
	vector<unsigned int> funcNeedLabel;	// ����� �� ����� ����������� ����� �������

	globalStartInBytecode = 0xffffffff;
	for(unsigned int i = 0; i < exFunctions.size(); i++)
	{
		exFuncInfo[i].startInByteCode = 0xffffffff;
		if(exFunctions[i]->funcPtr == NULL && exFunctions[i]->address != -1)
			funcNeedLabel.push_back(exFunctions[i]->address);
	}

	//������, ���� ����� ������
	while(pos < exCode.size())
	{
		CmdID cmd = *(CmdID*)(&exCode[pos]);
		CmdFlag cFlag = *(CmdFlag*)(&exCode[pos+2]);
		switch(cmd)
		{
		case cmdJmp:
			instrNeedLabel.push_back(*(unsigned int*)(&exCode[pos+2]));
			break;
		case cmdJmpZ:
		case cmdJmpNZ:
			instrNeedLabel.push_back(*(unsigned int*)(&exCode[pos+3]));
			break;
		}
		pos += CommandList::GetCommandLength(cmd, cFlag);
	}

	logASM << "use32\r\n";
	unsigned int typeSizeD[] = { 1, 2, 4, 8, 4, 8 };

	int pushLabels = 1;
	int movLabels = 1;
	//int skipLabels = 1;
	int aluLabels = 1;

	bool skipPopEAXOnIntALU = false;
	bool skipFldESPOnDoubleALU = false;
	bool skipFldOnMov = false;

	bool skipPopEDXOnPush = false;
	bool indexInEaxOnCti = false;

	bool knownEDXOnPush = false;
	bool addEBPtoEDXOnPush = false;
	int edxValueForPush = 0;

	bool skipPop = false;

	unsigned int lastVarSize = 0;
	bool mulByVarSize = false;

	pos = 0;
	pos2 = 0;
	while(pos < exCode.size())
	{
		cmd = *(CmdID*)(&exCode[pos]);
		for(unsigned int i = 0; i < instrNeedLabel.size(); i++)
		{
			if(pos == instrNeedLabel[i])
			{
				logASM << "  gLabel" << pos << ": \r\n";
				break;
			}
		}
		for(unsigned int i = 0; i < funcNeedLabel.size(); i++)
		{
			if(pos == funcNeedLabel[i])
			{
				logASM << "  dd " << (('N' << 24) | pos) << "; marker \r\n";
				logASM << "  function" << pos << ": \r\n";
				break;
			}
		}

		if(pos == offsetToGlobalCode)
		{
			logASM << "  dd " << (('G' << 24) | offsetToGlobalCode) << "; global marker \r\n";
			logASM << "push ebp\r\n";
		}

		pos2 = pos;
		pos += 2;
	//	const char *descStr = cmdList->GetDescription(pos2);
	//	if(descStr)
	//		logASM << "\r\n  ; \"" << descStr << "\" codeinfo\r\n";

		switch(cmd)
		{
		case cmdDTOF:
			logASM << "  ; DTOF \r\n";
			logASM << "fld qword [esp] \r\n";
			logASM << "fstp dword [esp+4] \r\n";
			logASM << "add esp, 4 \r\n";
			break;
		case cmdCallStd:
			logASM << "  ; CALLSTD ";
			valind = *(unsigned int*)(&exCode[pos]);
			pos += sizeof(unsigned int);

			if(exFunctions[valind]->funcPtr == NULL)
			{
				if(exFunctions[valind]->nameHash == GetStringHash("cos"))
				{
					logASM << "cos \r\n";
					logASM << "fld qword [esp] \r\n";
					logASM << "fsincos \r\n";
					logASM << "fstp qword [esp] \r\n";
					logASM << "fstp st \r\n";
				}else if(exFunctions[valind]->nameHash == GetStringHash("sin")){
					logASM << "sin \r\n";
					logASM << "fld qword [esp] \r\n";
					logASM << "fsincos \r\n";
					logASM << "fstp st \r\n";
					logASM << "fstp qword [esp] \r\n";
				}else if(exFunctions[valind]->nameHash == GetStringHash("tan")){
					logASM << "tan \r\n";
					logASM << "fld qword [esp] \r\n";
					logASM << "fptan \r\n";
					logASM << "fstp st \r\n";
					logASM << "fstp qword [esp] \r\n";
				}else if(exFunctions[valind]->nameHash == GetStringHash("ctg")){
					logASM << "ctg \r\n";
					logASM << "fld qword [esp] \r\n";
					logASM << "fptan \r\n";
					logASM << "fdivrp \r\n";
					logASM << "fstp qword [esp] \r\n";
				}else if(exFunctions[valind]->nameHash == GetStringHash("ceil")){
					logASM << "ceil \r\n";
					logASM << "fld qword [esp] \r\n";
					logASM << "push eax ; ���� ������� ���� fpu \r\n";
					logASM << "fstcw word [esp] ; �������� ���� �������� \r\n";
					logASM << "mov word [esp+2], 1BBFh ; �������� ���� � ����������� � +inf \r\n";
					logASM << "fldcw word [esp+2] ; ��������� ��� \r\n";
					logASM << "frndint ; �������� �� ������ \r\n";
					logASM << "fldcw word [esp] ; ���������� ���� �������� \r\n";
					logASM << "fstp qword [esp+4] \r\n";
					logASM << "pop eax ; \r\n";
				}else if(exFunctions[valind]->nameHash == GetStringHash("floor")){
					logASM << "floor \r\n";
					logASM << "fld qword [esp] \r\n";
					logASM << "push eax ; ���� ������� ���� fpu \r\n";
					logASM << "fstcw word [esp] ; �������� ���� �������� \r\n";
					logASM << "mov word [esp+2], 17BFh ; �������� ���� � ����������� � -inf \r\n";
					logASM << "fldcw word [esp+2] ; ��������� ��� \r\n";
					logASM << "frndint ; �������� �� ������ \r\n";
					logASM << "fldcw word [esp] ; ���������� ���� �������� \r\n";
					logASM << "fstp qword [esp+4] \r\n";
					logASM << "pop eax ; \r\n";
				}else if(exFunctions[valind]->nameHash == GetStringHash("sqrt")){
					logASM << "sqrt \r\n";
					logASM << "fld qword [esp] \r\n";
					logASM << "fsqrt \r\n";
					logASM << "fstp qword [esp] \r\n";
					logASM << "fstp st \r\n";
				}else{
					throw std::string("ERROR: there is no such function: ") + exFunctions[valind]->name;
				}
			}else{
				if(exTypes[exFunctions[valind]->retType]->size > 4 && exTypes[exFunctions[valind]->retType]->type != TypeInfo::TYPE_DOUBLE)
					throw std::string("ERROR: user functions with return type size larger than 4 bytes are not supported");
				unsigned int bytesToPop = 0;
				for(unsigned int i = 0; i < exFunctions[valind]->paramCount; i++)
				{
					bytesToPop += exTypes[exFunctions[valind]->paramList[i]]->size > 4 ? exTypes[exFunctions[valind]->paramList[i]]->size : 4;
				}
				logASM << exFunctions[valind]->name << "\r\n";
				logASM << "mov ecx, 0x" << exFunctions[valind]->funcPtr << " ; " << exFunctions[valind]->name << "() \r\n";
				logASM << "call ecx \r\n";
				logASM << "add esp, " << bytesToPop << " \r\n";
				if(exTypes[exFunctions[valind]->retType]->size != 0)
					logASM << "push eax \r\n";
			}
			break;
		case cmdPushVTop:
			logASM << "  ; PUSHT\r\n";
			logASM << "push ebp ; ��������� ������� ���� ����� ����������\r\n";
			logASM << "mov ebp, edi ; ���������� ����� ���� ����� ����������, �� ������� �����\r\n";
			break;
		case cmdPopVTop:
			logASM << "  ; POPT\r\n";
			logASM << "mov edi, ebp ; ������������ ���������� ������ ����� ����������\r\n";
			logASM << "pop ebp ; ������������ ���������� ���� ����� ����������\r\n";
			break;
		case cmdCall:
			{
				RetFlag retFlag;
				valind = *(unsigned int*)(&exCode[pos]);
				pos += 4;
				retFlag = *(unsigned short*)(&exCode[pos]);
				pos += 2;
				logASM << "  ; CALL " << valind << " ret " << (retFlag & bitRetSimple ? "simple " : "") << "size: ";
				if(retFlag & bitRetSimple)
				{
					oFlag = (OperFlag)(retFlag & 0x0FFF);
					if(oFlag == OTYPE_DOUBLE)
						logASM << "double\r\n";
					if(oFlag == OTYPE_LONG)
						logASM << "long\r\n";
					if(oFlag == OTYPE_INT)
						logASM << "int\r\n";
				}else{
					logASM << (retFlag&0x0FFF) << "\r\n";
				}
				if(valind == -1)
				{
					logASM << "pop eax ;\r\n";
					logASM << "call eax ; \r\n";
				}else{
					logASM << "call function" << valind << "\r\n";
				}
				if(retFlag & bitRetSimple)
				{
					oFlag = (OperFlag)(retFlag & 0x0FFF);
					if(oFlag == OTYPE_INT)
						logASM << "push eax ; �������� int ������� � ����\r\n";
					if(oFlag == OTYPE_DOUBLE)
					{
						logASM << "push eax ; \r\n";
						logASM << "push edx ; �������� double ������� � ����\r\n";
					}
					if(oFlag == OTYPE_LONG)
					{
						logASM << "push eax ; \r\n";
						logASM << "push edx ; �������� long ������� � ����\r\n";
					}
				}else{
					if(retFlag != 0)
					{
						if(retFlag == 4)
						{
							logASM << "push eax ; �������� �����. ���������� � 4 ����� �� ��������\r\n";
						}else if(retFlag == 8){
							logASM << "push eax \r\n";
							logASM << "push edx ; �������� �����. ���������� � 8 ���� � ��������\r\n";
						}else if(retFlag == 12){
							logASM << "push eax \r\n";
							logASM << "push edx \r\n";
							logASM << "push ecx ; �������� �����. ���������� � 12 ���� � ��������\r\n";
						}else if(retFlag == 16){
							logASM << "push eax \r\n";
							logASM << "push edx \r\n";
							logASM << "push ecx \r\n";
							logASM << "push ebx ; �������� �����. ���������� � 16 ���� � ��������\r\n";
						}else{
							logASM << "sub esp, " << retFlag << "; ��������� � ����� ����� ��� ����������\r\n";

							logASM << "mov ebx, edi ; �������� ����� edi\r\n";
							
							logASM << "lea esi, [eax + " << paramBase << "] ; �������� ���� � ������� ����� ����������\r\n";
							logASM << "mov edi, esp ; ���������� �� ����� �� ������� ����� ����������\r\n";
							logASM << "mov ecx, " << retFlag/4 << " ; ������ ����������\r\n";
							logASM << "rep movsd ; ��������\r\n";

							logASM << "mov edi, ebx ; �������������� edi\r\n";
						}
					}
				}
			}
			break;
		case cmdReturn:
			{
				unsigned short	retFlag, popCnt;
				logASM << "  ; RET\r\n";
				retFlag = *(unsigned short*)(&exCode[pos]);
				pos += 2;
				popCnt = *(unsigned short*)(&exCode[pos]);
				pos += 2;
				if(retFlag & bitRetError)
				{
					logASM << "mov ecx, " << 0xffffffff << " ; ������, ����� �� ������� �������\r\n";
					logASM << "int 3 ; ��������� ����������\r\n";
					break;
				}
				if(retFlag == 0)
				{
					logASM << "mov edi, ebp ; ������������ ���������� ������ ����� ����������\r\n";
					logASM << "pop ebp ; ������������ ���������� ���� ����� ����������\r\n";
					logASM << "ret ; ������������ �� �������\r\n";
					break;
				}
				if(retFlag & bitRetSimple)
				{
					oFlag = (OperFlag)(retFlag & 0x0FFF);
					if(oFlag == OTYPE_DOUBLE)
					{
						logASM << "pop edx \r\n";
						logASM << "pop eax ; �� ����� �������� double � ��������\r\n";
					}else if(oFlag == OTYPE_LONG){
						logASM << "pop edx \r\n";
						logASM << "pop eax ; �� ����� �������� long � ��������\r\n";
					}else if(oFlag == OTYPE_INT){
						logASM << "pop eax ; �� ����� �������� int � �������\r\n";
					}
					for(int pops = 0; pops < (popCnt > 0 ? popCnt : 1); pops++)
					{
						logASM << "mov edi, ebp ; ������������ ���������� ������ ����� ����������\r\n";
						logASM << "pop ebp ; ������������ ���������� ���� ����� ����������\r\n";
					}
					if(popCnt == 0)
						logASM << "mov ebx, " << (unsigned int)(oFlag) << " ; �������� oFlag ����� ������� �����, ����� ��� ��������\r\n";
				}else{
					if(retFlag == 4)
					{
						logASM << "pop eax ; �������� �����. ���������� � 4 ����� � �������\r\n";
					}else if(retFlag == 8){
						logASM << "pop edx \r\n";
						logASM << "pop eax ; �������� �����. ���������� � 8 ���� � ��������\r\n";
					}else if(retFlag == 12){
						logASM << "pop ecx \r\n";
						logASM << "pop edx \r\n";
						logASM << "pop eax ; �������� �����. ���������� � 12 ���� � ��������\r\n";
					}else if(retFlag == 16){
						logASM << "pop ebx \r\n";
						logASM << "pop ecx \r\n";
						logASM << "pop edx \r\n";
						logASM << "pop eax ; �������� �����. ���������� � 12 ���� � ��������\r\n";
					}else{
						logASM << "mov ebx, edi ; �������� edi\r\n";

						logASM << "mov esi, esp ; �������� ���� �� ����� � ����\r\n";
						logASM << "lea edi, [edi + " << paramBase << "] ; ���������� �� ����� �� ������� ����� ����������\r\n";
						logASM << "mov ecx, " << retFlag/4 << " ; ������ ����������\r\n";
						logASM << "rep movsd ; ��������\r\n";

						logASM << "mov edi, ebx ; �������������� edi\r\n";

						logASM << "add esp, " << retFlag << "; ������� ���� �� �������� ���� ����� ����������\r\n";
					}
					for(int pops = 0; pops < popCnt-1; pops++)
					{
						logASM << "mov edi, ebp ; ������������ ���������� ������ ����� ����������\r\n";
						logASM << "pop ebp ; ������������ ���������� ���� ����� ����������\r\n";
					}
					if(retFlag > 16)
						logASM << "mov eax, edi ; �������� ������ edi\r\n";
					logASM << "mov edi, ebp ; ������������ ���������� ������ ����� ����������\r\n";
					logASM << "pop ebp ; ������������ ���������� ���� ����� ����������\r\n";
					if(popCnt == 0)
						logASM << "mov ebx, " << 16 << " ; ���� ���������� return, �� ���������, ����� ��� ��������\r\n";
				}
				logASM << "ret ; ������������ �� �������\r\n";
			}
			break;
		case cmdPushV:
			logASM << "  ; PUSHV\r\n";
			valind = *(int*)(&exCode[pos]);
			pos += sizeof(int);
			logASM << "add edi, " << valind << " ; �������� ����� ��� ����� ���������� � �����\r\n";
			break;
		case cmdNop:
			logASM << "  ; NOP\r\n";
			logASM << "nop \r\n";
			break;
		case cmdCTI:
			oFlag = *(unsigned char*)(&exCode[pos]);
			pos += 1;
			valind = *(unsigned int*)(&exCode[pos]);
			pos += 4;
			logASM << "  ; CTI " << valind << "\r\n";
			switch(oFlag)
			{
			case OTYPE_DOUBLE:
				logASM << "fld qword [esp] \r\n";
				logASM << "fistp dword[esp+4] \r\n";
				logASM << "pop eax ; �������� double int'��\r\n";
				break;
			case OTYPE_LONG:
				logASM << "pop edx ; ������ ������� ���� long �� �����\r\n";
				break;
			case OTYPE_INT:
				break;
			}
			//look at the next command
			cmdNext = *(CmdID*)(&exCode[pos]);
			if(valind != 1)
			{
				char *indexPlace = "dword [esp]";
				if(indexInEaxOnCti)
				{
					indexPlace = "eax";
					skipPopEAXOnIntALU = true;
				}
				if((cmdNext == cmdPush || cmdNext == cmdMov || cmdNext == cmdIncAt || cmdNext == cmdDecAt) && (valind == 2 || valind == 4 || valind == 8))
				{
					mulByVarSize = true;
					lastVarSize = valind;
				}else if(valind == 2)
				{
					logASM << "shl " << indexPlace << ", 1 ; ������� ����� �� ������ ����������\r\n";
				}else if(valind == 4){
					logASM << "shl " << indexPlace << ", 2 ; ������� ����� �� ������ ����������\r\n";
				}else if(valind == 8){
					logASM << "shl " << indexPlace << ", 3 ; ������� ����� �� ������ ����������\r\n";
				}else if(valind == 16){
					logASM << "shl " << indexPlace << ", 4 ; ������� ����� �� ������ ����������\r\n";
				}else{
					if(!indexInEaxOnCti)
						logASM << "pop eax ; ������ � eax\r\n";
					logASM << "imul eax, " << valind << " ; ������� ����� �� ������ ����������\r\n";
					if(!indexInEaxOnCti)
						logASM << "push eax \r\n";
				}
			}else{
				if(indexInEaxOnCti)
				{
					if(cmdNext != cmdAdd)
						logASM << "push eax \r\n";
					else
						skipPopEAXOnIntALU = true;
				}
			}
			indexInEaxOnCti = false;
			break;
		case cmdPushImmt:
			{
				logASM << "  ; PUSHIMMT\r\n";
				unsigned int	highDW = 0, lowDW = 0;
				unsigned short sdata;
				unsigned char cdata;
				cFlag = *(unsigned short*)(&exCode[pos]);
				pos += 2;
				st = flagStackType(cFlag);
				dt = flagDataType(cFlag);

				char *texts[] = { "", "edx + ", "ebp + ", "push ", "mov eax, " };
				char *needPush = texts[3];
				addEBPtoEDXOnPush = false;

				mulByVarSize = false;

				if(dt == DTYPE_DOUBLE || dt == DTYPE_LONG)
				{
					highDW = *(unsigned int*)(&exCode[pos]); pos += 4;
					lowDW = *(unsigned int*)(&exCode[pos]); pos += 4;
					logASM << "push " << lowDW << "\r\n";
					logASM << "push " << highDW << " ; �������� double ��� long long\r\n";
				}
				if(dt == DTYPE_FLOAT)
				{
					// ����� ����� ��� double
					lowDW = *(unsigned int*)(&exCode[pos]); pos += 4;
					double res = (double)*((float*)(&lowDW));
					logASM << "push " << *((unsigned int*)(&res)+1) << "\r\n";
					logASM << "push " << *((unsigned int*)(&res)) << " ; �������� float ��� double\r\n";
				}
				if(dt == DTYPE_INT)
				{
					//look at the next command
					cmdNext = *(CmdID*)(&exCode[pos+4]);
					if(cmdNext >= cmdAdd && cmdNext <= cmdLogOr) // for binary commands except LogicalXOR
					{
						needPush = texts[4];
						skipPopEAXOnIntALU = true;
					}
					if(cmdNext == cmdPush || cmdNext == cmdMov || cmdNext == cmdIncAt || cmdNext == cmdDecAt)
					{
						CmdFlag lcFlag;
						lcFlag = *(unsigned short*)(&exCode[pos+6]);
						if((flagAddrAbs(lcFlag) || flagAddrRel(lcFlag)) && flagShiftStk(lcFlag))
							knownEDXOnPush = true;
					}

					lowDW = *(unsigned int*)(&exCode[pos]); pos += 4;
					if(knownEDXOnPush)
						edxValueForPush = (int)lowDW;
					else
						logASM << needPush << (int)lowDW << " ; �������� int\r\n";
				}
				if(dt == DTYPE_SHORT)
				{
					sdata = *(unsigned short*)(&exCode[pos]); pos += 2;
					lowDW = (sdata > 0 ? sdata : sdata | 0xFFFF0000);
					logASM << "push " << lowDW << " ; �������� short\r\n";
				}
				if(dt == DTYPE_CHAR)
				{
					cdata = *(unsigned char*)(&exCode[pos]); pos += 1;
					lowDW = cdata;
					logASM << "push " << lowDW << " ; �������� char\r\n";
				}
			}
			break;
		case cmdPush:
			{
				logASM << "  ; PUSH\r\n";
				int valind = -1, /*shift, */size;
				unsigned int	highDW = 0, lowDW = 0;
				unsigned short sdata;
				unsigned char cdata;
				cFlag = *(unsigned short*)(&exCode[pos]);
				pos += 2;
				st = flagStackType(cFlag);
				dt = flagDataType(cFlag);

				char *texts[] = { "", "edx + ", "ebp + ", "push ", "mov eax, " };
				char *needPush = texts[3];
				char *needEDX = texts[1];
				char *needEBP = texts[2];
				if(flagAddrAbs(cFlag) && !addEBPtoEDXOnPush)
					needEBP = texts[0];
				addEBPtoEDXOnPush = false;
				unsigned int numEDX = 0;

				// ���� �������� �� ���������� �...
				if((flagAddrAbs(cFlag) || flagAddrRel(cFlag)) && flagShiftStk(cFlag))
				{
					// ...���� ����� � ������� � ������� ����� � �����
					valind = *(int*)(&exCode[pos]);
					pos += 4;
					if(knownEDXOnPush)
					{
						if(mulByVarSize)
							numEDX = edxValueForPush * lastVarSize + valind;
						else
							numEDX = edxValueForPush + valind;
						needEDX = texts[0];
						knownEDXOnPush = false;
					}else{
						if(skipPopEDXOnPush)
						{
							if(mulByVarSize)
							{
								if(valind != 0)
									logASM << "lea edx, [edx*" << lastVarSize << " + " << valind << "]\r\n";
								else
									logASM << "lea edx, [edx*" << lastVarSize << "]\r\n";
							}else{
								numEDX = valind;
							}
							skipPopEDXOnPush = false;
						}else{
							if(mulByVarSize)
							{
								logASM << "pop eax ; ����� �����\r\n";
								if(valind != 0)
									logASM << "lea edx, [eax*" << lastVarSize << " + " << valind << "] ; ����� ��������� �� ���� ���������� � ������� �� ����� � ����� � �� ������������ ������\r\n";
								else
									logASM << "lea edx, [eax*" << lastVarSize << "] ; ����� ��������� �� ���� ���������� � ������� �� ����� � ����� (opt: addr==0)\r\n";
							}else{
								if(valind != 0)
								{
									logASM << "pop edx ; ����� �����\r\n";
									numEDX = valind;
								}else{
									logASM << "pop edx ; ����� ��������� �� ���� ���������� � ������� �� ����� � ����� (opt: addr==0)\r\n";
								}
							}
						}
					}
				}else if((flagAddrAbs(cFlag) || flagAddrRel(cFlag)))
				{
					// ...���� ����� � �������
					valind = *(int*)(&exCode[pos]);
					pos += 4;

					needEDX = texts[0];
					numEDX = valind;
				}

				if(flagSizeOn(cFlag))
				{
					size = *(int*)(&exCode[pos]);
					pos += 4;
					logASM << "cmp eax, " << (mulByVarSize ? size/lastVarSize : size) << " ; ������� ����� � ������������\r\n";
					logASM << "jb pushLabel" << pushLabels << " ; ���� ����� ������ ��������� (� �� �������������) �� �� ��\r\n";
					logASM << "int 3 \r\n";
					logASM << "  pushLabel" << pushLabels << ":\r\n";
					pushLabels++;
				}
				if(flagSizeStk(cFlag))
				{
					logASM << "cmp [esp], eax ; ������� � ������������ ������� � �����\r\n";
					logASM << "ja pushLabel" << pushLabels << " ; ���� ����� ������ ��������� (� �� �������������) �� �� ��\r\n";
					logASM << "int 3 \r\n";
					logASM << "  pushLabel" << pushLabels << ":\r\n";
					logASM << "pop eax ; ������ �������������� ������\r\n";
					pushLabels++;
				}
				mulByVarSize = false;

				if(flagNoAddr(cFlag))
				{
					if(dt == DTYPE_DOUBLE || dt == DTYPE_LONG)
					{
						highDW = *(unsigned int*)(&exCode[pos]); pos += 4;
						lowDW = *(unsigned int*)(&exCode[pos]); pos += 4;
						logASM << "push " << lowDW << "\r\n";
						logASM << "push " << highDW << " ; �������� double ��� long long\r\n";
					}
					if(dt == DTYPE_FLOAT)
					{
						// ����� ����� ��� double
						lowDW = *(unsigned int*)(&exCode[pos]); pos += 4;
						double res = (double)*((float*)(&lowDW));
						logASM << "push " << *((unsigned int*)(&res)+1) << "\r\n";
						logASM << "push " << *((unsigned int*)(&res)) << " ; �������� float ��� double\r\n";
					}
					if(dt == DTYPE_INT)
					{
						//look at the next command
						cmdNext = *(CmdID*)(&exCode[pos+4]);
						if(cmdNext >= cmdAdd && cmdNext <= cmdLogOr) // for binary commands except LogicalXOR
						{
							needPush = texts[4];
							skipPopEAXOnIntALU = true;
						}
						if(cmdNext == cmdPush || cmdNext == cmdMov || cmdNext == cmdIncAt || cmdNext == cmdDecAt)
						{
							CmdFlag lcFlag;
							lcFlag = *(unsigned short*)(&exCode[pos+6]);
							if((flagAddrAbs(lcFlag) || flagAddrRel(lcFlag)) && flagShiftStk(lcFlag))
								knownEDXOnPush = true;
						}

						lowDW = *(unsigned int*)(&exCode[pos]); pos += 4;
						if(knownEDXOnPush)
							edxValueForPush = (int)lowDW;
						else
							logASM << needPush << (int)lowDW << " ; �������� int\r\n";
					}
					if(dt == DTYPE_SHORT)
					{
						sdata = *(unsigned short*)(&exCode[pos]); pos += 2;
						lowDW = (sdata > 0 ? sdata : sdata | 0xFFFF0000);
						logASM << "push " << lowDW << " ; �������� short\r\n";
					}
					if(dt == DTYPE_CHAR)
					{
						cdata = *(unsigned char*)(&exCode[pos]); pos += 1;
						lowDW = cdata;
						logASM << "push " << lowDW << " ; �������� char\r\n";
					}
				}else{
					unsigned int sizeOfVar = 0;
					if(dt == DTYPE_COMPLEX_TYPE)
					{
						sizeOfVar = *(unsigned int*)(&exCode[pos]);
						pos += 4;
					}

					//look at the next command
					cmdNext = *(CmdID*)(&exCode[pos]);

					if(dt == DTYPE_COMPLEX_TYPE)
					{
						unsigned int currShift = sizeOfVar;
						while(sizeOfVar >= 4)
						{
							currShift -= 4;
							logASM << "push dword [" << needEDX << needEBP << paramBase+numEDX+currShift << "] ; �������� ����� complex\r\n";
							sizeOfVar -= 4;
						}
						if(sizeOfVar)
						{
							logASM << "push dword [" << needEDX << needEBP << paramBase+numEDX+currShift << "] ; �������� ����� complex\r\n";
							logASM << "add esp, " << 4-sizeOfVar << " ; ������ ������\r\n";
						}
					}
					if(dt == DTYPE_DOUBLE)
					{
						if(cmdNext >= cmdAdd && cmdNext <= cmdNEqual)
						{
							skipFldESPOnDoubleALU = true;
							logASM << "fld qword [" << needEDX << needEBP << paramBase+numEDX << "] ; �������� double ����� � FPU\r\n";
						}else if(cmdNext == cmdMov){
							logASM << "fld qword [" << needEDX << needEBP << paramBase+numEDX << "] ; �������� double ����� � FPU\r\n";
							skipFldOnMov = true;
						}else{
							logASM << "push dword [" << needEDX << needEBP << paramBase+4+numEDX << "]\r\n";
							logASM << "push dword [" << needEDX << needEBP << paramBase+numEDX << "] ; �������� double\r\n";
						}
					}
					if(dt == DTYPE_LONG)
					{
						logASM << "push dword [" << needEDX << needEBP << paramBase+4+numEDX << "]\r\n";
						logASM << "push dword [" << needEDX << needEBP << paramBase+numEDX << "] ; �������� long long\r\n";
					}
					if(dt == DTYPE_FLOAT)
					{
						if(cmdNext == cmdMov)
						{
							logASM << "fld dword [" << needEDX << needEBP << paramBase+numEDX << "] ; �������� float � fpu ����\r\n";
							skipFldOnMov = true;
						}else{
							logASM << "sub esp, 8 ; ��������� ����� ��� double\r\n";
							logASM << "fld dword [" << needEDX << needEBP << paramBase+numEDX << "] ; �������� float � fpu ����\r\n";
							logASM << "fstp qword [esp] ; �������� double � ������� ����\r\n";
						}
					}
					if(dt == DTYPE_INT)
					{
						if(cmdNext >= cmdAdd && cmdNext <= cmdLogOr) // for binary commands except LogicalXOR
						{
							needPush = texts[4];
							skipPopEAXOnIntALU = true;
						}
						if(cmdNext == cmdPush || cmdNext == cmdMov || cmdNext == cmdIncAt || cmdNext == cmdDecAt)
						{
							CmdFlag lcFlag;
							lcFlag = *(unsigned short*)(&exCode[pos+2]);
							if((flagAddrAbs(lcFlag) || flagAddrRel(lcFlag)) && flagShiftStk(lcFlag))
							{
								skipPopEDXOnPush = true;
								needPush = "mov edx, ";
							}
						}
						if(cmdNext == cmdCTI)
						{
							indexInEaxOnCti = true;
							needPush = "mov eax, ";
						}
						logASM << needPush << "dword [" << needEDX << needEBP << paramBase+numEDX << "] ; �������� int\r\n";
					}
					if(dt == DTYPE_SHORT)
					{
						logASM << "movsx eax, word [" << needEDX << needEBP << paramBase+numEDX << "] ; �������� short\r\n";
						logASM << "push eax \r\n";
					}
					if(dt == DTYPE_CHAR)
					{
						logASM << "movsx eax, byte [" << needEDX << needEBP << paramBase+numEDX << "] ; �������� char\r\n";
						logASM << "push eax \r\n";
					}
				}
			}
			break;
		case cmdMov:
			{
				logASM << "  ; MOV\r\n";
				int valind = -1, size;

				cFlag = *(unsigned short*)(&exCode[pos]);
				pos += 2;
				st = flagStackType(cFlag);
				dt = flagDataType(cFlag);

				unsigned int numEDX = 0;
				bool knownEDX = false;

				// ���� ������� ����� � �����
				if(flagShiftStk(cFlag))
				{
					valind = *(int*)(&exCode[pos]);
					pos += 4;
					if(knownEDXOnPush)
					{
						if(mulByVarSize)
							numEDX = edxValueForPush * lastVarSize + valind;
						else
							numEDX = edxValueForPush + valind;
						knownEDX = true;
						knownEDXOnPush = false;
					}else{
						if(skipPopEDXOnPush)
						{
							if(mulByVarSize)
							{
								if(valind != 0)
									logASM << "lea edx, [edx*" << lastVarSize << " + " << valind << "]\r\n";
								else
									logASM << "lea edx, [edx*" << lastVarSize << "]\r\n";
							}else{
								numEDX = valind;
							}
							skipPopEDXOnPush = false;
						}else{
							if(mulByVarSize)
							{
								logASM << "pop eax ; ����� �����\r\n";
								if(valind != 0)
									logASM << "lea edx, [eax*" << lastVarSize << " + " << valind << "] ; ����� ��������� �� ���� ���������� � ������� �� ����� � ����� � �� ������������ ������\r\n";
								else
									logASM << "lea edx, [eax*" << lastVarSize << "] ; ����� ��������� �� ���� ���������� � ������� �� ����� � ����� (opt: addr==0)\r\n";
							}else{
								if(valind != 0)
								{
									logASM << "pop edx ; ����� �����\r\n";
									numEDX = valind;
								}else{
									logASM << "pop edx ; ����� ��������� �� ���� ���������� � ������� �� ����� � ����� (opt: addr==0)\r\n";
								}
							}
						}
					}
				}else{
					valind = *(int*)(&exCode[pos]);
					pos += 4;

					knownEDX = true;
					numEDX = valind;
				}

				if(flagSizeOn(cFlag))
				{
					size = *(int*)(&exCode[pos]);
					pos += 4;
					logASM << "cmp eax, " << (mulByVarSize ? size/lastVarSize : size) << " ; ������� ����� � ������������\r\n";
					logASM << "jb movLabel" << movLabels << " ; ���� ����� ������ ��������� (� �� �������������) �� �� ��\r\n";
					logASM << "int 3 \r\n";
					logASM << "  movLabel" << movLabels << ":\r\n";
					movLabels++;
				}
				if(flagSizeStk(cFlag))
				{
					logASM << "cmp [esp], eax ; ������� � ������������ ������� � �����\r\n";
					logASM << "ja movLabel" << movLabels << " ; ���� ����� ������ ��������� (� �� �������������) �� �� ��\r\n";
					logASM << "int 3 \r\n";
					logASM << "  movLabel" << movLabels << ":\r\n";
					logASM << "pop eax ; ������ �������������� ������\r\n";
					movLabels++;
				}
				mulByVarSize = false;

				char *texts[] = { "", "edx + ", "ebp + ", "edi + " };
				char *dontNeed = texts[0];
				char *needEDX = texts[1];
				char *needEBP = texts[2];
				char *useEDI = texts[3];
				if(knownEDX)
					needEDX = dontNeed;
				if(flagAddrAbs(cFlag) && !addEBPtoEDXOnPush)
					needEBP = dontNeed;
				addEBPtoEDXOnPush = false;
				if(flagAddrRelTop(cFlag))
					needEBP = useEDI;

				unsigned int final = paramBase+numEDX;

				unsigned int sizeOfVar = 0;
				if(dt == DTYPE_COMPLEX_TYPE)
				{
					sizeOfVar = *(unsigned int*)(&exCode[pos]);
					pos += 4;
				}

				//look at the next command
				cmdNext = *(CmdID*)(&exCode[pos]);
				if(cmdNext == cmdPop)
					skipPop = true;

				if(dt == DTYPE_COMPLEX_TYPE)
				{
					if(skipPop)
					{
						unsigned int currShift = 0;
						while(sizeOfVar >= 4)
						{
							logASM << "pop dword [" << needEDX << needEBP << final+currShift << "] ; ��������� ����� complex\r\n";
							sizeOfVar -= 4;
							currShift += 4;
						}
						assert(sizeOfVar == 0);
					}else{
						unsigned int currShift = sizeOfVar;
						while(sizeOfVar >= 4)
						{
							currShift -= 4;
							logASM << "mov ebx, [esp+" << sizeOfVar-4 << "] \r\n";
							logASM << "mov dword [" << needEDX << needEBP << final+currShift << "], ebx ; ��������� ����� complex\r\n";
							sizeOfVar -= 4;
						}
						assert(sizeOfVar == 0);
					}
				}
				if(dt == DTYPE_DOUBLE)
				{
					if(skipFldOnMov)
					{
						if(skipPop)
						{
							logASM << "fstp qword [" << needEDX << needEBP << final << "] ; ��������� double ����������\r\n";
							skipFldOnMov = false;
						}else{
							if(cmdNext == cmdMov)
							{
								logASM << "fst qword [" << needEDX << needEBP << final << "] ; ��������� double ����������\r\n";
							}else{
								logASM << "fst qword [" << needEDX << needEBP << final << "] ; ��������� double ����������\r\n";
								logASM << "sub esp, 8 ; ��������� ����� ��� double\r\n";
								logASM << "fstp qword [esp]\r\n";
								skipFldOnMov = false;
							}
						}
					}else{
						if(skipPop)
						{
							logASM << "pop dword [" << needEDX << needEBP << final << "] \r\n";
							logASM << "pop dword [" << needEDX << needEBP << final+4 << "] ; ��������� double ����������.\r\n";
						}else{
							logASM << "fld qword [esp] ; �������� double �� ����� � fpu ����\r\n";
							logASM << "fstp qword [" << needEDX << needEBP << final << "] ; ��������� double ����������\r\n";
						}
					}
				}
				if(dt == DTYPE_LONG)
				{
					if(skipPop)
					{
						logASM << "pop dword [" << needEDX << needEBP << final << "] \r\n";
						logASM << "pop dword [" << needEDX << needEBP << final+4 << "] ; ��������� long long ����������.\r\n";
					}else{
						logASM << "mov ebx, [esp] \r\n";
						logASM << "mov ecx, [esp+4] \r\n";
						logASM << "mov [" << needEDX << needEBP << final << "], ebx \r\n";
						logASM << "mov [" << needEDX << needEBP << final+4 << "], ecx ; ��������� long long ����������.\r\n";
					}
				}
				if(dt == DTYPE_FLOAT)
				{
					if(skipFldOnMov)
					{
						if(skipPop)
						{
							logASM << "fstp dword [" << needEDX << needEBP << final << "] ; ��������� float ����������\r\n";
							skipFldOnMov = false;
						}else{
							if(cmdNext == cmdMov)
							{
								logASM << "fst dword [" << needEDX << needEBP << final << "] ; ��������� float ����������\r\n";
							}else{
								logASM << "fst dword [" << needEDX << needEBP << final << "] ; ��������� float ����������\r\n";
								logASM << "sub esp, 8 ; ��������� ����� ��� double\r\n";
								logASM << "fstp qword [esp]\r\n";
								skipFldOnMov = false;
							}
						}
					}else{
						logASM << "fld qword [esp] ; �������� double �� ����� � fpu ����\r\n";
						logASM << "fstp dword [" << needEDX << needEBP << final << "] ; ��������� float ����������\r\n";
						if(skipPop)
							logASM << "add esp, 8 ;\r\n";
					}
				}
				if(dt == DTYPE_INT)
				{
					if(skipPop)
					{
						logASM << "pop dword [" << needEDX << needEBP << final << "] ; ��������� int ����������\r\n";
					}else{
						logASM << "mov ebx, [esp] \r\n";
						logASM << "mov [" << needEDX << needEBP << final << "], ebx ; ��������� int ����������\r\n";
					}
				}
				if(dt == DTYPE_SHORT)
				{
					if(skipPop)
					{
						logASM << "pop ebx \r\n";
						logASM << "mov word [" << needEDX << needEBP << final << "], bx ; ��������� short ����������\r\n";
					}else{
						logASM << "mov ebx, [esp] \r\n";
						logASM << "mov word [" << needEDX << needEBP << final << "], bx ; ��������� short ����������\r\n";
					}
				}
				if(dt == DTYPE_CHAR)
				{
					if(skipPop)
					{
						logASM << "pop ebx \r\n";
						logASM << "mov byte [" << needEDX << needEBP << final << "], bl ; ��������� char ����������\r\n";
					}else{
						logASM << "mov ebx, [esp] \r\n";
						logASM << "mov byte [" << needEDX << needEBP << final << "], bl ; ��������� char ����������\r\n";
					}
				}
			}
			break;
		case cmdPop:
			logASM << "  ; POP\r\n";

			if(skipPop)
			{
				pos += 4;
				skipPop = false;
				break;
			}

			valind = *(unsigned int*)(&exCode[pos]);
			pos += 4;
			if(valind == 4)
				logASM << "pop eax ; ������ int\r\n";
			else
				logASM << "add esp, " << valind << " ; ������ complex\r\n";
			break;
		case cmdRTOI:
			{
				logASM << "  ; RTOI\r\n";
				cFlag = *(unsigned short*)(&exCode[pos]);
				pos += 2;

				asmStackType st = flagStackType(cFlag);
				asmDataType dt = flagDataType(cFlag);

				if(st == STYPE_DOUBLE && dt == DTYPE_INT)
				{
					logASM << "fld qword [esp] \r\n";
					logASM << "fistp dword [esp+4] \r\n";
					logASM << "add esp, 4 \r\n";
				}else if(st == STYPE_DOUBLE && dt == DTYPE_LONG){
					logASM << "fld qword [esp] \r\n";
					logASM << "fistp qword [esp] \r\n";
				}
			}
			break;
		case cmdITOR:
			{
				logASM << "  ; ITOR\r\n";
				cFlag = *(unsigned short*)(&exCode[pos]);
				pos += 2;
				asmStackType st = flagStackType(cFlag);
				asmDataType dt = flagDataType(cFlag);

				if(st == STYPE_INT && dt == DTYPE_DOUBLE)
				{
					logASM << "fild dword [esp] ; �������� � double\r\n";
					logASM << "push eax ; ��������� ����� ��� double\r\n";
					logASM << "fstp qword [esp] ; ��������� double � ����\r\n";
				}
				if(st == STYPE_LONG && dt == DTYPE_DOUBLE)
				{
					logASM << "fild qword [esp] ; �������� � double\r\n";
					logASM << "fstp qword [esp] ; ��������� double � ����\r\n";
				}
			}
			break;
		case cmdITOL:
			logASM << "  ; ITOL\r\n";
			logASM << "pop eax ; ����� int\r\n";
			logASM << "cdq ; ��������� �� long � edx\r\n";
			logASM << "push edx ; �������� ������� � ����\r\n";
			logASM << "push eax ; ������� ������� � ����\r\n";
			break;
		case cmdLTOI:
			logASM << "  ; LTOI\r\n";
			logASM << "pop eax ; ����� ������� ����\r\n";
			logASM << "xchg eax, [esp] ; �������� ������� ��������\r\n";
			break;
		case cmdSwap:
			logASM << "  ; SWAP\r\n";
			cFlag = *(unsigned short*)(&exCode[pos]);
			pos += 2;
			switch(cFlag)
			{
			case (STYPE_DOUBLE)+(DTYPE_DOUBLE):
			case (STYPE_LONG)+(DTYPE_LONG):
				logASM << "pop eax \r\n";
				logASM << "xchg eax, [esp+4]\r\n";
				logASM << "pop edx \r\n";
				logASM << "xchg edx, [esp+4]\r\n";
				logASM << "push edx\r\n";
				logASM << "push eax ; �������� ������� ��� long ��� double\r\n";
				break;
			case (STYPE_DOUBLE)+(DTYPE_INT):
			case (STYPE_LONG)+(DTYPE_INT):
				logASM << "pop eax \r\n";
				logASM << "xchg eax, [esp+4h]\r\n";
				logASM << "xchg eax, [esp]\r\n";
				logASM << "push eax ; �������� ������� (long ��� double) � int\r\n";
				break;
			case (STYPE_INT)+(DTYPE_DOUBLE):
			case (STYPE_INT)+(DTYPE_LONG):
				logASM << "pop eax \r\n";
				logASM << "xchg eax, [esp]\r\n";
				logASM << "xchg eax, [esp+4h]\r\n";
				logASM << "push eax ; �������� ������� int � (long ��� double)\r\n";
				break;
			case (STYPE_INT)+(DTYPE_INT):
				logASM << "pop eax \r\n";
				logASM << "xchg eax, [esp]\r\n";
				logASM << "push eax ; �������� ������� ��� int\r\n";
				break;
			default:
				throw std::string("cmdSwap, unimplemented type combo");
			}
			break;
		case cmdCopy:
			logASM << "  ; COPY\r\n";
			oFlag = *(unsigned char*)(&exCode[pos]);
			pos += 1;
			switch(oFlag)
			{
			case OTYPE_DOUBLE:
			case OTYPE_LONG:
				logASM << "mov edx, [esp]\r\n";
				logASM << "mov eax, [esp+4]\r\n";
				logASM << "push eax\r\n";
				logASM << "push edx ; ����������� long ��� double\r\n";
				break;
			case OTYPE_INT:
				logASM << "mov eax, [esp]\r\n";
				logASM << "push eax ; ����������� int\r\n";
				break;
			}
			break;
		case cmdJmp:
			logASM << "  ; JMP\r\n";
			valind = *(unsigned int*)(&exCode[pos]);
			pos += 4;
			{
				bool jFar = false;
				for(unsigned int i = 0; i < funcNeedLabel.size(); i++)
					if(funcNeedLabel[i] == pos)
						jFar = true;
				logASM << "jmp near gLabel" << valind << "\r\n";
			}
			break;
		case cmdJmpZ:
			logASM << "  ; JMPZ\r\n";
			oFlag = *(unsigned char*)(&exCode[pos]);
			pos += 1;
			valind = *(unsigned int*)(&exCode[pos]);
			pos += 4;

			if(oFlag == OTYPE_DOUBLE)
			{
				logASM << "fldz ; ������� ���� � fpu ����\r\n";
				logASM << "fcomp qword [esp] ; �������\r\n"; 
				logASM << "fnstsw ax ; ��������� ��� �������� �� fpu ���������� ������� � ax\r\n";
				logASM << "pop ebx \r\n";
				logASM << "pop ebx ; ������ double �� �����\r\n";
				logASM << "test ah, 44h ; MSVS � ���-�� ����������\r\n";
				logASM << "jnp near gLabel" << valind << "\r\n";
			}else if(oFlag == OTYPE_LONG){
				logASM << "pop edx \r\n";
				logASM << "pop eax \r\n";
				logASM << "or edx, eax ; ���������� long == 0\r\n";
				logASM << "jne near gLabel" << valind << "\r\n";
			}else if(oFlag == OTYPE_INT){
				logASM << "pop eax \r\n";
				logASM << "test eax, eax ; ���������� int == 0\r\n";
				logASM << "jz near gLabel" << valind << "\r\n";
			}
			break;
		case cmdJmpNZ:
			logASM << "  ; JMPNZ\r\n";
			oFlag = *(unsigned char*)(&exCode[pos]);
			pos += 1;
			valind = *(unsigned int*)(&exCode[pos]);
			pos += 4;
			if(oFlag == OTYPE_DOUBLE)
			{
				logASM << "fldz ; ������� ���� � fpu ����\r\n";
				logASM << "fcomp qword [esp] ; �������\r\n"; 
				logASM << "fnstsw ax ; ��������� ��� �������� �� fpu ���������� ������� � ax\r\n";
				logASM << "pop ebx \r\n";
				logASM << "pop ebx ; ������ double �� �����\r\n";
				logASM << "test ah, 44h ; MSVS � ���-�� ����������\r\n";
				logASM << "jp near gLabel" << valind << "\r\n";
			}else if(oFlag == OTYPE_LONG){
				logASM << "pop edx \r\n";
				logASM << "pop eax \r\n";
				logASM << "or edx, eax ; ���������� long == 0\r\n";
				logASM << "je near gLabel" << valind << "\r\n";
			}else if(oFlag == OTYPE_INT){
				logASM << "pop eax \r\n";
				logASM << "test eax, eax \r\n";
				logASM << "jnz near gLabel" << valind << "\r\n";
			}
			break;
		case cmdSetRange:
			logASM << "  ; SETRANGE\r\n";
			cFlag = *(unsigned short*)(&exCode[pos]);
			pos += 2;
			valind = *(unsigned int*)(&exCode[pos]);
			pos += 4;
			valind2 = *(unsigned int*)(&exCode[pos]);
			pos += 4;
			logASM << "lea ebx, [ebp + " << paramBase+valind << "] ; ��������� �����\r\n";
			logASM << "lea ecx, [ebp + " << paramBase+valind+(valind2-1)*typeSizeD[(cFlag>>2)&0x00000007] << "] ; �������� �����\r\n";
			if(cFlag == DTYPE_FLOAT)
			{
				logASM << "fld qword [esp] ; float � ����\r\n";
			}else{
				logASM << "mov eax, [esp] \r\n";
				logASM << "mov edx, [esp+4] ; ���������� � ��������\r\n";
			}
			logASM << " loopStart" << aluLabels << ": \r\n";
			logASM << "cmp ebx, ecx \r\n";
			logASM << "jg loopEnd" << aluLabels << " \r\n";
			switch(cFlag)
			{
			case DTYPE_DOUBLE:
				logASM << "mov dword [ebx+4], edx \r\n";
				logASM << "mov dword [ebx], eax \r\n";
				break;
			case DTYPE_FLOAT:
				// ����� ��������������� float � ����
				logASM << "fst dword [ebx] \r\n";
				break;
			case DTYPE_LONG:
				logASM << "mov dword [ebx+4], edx \r\n";
				logASM << "mov dword [ebx], eax \r\n";
				break;
			case DTYPE_INT:
				logASM << "mov dword [ebx], eax \r\n";
				break;
			case DTYPE_SHORT:
				logASM << "mov word [ebx], ax \r\n";
				break;
			case DTYPE_CHAR:
				logASM << "mov byte [ebx], al \r\n";
				break;
			}
			logASM << "add ebx, " << typeSizeD[(cFlag>>2)&0x00000007] << " ; ������� ��������� �� ��������� �������\r\n";
			logASM << "jmp loopStart" << aluLabels << " \r\n";
			logASM << "  loopEnd" << aluLabels << ": \r\n";
			if(cFlag == DTYPE_FLOAT)
				logASM << "fstp st0 ; float �� �����\r\n";
			aluLabels++;
			break;
		case cmdGetAddr:
			logASM << "  ; GETADDR\r\n";
			valind = *(unsigned int*)(&exCode[pos]);
			pos += 4;
			cmdNext = *(CmdID*)(&exCode[pos]);
			if(cmdNext == cmdPush)
			{
				CmdFlag lcFlag;
				lcFlag = *(unsigned short*)(&exCode[pos+3]);
				if((flagAddrAbs(lcFlag) || flagAddrRel(lcFlag)) && flagShiftStk(lcFlag))
					knownEDXOnPush = true;
			}
			if(!knownEDXOnPush)
			{
				if(valind)
				{
					logASM << "lea eax, [ebp + " << (int)valind << "] ; �������� ����� ������������ ���� �����\r\n";
					logASM << "push eax ; �������� ����� � ����\r\n";
				}else{
					logASM << "push ebp ; �������� ����� � ���� (valind == 0)\r\n";
				}
			}else{
				addEBPtoEDXOnPush = true;
				edxValueForPush = (int)valind;
			}
			break;
		case cmdFuncAddr:
		{
			valind = *(unsigned int*)(&exCode[pos]);
			pos += sizeof(unsigned int);

			if(exFunctions[valind]->funcPtr == NULL)
			{
				logASM << "lea eax, [function" << exFunctions[valind]->address << " + " << binCodeStart << "] ; ����� ������� \r\n";
				logASM << "push eax ; \r\n";
			}else{
				logASM << "push 0x" << exFunctions[valind]->funcPtr << " \r\n";
			}
			break;
		}
		}
		if(cmd >= cmdAdd && cmd <= cmdLogXor)
		{
			oFlag = *(unsigned char*)(&exCode[pos]);
			pos += 1;

			//look at the next command
			cmdNext = *(CmdID*)(&exCode[pos]);

			bool skipFstpOnDoubleALU = false;
			if(cmdNext >= cmdAdd && cmdNext <= cmdNEqual)
				skipFstpOnDoubleALU = true;

			switch(cmd + (oFlag << 16))
			{
			case cmdAdd+(OTYPE_DOUBLE<<16):
				logASM << "  ; ADD  double\r\n";
			//	logASM << "fld qword [esp] \r\n";
				if(!skipFldESPOnDoubleALU)
					logASM << "fld qword [esp+8] \r\n";
				logASM << "fadd qword [esp] \r\n";
			//	logASM << "faddp \r\n";
				if(!skipFstpOnDoubleALU)
				{
					logASM << "fstp qword [esp" << (skipFldESPOnDoubleALU ? "" : "+8") << "] \r\n";
					if(!skipFldESPOnDoubleALU)
						logASM << "add esp, 8\r\n";
				}else{
					logASM << "add esp, " << (skipFldESPOnDoubleALU ? 8 : 16) << "\r\n";
					skipFldESPOnDoubleALU = true;
				}
				break;
			case cmdAdd+(OTYPE_LONG<<16):
				logASM << "  ; ADD long\r\n";
				logASM << "pop eax \r\n";
				logASM << "pop edx \r\n";
				logASM << "add [esp], eax \r\n";
				logASM << "adc [esp+4], edx \r\n";
				break;
			case cmdAdd+(OTYPE_INT<<16):
				logASM << "  ; ADD int\r\n";
				if(!skipPopEAXOnIntALU)
					logASM << "pop eax \r\n";
				logASM << "add [esp], eax \r\n";
				break;

			case cmdSub+(OTYPE_DOUBLE<<16):
				logASM << "  ; SUB  double\r\n";
			//	logASM << "fld qword [esp] \r\n";
				if(!skipFldESPOnDoubleALU)
					logASM << "fld qword [esp+8] \r\n";
				if(skipFldESPOnDoubleALU)
					logASM << "fsubr qword [esp] \r\n";
				else
					logASM << "fsub qword [esp] \r\n";
			//	logASM << "fsubrp \r\n";
				if(!skipFstpOnDoubleALU)
				{
					logASM << "fstp qword [esp" << (skipFldESPOnDoubleALU ? "" : "+8") << "] \r\n";
					if(!skipFldESPOnDoubleALU)
						logASM << "add esp, 8\r\n";
				}else{
					logASM << "add esp, " << (skipFldESPOnDoubleALU ? 8 : 16) << "\r\n";
					skipFldESPOnDoubleALU = true;
				}
				break;
			case cmdSub+(OTYPE_LONG<<16):
				logASM << "  ; SUB long\r\n";
				logASM << "pop eax \r\n";
				logASM << "pop edx \r\n";
				logASM << "sub [esp], eax \r\n";
				logASM << "sbb [esp+4], edx \r\n";
				break;
			case cmdSub+(OTYPE_INT<<16):
				logASM << "  ; SUB int\r\n";
				if(!skipPopEAXOnIntALU)
					logASM << "pop eax \r\n";
				logASM << "sub [esp], eax \r\n";
				break;

			case cmdMul+(OTYPE_DOUBLE<<16):
				logASM << "  ; MUL  double\r\n";
			//	logASM << "fld qword [esp] \r\n";
				if(!skipFldESPOnDoubleALU)
					logASM << "fld qword [esp+8] \r\n";
				logASM << "fmul qword [esp] \r\n";
			//	logASM << "fmulp \r\n";
				if(!skipFstpOnDoubleALU)
				{
					logASM << "fstp qword [esp" << (skipFldESPOnDoubleALU ? "" : "+8") << "] \r\n";
					if(!skipFldESPOnDoubleALU)
						logASM << "add esp, 8\r\n";
				}else{
					logASM << "add esp, " << (skipFldESPOnDoubleALU ? 8 : 16) << "\r\n";
					skipFldESPOnDoubleALU = true;
				}
				break;
			case cmdMul+(OTYPE_LONG<<16):
				logASM << "  ; MUL long\r\n";
				logASM << "mov ecx, 0x" << longMul << " ; longMul(), result in edx:eax\r\n";
				logASM << "call ecx \r\n";
				logASM << "add esp, 8 ; ������ ����\r\n";
				logASM << "mov [esp+4], edx \r\n";
				logASM << "mov [esp], eax \r\n";
				break;
			case cmdMul+(OTYPE_INT<<16):
				logASM << "  ; MUL int\r\n";
				if(!skipPopEAXOnIntALU)
					logASM << "pop eax \r\n";
				logASM << "pop edx \r\n";
				logASM << "imul edx \r\n";
				logASM << "push eax \r\n";
				break;

			case cmdDiv+(OTYPE_DOUBLE<<16):
				logASM << "  ; DIV  double\r\n";
			//	logASM << "fld qword [esp] \r\n";
				if(!skipFldESPOnDoubleALU)
					logASM << "fld qword [esp+8] \r\n";
				if(skipFldESPOnDoubleALU)
					logASM << "fdivr qword [esp] \r\n";
				else
					logASM << "fdiv qword [esp] \r\n";
			//	logASM << "fdivrp \r\n";
				if(!skipFstpOnDoubleALU)
				{
					logASM << "fstp qword [esp" << (skipFldESPOnDoubleALU ? "" : "+8") << "] \r\n";
					if(!skipFldESPOnDoubleALU)
						logASM << "add esp, 8\r\n";
				}else{
					logASM << "add esp, " << (skipFldESPOnDoubleALU ? 8 : 16) << "\r\n";
					skipFldESPOnDoubleALU = true;
				}
				break;
			case cmdDiv+(OTYPE_LONG<<16):
				logASM << "  ; DIV long\r\n";
				logASM << "mov ecx, 0x" << longDiv << " ; longDiv(), result in edx:eax\r\n";
				logASM << "call ecx \r\n";
				logASM << "add esp, 8 ; ������ ����\r\n";
				logASM << "mov [esp+4], edx \r\n";
				logASM << "mov [esp], eax \r\n";
				break;
			case cmdDiv+(OTYPE_INT<<16):
				logASM << "  ; DIV int\r\n";
				if(!skipPopEAXOnIntALU)
					logASM << "pop eax \r\n";
				logASM << "xchg eax, [esp] \r\n";
				logASM << "cdq \r\n";
				logASM << "idiv dword [esp] ; � �������� �� 0?\r\n";
				logASM << "xchg eax, [esp]\r\n";
				break;

			case cmdPow+(OTYPE_DOUBLE<<16):
				logASM << "  ; POW double\r\n";
				logASM << "fld qword [esp] \r\n";
				if(!skipFldESPOnDoubleALU)
					logASM << "fld qword [esp+8] \r\n";
				logASM << "mov ecx, 0x" << doublePow << " ; doublePow(), result in st0\r\n";
				logASM << "call ecx \r\n";
				if(!skipFstpOnDoubleALU)
				{
					logASM << "fstp qword [esp" << (skipFldESPOnDoubleALU ? "" : "+8") << "] \r\n";
					if(!skipFldESPOnDoubleALU)
						logASM << "add esp, 8\r\n";
				}else{
					logASM << "add esp, " << (skipFldESPOnDoubleALU ? 8 : 16) << "\r\n";
					skipFldESPOnDoubleALU = true;
				}
				break;
			case cmdPow+(OTYPE_LONG<<16):
				logASM << "  ; MOD long\r\n";
				logASM << "mov ecx, 0x" << longPow << " ; longPow(), result in edx:eax\r\n";
				logASM << "call ecx \r\n";
				logASM << "add esp, 8 ; ������ ����\r\n";
				logASM << "mov [esp+4], edx \r\n";
				logASM << "mov [esp], eax \r\n";
				break;
			case cmdPow+(OTYPE_INT<<16):
				logASM << "  ; POW int\r\n";
				if(!skipPopEAXOnIntALU)
					logASM << "pop eax \r\n";
				logASM << "pop ebx \r\n";
				logASM << "mov ecx, 0x" << intPow << " ; intPow(), result in edx\r\n";
				logASM << "call ecx \r\n";
				logASM << "push edx \r\n";
				break;

			case cmdMod+(OTYPE_DOUBLE<<16):
				logASM << "  ; MOD  double\r\n";
				logASM << "fld qword [esp] \r\n";
				if(!skipFldESPOnDoubleALU)
					logASM << "fld qword [esp+8] \r\n";
				logASM << "fprem \r\n";
				logASM << "fstp st1 \r\n";
				if(!skipFstpOnDoubleALU)
				{
					logASM << "fstp qword [esp" << (skipFldESPOnDoubleALU ? "" : "+8") << "] \r\n";
					if(!skipFldESPOnDoubleALU)
						logASM << "add esp, 8\r\n";
				}else{
					logASM << "add esp, " << (skipFldESPOnDoubleALU ? 8 : 16) << "\r\n";
					skipFldESPOnDoubleALU = true;
				}
				break;
			case cmdMod+(OTYPE_LONG<<16):
				logASM << "  ; MOD long\r\n";
				logASM << "mov ecx, 0x" << longMod << " ; longMod(), result in edx:eax\r\n";
				logASM << "call ecx \r\n";
				logASM << "add esp, 8 ; ������ ����\r\n";
				logASM << "mov [esp+4], edx \r\n";
				logASM << "mov [esp], eax \r\n";
				break;
			case cmdMod+(OTYPE_INT<<16):
				logASM << "  ; MOD int\r\n";
				if(!skipPopEAXOnIntALU)
					logASM << "pop eax \r\n";
				logASM << "xchg eax, [esp] \r\n";
				logASM << "cdq \r\n";
				logASM << "idiv dword [esp] ; � �������� �� 0?\r\n";
				logASM << "xchg edx, [esp]\r\n";
				break;

			case cmdLess+(OTYPE_DOUBLE<<16):
				logASM << "  ; LES double\r\n";
				if(!skipFldESPOnDoubleALU)
				{
					logASM << "fld qword [esp] \r\n";
					logASM << "fcomp qword [esp+8] \r\n";
				}else{
					logASM << "fcomp qword [esp] \r\n";
				}
				logASM << "fnstsw ax ; ����� ������\r\n";
				logASM << "test ah, 41h ; �������� � '������'\r\n";
				logASM << "jne pushZero" << aluLabels << " ; ��, �� ������\r\n";
				logASM << "mov dword [esp], 1 \r\n";
				logASM << "jmp pushedOne" << aluLabels << " \r\n";
				logASM << "  pushZero" << aluLabels << ": \r\n";
				logASM << "mov dword [esp], 0 \r\n";
				logASM << "  pushedOne" << aluLabels << ": \r\n";
				logASM << "fild dword [esp] \r\n";
				aluLabels++;
				if(!skipFstpOnDoubleALU)
				{
					logASM << "fstp qword [esp" << (skipFldESPOnDoubleALU ? "" : "+8") << "] \r\n";
					if(!skipFldESPOnDoubleALU)
						logASM << "add esp, 8\r\n";
				}else{
					logASM << "add esp, " << (skipFldESPOnDoubleALU ? 8 : 16) << "\r\n";
					skipFldESPOnDoubleALU = true;
				}
				break;
			case cmdLess+(OTYPE_LONG<<16):
				logASM << "  ; LES long\r\n";
				logASM << "pop eax \r\n";
				logASM << "pop edx ; edx:eax\r\n";
				logASM << "cmp dword [esp+4], edx \r\n";
				logASM << "jg SetZero" << aluLabels << " \r\n";
				logASM << "jl SetOne" << aluLabels << " \r\n";
				logASM << "cmp dword [esp], eax \r\n";
				logASM << "jae SetZero" << aluLabels << " \r\n";
				logASM << "  SetOne" << aluLabels << ": \r\n";
				logASM << "mov dword [esp], 1 \r\n";
				logASM << "jmp OneSet" << aluLabels << " \r\n";
				logASM << "  SetZero" << aluLabels << ": \r\n";
				logASM << "mov dword [esp], 0 \r\n";
				logASM << "  OneSet" << aluLabels << ": \r\n";
				logASM << "mov dword [esp+4], 0 \r\n";
				aluLabels++;
				break;
			case cmdLess+(OTYPE_INT<<16):
				logASM << "  ; LES int\r\n";
				if(!skipPopEAXOnIntALU)
					logASM << "pop eax \r\n";
				logASM << "xor ecx, ecx\r\n";
				logASM << "cmp [esp], eax ; \r\n";
				logASM << "setl cl \r\n";
				logASM << "mov [esp], ecx\r\n";
				break;

			case cmdGreater+(OTYPE_DOUBLE<<16):
				logASM << "  ; GRT double\r\n";
				if(!skipFldESPOnDoubleALU)
				{
					logASM << "fld qword [esp] \r\n";
					logASM << "fcomp qword [esp+8] \r\n";
				}else{
					logASM << "fcomp qword [esp] \r\n";
				}
				logASM << "fnstsw ax ; ����� ������\r\n";
				logASM << "test ah, 5h ; �������� � '������'\r\n";
				logASM << "jp pushZero" << aluLabels << " ; ��, �� ������\r\n";
				logASM << "mov dword [esp], 1 \r\n";
				logASM << "jmp pushedOne" << aluLabels << " \r\n";
				logASM << "  pushZero" << aluLabels << ": \r\n";
				logASM << "mov dword [esp], 0 \r\n";
				logASM << "  pushedOne" << aluLabels << ": \r\n";
				logASM << "fild dword [esp] \r\n";
				aluLabels++;
				if(!skipFstpOnDoubleALU)
				{
					logASM << "fstp qword [esp" << (skipFldESPOnDoubleALU ? "" : "+8") << "] \r\n";
					if(!skipFldESPOnDoubleALU)
						logASM << "add esp, 8\r\n";
				}else{
					logASM << "add esp, " << (skipFldESPOnDoubleALU ? 8 : 16) << "\r\n";
					skipFldESPOnDoubleALU = true;
				}
				break;
			case cmdGreater+(OTYPE_LONG<<16):
				logASM << "  ; GRT long\r\n";
				logASM << "pop eax \r\n";
				logASM << "pop edx ; edx:eax\r\n";
				logASM << "cmp dword [esp+4], edx \r\n";
				logASM << "jl SetZero" << aluLabels << " \r\n";
				logASM << "jg SetOne" << aluLabels << " \r\n";
				logASM << "cmp dword [esp], eax \r\n";
				logASM << "jbe SetZero" << aluLabels << " \r\n";
				logASM << "  SetOne" << aluLabels << ": \r\n";
				logASM << "mov dword [esp], 1 \r\n";
				logASM << "jmp OneSet" << aluLabels << " \r\n";
				logASM << "  SetZero" << aluLabels << ": \r\n";
				logASM << "mov dword [esp], 0 \r\n";
				logASM << "  OneSet" << aluLabels << ": \r\n";
				logASM << "mov dword [esp+4], 0 \r\n";
				aluLabels++;
				break;
			case cmdGreater+(OTYPE_INT<<16):
				logASM << "  ; GRT int\r\n";
				if(!skipPopEAXOnIntALU)
					logASM << "pop eax \r\n";
				logASM << "xor ecx, ecx\r\n";
				logASM << "cmp [esp], eax ; \r\n";
				logASM << "setg cl \r\n";
				logASM << "mov [esp], ecx\r\n";
				break;

			case cmdLEqual+(OTYPE_DOUBLE<<16):
				logASM << "  ; LEQL double\r\n";
				if(!skipFldESPOnDoubleALU)
				{
					logASM << "fld qword [esp] \r\n";
					logASM << "fcomp qword [esp+8] \r\n";
				}else{
					logASM << "fcomp qword [esp] \r\n";
				}
				logASM << "fnstsw ax ; ����� ������\r\n";
				logASM << "test ah, 1h ; �������� � '������ ��� �����'\r\n";
				logASM << "jne pushZero" << aluLabels << " ; ��, �� ������ ��� �����\r\n";
				logASM << "mov dword [esp], 1 \r\n";
				logASM << "jmp pushedOne" << aluLabels << " \r\n";
				logASM << "  pushZero" << aluLabels << ": \r\n";
				logASM << "mov dword [esp], 0 \r\n";
				logASM << "  pushedOne" << aluLabels << ": \r\n";
				logASM << "fild dword [esp] \r\n";
				aluLabels++;
				if(!skipFstpOnDoubleALU)
				{
					logASM << "fstp qword [esp" << (skipFldESPOnDoubleALU ? "" : "+8") << "] \r\n";
					if(!skipFldESPOnDoubleALU)
						logASM << "add esp, 8\r\n";
				}else{
					logASM << "add esp, " << (skipFldESPOnDoubleALU ? 8 : 16) << "\r\n";
					skipFldESPOnDoubleALU = true;
				}
				break;
			case cmdLEqual+(OTYPE_LONG<<16):
				logASM << "  ; LEQL long\r\n";
				logASM << "pop eax \r\n";
				logASM << "pop edx ; edx:eax\r\n";
				logASM << "cmp dword [esp+4], edx \r\n";
				logASM << "jg SetZero" << aluLabels << " \r\n";
				logASM << "jl SetOne" << aluLabels << " \r\n";
				logASM << "cmp dword [esp], eax \r\n";
				logASM << "ja SetZero" << aluLabels << " \r\n";
				logASM << "  SetOne" << aluLabels << ": \r\n";
				logASM << "mov dword [esp], 1 \r\n";
				logASM << "jmp OneSet" << aluLabels << " \r\n";
				logASM << "  SetZero" << aluLabels << ": \r\n";
				logASM << "mov dword [esp], 0 \r\n";
				logASM << "  OneSet" << aluLabels << ": \r\n";
				logASM << "mov dword [esp+4], 0 \r\n";
				aluLabels++;
				break;
			case cmdLEqual+(OTYPE_INT<<16):
				logASM << "  ; LEQL int\r\n";
				if(!skipPopEAXOnIntALU)
					logASM << "pop eax \r\n";
				logASM << "xor ecx, ecx\r\n";
				logASM << "cmp [esp], eax ; \r\n";
				logASM << "setle cl \r\n";
				logASM << "mov [esp], ecx\r\n";
				break;

			case cmdGEqual+(OTYPE_DOUBLE<<16):
				logASM << "  ; GEQL double\r\n";
				if(!skipFldESPOnDoubleALU)
				{
					logASM << "fld qword [esp] \r\n";
					logASM << "fcomp qword [esp+8] \r\n";
				}else{
					logASM << "fcomp qword [esp] \r\n";
				}
				logASM << "fnstsw ax ; ����� ������\r\n";
				logASM << "test ah, 41h ; �������� � '������ ��� �����'\r\n";
				logASM << "jp pushZero" << aluLabels << " ; ��, �� ������ ��� �����\r\n";
				logASM << "mov dword [esp], 1 \r\n";
				logASM << "jmp pushedOne" << aluLabels << " \r\n";
				logASM << "  pushZero" << aluLabels << ": \r\n";
				logASM << "mov dword [esp], 0 \r\n";
				logASM << "  pushedOne" << aluLabels << ": \r\n";
				logASM << "fild dword [esp] \r\n";
				aluLabels++;
				if(!skipFstpOnDoubleALU)
				{
					logASM << "fstp qword [esp" << (skipFldESPOnDoubleALU ? "" : "+8") << "] \r\n";
					if(!skipFldESPOnDoubleALU)
						logASM << "add esp, 8\r\n";
				}else{
					logASM << "add esp, " << (skipFldESPOnDoubleALU ? 8 : 16) << "\r\n";
					skipFldESPOnDoubleALU = true;
				}
				break;
			case cmdGEqual+(OTYPE_LONG<<16):
				logASM << "  ; GEQL long\r\n";
				logASM << "pop eax \r\n";
				logASM << "pop edx ; edx:eax\r\n";
				logASM << "cmp dword [esp+4], edx \r\n";
				logASM << "jl SetZero" << aluLabels << " \r\n";
				logASM << "jg SetOne" << aluLabels << " \r\n";
				logASM << "cmp dword [esp], eax \r\n";
				logASM << "jb SetZero" << aluLabels << " \r\n";
				logASM << "  SetOne" << aluLabels << ": \r\n";
				logASM << "mov dword [esp], 1 \r\n";
				logASM << "jmp OneSet" << aluLabels << " \r\n";
				logASM << "  SetZero" << aluLabels << ": \r\n";
				logASM << "mov dword [esp], 0 \r\n";
				logASM << "  OneSet" << aluLabels << ": \r\n";
				logASM << "mov dword [esp+4], 0 \r\n";
				aluLabels++;
				break;
			case cmdGEqual+(OTYPE_INT<<16):
				logASM << "  ; GEQL int\r\n";
				if(!skipPopEAXOnIntALU)
					logASM << "pop eax \r\n";
				logASM << "xor ecx, ecx\r\n";
				logASM << "cmp [esp], eax ; \r\n";
				logASM << "setge cl \r\n";
				logASM << "mov [esp], ecx\r\n";
				break;

			case cmdEqual+(OTYPE_DOUBLE<<16):
				logASM << "  ; EQL double\r\n";
				if(!skipFldESPOnDoubleALU)
				{
					logASM << "fld qword [esp] \r\n";
					logASM << "fcomp qword [esp+8] \r\n";
				}else{
					logASM << "fcomp qword [esp] \r\n";
				}
				logASM << "fnstsw ax ; ����� ������\r\n";
				logASM << "test ah, 44h ; �������� � '�����'\r\n";
				logASM << "jp pushZero" << aluLabels << " ; ��, �� �����\r\n";
				logASM << "mov dword [esp], 1 \r\n";
				logASM << "jmp pushedOne" << aluLabels << " \r\n";
				logASM << "  pushZero" << aluLabels << ": \r\n";
				logASM << "mov dword [esp], 0 \r\n";
				logASM << "  pushedOne" << aluLabels << ": \r\n";
				logASM << "fild dword [esp] \r\n";
				aluLabels++;
				if(!skipFstpOnDoubleALU)
				{
					logASM << "fstp qword [esp" << (skipFldESPOnDoubleALU ? "" : "+8") << "] \r\n";
					if(!skipFldESPOnDoubleALU)
						logASM << "add esp, 8\r\n";
				}else{
					logASM << "add esp, " << (skipFldESPOnDoubleALU ? 8 : 16) << "\r\n";
					skipFldESPOnDoubleALU = true;
				}
				break;
			case cmdEqual+(OTYPE_LONG<<16):
				logASM << "  ; EQL long\r\n";
				logASM << "pop eax \r\n";
				logASM << "pop edx ; edx:eax\r\n";
				logASM << "cmp dword [esp+4], edx \r\n";
				logASM << "jne SetZero" << aluLabels << " \r\n";
				logASM << "cmp dword [esp], eax \r\n";
				logASM << "jne SetZero" << aluLabels << " \r\n";
				logASM << "mov dword [esp], 1 \r\n";
				logASM << "jmp OneSet" << aluLabels << " \r\n";
				logASM << "  SetZero" << aluLabels << ": \r\n";
				logASM << "mov dword [esp], 0 \r\n";
				logASM << "  OneSet" << aluLabels << ": \r\n";
				logASM << "mov dword [esp+4], 0 \r\n";
				aluLabels++;
				break;
			case cmdEqual+(OTYPE_INT<<16):
				logASM << "  ; EQL int\r\n";
				if(!skipPopEAXOnIntALU)
					logASM << "pop eax \r\n";
				logASM << "xor ecx, ecx\r\n";
				logASM << "cmp [esp], eax ; \r\n";
				logASM << "sete cl \r\n";
				logASM << "mov [esp], ecx\r\n";
				break;

			case cmdNEqual+(OTYPE_DOUBLE<<16):
				logASM << "  ; NEQL double\r\n";
				if(!skipFldESPOnDoubleALU)
				{
					logASM << "fld qword [esp] \r\n";
					logASM << "fcomp qword [esp+8] \r\n";
				}else{
					logASM << "fcomp qword [esp] \r\n";
				}
				logASM << "fnstsw ax ; ����� ������\r\n";
				logASM << "test ah, 44h ; �������� � '�������'\r\n";
				logASM << "jnp pushZero" << aluLabels << " ; ��, �� �������\r\n";
				logASM << "mov dword [esp], 1 \r\n";
				logASM << "jmp pushedOne" << aluLabels << " \r\n";
				logASM << "  pushZero" << aluLabels << ": \r\n";
				logASM << "mov dword [esp], 0 \r\n";
				logASM << "  pushedOne" << aluLabels << ": \r\n";
				logASM << "fild dword [esp] \r\n";
				aluLabels++;
				if(!skipFstpOnDoubleALU)
				{
					logASM << "fstp qword [esp" << (skipFldESPOnDoubleALU ? "" : "+8") << "] \r\n";
					if(!skipFldESPOnDoubleALU)
						logASM << "add esp, 8\r\n";
				}else{
					logASM << "add esp, " << (skipFldESPOnDoubleALU ? 8 : 16) << "\r\n";
					skipFldESPOnDoubleALU = true;
				}
				break;
			case cmdNEqual+(OTYPE_LONG<<16):
				logASM << "  ; NEQL long\r\n";
				logASM << "pop eax \r\n";
				logASM << "pop edx ; edx:eax\r\n";
				logASM << "cmp dword [esp+4], edx \r\n";
				logASM << "jne SetOne" << aluLabels << " \r\n";
				logASM << "cmp dword [esp], eax \r\n";
				logASM << "je SetZero" << aluLabels << " \r\n";
				logASM << "  SetOne" << aluLabels << ": \r\n";
				logASM << "mov dword [esp], 1 \r\n";
				logASM << "jmp OneSet" << aluLabels << " \r\n";
				logASM << "  SetZero" << aluLabels << ": \r\n";
				logASM << "mov dword [esp], 0 \r\n";
				logASM << "  OneSet" << aluLabels << ": \r\n";
				logASM << "mov dword [esp+4], 0 \r\n";
				aluLabels++;
				break;
			case cmdNEqual+(OTYPE_INT<<16):
				logASM << "  ; NEQL int\r\n";
				if(!skipPopEAXOnIntALU)
					logASM << "pop eax \r\n";
				logASM << "xor ecx, ecx\r\n";
				logASM << "cmp [esp], eax ; \r\n";
				logASM << "setne cl \r\n";
				logASM << "mov [esp], ecx\r\n";
				break;

			case cmdShl+(OTYPE_LONG<<16):
				logASM << "  ; SHL long\r\n";
				logASM << "mov ecx, 0x" << longShl << " ; longShl(), result in [esp+8]\r\n";
				logASM << "call ecx \r\n";
				logASM << "add esp, 8 ; ������ ����\r\n";
				break;
			case cmdShl+(OTYPE_INT<<16):
				logASM << "  ; SHL int\r\n";
				if(!skipPopEAXOnIntALU)
					logASM << "pop eax \r\n";
				logASM << "pop ecx \r\n";
				logASM << "xchg ecx, eax \r\n";
				logASM << "sal eax, cl ; \r\n";
				logASM << "push eax \r\n";
				break;

			case cmdShr+(OTYPE_LONG<<16):
				logASM << "  ; SHR long\r\n";
				logASM << "mov ecx, 0x" << longShr << " ; longShr(), result in [esp+8]\r\n";
				logASM << "call ecx \r\n";
				logASM << "add esp, 8 ; ������ ����\r\n";
				break;
			case cmdShr+(OTYPE_INT<<16):
				logASM << "  ; SHR int\r\n";
				if(!skipPopEAXOnIntALU)
					logASM << "pop eax \r\n";
				logASM << "pop ecx \r\n";
				logASM << "xchg ecx, eax \r\n";
				logASM << "sar eax, cl ; \r\n";
				logASM << "push eax \r\n";
				break;

			case cmdBitAnd+(OTYPE_LONG<<16):
				logASM << "  ; BAND long\r\n";
				logASM << "pop eax \r\n";
				logASM << "pop edx \r\n";
				logASM << "and [esp], eax ; \r\n";
				logASM << "and [esp+4], edx ; \r\n";
				break;
			case cmdBitAnd+(OTYPE_INT<<16):
				logASM << "  ; BAND int\r\n";
				if(!skipPopEAXOnIntALU)
					logASM << "pop eax \r\n";
				logASM << "and [esp], eax ; \r\n";
				break;

			case cmdBitOr+(OTYPE_LONG<<16):
				logASM << "  ; BOR long\r\n";
				logASM << "pop eax \r\n";
				logASM << "pop edx \r\n";
				logASM << "or [esp], eax ; \r\n";
				logASM << "or [esp+4], edx ; \r\n";
				break;
			case cmdBitOr+(OTYPE_INT<<16):
				logASM << "  ; BOR int\r\n";
				if(!skipPopEAXOnIntALU)
					logASM << "pop eax \r\n";
				logASM << "or [esp], eax ; \r\n";
				break;

			case cmdBitXor+(OTYPE_LONG<<16):
				logASM << "  ; BXOR long\r\n";
				logASM << "pop eax \r\n";
				logASM << "pop edx \r\n";
				logASM << "xor [esp], eax ; \r\n";
				logASM << "xor [esp+4], edx ; \r\n";
				break;
			case cmdBitXor+(OTYPE_INT<<16):
				logASM << "  ; BXOR int\r\n";
				if(!skipPopEAXOnIntALU)
					logASM << "pop eax \r\n";
				logASM << "xor [esp], eax ; \r\n";
				break;

			case cmdLogAnd+(OTYPE_LONG<<16):
				logASM << "  ; LAND long\r\n";
				logASM << "mov eax, dword [esp] \r\n";
				logASM << "or eax, dword [esp+4] \r\n";
				logASM << "jz SetZero" << aluLabels << " \r\n";
				logASM << "mov eax, dword [esp+8] \r\n";
				logASM << "or eax, dword [esp+12] \r\n";
				logASM << "jz SetZero" << aluLabels << " \r\n";
				logASM << "mov dword [esp+8], 1 \r\n";
				logASM << "jmp OneSet" << aluLabels << " \r\n";
				logASM << "  SetZero" << aluLabels << ": \r\n";
				logASM << "mov dword [esp+8], 0 \r\n";
				logASM << "  OneSet" << aluLabels << ": \r\n";
				logASM << "mov dword [esp+12], 0 \r\n";
				logASM << "add esp, 8 \r\n";
				aluLabels++;
				break;
			case cmdLogAnd+(OTYPE_INT<<16):
				logASM << "  ; LAND int\r\n";
				if(!skipPopEAXOnIntALU)
					logASM << "pop eax \r\n";
				logASM << "cmp eax, 0 \r\n";
				logASM << "je pushZero" << aluLabels << "\r\n";
				logASM << "cmp dword [esp], 0 \r\n";
				logASM << "je pushZero" << aluLabels << "\r\n";
				logASM << "mov dword [esp], 1 ; true\r\n";
				logASM << "jmp pushedOne" << aluLabels << " \r\n";
				logASM << "  pushZero" << aluLabels << ": \r\n";
				logASM << "mov dword [esp], 0 ; false\r\n";
				logASM << "  pushedOne" << aluLabels << ": \r\n";
				aluLabels++;
				break;

			case cmdLogOr+(OTYPE_LONG<<16):
				logASM << "  ; LOR long\r\n";
				logASM << "mov eax, dword [esp] \r\n";
				logASM << "or eax, dword [esp+4] \r\n";
				logASM << "jnz SetOne" << aluLabels << " \r\n";
				logASM << "mov eax, dword [esp+8] \r\n";
				logASM << "or eax, dword [esp+12] \r\n";
				logASM << "jnz SetOne" << aluLabels << " \r\n";
				logASM << "mov dword [esp+8], 0 \r\n";
				logASM << "jmp ZeroSet" << aluLabels << " \r\n";
				logASM << "  SetOne" << aluLabels << ": \r\n";
				logASM << "mov dword [esp+8], 1 \r\n";
				logASM << "  ZeroSet" << aluLabels << ": \r\n";
				logASM << "mov dword [esp+12], 0 \r\n";
				logASM << "add esp, 8 \r\n";
				aluLabels++;
				break;
			case cmdLogOr+(OTYPE_INT<<16):
				logASM << "  ; LOR int\r\n";
				if(!skipPopEAXOnIntALU)
					logASM << "pop eax \r\n";
				logASM << "pop ebx \r\n";
				logASM << "or eax, ebx \r\n";
				logASM << "cmp eax, 0 \r\n";
				logASM << "je pushZero" << aluLabels << "\r\n";
				logASM << "push 1 ; true\r\n";
				logASM << "jmp pushedOne" << aluLabels << " \r\n";
				logASM << "  pushZero" << aluLabels << ": \r\n";
				logASM << "push 0 ; false\r\n";
				logASM << "  pushedOne" << aluLabels << ": \r\n";
				aluLabels++;
				break;

			case cmdLogXor+(OTYPE_LONG<<16):
				logASM << "  ; LXOR long\r\n";
				logASM << "xor eax, eax \r\n";
				logASM << "mov ebx, dword [esp] \r\n";
				logASM << "or ebx, dword [esp+4] \r\n";
				logASM << "setnz al \r\n";
				logASM << "xor ecx, ecx \r\n";
				logASM << "mov ebx, dword [esp+8] \r\n";
				logASM << "or ebx, dword [esp+12] \r\n";
				logASM << "setnz cl \r\n";
				logASM << "xor eax, ecx \r\n";
				logASM << "add esp, 8 \r\n";
				logASM << "mov dword [esp+4], 0 \r\n";
				logASM << "mov dword [esp], eax \r\n";
				break;
			case cmdLogXor+(OTYPE_INT<<16):
				logASM << "  ; LXOR int\r\n";
				logASM << "xor eax, eax \r\n";
				logASM << "cmp dword [esp], 0 \r\n";
				logASM << "setne al \r\n";
				logASM << "xor ecx, ecx \r\n";
				logASM << "cmp dword [esp+4], 0 \r\n";
				logASM << "setne cl \r\n";
				logASM << "xor eax, ecx \r\n";
				logASM << "pop ecx \r\n";
				logASM << "mov dword [esp], eax \r\n";
				break;
			default:
				throw string("Operation is not implemented");
			}
			skipPopEAXOnIntALU = false;
			if(!skipFstpOnDoubleALU)
				skipFldESPOnDoubleALU = false;
		}
		if(cmd >= cmdNeg && cmd <= cmdLogNot)
		{
			oFlag = *(unsigned char*)(&exCode[pos]);
			pos += 1;
			switch(cmd + (oFlag << 16))
			{
			case cmdNeg+(OTYPE_DOUBLE<<16):
				logASM << "  ; NEG double\r\n";
				logASM << "fld qword [esp] \r\n";
				logASM << "fchs \r\n";
				logASM << "fstp qword [esp] \r\n";
				break;
			case cmdNeg+(OTYPE_LONG<<16):
				logASM << "  ; NEG long\r\n";
				logASM << "neg dword [esp] \r\n";
				logASM << "adc dword [esp+4], 0 \r\n";
				logASM << "neg dword [esp+4] \r\n";
				break;
			case cmdNeg+(OTYPE_INT<<16):
				logASM << "  ; NEG int\r\n";
				logASM << "neg dword [esp] \r\n";
				break;

			case cmdLogNot+(OTYPE_DOUBLE<<16):
				logASM << "  ; LNOT double\r\n";
				logASM << "fldz \r\n";
				logASM << "fcomp qword [esp] ; ��������\r\n";
				logASM << "fnstsw ax ; ����� ������\r\n";
				logASM << "test ah, 44h ; �������� � '�����'\r\n";
				logASM << "jp pushZero" << aluLabels << " ; ��, �� �����\r\n";
				logASM << "mov dword [esp], 1 \r\n";
				logASM << "jmp pushedOne" << aluLabels << " \r\n";
				logASM << "  pushZero" << aluLabels << ": \r\n";
				logASM << "mov dword [esp], 0 \r\n";
				logASM << "  pushedOne" << aluLabels << ": \r\n";
				logASM << "fild dword [esp] \r\n";
				logASM << "fstp qword [esp] \r\n";
				aluLabels++;
				break;
			case cmdLogNot+(OTYPE_LONG<<16):
				logASM << "xor eax, eax \r\n";
				logASM << "mov ebx, dword [esp+4] \r\n";
				logASM << "or ebx, dword [esp] \r\n";
				logASM << "setz al \r\n";
				logASM << "mov dword [esp+4], 0 \r\n";
				logASM << "mov dword [esp], eax \r\n";
				break;
			case cmdLogNot+(OTYPE_INT<<16):
				logASM << "  ; LNOT int\r\n";
				logASM << "xor eax, eax \r\n";
				logASM << "cmp dword [esp], 0 \r\n";
				logASM << "sete al \r\n";
				logASM << "mov dword [esp], eax \r\n";
				break;

			case cmdBitNot+(OTYPE_LONG<<16):
				logASM << "  ; BNOT long\r\n";
				logASM << "not dword [esp] \r\n";
				logASM << "not dword [esp+4] \r\n";
				break;
			case cmdBitNot+(OTYPE_INT<<16):
				logASM << "  ; BNOT int\r\n";
				logASM << "not dword [esp] \r\n";
				break;
			default:
				throw string("Operation is not implemented");
			}
		}
		if(cmd >= cmdIncAt && cmd <= cmdDecAt)
		{
			int valind = -1, size;

			cFlag = *(unsigned short*)(&exCode[pos]);
			pos += 2;
			dt = flagDataType(cFlag);

			if(cmd == cmdIncAt)
				logASM << "  ; INCAT ";
			else
				logASM << "  ; DECAT ";
			char *typeNameD[] = { "char", "short", "int", "long", "float", "double" };
			logASM << typeNameD[dt/4] << "\r\n";

			unsigned int numEDX = 0;
			bool knownEDX = false;
			
			// ���� ������� ����� � �����
			if(flagShiftStk(cFlag))
			{
				valind = *(int*)(&exCode[pos]);
				pos += 4;
				if(knownEDXOnPush)
				{
					if(mulByVarSize)
						numEDX = edxValueForPush * lastVarSize + valind;
					else
						numEDX = edxValueForPush + valind;
					knownEDX = true;
					knownEDXOnPush = false;
				}else{
					if(skipPopEDXOnPush)
					{
						if(mulByVarSize)
						{
							if(valind != 0)
								logASM << "lea edx, [edx*" << lastVarSize << " + " << valind << "]\r\n";
							else
								logASM << "lea edx, [edx*" << lastVarSize << "]\r\n";
						}else{
							numEDX = valind;
						}
						skipPopEDXOnPush = false;
					}else{
						if(mulByVarSize)
						{
							logASM << "pop eax ; ����� �����\r\n";
							if(valind != 0)
								logASM << "lea edx, [eax*" << lastVarSize << " + " << valind << "] ; ����� ��������� �� ���� ���������� � ������� �� ����� � ����� � �� ������������ ������\r\n";
							else
								logASM << "lea edx, [eax*" << lastVarSize << "] ; ����� ��������� �� ���� ���������� � ������� �� ����� � ����� (opt: addr==0)\r\n";
						}else{
							if(valind != 0)
							{
								logASM << "pop edx ; ����� �����\r\n";
								numEDX = valind;
							}else{
								logASM << "pop edx ; ����� ��������� �� ���� ���������� � ������� �� ����� � ����� (opt: addr==0)\r\n";
							}
						}
					}
				}
			}else{
				valind = *(int*)(&exCode[pos]);
				pos += 4;

				knownEDX = true;
				numEDX = valind;
			}

			if(flagSizeOn(cFlag))
			{
				size = *(int*)(&exCode[pos]);
				pos += 4;
				logASM << "cmp eax, " << (mulByVarSize ? size/lastVarSize : size) << " ; ������� ����� � ������������\r\n";
				logASM << "jb movLabel" << movLabels << " ; ���� ����� ������ ��������� (� �� �������������) �� �� ��\r\n";
				logASM << "int 3 \r\n";
				logASM << "  movLabel" << movLabels << ":\r\n";
				movLabels++;
			}
			if(flagSizeStk(cFlag))
			{
				logASM << "cmp [esp], eax ; ������� � ������������ ������� � �����\r\n";
				logASM << "ja movLabel" << movLabels << " ; ���� ����� ������ ��������� (� �� �������������) �� �� ��\r\n";
				logASM << "int 3 \r\n";
				logASM << "  movLabel" << movLabels << ":\r\n";
				logASM << "pop eax ; ������ �������������� ������\r\n";
				movLabels++;
			}
			mulByVarSize = false;

			char *texts[] = { "", "edx + ", "ebp + " };
			char *needEDX = texts[1];
			char *needEBP = texts[2];
			if(knownEDX)
				needEDX = texts[0];
			if(flagAddrAbs(cFlag) && !addEBPtoEDXOnPush)
				needEBP = texts[0];
			addEBPtoEDXOnPush = false;

			unsigned int final = paramBase+numEDX;
			switch(cmd + (dt << 16))
			{
			case cmdIncAt+(DTYPE_DOUBLE<<16):
				logASM << "fld qword [" << needEDX << needEBP << final << "] ;\r\n";
				if(flagPushBefore(cFlag))
					logASM << "fld st0\r\n";
				logASM << "fld1 \r\n";
				logASM << "faddp \r\n";
				if(flagPushAfter(cFlag))
				{
					logASM << "fst qword [" << needEDX << needEBP << final << "] ;\r\n";
					logASM << "sub esp, 8\r\n";
					logASM << "fstp qword [esp]\r\n";
				}else{
					logASM << "fstp qword [" << needEDX << needEBP << final << "] ;\r\n";
				}
				if(flagPushBefore(cFlag))
					logASM << "sub esp, 8\r\nfstp qword [esp]\r\n";
				break;
			case cmdIncAt+(DTYPE_FLOAT<<16):
				logASM << "fld dword [" << needEDX << needEBP << final << "] ;\r\n";
				if(flagPushBefore(cFlag))
					logASM << "fld st0\r\n";
				logASM << "fld1 \r\n";
				logASM << "faddp \r\n";
				if(flagPushAfter(cFlag))
				{
					logASM << "fst dword [" << needEDX << needEBP << final << "] ;\r\n";
					logASM << "sub esp, 8\r\n";
					logASM << "fstp qword [esp]\r\n";
				}else{
					logASM << "fstp dword [" << needEDX << needEBP << final << "] ;\r\n";
				}
				if(flagPushBefore(cFlag))
					logASM << "sub esp, 8\r\nfstp qword [esp]\r\n";
				break;
			case cmdIncAt+(DTYPE_LONG<<16):
				logASM << "mov eax, dword [" << needEDX << needEBP << final << "] ;\r\n";
				logASM << "mov edx, dword [" << needEDX << needEBP << final+4 << "] ;\r\n";
				if(flagPushBefore(cFlag))
				{
					logASM << "push edx\r\n";
					logASM << "push eax\r\n";
				}
				logASM << "add eax, 1 \r\n";
				logASM << "adc edx, 0 \r\n";
				logASM << "mov dword [" << needEDX << needEBP << final << "], eax ;\r\n";
				logASM << "mov dword [" << needEDX << needEBP << final+4 << "], edx ;\r\n";
				if(flagPushAfter(cFlag))
				{
					logASM << "push edx\r\n";
					logASM << "push eax\r\n";
				}
				break;
			case cmdIncAt+(DTYPE_INT<<16):
				logASM << "mov eax, dword [" << needEDX << needEBP << final << "] ;\r\n";
				if(flagPushBefore(cFlag))
					logASM << "push eax\r\n";
				logASM << "add eax, 1 \r\n";
				logASM << "mov dword [" << needEDX << needEBP << final << "], eax ;\r\n";
				if(flagPushAfter(cFlag))
					logASM << "push eax\r\n";
				break;
			case cmdIncAt+(DTYPE_SHORT<<16):
				logASM << "movsx eax, word [" << needEDX << needEBP << final << "] ;\r\n";
				if(flagPushBefore(cFlag))
					logASM << "push eax\r\n";
				logASM << "add eax, 1 \r\n";
				logASM << "mov word [" << needEDX << needEBP << final << "], ax ;\r\n";
				if(flagPushAfter(cFlag))
					logASM << "push eax\r\n";
				break;
			case cmdIncAt+(DTYPE_CHAR<<16):
				logASM << "movsx eax, byte [" << needEDX << needEBP << final << "] ;\r\n";
				if(flagPushBefore(cFlag))
					logASM << "push eax\r\n";
				logASM << "add eax, 1 \r\n";
				logASM << "mov byte [" << needEDX << needEBP << final << "], al ;\r\n";
				if(flagPushAfter(cFlag))
					logASM << "push eax\r\n";
				break;

			case cmdDecAt+(DTYPE_DOUBLE<<16):
				logASM << "fld qword [" << needEDX << needEBP << final << "] ;\r\n";
				if(flagPushBefore(cFlag))
					logASM << "fld st0\r\n";
				logASM << "fld1 \r\n";
				logASM << "fsubp \r\n";
				if(flagPushAfter(cFlag))
				{
					logASM << "fst qword [" << needEDX << needEBP << final << "] ;\r\n";
					logASM << "sub esp, 8\r\n";
					logASM << "fstp qword [esp]\r\n";
				}else{
					logASM << "fstp qword [" << needEDX << needEBP << final << "] ;\r\n";
				}
				if(flagPushBefore(cFlag))
					logASM << "sub esp, 8\r\nfstp qword [esp]\r\n";
				break;
			case cmdDecAt+(DTYPE_FLOAT<<16):
				logASM << "fld dword [" << needEDX << needEBP << final << "] ;\r\n";
				if(flagPushBefore(cFlag))
					logASM << "fld st0\r\n";
				logASM << "fld1 \r\n";
				logASM << "fsubp \r\n";
				if(flagPushAfter(cFlag))
				{
					logASM << "fst dword [" << needEDX << needEBP << final << "] ;\r\n";
					logASM << "sub esp, 8\r\n";
					logASM << "fstp qword [esp]\r\n";
				}else{
					logASM << "fstp dword [" << needEDX << needEBP << final << "] ;\r\n";
				}
				if(flagPushBefore(cFlag))
					logASM << "sub esp, 8\r\nfstp qword [esp]\r\n";
				break;
			case cmdDecAt+(DTYPE_LONG<<16):
				logASM << "mov eax, dword [" << needEDX << needEBP << final << "] ;\r\n";
				logASM << "mov edx, dword [" << needEDX << needEBP << final+4 << "] ;\r\n";
				if(flagPushBefore(cFlag))
				{
					logASM << "push edx\r\n";
					logASM << "push eax\r\n";
				}
				logASM << "sub eax, 1 \r\n";
				logASM << "sbb edx, 0 \r\n";
				logASM << "mov dword [" << needEDX << needEBP << final << "], eax ;\r\n";
				logASM << "mov dword [" << needEDX << needEBP << final+4 << "], edx ;\r\n";
				if(flagPushAfter(cFlag))
				{
					logASM << "push edx\r\n";
					logASM << "push eax\r\n";
				}
				break;
			case cmdDecAt+(DTYPE_INT<<16):
				logASM << "mov eax, dword [" << needEDX << needEBP << final << "] ;\r\n";
				if(flagPushBefore(cFlag))
					logASM << "push eax\r\n";
				logASM << "sub eax, 1 \r\n";
				logASM << "mov dword [" << needEDX << needEBP << final << "], eax ;\r\n";
				if(flagPushAfter(cFlag))
					logASM << "push eax\r\n";
				break;
			case cmdDecAt+(DTYPE_SHORT<<16):
				logASM << "movsx eax, word [" << needEDX << needEBP << final << "] ;\r\n";
				if(flagPushBefore(cFlag))
					logASM << "push eax\r\n";
				logASM << "sub eax, 1 \r\n";
				logASM << "mov word [" << needEDX << needEBP << final << "], ax ;\r\n";
				if(flagPushAfter(cFlag))
					logASM << "push eax\r\n";
				break;
			case cmdDecAt+(DTYPE_CHAR<<16):
				logASM << "movsx eax, byte [" << needEDX << needEBP << final << "] ;\r\n";
				if(flagPushBefore(cFlag))
					logASM << "push eax\r\n";
				logASM << "sub eax, 1 \r\n";
				logASM << "mov byte [" << needEDX << needEBP << final << "], al ;\r\n";
				if(flagPushAfter(cFlag))
					logASM << "push eax\r\n";
				break;
			}
		}
	}
	logASM << "  gLabel" << pos << ": \r\n";
	logASM << "pop ebp\r\n";
	logASM << "ret; final return, if user skipped it\r\n";

	std::string	logASMstr = logASM.str();

#ifdef NULLC_LOG_FILES
	ofstream noOptFile("asmX86_noopt.txt", std::ios::binary);
	noOptFile << logASMstr;
	noOptFile.flush();
	noOptFile.close();
#endif

	std::vector<Command>*	x86Cmd = NULL;

#ifdef NULLC_LOG_FILES
	DeleteFile("asmX86.txt");
	ofstream m_FileStream("asmX86.txt", std::ios::binary | std::ios::out);
#endif
	if(optimize)
	{
		Optimizer_x86 optiMan;
		x86Cmd = optiMan.HashListing(logASMstr.c_str(), (int)logASMstr.length());
		std::vector<std::string> *optiList = optiMan.Optimize();
#ifdef NULLC_LOG_FILES
		for(unsigned int i = 0; i < optiList->size(); i++)
			m_FileStream << (*optiList)[i] << "\r\n";
#else
		(void)optiList;
#endif
	}else{
		Optimizer_x86 optiMan;
		x86Cmd = optiMan.HashListing(logASMstr.c_str(), (int)logASMstr.length());
#ifdef NULLC_LOG_FILES
		m_FileStream << logASMstr;
#endif
	}
#ifdef NULLC_LOG_FILES
	m_FileStream.flush();
	m_FileStream.close();
#endif

	// Translate to x86
	unsigned char *bytecode = binCode+20;//new unsigned char[16000];
	unsigned char *code = bytecode;
	char labelName[16];

	x86Reg	optiReg[] = { rNONE, rNONE, rEAX, rEBX, rECX, rEDX, rEDI, rESI, rESP, rEBP, rEAX, rEAX, rEBX, rEBX, rECX, rECX, rNONE, rNONE, rNONE };
	x86Size	optiSize[] = { sNONE, sBYTE, sWORD, sDWORD, sQWORD };

	x86ClearLabels();

	for(unsigned int i = 0, e = (unsigned int)x86Cmd->size(); i != e; i++)
	{
		//if(code-bytecode >= 0x0097)
		//	__asm int 3;
		Command	cmd = (*x86Cmd)[i];
		switch(cmd.Name)
		{
		case o_none:
			break;
		case o_mov:
			if(cmd.argA.type != Argument::ptr)
			{
				if(cmd.argB.type == Argument::number)
					code += x86MOV(code, optiReg[cmd.argA.type], cmd.argB.num);
				else if(cmd.argB.type == Argument::ptr)
					code += x86MOV(code, optiReg[cmd.argA.type], optiReg[cmd.argB.ptrReg[0]], sDWORD, cmd.argB.ptrNum);
				else
					code += x86MOV(code, optiReg[cmd.argA.type], optiReg[cmd.argB.type]);
			}else{
				if(cmd.argB.type == Argument::number)
					code += x86MOV(code, optiSize[cmd.argA.ptrSize], optiReg[cmd.argA.ptrReg[0]], cmd.argA.ptrNum, cmd.argB.num);
				else
					code += x86MOV(code, optiSize[cmd.argA.ptrSize], optiReg[cmd.argA.ptrReg[0]], optiReg[cmd.argA.ptrReg[1]], cmd.argA.ptrNum, optiReg[cmd.argB.type]);
			}
			break;
		case o_movsx:
			code += x86MOVSX(code, optiReg[cmd.argA.type], optiSize[cmd.argB.ptrSize], optiReg[cmd.argB.ptrReg[0]], optiReg[cmd.argB.ptrReg[1]], cmd.argB.ptrNum);
			break;
		case o_push:
			if(cmd.argA.type == Argument::number)
				code += x86PUSH(code, cmd.argA.num);
			else if(cmd.argA.type == Argument::ptr)
				code += x86PUSH(code, optiSize[cmd.argA.ptrSize], optiReg[cmd.argA.ptrReg[0]], optiReg[cmd.argA.ptrReg[1]], cmd.argA.ptrNum);
			else
				code += x86PUSH(code, optiReg[cmd.argA.type]);
			break;
		case o_pop:
			if(cmd.argA.type == Argument::ptr)
				code += x86POP(code, sDWORD, optiReg[cmd.argA.ptrReg[0]], optiReg[cmd.argA.ptrReg[1]], cmd.argA.ptrNum);
			else
				code += x86POP(code, optiReg[cmd.argA.type]);
			break;
		case o_lea:
			if(cmd.argB.labelName[0] != 0)
			{
				code += x86LEA(code, optiReg[cmd.argA.type], cmd.argB.labelName, cmd.argB.ptrNum);
			}else{
				if(cmd.argB.ptrMult != 1)
					code += x86LEA(code, optiReg[cmd.argA.type], optiReg[cmd.argB.ptrReg[0]], cmd.argB.ptrMult, cmd.argB.ptrNum);
				else
					code += x86LEA(code, optiReg[cmd.argA.type], optiReg[cmd.argB.ptrReg[0]], cmd.argB.ptrNum);
			}
			break;
		case o_xchg:
			if(cmd.argA.type == Argument::ptr)
				code += x86XCHG(code, sDWORD, optiReg[cmd.argA.ptrReg[0]], cmd.argA.ptrNum, optiReg[cmd.argB.type]);
			else if(cmd.argB.type == Argument::ptr)
				code += x86XCHG(code, sDWORD, optiReg[cmd.argB.ptrReg[0]], cmd.argB.ptrNum, optiReg[cmd.argA.type]);
			else
				code += x86XCHG(code, optiReg[cmd.argA.type], optiReg[cmd.argB.type]);
			break;
		case o_cdq:
			code += x86CDQ(code);
			break;
		case o_rep_movsd:
			code += x86REP_MOVSD(code);
			break;

		case o_jmp:
			code += x86JMP(code, cmd.argA.labelName, memcmp(cmd.argA.labelName, "near", 4) == 0 ? true : false);
			break;
		case o_ja:
			code += x86Jcc(code, cmd.argA.labelName, condA, memcmp(cmd.argA.labelName, "near", 4) == 0 ? true : false);
			break;
		case o_jae:
			code += x86Jcc(code, cmd.argA.labelName, condAE, memcmp(cmd.argA.labelName, "near", 4) == 0 ? true : false);
			break;
		case o_jb:
			code += x86Jcc(code, cmd.argA.labelName, condB, memcmp(cmd.argA.labelName, "near", 4) == 0 ? true : false);
			break;
		case o_jbe:
			code += x86Jcc(code, cmd.argA.labelName, condBE, memcmp(cmd.argA.labelName, "near", 4) == 0 ? true : false);
			break;
		case o_jc:
			code += x86Jcc(code, cmd.argA.labelName, condC, memcmp(cmd.argA.labelName, "near", 4) == 0 ? true : false);
			break;
		case o_je:
			code += x86Jcc(code, cmd.argA.labelName, condE, memcmp(cmd.argA.labelName, "near", 4) == 0 ? true : false);
			break;
		case o_jz:
			code += x86Jcc(code, cmd.argA.labelName, condZ, memcmp(cmd.argA.labelName, "near", 4) == 0 ? true : false);
			break;
		case o_jg:
			code += x86Jcc(code, cmd.argA.labelName, condG, memcmp(cmd.argA.labelName, "near", 4) == 0 ? true : false);
			break;
		case o_jl:
			code += x86Jcc(code, cmd.argA.labelName, condL, memcmp(cmd.argA.labelName, "near", 4) == 0 ? true : false);
			break;
		case o_jne:
			code += x86Jcc(code, cmd.argA.labelName, condNE, memcmp(cmd.argA.labelName, "near", 4) == 0 ? true : false);
			break;
		case o_jnp:
			code += x86Jcc(code, cmd.argA.labelName, condNP, memcmp(cmd.argA.labelName, "near", 4) == 0 ? true : false);
			break;
		case o_jnz:
			code += x86Jcc(code, cmd.argA.labelName, condNZ, memcmp(cmd.argA.labelName, "near", 4) == 0 ? true : false);
			break;
		case o_jp:
			code += x86Jcc(code, cmd.argA.labelName, condP, memcmp(cmd.argA.labelName, "near", 4) == 0 ? true : false);
			break;
		case o_call:
			if(cmd.argA.type == Argument::label)
				code += x86CALL(code, cmd.argA.labelName);
			else
				code += x86CALL(code, optiReg[cmd.argA.type]);
			break;
		case o_ret:
			code += x86RET(code);
			break;

		case o_fld:
			if(cmd.argA.type == Argument::ptr)
			{
				if(cmd.argA.ptrReg[1] != Argument::none)
					code += x86FLD(code, optiSize[cmd.argA.ptrSize], optiReg[cmd.argA.ptrReg[0]], optiReg[cmd.argA.ptrReg[1]], cmd.argA.ptrNum);
				else
					code += x86FLD(code, optiSize[cmd.argA.ptrSize], optiReg[cmd.argA.ptrReg[0]], cmd.argA.ptrNum);
			}else{
				code += x86FLD(code, (x87Reg)cmd.argA.fpArg);
			}
			break;
		case o_fild:
			code += x86FILD(code, optiSize[cmd.argA.ptrSize], optiReg[cmd.argA.ptrReg[0]]);
			break;
		case o_fistp:
			code += x86FISTP(code, optiSize[cmd.argA.ptrSize], optiReg[cmd.argA.ptrReg[0]], cmd.argA.ptrNum);
			break;
		case o_fst:
			if(cmd.argA.ptrReg[1] != Argument::none)
				code += x86FST(code, optiSize[cmd.argA.ptrSize], optiReg[cmd.argA.ptrReg[0]], optiReg[cmd.argA.ptrReg[1]], cmd.argA.ptrNum);
			else
				code += x86FST(code, optiSize[cmd.argA.ptrSize], optiReg[cmd.argA.ptrReg[0]], cmd.argA.ptrNum);
			break;
		case o_fstp:
			if(cmd.argA.type == Argument::ptr)
			{
				if(cmd.argA.ptrReg[1] != Argument::none)
					code += x86FSTP(code, optiSize[cmd.argA.ptrSize], optiReg[cmd.argA.ptrReg[0]], optiReg[cmd.argA.ptrReg[1]], cmd.argA.ptrNum);
				else
					code += x86FSTP(code, optiSize[cmd.argA.ptrSize], optiReg[cmd.argA.ptrReg[0]], cmd.argA.ptrNum);
			}else{
				code += x86FSTP(code, (x87Reg)cmd.argA.fpArg);
			}
			break;
		case o_fnstsw:
			code += x86FNSTSW(code);
			break;
		case o_fstcw:
			code += x86FSTCW(code);
			break;
		case o_fldcw:
			code += x86FLDCW(code, cmd.argA.ptrNum);
			break;

		case o_neg:
			code += x86NEG(code, sDWORD, optiReg[cmd.argA.ptrReg[0]], cmd.argA.ptrNum);
			break;
		case o_add:
			if(cmd.argA.type == Argument::ptr)
				code += x86ADD(code, sDWORD, optiReg[cmd.argA.ptrReg[0]], cmd.argA.ptrNum, optiReg[cmd.argB.type]);
			else
				code += x86ADD(code, optiReg[cmd.argA.type], cmd.argB.num);
			break;
		case o_adc:
			if(cmd.argA.type == Argument::ptr)
			{
				if(cmd.argB.type == Argument::number)
					code += x86ADC(code, sDWORD, optiReg[cmd.argA.ptrReg[0]], cmd.argA.ptrNum, cmd.argB.num);
				else
					code += x86ADC(code, sDWORD, optiReg[cmd.argA.ptrReg[0]], cmd.argA.ptrNum, optiReg[cmd.argB.type]);
			}else{
				code += x86ADC(code, optiReg[cmd.argA.type], cmd.argB.num);
			}
			break;
		case o_sub:
			if(cmd.argA.type == Argument::ptr)
				code += x86SUB(code, sDWORD, optiReg[cmd.argA.ptrReg[0]], cmd.argA.ptrNum, optiReg[cmd.argB.type]);
			else
				code += x86SUB(code, optiReg[cmd.argA.type], cmd.argB.num);
			break;
		case o_sbb:
			if(cmd.argA.type == Argument::ptr)
				code += x86SBB(code, sDWORD, optiReg[cmd.argA.ptrReg[0]], cmd.argA.ptrNum, optiReg[cmd.argB.type]);
			else
				code += x86SBB(code, optiReg[cmd.argA.type], cmd.argB.num);
			break;
		case o_imul:
			if(cmd.argB.type != Argument::none)
				code += x86IMUL(code, optiReg[cmd.argA.type], cmd.argB.num);
			else
				code += x86IMUL(code, optiReg[cmd.argA.type]);
			break;
		case o_idiv:
			code += x86IDIV(code, sDWORD, optiReg[cmd.argA.ptrReg[0]]);
			break;
		case o_shl:
			if(cmd.argA.type == Argument::ptr)
				code += x86SHL(code, sDWORD, optiReg[cmd.argA.ptrReg[0]], cmd.argB.num);
			else
				code += x86SHL(code, optiReg[cmd.argA.type], cmd.argB.num);
			break;
		case o_sal:
			code += x86SAL(code);
			break;
		case o_sar:
			code += x86SAR(code);
			break;
		case o_not:
			code += x86NOT(code, sDWORD, optiReg[cmd.argA.ptrReg[0]], cmd.argA.ptrNum);
			break;
		case o_and:
			code += x86AND(code, sDWORD, optiReg[cmd.argA.ptrReg[0]], cmd.argA.ptrNum, optiReg[cmd.argB.type]);
			break;
		case o_or:
			if(cmd.argA.type == Argument::ptr)
				code += x86OR(code, sDWORD, optiReg[cmd.argA.ptrReg[0]], cmd.argA.ptrNum, optiReg[cmd.argB.type]);
			else if(cmd.argB.type == Argument::ptr)
				code += x86OR(code, optiReg[cmd.argA.type], sDWORD, optiReg[cmd.argB.ptrReg[0]], cmd.argB.ptrNum);
			else
				code += x86OR(code, optiReg[cmd.argA.type], optiReg[cmd.argB.type]);
			break;
		case o_xor:
			if(cmd.argA.type == Argument::ptr)
				code += x86XOR(code, sDWORD, optiReg[cmd.argA.ptrReg[0]], cmd.argA.ptrNum, optiReg[cmd.argB.type]);
			else
				code += x86XOR(code, optiReg[cmd.argA.type], optiReg[cmd.argB.type]);
			break;
		case o_cmp:
			if(cmd.argA.type == Argument::ptr)
			{
				if(cmd.argB.type == Argument::number)
					code += x86CMP(code, sDWORD, optiReg[cmd.argA.ptrReg[0]], cmd.argA.ptrNum, cmd.argB.num);
				else
					code += x86CMP(code, sDWORD, optiReg[cmd.argA.ptrReg[0]], cmd.argA.ptrNum, optiReg[cmd.argB.type]);
			}else{
				if(cmd.argB.type == Argument::number)
					code += x86CMP(code, optiReg[cmd.argA.type], cmd.argB.num);
				else
					code += x86CMP(code, optiReg[cmd.argA.type], optiReg[cmd.argB.type]);
			}
			break;
		case o_test:
			if(cmd.argB.type == Argument::number)
				code += x86TESTah(code, (char)cmd.argB.num);
			else
				code += x86TEST(code, optiReg[cmd.argA.type], optiReg[cmd.argB.type]);
			break;

		case o_setl:
			code += x86SETcc(code, condL, optiReg[cmd.argA.type]);
			break;
		case o_setg:
			code += x86SETcc(code, condG, optiReg[cmd.argA.type]);
			break;
		case o_setle:
			code += x86SETcc(code, condLE, optiReg[cmd.argA.type]);
			break;
		case o_setge:
			code += x86SETcc(code, condGE, optiReg[cmd.argA.type]);
			break;
		case o_sete:
			code += x86SETcc(code, condE, optiReg[cmd.argA.type]);
			break;
		case o_setne:
			code += x86SETcc(code, condNE, optiReg[cmd.argA.type]);
			break;
		case o_setz:
			code += x86SETcc(code, condZ, optiReg[cmd.argA.type]);
			break;
		case o_setnz:
			code += x86SETcc(code, condNZ, optiReg[cmd.argA.type]);
			break;

		case o_fadd:
			code += x86FADD(code, optiSize[cmd.argA.ptrSize], optiReg[cmd.argA.ptrReg[0]]);
			break;
		case o_faddp:
			code += x86FADDP(code);
			break;
		case o_fmul:
			code += x86FMUL(code, optiSize[cmd.argA.ptrSize], optiReg[cmd.argA.ptrReg[0]]);
			break;
		case o_fmulp:
			code += x86FMULP(code);
			break;
		case o_fsub:
			code += x86FSUB(code, optiSize[cmd.argA.ptrSize], optiReg[cmd.argA.ptrReg[0]]);
			break;
		case o_fsubr:
			code += x86FSUBR(code, optiSize[cmd.argA.ptrSize], optiReg[cmd.argA.ptrReg[0]]);
			break;
		case o_fsubp:
			code += x86FSUBP(code);
			break;
		case o_fsubrp:
			code += x86FSUBRP(code);
			break;
		case o_fdiv:
			code += x86FDIV(code, optiSize[cmd.argA.ptrSize], optiReg[cmd.argA.ptrReg[0]]);
			break;
		case o_fdivr:
			code += x86FDIVR(code, optiSize[cmd.argA.ptrSize], optiReg[cmd.argA.ptrReg[0]]);
			break;
		case o_fdivrp:
			code += x86FDIVRP(code);
			break;
		case o_fchs:
			code += x86FCHS(code);
			break;
		case o_fprem:
			code += x86FPREM(code);
			break;
		case o_fcomp:
			code += x86FCOMP(code, optiSize[cmd.argA.ptrSize], optiReg[cmd.argA.ptrReg[0]], cmd.argA.ptrNum);
			break;
		case o_fldz:
			code += x86FLDZ(code);
			break;
		case o_fld1:
			code += x86FLD1(code);
			break;
		case o_fsincos:
			code += x86FSINCOS(code);
			break;
		case o_fptan:
			code += x86FPTAN(code);
			break;
		case o_fsqrt:
			code += x86FSQRT(code);
			break;
		case o_frndint:
			code += x86FRNDINT(code);
			break;

		case o_int:
			code += x86INT(code, 3);
			break;
		case o_dd:
			*(int*)code = cmd.argA.num;
#ifdef NULLC_X86_CMP_FASM
			code += 4;
#endif
			for(unsigned int i = 0; i < exFunctions.size(); i++)
			{
				int marker = (('N' << 24) | exFunctions[i]->address);
				if(marker == cmd.argA.num)
					exFuncInfo[i].startInByteCode = (int)(code-bytecode);
			}
			if(cmd.argA.num == int(('G' << 24) | offsetToGlobalCode))
			{
				globalStartInBytecode = (int)(code-bytecode);
			}
			break;
		case o_label:
			memset(labelName, 0, 16);
			strncpy(labelName, cmd.strName->c_str(), strchr(cmd.strName->c_str(), ':')-cmd.strName->c_str());
			x86AddLabel(code, labelName);
			break;
		case o_other:
			if(memcmp(cmd.strName->c_str(), "use32", 5) == 0)
				break;
			else if((*cmd.strName)[0] == 0)
				break;
			else
				__asm int 3;
			break;
		}
	}
	binCodeSize = (unsigned int)(code-bytecode);

#ifdef NULLC_X86_CMP_FASM
	FILE *fMyCode = fopen("asmX86my.bin", "wb");
	fwrite(bytecode, 1, code-bytecode, fMyCode);
	fclose(fMyCode);

	// debug
	unsigned char *bytecodeCopy = new unsigned char[code-bytecode+1];
	memcpy(bytecodeCopy, bytecode, code-bytecode);

	STARTUPINFO stInfo;
	PROCESS_INFORMATION prInfo;

	// Compile using fasm
	memset(&stInfo, 0, sizeof(stInfo));
	stInfo.cb = sizeof(stInfo);
	stInfo.dwFlags = STARTF_USESHOWWINDOW;
	stInfo.wShowWindow = SW_HIDE;
	memset(&prInfo, 0, sizeof(prInfo));

	DeleteFile("asmX86.bin");

	if(!CreateProcess(NULL, "fasm.exe asmX86.txt", NULL, NULL, false, 0, NULL, ".\\", &stInfo, &prInfo))
		throw std::string("Failed to create process");

	if(WAIT_TIMEOUT == WaitForSingleObject(prInfo.hProcess, 5000))
		throw std::string("Compilation to x86 binary takes too much time (timeout=5sec)");

	CloseHandle(prInfo.hProcess);
	CloseHandle(prInfo.hThread);

	FILE *fCode = fopen("asmX86.bin", "rb");
	if(!fCode)
		throw std::string("Failed to open output file");
	
	fseek(fCode, 0, SEEK_END);
	unsigned int size = ftell(fCode);
	fseek(fCode, 0, SEEK_SET);
	if(size > 200000)
		throw std::string("Byte code is too big (size > 200000)");
	fread(binCode+20, 1, size, fCode);
	binCodeSize = size;

	for(int i = 0; i < code-bytecode; i++)
		if(binCode[i+20] != bytecodeCopy[i])
			__asm int 3;
	//memcpy(binCode+20, bytecode, code-bytecode);
	//binCodeSize = code-bytecode;

	delete[] bytecodeCopy;
#endif NULLC_X86_CMP_FASM
}

string ExecutorX86::GetListing()
{
	return logASM.str();
}

const char* ExecutorX86::GetResult() throw()
{
	long long combined = 0;
	*((int*)(&combined)) = runResult2;
	*((int*)(&combined)+1) = runResult;

	switch(runResultType)
	{
	case OTYPE_DOUBLE:
		sprintf(execResult, "%f", *(double*)(&combined));
		break;
	case OTYPE_LONG:
		sprintf(execResult, "%I64dL", combined);
		break;
	case OTYPE_INT:
		sprintf(execResult, "%d", runResult);
		break;
	}
	return execResult;
}

const char*	ExecutorX86::GetExecError() throw()
{
	return execError;
}

void ExecutorX86::SetOptimization(int toggle)
{
	optimize = toggle;
}

char* ExecutorX86::GetVariableData()
{
	return paramData;
}

#endif
