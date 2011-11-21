#include "SyntaxTree.h"
#include "CodeInfo.h"

#ifdef NULLC_LLVM_SUPPORT

#pragma warning(push)
#pragma warning(disable: 4530 4512 4800 4146 4244 4245 4146 4355 4100 4267)

#include <time.h>

#include "llvm/DerivedTypes.h"
#include "llvm/LLVMContext.h"
#include "llvm/Module.h"
#include "llvm/Analysis/Verifier.h"
#include "llvm/Support/IRBuilder.h"
#include "llvm/Support/raw_ostream.h"

#include "llvm/Bitcode/BitstreamReader.h"
#include "llvm/Bitcode/BitstreamWriter.h"
#include "llvm/Support/IRReader.h"

#include "llvm/PassManager.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/JIT.h"
#include "llvm/Target/TargetData.h"
#include "llvm/Support/TargetSelect.h"

#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/IPO.h"

#include "llvm/Support/TypeBuilder.h"
#include "llvm/Support/ManagedStatic.h"

#include <cstdio>
#include <string>
#include <map>
#include <vector>
#include <iostream>
using namespace llvm;

#pragma comment(lib, "LLVMCore.lib")
#pragma comment(lib, "LLVMSupport.lib")
#pragma comment(lib, "LLVMSystem.lib")

#pragma comment(lib, "LLVMTarget.lib")
#pragma comment(lib, "LLVMAnalysis.lib")
#pragma comment(lib, "LLVMExecutionEngine.lib")
#pragma comment(lib, "LLVMJIT.lib")
#pragma comment(lib, "LLVMCodeGen.lib")
#pragma comment(lib, "LLVMScalarOpts.lib")
#pragma comment(lib, "LLVMInstCombine.lib")
#pragma comment(lib, "LLVMTransformUtils.lib")
#pragma comment(lib, "LLVMMC.lib")

#pragma comment(lib, "LLVMInterpreter.lib")
#pragma comment(lib, "LLVMX86CodeGen.lib")
#pragma comment(lib, "LLVMSelectionDAG.lib")
#pragma comment(lib, "LLVMX86Info.lib")
#pragma comment(lib, "LLVMAsmPrinter.lib")
#pragma comment(lib, "LLVMipa.lib")
#pragma comment(lib, "LLVMipo.lib")

#pragma comment(lib, "LLVMBitReader.lib")
#pragma comment(lib, "LLVMBitWriter.lib")

#pragma comment(lib, "LLVMX86Utils.lib")
#pragma comment(lib, "LLVMX86Desc.lib")
#pragma comment(lib, "LLVMX86AsmPrinter.lib")

#pragma warning(pop)

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

	llvm::Module	*module = 0;

	llvm::IRBuilder<>	*builder = 0;

	ExecutionEngine *TheExecutionEngine = NULL;
	FunctionPassManager *OurFPM = NULL;
	PassManager *OurPM = NULL;

	std::vector<GlobalVariable*>	globals;

	bool	enableOptimization = false;

	FastVector<BasicBlock*>	breakStack;
	FastVector<BasicBlock*> continueStack;

	llvm::Function *F = NULL;

	llvm::Type		*typeClosure = NULL;
	llvm::Function	*functionCloseUpvalues = NULL;
}

void		StartLLVMGeneration(unsigned functionsInModules)
{
	InitializeNativeTarget();

	delete module;
	module = 0;

	delete builder;
	builder = 0;

	delete ctx;
	ctx = 0;

	ctx = new ContextHolder();

	builder = new llvm::IRBuilder<>(getContext());

	module = new llvm::Module("NULLCCode", getContext());

	std::string ErrStr;
	TheExecutionEngine = EngineBuilder(module).setErrorStr(&ErrStr).create();
	if(!TheExecutionEngine)
	{
		printf("Could not create ExecutionEngine: %s\n", ErrStr.c_str());
		exit(1);
	}
	//TheExecutionEngine->InstallLazyFunctionCreator(funcCreator);

	OurPM = new PassManager();

	OurFPM = new FunctionPassManager(module);

	// Set up the optimizer pipeline.  Start with registering info about how the
	// target lays out data structures.
	OurFPM->add(new TargetData(*TheExecutionEngine->getTargetData()));
	// Promote allocas to registers.
	OurFPM->add(createPromoteMemoryToRegisterPass());

	// Do simple "peephole" optimizations and bit-twiddling optzns.
	OurFPM->add(createInstructionCombiningPass());
	// Reassociate expressions.
	OurFPM->add(createReassociatePass());
	// Eliminate Common SubExpressions.
	OurFPM->add(createGVNPass());
	// Simplify the control flow graph (deleting unreachable blocks, etc).
	OurFPM->add(createCFGSimplificationPass());

	OurFPM->add(createConstantPropagationPass());

	OurPM->add(createFunctionInliningPass());

	OurFPM->doInitialization();

	//OurFPM->doFinalization();

	for(unsigned int i = 0; i < CodeInfo::typeInfo.size(); i++)
	{
		CodeInfo::typeInfo[i]->llvmType = NULL;
	}

	// Generate types
	typeVoid->llvmType = Type::getVoidTy(getContext());
	typeBool->llvmType = Type::getInt1Ty(getContext());
	typeChar->llvmType = Type::getInt8Ty(getContext());
	typeShort->llvmType = Type::getInt16Ty(getContext());
	typeInt->llvmType = Type::getInt32Ty(getContext());
	typeLong->llvmType = Type::getInt64Ty(getContext());
	typeFloat->llvmType = Type::getFloatTy(getContext());
	typeDouble->llvmType = Type::getDoubleTy(getContext());
	typeObject->llvmType = StructType::get((Type*)typeInt->llvmType, Type::getInt8PtrTy(getContext()), (Type*)NULL);
	typeTypeid->llvmType = typeInt->llvmType;
	typeAutoArray->llvmType = StructType::get((Type*)typeInt->llvmType, Type::getInt8PtrTy(getContext()), (Type*)typeInt->llvmType, (Type*)NULL);

	typeFunction->llvmType = llvm::PointerType::getUnqual(llvm::FunctionType::get(llvm::Type::getVoidTy(getContext()), false));

	std::vector<Type*> typeList;
	// Construct abstract types for all classes
	for(unsigned int i = 0; i < CodeInfo::typeInfo.size(); i++)
	{
		TypeInfo *type = CodeInfo::typeInfo[i];
		if(type->llvmType || type->arrLevel || type->refLevel || type->funcType)
			continue;
		//printf("Adding opaque %s\n", type->GetFullTypeName());
		type->llvmType = StructType::create(getContext(), type->GetFullTypeName());
	}
	for(unsigned int i = 0; i < CodeInfo::typeInfo.size(); i++)
	{
		TypeInfo *type = CodeInfo::typeInfo[i];
		if(type->llvmType)
			continue;

		llvm::Type *subType = type->subType ? (llvm::Type*)type->subType->llvmType : NULL;
		//printf("Adding %s\n", type->GetFullTypeName());
		if(type->arrLevel && type->arrSize != TypeInfo::UNSIZED_ARRAY)
		{
			assert(subType);
			unsigned size = type->subType->size * type->arrSize;
			size = (size + 3) & ~3;
			type->llvmType = ArrayType::get(subType, type->subType->size ? size / type->subType->size : type->arrSize);
		}else if(type->arrLevel && type->arrSize == TypeInfo::UNSIZED_ARRAY){
			assert(subType);
			type->llvmType = StructType::get(PointerType::getUnqual(subType), Type::getInt32Ty(getContext()), (Type*)NULL);

			/*type->llvmType = StructType::create(getContext(), type->GetFullTypeName());
			typeList.clear();
			typeList.push_back(PointerType::getUnqual(subType));
			typeList.push_back(Type::getInt32Ty(getContext()));
			((StructType*)type->llvmType)->setBody(llvm::ArrayRef<Type*>(typeList));*/
		}else if(type->refLevel){
			assert(subType);
			type->llvmType = type->subType == typeVoid ? Type::getInt8PtrTy(getContext()) : PointerType::getUnqual(subType);
		}else if(type->funcType){
			typeList.clear();
			for(unsigned int k = 0; k < type->funcType->paramCount; k++)
			{
				TypeInfo *argTypeInfo = type->funcType->paramType[k];
				llvm::Type *argType = (Type*)argTypeInfo->llvmType;
				assert(argType);
				typeList.push_back(argType);
			}
			typeList.push_back(Type::getInt8PtrTy(getContext()));
			llvm::Type *retType = (Type*)type->funcType->retType->llvmType;
			assert(retType);
			Type *functionType = llvm::FunctionType::get(retType, llvm::ArrayRef<Type*>(typeList), false);
			type->llvmType = StructType::create(getContext(), type->GetFullTypeName());
			typeList.clear();
			typeList.push_back(Type::getInt8PtrTy(getContext()));
			typeList.push_back(PointerType::getUnqual(functionType));
			((StructType*)type->llvmType)->setBody(llvm::ArrayRef<Type*>(typeList));
			//type->llvmType = StructType::get(Type::getInt8PtrTy(getContext()), PointerType::getUnqual(functionType), (Type*)NULL);
		}
	}
	for(unsigned int i = 0; i < CodeInfo::typeInfo.size(); i++)
	{
		TypeInfo *type = CodeInfo::typeInfo[i];

		if(type->arrLevel || type->refLevel || type->funcType || (type->type != TypeInfo::TYPE_COMPLEX && type->firstVariable == NULL) || type == typeObject || type == typeTypeid || type == typeAutoArray)
			continue;

		if(type->type != TypeInfo::TYPE_COMPLEX && type->firstVariable != NULL)
		{
			type->llvmType = Type::getInt32Ty(getContext());
			continue;
		}

		typeList.clear();
		for(TypeInfo::MemberVariable *curr = type->firstVariable; curr; curr = curr->next)
			typeList.push_back((Type*)curr->type->llvmType);

		((StructType*)type->llvmType)->setBody(llvm::ArrayRef<Type*>(typeList));
	}

	// Generate global variables
	for(unsigned int i = 0; i < CodeInfo::varInfo.size(); i++)
	{
		InplaceStr name = CodeInfo::varInfo[i]->name;
		globals.push_back(new GlobalVariable(*module, (Type*)CodeInfo::varInfo[i]->varType->llvmType, false,
			CodeInfo::varInfo[i]->pos >> 24 ? llvm::GlobalVariable::ExternalWeakLinkage : llvm::GlobalVariable::ExternalLinkage,
			0, std::string(name.begin, name.end)));
		CodeInfo::varInfo[i]->llvmValue = globals.back();
		globals.back()->setInitializer(Constant::getNullValue((Type*)CodeInfo::varInfo[i]->varType->llvmType));
		//printf("global %s %.*s\n", CodeInfo::varInfo[i]->pos >> 24 ? "extern" : "", int(name.end - name.begin), name.begin);
	}

	typeClosure = llvm::StructType::create(getContext(), "#upvalue");
	typeList.clear();
	typeList.push_back(llvm::Type::getInt8PtrTy(getContext()));
	typeList.push_back(llvm::PointerType::getUnqual(typeClosure));
	typeList.push_back(llvm::Type::getInt32Ty(getContext()));
	llvm::cast<StructType>(typeClosure)->setBody(llvm::ArrayRef<Type*>(typeList));

	typeList.clear();
	typeList.push_back(llvm::PointerType::getUnqual(llvm::PointerType::getUnqual(typeClosure)));
	typeList.push_back(llvm::Type::getInt8PtrTy(getContext()));
	functionCloseUpvalues = Function::Create(llvm::FunctionType::get(Type::getVoidTy(getContext()), llvm::ArrayRef<Type*>(typeList), false), Function::ExternalLinkage, "llvmCloseUpvalue", module);

	Function::Create(TypeBuilder<types::i<32>(types::i<32>, types::i<32>), true>::get(getContext()), Function::ExternalLinkage, "llvmIntPow", module);
	Function::Create(TypeBuilder<types::i<64>(types::i<64>, types::i<64>), true>::get(getContext()), Function::ExternalLinkage, "llvmLongPow", module);
	Function::Create(TypeBuilder<types::ieee_double(types::ieee_double, types::ieee_double), true>::get(getContext()), Function::ExternalLinkage, "llvmDoublePow", module);

	std::vector<llvm::Type*> arrSetParams;
	arrSetParams.push_back(Type::getInt8PtrTy(getContext()));
	arrSetParams.push_back(Type::getInt8Ty(getContext()));
	arrSetParams.push_back(Type::getInt32Ty(getContext()));
	Function::Create(llvm::FunctionType::get(Type::getVoidTy(getContext()), arrSetParams, false), Function::ExternalLinkage, "__nullcSetArrayC", module);

	arrSetParams[0] = Type::getInt16PtrTy(getContext());
	arrSetParams[1] = Type::getInt16Ty(getContext());
	Function::Create(llvm::FunctionType::get(Type::getVoidTy(getContext()), arrSetParams, false), Function::ExternalLinkage, "__llvmSetArrayS", module);

	arrSetParams[0] = Type::getInt32PtrTy(getContext());
	arrSetParams[1] = Type::getInt32Ty(getContext());
	Function::Create(llvm::FunctionType::get(Type::getVoidTy(getContext()), arrSetParams, false), Function::ExternalLinkage, "__llvmSetArrayI", module);

	arrSetParams[0] = Type::getInt64PtrTy(getContext());
	arrSetParams[1] = Type::getInt64Ty(getContext());
	Function::Create(llvm::FunctionType::get(Type::getVoidTy(getContext()), arrSetParams, false), Function::ExternalLinkage, "__llvmSetArrayL", module);

	arrSetParams[0] = Type::getFloatPtrTy(getContext());
	arrSetParams[1] = Type::getFloatTy(getContext());
	Function::Create(llvm::FunctionType::get(Type::getVoidTy(getContext()), arrSetParams, false), Function::ExternalLinkage, "__llvmSetArrayF", module);

	arrSetParams[0] = Type::getDoublePtrTy(getContext());
	arrSetParams[1] = Type::getDoubleTy(getContext());
	Function::Create(llvm::FunctionType::get(Type::getVoidTy(getContext()), arrSetParams, false), Function::ExternalLinkage, "__llvmSetArrayD", module);

	Function::Create(TypeBuilder<void(types::i<32>), true>::get(getContext()), Function::ExternalLinkage, "llvmReturnInt", module);
	Function::Create(TypeBuilder<void(types::i<64>), true>::get(getContext()), Function::ExternalLinkage, "llvmReturnLong", module);
	Function::Create(TypeBuilder<void(types::ieee_double), true>::get(getContext()), Function::ExternalLinkage, "llvmReturnDouble", module);	

	std::vector<llvm::Type*> Arguments;
	for(unsigned int i = 0; i < CodeInfo::funcInfo.size(); i++)
	{
		FunctionInfo *funcInfo = CodeInfo::funcInfo[i];

		if(!funcInfo->retType)
			continue;

		if((funcInfo->address & 0x80000000) && funcInfo->address != -1)
			continue;

		Arguments.clear();
		VariableInfo *curr = NULL;
		for(curr = funcInfo->firstParam; curr; curr = curr->next)
			Arguments.push_back((Type*)curr->varType->llvmType);
		Arguments.push_back(funcInfo->extraParam ? (Type*)funcInfo->extraParam->varType->llvmType : Type::getInt32PtrTy(getContext()));
		llvm::FunctionType *FT = llvm::FunctionType::get((Type*)funcInfo->retType->llvmType, llvm::ArrayRef<Type*>(Arguments), false);
		funcInfo->llvmFunction = llvm::Function::Create(FT, i < functionsInModules ? llvm::Function::ExternalWeakLinkage : llvm::Function::ExternalLinkage, funcInfo->name, module);

		// Generate closures
		if(i >= functionsInModules)
		{
			char name[NULLC_MAX_VARIABLE_NAME_LENGTH];

			for(VariableInfo *local = funcInfo->firstParam; local; local = local->next)
			{
				if(local->usedAsExternal)
				{
					SafeSprintf(name, NULLC_MAX_VARIABLE_NAME_LENGTH, "#upvalue#%.*s", local->name.end - local->name.begin, local->name.begin);
					globals.push_back(new GlobalVariable(*module, llvm::PointerType::getUnqual(typeClosure), false, llvm::GlobalVariable::ExternalLinkage, 0, name));
					local->llvmUpvalue = globals.back();
					globals.back()->setInitializer(Constant::getNullValue(llvm::PointerType::getUnqual(typeClosure)));
				}
			}
			if(funcInfo->closeUpvals)
			{
				SafeSprintf(name, NULLC_MAX_VARIABLE_NAME_LENGTH, "#upvalue#%s", funcInfo->name);
				globals.push_back(new GlobalVariable(*module, llvm::PointerType::getUnqual(typeClosure), false, llvm::GlobalVariable::ExternalLinkage, 0, name));
				funcInfo->extraParam->llvmUpvalue = globals.back();
				globals.back()->setInitializer(Constant::getNullValue(llvm::PointerType::getUnqual(typeClosure)));
			}

			for(VariableInfo *local = funcInfo->firstLocal; local; local = local->next)
			{
				if(local->usedAsExternal)
				{
					SafeSprintf(name, NULLC_MAX_VARIABLE_NAME_LENGTH, "#upvalue#%.*s", local->name.end - local->name.begin, local->name.begin);
					globals.push_back(new GlobalVariable(*module, llvm::PointerType::getUnqual(typeClosure), false, llvm::GlobalVariable::ExternalLinkage, 0, name));
					local->llvmUpvalue = globals.back();
					globals.back()->setInitializer(Constant::getNullValue(llvm::PointerType::getUnqual(typeClosure)));
				}
			}
		}
	}

	for(unsigned int i = 0; i < CodeInfo::funcInfo.size(); i++)
	{
		FunctionInfo *funcInfo = CodeInfo::funcInfo[i];

		if(funcInfo->retType && (funcInfo->address & 0x80000000) && funcInfo->address != -1)
			funcInfo->llvmFunction = (llvm::Function*)CodeInfo::funcInfo[funcInfo->address & ~0x80000000]->llvmFunction;
	}

	breakStack.clear();
	continueStack.clear();
}

struct my_stream : public llvm::raw_ostream
{
	virtual void write_impl(const char * a,size_t b)
	{
		printf("%.*s", b, a);
	}
	uint64_t current_pos(void) const
	{
		return 0;
	}
};

//#define DUMP(x){ x->dump(); printf("\n"); }
#define DUMP(x) x

//BasicBlock *globalExit = NULL;

void		StartGlobalCode()
{
	F = llvm::Function::Create(TypeBuilder<void(), true>::get(getContext()), llvm::Function::ExternalLinkage, "Global", module);

	llvm::BasicBlock *BB = llvm::BasicBlock::Create(getContext(), "global_entry", F);
	builder->SetInsertPoint(BB);

	//globalExit = BasicBlock::Create(getContext(), "globalReturn");
}

std::vector<unsigned char> out;
const char*	GetLLVMIR(unsigned& length)
{
	//if(F->back().empty() || !F->back().back().isTerminator())
		builder->CreateRetVoid();

	/*F->getBasicBlockList().push_back(globalExit);
	builder->SetInsertPoint(globalExit);
	builder->CreateRet(ConstantInt::get(getContext(), APInt(32, ~0ull, true)));*/

	if(enableOptimization)
	{
		if(!F->getEntryBlock().empty())
			OurFPM->run(*F);
		OurPM->run(*module);
		if(!F->getEntryBlock().empty())
			OurFPM->run(*F);
	}

	DUMP(module);

//	unsigned start = clock();
	//module->dump();
	verifyFunction(*F);
	verifyModule(*module);

//	unsigned verified = clock() - start;
//	start = clock();

	out.clear();
	BitstreamWriter bw = BitstreamWriter(out);
	WriteBitcodeToStream(module, bw);
	length = (unsigned)out.size();
	
	//printf("%d\n", out.size());

	/*if(!F->getEntryBlock().empty())
	{
		void *FPtr = TheExecutionEngine->getPointerToFunction(F);
		unsigned compiled = clock() - start;
		start = clock();
		int (*FP)() = (int (*)())(intptr_t)FPtr;
		printf("Evaluated to %d (verification %d; compilation %d; execution %d)\n", FP(), verified, compiled, clock() - start);
		for(std::vector<void*>::iterator c = memBlocks.begin(), e = memBlocks.end(); c != e; c++)
			free(*c);
		memBlocks.clear();
	}*/

	return (char*)&out[0];
}
llvm::Value *V;


void	EndLLVMGeneration()
{
	//delete llvm::getPassRegistrar();
	//for(unsigned int i = 0; i < globals.size(); i++)
	//	delete globals[i];
	//globals.clear();
	/*for(unsigned int i = 0; i < CodeInfo::typeInfo.size(); i++)
		if(((llvm::Type*)CodeInfo::typeInfo[i]->llvmType)->isAbstract())
			((llvm::Type*)CodeInfo::typeInfo[i]->llvmType)->dropRef();*/

	//TheExecutionEngine->~ExecutionEngine();
	delete OurPM;
	OurPM = NULL;
	delete OurFPM;
	OurFPM = NULL;
	V = NULL;
	F = NULL;
	delete module;
	module = NULL;

	//getContext().~LLVMContext();
	//builder->~IRBuilder();
	//for(unsigned int i = 0; i < globals.size(); i++)
	//	delete globals[i];
	//globals.clear();
//	llvm_shutdown();
}

Value*	PromoteToStackType(Value *V, TypeInfo *type)
{
	asmDataType	fDT = type->dataType;
	switch(fDT)
	{
	case DTYPE_CHAR:
	case DTYPE_SHORT:
		V = builder->CreateIntCast(V, Type::getInt32Ty(getContext()), true, "tmp_toint");
		break;
	case DTYPE_FLOAT:
		V = builder->CreateFPCast(V, Type::getDoubleTy(getContext()), "tmp_ftod");
		break;
	}
	return V;
}

Value*	ConvertFirstToSecond(Value *V, TypeInfo *firstType, TypeInfo *secondType)
{
	asmDataType	fDT = firstType->dataType, sDT = secondType->dataType;
	asmStackType first = stackTypeForDataType(fDT), second = stackTypeForDataType(sDT);
	if(second == STYPE_DOUBLE)
	{
		if(first == STYPE_INT)
			return builder->CreateSIToFP(V, (Type*)secondType->llvmType, "tmp_itod");
		else if(first == STYPE_LONG)
			return builder->CreateSIToFP(V, (Type*)secondType->llvmType, "tmp_ltod");
		else if(fDT != sDT)
			return builder->CreateFPCast(V, (Type*)secondType->llvmType, "tmp_fpcast");
	}else if(second == STYPE_LONG){
		if(first == STYPE_INT)
			return builder->CreateIntCast(V, (Type*)secondType->llvmType, true, "tmp_itol");
		else if(first == STYPE_DOUBLE)
			return builder->CreateFPToSI(V, (Type*)secondType->llvmType, "tmp_dtol");
		else if(fDT != sDT)
			return builder->CreateIntCast(V, (Type*)secondType->llvmType, true, "tmp_lcast");
	}else if(second == STYPE_INT){
		if(first == STYPE_DOUBLE)
			return builder->CreateFPToSI(V, (Type*)secondType->llvmType, "tmp_dtoi");
		else if(first == STYPE_LONG)
			return builder->CreateIntCast(V, (Type*)secondType->llvmType, true, "tmp_ltoi");
		else if(fDT != sDT)
			return builder->CreateIntCast(V, (Type*)secondType->llvmType, true, "tmp_icast");
	}
	return V;
}

Value*	ConvertFirstForSecond(Value *V, TypeInfo *firstType, TypeInfo *secondType)
{
	asmDataType	fDT = firstType->dataType, sDT = secondType->dataType;
	asmStackType first = stackTypeForDataType(fDT), second = stackTypeForDataType(sDT);
	if(first == STYPE_INT && second == STYPE_DOUBLE)
		return builder->CreateSIToFP(V, Type::getDoubleTy(getContext()), "tmp_itod");
	if(first == STYPE_LONG && second == STYPE_DOUBLE)
		return builder->CreateSIToFP(V, Type::getDoubleTy(getContext()), "tmp_ltod");
	if(first == STYPE_INT && second == STYPE_LONG)
		return builder->CreateIntCast(V, Type::getInt64Ty(getContext()), true, "tmp_itol");

	return V;
}

TypeInfo*	GetStackType(TypeInfo* type)
{
	if(type == typeFloat)
		return typeDouble;
	if(type == typeChar || type == typeShort)
		return typeInt;
	return type;
}

void NodeZeroOP::CompileLLVM()
{
	V = NULL;
}

void NodeZeroOP::CompileLLVMExtra()
{
	NodeZeroOP *curr = head;
	while(curr)
	{
		curr->CompileLLVM();
		curr = curr->next;
	}
}

void NodeNumber::CompileLLVM()
{
	switch(typeInfo->dataType)
	{
	case DTYPE_CHAR:
		V = ConstantInt::get(getContext(), APInt(8, num.integer, true));
		break;
	case DTYPE_SHORT:
		V = ConstantInt::get(getContext(), APInt(16, num.integer, true));
		break;
	case DTYPE_INT:
		if(typeInfo->refLevel && num.integer == 0)
			V = ConstantPointerNull::get((PointerType*)typeInfo->llvmType);
		else
			V = ConstantInt::get(getContext(), APInt(32, num.integer, true));
		break;
	case DTYPE_LONG:
		if(typeInfo->refLevel && num.integer64 == 0)
			V = ConstantPointerNull::get((PointerType*)typeInfo->llvmType);
		else
			V = ConstantInt::get(getContext(), APInt(64, num.integer64, true));
		break;
	case DTYPE_FLOAT:
		V = ConstantFP::get(getContext(), APFloat((float)num.real));
		break;
	case DTYPE_DOUBLE:
		V = ConstantFP::get(getContext(), APFloat(num.real));
		break;
	default:
		assert(!"unsupported data type");
	}
}

void NodeReturnOp::CompileLLVM()
{
	CompileLLVMExtra();

	//if(typeInfo && first->typeInfo != typeInfo)
	//	ThrowError(CodeInfo::lastKnownStartPos, "ERROR: NodeReturnOp typeInfo && first->typeInfo != typeInfo");
	first->CompileLLVM();
	if(typeInfo != typeVoid && !V)
		ThrowError(CodeInfo::lastKnownStartPos, "ERROR: NodeReturnOp typeInfo != typeVoid && !V");
	if(typeInfo != typeVoid)
	{
		//value = PromoteToStackType(value, second->typeInfo);
		//value = ConvertFirstToSecond(value, GetStackType(second->typeInfo), first->typeInfo->subType);
		V = PromoteToStackType(V, first->typeInfo);
		V = ConvertFirstToSecond(V, GetStackType(first->typeInfo), typeInfo);
	}


	llvm::BasicBlock *returnBlock = llvm::BasicBlock::Create(getContext(), "return_block", F);
	llvm::BasicBlock *afterReturn = llvm::BasicBlock::Create(getContext(), "after_break", F);

	builder->CreateBr(returnBlock);
	//builder->CreateCondBr(llvm::ConstantInt::get(llvm::Type::getInt1Ty(getContext()), 1), returnBlock, afterReturn);
	builder->SetInsertPoint(returnBlock);
	

	if(!parentFunction)
	{//&& first->typeInfo != typeInt)
	//	ThrowError(CodeInfo::lastKnownStartPos, "ERROR: NodeReturnOp !typeInfo && first->typeInfo != typeInt");
		V = PromoteToStackType(V, first->typeInfo);
		TypeInfo *retType = GetStackType(first->typeInfo);
		if(retType == typeInt || retType == typeBool || (retType->type != TypeInfo::TYPE_COMPLEX && retType->firstVariable != NULL))
			builder->CreateCall(module->getFunction("llvmReturnInt"), V);
		else if(retType == typeLong)
			builder->CreateCall(module->getFunction("llvmReturnLong"), V);
		else if(retType == typeFloat || retType == typeDouble)
			builder->CreateCall(module->getFunction("llvmReturnDouble"), V);
		else
			ThrowError(CodeInfo::lastKnownStartPos, "ERROR: unknown global return type");
		builder->CreateRetVoid();
		V = NULL;
	}else{

		if(parentFunction && parentFunction->closeUpvals)
		{
			// Glue together parameter list, extra parameter and local list. Every list could be empty.
			VariableInfo *curr = parentFunction->firstParam ? parentFunction->firstParam : (parentFunction->firstLocal ? parentFunction->firstLocal : parentFunction->extraParam);
			if(parentFunction->firstParam)
				parentFunction->lastParam->next = (parentFunction->firstLocal ? parentFunction->firstLocal : parentFunction->extraParam);
			if(parentFunction->firstLocal)
				parentFunction->lastLocal->next = parentFunction->extraParam;
			for(; curr; curr = curr->next)
			{
				if(curr->usedAsExternal)
				{
					//curr->llvmUpvalue->dump();
					//curr->llvmUpvalue->getType()->dump(); printf("\n");

					llvm::Value *ptr = curr->llvmValue;

					//ptr->dump();
					//ptr->getType()->dump(); printf("\n");

					ptr = builder->CreatePointerCast(ptr, llvm::Type::getInt8PtrTy(getContext()));

					builder->CreateCall2(functionCloseUpvalues, curr->llvmUpvalue, ptr);
					/*const char *namePrefix = *curr->name.begin == '$' ? "__" : "";
					unsigned int nameShift = *curr->name.begin == '$' ? 1 : 0;
					sprintf(name, "%s%.*s_%d", namePrefix, int(curr->name.end - curr->name.begin) -nameShift, curr->name.begin+nameShift, curr->pos);
					GetEscapedName(name);
					OutputIdent(fOut);
					if(curr->nameHash == hashThis)
						fprintf(fOut, "__nullcCloseUpvalue(__upvalue_%d___context, &__context);\r\n", CodeInfo::FindFunctionByPtr(curr->parentFunction));
					else
						fprintf(fOut, "__nullcCloseUpvalue(__upvalue_%d_%s, &%s);\r\n", CodeInfo::FindFunctionByPtr(curr->parentFunction), name, name);*/
				}
			}
			if(parentFunction->firstParam)
				parentFunction->lastParam->next = NULL;
			if(parentFunction->firstLocal)
				parentFunction->lastLocal->next = NULL;
		}
		// If return is from coroutine, we either need to reset jumpOffset to the beginning of a function, or set it to instruction after return
		if(parentFunction && parentFunction->type == FunctionInfo::COROUTINE)
			ThrowError(CodeInfo::lastKnownStartPos, "ERROR: NodeReturnOp yield unsupported");
		
		if(typeInfo == typeBool)
		{
			V = builder->CreateIntCast(V, (Type*)typeInfo->llvmType, true, "tmp_to_bool");
			DUMP(V->getType());
		}

		builder->CreateRet(V);
	}

	builder->SetInsertPoint(afterReturn);

	//BasicBlock *afterReturn = BasicBlock::Create(getContext(), "afterReturn", F);
	//builder->SetInsertPoint(afterReturn);

	//builder->CreateRetVoid();*/
}

// CreateEntryBlockAlloca - Create an alloca instruction in the entry block of the function.  This is used for mutable variables etc.
AllocaInst* CreateEntryBlockAlloca(Function *TheFunction, const std::string &VarName, llvm::Type* type)
{
	IRBuilder<> TmpB(&TheFunction->getEntryBlock(), TheFunction->getEntryBlock().begin());
	return TmpB.CreateAlloca(type, 0, VarName.c_str());
}

void NodeExpressionList::CompileLLVM()
{
	Value *aggr = NULL;
	if(typeInfo->arrLevel)// && typeInfo->arrSize == TypeInfo::UNSIZED_ARRAY)
	{
		aggr = CreateEntryBlockAlloca(F, "arr_tmp", (Type*)typeInfo->llvmType);
	}
	if(typeInfo == typeAutoArray)
	{
		aggr = CreateEntryBlockAlloca(F, "tmp", (Type*)typeInfo->llvmType);
	}
	
	//else if(typeInfo != typeVoid)
	//	ThrowError(CodeInfo::lastKnownStartPos, "ERROR: NodeExpressionList typeInfo != typeVoid");
	int index = 0;
	if(typeInfo->arrLevel)
		index = typeInfo->arrSize == TypeInfo::UNSIZED_ARRAY ? 1 : (typeInfo->subType == typeChar ? ((typeInfo->arrSize + 3) / 4) - 1 : typeInfo->arrSize - 1);
	NodeZeroOP	*curr = first;
	do 
	{
		curr->CompileLLVM();
		if(typeInfo != typeVoid && curr->typeInfo == typeInfo)
		{
			Value *retVal = V;
			while(curr->next)
			{
				curr->next->CompileLLVM();
				assert(curr->next->typeInfo == typeVoid);
				curr = curr->next;
			}
			V = retVal;
			return;
		}
		if(V && typeInfo->arrLevel && typeInfo->arrSize == TypeInfo::UNSIZED_ARRAY)
		{
			DUMP(V->getType());
			if(index == 0)
			{
				//assert(typeInfo->subType->refType);
				V = builder->CreatePointerCast(V, PointerType::getUnqual((Type*)typeInfo->subType->llvmType), "ptr_any");
				DUMP(V->getType());
			}
			Value *arrayIndexHelper[2];
			DUMP(aggr->getType());
			arrayIndexHelper[0] = ConstantInt::get(getContext(), APInt(64, 0, true));
			arrayIndexHelper[1] = ConstantInt::get(getContext(), APInt(32, index--, true));
			Value *target = builder->CreateGEP(aggr, llvm::ArrayRef<Value*>(&arrayIndexHelper[0], 2), "arr_part");
			DUMP(target->getType());
			builder->CreateStore(V, target);
		}else if(V && typeInfo->arrLevel && curr->nodeType != typeNodeZeroOp){
			DUMP(V->getType());
			V = PromoteToStackType(V, curr->typeInfo);
			DUMP(V->getType());
			DUMP(aggr->getType());
			Value *arrayIndexHelper[2];
			arrayIndexHelper[0] = ConstantInt::get(getContext(), APInt(64, 0, true));
			//printf("Indexing %d\n", typeInfo->subType == typeChar ? index * 4 : index);
			arrayIndexHelper[1] = ConstantInt::get(getContext(), APInt(32, typeInfo->subType == typeChar ? index * 4 : index, true));
			index--;
			Value *target = builder->CreateGEP(aggr, llvm::ArrayRef<Value*>(&arrayIndexHelper[0], 2), "arr_part");
			DUMP(target->getType());
			if(typeInfo->subType == typeChar)
			{
				target = builder->CreatePointerCast(target, Type::getInt32PtrTy(getContext()), "ptr_any");
				DUMP(target->getType());
			}
			builder->CreateStore(V, target);
		}else if(V && typeInfo == typeAutoArray){
			Value *arrayStruct = CreateEntryBlockAlloca(F, "array_struct", (Type*)curr->typeInfo->llvmType);
			builder->CreateStore(V, arrayStruct);

			assert(curr->next);
			curr->next->CompileLLVM();
			Value *arrayType = V;
			Value *arrayPtr = builder->CreateStructGEP(arrayStruct, 0, "array_ptr");
			arrayPtr = builder->CreateLoad(arrayPtr);
			arrayPtr = builder->CreatePointerCast(arrayPtr, Type::getInt8PtrTy(getContext()), "ptr_any");
			Value *arrayLength = builder->CreateStructGEP(arrayStruct, 1, "array_length");
			arrayLength = builder->CreateLoad(arrayLength);

			Value *autoType = builder->CreateStructGEP(aggr, 0, "auto_arr_type");
			Value *autoPtr = builder->CreateStructGEP(aggr, 1, "auto_arr_ptr");
			Value *autoLength = builder->CreateStructGEP(aggr, 2, "auto_arr_length");

			/*arrayType->getType()->dump(); printf("\n");
			autoType->getType()->dump(); printf("\n");
			arrayPtr->getType()->dump(); printf("\n");
			autoPtr->getType()->dump(); printf("\n");
			arrayLength->getType()->dump(); printf("\n");
			autoLength->getType()->dump(); printf("\n");*/

			builder->CreateStore(arrayType, autoType);
			builder->CreateStore(arrayPtr, autoPtr);
			builder->CreateStore(arrayLength, autoLength);
			break;
		}
		curr = curr->next;
	}while(curr);
	if(typeInfo->arrLevel || typeInfo == typeAutoArray)// && typeInfo->arrSize == TypeInfo::UNSIZED_ARRAY)
		V = builder->CreateLoad(aggr, "arr_aggr");
	if(typeInfo != typeVoid && !V)
		ThrowError(CodeInfo::lastKnownStartPos, "ERROR: NodeExpressionList typeInfo && !V");
}

void NodeFuncDef::CompileLLVM()
{
	if(disabled)
	{
		CompileLLVMExtra();
		return;
	}

	//Function *CalleeF = module->getFunction(funcInfo->name);
	//if(CalleeF)
	//	ThrowError(CodeInfo::lastKnownStartPos, "ERROR: NodeFuncDef CalleeF");

	/*std::vector<const llvm::Type*> Arguments;
	VariableInfo *curr = NULL;
	for(curr = funcInfo->firstParam; curr; curr = curr->next)
		Arguments.push_back((Type*)curr->varType->llvmType);
	Arguments.push_back((Type*)funcInfo->extraParam->varType->llvmType);
	llvm::FunctionType *FT = llvm::FunctionType::get((Type*)funcInfo->retType->llvmType, Arguments, false);
	F = llvm::Function::Create(FT, llvm::Function::ExternalLinkage, funcInfo->name, module);*/
	F = (Function*)funcInfo->llvmFunction;
	funcInfo->llvmImplemented = true;

	llvm::BasicBlock *BB = llvm::BasicBlock::Create(getContext(), "entry", F);
	builder->SetInsertPoint(BB);

	Function::arg_iterator AI;
	VariableInfo *curr = NULL;
	for(AI = F->arg_begin(), curr = funcInfo->firstParam; AI != F->arg_end() && curr; AI++, curr = curr->next)
	{
		AI->setName(std::string(curr->name.begin, curr->name.end));
		AllocaInst *Alloca = CreateEntryBlockAlloca(F, std::string(curr->name.begin, curr->name.end), (Type*)curr->varType->llvmType);
		builder->CreateStore(AI, Alloca);
		curr->llvmValue = Alloca;
	}
	AI->setName(std::string(funcInfo->extraParam->name.begin, funcInfo->extraParam->name.end));
	AllocaInst *Alloca = CreateEntryBlockAlloca(F, std::string(funcInfo->extraParam->name.begin, funcInfo->extraParam->name.end), (Type*)funcInfo->extraParam->varType->llvmType);
	builder->CreateStore(AI, Alloca);
	funcInfo->extraParam->llvmValue = Alloca;

	for(curr = funcInfo->firstLocal; curr; curr = curr->next)
		curr->llvmValue = CreateEntryBlockAlloca(F, std::string(curr->name.begin, curr->name.end), (Type*)curr->varType->llvmType);

	first->CompileLLVM();
	if(first->nodeType == typeNodeZeroOp)
	{
		if(funcInfo->retType != typeVoid)
			ThrowError(CodeInfo::lastKnownStartPos, "ERROR: NodeFuncDef function must return a value");
		//builder->CreateRetVoid();
	}

	if(funcInfo->retType == typeVoid)
		builder->CreateRetVoid();
	else
		builder->CreateUnreachable();
	/*if(F->back().empty() || !F->back().back().isTerminator())
	{
		if(funcInfo->retType == typeVoid)
			builder->CreateRetVoid();
		else
			builder->CreateRet(ConstantInt::get(getContext(), APInt(32, 0, true)));
	}*/

	DUMP(F);
	verifyFunction(*F, llvm::PrintMessageAction);
	if(enableOptimization && !F->getEntryBlock().empty())
		OurFPM->run(*F);
	F = NULL;
}

void NodeFuncCall::CompileLLVM()
{
	CompileLLVMExtra();

	std::vector<llvm::Value*> args;
	//printf("Function call\n");
	if(funcType->paramCount)
	{
		NodeZeroOP	*curr = paramTail;
		TypeInfo	**paramType = funcType->paramType;// + funcType->paramCount - 1;
		do
		{
			curr->CompileLLVM();
			if(!V)
				ThrowError(CodeInfo::lastKnownStartPos, "ERROR: NodeFuncCall V = NULL");
			//printf("%s\t", curr->typeInfo->GetFullTypeName());
			DUMP(V->getType());
			V = ConvertFirstToSecond(V, curr->typeInfo, *paramType);
			if(*paramType == typeFloat)
				V = builder->CreateFPCast(V, (Type*)typeFloat->llvmType, "dtof");
			DUMP(V->getType());
			args.push_back(V);
			curr = curr->prev;
			paramType++;
		}while(curr);
	}
	Function *CalleeF = NULL;
	if(!funcInfo)
	{
		first->CompileLLVM();
		Value *func = V;
		DUMP(func->getType());
		Value *funcPtr = CreateEntryBlockAlloca(F, "func_tmp", (Type*)first->typeInfo->llvmType);
		DUMP(funcPtr->getType());
		builder->CreateStore(func, funcPtr);

		Value *ptr = builder->CreateStructGEP(funcPtr, 1, "ptr_part");
		DUMP(ptr->getType());
		Value *context = builder->CreateStructGEP(funcPtr, 0, "ctx_part");
		DUMP(context->getType());

		args.push_back(builder->CreateLoad(context, "ctx_deref"));

		CalleeF = (Function*)builder->CreateLoad(ptr, "func_deref");

		//ThrowError(CodeInfo::lastKnownStartPos, "ERROR: NodeFuncCall funcInfo = NULL");
	}else{
		CalleeF = (Function*)funcInfo->llvmFunction;// module->getFunction(funcInfo->name);
		if(!CalleeF)
			ThrowError(CodeInfo::lastKnownStartPos, "ERROR: !CalleeF");
		if(first)
		{
			first->CompileLLVM();
			DUMP(V->getType());
			Argument *last = (--CalleeF->arg_end());
			DUMP(last->getType());
			if(V->getType() != last->getType())
			{
				V = builder->CreatePointerCast(V, last->getType());
				DUMP(V->getType());
			}
			args.push_back(V);
		}else{
			args.push_back(ConstantPointerNull::get(Type::getInt32PtrTy(getContext())));
		}
	}
	if(funcType->retType != typeVoid)
	{
		DUMP(CalleeF->getType());
		V = builder->CreateCall(CalleeF, llvm::ArrayRef<Value*>(args));//, funcInfo->name);
	}else{
		builder->CreateCall(CalleeF, llvm::ArrayRef<Value*>(args));
		V = NULL;
	}
	if(funcInfo && funcInfo->nameHash == GetStringHash("__newA"))
	{
		DUMP(V->getType());
		DUMP(((Type*)typeInfo->llvmType));

		Value *arrTemp = CreateEntryBlockAlloca(F, "newa_tmp", V->getType());
		DUMP(arrTemp->getType());
		builder->CreateStore(V, arrTemp);

		DUMP(arrTemp->getType());
		DUMP(((Type*)typeInfo->llvmType));

		V = builder->CreatePointerCast(arrTemp, PointerType::getUnqual((Type*)typeInfo->llvmType), "newa_rescast");
		DUMP(V->getType());
		V = builder->CreateLoad(V, "newa_load");
		DUMP(V->getType());
	}
	if(funcInfo && funcInfo->nameHash == GetStringHash("__newS"))
	{
		DUMP(V->getType());
		DUMP(((Type*)typeInfo->llvmType));
		V = builder->CreatePointerCast(V, (Type*)typeInfo->llvmType, "news_rescast");
	}
	if(funcInfo && (funcInfo->nameHash == GetStringHash("__redirect") || funcInfo->nameHash == GetStringHash("__redirect_ptr")))
	{
		DUMP(V->getType());
		DUMP(((Type*)typeInfo->llvmType));

		Value *funcTemp = CreateEntryBlockAlloca(F, "redirect_tmp", V->getType());
		DUMP(funcTemp->getType());
		builder->CreateStore(V, funcTemp);

		DUMP(funcTemp->getType());
		DUMP(((Type*)typeInfo->llvmType));

		V = builder->CreatePointerCast(funcTemp, PointerType::getUnqual((Type*)typeInfo->llvmType), "redirect_cast");
		DUMP(V->getType());
		V = builder->CreateLoad(V, "redirect_res");
		DUMP(V->getType());
	}
}

void NodeUnaryOp::CompileLLVM()
{
	CompileLLVMExtra();

	asmOperType aOT = operTypeForStackType[first->typeInfo->stackType];

	CmdID id = vmCmd.cmd;
	if(id != cmdPushTypeID && id != cmdFuncAddr && id != cmdCheckedRet)
	{
		if(aOT == OTYPE_INT)
			id = vmCmd.cmd;
		else if(aOT == OTYPE_LONG)
			id = (CmdID)(vmCmd.cmd + 1);
		else
			id = (CmdID)(vmCmd.cmd + 2);
	}

	// Child node computes value to V
	first->CompileLLVM();
	Value *zeroV = ConstantInt::get(getContext(), APInt(id == cmdLogNotL ? 64 : 32, 0, true));
	if(first->typeInfo == typeObject)
	{
		DUMP(V->getType());
		Value *aggr = CreateEntryBlockAlloca(F, "ref_tmp", (Type*)typeObject->llvmType);
		builder->CreateStore(V, aggr);
		DUMP(aggr->getType());
		V = builder->CreateStructGEP(aggr, 1, "tmp_ptr");
		DUMP(V->getType());
		V = builder->CreateLoad(V, "tmp_ptr");
		DUMP(V->getType());
		V = builder->CreatePtrToInt(V, (Type*)typeInt->llvmType, "as_int");
		DUMP(V->getType());
		id = cmdLogNot;
		//ThrowError(CodeInfo::lastKnownStartPos, "ERROR: !CalleeF");
	}else if(first->typeInfo->refLevel){
		DUMP(V->getType());
		zeroV = ConstantPointerNull::get((PointerType*)V->getType());
	}

	V = PromoteToStackType(V, first->typeInfo);

	switch(id)
	{
	case cmdNeg:
	case cmdNegL:
		V = builder->CreateNeg(V, "neg_tmp");
		break;

	case cmdNegD:
		V = builder->CreateFNeg(V, "neg_tmp");
		break;

	case cmdBitNot:
	case cmdBitNotL:
		V = builder->CreateNot(V, "not_tmp");
		break;
	case cmdLogNot:	// !	logical NOT
	case cmdLogNotL:
		// Compare that not equal to 0
		V = builder->CreateICmpNE(V, zeroV, "to_bool");
		// Not 
		V = builder->CreateNot(V, "lognot_tmp");
		V = builder->CreateIntCast(V, Type::getInt32Ty(getContext()), true, "booltmp");
		break;
	case cmdPushTypeID:
		V = ConstantInt::get((Type*)typeInt->llvmType, APInt(32, vmCmd.argument));
		break;
	case cmdFuncAddr:
		V = (llvm::Function*)CodeInfo::funcInfo[vmCmd.argument]->llvmFunction;

		V = builder->CreatePointerCast(V, (llvm::Type*)typeFunction->llvmType, "function_to_vtbl_element");

		//V = ConstantInt::get((Type*)typeInt->llvmType, APInt(32, vmCmd.argument));
		break;
	case cmdCheckedRet:
		//printf("LLVM: unsupported command cmdCheckedRet\n");
		break;
	default:
		ThrowError(CodeInfo::lastKnownStartPos, "ERROR: unknown unary command %s", vmInstructionText[vmCmd.cmd]);
	}
}

void NodeGetAddress::CompileLLVM()
{
	CompileLLVMExtra();

	if(!varInfo)
		ThrowError(CodeInfo::lastKnownStartPos, "ERROR: NodeGetAddress varInfo = NULL");
	//builder->CreateIntToPtr
	V = (Value*)varInfo->llvmValue;//NamedValues[varInfo->nameHash];//builder->CreateGEP(NamedValues[varInfo->nameHash], ConstantInt::get(Type::getInt32Ty(getContext()), APInt(32, 0)), "tmp_ptr");
}

void NodeDereference::CompileLLVM()
{
	CompileLLVMExtra();

	if(neutralized)
	{
		originalNode->CompileLLVM();
		return;
	}
	if(closureFunc)
	{
		assert(first->nodeType == typeNodeVariableSet);

		VariableInfo *closure = ((NodeOneOP*)first)->GetFirstNode()->nodeType == typeNodeGetAddress ? ((NodeGetAddress*)((NodeOneOP*)first)->GetFirstNode())->varInfo : NULL;
		if(closure)
			assert(closure->varType == first->typeInfo);

		/*__lambda_80_cls * __lambda_80_ext_12;
		*(&n_4) = 5;
		*(&res1_8) = test_79(3, (	(__lambda_80_ext_12 = (__lambda_80_cls * )__newS(12, __nullcTR[122])),
		(*(int**)((char*)__lambda_80_ext_12 + 0) = (int*)&n_4),
		(*(int**)((char*)__lambda_80_ext_12 + 4) = (int*)__upvalue_78_n_4),
		(*(int*)((char*)__lambda_80_ext_12 + 8) = 4),
		(__upvalue_78_n_4 = (__nullcUpvalue*)((int*)__lambda_80_ext_12 + 0)),*/

		llvm::Value	*targetClosure = 0;

		if(closure)
		{
			targetClosure = closure->llvmValue;
		}else{
			((NodeOneOP*)first)->GetFirstNode()->CompileLLVM();
			targetClosure = V;
		}

		//closure->llvmValue->dump();
		//closure->llvmValue->getType()->dump(); printf("\n");

		// Allocate closure
		llvm::Value *value = builder->CreateCall3(module->getFunction("__newS"), ConstantInt::get(getContext(), APInt(32, first->typeInfo->subType->size, true)), ConstantInt::get(getContext(), APInt(32, first->typeInfo->subType->typeIndex, true)), llvm::ConstantPointerNull::get(llvm::Type::getInt32PtrTy(getContext())));
		
		llvm::Value *closureStart = value;

		value = builder->CreatePointerCast(value, llvm::PointerType::getUnqual(typeInfo->llvmType), "news_rescast");

		//value->dump();
		//value->getType()->dump(); printf("\n");

		builder->CreateStore(value, targetClosure);

		unsigned int pos = 0;
		for(FunctionInfo::ExternalInfo *curr = closureFunc->firstExternal; curr; curr = curr->next)
		{
			//closureStart->dump();
			//closureStart->getType()->dump(); printf("\n");

			llvm::Value *gep[2];
			gep[0] = ConstantInt::get(getContext(), APInt(32, pos, true));
			llvm::Value *closureCurr = builder->CreateGEP(closureStart, llvm::ArrayRef<llvm::Value*>(gep[0]));

			//closureCurr->dump();
			//closureCurr->getType()->dump(); printf("\n");

			closureCurr = builder->CreatePointerCast(closureCurr, llvm::PointerType::getUnqual(typeClosure));

			//closureCurr->dump();
			//closureCurr->getType()->dump(); printf("\n");

			llvm::Value* closurePtr = builder->CreateStructGEP(closureCurr, 0, "cls_ptr");

			//closurePtr->dump();
			//closurePtr->getType()->dump(); printf("\n");

			//curr->variable->llvmValue->dump();
			//curr->variable->llvmValue->getType()->dump(); printf("\n");

			if(curr->targetPos == ~0u)
				assert(0);

			if(curr->targetLocal)
			{
				builder->CreateStore(builder->CreatePointerCast(curr->variable->llvmValue, llvm::Type::getInt8PtrTy(getContext())), closurePtr);
			}else{
				assert(closureFunc->parentFunc);
				assert(closureFunc->parentFunc->extraParam);

				// Get function closure
				Value *closure = builder->CreateLoad(closureFunc->parentFunc->extraParam->llvmValue, "closure");
				// Cast to char*
				closure = builder->CreatePointerCast(closure, Type::getInt8PtrTy(getContext()));
				// Offset to target upvalue
				closure = builder->CreateGEP(closure, llvm::ArrayRef<Value*>(ConstantInt::get(getContext(), APInt(32, curr->targetPos, true))));
				// Convert char* to type**
				closure = builder->CreatePointerCast(closure, llvm::PointerType::getUnqual(llvm::PointerType::getUnqual(curr->variable->varType->llvmType)));
				// Get pointer to value
				closure = builder->CreateLoad(closure, "closure_deref");
				// Cast to char*
				closure = builder->CreatePointerCast(closure, Type::getInt8PtrTy(getContext()));
				// Store in upvalue
				builder->CreateStore(closure, closurePtr);
			}

			llvm::Value* closureUpvalue = builder->CreateStructGEP(closureCurr, 1, "cls_upvalue");

			//closureUpvalue->dump();
			//closureUpvalue->getType()->dump(); printf("\n");

			//curr->variable->llvmUpvalue->dump();
			//curr->variable->llvmUpvalue->getType()->dump(); printf("\n");

			builder->CreateStore(builder->CreateLoad(curr->variable->llvmUpvalue), closureUpvalue);

			llvm::Value* closureSize = builder->CreateStructGEP(closureCurr, 2, "cls_size");

			//closureSize->dump();
			//closureSize->getType()->dump(); printf("\n");

			builder->CreateStore(ConstantInt::get(getContext(), APInt(32, curr->variable->varType->size, true)), closureSize);

			builder->CreateStore(closureCurr, curr->variable->llvmUpvalue);

			pos += curr->variable->varType->size > (NULLC_PTR_SIZE + NULLC_PTR_SIZE + 4) ? curr->variable->varType->size : (NULLC_PTR_SIZE + NULLC_PTR_SIZE + 4);
		}
	}else{
		first->CompileLLVM();
		if(typeInfo != first->typeInfo->subType)
		{
			DUMP(V->getType());
			V = builder->CreatePointerCast(V, PointerType::getUnqual((Type*)typeInfo->llvmType), "deref_cast");
			DUMP(V->getType());
		}
		if(!V)
			ThrowError(CodeInfo::lastKnownStartPos, "ERROR: NodeDereference V = NULL");
		V = builder->CreateLoad(V, "temp_deref");
	}
}

void MakeBinaryOp(Value *left, Value *right, CmdID cmdID, asmStackType fST)
{
	switch(cmdID)
	{
	case cmdAdd:
		if(fST == STYPE_INT || fST == STYPE_LONG)
			V = builder->CreateAdd(left, right, "add_tmp");
		else
			V = builder->CreateFAdd(left, right, "fadd_tmp");
		break;		// a + b
	case cmdSub:
		if(fST == STYPE_INT || fST == STYPE_LONG)
			V = builder->CreateSub(left, right, "sub_tmp");
		else
			V = builder->CreateFSub(left, right, "fsub_tmp");
		break;		// a - b
	case cmdMul:
		if(fST == STYPE_INT || fST == STYPE_LONG)
			V = builder->CreateMul(left, right, "mul_tmp");
		else
			V = builder->CreateFMul(left, right, "fmul_tmp");
		break;		// a * b
	case cmdDiv:
		if(fST == STYPE_DOUBLE)
			V = builder->CreateFDiv(left, right, "fdiv_tmp");
		else
			V = builder->CreateSDiv(left, right, "div_tmp");
		break;		// a / b
	case cmdPow:
		if(fST == STYPE_INT)
			V = builder->CreateCall2(module->getFunction("llvmIntPow"), left, right, "ipow_tmp");
		else if(fST == STYPE_LONG)
			V = builder->CreateCall2(module->getFunction("llvmLongPow"), left, right, "lpow_tmp");
		else
			V = builder->CreateCall2(module->getFunction("llvmDoublePow"), left, right, "fpow_tmp");
		break;		// power(a, b) (**)
	case cmdMod:
		if(fST == STYPE_DOUBLE)
			V = builder->CreateFRem(left, right, "frem_tmp");
		else
			V = builder->CreateSRem(left, right, "rem_tmp");
		break;		// a % b
	case cmdLess:
		if(fST == STYPE_DOUBLE)
			V = builder->CreateFCmpOLT(left, right);
		else
			V = builder->CreateICmpSLT(left, right);
		V = builder->CreateZExt(V, Type::getInt32Ty(getContext()), "to_int");
		break;	// a < b
	case cmdGreater:
		if(fST == STYPE_DOUBLE)
			V = builder->CreateFCmpOGT(left, right);
		else
			V = builder->CreateICmpSGT(left, right);
		V = builder->CreateZExt(V, Type::getInt32Ty(getContext()), "to_int");
		break;	// a > b
	case cmdLEqual:
		if(fST == STYPE_DOUBLE)
			V = builder->CreateFCmpOLE(left, right);
		else
			V = builder->CreateICmpSLE(left, right);
		V = builder->CreateZExt(V, Type::getInt32Ty(getContext()), "to_int");
		break;	// a <= b
	case cmdGEqual:
		if(fST == STYPE_DOUBLE)
			V = builder->CreateFCmpOGE(left, right);
		else
			V = builder->CreateICmpSGE(left, right);
		V = builder->CreateZExt(V, Type::getInt32Ty(getContext()), "to_int");
		break;	// a >= b
	case cmdEqual:
		if(fST == STYPE_DOUBLE)
			V = builder->CreateFCmpOEQ(left, right);
		else
			V = builder->CreateICmpEQ(left, right);
		V = builder->CreateZExt(V, Type::getInt32Ty(getContext()), "to_int");
		break;	// a == b
	case cmdNEqual:
		if(fST == STYPE_DOUBLE)
			V = builder->CreateFCmpONE(left, right);
		else
			V = builder->CreateICmpNE(left, right);
		V = builder->CreateZExt(V, Type::getInt32Ty(getContext()), "to_int");
		break;	// a != b
	case cmdShl:
		V = builder->CreateShl(left, right);
		break;		// a << b
	case cmdShr:
		V = builder->CreateAShr(left, right);
		break;		// a >> b
	case cmdBitAnd:
		V = builder->CreateBinOp(llvm::Instruction::And, left, right);
		break;
	case cmdBitOr:
		V = builder->CreateBinOp(llvm::Instruction::Or, left, right);
		break;
	case cmdBitXor:
		V = builder->CreateBinOp(llvm::Instruction::Xor, left, right);
		break;
	case cmdLogAnd:
		ThrowError(CodeInfo::lastKnownStartPos, "ERROR: MakeBinaryOp cmdLogAnd");
		break;
	case cmdLogOr:
		ThrowError(CodeInfo::lastKnownStartPos, "ERROR: MakeBinaryOp cmdLogOr");
		break;
	case cmdLogXor:
		left = builder->CreateICmpNE(left, ConstantInt::get(getContext(), APInt(fST == STYPE_LONG ? 64 : 32, 0, true)), "to_bool_left");
		right = builder->CreateICmpNE(right, ConstantInt::get(getContext(), APInt(fST == STYPE_LONG ? 64 : 32, 0, true)), "to_bool_rught");
		V = builder->CreateBinOp(llvm::Instruction::Xor, left, right, "less_tmp");
		V = builder->CreateIntCast(V, Type::getInt32Ty(getContext()), true, "to_int");
		break;
	default:
		assert(!"unimplemented");
	}
}

void NodeBinaryOp::CompileLLVM()
{
	CompileLLVMExtra();

	asmStackType fST = ChooseBinaryOpResultType(first->typeInfo, second->typeInfo)->stackType;
	if(cmdID == cmdLogOr || cmdID == cmdLogAnd)
	{
		first->CompileLLVM();
		Value *left = V;
		left = PromoteToStackType(left, first->typeInfo);
		if(fST == STYPE_LONG)
			left = builder->CreateIntCast(left, (Type*)typeInt->llvmType, true, "sc_l");
		left = builder->CreateICmpNE(left, ConstantInt::get(getContext(), APInt(32, 0)), "left_cond");

		BasicBlock *fastExit = BasicBlock::Create(getContext(), "fastExit", F);
		BasicBlock *slowExit = BasicBlock::Create(getContext(), "slowExit", F);
		BasicBlock *slowOneExit = BasicBlock::Create(getContext(), "slowOneExit", F);
		BasicBlock *slowZeroExit = BasicBlock::Create(getContext(), "slowZeroExit", F);
		BasicBlock *slowMergeExit = BasicBlock::Create(getContext(), "slowMergeExit", F);
		BasicBlock *mergeExit = BasicBlock::Create(getContext(), "mergeExit", F);

		builder->CreateCondBr(left, cmdID == cmdLogOr ? fastExit : slowExit, cmdID == cmdLogOr ? slowExit : fastExit);

		builder->SetInsertPoint(slowExit);
		second->CompileLLVM();
		Value *right = V;
		right = PromoteToStackType(right, second->typeInfo);
		if(fST == STYPE_LONG)
			right = builder->CreateIntCast(right, (Type*)typeInt->llvmType, true, "sc_l");
		right = builder->CreateICmpNE(right, ConstantInt::get(getContext(), APInt(32, 0)), "left_cond");
		builder->CreateCondBr(right, slowOneExit, slowZeroExit);

		builder->SetInsertPoint(slowOneExit);
		Value *rightOne = ConstantInt::get(getContext(), APInt(32, 1));
		builder->CreateBr(slowMergeExit);
		slowOneExit = builder->GetInsertBlock();

		builder->SetInsertPoint(slowZeroExit);
		Value *rightZero = ConstantInt::get(getContext(), APInt(32, 0));
		builder->CreateBr(slowMergeExit);
		slowZeroExit = builder->GetInsertBlock();

		builder->SetInsertPoint(slowMergeExit);
		PHINode *PN = builder->CreatePHI((Type*)typeInt->llvmType, 2, "slow_merge_tmp");
		PN->addIncoming(rightOne, slowOneExit);
		PN->addIncoming(rightZero, slowZeroExit);
		builder->CreateBr(mergeExit);
		slowMergeExit = builder->GetInsertBlock();

		Value *leftUnknown = PN;

		builder->SetInsertPoint(fastExit);
		Value *leftZero = ConstantInt::get(getContext(), APInt(32, cmdID == cmdLogOr ? 1 : 0));
		builder->CreateBr(mergeExit);
		fastExit = builder->GetInsertBlock();

		builder->SetInsertPoint(mergeExit);
		PHINode *PN2 = builder->CreatePHI((Type*)typeInt->llvmType, 2, "merge_tmp");
		PN2->addIncoming(leftUnknown, slowMergeExit);
		PN2->addIncoming(leftZero, fastExit);

		V = PN2;
		return;
	}

	first->CompileLLVM();
	Value *left = V;
	second->CompileLLVM();
	Value *right = V;
	//printf("before promote: ");
	DUMP(left->getType());
	DUMP(right->getType());
	left = PromoteToStackType(left, first->typeInfo);
	right = PromoteToStackType(right, second->typeInfo);
	//printf("after promote: ");
	DUMP(left->getType());
	DUMP(right->getType());
	left = ConvertFirstForSecond(left, first->typeInfo, second->typeInfo);
	right = ConvertFirstForSecond(right, second->typeInfo, first->typeInfo);
	//printf("after convertion: ");
	DUMP(left->getType());
	DUMP(right->getType());
	MakeBinaryOp(left, right, cmdID, fST);
}

void NodeVariableSet::CompileLLVM()
{
	CompileLLVMExtra();

	first->CompileLLVM();
	Value *target = V;
	if(!target)
		ThrowError(CodeInfo::lastKnownStartPos, "ERROR: NodeVariableSet target = NULL");
	DUMP(target->getType());
	second->CompileLLVM();
	Value *value = V;
	if(!value)
		ThrowError(CodeInfo::lastKnownStartPos, "ERROR: NodeVariableSet value = NULL");
	DUMP(value->getType());

	if(arrSetAll)
	{
		char fName[20];
		Value *arrayIndexHelper[2];
		arrayIndexHelper[0] = ConstantInt::get(getContext(), APInt(64, 0, true));
		arrayIndexHelper[1] = ConstantInt::get(getContext(), APInt(32, 0));
		target = builder->CreateGEP(target, llvm::ArrayRef<Value*>(&arrayIndexHelper[0], 2), "arr_begin");
		value = PromoteToStackType(value, second->typeInfo);
		value = ConvertFirstToSecond(value, GetStackType(second->typeInfo), first->typeInfo->subType->subType);
		char suffix = '\0';
		switch(first->typeInfo->subType->subType->type)
		{
		case TypeInfo::TYPE_CHAR: suffix = 'C'; break;
		case TypeInfo::TYPE_SHORT: suffix = 'S'; break;
		case TypeInfo::TYPE_INT: suffix = 'I'; break;
		case TypeInfo::TYPE_LONG: suffix = 'L'; break;
		case TypeInfo::TYPE_FLOAT: suffix = 'F'; break;
		case TypeInfo::TYPE_DOUBLE: suffix = 'D'; break;
		default: ThrowError(CodeInfo::lastKnownStartPos, "ERROR: NodeVariableSet arrSetAll unknown type");
		}
		SafeSprintf(fName, 20, "__llvmSetArray%c", suffix);
		Function *setFunc = module->getFunction(fName);
		assert(setFunc);
		DUMP(target->getType());
		DUMP(value->getType());
		builder->CreateCall3(setFunc, target, value, ConstantInt::get((Type*)typeInt->llvmType, APInt(32u, elemCount, true)));
		V = NULL;
		return;
	}

	value = PromoteToStackType(value, second->typeInfo);
	value = ConvertFirstToSecond(value, GetStackType(second->typeInfo), first->typeInfo->subType);
	if(!value)
		ThrowError(CodeInfo::lastKnownStartPos, "ERROR: NodeVariableSet value = NULL after");
	DUMP(value->getType());

	if(typeInfo == typeBool)
	{
		value = builder->CreateIntCast(value, (Type*)typeInfo->llvmType, true, "tmp_to_bool");
		DUMP(value->getType());
	}

	builder->CreateStore(value, target);
	V = value;
}

void NodePopOp::CompileLLVM()
{
	CompileLLVMExtra();

	first->CompileLLVM();
	V = NULL;
}

Value* GenerateCompareToZero(Value *V, TypeInfo *typeInfo)
{
	Value *zeroV = ConstantInt::get(getContext(), APInt(32, 0, true));
	if(typeInfo == typeObject)
	{
		DUMP(V->getType());
		Value *arrTemp = CreateEntryBlockAlloca(F, "autoref_tmp", V->getType());
		DUMP(arrTemp->getType());
		builder->CreateStore(V, arrTemp);

		V = builder->CreateStructGEP(arrTemp, 1, "autoref_ptr");
		DUMP(V->getType());
		V = builder->CreateLoad(V);
		DUMP(V->getType());
		V = builder->CreatePtrToInt(V, Type::getInt32Ty(getContext()), "as_int");
		DUMP(V->getType());
	}else if(typeInfo->refLevel){
		DUMP(V->getType());
		zeroV = ConstantPointerNull::get((PointerType*)V->getType());
	}else if(typeInfo != typeInt){
		DUMP(V->getType());
		V = PromoteToStackType(V, typeInfo);
		DUMP(V->getType());
		V = builder->CreateIntCast(V, Type::getInt32Ty(getContext()), true, "as_int");
		DUMP(V->getType());
	}
	return builder->CreateICmpNE(V, zeroV, "cond_value");
}

void NodeIfElseExpr::CompileLLVM()
{
	CompileLLVMExtra();

	BasicBlock *trueBlock = BasicBlock::Create(getContext(), "trueBlock", F);
	BasicBlock *falseBlock = third ? BasicBlock::Create(getContext(), "falseBlock") : NULL;
	BasicBlock *exitBlock = BasicBlock::Create(getContext(), "exitBlock");
	
	Value *left = NULL, *right = NULL;

	// Compute condition
	first->CompileLLVM();
	Value *cond = GenerateCompareToZero(V, first->typeInfo);

	builder->CreateCondBr(cond, trueBlock, third ? falseBlock : exitBlock);

	builder->SetInsertPoint(trueBlock);

	second->CompileLLVM();
	left = V;
	if(typeInfo != typeVoid)
		left = ConvertFirstToSecond(left, second->typeInfo, typeInfo);
	
	trueBlock = builder->GetInsertBlock();
	if(trueBlock->empty() || !trueBlock->back().isTerminator())//if(F->back().empty() || !F->back().back().isTerminator())
		builder->CreateBr(exitBlock);
	
	if(third)
	{
		F->getBasicBlockList().push_back(falseBlock);
		builder->SetInsertPoint(falseBlock);
		third->CompileLLVM();
		right = V;
		if(typeInfo != typeVoid)
			right = ConvertFirstToSecond(right, third->typeInfo, typeInfo);

		falseBlock = builder->GetInsertBlock();
		if(falseBlock->empty() || !falseBlock->back().isTerminator())//F->back().empty() || !F->back().back().isTerminator())
			builder->CreateBr(exitBlock);
		
	}
	F->getBasicBlockList().push_back(exitBlock);
	builder->SetInsertPoint(exitBlock);

	V = NULL;
	if(typeInfo != typeVoid)
	{
		PHINode *PN2 = builder->CreatePHI((Type*)typeInfo->llvmType, 2, "merge_tmp");
		DUMP(left->getType());
		DUMP(right->getType());
		PN2->addIncoming(left, trueBlock);
		PN2->addIncoming(right, falseBlock);

		V = PN2;
	}
}

void NodeForExpr::CompileLLVM()
{
	CompileLLVMExtra();

	first->CompileLLVM();

	BasicBlock *condBlock = BasicBlock::Create(getContext(), "condBlock", F);
	BasicBlock *bodyBlock = BasicBlock::Create(getContext(), "bodyBlock");
	BasicBlock *incrementBlock = BasicBlock::Create(getContext(), "incrementBlock");
	BasicBlock *exitBlock = BasicBlock::Create(getContext(), "exitBlock");

	builder->CreateBr(condBlock);
	builder->SetInsertPoint(condBlock);

	second->CompileLLVM();
	Value *cond = GenerateCompareToZero(V, second->typeInfo);
	builder->CreateCondBr(cond, bodyBlock, exitBlock);

	breakStack.push_back(exitBlock);
	continueStack.push_back(incrementBlock);

	F->getBasicBlockList().push_back(bodyBlock);
	builder->SetInsertPoint(bodyBlock);
	fourth->CompileLLVM();
	builder->CreateBr(incrementBlock);

	continueStack.pop_back();
	breakStack.pop_back();

	F->getBasicBlockList().push_back(incrementBlock);
	builder->SetInsertPoint(incrementBlock);
	third->CompileLLVM();
	builder->CreateBr(condBlock);

	F->getBasicBlockList().push_back(exitBlock);
	builder->SetInsertPoint(exitBlock);
	V = NULL;
}

void NodeWhileExpr::CompileLLVM()
{
	CompileLLVMExtra();

	BasicBlock *condBlock = BasicBlock::Create(getContext(), "condBlock", F);
	BasicBlock *bodyBlock = BasicBlock::Create(getContext(), "bodyBlock", F);
	BasicBlock *exitBlock = BasicBlock::Create(getContext(), "exitBlock", F);

	builder->CreateBr(condBlock);
	builder->SetInsertPoint(condBlock);

	first->CompileLLVM();
	Value *cond = GenerateCompareToZero(V, first->typeInfo);
	builder->CreateCondBr(cond, bodyBlock, exitBlock);

	breakStack.push_back(exitBlock);
	continueStack.push_back(condBlock);

	builder->SetInsertPoint(bodyBlock);
	second->CompileLLVM();
	builder->CreateBr(condBlock);

	continueStack.pop_back();
	breakStack.pop_back();

	builder->SetInsertPoint(exitBlock);
	V = NULL;
}

void NodeDoWhileExpr::CompileLLVM()
{
	CompileLLVMExtra();

	BasicBlock *bodyBlock = BasicBlock::Create(getContext(), "bodyBlock", F);
	BasicBlock *condBlock = BasicBlock::Create(getContext(), "condBlock", F);
	BasicBlock *exitBlock = BasicBlock::Create(getContext(), "exitBlock", F);

	builder->CreateBr(bodyBlock);
	builder->SetInsertPoint(bodyBlock);

	breakStack.push_back(exitBlock);
	continueStack.push_back(condBlock);

	first->CompileLLVM();

	continueStack.pop_back();
	breakStack.pop_back();

	builder->CreateBr(condBlock);
	builder->SetInsertPoint(condBlock);

	second->CompileLLVM();
	Value *cond = GenerateCompareToZero(V, second->typeInfo);
	builder->CreateCondBr(cond, bodyBlock, exitBlock);

	builder->SetInsertPoint(exitBlock);
	V = NULL;
}

void NodePreOrPostOp::CompileLLVM()
{
	CompileLLVMExtra();

	first->CompileLLVM();
	Value *address = V;

	DUMP(address->getType());

	if(prefixOp)
	{
		// ++x
		V = builder->CreateLoad(address, "incdecpre_tmp");
		DUMP(V->getType());
		//DUMP(V->getType());
		Value *mod = 0;
		if(typeInfo->stackType == STYPE_DOUBLE)
			mod = builder->CreateFAdd(V, ConvertFirstToSecond(ConstantInt::get(getContext(), APInt(32, incOp ? 1 : -1, true)), typeInt, typeInfo), "pre_res");
		else
			mod = builder->CreateAdd(V, ConvertFirstToSecond(ConstantInt::get(getContext(), APInt(32, incOp ? 1 : -1, true)), typeInt, typeInfo), "pre_res");
		builder->CreateStore(mod, address);
		V = mod;
	}else{
		// x++
		V = builder->CreateLoad(address, "incdecpre_tmp");
		DUMP(V->getType());
		Value *mod = 0;
		if(typeInfo->stackType == STYPE_DOUBLE)
			mod = builder->CreateFAdd(V, ConvertFirstToSecond(ConstantInt::get(getContext(), APInt(32, incOp ? 1 : -1, true)), typeInt, typeInfo), "post_res");
		else
			mod = builder->CreateAdd(V, ConvertFirstToSecond(ConstantInt::get(getContext(), APInt(32, incOp ? 1 : -1, true)), typeInt, typeInfo), "post_res");
		builder->CreateStore(mod, address);
	}
}

void NodeVariableModify::CompileLLVM()
{
	CompileLLVMExtra();

	first->CompileLLVM();
	Value *address = V;

	second->CompileLLVM();
	Value *right = V;

	Value *left = builder->CreateLoad(address, "modify_left");
	
	TypeInfo *resType = ChooseBinaryOpResultType(first->typeInfo->subType, second->typeInfo);
	
	DUMP(left->getType());
	DUMP(right->getType());
	left = PromoteToStackType(left, first->typeInfo->subType);
	right = PromoteToStackType(right, second->typeInfo);
	DUMP(left->getType());
	DUMP(right->getType());
	left = ConvertFirstForSecond(left, first->typeInfo->subType, second->typeInfo);
	right = ConvertFirstForSecond(right, second->typeInfo, first->typeInfo->subType);
	DUMP(left->getType());
	DUMP(right->getType());
	MakeBinaryOp(left, right, cmdID, resType->stackType);

	/*if(resType == typeFloat)
		resType = typeDouble;
	if(resType == typeChar || resType == typeShort)
		resType = typeInt;*/
	V = ConvertFirstToSecond(V, GetStackType(resType), typeInfo);
	DUMP(V->getType());
	builder->CreateStore(V, address);
}

void NodeArrayIndex::CompileLLVM()
{
	CompileLLVMExtra();

	first->CompileLLVM();
	Value *address = V;
	DUMP(address->getType());
	second->CompileLLVM();
	Value *index = V;
	DUMP(index->getType());
	Value *arrayIndexHelper[2];
	if(typeParent->arrSize == TypeInfo::UNSIZED_ARRAY)
	{
		Value *aggr = CreateEntryBlockAlloca(F, "arr_tmp", (Type*)typeParent->llvmType);
		builder->CreateStore(address, aggr);
		DUMP(aggr->getType());
		/*V = builder->CreateStructGEP(aggr, 1, "tmp_arr");
		Value *arrSize = builder->CreateLoad(V, "arr_size");
		// Check array size
		BasicBlock *failBlock = BasicBlock::Create(getContext(), "failBlock", F);
		BasicBlock *passBlock = BasicBlock::Create(getContext(), "passBlock", F);
		Value *check = builder->CreateICmpULT(index, arrSize, "check_res");
		builder->CreateCondBr(check, passBlock, failBlock);
		builder->SetInsertPoint(failBlock);
		builder->CreateBr(globalExit);
		builder->SetInsertPoint(passBlock);*/
		// Access array
		V = builder->CreateStructGEP(aggr, 0, "tmp_arr");
		Value *arrPtr = builder->CreateLoad(V, "tmp_arrptr");
		arrayIndexHelper[0] = index;
		V = builder->CreateGEP(arrPtr, llvm::ArrayRef<Value*>(&arrayIndexHelper[0], 1), "tmp_elem");
	}else{
		arrayIndexHelper[0] = ConstantInt::get(getContext(), APInt(64, 0, true));
		index = ConvertFirstToSecond(index, GetStackType(second->typeInfo), typeInt);
		arrayIndexHelper[1] = index;
		V = builder->CreateGEP(address, llvm::ArrayRef<Value*>(&arrayIndexHelper[0], 2), "tmp_arr");
		DUMP(V->getType());
	}
}

void NodeShiftAddress::CompileLLVM()
{
	CompileLLVMExtra();

	TypeInfo *parentType = first->typeInfo->subType;
	int index = 0;
	for(TypeInfo::MemberVariable *curr = parentType->firstVariable; curr && curr != member; curr = curr->next, index++){}
	if(parentType->arrLevel && parentType->arrSize == TypeInfo::UNSIZED_ARRAY)
	{
		assert(member->nameHash == GetStringHash("size"));
		index = 1;
	}
	first->CompileLLVM();
	Value *address = V;
	DUMP(address->getType());
	Value *arrayIndexHelper[2];
	arrayIndexHelper[0] = ConstantInt::get(getContext(), APInt(64, 0, true));
	arrayIndexHelper[1] = ConstantInt::get(getContext(), APInt(32, index, true));
	V = builder->CreateGEP(address, llvm::ArrayRef<Value*>(&arrayIndexHelper[0], 2), "tmp_member");
}

void NodeBreakOp::CompileLLVM()
{
	CompileLLVMExtra();

	builder->CreateBr(breakStack[breakStack.size() - breakDepth]);
	BasicBlock *afterBreak = BasicBlock::Create(getContext(), "afterBreak", F);
	builder->SetInsertPoint(afterBreak);
	V = NULL;
}

void NodeContinueOp::CompileLLVM()
{
	CompileLLVMExtra();

	builder->CreateBr(continueStack[breakStack.size() - continueDepth]);
	BasicBlock *afterContinue = BasicBlock::Create(getContext(), "afterContinue", F);
	builder->SetInsertPoint(afterContinue);
	V = NULL;
}

void NodeSwitchExpr::CompileLLVM()
{
	CompileLLVMExtra();

	ThrowError(CodeInfo::lastKnownStartPos, "ERROR: NodeSwitchExpr");
}

void NodeConvertPtr::CompileLLVM()
{
	CompileLLVMExtra();

	if(typeInfo == typeTypeid)
	{
		V = ConstantInt::get((Type*)typeInt->llvmType, APInt(32, first->typeInfo->subType->typeIndex));
	}else if(typeInfo == typeObject){
		first->CompileLLVM();
		DUMP(V->getType());
		Value *ptr = builder->CreatePointerCast(V, Type::getInt8PtrTy(getContext()), "ptr_cast");
		DUMP(ptr->getType());
		// Allocate space for auto ref
		Value *aggr = CreateEntryBlockAlloca(F, "tmp_autoref", (Type*)typeObject->llvmType);
		DUMP(aggr->getType());
		V = builder->CreateStructGEP(aggr, 0, "tmp_arr");
		DUMP(V->getType());
		builder->CreateStore(ConstantInt::get((Type*)typeInt->llvmType, APInt(32, first->typeInfo->subType->typeIndex)), V);
		V = builder->CreateStructGEP(aggr, 1, "tmp_arr");
		DUMP(V->getType());
		builder->CreateStore(ptr, V);
		V = builder->CreateLoad(aggr, "autoref");
	}else{
		first->CompileLLVM();
		DUMP(V->getType());
		// Allocate space for auto ref
		Value *aggr = CreateEntryBlockAlloca(F, "tmp_autoref", (Type*)typeObject->llvmType);
		DUMP(aggr->getType());
		builder->CreateStore(V, aggr);
		V = builder->CreateStructGEP(aggr, 1, "tmp_ptr");
		DUMP(V->getType());
		V = builder->CreateLoad(V, "ptr");
		DUMP(V->getType());
		DUMP(((Type*)typeInfo->llvmType));
		V = builder->CreatePointerCast(V, (Type*)typeInfo->llvmType, "ptr_cast");
		DUMP(V->getType());
	}
}

void NodeOneOP::CompileLLVM()
{
	CompileLLVMExtra();

	V = NULL;
	assert(first);
	first->CompileLLVM();
}

void NodeFunctionAddress::CompileLLVM()
{
	CompileLLVMExtra();

	Value *aggr = CreateEntryBlockAlloca(F, "func_tmp", (Type*)typeInfo->llvmType);
	DUMP(aggr->getType());
	Value *ptr = builder->CreateStructGEP(aggr, 1, "ptr_part");
	DUMP(ptr->getType());
	Value *context = builder->CreateStructGEP(aggr, 0, "ctx_part");
	DUMP(context->getType());

	Type *functionType = ((llvm::StructType*)typeInfo->llvmType)->getTypeAtIndex(1u);//typeInfo->lastVariable->type;
	DUMP(functionType);

	Value *tmp = (llvm::Function*)funcInfo->llvmFunction;
	if(funcInfo == CodeInfo::funcInfo[0])
		tmp = ConstantPointerNull::get(llvm::PointerType::getUnqual((llvm::Type*)funcInfo->funcType->llvmType));

	DUMP(tmp->getType());
	tmp = builder->CreatePointerCast(tmp, functionType, "func_cast");
	builder->CreateStore(tmp, ptr);

	const Type *contextType = ((llvm::StructType*)typeInfo->llvmType)->getTypeAtIndex(0u);//TypeInfo *contextType = typeInfo->firstVariable->type;
	DUMP(contextType);

	if(funcInfo->type == FunctionInfo::NORMAL)
	{
		builder->CreateStore(ConstantPointerNull::get((PointerType*)contextType), context);
	}else{
		first->CompileLLVM();
		DUMP(V->getType());
		V = builder->CreatePointerCast(V, Type::getInt8PtrTy(getContext()), "context_cast");
		//V = builder->CreatePtrToInt(V, Type::getInt32Ty(getContext()), "int_cast");
		DUMP(V->getType());
		builder->CreateStore(V, context);
	}

	V = builder->CreateLoad(aggr, "func_res");
	DUMP(V->getType());
	//ThrowError(CodeInfo::lastKnownStartPos, "ERROR: NodeFunctionAddress");*/
}

void NodeGetUpvalue::CompileLLVM()
{
	CompileLLVMExtra();

	assert(parentFunc->extraParam);
	assert(parentFunc->extraParam->llvmValue);

	Value *closure = (Value*)parentFunc->extraParam->llvmValue;
	DUMP(closure->getType());

	// Load pointer to closure into a variable
	V = builder->CreateLoad(closure, "closure");
	DUMP(V->getType());
	// Cast it to char*
	V = builder->CreatePointerCast(V, Type::getInt8PtrTy(getContext()));
	DUMP(V->getType());
	// Shift to offset
	Value *arrayIndexHelper[2];
	//arrayIndexHelper[0] = ConstantInt::get(getContext(), APInt(64, 0, true));
	arrayIndexHelper[0] = ConstantInt::get(getContext(), APInt(32, closureElem, true));
	V = builder->CreateGEP(V, llvm::ArrayRef<Value*>(&arrayIndexHelper[0], 1));
	DUMP(V->getType());
	// cast to result type
	V = builder->CreatePointerCast(V, llvm::PointerType::getUnqual(typeInfo->llvmType));
	DUMP(V->getType());

	V = builder->CreateLoad(V, "closure_deref");

	//cmdList.push_back(VMCmd(cmdPushPtr, ADDRESS_RELATIVE, (unsigned short)typeInfo->size, closurePos));
	//cmdList.push_back(VMCmd(cmdPushPtrStk, 0, (unsigned short)typeInfo->size, closureElem));

	//ThrowError(CodeInfo::lastKnownStartPos, "ERROR: NodeGetUpvalue");
}

void NodeBlock::CompileLLVM()
{
	CompileLLVMExtra();

	first->CompileLLVM();
	if(parentFunction->closeUpvals)
	{
		assert(parentFunction->maxBlockDepth - stackFrameShift > 0);

		// Only local variables are closed in blocks
		for(VariableInfo *curr = parentFunction->firstLocal; curr; curr = curr->next)
		{
			if(curr->usedAsExternal && curr->blockDepth - parentFunction->vTopSize > stackFrameShift)
				builder->CreateCall2(functionCloseUpvalues, curr->llvmUpvalue, builder->CreatePointerCast(curr->llvmValue, llvm::Type::getInt8PtrTy(getContext())));
		}
	}
}

void NodeFunctionProxy::CompileLLVM()
{
}

void NodePointerCast::CompileLLVM()
{
	CompileLLVMExtra();

	first->CompileLLVM();

	V = builder->CreatePointerCast(V, (llvm::Type*)typeInfo->llvmType, "ptr_cast");
}

#else
void	NULLC_PreventLLVMWarning(){}
#endif
