#include "Linker.h"

#include "StdLib.h"
#include "BinaryCache.h"
#include "DenseMap.h"
#include "InstructionTreeRegVmLowerGraph.h"
#include "Trace.h"
#include "StrAlgo.h"

extern "C"
{
	NULLC_DEBUG_EXPORT uintptr_t nullcModuleBytecodeLocation = 0;
	NULLC_DEBUG_EXPORT uintptr_t nullcModuleBytecodeSize = 0;
	NULLC_DEBUG_EXPORT uintptr_t nullcModuleBytecodeVersion = 0;
}

namespace NULLC
{
	extern bool enableLogFiles;
	extern void* (*lookupMissingFunction)(const char* name);

	template<typename T>
	unsigned GetArrayDataSize(const FastVector<T> &arr)
	{
		return arr.count * sizeof(arr.data[0]);
	}

	template<typename T>
	unsigned WriteArraySizeAndData(unsigned char *target, const FastVector<T> &arr)
	{
		memcpy(target, &arr.count, sizeof(unsigned));
		unsigned size = sizeof(unsigned);
		target += sizeof(unsigned);

		memcpy(target, arr.data, arr.count * sizeof(arr.data[0]));
		size += arr.count * sizeof(arr.data[0]);

		return size;
	}
}

Linker::Linker(): exTypes(128), exTypeExtra(256), exTypeConstants(256), exVariables(128), exFunctions(256), exLocals(1024), exSymbols(8192), regVmJumpTargets(1024)
{
	globalVarSize = 0;

	typeMap.init();
	funcMap.init();

	debugOutputIndent = 0;

	NULLC::SetLinker(this);
}

Linker::~Linker()
{
	CleanCode();
}

void Linker::CleanCode()
{
	exTypes.clear();
	exTypeExtra.clear();
	exTypeConstants.clear();
	exVariables.clear();
	exFunctions.clear();
	exFunctionExplicitTypeArrayOffsets.clear();
	exFunctionExplicitTypes.clear();
	exSymbols.clear();
	exLocals.clear();
	exModules.clear();
	exSource.clear();
	exImportPaths.clear();
	exMainModuleName.clear();

	exRegVmCode.clear();
	exRegVmSourceInfo.clear();
	exRegVmExecCount.clear();
	exRegVmConstants.clear();
	exRegVmRegKillInfo.clear();
	memset(exRegVmInstructionExecCount.data, 0, sizeof(exRegVmInstructionExecCount));

	for(unsigned i = 0; i < expiredRegVmCode.size(); i++)
		NULLC::dealloc(expiredRegVmCode[i]);
	expiredRegVmCode.clear();

	for(unsigned i = 0; i < expiredRegVmConstants.size(); i++)
		NULLC::dealloc(expiredRegVmConstants[i]);
	expiredRegVmConstants.clear();

#ifdef NULLC_LLVM_SUPPORT
	llvmModuleSizes.clear();
	llvmModuleCodes.clear();

	llvmTypeRemapSizes.clear();
	llvmTypeRemapOffsets.clear();
	llvmTypeRemapValues.clear();

	llvmFuncRemapSizes.clear();
	llvmFuncRemapOffsets.clear();
	llvmFuncRemapValues.clear();
#endif

	regVmJumpTargets.clear();

	globalVarSize = 0;

	typeRemap.clear();
	funcRemap.clear();
	moduleRemap.clear();

	funcMap.clear();

	debugOutputIndent = 0;

	NULLC::ClearMemory();
}

bool Linker::LinkCode(const char *code, const char *moduleName, bool rootModule)
{
	TRACE_SCOPE("link", "LinkCode");

	linkError[0] = 0;

#ifdef VERBOSE_DEBUG_OUTPUT
	for(unsigned indent = 0; indent < debugOutputIndent; indent++)
		printf("  ");

	printf("Linking %s\n", moduleName ? moduleName : "(unnamed)");
#endif

	debugOutputIndent++;

	ByteCode *bCode = (ByteCode*)code;

	ExternTypeInfo *tInfo = FindFirstType(bCode), *tStart = tInfo;
	ExternMemberInfo *memberList = FindFirstMember(bCode);
	ExternConstantInfo *constantList = FindFirstConstant(bCode);

	unsigned int moduleFuncCount = 0;

	ExternModuleInfo *mInfo = FindFirstModule(bCode);
	for(unsigned int i = 0; i < bCode->dependsCount; i++)
	{
		const char *path = FindSymbols(bCode) + mInfo->nameOffset;

		//Search for it in loaded modules
		int loadedId = -1;
		for(unsigned int n = 0; n < exModules.size(); n++)
		{
			if(exModules[n].nameHash == NULLC::GetStringHash(path))
			{
				loadedId = n;
				break;
			}
		}

		const unsigned nestedModuleFileNameLength = 1024;
		char nestedModuleFileName[nestedModuleFileNameLength];
		*nestedModuleFileName = 0;

		// Search from parent folder
		if(loadedId == -1 && moduleName)
		{
			if(const char *folderPos = strrchr(moduleName, '/'))
			{
				NULLC::SafeSprintf(nestedModuleFileName, nestedModuleFileNameLength, "%.*s/%s", unsigned(folderPos - moduleName), moduleName, path);

				for(unsigned int n = 0; n < exModules.size(); n++)
				{
					if(exModules[n].nameHash == NULLC::GetStringHash(nestedModuleFileName))
					{
						loadedId = n;
						break;
					}
				}
			}
		}

		if(loadedId == -1)
		{
			const char *bytecode = BinaryCache::FindBytecode(path, false);

			// Search from parent folder
			if(!bytecode && *nestedModuleFileName)
			{
				path = nestedModuleFileName;

				bytecode = BinaryCache::FindBytecode(nestedModuleFileName, false);
			}

			// last module is not imported
			if(strcmp(path, "__last.nc") != 0)
			{
				if(bytecode)
				{
					if(!LinkCode(bytecode, path, false))
					{
						debugOutputIndent--;

						NULLC::SafeSprintf(linkError + strlen(linkError), LINK_ERROR_BUFFER_SIZE - strlen(linkError), "\r\nLink Error: failure to load module %s", path);
						return false;
					}
				}
				else
				{
					debugOutputIndent--;

					NULLC::SafeSprintf(linkError + strlen(linkError), LINK_ERROR_BUFFER_SIZE - strlen(linkError), "\r\nFailure to load module %s", path);
					return false;
				}
			}

			exModules.push_back(*mInfo);
			exModules.back().nameOffset = 0;
			exModules.back().nameHash = NULLC::GetStringHash(path);
			exModules.back().funcStart = exFunctions.size() - mInfo->funcCount;
			exModules.back().variableOffset = globalVarSize - ((ByteCode*)bytecode)->globalVarSize;
			exModules.back().sourceOffset = exSource.size() - ((ByteCode*)bytecode)->sourceSize;
			exModules.back().sourceSize = ((ByteCode*)bytecode)->sourceSize;

#ifdef VERBOSE_DEBUG_OUTPUT
			for(unsigned indent = 0; indent < debugOutputIndent; indent++)
				printf("  ");

			printf("Module %s variables are found at %d (size is %d).\n", path, exModules.back().variableOffset, ((ByteCode*)bytecode)->globalVarSize);
#endif
			loadedId = exModules.size() - 1;
		}

		moduleFuncCount += mInfo->funcCount;
		mInfo++;
	}

#ifdef VERBOSE_DEBUG_OUTPUT
	for(unsigned indent = 0; indent < debugOutputIndent; indent++)
		printf("  ");

	printf("Function remap table is extended to %d functions (%d modules, %d new)\n", bCode->functionCount, moduleFuncCount, bCode->functionCount - moduleFuncCount);
#endif

	funcRemap.resize(bCode->functionCount);
	for(unsigned int i = moduleFuncCount; i < bCode->functionCount; i++)
		funcRemap[i] = (exFunctions.size() ? exFunctions.size() - moduleFuncCount : 0) + i;

	moduleRemap.resize(bCode->dependsCount);

	unsigned int oldFunctionCount = exFunctions.size();
	unsigned int oldSymbolSize = exSymbols.size();
	unsigned int oldTypeCount = exTypes.size();
	unsigned int oldMemberSize = exTypeExtra.size();
	unsigned int oldConstantSize = exTypeConstants.size();

	mInfo = FindFirstModule(bCode);

	// Fixup function table
	for(unsigned int i = 0; i < bCode->dependsCount; i++)
	{
		const char *path = FindSymbols(bCode) + mInfo->nameOffset;

		// Search for it in loaded modules
		int loadedId = -1;

		for(unsigned int n = 0; n < exModules.size(); n++)
		{
			if(exModules[n].nameHash == NULLC::GetStringHash(path))
			{
				loadedId = n;
				break;
			}
		}

		// Search from parent folder
		if(loadedId == -1 && moduleName)
		{
			if(const char *folderPos = strrchr(moduleName, '/'))
			{
				const unsigned nestedModuleFileNameLength = 1024;
				char nestedModuleFileName[nestedModuleFileNameLength];

				NULLC::SafeSprintf(nestedModuleFileName, nestedModuleFileNameLength, "%.*s/%s", unsigned(folderPos - moduleName), moduleName, path);

				for(unsigned int n = 0; n < exModules.size(); n++)
				{
					if(exModules[n].nameHash == NULLC::GetStringHash(nestedModuleFileName))
					{
						loadedId = n;
						break;
					}
				}
			}
		}

		if(loadedId == -1)
		{
			debugOutputIndent--;

			NULLC::SafeSprintf(linkError, LINK_ERROR_BUFFER_SIZE, "Link Error: Failed to find module '%s' after it was linked in", path);
			return false;
		}

		ExternModuleInfo *rInfo = &exModules[loadedId];
		for(unsigned int n = mInfo->funcStart; n < mInfo->funcStart + mInfo->funcCount; n++)
			funcRemap[n] = rInfo->funcStart + n - mInfo->funcStart;
		if(!rInfo->nameOffset)
			rInfo->nameOffset = mInfo->nameOffset + oldSymbolSize;

		moduleRemap[i] = loadedId;

#ifdef VERBOSE_DEBUG_OUTPUT
		for(unsigned indent = 0; indent < debugOutputIndent; indent++)
			printf("  ");

		printf("Module %d (%s) is found at index %d.\n", i, path, loadedId);
#endif

		mInfo++;
	}

	typeRemap.clear();

	// Add new symbols
	exSymbols.resize(oldSymbolSize + bCode->symbolLength);
	memcpy(&exSymbols[oldSymbolSize], FindSymbols(bCode), bCode->symbolLength);
	const char *symbolInfo = FindSymbols(bCode);

	// Create type map for fast searches
	typeMap.clear();
	for(unsigned int i = 0; i < oldTypeCount; i++)
		typeMap.insert(exTypes[i].nameHash, i);

	// Add all types from bytecode to the list
	tInfo = tStart;
	for(unsigned int i = 0; i < bCode->typeCount; i++)
	{
		unsigned int *lastType = typeMap.find(tInfo->nameHash);

		if(!lastType)
		{
			typeRemap.push_back(exTypes.size());
			exTypes.push_back(*tInfo);
			exTypes.back().offsetToName += oldSymbolSize;
			
			if(exTypes.back().subCat == ExternTypeInfo::CAT_ARRAY || exTypes.back().subCat == ExternTypeInfo::CAT_POINTER)
				exTypes.back().subType = typeRemap[exTypes.back().subType];

			if(tInfo->subCat == ExternTypeInfo::CAT_FUNCTION || tInfo->subCat == ExternTypeInfo::CAT_CLASS)
			{
				exTypes.back().memberOffset = exTypeExtra.size();
				exTypeExtra.push_back(memberList + tInfo->memberOffset, tInfo->memberCount + (tInfo->subCat == ExternTypeInfo::CAT_FUNCTION ? 1 : 0));

				// Additional list of members with pointer
				if(tInfo->subCat == ExternTypeInfo::CAT_CLASS && tInfo->pointerCount)
					exTypeExtra.push_back(memberList + tInfo->memberOffset + tInfo->memberCount, tInfo->pointerCount);

				exTypes.back().constantOffset = exTypeConstants.size();
				exTypeConstants.push_back(constantList + tInfo->constantOffset, tInfo->constantCount);
			}
		}
		else
		{
			ExternTypeInfo &lastTypeInfo = exTypes[*lastType];

			if((lastTypeInfo.typeFlags & ExternTypeInfo::TYPE_IS_COMPLETED) == 0 && (tInfo->typeFlags & ExternTypeInfo::TYPE_IS_COMPLETED) != 0)
			{
				assert(tInfo->subCat == ExternTypeInfo::CAT_CLASS);

				lastTypeInfo = *tInfo;
				lastTypeInfo.offsetToName += oldSymbolSize;

				lastTypeInfo.memberOffset = exTypeExtra.size();
				exTypeExtra.push_back(memberList + tInfo->memberOffset, tInfo->memberCount);

				// Additional list of members with pointer
				if(tInfo->pointerCount)
					exTypeExtra.push_back(memberList + tInfo->memberOffset + tInfo->memberCount, tInfo->pointerCount);

				lastTypeInfo.constantOffset = exTypeConstants.size();
				exTypeConstants.push_back(constantList + tInfo->constantOffset, tInfo->constantCount);
			}
			else if(lastTypeInfo.size != tInfo->size && (tInfo->typeFlags & ExternTypeInfo::TYPE_IS_COMPLETED) != 0)
			{
				debugOutputIndent--;

				NULLC::SafeSprintf(linkError, LINK_ERROR_BUFFER_SIZE, "Link Error: type %s is redefined (%s) with a different size (%d != %d)", lastTypeInfo.offsetToName + &exSymbols[0], tInfo->offsetToName + symbolInfo, lastTypeInfo.size, tInfo->size);
				return false;
			}

			typeRemap.push_back(*lastType);
		}

		tInfo++;
	}

	// Remap new derived types
	for(unsigned int i = oldTypeCount; i < exTypes.size(); i++)
	{
		if(exTypes[i].baseType)
			exTypes[i].baseType = typeRemap[exTypes[i].baseType];
	}

	// Remap new member types (while skipping member offsets)
	for(unsigned int i = oldMemberSize; i < exTypeExtra.size(); i++)
		exTypeExtra[i].type = typeRemap[exTypeExtra[i].type];

	// Remap new constant types
	for(unsigned int i = oldConstantSize; i < exTypeConstants.size(); i++)
		exTypeConstants[i].type = typeRemap[exTypeConstants[i].type];

#ifdef VERBOSE_DEBUG_OUTPUT
	for(unsigned indent = 0; indent < debugOutputIndent; indent++)
		printf("  ");

	printf("Global variable size is %d, starting from %d.\n", bCode->globalVarSize, globalVarSize);
#endif

	unsigned int oldGlobalSize = globalVarSize;
	globalVarSize += bCode->globalVarSize;

	// Add all global variables
	ExternVarInfo *vInfo = FindFirstVar(bCode);
	for(unsigned int i = 0; i < bCode->variableCount; i++)
	{
		exVariables.push_back(*vInfo);
		// Type index have to be updated
		exVariables.back().type = typeRemap[vInfo->type];
		exVariables.back().offsetToName += oldSymbolSize;
		exVariables.back().offset += oldGlobalSize;

#ifdef VERBOSE_DEBUG_OUTPUT
		for(unsigned indent = 0; indent < debugOutputIndent; indent++)
			printf("  ");

		printf("Variable %s %s at %d\n", &exSymbols[0] + exTypes[exVariables.back().type].offsetToName, &exSymbols[0] + exVariables.back().offsetToName, exVariables.back().offset);
#endif
		vInfo++;
	}

	// Add new locals
	unsigned int oldLocalsSize = exLocals.size();
	exLocals.resize(oldLocalsSize + bCode->localCount);
	memcpy(exLocals.data + oldLocalsSize, FindFirstLocal(bCode), bCode->localCount * sizeof(ExternLocalInfo));

	// Add new code information
	unsigned int oldRegVmSourceInfoSize = exRegVmSourceInfo.size();
	exRegVmSourceInfo.resize(oldRegVmSourceInfoSize + bCode->regVmInfoSize);
	memcpy(exRegVmSourceInfo.data + oldRegVmSourceInfoSize, FindRegVmSourceInfo(bCode), bCode->regVmInfoSize * sizeof(ExternSourceInfo));

	// Add new source code
	unsigned int oldSourceSize = exSource.size();
	exSource.resize(oldSourceSize + bCode->sourceSize);
	memcpy(exSource.data + oldSourceSize, FindSource(bCode), bCode->sourceSize);

	// Add new code
	assert(exRegVmCode.size() == exRegVmExecCount.size());

	unsigned int oldRegVmCodeSize = exRegVmCode.size();

	// Code might be executing at the moment
	RegVmCmd *oldRegVmCode = NULL;
	unsigned newRegVmCodeSize = oldRegVmCodeSize + bCode->regVmCodeSize + 1;

	if(exRegVmCode.data && oldRegVmCodeSize + bCode->regVmCodeSize + 1 >= exRegVmCode.max)
	{
		if(exRegVmCode.max + (exRegVmCode.max >> 1) > newRegVmCodeSize)
			newRegVmCodeSize = exRegVmCode.max + (exRegVmCode.max >> 1);

		oldRegVmCode = exRegVmCode.data;
		expiredRegVmCode.push_back(oldRegVmCode);

		exRegVmCode.data = NULL;
		exRegVmCode.count = 0;
		exRegVmCode.max = 0;
	}

	exRegVmCode.reserve(newRegVmCodeSize);
	exRegVmCode.resize(oldRegVmCodeSize + bCode->regVmCodeSize);
	memcpy(exRegVmCode.data + oldRegVmCodeSize, FindRegVmCode(bCode), bCode->regVmCodeSize * sizeof(RegVmCmd));

	if(oldRegVmCode)
		memcpy(exRegVmCode.data, oldRegVmCode, oldRegVmCodeSize * sizeof(RegVmCmd));

	exRegVmExecCount.reserve(oldRegVmCodeSize + bCode->regVmCodeSize + 1);
	exRegVmExecCount.resize(oldRegVmCodeSize + bCode->regVmCodeSize);
	memset(exRegVmExecCount.data + oldRegVmCodeSize, 0, bCode->regVmCodeSize * sizeof(unsigned));

	for(unsigned int i = oldRegVmSourceInfoSize; i < exRegVmSourceInfo.size(); i++)
	{
		ExternSourceInfo &sourceInfo = exRegVmSourceInfo[i];

		sourceInfo.instruction += oldRegVmCodeSize;

		if(sourceInfo.definitionModule)
			sourceInfo.sourceOffset += exModules[moduleRemap[sourceInfo.definitionModule - 1]].sourceOffset;
		else
			sourceInfo.sourceOffset += oldSourceSize;
	}

	unsigned int oldRegVmConstantsSize = exRegVmConstants.size();

	// Code might be executing at the moment
	unsigned *oldVmConstants = NULL;
	unsigned newRegVmConstantsSize = oldRegVmConstantsSize + bCode->regVmConstantCount;

	if(exRegVmConstants.data && oldRegVmConstantsSize + bCode->regVmConstantCount >= exRegVmConstants.max)
	{
		if(exRegVmConstants.max + (exRegVmConstants.max >> 1) > newRegVmConstantsSize)
			newRegVmConstantsSize = exRegVmConstants.max + (exRegVmConstants.max >> 1);

		oldVmConstants = exRegVmConstants.data;
		expiredRegVmConstants.push_back(oldVmConstants);

		exRegVmConstants.data = NULL;
		exRegVmConstants.count = 0;
		exRegVmConstants.max = 0;
	}

	exRegVmConstants.reserve(newRegVmConstantsSize);
	exRegVmConstants.resize(oldRegVmConstantsSize + bCode->regVmConstantCount);
	memcpy(exRegVmConstants.data + oldRegVmConstantsSize, FindRegVmConstants(bCode), bCode->regVmConstantCount * sizeof(exRegVmConstants[0]));

	if(oldVmConstants)
		memcpy(exRegVmConstants.data, oldVmConstants, oldRegVmConstantsSize * sizeof(unsigned));

	unsigned int oldRegVmRegKillInfoSize = exRegVmRegKillInfo.size();
	exRegVmRegKillInfo.resize(oldRegVmRegKillInfoSize + bCode->regVmRegKillInfoCount);
	memcpy(exRegVmRegKillInfo.data + oldRegVmRegKillInfoSize, FindRegVmRegKillInfo(bCode), bCode->regVmRegKillInfoCount * sizeof(exRegVmRegKillInfo[0]));

	debugOutputIndent--;

	// Add new functions
	ExternVarInfo *explicitInfo = FindFirstVar(bCode) + bCode->variableCount;

	ExternFuncInfo *fInfo = FindFirstFunc(bCode);

	for(unsigned i = 0; i < bCode->functionCount - bCode->moduleFunctionCount; i++, fInfo++)
	{
		const unsigned int index_none = ~0u;

		unsigned int index = index_none;

		ExternVarInfo *explicitInfoStart = explicitInfo;

		if(fInfo->isVisible)
		{
			unsigned int remappedType = typeRemap[fInfo->funcType];
			HashMap<unsigned int>::Node *curr = funcMap.first(fInfo->nameHash);
			while(curr)
			{
				ExternFuncInfo &prev = exFunctions[curr->value];

				if(curr->value < oldFunctionCount && prev.funcType == remappedType && prev.explicitTypeCount == fInfo->explicitTypeCount)
				{
					bool explicitTypeMatch = true;

					for(unsigned k = 0; k < fInfo->explicitTypeCount; k++)
					{
						ExternTypeInfo &prevType = exTypes[exFunctionExplicitTypes[exFunctionExplicitTypeArrayOffsets[curr->value] + k]];
						ExternTypeInfo &type = exTypes[typeRemap[explicitInfoStart[k].type]];

						if(&prevType != &type)
							explicitTypeMatch = false;
					}

					if(explicitTypeMatch)
					{
						index = curr->value;
						break;
					}
				}
				curr = funcMap.next(curr);
			}
		}

		explicitInfo += fInfo->explicitTypeCount;

		// There is no conflict between internal funcitons
		if(*(symbolInfo + fInfo->offsetToName) == '$')
			index = index_none;

		// If the function exists, check if redefinition is allowed
		if(index != index_none)
		{
			// It is allowed for generic base function and generic function instances
			if(fInfo->isGenericInstance || fInfo->funcType == 0)
			{
				exFunctions.push_back(exFunctions[index]);
				funcMap.insert(exFunctions.back().nameHash, exFunctions.size()-1);

				exFunctionExplicitTypeArrayOffsets.push_back(exFunctionExplicitTypes.size());

				for(unsigned k = 0; k < fInfo->explicitTypeCount; k++)
					exFunctionExplicitTypes.push_back(typeRemap[explicitInfoStart[k].type]);

#ifdef LINK_VERBOSE_DEBUG_OUTPUT
				printf("Rebind function %3d %-20s (to address %4d [external %p] function %3d)\n", exFunctions.size() - 1, &exSymbols[0] + exFunctions.back().offsetToName, exFunctions.back().address, exFunctions.back().funcPtr, index);
#endif
				continue;
			}else{
				NULLC::SafeSprintf(linkError, LINK_ERROR_BUFFER_SIZE, "Link Error: function '%s' is redefined", symbolInfo + fInfo->offsetToName);
				// Try to find module where previous definition was found
				for(unsigned k = 0; k < exModules.size(); k++)
				{
					if(exModules[k].funcStart >= index && index < exModules[k].funcStart + exModules[k].funcCount)
						NULLC::SafeSprintf(linkError, LINK_ERROR_BUFFER_SIZE, "Link Error: redefinition of module %s function '%s'", symbolInfo + exModules[k].nameOffset, symbolInfo + fInfo->offsetToName);
				}
				return false;
			}
		}
		if(index == index_none)
		{
			exFunctions.push_back(*fInfo);
			funcMap.insert(exFunctions.back().nameHash, exFunctions.size()-1);

			exFunctionExplicitTypeArrayOffsets.push_back(exFunctionExplicitTypes.size());

			for(unsigned k = 0; k < fInfo->explicitTypeCount; k++)
				exFunctionExplicitTypes.push_back(typeRemap[explicitInfoStart[k].type]);

			if(exFunctions.back().regVmAddress == 0)
			{
				if(NULLC::lookupMissingFunction)
					exFunctions.back().funcPtrRaw = (void (*)())NULLC::lookupMissingFunction(FindSymbols(bCode) + exFunctions.back().offsetToName);

				if(exFunctions.back().funcPtrRaw || exFunctions.back().funcPtrWrap)
				{
					exFunctions.back().regVmAddress = ~0u;
				}
				else
				{
					NULLC::SafeSprintf(linkError, LINK_ERROR_BUFFER_SIZE, "Link Error: External function '%s' '%s' doesn't have implementation", FindSymbols(bCode) + exFunctions.back().offsetToName, &exSymbols[0] + exTypes[typeRemap[exFunctions.back().funcType]].offsetToName);
					return false;
				}
			}

			// For function prototypes
			if(exFunctions.back().regVmCodeSize & 0x80000000)
			{
				// fix remapping table so that this function index will point to target function index
				funcRemap.data[moduleFuncCount + i] = funcRemap[exFunctions.back().regVmCodeSize & ~0x80000000];

				exFunctions.back().regVmCodeSize = 0;
			}

			// Move based pointer to the new section of symbol information
			exFunctions.back().offsetToName += oldSymbolSize;
			exFunctions.back().offsetToFirstLocal += oldLocalsSize;
			exFunctions.back().funcType = typeRemap[exFunctions.back().funcType];

			if(exFunctions.back().parentType != ~0u)
				exFunctions.back().parentType = typeRemap[exFunctions.back().parentType];
			if(exFunctions.back().contextType != ~0u)
				exFunctions.back().contextType = typeRemap[exFunctions.back().contextType];

			// Update internal function address
			if(exFunctions.back().regVmAddress != -1)
			{
				exFunctions.back().regVmAddress = oldRegVmCodeSize + fInfo->regVmAddress;

				regVmJumpTargets.push_back(exFunctions.back().regVmAddress);
			}

#ifdef LINK_VERBOSE_DEBUG_OUTPUT
			printf("Adding function %3d %-20s (at address %4d [external %p])\n", exFunctions.size() - 1, &exSymbols[0] + exFunctions.back().offsetToName, exFunctions.back().address, exFunctions.back().funcPtr);
#endif
		}
	}

	for(unsigned int i = oldLocalsSize; i < oldLocalsSize + bCode->localCount; i++)
	{
		exLocals[i].type = typeRemap[exLocals[i].type];
		exLocals[i].offsetToName += oldSymbolSize;
	}

	assert((fInfo = FindFirstFunc(bCode)) != NULL); // this is fine, we need this assignment only in debug configuration

	// Fix register VM command arguments
	unsigned pos = oldRegVmCodeSize;
	while(pos < exRegVmCode.size())
	{
		RegVmCmd &cmd = exRegVmCode[pos];
		pos++;
		switch(cmd.code)
		{
		case rviLoadByte:
		case rviLoadWord:
		case rviLoadDword:
		case rviLoadLong:
		case rviLoadFloat:
		case rviLoadDouble:
		case rviStoreByte:
		case rviStoreWord:
		case rviStoreDword:
		case rviStoreLong:
		case rviStoreFloat:
		case rviStoreDouble:
		case rviGetAddr:
		case rviAdd:
		case rviSub:
		case rviMul:
		case rviDiv:
		case rviPow:
		case rviMod:
		case rviLess:
		case rviGreater:
		case rviLequal:
		case rviGequal:
		case rviEqual:
		case rviNequal:
		case rviShl:
		case rviShr:
		case rviBitAnd:
		case rviBitOr:
		case rviBitXor:
		case rviAddl:
		case rviSubl:
		case rviMull:
		case rviDivl:
		case rviPowl:
		case rviModl:
		case rviLessl:
		case rviGreaterl:
		case rviLequall:
		case rviGequall:
		case rviEquall:
		case rviNequall:
		case rviShll:
		case rviShrl:
		case rviBitAndl:
		case rviBitOrl:
		case rviBitXorl:
		case rviAddd:
		case rviSubd:
		case rviMuld:
		case rviDivd:
		case rviAddf:
		case rviSubf:
		case rviMulf:
		case rviDivf:
		case rviPowd:
		case rviModd:
		case rviLessd:
		case rviGreaterd:
		case rviLequald:
		case rviGequald:
		case rviEquald:
		case rviNequald:
			if(cmd.rC == rvrrGlobals)
			{
				if(cmd.argument >> 24)
					cmd.argument = (cmd.argument & 0x00ffffff) + exModules[moduleRemap[(cmd.argument >> 24) - 1]].variableOffset;
				else
					cmd.argument += oldGlobalSize;
			}
			else if(cmd.rC == rvrrConstants)
			{
				cmd.argument += oldRegVmConstantsSize * 4;
			}
			break;
		case rviJmp:
		case rviJmpz:
		case rviJmpnz:
			cmd.argument += oldRegVmCodeSize;
			regVmJumpTargets.push_back(cmd.argument);
			break;
		case rviCall:
		{
			unsigned microcode = (cmd.rA << 16) | (cmd.rB << 8) | cmd.rC;

			microcode += oldRegVmConstantsSize;

			cmd.rA = (microcode >> 16) & 0xff;
			cmd.rB = (microcode >> 8) & 0xff;
			cmd.rC = microcode & 0xff;

			assert(!(cmd.argument != funcRemap[cmd.argument] && int(cmd.argument - bCode->moduleFunctionCount) >= 0) || (fInfo[cmd.argument - bCode->moduleFunctionCount].nameHash == exFunctions[funcRemap[cmd.argument]].nameHash));

			cmd.argument = funcRemap[cmd.argument];

			FixupCallMicrocode(microcode, oldGlobalSize);
		}
			break;
		case rviCallPtr:
			cmd.argument += oldRegVmConstantsSize;

			FixupCallMicrocode(cmd.argument, oldGlobalSize);
			break;
		case rviReturn:
			cmd.argument += oldRegVmConstantsSize;

			exRegVmConstants[cmd.argument] = typeRemap[exRegVmConstants[cmd.argument]];
			break;
		case rviConvertPtr:
			cmd.argument = typeRemap[cmd.argument];
			break;
		case rviFuncAddr:
			cmd.code = rviLoadImm;
			cmd.argument = funcRemap[cmd.argument];
			break;
		case rviTypeid:
			cmd.code = rviLoadImm;
			cmd.argument = typeRemap[cmd.argument];
			break;
		}
	}

	{
		exImportPaths.clear();

		unsigned pos = 0;
		while(const char* path = BinaryCache::EnumImportPath(pos++))
		{
			exImportPaths.push_back(path, (unsigned)strlen(path));
			exImportPaths.push_back(';');
		}
	}

	if(rootModule && moduleName)
	{
		exMainModuleName.clear();
		exMainModuleName.push_back(moduleName, (unsigned)strlen(moduleName));
	}

#ifdef NULLC_LLVM_SUPPORT
	unsigned llvmOldSize = llvmModuleCodes.size();
	llvmModuleSizes.push_back(bCode->llvmSize);
	llvmModuleCodes.resize(llvmModuleCodes.size() + llvmModuleSizes.back());
	memcpy(&llvmModuleCodes[llvmOldSize], ((char*)bCode) + bCode->llvmOffset, bCode->llvmSize);

	llvmTypeRemapSizes.push_back(typeRemap.size());
	llvmTypeRemapOffsets.push_back(llvmTypeRemapValues.size());
	llvmTypeRemapValues.resize(llvmTypeRemapValues.size() + typeRemap.size());
	memcpy(&llvmTypeRemapValues[llvmTypeRemapOffsets.back()], &typeRemap[0], typeRemap.size() * sizeof(typeRemap[0]));

	llvmFuncRemapSizes.push_back(funcRemap.size());
	llvmFuncRemapOffsets.push_back(llvmFuncRemapValues.size());
	llvmFuncRemapValues.resize(llvmFuncRemapValues.size() + funcRemap.size());
	memcpy(&llvmFuncRemapValues[llvmFuncRemapOffsets.back()], &funcRemap[0], funcRemap.size() * sizeof(funcRemap[0]));
#endif

#ifdef VERBOSE_DEBUG_OUTPUT
	if(rootModule)
	{
		unsigned int size = 0;
		printf("Data managed by linker.\n");
		printf("Types: %ub, ", exTypes.size() * (unsigned)sizeof(ExternTypeInfo));
		size += exTypes.size() * sizeof(ExternTypeInfo);
		printf("Variables: %ub, ", exVariables.size() * (unsigned)sizeof(ExternVarInfo));
		size += exVariables.size() * sizeof(ExternVarInfo);
		printf("Functions: %ub, ", exFunctions.size() * (unsigned)sizeof(ExternFuncInfo));
		size += exFunctions.size() * sizeof(ExternFuncInfo);
		printf("Function explicit type array offsets: %ub, ", exFunctionExplicitTypeArrayOffsets.size() * (unsigned)sizeof(unsigned));
		size += exFunctionExplicitTypeArrayOffsets.size() * sizeof(unsigned);
		printf("Function explicit types: %ub, ", exFunctionExplicitTypes.size() * (unsigned)sizeof(unsigned));
		size += exFunctionExplicitTypes.size() * sizeof(unsigned);
		printf("Reg VM Code: %ub\n", exRegVmCode.size() * (unsigned)sizeof(RegVmCmd));
		size += exRegVmCode.size() * sizeof(RegVmCmd);
		printf("Symbols: %ub, ", exSymbols.size() * (unsigned)sizeof(char));
		size += exSymbols.size() * sizeof(char);
		printf("Locals: %ub, ", exLocals.size() * (unsigned)sizeof(ExternLocalInfo));
		size += exLocals.size() * sizeof(ExternLocalInfo);
		printf("Modules: %ub, ", exModules.size() * (unsigned)sizeof(ExternModuleInfo));
		size += exModules.size() * sizeof(ExternModuleInfo);
		printf("Source info: %ub, ", exRegVmSourceInfo.size() * (unsigned)sizeof(ExternSourceInfo));
		size += exRegVmSourceInfo.size() * sizeof(ExternSourceInfo);
		printf("Source: %ub\n", exSource.size() * (unsigned)sizeof(char));
		size += exSource.size() * sizeof(char);
		printf("Overall: %u bytes\n\n", size);
	}
#endif

	return true;
}

bool Linker::SaveRegVmListing(OutputContext &output, bool withProfileInfo)
{
	TRACE_SCOPE("link", "SaveRegVmListing");

	unsigned line = 0, lastLine = ~0u;

	unsigned long long total = 0;

	if(withProfileInfo)
	{
		for(unsigned i = 0; i < 256; i++)
			total += exRegVmInstructionExecCount[i];
	}

	ExternSourceInfo *info = (ExternSourceInfo*)exRegVmSourceInfo.data;
	unsigned infoSize = exRegVmSourceInfo.size();

	SmallDenseSet<unsigned, SmallDenseMapUnsignedHasher, 32> regVmJumpTargetSet;

	for(unsigned i = 0; i < regVmJumpTargets.size(); i++)
		regVmJumpTargetSet.insert(regVmJumpTargets[i]);

	SmallDenseMap<unsigned, ExternFuncInfo*, SmallDenseMapUnsignedHasher, 32> regVmFunctionMap;

	for(unsigned i = 0; i < exFunctions.size(); i++)
		regVmFunctionMap.insert(exFunctions[i].regVmAddress, &exFunctions[i]);

	const char *lastSourcePos = exSource.data;
	const char *lastCodeStart = NULL;

	for(unsigned i = 0; infoSize && i < exRegVmCode.size(); i++)
	{
		while((line < infoSize - 1) && (i >= info[line + 1].instruction))
			line++;

		if(line != lastLine)
		{
			lastLine = line;

			const char *codeStart = exSource.data + info[line].sourceOffset;

			// Find beginning of the line
			while(codeStart != exSource.data && *(codeStart-1) != '\n')
				codeStart--;

			// Skip whitespace
			while(*codeStart == ' ' || *codeStart == '\t')
				codeStart++;

			const char *codeEnd = codeStart;
			while(*codeEnd != '\0' && *codeEnd != '\r' && *codeEnd != '\n')
				codeEnd++;

			if(codeEnd > lastSourcePos)
			{
				output.Printf("%.*s\r\n", int(codeEnd - lastSourcePos), lastSourcePos);
				lastSourcePos = codeEnd;
			}
			else
			{
				if(codeStart != lastCodeStart)
					output.Printf("%.*s\r\n", int(codeEnd - codeStart), codeStart);

				lastCodeStart = codeStart;
			}
		}

		RegVmCmd cmd = exRegVmCode[i];

		bool found = regVmJumpTargetSet.contains(i);

		if(ExternFuncInfo **func = regVmFunctionMap.find(i))
			output.Printf("// %s#%d\n", exSymbols.data + (*func)->offsetToName, unsigned(*func - exFunctions.data));

		if(withProfileInfo)
		{
			if(found)
			{
				output.Printf("//            %4d:\n", i);

				double percent = double(exRegVmExecCount[i]) / total * 100.0;

				if(percent > 0.1)
					output.Printf("// (%8d %4.1f)      ", exRegVmExecCount[i], percent);
				else
					output.Printf("// (%8d     )      ", exRegVmExecCount[i]);
			}
			else
			{
				double percent = double(exRegVmExecCount[i]) / total * 100.0;

				if(percent > 0.1)
					output.Printf("// (%8d %4.1f) %4d ", exRegVmExecCount[i], percent, i);
				else
					output.Printf("// (%8d     ) %4d ", exRegVmExecCount[i], i);
			}
		}
		else
		{
			if(found)
			{
				output.Printf("// %4d:\n", i);
				output.Printf("//      ");
			}
			else
			{
				output.Printf("// %4d ", i);
			}
		}

		PrintInstruction(output, (char*)exRegVmConstants.data, exFunctions.data, exSymbols.data, RegVmInstructionCode(cmd.code), cmd.rA, cmd.rB, cmd.rC, cmd.argument, NULL);

		if(cmd.code == rviCall || cmd.code == rviFuncAddr)
			output.Printf(" (%s)", exSymbols.data + exFunctions[exRegVmCode[i].argument].offsetToName);
		else if(cmd.code == rviConvertPtr)
			output.Printf(" (%s)", exSymbols.data + exTypes[exRegVmCode[i].argument].offsetToName);

		output.Printf("\n");
	}

	if(withProfileInfo)
	{
		output.Printf("\n");

		for(unsigned i = 0; i < 256; i++)
		{
			if(unsigned count = exRegVmInstructionExecCount[i])
				output.Printf("// %9s: %10d (%4.1f%%)\n", GetInstructionName(RegVmInstructionCode(i)), count, float(count) / total * 100.0);
		}

		output.Printf("// %9s: %10lld (%4.0f%%)\n", "total", total, 100.0);
	}

	output.Flush();

	return true;
}

void Linker::CollectDebugInfo(FastVector<unsigned char*> *instAddress)
{
	nullcModuleBytecodeSize = 0;

	nullcModuleBytecodeSize += sizeof(unsigned) + NULLC::GetArrayDataSize(exTypes);
	nullcModuleBytecodeSize += sizeof(unsigned) + NULLC::GetArrayDataSize(exTypeExtra);
	nullcModuleBytecodeSize += sizeof(unsigned) + NULLC::GetArrayDataSize(exTypeConstants);
	nullcModuleBytecodeSize += sizeof(unsigned) + NULLC::GetArrayDataSize(exVariables);
	nullcModuleBytecodeSize += sizeof(unsigned) + NULLC::GetArrayDataSize(exFunctions);
	nullcModuleBytecodeSize += sizeof(unsigned) + NULLC::GetArrayDataSize(exFunctionExplicitTypeArrayOffsets);
	nullcModuleBytecodeSize += sizeof(unsigned) + NULLC::GetArrayDataSize(exFunctionExplicitTypes);
	nullcModuleBytecodeSize += sizeof(unsigned) + NULLC::GetArrayDataSize(exLocals);
	nullcModuleBytecodeSize += sizeof(unsigned) + NULLC::GetArrayDataSize(exModules);
	nullcModuleBytecodeSize += sizeof(unsigned) + NULLC::GetArrayDataSize(exSymbols);
	nullcModuleBytecodeSize += sizeof(unsigned) + NULLC::GetArrayDataSize(exSource);
	nullcModuleBytecodeSize += sizeof(unsigned) + NULLC::GetArrayDataSize(exImportPaths);
	nullcModuleBytecodeSize += sizeof(unsigned) + NULLC::GetArrayDataSize(exMainModuleName);
	nullcModuleBytecodeSize += sizeof(unsigned) + NULLC::GetArrayDataSize(exRegVmCode);
	nullcModuleBytecodeSize += sizeof(unsigned) + NULLC::GetArrayDataSize(exRegVmSourceInfo);
	nullcModuleBytecodeSize += sizeof(unsigned) + NULLC::GetArrayDataSize(exRegVmConstants);

	if(instAddress)
		nullcModuleBytecodeSize += sizeof(unsigned) + NULLC::GetArrayDataSize(*instAddress);
	else
		nullcModuleBytecodeSize += sizeof(unsigned);

	nullcModuleBytecodeSize += sizeof(unsigned); // exLinker->globalVarSize

	fullLinkerData.resize((unsigned)nullcModuleBytecodeSize);

	unsigned char *pos = fullLinkerData.data;

	pos += NULLC::WriteArraySizeAndData(pos, exTypes);
	pos += NULLC::WriteArraySizeAndData(pos, exTypeExtra);
	pos += NULLC::WriteArraySizeAndData(pos, exTypeConstants);
	pos += NULLC::WriteArraySizeAndData(pos, exVariables);
	pos += NULLC::WriteArraySizeAndData(pos, exFunctions);
	pos += NULLC::WriteArraySizeAndData(pos, exFunctionExplicitTypeArrayOffsets);
	pos += NULLC::WriteArraySizeAndData(pos, exFunctionExplicitTypes);
	pos += NULLC::WriteArraySizeAndData(pos, exLocals);
	pos += NULLC::WriteArraySizeAndData(pos, exModules);
	pos += NULLC::WriteArraySizeAndData(pos, exSymbols);
	pos += NULLC::WriteArraySizeAndData(pos, exSource);
	pos += NULLC::WriteArraySizeAndData(pos, exImportPaths);
	pos += NULLC::WriteArraySizeAndData(pos, exMainModuleName);
	pos += NULLC::WriteArraySizeAndData(pos, exRegVmCode);
	pos += NULLC::WriteArraySizeAndData(pos, exRegVmSourceInfo);
	pos += NULLC::WriteArraySizeAndData(pos, exRegVmConstants);

	if(instAddress)
	{
		pos += NULLC::WriteArraySizeAndData(pos, *instAddress);
	}
	else
	{
		unsigned zero = 0;
		memcpy(pos, &zero, sizeof(zero));
		pos += sizeof(zero);
	}


	memcpy(pos, &globalVarSize, sizeof(globalVarSize));
	pos += sizeof(globalVarSize);

	nullcModuleBytecodeLocation = uintptr_t(fullLinkerData.data);

	nullcModuleBytecodeVersion += 1;
}

const char*	Linker::GetLinkError()
{
	return linkError;
}

void Linker::FixupCallMicrocode(unsigned microcode, unsigned oldGlobalSize)
{
	while(exRegVmConstants[microcode] != rvmiCall)
	{
		switch(exRegVmConstants[microcode++])
		{
		case rvmiPush:
			microcode++;
			break;
		case rvmiPushQword:
			microcode++;
			break;
		case rvmiPushImm:
			microcode++;
			break;
		case rvmiPushImmq:
			microcode++;
			break;
		case rvmiPushMem:
			if(exRegVmConstants[microcode] == rvrrGlobals)
			{
				unsigned &offset = exRegVmConstants[microcode + 1];

				if(offset >> 24)
					offset = (offset & 0x00ffffff) + exModules[moduleRemap[(offset >> 24) - 1]].variableOffset;
				else
					offset += oldGlobalSize;
			}

			microcode += 3;
			break;
		}
	}

	microcode += 3;

	while(exRegVmConstants[microcode] != rvmiReturn)
	{
		switch(exRegVmConstants[microcode++])
		{
		case rvmiPop:
		case rvmiPopq:
			microcode++;
			break;
		case rvmiPopMem:
			if(exRegVmConstants[microcode] == rvrrGlobals)
			{
				unsigned &offset = exRegVmConstants[microcode + 1];

				if(offset >> 24)
					offset = (offset & 0x00ffffff) + exModules[moduleRemap[(offset >> 24) - 1]].variableOffset;
				else
					offset += oldGlobalSize;
			}

			microcode += 3;
			break;
		}
	}
}

