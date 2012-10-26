#include "Executor_LLVM.h"

#ifdef NULLC_LLVM_SUPPORT

#pragma warning(push)
#pragma warning(disable: 4530 4512 4800 4146 4244 4245 4146 4355 4100 4267)

#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Bitcode/BitstreamReader.h"
#include "llvm/Bitcode/ReaderWriter.h"

#include "llvm/LLVMContext.h"
#include "llvm/Module.h"
#include "llvm/Linker.h"

#include "llvm/Support/TypeBuilder.h"
#include "llvm/Support/IRBuilder.h"

#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/JIT.h"

#pragma comment(lib, "LLVMLinker.lib")

#include <string>

#pragma warning(pop)

int llvmIntPow(int number, int power)
{
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

long long llvmLongPow(long long num, long long power)
{
	if(power < 0)
		return (num == 1 ? 1 : 0);
	long long res = 1;
	while(power)
	{
		if(power & 0x01)
		{
			res *= num;
			power--;
		}
		num *= num;
		power >>= 1;
	}
	return res;
}

double llvmDoublePow(double number, double power)
{
	return pow(number, power);
}

long long llvmLongDiv(long long a, long long b)
{
	return a / b;
}

long long llvmLongMod(long long a, long long b)
{
	return a % b;
}

void	llvmSetArrayC(char arr[], char val, unsigned int count)
{
	for(unsigned int i = 0; i < count; i++)
		arr[i] = val;
}
void	llvmSetArrayS(short arr[], short val, unsigned int count)
{
	for(unsigned int i = 0; i < count; i++)
		arr[i] = val;
}
void	llvmSetArrayI(int arr[], int val, unsigned int count)
{
	for(unsigned int i = 0; i < count; i++)
		arr[i] = val;
}
void	llvmSetArrayF(float arr[], float val, unsigned int count)
{
	for(unsigned int i = 0; i < count; i++)
		arr[i] = val;
}
void	llvmSetArrayD(double arr[], double val, unsigned int count)
{
	for(unsigned int i = 0; i < count; i++)
		arr[i] = val;
}
void	llvmSetArrayL(long long arr[], long long val, unsigned int count)
{
	for(unsigned int i = 0; i < count; i++)
		arr[i] = val;
}

namespace GC
{
	extern char	*unmanageableBase;
	extern char	*unmanageableTop;
}

struct LLVMUpvalue
{
	void		*ptr;
	LLVMUpvalue	*next;
	unsigned	size;
};

void	llvmCloseUpvalue(void* upvalue, char* ptr)
{
	LLVMUpvalue **head = (LLVMUpvalue**)upvalue;
	LLVMUpvalue *curr = *head;

	GC::unmanageableBase = (char*)&curr;
	// close upvalue if it's target is equal to local variable, or it's address is out of stack
	while(curr && ((char*)curr->ptr == ptr || (char*)curr->ptr < GC::unmanageableBase || (char*)curr->ptr > GC::unmanageableTop))
	{
		LLVMUpvalue *next = curr->next;
		unsigned size = curr->size;
		*head = curr->next;
		memcpy(&curr->next, curr->ptr, size);
		curr->ptr = (unsigned*)&curr->next;
		curr = next;
	}
}

enum LLVMReturnType
{
	LLVM_NONE,
	LLVM_INT,
	LLVM_LONG,
	LLVM_DOUBLE,
};

LLVMReturnType llvmReturnedType = LLVM_NONE;
int llvmReturnedInt = 0;
long long llvmReturnedLong = 0ll;
double llvmReturnedDouble = 0.0;

void *llvmStackTop = NULL;

void	llvmReturnInt(int x)
{
	llvmReturnedType = LLVM_INT;
	llvmReturnedInt = x;
}

void	llvmReturnLong(long long x)
{
	llvmReturnedType = LLVM_LONG;
	llvmReturnedLong = x;
}

void	llvmReturnDouble(double x)
{
	llvmReturnedType = LLVM_DOUBLE;
	llvmReturnedDouble = x;
}

Linker	*currentLinker = NULL;
typedef void (*functionType)();

functionType __llvmIndexToFunction(unsigned index)
{
	return (void(*)())(currentLinker->exFunctions[index].funcPtr);
}

namespace
{
	struct ContextHolder
	{
		llvm::LLVMContext	context;
	};

	ContextHolder	*ctx = 0;

	llvm::LLVMContext&	getContext()
	{
		return ctx->context;
	}

	llvm::ExecutionEngine	*TheExecutionEngine = NULL;
	llvm::Linker			*linker = NULL;
	llvm::Module			*module = NULL;

	FastVector<llvm::Module*>	modules;

	FastVector<char>	globalVars;
}

ExecutorLLVM::ExecutorLLVM(Linker* linker)
{
	exLinker = linker;
}

ExecutorLLVM::~ExecutorLLVM()
{
}

unsigned ExecutorLLVM::GetFunctionID(const char* name, unsigned length)
{
	const char* symbols = exLinker->exSymbols.data;
	for(unsigned funcID = 0; funcID < exLinker->exFunctions.size(); funcID++)
	{
		if(length == strlen(exLinker->exFunctions[funcID].offsetToName + symbols) && memcmp(name, exLinker->exFunctions[funcID].offsetToName + symbols, length) == 0)
		{
			if(mapped[funcID])
				continue;

			if(exLinker->exFunctions[funcID].address != -1 && exLinker->exFunctions[funcID].codeSize == 0)
				continue;

			mapped[funcID] = 1;

			// Skip generic base functions
			if(exLinker->exFunctions[funcID].funcType == 0)
				continue;

			return funcID;
		}
	}
	return ~0u;
}

bool	ExecutorLLVM::TranslateToNative()
{
	delete TheExecutionEngine;
	TheExecutionEngine = NULL;
	delete linker;
	linker = NULL;

	delete ctx;
	ctx = new ContextHolder();

	CommonSetLinker(exLinker);

	llvmReturnedType = LLVM_NONE;

	execError[0] = 0;

	if(!exLinker->llvmModuleSizes.size())
	{
		printf("Code not found\n");
		assert(0);
		return false;
	}
	modules.clear();
	linker = new llvm::Linker("main", "main", getContext());

	std::string error;

	unsigned offset = 0;
	for(unsigned i = 0; i < exLinker->llvmModuleSizes.size(); i++)
	{
		char buf[32];

		// Load module code
		llvm::MemoryBuffer *buffer = llvm::MemoryBuffer::getMemBuffer(llvm::StringRef(&exLinker->llvmModuleCodes[offset], exLinker->llvmModuleSizes[i]), "module", false);
		llvm::Module *module = llvm::ParseBitcodeFile(buffer, getContext(), &error);
		if(!error.empty())
		{
			printf("%s\n", error.c_str());
			assert(0);
			return false;
		}

		// Change global code function name, because every module has one
		llvm::Function *glob = module->getFunction("Global");
		sprintf(buf, "Global%d", i);
		glob->setName(buf);

		// Change the type index constant values
		unsigned	*typeRemap = &exLinker->llvmTypeRemapValues[exLinker->llvmTypeRemapOffsets[i]];
		unsigned	typeCount = exLinker->llvmTypeRemapSizes[i];
		for(unsigned k = 0; k < typeCount; k++)
		{
			char buf[32];
			sprintf(buf, "^type_index_%d", k);

			if(llvm::GlobalVariable *typeIndexValue = module->getGlobalVariable(buf, true))
				typeIndexValue->setInitializer(llvm::ConstantInt::get(llvm::Type::getInt32Ty(getContext()), llvm::APInt(32, uint64_t(typeRemap[k]), false)));
		}

		// Change the function index constant values
		unsigned	*funcRemap = &exLinker->llvmFuncRemapValues[exLinker->llvmFuncRemapOffsets[i]];
		unsigned	functionCount = exLinker->llvmFuncRemapSizes[i];
		for(unsigned k = 0; k < functionCount; k++)
		{
			char buf[32];
			sprintf(buf, "^func_index_%d", k);

			if(llvm::GlobalVariable *funcIndexValue = module->getGlobalVariable(buf, true))
				funcIndexValue->setInitializer(llvm::ConstantInt::get(llvm::Type::getInt32Ty(getContext()), llvm::APInt(32, uint64_t(funcRemap[k]), false)));
		}

		// Link module to the other
		if(linker->LinkInModule(module, &error))
		{
			printf("%s\n", error.c_str());
			assert(0);
			return false;
		}
		offset += exLinker->llvmModuleSizes[i];
		modules.push_back(module);
	}
	module = linker->releaseModule();
	TheExecutionEngine = llvm::EngineBuilder(module).setErrorStr(&error).create();
	if(!TheExecutionEngine)
	{
		printf("Could not create ExecutionEngine: %s\n", error.c_str());
		assert(0);
		return false;
	}

	// Set basic functions
	TheExecutionEngine->updateGlobalMapping(module->getFunction("llvmIntPow"), (void*)llvmIntPow);
	TheExecutionEngine->updateGlobalMapping(module->getFunction("llvmLongPow"), (void*)llvmLongPow);
	TheExecutionEngine->updateGlobalMapping(module->getFunction("llvmDoublePow"), (void*)llvmDoublePow);
	TheExecutionEngine->updateGlobalMapping(module->getFunction("__moddi3"), (void*)llvmLongMod);
	TheExecutionEngine->updateGlobalMapping(module->getFunction("__divdi3"), (void*)llvmLongDiv);
	TheExecutionEngine->updateGlobalMapping(module->getFunction("__llvmSetArrayC"), (void*)llvmSetArrayC);
	TheExecutionEngine->updateGlobalMapping(module->getFunction("__llvmSetArrayS"), (void*)llvmSetArrayS);
	TheExecutionEngine->updateGlobalMapping(module->getFunction("__llvmSetArrayI"), (void*)llvmSetArrayI);
	TheExecutionEngine->updateGlobalMapping(module->getFunction("__llvmSetArrayL"), (void*)llvmSetArrayL);
	TheExecutionEngine->updateGlobalMapping(module->getFunction("__llvmSetArrayF"), (void*)llvmSetArrayF);
	TheExecutionEngine->updateGlobalMapping(module->getFunction("__llvmSetArrayD"), (void*)llvmSetArrayD);
	TheExecutionEngine->updateGlobalMapping(module->getFunction("llvmReturnInt"), (void*)llvmReturnInt);
	TheExecutionEngine->updateGlobalMapping(module->getFunction("llvmReturnLong"), (void*)llvmReturnLong);
	TheExecutionEngine->updateGlobalMapping(module->getFunction("llvmReturnDouble"), (void*)llvmReturnDouble);
	TheExecutionEngine->updateGlobalMapping(module->getFunction("llvmCloseUpvalue"), (void*)llvmCloseUpvalue);
	TheExecutionEngine->updateGlobalMapping(module->getFunction("__llvmIndexToFunction"), (void*)__llvmIndexToFunction);

	// external functions binding
	llvm::Module::FunctionListType &funcs = module->getFunctionList();
	
	mapped = new bool[exLinker->exFunctions.size()];
	for(unsigned funcID = 0; funcID < exLinker->exFunctions.size(); funcID++)
		mapped[funcID] = 0;

	for(llvm::Module::FunctionListType::iterator c = funcs.begin(), e = funcs.end(); c != e; c++)
	{
		std::string name = c->getName();

		const char *str = name.c_str();
		str += name.length() - 1;

		bool found = false;
		do 
		{
			unsigned length = unsigned(str - name.c_str() + 1);

			unsigned funcID = GetFunctionID(name.c_str(), length);

			if(funcID != ~0u)
			{
				if(exLinker->exFunctions[funcID].address == ~0u)
					TheExecutionEngine->updateGlobalMapping(c, exLinker->exFunctions[funcID].funcPtr);
				else
					exLinker->exFunctions[funcID].funcPtr = TheExecutionEngine->getPointerToFunction(c);

				found = true;
			}
			if((unsigned)(*str - '0') >= 10 || str == name)
			{
				break;
			}else{
				str--;
			}
		}while(!found);
	}
	delete[] mapped;

	// Force compilation of the main function
	for(unsigned i = 0; i < exLinker->llvmModuleSizes.size(); i++)
	{
		char buf[32];
		sprintf(buf, "Global%d", i);
		assert(module->getFunction(buf));
		TheExecutionEngine->getPointerToFunction(module->getFunction(buf));
	}

	if(!error.empty())
	{
		printf("%s\n", error.c_str());
		assert(0);
		return false;
	}

	return true;
}

void	ExecutorLLVM::Run(unsigned int functionID, const char *arguments)
{
	if(functionID != ~0u)
	{
		unsigned int dwordsToPop = (exLinker->exFunctions[functionID].bytesToPop >> 2);
		void* fPtr = exLinker->exFunctions[functionID].funcPtr;
		unsigned int retType = exLinker->exFunctions[functionID].retType;

		unsigned int *stackStart = ((unsigned int*)arguments) + dwordsToPop - 1;
		for(unsigned int i = 0; i < dwordsToPop; i++)
		{
#ifdef __GNUC__
			asm("movl %0, %%eax"::"r"(stackStart):"%eax");
			asm("pushl (%eax)");
#else
			__asm{ mov eax, dword ptr[stackStart] }
			__asm{ push dword ptr[eax] }
#endif
			stackStart--;
		}
		switch(retType)
		{
		case ExternFuncInfo::RETURN_VOID:
			((void (*)())fPtr)();
			llvmReturnedType = LLVM_NONE;
			break;
		case ExternFuncInfo::RETURN_INT:
			llvmReturnedInt = ((int (*)())fPtr)();
			llvmReturnedType = LLVM_INT;
			break;
		case ExternFuncInfo::RETURN_DOUBLE:
			llvmReturnedDouble = ((double (*)())fPtr)();
			llvmReturnedType = LLVM_DOUBLE;
			break;
		case ExternFuncInfo::RETURN_LONG:
			llvmReturnedLong = ((long long (*)())fPtr)();
			llvmReturnedType = LLVM_LONG;
			break;
		}
#ifdef __GNUC__
		asm("movl %0, %%eax"::"r"(dwordsToPop):"%eax");
		asm("leal (%esp, %eax, 0x4), %esp");
#else
		__asm{ mov eax, dwordsToPop }
		__asm{ lea esp, [eax * 4 + esp] }
#endif
		return;
	}

	int stackHelper = 0;
	llvmStackTop = &stackHelper;
	currentLinker = exLinker;

	for(unsigned i = 0; i < exLinker->llvmModuleSizes.size(); i++)
	{
		char buf[32];
		sprintf(buf, "Global%d", i);
		assert(module->getFunction(buf));
		void *FPtr = TheExecutionEngine->getPointerToFunction(module->getFunction(buf));
		void (*FP)() = (void (*)())(intptr_t)FPtr;
		assert(FP);
		FP();
	}

	globalVars.resize(exLinker->globalVarSize);
	unsigned int globalID = 0;

	llvm::Module::GlobalListType &globals = module->getGlobalList();
	for(llvm::Module::GlobalListType::iterator c = globals.begin(), e = globals.end(); c != e; c++)
	{
		if(*c->getName().begin() == '^')
			continue;

		if(globalID >= exLinker->exVariables.size())
			break;

		void *data = TheExecutionEngine->getPointerToGlobal(c);
		if(exLinker->exTypes[exLinker->exVariables[globalID].type].size)
			memcpy(&globalVars[exLinker->exVariables[globalID].offset], data, exLinker->exTypes[exLinker->exVariables[globalID].type].size);
		globalID++;
	}
}

void	ExecutorLLVM::Stop(const char* error)
{
	(void)error;
	assert(!"ExecutorLLVM::Stop");
}

const char*	ExecutorLLVM::GetResult()
{
	switch(llvmReturnedType)
	{
	case LLVM_DOUBLE:
		SafeSprintf(execResult, 64, "%f", llvmReturnedDouble);
		break;
	case LLVM_LONG:
		SafeSprintf(execResult, 64, "%lldL", llvmReturnedLong);
		break;
	case LLVM_INT:
		SafeSprintf(execResult, 64, "%d", llvmReturnedInt);
		break;
	default:
		SafeSprintf(execResult, 64, "no return value");
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

const char*	ExecutorLLVM::GetExecError()
{
	return execError;
}

char*		ExecutorLLVM::GetVariableData(unsigned int *count)
{
	globalVars.resize(exLinker->globalVarSize);
	unsigned int globalID = 0;

	llvm::Module::GlobalListType &globals = module->getGlobalList();
	for(llvm::Module::GlobalListType::iterator c = globals.begin(), e = globals.end(); c != e; c++)
	{
		if(*c->getName().begin() == '^')
			continue;

		if(globalID >= exLinker->exVariables.size())
			break;

		void *data = TheExecutionEngine->getPointerToGlobal(c);
		if(exLinker->exTypes[exLinker->exVariables[globalID].type].size)
			memcpy(&globalVars[exLinker->exVariables[globalID].offset], data, exLinker->exTypes[exLinker->exVariables[globalID].type].size);
		globalID++;
	}

	if(count)
		*count = exLinker->exVariables.size();
	return globalVars.data;
}

void			ExecutorLLVM::BeginCallStack()
{
}

unsigned int	ExecutorLLVM::GetNextAddress()
{
	return 0;
}

void*			ExecutorLLVM::GetStackStart()
{
#pragma warning(push)
#pragma warning(disable: 4172) // returning address of local variable or temporary
	int stackHelper = 0;
	return &stackHelper;
#pragma warning(pop)
}

void*			ExecutorLLVM::GetStackEnd()
{
	return llvmStackTop;
}

#endif
