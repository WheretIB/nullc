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

void* funcCreator(const std::string& name)
{
	if(name == "llvmIntPow")
		return llvmIntPow;
	if(name == "llvmLongPow")
		return llvmLongPow;
	if(name == "llvmDoublePow")
		return llvmDoublePow;
	if(name == "__moddi3")
		return llvmLongMod;
	if(name == "__divdi3")
		return llvmLongDiv;
	if(name == "__llvmSetArrayC")
		return llvmSetArrayC;
	if(name == "__llvmSetArrayS")
		return llvmSetArrayS;
	if(name == "__llvmSetArrayI")
		return llvmSetArrayI;
	if(name == "__llvmSetArrayL")
		return llvmSetArrayL;
	if(name == "__llvmSetArrayF")
		return llvmSetArrayF;
	if(name == "__llvmSetArrayD")
		return llvmSetArrayD;
	if(name == "llvmReturnInt")
		return llvmReturnInt;
	if(name == "llvmReturnLong")
		return llvmReturnLong;
	if(name == "llvmReturnDouble")
		return llvmReturnDouble;

	return NULL;
}

namespace LLVM
{
	llvm::ExecutionEngine	*TheExecutionEngine = NULL;
	llvm::LLVMContext		context;
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

void	ExecutorLLVM::Run(unsigned int functionID, const char *arguments)
{
	(void)functionID;
	(void)arguments;

	if(functionID != ~0u)
	{
		sprintf(execError, "ERROR: function call unsupported");
		return;
	}
	llvmReturnedType = LLVM_NONE;

	execError[0] = 0;

	printf("LLVM executor\n");

	if(!exLinker->llvmModuleSizes.size())
	{
		printf("Code not found\n");
		assert(0);
	}
	LLVM::modules.clear();
	LLVM::linker = new llvm::Linker("main", "main", LLVM::context);

	std::string error;

	unsigned offset = 0;
	for(unsigned i = 0; i < exLinker->llvmModuleSizes.size(); i++)
	{
		char buf[32];
		llvm::MemoryBuffer *buffer = llvm::MemoryBuffer::getMemBuffer(&exLinker->llvmModuleCodes[offset], &exLinker->llvmModuleCodes[offset + exLinker->llvmModuleSizes[i] - 1]);
		llvm::Module *module = llvm::ParseBitcodeFile(buffer, LLVM::context, &error);
		if(!error.empty())
		{
			printf("%s\n", error.c_str());
			assert(0);
		}

		llvm::Function *glob = module->getFunction("Global");
		sprintf(buf, "Global%d", i);
		glob->setName(buf);
		if(LLVM::linker->LinkInModule(module, &error))
		{
			printf("%s\n", error.c_str());
			assert(0);
		}
		offset += exLinker->llvmModuleSizes[i];
		LLVM::modules.push_back(module);
	}
	llvm::Module *module = LLVM::linker->releaseModule();
	LLVM::TheExecutionEngine = llvm::EngineBuilder(module).setErrorStr(&error).create();
	if(!LLVM::TheExecutionEngine)
	{
		printf("Could not create ExecutionEngine: %s\n", error.c_str());
		assert(0);
	}
	LLVM::TheExecutionEngine->InstallLazyFunctionCreator(funcCreator);

	/*const char* symbols = exLinker->exSymbols.data;
	llvm::Module::FunctionListType &funcs = module->getFunctionList();
	unsigned funcID = 0;
	for(llvm::Module::FunctionListType::iterator c = funcs.begin(), e = funcs.end(); c != e; c++)
	{
		const char *name = c->getNameStr().c_str();
		printf("%s\n", name);
		while(memcmp(name, exLinker->exFunctions[funcID].offsetToName + symbols,
			strlen(exLinker->exFunctions[funcID].offsetToName + symbols)) != 0)
		{
			printf("skipping %s\n", exLinker->exFunctions[funcID].offsetToName + symbols);
			funcID++;
		}
		printf("matched with %s\n", exLinker->exFunctions[funcID].offsetToName + symbols);
	}*/

	// external functions binding
	const char* symbols = exLinker->exSymbols.data;
	llvm::Module::FunctionListType &funcs = module->getFunctionList();
	llvm::Module::FunctionListType::iterator c = funcs.begin(), e = funcs.end();
	for(unsigned funcID = 0; funcID < exLinker->exFunctions.size(); funcID++)
	{
		std::string name = c->getNameStr();
		//printf("%s\n", exLinker->exFunctions[funcID].offsetToName + symbols);
		while(memcmp(name.c_str(), exLinker->exFunctions[funcID].offsetToName + symbols,
			strlen(exLinker->exFunctions[funcID].offsetToName + symbols)) != 0)
		{
			//printf("skipping %s\n", name.c_str());
			c++;
			name = c->getNameStr();
		}
		//printf("matched with %s\n", name.c_str());

		if(exLinker->exFunctions[funcID].address == ~0u)
		{
			//printf("binding %s to %s\n", exLinker->exFunctions[funcID].offsetToName + symbols, name.c_str());
			LLVM::TheExecutionEngine->updateGlobalMapping(c, exLinker->exFunctions[funcID].funcPtr);
		}
		c++;
	}

	/*// Function address array creation
	exLinker->functionAddress.resize(exLinker->exFunctions.size());
	for(unsigned i = 0; i < exLinker->exFunctions.size(); i++)
	{
		if(exLinker->exFunctions[i].address == ~0u)
		{
			exLinker->functionAddress[i] = (unsigned int)(intptr_t)exLinker->exFunctions[i].funcPtr;
		}else{
			llvm::Function *func = module->getFunction(&exLinker->exSymbols[exLinker->exFunctions[i].offsetToName]);
			if(!func)
			{
				printf("Function %s not found\n", &exLinker->exSymbols[exLinker->exFunctions[i].offsetToName]);
				exLinker->functionAddress[i] = NULL;
			}else{
				exLinker->functionAddress[i] = (unsigned int)(intptr_t)LLVM::TheExecutionEngine->getPointerToFunction(func);
			}
		}
	}*/
	
	
	/*for(unsigned i = 0; i < exLinker->exFunctions.size(); i++)
		printf("%s\n", exLinker->exFunctions[i].offsetToName + symbols);

	if(module->getFunction("typeid"))
		LLVM::TheExecutionEngine->updateGlobalMapping(module->getFunction("typeid"), (void*)llvmLongDiv);
	*/
	//LLVM::TheExecutionEngine->getPointerToGlobal
	for(unsigned i = 0; i < exLinker->llvmModuleSizes.size(); i++)
	{
		char buf[32];
		sprintf(buf, "Global%d", i);
		assert(module->getFunction(buf));
		void *FPtr = LLVM::TheExecutionEngine->getPointerToFunction(module->getFunction(buf));
		void (*FP)() = (void (*)())(intptr_t)FPtr;
		assert(FP);
		FP();
	}

	//printf("Evaluated to %s\n", GetResult());

	
	LLVM::globalVars.resize(exLinker->globalVarSize);
	unsigned int globalID = 0;
	//for(unsigned i = 0; i < LLVM::modules.size(); i++)
	//{
	//	llvm::Module *module = LLVM::modules[i];
		llvm::Module::GlobalListType &globals = module->getGlobalList();
		for(llvm::Module::GlobalListType::iterator c = globals.begin(), e = globals.end(); c != e; c++)
		{
			//llvm::GlobalVariable &var = *c;
			//const char *name = c->getNameStr().c_str();
			//printf("%s %s\n", exLinker->exVariables[globalID].offsetToName + symbols, name);
			void *data = LLVM::TheExecutionEngine->getPointerToGlobal(c);
			if(exLinker->exTypes[exLinker->exVariables[globalID].type].size)
				memcpy(&LLVM::globalVars[exLinker->exVariables[globalID].offset], data, exLinker->exTypes[exLinker->exVariables[globalID].type].size);
			globalID++;
			/*
			for(unsigned l = 0; l < exLinker->exVariables.size(); l++)
			{
				if(strcmp(name, symbols + exLinker->exVariables[l].offsetToName) == 0)
					memcpy(&LLVM::globalVars[exLinker->exVariables[l].offset], data, exLinker->exTypes[exLinker->exVariables[l].type].size);
			}*/
		}
	//}

	delete LLVM::TheExecutionEngine;
	delete LLVM::linker;
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
	return LLVM::globalVars.data;
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
	return NULL;
}

void*			ExecutorLLVM::GetStackEnd()
{
	return NULL;
}

#endif
