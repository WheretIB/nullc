#include "Linker.h"

#include "StdLib.h"
#include "BinaryCache.h"
#include "InstructionSet.h"
#include "InstructionTreeRegVmLowerGraph.h"

#ifdef NULLC_AUTOBINDING
	#if defined(__linux)
		#include <dlfcn.h>
	#else
		#define WIN32_LEAN_AND_MEAN
		#include <windows.h>
	#endif
#endif

namespace NULLC
{
	extern bool enableLogFiles;
}

Linker::Linker(): exTypes(128), exTypeExtra(256), exVariables(128), exFunctions(256), exLocals(1024), exSymbols(8192), vmJumpTargets(1024), regVmJumpTargets(1024)
{
	globalVarSize = 0;

	vmOffsetToGlobalCode = 0;

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
	exVariables.clear();
	exFunctions.clear();
	exFunctionExplicitTypeArrayOffsets.clear();
	exFunctionExplicitTypes.clear();
	exSymbols.clear();
	exLocals.clear();
	exModules.clear();
	exSource.clear();
	exDependencies.clear();

	exVmCode.clear();
	exVmSourceInfo.clear();

	exRegVmCode.clear();
	exRegVmSourceInfo.clear();
	exRegVmExecCount.clear();
	exRegVmConstants.clear();
	memset(exRegVmInstructionExecCount.data, 0, sizeof(exRegVmInstructionExecCount));

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

	vmJumpTargets.clear();

	vmOffsetToGlobalCode = 0;

	regVmJumpTargets.clear();

	globalVarSize = 0;

	typeRemap.clear();
	funcRemap.clear();
	moduleRemap.clear();

	funcMap.clear();

	debugOutputIndent = 0;

	NULLC::ClearMemory();
}

bool Linker::LinkCode(const char *code, const char *moduleName)
{
	(void)moduleName;

	linkError[0] = 0;

	unsigned dependeciesBase = exDependencies.size();

#ifdef VERBOSE_DEBUG_OUTPUT
	for(unsigned indent = 0; indent < debugOutputIndent; indent++)
		printf("  ");

	printf("Linking %s (dependencies base %d).\r\n", moduleName, dependeciesBase);
#endif

	debugOutputIndent++;

	ByteCode *bCode = (ByteCode*)code;

	ExternTypeInfo *tInfo = FindFirstType(bCode), *tStart = tInfo;
	ExternMemberInfo *memberList = FindFirstMember(bCode);

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
		if(loadedId == -1)
		{
			const char *bytecode = BinaryCache::FindBytecode(path, false);

			unsigned dependencySlot = exDependencies.size();
			exDependencies.push_back(~0u);

			// last module is not imported
			if(strcmp(path, "__last.nc") != 0)
			{
				if(bytecode)
				{
					if(!LinkCode(bytecode, path))
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

#ifdef VERBOSE_DEBUG_OUTPUT
			for(unsigned indent = 0; indent < debugOutputIndent; indent++)
				printf("  ");

			printf("Linking dependency %d to module %d (%s) (%d dependencies).\r\n", dependencySlot, exModules.size(), path, exDependencies.size() - dependencySlot);
#endif

			exDependencies[dependencySlot] = exModules.size();

			exModules.push_back(*mInfo);
			exModules.back().nameOffset = 0;
			exModules.back().nameHash = NULLC::GetStringHash(path);
			exModules.back().funcStart = exFunctions.size() - mInfo->funcCount;
			exModules.back().variableOffset = globalVarSize - ((ByteCode*)bytecode)->globalVarSize;
			exModules.back().sourceOffset = exSource.size() - ((ByteCode*)bytecode)->sourceSize;
			exModules.back().sourceSize = ((ByteCode*)bytecode)->sourceSize;

			exModules.back().dependencyStart = dependencySlot;
			exModules.back().dependencyCount = exDependencies.size() - dependencySlot;

#ifdef VERBOSE_DEBUG_OUTPUT
			printf("Module %s variables are found at %d (size is %d).\r\n", path, exModules.back().variableOffset, ((ByteCode*)bytecode)->globalVarSize);
#endif
			loadedId = exModules.size() - 1;
		}
		else
		{
			ExternModuleInfo &prevData = exModules[loadedId];

			for(unsigned k = 0; k < prevData.dependencyCount; k++)
			{
				unsigned targetModuleIndex = exDependencies[prevData.dependencyStart + k];

#ifdef VERBOSE_DEBUG_OUTPUT
				for(unsigned indent = 0; indent < debugOutputIndent; indent++)
					printf("  ");

				printf("Linking dependency %d to module %d (%s) (%d dependencies) [skip].\r\n", exDependencies.size(), targetModuleIndex, exSymbols.data + exModules[targetModuleIndex].nameOffset, exModules[targetModuleIndex].dependencyCount);
#endif

				exDependencies.push_back(targetModuleIndex);
			}
		}

		moduleFuncCount += mInfo->funcCount;
		mInfo++;
	}

#ifdef LINK_VERBOSE_DEBUG_OUTPUT
		printf("Function remap table is extended to %d functions (%d modules, %d new)\r\n", bCode->functionCount, moduleFuncCount, bCode->functionCount - moduleFuncCount);
#endif
	funcRemap.resize(bCode->functionCount);
	for(unsigned int i = moduleFuncCount; i < bCode->functionCount; i++)
		funcRemap[i] = (exFunctions.size() ? exFunctions.size() - moduleFuncCount : 0) + i;

	moduleRemap.resize(bCode->dependsCount);

	unsigned int oldFunctionCount = exFunctions.size();
	unsigned int oldSymbolSize = exSymbols.size();
	unsigned int oldTypeCount = exTypes.size();
	unsigned int oldMemberSize = exTypeExtra.size();

	mInfo = FindFirstModule(bCode);
	// Fixup function table
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
		ExternModuleInfo *rInfo = &exModules[loadedId];
		for(unsigned int n = mInfo->funcStart; n < mInfo->funcStart + mInfo->funcCount; n++)
			funcRemap[n] = rInfo->funcStart + n - mInfo->funcStart;
		if(!rInfo->nameOffset)
			rInfo->nameOffset = mInfo->nameOffset + oldSymbolSize;

		moduleRemap[i] = loadedId;

#ifdef VERBOSE_DEBUG_OUTPUT
		for(unsigned indent = 0; indent < debugOutputIndent; indent++)
			printf("  ");

		printf("Module %d (%s) is found at index %d.\r\n", i, path, loadedId);
#endif

		mInfo++;
	}

	typeRemap.clear();

	// Add new symbols
	exSymbols.resize(oldSymbolSize + bCode->symbolLength);
	memcpy(&exSymbols[oldSymbolSize], FindSymbols(bCode), bCode->symbolLength);
	const char *symbolInfo = FindSymbols(bCode);

#ifdef VERBOSE_DEBUG_OUTPUT
	for(unsigned i = dependeciesBase; i < exDependencies.size(); i++)
	{
		for(unsigned indent = 0; indent < debugOutputIndent; indent++)
			printf("  ");

		printf("Dependency %d target is module %d (%s)\r\n", i - dependeciesBase, exDependencies[i], exSymbols.data + exModules[exDependencies[i]].nameOffset);
	}
#endif

	// Create type map for fast searches
	typeMap.clear();
	for(unsigned int i = 0; i < oldTypeCount; i++)
		typeMap.insert(exTypes[i].nameHash, i);

	// Add all types from bytecode to the list
	tInfo = tStart;
	for(unsigned int i = 0; i < bCode->typeCount; i++)
	{
		unsigned int *lastType = typeMap.find(tInfo->nameHash);

		if(lastType && exTypes[*lastType].size != tInfo->size)
		{
			debugOutputIndent--;

			NULLC::SafeSprintf(linkError, LINK_ERROR_BUFFER_SIZE, "Link Error: type %s is redefined (%s) with a different size (%d != %d)", exTypes[*lastType].offsetToName + &exSymbols[0], tInfo->offsetToName + symbolInfo, exTypes[*lastType].size, tInfo->size);
			return false;
		}
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
			}
		}else{
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

#ifdef VERBOSE_DEBUG_OUTPUT
	printf("Global variable size is %d, starting from %d.\r\n", bCode->globalVarSize, globalVarSize);
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
		printf("Variable %s %s at %d\r\n", &exSymbols[0] + exTypes[exVariables.back().type].offsetToName, &exSymbols[0] + exVariables.back().offsetToName, exVariables.back().offset);
#endif
		vInfo++;
	}

	// Add new locals
	unsigned int oldLocalsSize = exLocals.size();
	exLocals.resize(oldLocalsSize + bCode->localCount);
	memcpy(exLocals.data + oldLocalsSize, FindFirstLocal(bCode), bCode->localCount * sizeof(ExternLocalInfo));

	// Add new code information
	unsigned int oldVmSourceInfoSize = exVmSourceInfo.size();
	exVmSourceInfo.resize(oldVmSourceInfoSize + bCode->vmInfoSize);
	memcpy(exVmSourceInfo.data + oldVmSourceInfoSize, FindVmSourceInfo(bCode), bCode->vmInfoSize * sizeof(ExternSourceInfo));

	unsigned int oldRegVmSourceInfoSize = exRegVmSourceInfo.size();
	exRegVmSourceInfo.resize(oldRegVmSourceInfoSize + bCode->regVmInfoSize);
	memcpy(exRegVmSourceInfo.data + oldRegVmSourceInfoSize, FindRegVmSourceInfo(bCode), bCode->regVmInfoSize * sizeof(ExternSourceInfo));

	// Add new source code
	unsigned int oldSourceSize = exSource.size();
	exSource.resize(oldSourceSize + bCode->sourceSize);
	memcpy(exSource.data + oldSourceSize, FindSource(bCode), bCode->sourceSize);

	// Add new code
	unsigned int oldVmCodeSize = exVmCode.size();
	exVmCode.reserve(oldVmCodeSize + bCode->vmCodeSize + 1);
	exVmCode.resize(oldVmCodeSize + bCode->vmCodeSize);
	memcpy(exVmCode.data + oldVmCodeSize, FindVmCode(bCode), bCode->vmCodeSize * sizeof(VMCmd));

	for(unsigned int i = oldVmSourceInfoSize; i < exVmSourceInfo.size(); i++)
	{
		ExternSourceInfo &sourceInfo = exVmSourceInfo[i];

		sourceInfo.instruction += oldVmCodeSize;

		if(sourceInfo.definitionModule)
			sourceInfo.sourceOffset += exModules[exDependencies[dependeciesBase + sourceInfo.definitionModule - 1]].sourceOffset;
		else
			sourceInfo.sourceOffset += oldSourceSize;
	}

	assert(exRegVmCode.size() == exRegVmExecCount.size());

	unsigned int oldRegVmCodeSize = exRegVmCode.size();
	exRegVmCode.reserve(oldRegVmCodeSize + bCode->regVmCodeSize + 1);
	exRegVmCode.resize(oldRegVmCodeSize + bCode->regVmCodeSize);
	memcpy(exRegVmCode.data + oldRegVmCodeSize, FindRegVmCode(bCode), bCode->regVmCodeSize * sizeof(RegVmCmd));

	exRegVmExecCount.reserve(oldRegVmCodeSize + bCode->regVmCodeSize + 1);
	exRegVmExecCount.resize(oldRegVmCodeSize + bCode->regVmCodeSize);
	memset(exRegVmExecCount.data + oldRegVmCodeSize, 0, bCode->regVmCodeSize * sizeof(unsigned));

	for(unsigned int i = oldRegVmSourceInfoSize; i < exRegVmSourceInfo.size(); i++)
	{
		ExternSourceInfo &sourceInfo = exRegVmSourceInfo[i];

		sourceInfo.instruction += oldRegVmCodeSize;

		if(sourceInfo.definitionModule)
			sourceInfo.sourceOffset += exModules[exDependencies[dependeciesBase + sourceInfo.definitionModule - 1]].sourceOffset;
		else
			sourceInfo.sourceOffset += oldSourceSize;
	}

	unsigned int oldRegVmConstantsSize = exRegVmConstants.size();
	exRegVmConstants.resize(oldRegVmConstantsSize + bCode->regVmConstantCount);
	memcpy(exRegVmConstants.data + oldRegVmConstantsSize, FindRegVmConstants(bCode), bCode->regVmConstantCount * sizeof(unsigned));

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
				printf("Rebind function %3d %-20s (to address %4d [external %p] function %3d)\r\n", exFunctions.size() - 1, &exSymbols[0] + exFunctions.back().offsetToName, exFunctions.back().address, exFunctions.back().funcPtr, index);
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

			if(exFunctions.back().vmAddress == 0)
			{
				assert(exFunctions.back().regVmAddress == 0);

#ifdef NULLC_AUTOBINDING
	#if defined(__linux)
				void* handle = dlopen(0, RTLD_LAZY | RTLD_LOCAL);
				exFunctions.back().funcPtrRaw = (void (*)())dlsym(handle, FindSymbols(bCode) + exFunctions.back().offsetToName);
				dlclose(handle);
	#else
				exFunctions.back().funcPtrRaw = (void (*)())GetProcAddress(GetModuleHandle(NULL), FindSymbols(bCode) + exFunctions.back().offsetToName);
	#endif
#endif
				if(exFunctions.back().funcPtrRaw || exFunctions.back().funcPtrWrap)
				{
					exFunctions.back().vmAddress = ~0u;
					exFunctions.back().regVmAddress = ~0u;
				}
				else
				{
					NULLC::SafeSprintf(linkError, LINK_ERROR_BUFFER_SIZE, "Link Error: External function '%s' '%s' doesn't have implementation", FindSymbols(bCode) + exFunctions.back().offsetToName, &exSymbols[0] + exTypes[exFunctions.back().funcType].offsetToName);
					return false;
				}
			}

			// For function prototypes
			if(exFunctions.back().vmCodeSize & 0x80000000)
			{
				assert(exFunctions.back().vmCodeSize & 0x80000000);

				// fix remapping table so that this function index will point to target function index
				funcRemap.data[moduleFuncCount + i] = funcRemap[exFunctions.back().vmCodeSize & ~0x80000000];

				exFunctions.back().vmCodeSize = 0;
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
			if(exFunctions.back().vmAddress != -1)
			{
				exFunctions.back().vmAddress = oldVmCodeSize + fInfo->vmAddress;
				exFunctions.back().regVmAddress = oldRegVmCodeSize + fInfo->regVmAddress;

				vmJumpTargets.push_back(exFunctions.back().vmAddress);
				regVmJumpTargets.push_back(exFunctions.back().regVmAddress);
			}

#ifdef LINK_VERBOSE_DEBUG_OUTPUT
			printf("Adding function %3d %-20s (at address %4d [external %p])\r\n", exFunctions.size() - 1, &exSymbols[0] + exFunctions.back().offsetToName, exFunctions.back().address, exFunctions.back().funcPtr);
#endif
		}
	}

	for(unsigned int i = oldLocalsSize; i < oldLocalsSize + bCode->localCount; i++)
	{
		exLocals[i].type = typeRemap[exLocals[i].type];
		exLocals[i].offsetToName += oldSymbolSize;
	}

	assert((fInfo = FindFirstFunc(bCode)) != NULL); // this is fine, we need this assignment only in debug configuration
	// Fix cmdJmp*, cmdCall, cmdCallStd and commands with absolute addressing in new code
	unsigned int pos = oldVmCodeSize;
	while(pos < exVmCode.size())
	{
		VMCmd &cmd = exVmCode[pos];
		pos++;
		switch(cmd.cmd)
		{
		case cmdPushChar:
		case cmdPushShort:
		case cmdPushInt:
		case cmdPushFloat:
		case cmdPushDorL:
		case cmdPushCmplx:
		case cmdMovChar:
		case cmdMovShort:
		case cmdMovInt:
		case cmdMovFloat:
		case cmdMovDorL:
		case cmdMovCmplx:
			if(cmd.flag == ADDRESS_ABOLUTE)
			{
				if(cmd.argument >> 24)
					cmd.argument = (cmd.argument & 0x00ffffff) + exModules[moduleRemap[(cmd.argument >> 24) - 1]].variableOffset;
				else
					cmd.argument += oldGlobalSize;
			}
			break;
		case cmdGetAddr:
			if(cmd.helper == ADDRESS_ABOLUTE)
			{
				if(cmd.argument >> 24)
					cmd.argument = (cmd.argument & 0x00ffffff) + exModules[moduleRemap[(cmd.argument >> 24) - 1]].variableOffset;
				else
					cmd.argument += oldGlobalSize;
			}
			break;
		case cmdJmp:
		case cmdJmpZ:
		case cmdJmpNZ:
			cmd.argument += oldVmCodeSize;
			vmJumpTargets.push_back(cmd.argument);
			break;
		case cmdFuncAddr:
			cmd.cmd = cmdPushImmt;
			cmd.argument = funcRemap[cmd.argument];
			break;
		case cmdCall:
			assert(!(cmd.argument != funcRemap[cmd.argument] && int(cmd.argument - bCode->moduleFunctionCount) >= 0) ||
				(fInfo[cmd.argument - bCode->moduleFunctionCount].nameHash == exFunctions[funcRemap[cmd.argument]].nameHash));

			cmd.argument = funcRemap[cmd.argument];
			break;
		case cmdPushTypeID:
			cmd.cmd = cmdPushImmt;
			cmd.argument = typeRemap[cmd.argument];
			break;
		case cmdConvertPtr:
		case cmdCheckedRet:
			cmd.argument = typeRemap[cmd.argument];
			break;
#ifdef _M_X64
		case cmdPushPtr:
			cmd.cmd = cmdPushDorL;
			break;
		case cmdPushPtrStk:
			cmd.cmd = cmdPushDorLStk;
			break;
#else
		case cmdPushPtr:
			cmd.cmd = cmdPushInt;
			break;
		case cmdPushPtrStk:
			cmd.cmd = cmdPushIntStk;
			break;
		case cmdPushPtrImmt:
			cmd.cmd = cmdPushImmt;
			break;
#endif
		}
	}

	// Fix register VM command arguments
	pos = oldRegVmCodeSize;
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
	unsigned int size = 0;
	printf("Data managed by linker.\r\n");
	printf("Types: %db, ", exTypes.size() * sizeof(ExternTypeInfo));
	size += exTypes.size() * sizeof(ExternTypeInfo);
	printf("Variables: %db, ", exVariables.size() * sizeof(ExternVarInfo));
	size += exVariables.size() * sizeof(ExternVarInfo);
	printf("Functions: %db, ", exFunctions.size() * sizeof(ExternFuncInfo));
	size += exFunctions.size() * sizeof(ExternFuncInfo);
	printf("Function explicit type array offsets: %db, ", exFunctionExplicitTypeArrayOffsets.size() * sizeof(unsigned));
	size += exFunctionExplicitTypeArrayOffsets.size() * sizeof(unsigned);
	printf("Function explicit types: %db, ", exFunctionExplicitTypes.size() * sizeof(unsigned));
	size += exFunctionExplicitTypes.size() * sizeof(unsigned);
	printf("VM Code: %db\r\n", exVmCode.size() * sizeof(VMCmd));
	size += exVmCode.size() * sizeof(VMCmd);
	printf("Reg VM Code: %db\r\n", exRegVmCode.size() * sizeof(RegVmCmd));
	size += exRegVmCode.size() * sizeof(RegVmCmd);
	printf("Symbols: %db, ", exSymbols.size() * sizeof(char));
	size += exSymbols.size() * sizeof(char);
	printf("Locals: %db, ", exLocals.size() * sizeof(ExternLocalInfo));
	size += exLocals.size() * sizeof(ExternLocalInfo);
	printf("Modules: %db, ", exModules.size() * sizeof(ExternModuleInfo));
	size += exModules.size() * sizeof(ExternModuleInfo);
	printf("Source info: %db, ", exSourceInfo.size() * sizeof(ExternSourceInfo));
	size += exSourceInfo.size() * sizeof(ExternSourceInfo);
	printf("Source: %db\r\n", exSource.size() * sizeof(char));
	size += exSource.size() * sizeof(char);
	printf("Overall: %d bytes\r\n\r\n", size);
#endif

	return true;
}

bool Linker::SaveVmListing(OutputContext &output)
{
	char instBuf[128];
	unsigned line = 0, lastLine = ~0u;

	ExternSourceInfo *info = (ExternSourceInfo*)exVmSourceInfo.data;
	unsigned infoSize = exVmSourceInfo.size();

	const char *lastSourcePos = exSource.data;
	const char *lastCodeStart = NULL;

	for(unsigned i = 0; infoSize && i < exVmCode.size(); i++)
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

		exVmCode[i].Decode(instBuf);
		if(exVmCode[i].cmd == cmdCall || exVmCode[i].cmd == cmdFuncAddr)
			output.Printf("// %4d: %s (%s)\r\n", i, instBuf, exSymbols.data + exFunctions[exVmCode[i].argument].offsetToName);
		else if(exVmCode[i].cmd == cmdPushTypeID)
			output.Printf("// %4d: %s (%s)\r\n", i, instBuf, exSymbols.data + exTypes[exVmCode[i].argument].offsetToName);
		else
			output.Printf("// %4d: %s\r\n", i, instBuf);
	}

	output.Flush();

	return true;
}

bool Linker::SaveRegVmListing(OutputContext &output, bool withProfileInfo)
{
	unsigned line = 0, lastLine = ~0u;

	ExternSourceInfo *info = (ExternSourceInfo*)exRegVmSourceInfo.data;
	unsigned infoSize = exRegVmSourceInfo.size();

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

		bool found = false;

		for(unsigned k = 0; k < regVmJumpTargets.size(); k++)
		{
			if(regVmJumpTargets[k] == i)
			{
				found = true;
				break;
			}
		}

		if(withProfileInfo)
		{
			if(found)
			{
				output.Printf("//            %4d:\n", i);
				output.Printf("// (%8d)      ", exRegVmExecCount[i]);
			}
			else
			{
				output.Printf("// (%8d) %4d ", exRegVmExecCount[i], i);
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

		unsigned long long total = 0;

		for(unsigned i = 0; i < 256; i++)
			total += exRegVmInstructionExecCount[i];

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

