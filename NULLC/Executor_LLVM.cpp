#include "Executor_LLVM.h"

#include "nullc.h"
#include "Linker.h"
#include "StdLib.h"

#ifdef NULLC_LLVM_SUPPORT

#include "llvm-c/BitReader.h"
#include "llvm-c/Core.h"
#include "llvm-c/ExecutionEngine.h"
#include "llvm-c/ErrorHandling.h"
#include "llvm-c/Linker.h"

#pragma comment(lib, "llvm-c/lib/LLVMAsmPrinter.lib")
#pragma comment(lib, "llvm-c/lib/LLVMCodeGen.lib")
#pragma comment(lib, "llvm-c/lib/LLVMDebugInfoCodeView.lib")
#pragma comment(lib, "llvm-c/lib/LLVMExecutionEngine.lib")
#pragma comment(lib, "llvm-c/lib/LLVMGlobalISel.lib")
#pragma comment(lib, "llvm-c/lib/LLVMLinker.lib")
#pragma comment(lib, "llvm-c/lib/LLVMMCDisassembler.lib")
#pragma comment(lib, "llvm-c/lib/LLVMMCJIT.lib")
#pragma comment(lib, "llvm-c/lib/LLVMOrcJIT.lib")
#pragma comment(lib, "llvm-c/lib/LLVMRuntimeDyld.lib")
#pragma comment(lib, "llvm-c/lib/LLVMSelectionDAG.lib")
#pragma comment(lib, "llvm-c/lib/LLVMTarget.lib")

#pragma comment(lib, "llvm-c/lib/LLVMInterpreter.lib")

#pragma comment(lib, "llvm-c/lib/LLVMX86AsmPrinter.lib")
#pragma comment(lib, "llvm-c/lib/LLVMX86CodeGen.lib")
#pragma comment(lib, "llvm-c/lib/LLVMX86Desc.lib")
#pragma comment(lib, "llvm-c/lib/LLVMX86Info.lib")
#pragma comment(lib, "llvm-c/lib/LLVMX86Utils.lib")

#define dcAllocMem NULLC::alloc
#define dcFreeMem  NULLC::dealloc

#include "../external/dyncall/dyncall.h"

namespace GC
{
	extern char	*unmanageableBase;
	extern char	*unmanageableTop;
}

enum LLVMReturnType
{
	LLVM_NONE,
	LLVM_INT,
	LLVM_LONG,
	LLVM_DOUBLE,
};

struct LlvmExecutionContext
{
	LlvmExecutionContext()
	{
		context = NULL;

		executionEngine = NULL;

		module = NULL;

		llvmVm = NULL;

		linker = NULL;
	}

	LLVMContextRef context;

	LLVMExecutionEngineRef executionEngine;
	LLVMModuleRef module;

	SmallArray<LLVMModuleRef, 16> modules;

	SmallArray<const char*, 16> functionNames;

	FastVector<char> globalVars;

	ExecutorLLVM *llvmVm;

	Linker *linker;
};

namespace
{
	LLVMReturnType llvmReturnedType = LLVM_NONE;
	int llvmReturnedInt = 0;
	long long llvmReturnedLong = 0ll;
	double llvmReturnedDouble = 0.0;

	void *llvmStackTop = NULL;

	LlvmExecutionContext *currentCtx = NULL; // TODO: pass from global inside the bytecode

	void llvmAbortNoReturn()
	{
		printf("LLVM Fatal error: function didn't return a value\n");
	}

	void* llvmConvertPtr(NULLCRef ref, unsigned typeID)
	{
		LlvmExecutionContext &ctx = *currentCtx;

		unsigned sourceTypeID = ref.typeID;

		if (sourceTypeID == typeID)
			return (void*)ref.ptr;

		while (ctx.linker->exTypes[sourceTypeID].baseType)
		{
			sourceTypeID = ctx.linker->exTypes[sourceTypeID].baseType;
			if (sourceTypeID == typeID)
				return (void*)ref.ptr;
		}

		const char* symbols = ctx.linker->exSymbols.data;

		nullcThrowError("ERROR: cannot convert from %s ref to %s ref", symbols + ctx.linker->exTypes[ref.typeID].offsetToName, symbols + ctx.linker->exTypes[typeID].offsetToName);

		return 0;
	}

	int llvmIntPow(int number, int power)
	{
		if (power < 0)
			return number == 1 ? 1 : (number == -1 ? ((power & 1) ? -1 : 1) : 0);

		int result = 1;
		while (power)
		{
			if (power & 1)
			{
				result *= number;
				power--;
			}
			number *= number;
			power >>= 1;
		}
		return result;
	}

	long long llvmLongPow(long long number, long long power)
	{
		if (power < 0)
			return number == 1 ? 1 : (number == -1 ? ((power & 1) ? -1 : 1) : 0);

		long long result = 1;
		while (power)
		{
			if (power & 1)
			{
				result *= number;
				power--;
			}
			number *= number;
			power >>= 1;
		}
		return result;
	}

	double llvmDoublePow(double number, double power)
	{
		return pow(number, power);
	}

	void llvmReturnInt(int x)
	{
		llvmReturnedType = LLVM_INT;
		llvmReturnedInt = x;
	}

	void llvmReturnLong(long long x)
	{
		llvmReturnedType = LLVM_LONG;
		llvmReturnedLong = x;
	}

	void llvmReturnDouble(double x)
	{
		llvmReturnedType = LLVM_DOUBLE;
		llvmReturnedDouble = x;
	}

	void llvmExternalCall(unsigned functionId, char *argumentBuffer, char *returnBuffer)
	{
		LlvmExecutionContext &ctx = *currentCtx;

		ExternFuncInfo &target = ctx.linker->exFunctions[functionId];

		if(target.regVmAddress == -1)
		{
			if(target.funcPtrWrap)
			{
				target.funcPtrWrap(target.funcPtrWrapTarget, returnBuffer, argumentBuffer);
			}
			else
			{
#if !defined(NULLC_NO_RAW_EXTERNAL_CALL)
				RunRawExternalFunction(ctx.llvmVm->dcCallVM, target, ctx.linker->exLocals.data, ctx.linker->exTypes.data, ctx.linker->exTypeExtra.data, (unsigned*)argumentBuffer, (unsigned*)returnBuffer);
#else
				// TODO: handle error
#endif
			}

			// TODO: handle error
		}
		else
		{
			// TODO: internal call
		}
	}

	void llvmFatalErrorHandler(const char *reason)
	{
		printf("LLVM Fatal error: %s\n", reason);
	}

	unsigned GetFunctionID(Linker *linker, const char *name, unsigned nameLength, const char *type, unsigned typeLength, const char *generics)
	{
		const char* symbols = linker->exSymbols.data;

		for(unsigned i = 0; i < linker->exFunctions.size(); i++)
		{
			ExternFuncInfo &function = linker->exFunctions[i];

			if(nameLength == strlen(function.offsetToName + symbols) && memcmp(name, function.offsetToName + symbols, nameLength) == 0)
			{
				// Check the function type
				if(typeLength != strlen(linker->exTypes[function.funcType].offsetToName + symbols) || memcmp(type, linker->exTypes[function.funcType].offsetToName + symbols, typeLength) != 0)
					continue;

				assert(!generics);

				return i;
			}
		}

		return ~0u;
	}
}

ExecutorLLVM::ExecutorLLVM(Linker* linker)
{
	execErrorBuffer = (char*)NULLC::alloc(NULLC_ERROR_BUFFER_SIZE);
	*execErrorBuffer = 0;

	exLinker = linker;

	ctx = NULL;

	dcCallVM = NULL;
}

ExecutorLLVM::~ExecutorLLVM()
{
	NULLC::dealloc(execErrorBuffer);

	LLVMDisposeExecutionEngine(ctx->executionEngine);

	LLVMContextDispose(ctx->context);

	delete ctx;

	if(dcCallVM)
		dcFree(dcCallVM);
	dcCallVM = NULL;
}

bool ExecutorLLVM::TranslateToNative()
{
	if(ctx)
	{
		LLVMDisposeExecutionEngine(ctx->executionEngine);

		LLVMResetFatalErrorHandler();

		LLVMContextDispose(ctx->context);

		delete ctx;
		ctx = NULL;
	}

	ctx = new LlvmExecutionContext();

	ctx->llvmVm = this;
	ctx->linker = exLinker;

	CommonSetLinker(exLinker);

	ctx->context = LLVMContextCreate();

	llvmReturnedType = LLVM_NONE;

	execErrorMessage = NULL;

	execErrorObject.typeID = 0;
	execErrorObject.ptr = NULL;

	if(!exLinker->llvmModuleSizes.size())
	{
		printf("Code not found\n");
		assert(0);
		return false;
	}

	ctx->modules.clear();

	LLVMInstallFatalErrorHandler(llvmFatalErrorHandler);

	ctx->module = LLVMModuleCreateWithNameInContext("module", ctx->context);

	unsigned offset = 0;
	for(unsigned i = 0; i < exLinker->llvmModuleSizes.size(); i++)
	{
		char buf[32];

		sprintf(buf, "module_%d", i);

		// Load module code
		LLVMMemoryBufferRef buffer = LLVMCreateMemoryBufferWithMemoryRange(&exLinker->llvmModuleCodes[offset], exLinker->llvmModuleSizes[i], buf, false);
		LLVMModuleRef moduleData = NULL;
		
		if(LLVMParseBitcodeInContext2(ctx->context, buffer, &moduleData))
		{
			// TODO: report error
			return false;
		}

		// Change global code function name, because every module has one
		LLVMValueRef entryFunction = LLVMGetNamedFunction(moduleData, "__llvmEntry");
		
		sprintf(buf, "__llvmEntry_%d", i);
		LLVMSetValueName2(entryFunction, buf, strlen(buf));

		// TODO: change the type index constant values
		/*unsigned *typeRemap = &exLinker->llvmTypeRemapValues[exLinker->llvmTypeRemapOffsets[i]];
		unsigned typeCount = exLinker->llvmTypeRemapSizes[i];

		for(unsigned k = 0; k < typeCount; k++)
		{
			char buf[32];
			sprintf(buf, "^type_index_%d", k);

			if(llvm::GlobalVariable *typeIndexValue = module->getGlobalVariable(buf, true))
				typeIndexValue->setInitializer(llvm::ConstantInt::get(llvm::Type::getInt32Ty(getContext()), llvm::APInt(32, uint64_t(typeRemap[k]), false)));
		}*/

		// Change the function index constant values
		/*unsigned *funcRemap = &exLinker->llvmFuncRemapValues[exLinker->llvmFuncRemapOffsets[i]];
		unsigned functionCount = exLinker->llvmFuncRemapSizes[i];

		for(unsigned k = 0; k < functionCount; k++)
		{
			char buf[32];
			sprintf(buf, "^func_index_%d", k);

			if(llvm::GlobalVariable *funcIndexValue = module->getGlobalVariable(buf, true))
				funcIndexValue->setInitializer(llvm::ConstantInt::get(llvm::Type::getInt32Ty(getContext()), llvm::APInt(32, uint64_t(funcRemap[k]), false)));
		}*/

		// Link module to the other
		if(LLVMLinkModules2(ctx->module, moduleData))
		{
			// TODO: report error
			return false;
		}

		offset += exLinker->llvmModuleSizes[i];

		ctx->modules.push_back(moduleData);
	}

	//LLVMDumpModule(ctx->module);

	LLVMLinkInMCJIT();

	LLVMInitializeNativeTarget();
	LLVMInitializeNativeAsmPrinter();

	LLVMMCJITCompilerOptions options;
	LLVMInitializeMCJITCompilerOptions(&options, sizeof(LLVMMCJITCompilerOptions));

	options.OptLevel = 0;
	options.NoFramePointerElim = true;
	options.EnableFastISel = true;

	char *error = NULL;
	if(LLVMCreateMCJITCompilerForModule(&ctx->executionEngine, ctx->module, &options, sizeof(LLVMMCJITCompilerOptions), &error))
	{
		printf("Could not create llvm execution engine: %s\n", error);
		return false;
	}

	LLVMDisposeMessage(error);

	// Set basic functions
	if (LLVMValueRef function = LLVMGetNamedFunction(ctx->module, "__llvmAbortNoReturn"))
		LLVMAddGlobalMapping(ctx->executionEngine, function, (void*)llvmAbortNoReturn);

	if (LLVMValueRef function = LLVMGetNamedFunction(ctx->module, "__llvmConvertPtr"))
		LLVMAddGlobalMapping(ctx->executionEngine, function, (void*)llvmConvertPtr);

	if(LLVMValueRef function = LLVMGetNamedFunction(ctx->module, "__llvmPowInt"))
		LLVMAddGlobalMapping(ctx->executionEngine, function, (void*)llvmIntPow);

	if(LLVMValueRef function = LLVMGetNamedFunction(ctx->module, "__llvmPowLong"))
		LLVMAddGlobalMapping(ctx->executionEngine, function, (void*)llvmLongPow);

	if(LLVMValueRef function = LLVMGetNamedFunction(ctx->module, "__llvmPowDouble"))
		LLVMAddGlobalMapping(ctx->executionEngine, function, (void*)llvmDoublePow);

	if(LLVMValueRef function = LLVMGetNamedFunction(ctx->module, "__llvmReturnInt"))
		LLVMAddGlobalMapping(ctx->executionEngine, function, (void*)llvmReturnInt);

	if(LLVMValueRef function = LLVMGetNamedFunction(ctx->module, "__llvmReturnLong"))
		LLVMAddGlobalMapping(ctx->executionEngine, function, (void*)llvmReturnLong);

	if(LLVMValueRef function = LLVMGetNamedFunction(ctx->module, "__llvmReturnDouble"))
		LLVMAddGlobalMapping(ctx->executionEngine, function, (void*)llvmReturnDouble);

	if(LLVMValueRef function = LLVMGetNamedFunction(ctx->module, "__llvmExternalCall"))
		LLVMAddGlobalMapping(ctx->executionEngine, function, (void*)llvmExternalCall);

	ctx->functionNames.resize(exLinker->exFunctions.size());
	memset(ctx->functionNames.data, 0, ctx->functionNames.count * sizeof(ctx->functionNames.data[0]));

	// External functions binding
	for(LLVMValueRef function = LLVMGetFirstFunction(ctx->module); function; function = LLVMGetNextFunction(function))
	{
		size_t length = 0;
		const char *name = LLVMGetValueName2(function, &length);

		const char *type = strchr(name, '#');

		if(!type)
			continue;

		unsigned nameLength = unsigned(type - name);

		type++;

		const char *generics = strchr(type, '$');

		unsigned typeLength = generics ? unsigned(generics - type) : unsigned(strlen(type));

		if(generics)
			generics++;

		unsigned funcID = ::GetFunctionID(exLinker, name, nameLength, type, typeLength, generics);

		if(funcID != ~0u)
			ctx->functionNames[funcID] = name;
	}

	if(!dcCallVM)
	{
		dcCallVM = dcNewCallVM(4096);
		dcMode(dcCallVM, DC_CALL_C_DEFAULT);
	}

	return true;
}

bool ExecutorLLVM::Run(unsigned int functionID, const char *arguments)
{
	if(functionID != ~0u)
	{
		ExternFuncInfo &targetFunction = exLinker->exFunctions[functionID];

		unsigned int dwordsToPop = (targetFunction.argumentSize >> 2);

		/*if(targetFunction.regVmAddress == ~0u)
		{
			// Copy all arguments
			memcpy(tempStackPtr, arguments, target.argumentSize);

			// Call function
			if(target.funcPtrWrap)
			{
				target.funcPtrWrap(target.funcPtrWrapTarget, (char*)tempStackPtr, (char*)tempStackPtr);

				if(!callContinue)
					errorState = true;
			}
			else
			{
				if(!RunExternalFunction(functionID, tempStackPtr))
					errorState = true;
			}

			// This will disable NULLC code execution while leaving error check and result retrieval
			instruction = NULL;
		}*/

		void* fPtr = NULL;

		if(targetFunction.startInByteCode == ~0u)
		{
			//fPtr = targetFunction.funcPtr;
		}
		else
		{

			fPtr = (void*)LLVMGetFunctionAddress(ctx->executionEngine, ctx->functionNames[functionID]);
		}

		// Can't find target function
		if(!fPtr)
		{
			Stop("ERROR: can't find target function address");
			return false;
		}

		// Can't return complex types here
		if(targetFunction.retType == ExternFuncInfo::RETURN_UNKNOWN)
		{
			Stop("ERROR: can't call external function with complex return type");
			return false;
		}

		dcReset(dcCallVM);

		unsigned int *stackStart = ((unsigned int*)arguments);

		for(unsigned i = 0; i < dwordsToPop; i++)
		{
			dcArgInt(dcCallVM, *(int*)stackStart);
			stackStart += 1;
		}

		currentCtx = ctx;

		switch(targetFunction.retType)
		{
		case ExternFuncInfo::RETURN_VOID:
			llvmReturnedType = LLVM_NONE;
			dcCallVoid(dcCallVM, fPtr);
			break;
		case ExternFuncInfo::RETURN_INT:
			llvmReturnedType = LLVM_INT;
			llvmReturnedInt = dcCallInt(dcCallVM, fPtr);
			break;
		case ExternFuncInfo::RETURN_DOUBLE:
			llvmReturnedType = LLVM_DOUBLE;
			llvmReturnedDouble = dcCallDouble(dcCallVM, fPtr);
			break;
		case ExternFuncInfo::RETURN_LONG:
			llvmReturnedType = LLVM_LONG;
			llvmReturnedLong = dcCallLongLong(dcCallVM, fPtr);
			break;
		}

		currentCtx = NULL;

		return true;
	}

	int stackHelper = 0;
	llvmStackTop = &stackHelper;
	GC::unmanageableTop = (char*)llvmStackTop;
	currentCtx = ctx;

	//char *error = NULL;
	//LLVMTargetMachineEmitToFile(currentTarget, ctx->module, "inst_llvm_asm.txt", LLVMAssemblyFile, &error);
	//LLVMDisposeMessage(error);

	for(unsigned i = 0; i < exLinker->llvmModuleSizes.size(); i++)
	{
		char buf[32];
		sprintf(buf, "__llvmEntry_%d", i);

		LLVMValueRef function = LLVMGetNamedFunction(ctx->module, buf);

		LLVMRunFunction(ctx->executionEngine, function, 0, NULL);
	}

	currentCtx = NULL;

	ctx->globalVars.resize(exLinker->globalVarSize);

	for(unsigned i = 0; i < exLinker->exVariables.size(); i++)
	{
		ExternVarInfo &varInfo = exLinker->exVariables[i];
		ExternTypeInfo &varType = exLinker->exTypes[varInfo.type];

		if(exLinker->exSymbols.data[varInfo.offsetToName] == '$')
			continue;

		//printf("Variable '%s' '%s' at %d (size %d)\n", exLinker->exSymbols.data + varType.offsetToName, exLinker->exSymbols.data + varInfo.offsetToName, varInfo.offset, varType.size);

		uint64_t address = LLVMGetGlobalValueAddress(ctx->executionEngine, exLinker->exSymbols.data + varInfo.offsetToName);

		if(address)
			memcpy(ctx->globalVars.data + varInfo.offset, (void*)address, varType.size);
	}

	return true;
}

void ExecutorLLVM::Stop(const char* error)
{
	assert(!"ExecutorLLVM::Stop is not implemented");

	NULLC::SafeSprintf(execErrorBuffer, NULLC_ERROR_BUFFER_SIZE, "%s", error);

	execErrorMessage = execErrorBuffer;

	execErrorObject.typeID = 0;
	execErrorObject.ptr = NULL;
}

void ExecutorLLVM::Stop(NULLCRef error)
{
	assert(!"ExecutorLLVM::Stop is not implemented");

	NULLC::SafeSprintf(execErrorBuffer, NULLC_ERROR_BUFFER_SIZE, "%s", exLinker->exSymbols.data + exLinker->exTypes[error.typeID].offsetToName);

	execErrorMessage = execErrorBuffer;

	if(nullcIsStackPointer(error.ptr))
		execErrorObject = NULLC::CopyObject(error);
	else
		execErrorObject = error;
}

void ExecutorLLVM::Resume()
{
	assert(!"ExecutorLLVM::Resume is not implemented");

	execErrorMessage = NULL;

	execErrorObject.typeID = 0;
	execErrorObject.ptr = NULL;
}

bool ExecutorLLVM::SetStackSize(unsigned bytes)
{
	(void)bytes;

	return true;
}

unsigned ExecutorLLVM::GetResultType()
{
	switch(llvmReturnedType)
	{
	case LLVM_DOUBLE:
		return NULLC_TYPE_DOUBLE;
	case LLVM_LONG:
		return NULLC_TYPE_LONG;
	case LLVM_INT:
		return NULLC_TYPE_INT;
	default:
		break;
	}

	return NULLC_TYPE_VOID;
}

NULLCRef ExecutorLLVM::GetResultObject()
{
	NULLCRef result = { 0, 0 };

	switch(llvmReturnedType)
	{
	case LLVM_DOUBLE:
		result.typeID = NULLC_TYPE_DOUBLE;
		result.ptr = (char*)nullcAllocateTyped(NULLC_TYPE_DOUBLE);
		memcpy(result.ptr, &llvmReturnedDouble, sizeof(llvmReturnedDouble));
		break;
	case LLVM_LONG:
		result.typeID = NULLC_TYPE_LONG;
		result.ptr = (char*)nullcAllocateTyped(NULLC_TYPE_LONG);
		memcpy(result.ptr, &llvmReturnedLong, sizeof(llvmReturnedLong));
		break;
	case LLVM_INT:
		result.typeID = NULLC_TYPE_INT;
		result.ptr = (char*)nullcAllocateTyped(NULLC_TYPE_INT);
		memcpy(result.ptr, &llvmReturnedInt, sizeof(llvmReturnedInt));
		break;
	default:
		break;
	}

	return result;
}

const char* ExecutorLLVM::GetResult()
{
	switch(llvmReturnedType)
	{
	case LLVM_DOUBLE:
		NULLC::SafeSprintf(execResult, execResultSize, "%f", llvmReturnedDouble);
		break;
	case LLVM_LONG:
		NULLC::SafeSprintf(execResult, execResultSize, "%lldL", llvmReturnedLong);
		break;
	case LLVM_INT:
		NULLC::SafeSprintf(execResult, execResultSize, "%d", llvmReturnedInt);
		break;
	default:
		NULLC::SafeSprintf(execResult, execResultSize, "no return value");
		break;
	}
	return execResult;
}

int ExecutorLLVM::GetResultInt()
{
	assert(llvmReturnedType == LLVM_INT);
	return llvmReturnedInt;
}

double ExecutorLLVM::GetResultDouble()
{
	assert(llvmReturnedType == LLVM_DOUBLE);
	return llvmReturnedDouble;
}

long long ExecutorLLVM::GetResultLong()
{
	assert(llvmReturnedType == LLVM_LONG);
	return llvmReturnedLong;
}

const char* ExecutorLLVM::GetErrorMessage()
{
	return execErrorMessage;
}

NULLCRef ExecutorLLVM::GetErrorObject()
{
	return execErrorObject;
}

char* ExecutorLLVM::GetVariableData(unsigned int *count)
{
	ctx->globalVars.resize(exLinker->globalVarSize);

	if(count)
		*count = exLinker->exVariables.size();
	return ctx->globalVars.data;
}

unsigned ExecutorLLVM::GetCallStackAddress(unsigned frame)
{
	// TODO: call stack
	(void)frame;
	return 0;
}

void* ExecutorLLVM::GetStackStart()
{
	int stackHelper = 0;
	uintptr_t stackTop = uintptr_t(&stackHelper);

	return (void*)((stackTop & 0xfull) + (stackTop & ~0xfull));
}

void* ExecutorLLVM::GetStackEnd()
{
	return llvmStackTop;
}

#endif
