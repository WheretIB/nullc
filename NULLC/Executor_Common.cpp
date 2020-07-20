#include "Executor_Common.h"

#include "StdLib.h"
#include "nullc.h"
#include "nullc_debug.h"
#include "Executor_X86.h"
#include "Executor_LLVM.h"
#include "Executor_RegVm.h"
#include "Linker.h"

#if !defined(NULLC_NO_RAW_EXTERNAL_CALL)
#define dcAllocMem NULLC::alloc
#define dcFreeMem  NULLC::dealloc

#include "../external/dyncall/dyncall.h"
#endif

typedef uintptr_t markerType;

namespace
{
	const uintptr_t OBJECT_VISIBLE = 1 << 0;
	const uintptr_t OBJECT_FREED = 1 << 1;
	const uintptr_t OBJECT_FINALIZABLE = 1 << 2;
	const uintptr_t OBJECT_FINALIZED = 1 << 3;
	const uintptr_t OBJECT_ARRAY = 1 << 4;
}

namespace NULLC
{
	Linker *commonLinker = NULL;
}

void CommonSetLinker(Linker* linker)
{
	NULLC::commonLinker = linker;
}

namespace GC
{
	// Range of memory that is not checked. Used to exclude pointers to stack from marking and GC
	char	*unmanageableBase = NULL;
	char	*unmanageableTop = NULL;
}

unsigned ConvertFromAutoRef(unsigned int target, unsigned int source)
{
	if(source == target)
		return 1;
	while(NULLC::commonLinker->exTypes[source].baseType)
	{
		source = NULLC::commonLinker->exTypes[source].baseType;
		if(source == target)
			return 1;
	}
	return 0;
}

bool AreMembersAligned(ExternTypeInfo *lType, ExternTypeInfo *exTypes, ExternMemberInfo *exTypeExtra)
{
	bool aligned = 1;
	for(unsigned m = 0; m < lType->memberCount; m++)
	{
		ExternMemberInfo &member = exTypeExtra[lType->memberOffset + m];
		ExternTypeInfo &memberType = exTypes[member.type];
		unsigned pos = member.offset;

		switch(memberType.type)
		{
		case ExternTypeInfo::TYPE_COMPLEX:
			break;
		case ExternTypeInfo::TYPE_VOID:
			break;
		case ExternTypeInfo::TYPE_INT:
			if(pos % 4 != 0)
				aligned = 0;
			break;
		case ExternTypeInfo::TYPE_FLOAT:
			break;
		case ExternTypeInfo::TYPE_LONG:
			if(pos % 8 != 0)
				aligned = 0;
			break;
		case ExternTypeInfo::TYPE_DOUBLE:
			break;
		case ExternTypeInfo::TYPE_SHORT:
			if(pos % 2 != 0)
				aligned = 0;
			break;
		case ExternTypeInfo::TYPE_CHAR:
			break;
		}
		pos += memberType.size;
	}
	//printf("%s\n", aligned ? "aligned" : "unaligned");
	return aligned;
}

bool HasIntegerMembersInRange(ExternTypeInfo &type, unsigned fromOffset, unsigned toOffset, ExternTypeInfo *exTypes, ExternMemberInfo *exTypeExtra)
{
	for(unsigned m = 0; m < type.memberCount; m++)
	{
		ExternMemberInfo &member = exTypeExtra[type.memberOffset + m];

		ExternTypeInfo &memberType = exTypes[member.type];

		if(memberType.type == ExternTypeInfo::TYPE_COMPLEX)
		{
			// Handle opaque types
			bool opaqueType = memberType.subCat != ExternTypeInfo::CAT_CLASS || memberType.memberCount == 0;

			if(opaqueType)
			{
				if(member.offset + memberType.size > fromOffset && member.offset < toOffset)
					return true;
			}
			else
			{
				if(HasIntegerMembersInRange(memberType, fromOffset - member.offset, toOffset - member.offset, exTypes, exTypeExtra))
					return true;
			}
		}
		else if(memberType.type != ExternTypeInfo::TYPE_FLOAT && memberType.type != ExternTypeInfo::TYPE_DOUBLE)
		{
			if(member.offset + memberType.size > fromOffset && member.offset < toOffset)
				return true;
		}
	}

	return false;
}

unsigned int PrintStackFrame(int address, char* current, unsigned int bufSize, bool withVariables)
{
	const char *start = current;

	unsigned exFunctionsSize = 0;
	ExternFuncInfo *exFunctions = nullcDebugFunctionInfo(&exFunctionsSize);

	char *exSymbols = nullcDebugSymbols(NULL);

	unsigned exModulesSize = 0;
	ExternModuleInfo *exModules = nullcDebugModuleInfo(&exModulesSize);

	unsigned exInfoSize = 0;
	ExternSourceInfo *exInfo = nullcDebugSourceInfo(&exInfoSize);

	char *source = nullcDebugSource();

	int funcID = -1;
	for(unsigned int i = 0; i < exFunctionsSize; i++)
	{
		if(address >= exFunctions[i].regVmAddress && address < (exFunctions[i].regVmAddress + exFunctions[i].regVmCodeSize))
			funcID = i;
	}

	if(funcID != -1)
		current += NULLC::SafeSprintf(current, bufSize - int(current - start), "%s", &exSymbols[exFunctions[funcID].offsetToName]);
	else
		current += NULLC::SafeSprintf(current, bufSize - int(current - start), "%s", address == -1 ? "external" : "global scope");

	if(address != -1)
	{
		unsigned int infoID = 0;
		unsigned int i = address - 1;
		while((infoID < exInfoSize - 1) && (i >= exInfo[infoID + 1].instruction))
			infoID++;
		const char *codeStart = source + exInfo[infoID].sourceOffset;

		// Find beginning of the line
		while(codeStart != source && *(codeStart-1) != '\n')
			codeStart--;

		// Skip whitespace
		while(*codeStart == ' ' || *codeStart == '\t')
			codeStart++;
		const char *codeEnd = codeStart;

		// Find corresponding module
		unsigned moduleID = ~0u;
		const char *prevEnd = NULL;
		for(unsigned l = 0; l < exModulesSize; l++)
		{
			// special check for main module
			if(source + exModules[l].sourceOffset > prevEnd && codeStart >= prevEnd && codeStart < source + exModules[l].sourceOffset)
				break;
			if(codeStart >= source + exModules[l].sourceOffset && codeStart < source + exModules[l].sourceOffset + exModules[l].sourceSize)
				moduleID = l;
			prevEnd = source + exModules[l].sourceOffset + exModules[l].sourceSize;
		}
		const char *moduleStart = NULL;
		if(moduleID != ~0u)
			moduleStart = source + exModules[moduleID].sourceOffset;
		else
			moduleStart = prevEnd;

		// Find line number
		unsigned line = 0;
		while(moduleStart < codeStart)
		{
			if(*moduleStart++ == '\n')
				line++;
		}

		// Find ending of the line
		while(*codeEnd != '\0' && *codeEnd != '\r' && *codeEnd != '\n')
			codeEnd++;

		int codeLength = (int)(codeEnd - codeStart);
		current += NULLC::SafeSprintf(current, bufSize - int(current - start), " (line %d: at %.*s)\r\n", line + 1, codeLength, codeStart);
	}

	if(withVariables)
	{
		unsigned exTypesSize = 0;
		ExternTypeInfo *exTypes = nullcDebugTypeInfo(&exTypesSize);

		if(funcID != -1)
		{
			unsigned exLocalsSize = 0;
			ExternLocalInfo *exLocals = nullcDebugLocalInfo(&exLocalsSize);

			for(unsigned int i = 0; i < exFunctions[funcID].localCount + exFunctions[funcID].externCount; i++)
			{
				ExternLocalInfo &lInfo = exLocals[exFunctions[funcID].offsetToFirstLocal + i];
				const char *typeName = &exSymbols[exTypes[lInfo.type].offsetToName];
				const char *localName = &exSymbols[lInfo.offsetToName];
				const char *localType = lInfo.paramType == ExternLocalInfo::PARAMETER ? "param" : (lInfo.paramType == ExternLocalInfo::EXTERNAL ? "extern" : "local");
				const char *offsetType = (lInfo.paramType == ExternLocalInfo::PARAMETER || lInfo.paramType == ExternLocalInfo::LOCAL) ? "base" : (lInfo.closeListID & 0x80000000 ? "local" : "closure");
				current += NULLC::SafeSprintf(current, bufSize - int(current - start), " %s %d: %s %s (at %s+%d size %d)\r\n",	localType, i, typeName, localName, offsetType, lInfo.offset, exTypes[lInfo.type].size);
			}
		}
		else
		{
			unsigned exVariablesSize = 0;
			ExternVarInfo *exVariables = nullcDebugVariableInfo(&exVariablesSize);

			for(unsigned i = 0; i < exVariablesSize; i++)
			{
				ExternVarInfo &vInfo = exVariables[i];

				const char *typeName = &exSymbols[exTypes[vInfo.type].offsetToName];
				const char *localName = &exSymbols[vInfo.offsetToName];
				const char *localType = "global";
				current += NULLC::SafeSprintf(current, bufSize - int(current - start), " %s %d: %s %s (at %d size %d)\r\n", localType, i, typeName, localName, vInfo.offset, exTypes[vInfo.type].size);
			}
		}
	}

	return (unsigned int)(current - start);
}

void DumpStackFrames()
{
	nullcDebugBeginCallStack();
	while(unsigned int address = nullcDebugGetStackFrame())
	{
		char buf[1024];
		PrintStackFrame(address, buf, 1024, true);

		printf("%s", buf);
	}
}

void nullcPrintDepthIndent(unsigned indentDepth)
{
	for(unsigned i = 0; i < indentDepth; i++)
		printf("  ");
}

void nullcPrintPointerMarkerInfo(markerType marker)
{
	char *codeSymbols = nullcDebugSymbols(NULL);
	unsigned codeTypeCount = 0;
	ExternTypeInfo *codeTypes = nullcDebugTypeInfo(&codeTypeCount);

	if(marker & OBJECT_VISIBLE)
		printf("visible");
	else
		printf("unmarked");

	if(marker & OBJECT_FREED)
	{
		printf(" freed");
		return;
	}

	if(marker & OBJECT_FINALIZABLE)
		printf(" finalizable");
	if(marker & OBJECT_FINALIZED)
		printf(" finalized");
	if(marker & OBJECT_ARRAY)
		printf(" array");

	unsigned type = unsigned(marker >> 8);

	printf(" type '%s' #%u", codeSymbols + codeTypes[type].offsetToName, type);
}

void nullcPrintBasicVariableInfo(const ExternTypeInfo& type, char* ptr)
{
	char *codeSymbols = nullcDebugSymbols(NULL);

	if(type.subCat == ExternTypeInfo::CAT_POINTER)
	{
		if(void *base = NULLC::GetBasePointer(*(void**)ptr))
		{
			markerType *marker = (markerType*)((char*)base - sizeof(markerType));

			printf("%p [base %p, ", *(void**)ptr, base);
			nullcPrintPointerMarkerInfo(*marker);
			printf("]");
		}
		else
		{
			printf("%p", *(void**)ptr);
		}

		return;
	}

	switch(type.type)
	{
	case ExternTypeInfo::TYPE_CHAR:
		if(strcmp(codeSymbols + type.offsetToName, "bool") == 0)
		{
			printf(*(unsigned char*)ptr ? "true" : "false");
		}
		else
		{
			if(*(unsigned char*)ptr)
				printf("'%c' (%d)", *(unsigned char*)ptr, (int)*(unsigned char*)ptr);
			else
				printf("0");
		}
		break;
	case ExternTypeInfo::TYPE_SHORT:
		printf("%d", *(short*)ptr);
		break;
	case ExternTypeInfo::TYPE_INT:
		printf(type.subType == 0 ? "%d" : "0x%x", *(int*)ptr);
		break;
	case ExternTypeInfo::TYPE_LONG:
		printf(type.subType == 0 ? "%lld" : "0x%llx", *(long long*)ptr);
		break;
	case ExternTypeInfo::TYPE_FLOAT:
		printf("%f", *(float*)ptr);
		break;
	case ExternTypeInfo::TYPE_DOUBLE:
		printf("%f", *(double*)ptr);
		break;
	default:
		printf("not basic type");
	}
}

void nullcPrintAutoInfo(char* ptr, unsigned indentDepth)
{
	char *codeSymbols = nullcDebugSymbols(NULL);
	ExternTypeInfo *codeTypes = nullcDebugTypeInfo(NULL);

	void *target = *(void**)(ptr + 4);

	nullcPrintDepthIndent(indentDepth);
	printf("typeid type = %d (%s)\n", *(int*)ptr, codeSymbols + codeTypes[*(int*)(ptr)].offsetToName);
	nullcPrintDepthIndent(indentDepth);
	printf("%s ref ptr = ", codeSymbols + codeTypes[*(int*)(ptr)].offsetToName);

	if(void *base = NULLC::GetBasePointer(target))
	{
		markerType *marker = (markerType*)((char*)base - sizeof(markerType));

		printf("%p [base %p, ", target, base);
		nullcPrintPointerMarkerInfo(*marker);
		printf("]\n");
	}
	else
	{
		printf("%p\n", target);
	}
}

void nullcPrintAutoArrayInfo(char* ptr, unsigned indentDepth)
{
	char *codeSymbols = nullcDebugSymbols(NULL);
	ExternTypeInfo *codeTypes = nullcDebugTypeInfo(NULL);

	NULLCAutoArray *arr = (NULLCAutoArray*)ptr;

	void *target = *(void**)arr->ptr;

	nullcPrintDepthIndent(indentDepth);
	printf("typeid type = %d (%s)\n", arr->typeID, codeSymbols + codeTypes[arr->typeID].offsetToName);

	nullcPrintDepthIndent(indentDepth);
	printf("%s[] data = ", codeSymbols + codeTypes[arr->typeID].offsetToName);

	if(void *base = NULLC::GetBasePointer(target))
	{
		markerType *marker = (markerType*)((char*)base - sizeof(markerType));

		printf("%p [base %p, ", target, base);
		nullcPrintPointerMarkerInfo(*marker);
		printf("]\n");
	}
	else
	{
		printf("%p\n", target);
	}

	nullcPrintDepthIndent(indentDepth);
	printf("int len = %u\n", arr->len);
}

void nullcPrintVariableInfo(const ExternTypeInfo& type, char* ptr, unsigned indentDepth);

void nullcPrintArrayVariableInfo(const ExternTypeInfo& type, char* ptr, unsigned indentDepth)
{
	char *codeSymbols = nullcDebugSymbols(NULL);
	ExternTypeInfo *codeTypes = nullcDebugTypeInfo(NULL);

	char *target = ptr;
	unsigned size = type.arrSize;

	if(type.arrSize == ~0u)
	{
		NULLCArray *arr = (NULLCArray*)ptr;

		target = arr->ptr;
		size = arr->len;

		nullcPrintDepthIndent(indentDepth);
		printf("%s[] data = ", codeSymbols + codeTypes[type.subType].offsetToName);

		if(void *base = NULLC::GetBasePointer(target))
		{
			markerType *marker = (markerType*)((char*)base - sizeof(markerType));

			printf("%p [base %p, ", target, base);
			nullcPrintPointerMarkerInfo(*marker);
			printf("]\n");
		}
		else
		{
			printf("%p\n", target);
		}

		nullcPrintDepthIndent(indentDepth);
		printf("len %u\n", arr->len);
	}

	if(type.pointerCount)
	{
		ExternTypeInfo &elementType = codeTypes[type.subType];

		for(unsigned i = 0; i < size && i < 32; i++)
		{
			nullcPrintDepthIndent(indentDepth);
			printf("[%u]", i);

			if(elementType.subCat == ExternTypeInfo::CAT_NONE || elementType.subCat == ExternTypeInfo::CAT_POINTER)
			{
				printf(" = ");
				nullcPrintBasicVariableInfo(elementType, target);
				printf("\n");
			}
			else if(strcmp(codeSymbols + elementType.offsetToName, "typeid") == 0)
			{
				printf(" = %s\n", codeSymbols + codeTypes[*(int*)(target)].offsetToName);
			}
			else
			{
				printf("\n");
				nullcPrintVariableInfo(elementType, target, indentDepth + 1);
			}

			target += elementType.size;
		}
	}
}

void nullcPrintFunctionPointerInfo(const ExternTypeInfo& type, char* ptr, unsigned indentDepth)
{
	char *codeSymbols = nullcDebugSymbols(NULL);
	ExternTypeInfo *codeTypes = nullcDebugTypeInfo(NULL);
	ExternFuncInfo *codeFunctions = nullcDebugFunctionInfo(NULL);
	ExternMemberInfo *codeTypeExtra = nullcDebugTypeExtraInfo(NULL);
	ExternLocalInfo *codeLocals = nullcDebugLocalInfo(NULL);

	ExternFuncInfo &func = codeFunctions[*(int*)(ptr + NULLC_PTR_SIZE)];
	ExternTypeInfo &returnType = codeTypes[codeTypeExtra[type.memberOffset].type];

	nullcPrintDepthIndent(indentDepth);
	printf("function %d %s %s(", *(int*)(ptr + NULLC_PTR_SIZE), codeSymbols + returnType.offsetToName, codeSymbols + func.offsetToName);
	for(unsigned arg = 0; arg < func.paramCount; arg++)
	{
		ExternLocalInfo &lInfo = codeLocals[func.offsetToFirstLocal + arg];
		printf("%s %s%s", codeSymbols + codeTypes[lInfo.type].offsetToName, codeSymbols + lInfo.offsetToName, arg == func.paramCount - 1 ? "" : ", ");
	}
	printf(")\n");

	nullcPrintDepthIndent(indentDepth);
	printf("%s context = %p\n", func.contextType == ~0u ? "void ref" : codeSymbols + codeTypes[func.contextType].offsetToName, *(void**)(ptr));

	if(*(char**)(ptr))
		nullcPrintVariableInfo(codeTypes[codeTypes[func.contextType].subType], *(char**)(ptr), indentDepth + 1);
}

void nullcPrintComplexVariableInfo(const ExternTypeInfo& type, char* ptr, unsigned indentDepth)
{
	char *codeSymbols = nullcDebugSymbols(NULL);
	ExternTypeInfo *codeTypes = nullcDebugTypeInfo(NULL);
	ExternMemberInfo *codeTypeExtra = nullcDebugTypeExtraInfo(NULL);

	const char *memberName = codeSymbols + type.offsetToName + (unsigned)strlen(codeSymbols + type.offsetToName) + 1;

	for(unsigned i = 0; i < type.memberCount; i++)
	{
		ExternTypeInfo &memberType = codeTypes[codeTypeExtra[type.memberOffset + i].type];

		unsigned localOffset = codeTypeExtra[type.memberOffset + i].offset;

		nullcPrintDepthIndent(indentDepth);
		printf("%s %s", codeSymbols + memberType.offsetToName, memberName);

		if(memberType.subCat == ExternTypeInfo::CAT_NONE || memberType.subCat == ExternTypeInfo::CAT_POINTER)
		{
			printf(" = ");
			nullcPrintBasicVariableInfo(memberType, ptr + localOffset);
			printf("\n");
		}
		else if(strcmp(codeSymbols + memberType.offsetToName, "typeid") == 0)
		{
			printf(" = %s\n", codeSymbols + codeTypes[*(int*)(ptr + localOffset)].offsetToName);
		}
		else
		{
			printf("\n");
			nullcPrintVariableInfo(memberType, ptr + localOffset, indentDepth + 1);
		}

		memberName += (unsigned)strlen(memberName) + 1;
	}
}

void nullcPrintVariableInfo(const ExternTypeInfo& type, char* ptr, unsigned indentDepth)
{
	char *codeSymbols = nullcDebugSymbols(NULL);

	if(strcmp(codeSymbols + type.offsetToName, "typeid") == 0)
		return;

	if(strcmp(codeSymbols + type.offsetToName, "auto ref") == 0)
	{
		nullcPrintAutoInfo(ptr, indentDepth);
		return;
	}
	if(strcmp(codeSymbols + type.offsetToName, "auto[]") == 0)
	{
		nullcPrintAutoArrayInfo(ptr, indentDepth);
		return;
	}

	switch(type.subCat)
	{
	case ExternTypeInfo::CAT_NONE:
		break;
	case ExternTypeInfo::CAT_ARRAY:
		nullcPrintArrayVariableInfo(type, ptr, indentDepth);
		break;
	case ExternTypeInfo::CAT_POINTER:
		break;
	case ExternTypeInfo::CAT_FUNCTION:
		nullcPrintFunctionPointerInfo(type, ptr, indentDepth);
		break;
	case ExternTypeInfo::CAT_CLASS:
		nullcPrintComplexVariableInfo(type, ptr, indentDepth);
		break;
	}
}

void nullcDumpStackData()
{
	unsigned dataCount = ~0u;
	char *data = (char*)nullcGetVariableData(&dataCount);

	unsigned variableCount = 0;
	ExternVarInfo *codeVars = nullcDebugVariableInfo(&variableCount);

	unsigned codeTypeCount = 0;
	ExternTypeInfo *codeTypes = nullcDebugTypeInfo(&codeTypeCount);

	unsigned functionCount = 0;
	ExternFuncInfo *codeFunctions = nullcDebugFunctionInfo(&functionCount);

	ExternLocalInfo *codeLocals = nullcDebugLocalInfo(NULL);
	char *codeSymbols = nullcDebugSymbols(NULL);

	unsigned offset = 0;

	nullcDebugBeginCallStack();
	while(unsigned address = nullcDebugGetStackFrame())
	{
		char buf[1024];
		PrintStackFrame(address, buf, 1024, false);

		printf("%s", buf);

		ExternFuncInfo *func = nullcDebugConvertAddressToFunction(address, codeFunctions, functionCount);

		unsigned indent = 1;

		if(func)
		{
			ExternFuncInfo &function = *func;

			// Align offset to the first variable (by 16 byte boundary)
			int alignOffset = (offset % 16 != 0) ? (16 - (offset % 16)) : 0;
			offset += alignOffset;

			printf("%p: function %s(", (void*)(data + offset), codeSymbols + function.offsetToName);
			for(unsigned arg = 0; arg < function.paramCount; arg++)
			{
				ExternLocalInfo &lInfo = codeLocals[function.offsetToFirstLocal + arg];
				printf("%s %s", codeSymbols + codeTypes[lInfo.type].offsetToName, codeSymbols + lInfo.offsetToName);
				if(arg != function.paramCount - 1)
					printf(", ");
			}

			unsigned argumentsSize = function.bytesToPop;
			unsigned stackSize = (function.stackSize + 0xf) & ~0xf;

			printf(") argument size %u, stack size %u\n", argumentsSize, stackSize);

			for(unsigned i = 0; i < function.localCount; i++)
			{
				ExternLocalInfo &lInfo = codeLocals[function.offsetToFirstLocal + i];
				ExternTypeInfo &localType = codeTypes[lInfo.type];

				nullcPrintDepthIndent(indent);
				printf("%p: %s %s", (void*)(data + offset + lInfo.offset), codeSymbols + codeTypes[lInfo.type].offsetToName, codeSymbols + lInfo.offsetToName);

				if(localType.subCat == ExternTypeInfo::CAT_NONE || localType.subCat == ExternTypeInfo::CAT_POINTER)
				{
					printf(" = ");
					nullcPrintBasicVariableInfo(localType, data + offset + lInfo.offset);
					printf("\n");
				}
				else if(strcmp(codeSymbols + localType.offsetToName, "typeid") == 0)
				{
					printf(" = %s\n", codeSymbols + codeTypes[*(int*)(data + offset + lInfo.offset)].offsetToName);
				}
				else
				{
					printf("\n");
					nullcPrintVariableInfo(localType, data + offset + lInfo.offset, indent + 1);
				}
			}

			if(function.parentType != ~0u)
			{
				char *ptr = (char*)(data + offset + function.bytesToPop - NULLC_PTR_SIZE);

				nullcPrintDepthIndent(indent);
				printf("%p: %s %s = %p\n", (void*)ptr, "$this", codeSymbols + codeTypes[function.parentType].offsetToName, *(void**)ptr);
			}

			if(function.contextType != ~0u)
			{
				char *ptr = (char*)(data + offset + function.bytesToPop - NULLC_PTR_SIZE);

				nullcPrintDepthIndent(indent);
				printf("%p: %s %s = %p\n", (void*)ptr, "$context", codeSymbols + codeTypes[function.contextType].offsetToName, *(void**)ptr);
			}

			offset += stackSize;
		}
		else
		{
			for(unsigned i = 0; i < variableCount; i++)
			{
				ExternTypeInfo &type = codeTypes[codeVars[i].type];

				nullcPrintDepthIndent(indent);
				printf("%p: %s %s", (void*)(data + codeVars[i].offset), codeSymbols + type.offsetToName, codeSymbols + codeVars[i].offsetToName);

				if(type.subCat == ExternTypeInfo::CAT_NONE || type.subCat == ExternTypeInfo::CAT_POINTER)
				{
					printf(" = ");
					nullcPrintBasicVariableInfo(type, data + codeVars[i].offset);
					printf("\n");
				}
				else if(strcmp(codeSymbols + type.offsetToName, "typeid") == 0)
				{
					printf(" = %s\n", codeSymbols + codeTypes[*(int*)(data + codeVars[i].offset)].offsetToName);
				}
				else
				{
					printf("\n");
					nullcPrintVariableInfo(type, data + codeVars[i].offset, indent + 1);
				}

				if(codeVars[i].offset + type.size > offset)
					offset = codeVars[i].offset + type.size;
			}
		}
	}
}

#define GC_DEBUG_PRINT(...) (void)0
//#define GC_DEBUG_PRINT printf

namespace GC
{
	unsigned int	objectName = NULLC::GetStringHash("auto ref");
	unsigned int	autoArrayName = NULLC::GetStringHash("auto[]");

	void CheckArray(char* ptr, const ExternTypeInfo& type);
	void CheckClass(char* ptr, const ExternTypeInfo& type);
	void CheckFunction(char* ptr);
	void CheckVariable(char* ptr, const ExternTypeInfo& type);

	struct RootInfo
	{
		RootInfo(): ptr(0), type(0){}
		RootInfo(char* ptr, const ExternTypeInfo* type): ptr(ptr), type(type){}

		char *ptr;
		const ExternTypeInfo* type;
	};
	FastVector<RootInfo> rootsA, rootsB;
	FastVector<RootInfo> *curr = NULL, *next = NULL;

	HashMap<int> functionIDs;

	void PrintMarker(markerType marker)
	{
		GC_DEBUG_PRINT("\tMarker is 0x%2x [", unsigned(marker));

		if(marker & OBJECT_VISIBLE)
		{
			GC_DEBUG_PRINT("visible");
		}
		else
		{
			GC_DEBUG_PRINT("unmarked");
		}

		if(marker & OBJECT_FREED)
		{
			GC_DEBUG_PRINT(" freed]\r\n");

			assert(!"reached a freed pointer");
			return;
		}

		if(marker & OBJECT_FINALIZABLE)
			GC_DEBUG_PRINT(" finalizable");

		if(marker & OBJECT_FINALIZED)
			GC_DEBUG_PRINT(" finalized");

		if(marker & OBJECT_ARRAY)
			GC_DEBUG_PRINT(" array");

		GC_DEBUG_PRINT("] type %d '%s'\r\n", unsigned(marker >> 8), NULLC::commonLinker->exSymbols.data + NULLC::commonLinker->exTypes[unsigned(marker >> 8)].offsetToName);
	}

	char* ReadVmMemoryPointer(void* address)
	{
		char *result;
		memcpy(&result, address, sizeof(char*));
		return result;
	}

	// Function that marks memory blocks belonging to GC
	void MarkPointer(char* ptr, const ExternTypeInfo& type, bool takeSubtype)
	{
		// We have pointer to stack that has a pointer inside, so 'ptr' is really a pointer to pointer
		char *target = ReadVmMemoryPointer(ptr);

		// Check for unmanageable ranges. Range of 0x00000000-0x00010000 is unmanageable by default due to upvalues with offsets inside closures.
		if(target > (char*)0x00010000 && (target < unmanageableBase || target > unmanageableTop))
		{
			// Get type that pointer points to
			GC_DEBUG_PRINT("\tGlobal pointer [ref] %s %p (at %p)\r\n", NULLC::commonLinker->exSymbols.data + type.offsetToName, target, ptr);

			// Get pointer to the start of memory block. Some pointers may point to the middle of memory blocks
			unsigned int *basePtr = (unsigned int*)NULLC::GetBasePointer(target);

			// If there is no base, this pointer points to memory that is not GCs memory
			if(!basePtr)
				return;

			GC_DEBUG_PRINT("\tPointer base is %p\r\n", basePtr);

			// Marker is before the block
			markerType *marker = (markerType*)((char*)basePtr - sizeof(markerType));
			PrintMarker(*marker);

			// If block is unmarked
			if(!(*marker & OBJECT_VISIBLE))
			{
				// Mark block as used
				*marker |= OBJECT_VISIBLE;

				GC_DEBUG_PRINT("\tMarked as used\r\n");

				unsigned targetSubType = type.subType;

				if(takeSubtype && targetSubType == 0)
					targetSubType = unsigned(*marker >> 8);

				// And if type is not simple, check memory to which pointer points to
				if(type.subCat != ExternTypeInfo::CAT_NONE)
				{
					GC_DEBUG_PRINT("\tPointer %p scheduled on next loop\r\n", target);

					next->push_back(RootInfo(target, takeSubtype ? &NULLC::commonLinker->exTypes[targetSubType] : &type));
				}
			}
		}
	}

	void CheckArrayElements(char* ptr, unsigned size, const ExternTypeInfo& elementType)
	{
		if(!elementType.pointerCount)
			return;

		// Check every array element
		switch(elementType.subCat)
		{
		case ExternTypeInfo::CAT_NONE:
			break;
		case ExternTypeInfo::CAT_ARRAY:
			for(unsigned i = 0; i < size; i++, ptr += elementType.size)
				CheckArray(ptr, elementType);
			break;
		case ExternTypeInfo::CAT_POINTER:
			for(unsigned i = 0; i < size; i++, ptr += elementType.size)
				MarkPointer(ptr, elementType, true);
			break;
		case ExternTypeInfo::CAT_FUNCTION:
			for(unsigned i = 0; i < size; i++, ptr += elementType.size)
				CheckFunction(ptr);
			break;
		case ExternTypeInfo::CAT_CLASS:
			for(unsigned i = 0; i < size; i++, ptr += elementType.size)
				CheckClass(ptr, elementType);
			break;
		}
	}

	// Function that checks arrays for pointers
	void CheckArray(char* ptr, const ExternTypeInfo& type)
	{
		// Get array element type
		ExternTypeInfo *subType = &NULLC::commonLinker->exTypes[type.subType];

		// Real array size (changed for unsized arrays)
		unsigned int size = type.arrSize;

		// If array type is an unsized array, check pointer that points to actual array contents
		if(type.arrSize == ~0u)
		{
			// Get real array size
			size = *(int*)(ptr + NULLC_PTR_SIZE);

			// Switch pointer to array data
			ptr = ReadVmMemoryPointer(ptr);

			// If uninitialized or points to stack memory, return
			if(!ptr || ptr <= (char*)0x00010000 || (ptr >= unmanageableBase && ptr <= unmanageableTop))
			{
				if(ptr > (char*)0x00010000)
					GC_DEBUG_PRINT("\tSkipping stack pointer [array] %p\r\n", ptr);

				return;
			}

			GC_DEBUG_PRINT("\tGlobal pointer [array] %p\r\n", ptr);

			// Get base pointer
			unsigned int *basePtr = (unsigned int*)NULLC::GetBasePointer(ptr);

			// If there is no base, this pointer points to memory that is not GCs memory
			if(!basePtr)
				return;

			GC_DEBUG_PRINT("\tPointer base is %p\r\n", basePtr);

			markerType	*marker = (markerType*)((char*)basePtr - sizeof(markerType));
			PrintMarker(*marker);

			// If there is no base pointer or memory already marked, exit
			if((*marker & OBJECT_VISIBLE))
				return;

			// Mark memory as used
			*marker |= OBJECT_VISIBLE;

			GC_DEBUG_PRINT("\tMarked as used\r\n");
		}
		else if(type.nameHash == autoArrayName)
		{
			NULLCAutoArray *data = (NULLCAutoArray*)ptr;

			// Get real variable type
			subType = &NULLC::commonLinker->exTypes[data->typeID];

			// Skip uninitialized array
			if(!data->ptr)
				return;

			// Mark target data
			MarkPointer((char*)&data->ptr, *subType, false);

			// Switch pointer to target
			ptr = data->ptr;

			// Get array size
			size = data->len;
		}

		CheckArrayElements(ptr, size, *subType);
	}

	// Function that checks classes for pointers
	void CheckClass(char* ptr, const ExternTypeInfo& type)
	{
		const ExternTypeInfo *realType = &type;
		if(type.nameHash == objectName)
		{
			// Get real variable type
			realType = &NULLC::commonLinker->exTypes[*(int*)ptr];

			// Switch pointer to target
			char *target = ReadVmMemoryPointer(ptr + 4);

			// If uninitialized or points to stack memory, return
			if(!target || target <= (char*)0x00010000 || (target >= unmanageableBase && target <= unmanageableTop))
			{
				if(target > (char*)0x00010000)
					GC_DEBUG_PRINT("\tSkipping stack pointer [class] %p\r\n", target);

				return;
			}

			GC_DEBUG_PRINT("\tGlobal pointer [class] %p\r\n", target);

			// Get base pointer
			unsigned int *basePtr = (unsigned int*)NULLC::GetBasePointer(target);

			// If there is no base, this pointer points to memory that is not GCs memory
			if(!basePtr)
				return;

			GC_DEBUG_PRINT("\tPointer base is %p\r\n", basePtr);

			markerType	*marker = (markerType*)((char*)basePtr - sizeof(markerType));
			PrintMarker(*marker);

			// If there is no base pointer or memory already marked, exit
			if((*marker & OBJECT_VISIBLE))
				return;

			// Mark memory as used
			*marker |= OBJECT_VISIBLE;

			GC_DEBUG_PRINT("\tMarked as used, fixing up target\r\n");

			// Fixup target
			CheckVariable(target, *realType);

			// Exit
			return;
		}
		else if(type.nameHash == autoArrayName)
		{
			CheckArray(ptr, type);
			// Exit
			return;
		}

		// Get class member type list
		ExternMemberInfo *memberList = realType->pointerCount ? &NULLC::commonLinker->exTypeExtra[realType->memberOffset + realType->memberCount] : NULL;

		// Check pointer members
		for(unsigned int n = 0; n < realType->pointerCount; n++)
		{
			// Get member type
			ExternTypeInfo &subType = NULLC::commonLinker->exTypes[memberList[n].type];
			unsigned int pos = memberList[n].offset;
			// Check member
			CheckVariable(ptr + pos, subType);
		}
	}

	// Function that checks function context for pointers
	void CheckFunction(char* ptr)
	{
		NULLCFuncPtr *fPtr = (NULLCFuncPtr*)ptr;

		// If there's no context, there's nothing to check
		if(!fPtr->context)
			return;

		const ExternFuncInfo &func = NULLC::commonLinker->exFunctions[fPtr->id];

		// External functions shouldn't be checked
		if(func.regVmAddress == -1)
			return;

		// If context is "this" pointer
		if(func.contextType != ~0u)
		{
			const ExternTypeInfo &classType = NULLC::commonLinker->exTypes[func.contextType];
			MarkPointer((char*)&fPtr->context, classType, true);
		}
	}

	// Function that decides, how variable of type 'type' should be checked for pointers
	void CheckVariable(char* ptr, const ExternTypeInfo& type)
	{
		const ExternTypeInfo *realType = &type;

		if(type.typeFlags & ExternTypeInfo::TYPE_IS_EXTENDABLE)
			realType = &NULLC::commonLinker->exTypes[*(int*)ptr];

		if(!realType->pointerCount)
			return;

		switch(type.subCat)
		{
		case ExternTypeInfo::CAT_NONE:
			break;
		case ExternTypeInfo::CAT_ARRAY:
			CheckArray(ptr, type);
			break;
		case ExternTypeInfo::CAT_POINTER:
			MarkPointer(ptr, type, true);
			break;
		case ExternTypeInfo::CAT_FUNCTION:
			CheckFunction(ptr);
			break;
		case ExternTypeInfo::CAT_CLASS:
			CheckClass(ptr, *realType);
			break;
		}
	}
}

// Set range of memory that is not checked. Used to exclude pointers to stack from marking and GC
void SetUnmanagableRange(char* base, unsigned int size)
{
	GC::unmanageableBase = base;
	GC::unmanageableTop = base + size;
}
int IsPointerUnmanaged(NULLCRef ptr)
{
	return ptr.ptr >= GC::unmanageableBase && ptr.ptr <= GC::unmanageableTop;
}

// Main function for marking all pointers in a program
void MarkUsedBlocks()
{
	GC_DEBUG_PRINT("Unmanageable range: %p-%p\r\n", GC::unmanageableBase, GC::unmanageableTop);

	// Get information about programs' functions, variables, types and symbols (for debug output)
	ExternFuncInfo	*functions = NULLC::commonLinker->exFunctions.data;
	ExternVarInfo	*vars = NULLC::commonLinker->exVariables.data;
	ExternTypeInfo	*types = NULLC::commonLinker->exTypes.data;
	char			*symbols = NULLC::commonLinker->exSymbols.data;
	(void)symbols;

	GC::functionIDs.init();
	GC::functionIDs.clear();

	GC::curr = &GC::rootsA;
	GC::next = &GC::rootsB;
	GC::curr->clear();
	GC::next->clear();

	// To check every stack frame, we have to get it first. But we have multiple executors, so flow alternates depending on which executor we are running
	void *unknownExec = NULL;
	unsigned int execID = nullcGetCurrentExecutor(&unknownExec);

	if(execID != NULLC_LLVM)
	{
		// Mark global variables
		for(unsigned int i = 0; i < NULLC::commonLinker->exVariables.size(); i++)
		{
			GC_DEBUG_PRINT("Global %s %s (with offset of %d)\r\n", symbols + types[vars[i].type].offsetToName, symbols + vars[i].offsetToName, vars[i].offset);
			GC::CheckVariable(GC::unmanageableBase + vars[i].offset, types[vars[i].type]);
		}
	}else{
#ifdef NULLC_LLVM_SUPPORT
		ExecutorLLVM *exec = (ExecutorLLVM*)unknownExec;

		unsigned count = 0;
		char *data = exec->GetVariableData(&count);

		for(unsigned int i = 0; i < NULLC::commonLinker->exVariables.size(); i++)
		{
			GC_DEBUG_PRINT("Global %s %s (with offset of %d)\r\n", symbols + types[vars[i].type].offsetToName, symbols + vars[i].offsetToName, vars[i].offset);
			GC::CheckVariable(data + vars[i].offset, types[vars[i].type]);
		}
#endif
	}

	// Starting stack offset is equal to global variable size
	int offset = NULLC::commonLinker->globalVarSize;
	
	// Init stack trace
	unsigned currentFrame = 0;

	// Mark local variables
	while(true)
	{
		int address = 0;

		// Get next address from call stack

#ifdef NULLC_BUILD_X86_JIT
		if(execID == NULLC_X86)
		{
			ExecutorX86 *exec = (ExecutorX86*)unknownExec;
			address = exec->GetCallStackAddress(currentFrame++);
		}
#endif

#ifdef NULLC_LLVM_SUPPORT
		if(execID == NULLC_LLVM)
		{
			ExecutorLLVM *exec = (ExecutorLLVM*)unknownExec;
			address = exec->GetCallStackAddress(currentFrame++);
		}
#endif

		if(execID == NULLC_REG_VM)
		{
			ExecutorRegVm *exec = (ExecutorRegVm*)unknownExec;
			address = exec->GetCallStackAddress(currentFrame++);
		}

		// If failed, exit
		if(address == 0)
			break;

		// Find corresponding function
		int *cachedFuncID = GC::functionIDs.find(address);

		int funcID = -1;
		if(cachedFuncID)
		{
			funcID = *cachedFuncID;
		}
		else
		{
			for(unsigned i = 0; i < NULLC::commonLinker->exFunctions.size(); i++)
			{
				if(address >= functions[i].regVmAddress && address < (functions[i].regVmAddress + functions[i].regVmCodeSize))
					funcID = i;
			}

			GC::functionIDs.insert(address, funcID);
		}

		// If we are not in global scope
		if(funcID != -1)
		{
			ExternFuncInfo &function = functions[funcID];

			// Align offset to the first variable (by 16 byte boundary)
			int alignOffset = (offset % 16 != 0) ? (16 - (offset % 16)) : 0;
			offset += alignOffset;
			GC_DEBUG_PRINT("In function %s (with offset of %d)\r\n", symbols + function.offsetToName, alignOffset);

			unsigned stackSize = (function.stackSize + 0xf) & ~0xf;

			// Check every function local
			for(unsigned i = 0; i < function.localCount; i++)
			{
				// Get information about local
				ExternLocalInfo &lInfo = NULLC::commonLinker->exLocals[function.offsetToFirstLocal + i];

				GC_DEBUG_PRINT("Local %s %s (with offset of %d+%d)\r\n", symbols + types[lInfo.type].offsetToName, symbols + lInfo.offsetToName, offset, lInfo.offset);
				// Check it
				GC::CheckVariable(GC::unmanageableBase + offset + lInfo.offset, types[lInfo.type]);
			}

			if(function.contextType != ~0u)
			{
				GC_DEBUG_PRINT("Local %s $context (with offset of %d+%d)\r\n", symbols + types[function.contextType].offsetToName, offset, function.bytesToPop - NULLC_PTR_SIZE);
				char *ptr = GC::unmanageableBase + offset + function.bytesToPop - NULLC_PTR_SIZE;
				GC::MarkPointer(ptr, types[function.contextType], false);
			}

			offset += stackSize;

			GC_DEBUG_PRINT("Moving offset to next frame by %d bytes\r\n", stackSize);
		}
	}

	// Check for pointers in stack
	char *tempStackBase = NULL, *tempStackTop = NULL;

#ifdef NULLC_BUILD_X86_JIT
	if(execID == NULLC_X86)
	{
		ExecutorX86 *exec = (ExecutorX86*)unknownExec;
		tempStackBase = (char*)exec->GetStackStart();
		tempStackTop = (char*)exec->GetStackEnd();
	}
#endif

#ifdef NULLC_LLVM_SUPPORT
	if(execID == NULLC_LLVM)
	{
		ExecutorLLVM *exec = (ExecutorLLVM*)unknownExec;
		tempStackBase = (char*)exec->GetStackStart();
		tempStackTop = (char*)exec->GetStackEnd();
	}
#endif

	if(execID == NULLC_REG_VM)
	{
		ExecutorRegVm *exec = (ExecutorRegVm*)unknownExec;
		tempStackBase = (char*)exec->GetStackStart();
		tempStackTop = (char*)exec->GetStackEnd();
	}

	GC_DEBUG_PRINT("Check stack from %p to %p\r\n", tempStackBase, tempStackTop);

	// Check that temporary stack range is correct
	assert(tempStackTop >= tempStackBase);

	// Check temporary stack for pointers
	while(tempStackBase + sizeof(void*) <= tempStackTop)
	{
		char *ptr = GC::ReadVmMemoryPointer(tempStackBase);

		// Check for unmanageable ranges. Range of 0x00000000-0x00010000 is unmanageable by default due to upvalues with offsets inside closures.
		if(ptr > (char*)0x00010000 && (ptr < GC::unmanageableBase || ptr > GC::unmanageableTop))
		{
			// Get pointer base
			unsigned int *basePtr = (unsigned int*)NULLC::GetBasePointer(ptr);
			// If there is no base, this pointer points to memory that is not GCs memory
			if(basePtr)
			{
				GC_DEBUG_PRINT("\tGlobal pointer [stack] %p\r\n", ptr);

				GC_DEBUG_PRINT("\tPointer base is %p\r\n", basePtr);

				markerType *marker = (markerType*)((char*)basePtr - sizeof(markerType));

				// Might step on a left-over pointer in stale registers
				if(*marker & OBJECT_FREED)
				{
					tempStackBase += 4;
					continue;
				}

				GC::PrintMarker(*marker);

				// If block is unmarked, mark it as used
				if(!(*marker & OBJECT_VISIBLE))
				{
					unsigned typeID = unsigned(*marker >> 8);
					ExternTypeInfo &type = types[typeID];

					*marker |= OBJECT_VISIBLE;

					GC_DEBUG_PRINT("\tMarked as used, checking content\r\n");

					if(*marker & OBJECT_ARRAY)
					{
						unsigned arrayPadding = type.defaultAlign > 4 ? type.defaultAlign : 4;

						char *elements = (char*)basePtr + arrayPadding;

						unsigned size;
						memcpy(&size, elements - sizeof(unsigned), sizeof(unsigned));

						GC::CheckArrayElements(elements, size, type);
					}
					else
					{
						// And if type is not simple, check memory to which pointer points to
						if(type.subCat != ExternTypeInfo::CAT_NONE)
							GC::CheckVariable((char*)basePtr, type);
					}
				}
			}
		}
		tempStackBase += 4;
	}

	while(GC::next->size())
	{
		GC_DEBUG_PRINT("Checking new roots\r\n");

		FastVector<GC::RootInfo> *tmp = GC::curr;
		GC::curr = GC::next;
		GC::next = tmp;

		for(GC::RootInfo *c = GC::curr->data, *e = GC::curr->data + GC::curr->size(); c != e; c++)
		{
			GC_DEBUG_PRINT("Root %s %p\r\n", NULLC::commonLinker->exSymbols.data + c->type->offsetToName, c->ptr);

			GC::CheckVariable(c->ptr, *c->type);
		}

		GC::curr->clear();
	}

	GC_DEBUG_PRINT("\r\n");
}

void ResetGC()
{
	GC::rootsA.reset();
	GC::rootsB.reset();

	GC::functionIDs.reset();
}

namespace
{
	long long vmLoadLong(void* target)
	{
		long long value;
		memcpy(&value, target, sizeof(long long));
		return value;
	}

	void vmStoreLong(void* target, long long value)
	{
		memcpy(target, &value, sizeof(long long));
	}

	double vmLoadDouble(void* target)
	{
		double value;
		memcpy(&value, target, sizeof(double));
		return value;
	}

	void vmStoreDouble(void* target, double value)
	{
		memcpy(target, &value, sizeof(double));
	}

	char* vmLoadPointer(void* target)
	{
		char* value;
		memcpy(&value, target, sizeof(char*));
		return value;
	}
}

#if !defined(NULLC_NO_RAW_EXTERNAL_CALL)
void RunRawExternalFunction(DCCallVM *dcCallVM, ExternFuncInfo &func, ExternLocalInfo *exLocals, ExternTypeInfo *exTypes, ExternMemberInfo *exTypeExtra, unsigned *argumentStorage, unsigned *resultStorage)
{
	(void)exTypeExtra;

	assert(func.funcPtrRaw);

	void* fPtr = (void*)func.funcPtrRaw;
	unsigned retType = func.retType;

	unsigned *stackStart = argumentStorage;

	dcReset(dcCallVM);

	ExternTypeInfo &funcType = exTypes[func.funcType];

	ExternMemberInfo &member = exTypeExtra[funcType.memberOffset];
	ExternTypeInfo &returnType = exTypes[member.type];

#if defined(_WIN64)
	bool returnByPointer = func.returnShift > 1;
#elif !defined(_M_X64)
	bool returnByPointer = true;
#elif defined(__aarch64__)
	bool returnByPointer = false;

	bool opaqueType = returnType.subCat != ExternTypeInfo::CAT_CLASS || returnType.memberCount == 0;

	bool firstQwordInteger = opaqueType || HasIntegerMembersInRange(returnType, 0, 8, exTypes, exTypeExtra);
	bool secondQwordInteger = opaqueType || HasIntegerMembersInRange(returnType, 8, 16, exTypes, exTypeExtra);
#else
	bool returnByPointer = func.returnShift > 4 || member.type == NULLC_TYPE_AUTO_REF || (returnType.subCat == ExternTypeInfo::CAT_CLASS && !AreMembersAligned(&returnType, exTypes, exTypeExtra));

	bool opaqueType = returnType.subCat != ExternTypeInfo::CAT_CLASS || returnType.memberCount == 0;

	bool firstQwordInteger = opaqueType || HasIntegerMembersInRange(returnType, 0, 8, exTypes, exTypeExtra);
	bool secondQwordInteger = opaqueType || HasIntegerMembersInRange(returnType, 8, 16, exTypes, exTypeExtra);
#endif

	unsigned ret[128];

	if(retType == ExternFuncInfo::RETURN_UNKNOWN && returnByPointer)
		dcArgPointer(dcCallVM, ret);

	for(unsigned i = 0; i < func.paramCount; i++)
	{
		// Get information about local
		ExternLocalInfo &lInfo = exLocals[func.offsetToFirstLocal + i];

		ExternTypeInfo &tInfo = exTypes[lInfo.type];

		switch(tInfo.type)
		{
		case ExternTypeInfo::TYPE_COMPLEX:
#if defined(_WIN64)
			if(tInfo.size <= 4)
			{
				// This branch also handles 0 byte structs
				dcArgInt(dcCallVM, *(int*)stackStart);
				stackStart += 1;
			}
			else if(tInfo.size <= 8)
			{
				dcArgLongLong(dcCallVM, vmLoadLong(stackStart));
				stackStart += 2;
			}
			else
			{
				dcArgPointer(dcCallVM, stackStart);
				stackStart += tInfo.size / 4;
			}
#elif defined(__aarch64__)
			if(tInfo.size <= 4)
			{
				// This branch also handles 0 byte structs
				dcArgInt(dcCallVM, *(int*)stackStart);
				stackStart += 1;
			}
			else if(tInfo.size <= 8)
			{
				dcArgLongLong(dcCallVM, vmLoadLong(stackStart));
				stackStart += 2;
			}
			else if(tInfo.size <= 12)
			{
				dcArgLongLong(dcCallVM, vmLoadLong(stackStart));
				dcArgInt(dcCallVM, *(int*)(stackStart + 2));
				stackStart += 3;
			}
			else if(tInfo.size <= 16)
			{
				dcArgLongLong(dcCallVM, vmLoadLong(stackStart));
				dcArgLongLong(dcCallVM, vmLoadLong(stackStart + 2));
				stackStart += 4;
			}
			else
			{
				dcArgPointer(dcCallVM, stackStart);
				stackStart += tInfo.size / 4;
			}
#elif defined(_M_X64)
			if(tInfo.size > 16 || lInfo.type == NULLC_TYPE_AUTO_REF || (tInfo.subCat == ExternTypeInfo::CAT_CLASS && !AreMembersAligned(&tInfo, exTypes, exTypeExtra)))
			{
				dcArgStack(dcCallVM, stackStart, (tInfo.size + 7) & ~7);
				stackStart += tInfo.size / 4;
			}
			else
			{
				bool opaqueType = tInfo.subCat != ExternTypeInfo::CAT_CLASS || tInfo.memberCount == 0;

				bool firstQwordInteger = opaqueType || HasIntegerMembersInRange(tInfo, 0, 8, exTypes, exTypeExtra);
				bool secondQwordInteger = opaqueType || HasIntegerMembersInRange(tInfo, 8, 16, exTypes, exTypeExtra);

				if(tInfo.size <= 4)
				{
					if(tInfo.size != 0)
					{
						if(firstQwordInteger)
							dcArgInt(dcCallVM, *(int*)stackStart);
						else
							dcArgFloat(dcCallVM, *(float*)stackStart);
					}
					else
					{
						stackStart += 1;
					}
				}
				else if(tInfo.size <= 8)
				{
					if(firstQwordInteger)
						dcArgLongLong(dcCallVM, vmLoadLong(stackStart));
					else
						dcArgDouble(dcCallVM, vmLoadDouble(stackStart));
				}
				else
				{
					int requredIRegs = (firstQwordInteger ? 1 : 0) + (secondQwordInteger ? 1 : 0);

					if(dcFreeIRegs(dcCallVM) < requredIRegs || dcFreeFRegs(dcCallVM) < (2 - requredIRegs))
					{
						dcArgStack(dcCallVM, stackStart, (tInfo.size + 7) & ~7);
					}
					else
					{
						if(firstQwordInteger)
							dcArgLongLong(dcCallVM, vmLoadLong(stackStart));
						else
							dcArgDouble(dcCallVM, vmLoadDouble(stackStart));

						if(secondQwordInteger)
							dcArgLongLong(dcCallVM, vmLoadLong(stackStart + 2));
						else
							dcArgDouble(dcCallVM, vmLoadDouble(stackStart + 2));
					}
				}

				stackStart += tInfo.size / 4;
			}
#else
			if(tInfo.size <= 4)
			{
				// This branch also handles 0 byte structs
				dcArgInt(dcCallVM, *(int*)stackStart);
				stackStart += 1;
			}
			else
			{
				for(unsigned k = 0; k < tInfo.size / 4; k++)
				{
					dcArgInt(dcCallVM, *(int*)stackStart);
					stackStart += 1;
				}
			}
#endif
			break;
		case ExternTypeInfo::TYPE_VOID:
			return;
		case ExternTypeInfo::TYPE_INT:
			dcArgInt(dcCallVM, *(int*)stackStart);
			stackStart += 1;
			break;
		case ExternTypeInfo::TYPE_FLOAT:
			dcArgFloat(dcCallVM, *(float*)stackStart);
			stackStart += 1;
			break;
		case ExternTypeInfo::TYPE_LONG:
			dcArgLongLong(dcCallVM, vmLoadLong(stackStart));
			stackStart += 2;
			break;
		case ExternTypeInfo::TYPE_DOUBLE:
			dcArgDouble(dcCallVM, vmLoadDouble(stackStart));
			stackStart += 2;
			break;
		case ExternTypeInfo::TYPE_SHORT:
			dcArgShort(dcCallVM, *(short*)stackStart);
			stackStart += 1;
			break;
		case ExternTypeInfo::TYPE_CHAR:
			dcArgChar(dcCallVM, *(char*)stackStart);
			stackStart += 1;
			break;
		}
	}

	dcArgPointer(dcCallVM, (DCpointer)vmLoadPointer(stackStart));

	unsigned *newStackPtr = resultStorage;

	switch(retType)
	{
	case ExternFuncInfo::RETURN_VOID:
		dcCallVoid(dcCallVM, fPtr);
		break;
	case ExternFuncInfo::RETURN_INT:
		if(returnType.size == 1)
			*newStackPtr = dcCallChar(dcCallVM, fPtr);
		else if(returnType.size == 2)
			*newStackPtr = dcCallShort(dcCallVM, fPtr);
		else
			*newStackPtr = dcCallInt(dcCallVM, fPtr);
		break;
	case ExternFuncInfo::RETURN_DOUBLE:
		if(func.returnShift == 1)
			vmStoreDouble(newStackPtr, dcCallFloat(dcCallVM, fPtr));
		else
			vmStoreDouble(newStackPtr, dcCallDouble(dcCallVM, fPtr));
		break;
	case ExternFuncInfo::RETURN_LONG:
		vmStoreLong(newStackPtr, dcCallLongLong(dcCallVM, fPtr));
		break;
	case ExternFuncInfo::RETURN_UNKNOWN:
#if defined(_WIN64)
		if(func.returnShift == 1)
		{
			*newStackPtr = dcCallInt(dcCallVM, fPtr);
		}
		else
		{
			dcCallVoid(dcCallVM, fPtr);

			// copy return value on top of the stack
			memcpy(newStackPtr, ret, func.returnShift * 4);
		}
#elif !defined(_M_X64)
		dcCallPointer(dcCallVM, fPtr);

		// copy return value on top of the stack
		memcpy(newStackPtr, ret, func.returnShift * 4);
#elif defined(__aarch64__)
		if(func.returnShift > 4)
		{
			DCcomplexbig res = dcCallComplexBig(dcCallVM, fPtr);

			memcpy(newStackPtr, &res, func.returnShift * 4); // copy return value on top of the stack
		}
		else
		{
			if(!firstQwordInteger && !secondQwordInteger)
			{
				DCcomplexdd res = dcCallComplexDD(dcCallVM, fPtr);

				memcpy(newStackPtr, &res, func.returnShift * 4); // copy return value on top of the stack
			}
			else if(firstQwordInteger && !secondQwordInteger)
			{
				DCcomplexld res = dcCallComplexLD(dcCallVM, fPtr);

				memcpy(newStackPtr, &res, func.returnShift * 4); // copy return value on top of the stack
			}
			else if(!firstQwordInteger && secondQwordInteger)
			{
				DCcomplexdl res = dcCallComplexDL(dcCallVM, fPtr);

				memcpy(newStackPtr, &res, func.returnShift * 4); // copy return value on top of the stack
			}
			else
			{
				DCcomplexll res = dcCallComplexLL(dcCallVM, fPtr);

				memcpy(newStackPtr, &res, func.returnShift * 4); // copy return value on top of the stack
			}
		}
#else
		if(returnByPointer)
		{
			dcCallPointer(dcCallVM, fPtr);

			// copy return value on top of the stack
			memcpy(newStackPtr, ret, func.returnShift * 4);
		}
		else
		{
			if(!firstQwordInteger && !secondQwordInteger)
			{
				DCcomplexdd res = dcCallComplexDD(dcCallVM, fPtr);

				memcpy(newStackPtr, &res, func.returnShift * 4); // copy return value on top of the stack
			}
			else if(firstQwordInteger && !secondQwordInteger)
			{
				DCcomplexld res = dcCallComplexLD(dcCallVM, fPtr);

				memcpy(newStackPtr, &res, func.returnShift * 4); // copy return value on top of the stack
			}
			else if(!firstQwordInteger && secondQwordInteger)
			{
				DCcomplexdl res = dcCallComplexDL(dcCallVM, fPtr);

				memcpy(newStackPtr, &res, func.returnShift * 4); // copy return value on top of the stack
			}
			else
			{
				DCcomplexll res = dcCallComplexLL(dcCallVM, fPtr);

				memcpy(newStackPtr, &res, func.returnShift * 4); // copy return value on top of the stack
			}
		}
#endif
		break;
	}
}
#endif

unsigned GetFunctionVmReturnType(ExternFuncInfo &function, ExternTypeInfo *exTypes, ExternMemberInfo *exTypeExtra)
{
	RegVmReturnType retType = rvrVoid;

	ExternTypeInfo &targetType = exTypes[function.funcType];

	unsigned targetReturnTypeId = exTypeExtra[targetType.memberOffset].type;
	ExternTypeInfo &targetReturnType = exTypes[targetReturnTypeId];

	switch(targetReturnTypeId)
	{
	case NULLC_TYPE_VOID:
		retType = rvrVoid;
		break;
	case NULLC_TYPE_BOOL:
	case NULLC_TYPE_CHAR:
	case NULLC_TYPE_SHORT:
	case NULLC_TYPE_INT:
		retType = rvrInt;
		break;
	case NULLC_TYPE_LONG:
		retType = rvrLong;
		break;
	case NULLC_TYPE_FLOAT:
	case NULLC_TYPE_DOUBLE:
		retType = rvrDouble;
		break;
	case NULLC_TYPE_TYPEID:
	case NULLC_TYPE_FUNCTION:
		retType = rvrInt;
		break;
	case NULLC_TYPE_NULLPTR:
	case NULLC_TYPE_VOID_REF:
		retType = NULLC_PTR_SIZE == 4 ? rvrInt : rvrLong;
		break;
	default:
		if(targetReturnType.subCat == ExternTypeInfo::CAT_POINTER)
			retType = NULLC_PTR_SIZE == 4 ? rvrInt : rvrLong;
		else if(targetReturnType.subCat == ExternTypeInfo::CAT_CLASS && targetReturnType.type == ExternTypeInfo::TYPE_INT)
			retType = rvrInt;
		else
			retType = rvrStruct;
		break;
	}

	return retType;
}

NULLCRef GetExecutorResultObject(unsigned tempStackType, unsigned *tempStackArrayBase)
{
	if(tempStackType == NULLC_TYPE_VOID)
	{
		NULLCRef result = { 0, 0 };

		return result;
	}
	else if(tempStackType == NULLC_TYPE_FLOAT)
	{
		// 'float' is placed as a 'double' value on the stack
		NULLCRef result = { NULLC_TYPE_FLOAT, (char*)nullcAllocateTyped(NULLC_TYPE_FLOAT) };

		double tmpDouble;
		memcpy(&tmpDouble, tempStackArrayBase, sizeof(tmpDouble));

		float tmpFloat = float(tmpDouble);
		memcpy(result.ptr, &tmpFloat, sizeof(tmpFloat));

		return result;
	}
	else if(tempStackType == NULLC_TYPE_AUTO_REF)
	{
		// Return 'auto ref' as is
		NULLCRef result;
		memcpy(&result, tempStackArrayBase, sizeof(result));

		return result;
	}

	NULLCRef tmp = { tempStackType, (char*)tempStackArrayBase };

	return NULLC::CopyObject(tmp);
}

const char* GetExecutorResult(char *execResult, unsigned execResultSize, unsigned tempStackType, unsigned *tempStackArrayBase, char *exSymbols, ExternTypeInfo *exTypes)
{
	switch(tempStackType)
	{
	case NULLC_TYPE_VOID:
		NULLC::SafeSprintf(execResult, execResultSize, "no return value");
		break;
	case NULLC_TYPE_BOOL:
	case NULLC_TYPE_CHAR:
	case NULLC_TYPE_SHORT:
	case NULLC_TYPE_INT:
	{
		int value;
		memcpy(&value, tempStackArrayBase, sizeof(value));
		NULLC::SafeSprintf(execResult, execResultSize, "%d", value);
	}
		break;
	case NULLC_TYPE_LONG:
	{
		long long value;
		memcpy(&value, tempStackArrayBase, sizeof(value));
		NULLC::SafeSprintf(execResult, execResultSize, "%lldL", value);
	}
		break;
	case NULLC_TYPE_FLOAT:
	case NULLC_TYPE_DOUBLE:
	{
		double value;
		memcpy(&value, tempStackArrayBase, sizeof(value));
		NULLC::SafeSprintf(execResult, execResultSize, "%f", value);
	}
		break;
	default:
		NULLC::SafeSprintf(execResult, execResultSize, "complex return value (%s)", exSymbols + exTypes[tempStackType].offsetToName);
		break;
	}

	return execResult;
}

int GetExecutorResultInt(unsigned tempStackType, unsigned *tempStackArrayBase)
{
	int value = 0;

	switch(tempStackType)
	{
	case NULLC_TYPE_BOOL:
	case NULLC_TYPE_CHAR:
	case NULLC_TYPE_SHORT:
	case NULLC_TYPE_INT:
		memcpy(&value, tempStackArrayBase, sizeof(value));
		return value;
	default:
		assert(!"return type is not 'int'");
		break;
	}

	return value;
}

double GetExecutorResultDouble(unsigned tempStackType, unsigned *tempStackArrayBase)
{
	double value = 0.0;

	switch(tempStackType)
	{
	case NULLC_TYPE_FLOAT:
	case NULLC_TYPE_DOUBLE:
		memcpy(&value, tempStackArrayBase, sizeof(value));
		return value;
	default:
		assert(!"return type is not 'double'");
		break;
	}

	return value;
}

long long GetExecutorResultLong(unsigned tempStackType, unsigned *tempStackArrayBase)
{
	long long value = 0;

	switch(tempStackType)
	{
	case NULLC_TYPE_LONG:
		memcpy(&value, tempStackArrayBase, sizeof(value));
		return value;
	default:
		assert(!"return type is not 'long'");
		break;
	}

	return value;
}

int VmIntPow(int power, int number)
{
	if(power < 0)
		return number == 1 ? 1 : (number == -1 ? ((power & 1) ? -1 : 1) : 0);

	int result = 1;
	while(power)
	{
		if(power & 1)
		{
			result *= number;
			power--;
		}
		number *= number;
		power >>= 1;
	}
	return result;
}

long long VmLongPow(long long power, long long number)
{
	if(power < 0)
		return number == 1 ? 1 : (number == -1 ? ((power & 1) ? -1 : 1) : 0);

	long long result = 1;
	while(power)
	{
		if(power & 1)
		{
			result *= number;
			power--;
		}
		number *= number;
		power >>= 1;
	}
	return result;
}
