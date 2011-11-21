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
	llvm::Linker			*linker;

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
			mapped[funcID] = 1;

			// Skip generic base functions
			if(exLinker->exFunctions[funcID].funcType == 0)
				continue;

			return funcID;
		}
	}
	return ~0u;
}

void	ExecutorLLVM::Run(unsigned int functionID, const char *arguments)
{
	(void)functionID;
	(void)arguments;

	if(functionID != ~0u)
	{
		sprintf(execError, "ERROR: function call unsupported");
		return;
	}

	delete ctx;
	ctx = new ContextHolder();

	CommonSetLinker(exLinker);

	llvmReturnedType = LLVM_NONE;

	execError[0] = 0;

	if(!exLinker->llvmModuleSizes.size())
	{
		printf("Code not found\n");
		assert(0);
	}
	modules.clear();
	linker = new llvm::Linker("main", "main", getContext());

	std::string error;

	unsigned offset = 0;
	for(unsigned i = 0; i < exLinker->llvmModuleSizes.size(); i++)
	{
		char buf[32];
		llvm::MemoryBuffer *buffer = llvm::MemoryBuffer::getMemBuffer(llvm::StringRef(&exLinker->llvmModuleCodes[offset], exLinker->llvmModuleSizes[i]), "module", false);
		llvm::Module *module = llvm::ParseBitcodeFile(buffer, getContext(), &error);
		if(!error.empty())
		{
			printf("%s\n", error.c_str());
			assert(0);
		}

		llvm::Function *glob = module->getFunction("Global");
		sprintf(buf, "Global%d", i);
		glob->setName(buf);
		if(linker->LinkInModule(module, &error))
		{
			printf("%s\n", error.c_str());
			assert(0);
		}
		offset += exLinker->llvmModuleSizes[i];
		modules.push_back(module);
	}
	llvm::Module *module = linker->releaseModule();
	TheExecutionEngine = llvm::EngineBuilder(module).setErrorStr(&error).create();
	if(!TheExecutionEngine)
	{
		printf("Could not create ExecutionEngine: %s\n", error.c_str());
		assert(0);
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

	// external functions binding
	llvm::Module::FunctionListType &funcs = module->getFunctionList();
	
	mapped = new bool[exLinker->exFunctions.size()];
	for(unsigned funcID = 0; funcID < exLinker->exFunctions.size(); funcID++)
		mapped[funcID] = 0;

	for(llvm::Module::FunctionListType::iterator c = funcs.begin(), e = funcs.end(); c != e; c++)
	{
		std::string name = c->getNameStr();

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

	int stackHelper = 0;
	llvmStackTop = &stackHelper;

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
		if(globalID >= exLinker->exVariables.size())
			break;

		void *data = TheExecutionEngine->getPointerToGlobal(c);
		if(exLinker->exTypes[exLinker->exVariables[globalID].type].size)
			memcpy(&globalVars[exLinker->exVariables[globalID].offset], data, exLinker->exTypes[exLinker->exVariables[globalID].type].size);
		globalID++;
	}

	delete TheExecutionEngine;
	delete linker;
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
