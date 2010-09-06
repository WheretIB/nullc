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
#include "llvm/Target/TargetSelect.h"

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

#pragma warning(pop)

llvm::Module *TheModule = NULL;
llvm::IRBuilder<> Builder(llvm::getGlobalContext());
ExecutionEngine *TheExecutionEngine = NULL;
FunctionPassManager *OurFPM = NULL;
PassManager *OurPM = NULL;

std::vector<GlobalVariable*>	globals;

bool	enableOptimization = false;


FastVector<BasicBlock*>	breakStack;
FastVector<BasicBlock*> continueStack;

llvm::Function *F = NULL;
void		StartLLVMGeneration(unsigned functionsInModules)
{
	InitializeNativeTarget();

	delete TheModule;
	TheModule = new llvm::Module("NULLCCode", llvm::getGlobalContext());

	std::string ErrStr;
	TheExecutionEngine = EngineBuilder(TheModule).setErrorStr(&ErrStr).create();
	if(!TheExecutionEngine)
	{
		printf("Could not create ExecutionEngine: %s\n", ErrStr.c_str());
		exit(1);
	}
	//TheExecutionEngine->InstallLazyFunctionCreator(funcCreator);

	OurPM = new PassManager();

	OurFPM = new FunctionPassManager(TheModule);

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

	// Generate types
	typeVoid->llvmType = Type::getVoidTy(getGlobalContext());
	typeChar->llvmType = Type::getInt8Ty(getGlobalContext());
	typeShort->llvmType = Type::getInt16Ty(getGlobalContext());
	typeInt->llvmType = Type::getInt32Ty(getGlobalContext());
	typeLong->llvmType = Type::getInt64Ty(getGlobalContext());
	typeFloat->llvmType = Type::getFloatTy(getGlobalContext());
	typeDouble->llvmType = Type::getDoubleTy(getGlobalContext());
	typeObject->llvmType = StructType::get(getGlobalContext(), (Type*)typeInt->llvmType, Type::getInt8PtrTy(getGlobalContext()), (Type*)NULL);
	typeTypeid->llvmType = typeInt->llvmType;//StructType::get(getGlobalContext(), (Type*)typeInt->llvmType, (Type*)NULL);
	typeAutoArray->llvmType = StructType::get(getGlobalContext(), (Type*)typeInt->llvmType, Type::getInt8PtrTy(getGlobalContext()), (Type*)typeInt->llvmType, (Type*)NULL);
	std::vector<const Type*> typeList;
	// Construct abstract types for all classes
	for(unsigned int i = 0; i < CodeInfo::typeInfo.size(); i++)
	{
		TypeInfo *type = CodeInfo::typeInfo[i];
		if(type->llvmType || type->arrLevel || type->refLevel || type->funcType)
			continue;
		type->llvmType = llvm::OpaqueType::get(getGlobalContext());
	}
	PATypeHolder *holders = (PATypeHolder*)new char[sizeof(PATypeHolder) * CodeInfo::typeInfo.size()];//PATypeHolder[CodeInfo::typeInfo.size()];
	memset(holders, 0, sizeof(PATypeHolder) * CodeInfo::typeInfo.size());
	for(unsigned int i = 0; i < CodeInfo::typeInfo.size(); i++)
	{
		if(CodeInfo::typeInfo[i]->llvmType)
			continue;
		TypeInfo *type = CodeInfo::typeInfo[i];
		llvm::Type *subType = type->subType ? (type->subType->llvmType ? (Type*)type->subType->llvmType : holders[type->subType->typeIndex].get()) : NULL;
		//printf("Adding %s\n", type->GetFullTypeName());
		if(type->arrLevel && type->arrSize != TypeInfo::UNSIZED_ARRAY)
		{
			assert(subType);
			unsigned size = type->subType->size * type->arrSize;
			size = (size + 3) & ~3;
			new(&holders[i]) PATypeHolder(ArrayType::get(subType, type->subType->size ? size / type->subType->size : type->arrSize));
		}else if(type->arrLevel && type->arrSize == TypeInfo::UNSIZED_ARRAY){
			assert(subType);
			new(&holders[i]) PATypeHolder(StructType::get(getGlobalContext(), PointerType::getUnqual(subType), Type::getInt32Ty(getGlobalContext()), (Type*)NULL));
		}else if(type->refLevel){
			assert(subType);
			new(&holders[i]) PATypeHolder(type->subType == typeVoid ? Type::getInt8PtrTy(getGlobalContext()) : PointerType::getUnqual(subType));
		}else if(type->funcType){
			typeList.clear();
			for(unsigned int k = 0; k < type->funcType->paramCount; k++)
			{
				TypeInfo *argTypeInfo = type->funcType->paramType[k];
				llvm::Type *argType = argTypeInfo->llvmType ? (Type*)argTypeInfo->llvmType : holders[argTypeInfo->typeIndex].get();
				assert(argType);
				typeList.push_back(argType);
			}
			typeList.push_back(Type::getInt8PtrTy(getGlobalContext()));
			llvm::Type *retType = type->funcType->retType->llvmType ? (Type*)type->funcType->retType->llvmType : holders[type->funcType->retType->typeIndex].get();
			assert(retType);
			Type *functionType = llvm::FunctionType::get(retType, typeList, false);
			new(&holders[i]) PATypeHolder(StructType::get(getGlobalContext(), Type::getInt8PtrTy(getGlobalContext()), PointerType::getUnqual(functionType), (Type*)NULL));
		}
	}
	for(unsigned int i = 0; i < CodeInfo::typeInfo.size(); i++)
	{
		TypeInfo *type = CodeInfo::typeInfo[i];
		if(type->arrLevel || type->refLevel || type->funcType || type->type != TypeInfo::TYPE_COMPLEX || type == typeObject || type == typeTypeid || type == typeAutoArray)
			continue;

		//printf("Refining type %s\n", type->GetFullTypeName());

		PATypeHolder StructTy = (llvm::Type*)type->llvmType;

		typeList.clear();
		for(TypeInfo::MemberVariable *curr = type->firstVariable; curr; curr = curr->next)
		{
			if(!curr->type->llvmType)
				typeList.push_back((Type*)holders[curr->type->typeIndex].get());
			else
				typeList.push_back((Type*)curr->type->llvmType);
		}
		StructType *newType = StructType::get(getGlobalContext(), typeList);
		cast<OpaqueType>(StructTy.get())->refineAbstractTypeTo(newType);
		type->llvmType = cast<StructType>(StructTy.get());
	}
	for(unsigned int i = 0; i < CodeInfo::typeInfo.size(); i++)
	{
		TypeInfo *type = CodeInfo::typeInfo[i];
		if(!type->llvmType)
			type->llvmType = holders[i].get();
		/*printf("Assigning name %s to ", type->GetFullTypeName());
		((Type*)type->llvmType)->dump();
		TheModule->addTypeName(type->GetFullTypeName(), (Type*)type->llvmType);*/
	}
	delete[] (char*)holders;

	// Generate global variables
	for(unsigned int i = 0; i < CodeInfo::varInfo.size(); i++)
	{
		InplaceStr name = CodeInfo::varInfo[i]->name;
		globals.push_back(new GlobalVariable(*TheModule, (Type*)CodeInfo::varInfo[i]->varType->llvmType, false,
			CodeInfo::varInfo[i]->pos >> 24 ? llvm::GlobalVariable::ExternalWeakLinkage : llvm::GlobalVariable::ExternalLinkage,
			0, std::string(name.begin, name.end)));
		CodeInfo::varInfo[i]->llvmValue = globals.back();
		globals.back()->setInitializer(Constant::getNullValue((Type*)CodeInfo::varInfo[i]->varType->llvmType));
		//printf("global %s %.*s\n", CodeInfo::varInfo[i]->pos >> 24 ? "extern" : "", int(name.end - name.begin), name.begin);
	}


	Function::Create(TypeBuilder<types::i<32>(types::i<32>, types::i<32>), true>::get(getGlobalContext()), Function::ExternalLinkage, "llvmIntPow", TheModule);
	Function::Create(TypeBuilder<types::i<64>(types::i<64>, types::i<64>), true>::get(getGlobalContext()), Function::ExternalLinkage, "llvmLongPow", TheModule);
	Function::Create(TypeBuilder<types::ieee_double(types::ieee_double, types::ieee_double), true>::get(getGlobalContext()), Function::ExternalLinkage, "llvmDoublePow", TheModule);

	std::vector<const llvm::Type*> arrSetParams;
	arrSetParams.push_back(Type::getInt8PtrTy(getGlobalContext()));
	arrSetParams.push_back(Type::getInt8Ty(getGlobalContext()));
	arrSetParams.push_back(Type::getInt32Ty(getGlobalContext()));
	Function::Create(llvm::FunctionType::get(Type::getVoidTy(getGlobalContext()), arrSetParams, false), Function::ExternalLinkage, "__nullcSetArrayC", TheModule);

	arrSetParams[0] = Type::getInt16PtrTy(getGlobalContext());
	arrSetParams[1] = Type::getInt16Ty(getGlobalContext());
	Function::Create(llvm::FunctionType::get(Type::getVoidTy(getGlobalContext()), arrSetParams, false), Function::ExternalLinkage, "__llvmSetArrayS", TheModule);

	arrSetParams[0] = Type::getInt32PtrTy(getGlobalContext());
	arrSetParams[1] = Type::getInt32Ty(getGlobalContext());
	Function::Create(llvm::FunctionType::get(Type::getVoidTy(getGlobalContext()), arrSetParams, false), Function::ExternalLinkage, "__llvmSetArrayI", TheModule);

	arrSetParams[0] = Type::getInt64PtrTy(getGlobalContext());
	arrSetParams[1] = Type::getInt64Ty(getGlobalContext());
	Function::Create(llvm::FunctionType::get(Type::getVoidTy(getGlobalContext()), arrSetParams, false), Function::ExternalLinkage, "__llvmSetArrayL", TheModule);

	arrSetParams[0] = Type::getFloatPtrTy(getGlobalContext());
	arrSetParams[1] = Type::getFloatTy(getGlobalContext());
	Function::Create(llvm::FunctionType::get(Type::getVoidTy(getGlobalContext()), arrSetParams, false), Function::ExternalLinkage, "__llvmSetArrayF", TheModule);

	arrSetParams[0] = Type::getDoublePtrTy(getGlobalContext());
	arrSetParams[1] = Type::getDoubleTy(getGlobalContext());
	Function::Create(llvm::FunctionType::get(Type::getVoidTy(getGlobalContext()), arrSetParams, false), Function::ExternalLinkage, "__llvmSetArrayD", TheModule);

	Function::Create(TypeBuilder<void(types::i<32>), true>::get(getGlobalContext()), Function::ExternalLinkage, "llvmReturnInt", TheModule);
	Function::Create(TypeBuilder<void(types::i<64>), true>::get(getGlobalContext()), Function::ExternalLinkage, "llvmReturnLong", TheModule);
	Function::Create(TypeBuilder<void(types::ieee_double), true>::get(getGlobalContext()), Function::ExternalLinkage, "llvmReturnDouble", TheModule);	

	std::vector<const llvm::Type*> Arguments;
	for(unsigned int i = 0; i < CodeInfo::funcInfo.size(); i++)
	{
		FunctionInfo *funcInfo = CodeInfo::funcInfo[i];
		Arguments.clear();
		VariableInfo *curr = NULL;
		for(curr = funcInfo->firstParam; curr; curr = curr->next)
			Arguments.push_back((Type*)curr->varType->llvmType);
		Arguments.push_back(funcInfo->extraParam ? (Type*)funcInfo->extraParam->varType->llvmType : Type::getInt32PtrTy(getGlobalContext()));
		llvm::FunctionType *FT = llvm::FunctionType::get((Type*)funcInfo->retType->llvmType, Arguments, false);
		funcInfo->llvmFunction = llvm::Function::Create(FT, i < functionsInModules ? llvm::Function::ExternalWeakLinkage : llvm::Function::ExternalLinkage, funcInfo->name, TheModule);
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

//#define DUMP(x) x->dump()
#define DUMP(x) x

//BasicBlock *globalExit = NULL;

void		StartGlobalCode()
{
	F = llvm::Function::Create(TypeBuilder<void(), true>::get(getGlobalContext()), llvm::Function::ExternalLinkage, "Global", TheModule);

	llvm::BasicBlock *BB = llvm::BasicBlock::Create(llvm::getGlobalContext(), "global_entry", F);
	Builder.SetInsertPoint(BB);

	//globalExit = BasicBlock::Create(getGlobalContext(), "globalReturn");
}

std::vector<unsigned char> out;
const char*	GetLLVMIR(unsigned& length)
{
	if(F->back().empty() || !F->back().back().isTerminator())
		Builder.CreateRetVoid();

	/*F->getBasicBlockList().push_back(globalExit);
	Builder.SetInsertPoint(globalExit);
	Builder.CreateRet(ConstantInt::get(getGlobalContext(), APInt(32, ~0ull, true)));*/

	if(enableOptimization)
	{
		if(!F->getEntryBlock().empty())
			OurFPM->run(*F);
		OurPM->run(*TheModule);
		if(!F->getEntryBlock().empty())
			OurFPM->run(*F);
	}

	DUMP(TheModule);

//	unsigned start = clock();

	verifyFunction(*F);
	verifyModule(*TheModule);

//	unsigned verified = clock() - start;
//	start = clock();

	out.clear();
	BitstreamWriter bw = BitstreamWriter(out);
	WriteBitcodeToStream(TheModule, bw);
	out.push_back(0);
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
	delete TheModule;
	TheModule = NULL;

	//llvm::getGlobalContext().~LLVMContext();
	//Builder.~IRBuilder();
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
		V = Builder.CreateIntCast(V, Type::getInt32Ty(getGlobalContext()), true, "tmp_toint");
		break;
	case DTYPE_FLOAT:
		V = Builder.CreateFPCast(V, Type::getDoubleTy(getGlobalContext()), "tmp_ftod");
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
			return Builder.CreateSIToFP(V, (Type*)secondType->llvmType, "tmp_itod");
		else if(first == STYPE_LONG)
			return Builder.CreateSIToFP(V, (Type*)secondType->llvmType, "tmp_ltod");
		else if(fDT != sDT)
			return Builder.CreateFPCast(V, (Type*)secondType->llvmType, "tmp_fpcast");
	}else if(second == STYPE_LONG){
		if(first == STYPE_INT)
			return Builder.CreateIntCast(V, (Type*)secondType->llvmType, true, "tmp_itol");
		else if(first == STYPE_DOUBLE)
			return Builder.CreateFPToSI(V, (Type*)secondType->llvmType, "tmp_dtol");
		else if(fDT != sDT)
			return Builder.CreateIntCast(V, (Type*)secondType->llvmType, true, "tmp_lcast");
	}else if(second == STYPE_INT){
		if(first == STYPE_DOUBLE)
			return Builder.CreateFPToSI(V, (Type*)secondType->llvmType, "tmp_dtoi");
		else if(first == STYPE_LONG)
			return Builder.CreateIntCast(V, (Type*)secondType->llvmType, true, "tmp_ltoi");
		else if(fDT != sDT)
			return Builder.CreateIntCast(V, (Type*)secondType->llvmType, true, "tmp_icast");
	}
	return V;
}

Value*	ConvertFirstForSecond(Value *V, TypeInfo *firstType, TypeInfo *secondType)
{
	asmDataType	fDT = firstType->dataType, sDT = secondType->dataType;
	asmStackType first = stackTypeForDataType(fDT), second = stackTypeForDataType(sDT);
	if(first == STYPE_INT && second == STYPE_DOUBLE)
		return Builder.CreateSIToFP(V, Type::getDoubleTy(getGlobalContext()), "tmp_itod");
	if(first == STYPE_LONG && second == STYPE_DOUBLE)
		return Builder.CreateSIToFP(V, Type::getDoubleTy(getGlobalContext()), "tmp_ltod");
	if(first == STYPE_INT && second == STYPE_LONG)
		return Builder.CreateIntCast(V, Type::getInt64Ty(getGlobalContext()), true, "tmp_itol");

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
		V = ConstantInt::get(getGlobalContext(), APInt(8, num.integer, true));
		break;
	case DTYPE_SHORT:
		V = ConstantInt::get(getGlobalContext(), APInt(16, num.integer, true));
		break;
	case DTYPE_INT:
		if(typeInfo->refLevel && num.integer == 0)
			V = ConstantPointerNull::get((PointerType*)typeInfo->llvmType);
		else
			V = ConstantInt::get(getGlobalContext(), APInt(32, num.integer, true));
		break;
	case DTYPE_LONG:
		if(typeInfo->refLevel && num.integer64 == 0)
			V = ConstantPointerNull::get((PointerType*)typeInfo->llvmType);
		else
			V = ConstantInt::get(getGlobalContext(), APInt(64, num.integer64, true));
		break;
	case DTYPE_FLOAT:
		V = ConstantFP::get(getGlobalContext(), APFloat((float)num.real));
		break;
	case DTYPE_DOUBLE:
		V = ConstantFP::get(getGlobalContext(), APFloat(num.real));
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
	if(!parentFunction)
	{//&& first->typeInfo != typeInt)
	//	ThrowError(CodeInfo::lastKnownStartPos, "ERROR: NodeReturnOp !typeInfo && first->typeInfo != typeInt");
		V = PromoteToStackType(V, first->typeInfo);
		TypeInfo *retType = GetStackType(first->typeInfo);
		if(retType == typeInt)
			Builder.CreateCall(TheModule->getFunction("llvmReturnInt"), V);
		else if(retType == typeLong)
			Builder.CreateCall(TheModule->getFunction("llvmReturnLong"), V);
		else if(retType == typeDouble)
			Builder.CreateCall(TheModule->getFunction("llvmReturnDouble"), V);
		else
			ThrowError(CodeInfo::lastKnownStartPos, "ERROR: unknown global return type");
		Builder.CreateRetVoid();
		V = NULL;
		return;
	}

	if(parentFunction && parentFunction->closeUpvals)
		ThrowError(CodeInfo::lastKnownStartPos, "ERROR: NodeReturnOp upvalues unsupported");
	// If return is from coroutine, we either need to reset jumpOffset to the beginning of a function, or set it to instruction after return
	if(parentFunction && parentFunction->type == FunctionInfo::COROUTINE)
		ThrowError(CodeInfo::lastKnownStartPos, "ERROR: NodeReturnOp yield unsupported");
	
	Builder.CreateRet(V);

	//BasicBlock *afterReturn = BasicBlock::Create(getGlobalContext(), "afterReturn", F);
	//Builder.SetInsertPoint(afterReturn);

	//Builder.CreateRetVoid();*/
}

// CreateEntryBlockAlloca - Create an alloca instruction in the entry block of the function.  This is used for mutable variables etc.
AllocaInst* CreateEntryBlockAlloca(Function *TheFunction, const std::string &VarName, const llvm::Type* type)
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
	}//else if(typeInfo != typeVoid)
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
				V = Builder.CreatePointerCast(V, PointerType::getUnqual((Type*)typeInfo->subType->llvmType), "ptr_any");
				DUMP(V->getType());
			}
			Value *arrayIndexHelper[2];
			DUMP(aggr->getType());
			arrayIndexHelper[0] = ConstantInt::get(getGlobalContext(), APInt(64, 0, true));
			arrayIndexHelper[1] = ConstantInt::get(getGlobalContext(), APInt(32, index--, true));
			Value *target = Builder.CreateGEP(aggr, &arrayIndexHelper[0], &arrayIndexHelper[2], "arr_part");
			DUMP(target->getType());
			Builder.CreateStore(V, target);
		}else if(V && typeInfo->arrLevel && curr->nodeType != typeNodeZeroOp){
			DUMP(V->getType());
			V = PromoteToStackType(V, curr->typeInfo);
			DUMP(V->getType());
			DUMP(aggr->getType());
			Value *arrayIndexHelper[2];
			arrayIndexHelper[0] = ConstantInt::get(getGlobalContext(), APInt(64, 0, true));
			//printf("Indexing %d\n", typeInfo->subType == typeChar ? index * 4 : index);
			arrayIndexHelper[1] = ConstantInt::get(getGlobalContext(), APInt(32, typeInfo->subType == typeChar ? index * 4 : index, true));
			index--;
			Value *target = Builder.CreateGEP(aggr, &arrayIndexHelper[0], &arrayIndexHelper[2], "arr_part");
			DUMP(target->getType());
			if(typeInfo->subType == typeChar)
			{
				target = Builder.CreatePointerCast(target, Type::getInt32PtrTy(getGlobalContext()), "ptr_any");
				DUMP(target->getType());
			}
			Builder.CreateStore(V, target);
		}
		curr = curr->next;
	}while(curr);
	if(typeInfo->arrLevel)// && typeInfo->arrSize == TypeInfo::UNSIZED_ARRAY)
		V = Builder.CreateLoad(aggr, "arr_aggr");
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

	//Function *CalleeF = TheModule->getFunction(funcInfo->name);
	//if(CalleeF)
	//	ThrowError(CodeInfo::lastKnownStartPos, "ERROR: NodeFuncDef CalleeF");

	/*std::vector<const llvm::Type*> Arguments;
	VariableInfo *curr = NULL;
	for(curr = funcInfo->firstParam; curr; curr = curr->next)
		Arguments.push_back((Type*)curr->varType->llvmType);
	Arguments.push_back((Type*)funcInfo->extraParam->varType->llvmType);
	llvm::FunctionType *FT = llvm::FunctionType::get((Type*)funcInfo->retType->llvmType, Arguments, false);
	F = llvm::Function::Create(FT, llvm::Function::ExternalLinkage, funcInfo->name, TheModule);*/
	F = (Function*)funcInfo->llvmFunction;
	funcInfo->llvmImplemented = true;

	llvm::BasicBlock *BB = llvm::BasicBlock::Create(llvm::getGlobalContext(), "entry", F);
	Builder.SetInsertPoint(BB);

	Function::arg_iterator AI;
	VariableInfo *curr = NULL;
	for(AI = F->arg_begin(), curr = funcInfo->firstParam; AI != F->arg_end() && curr; AI++, curr = curr->next)
	{
		AI->setName(std::string(curr->name.begin, curr->name.end));
		AllocaInst *Alloca = CreateEntryBlockAlloca(F, std::string(curr->name.begin, curr->name.end), (Type*)curr->varType->llvmType);
		Builder.CreateStore(AI, Alloca);
		curr->llvmValue = Alloca;
	}
	AI->setName(std::string(funcInfo->extraParam->name.begin, funcInfo->extraParam->name.end));
	AllocaInst *Alloca = CreateEntryBlockAlloca(F, std::string(funcInfo->extraParam->name.begin, funcInfo->extraParam->name.end), (Type*)funcInfo->extraParam->varType->llvmType);
	Builder.CreateStore(AI, Alloca);
	funcInfo->extraParam->llvmValue = Alloca;

	for(curr = funcInfo->firstLocal; curr; curr = curr->next)
		curr->llvmValue = CreateEntryBlockAlloca(F, std::string(curr->name.begin, curr->name.end), (Type*)curr->varType->llvmType);

	first->CompileLLVM();
	if(first->nodeType == typeNodeZeroOp)
	{
		if(funcInfo->retType != typeVoid)
			ThrowError(CodeInfo::lastKnownStartPos, "ERROR: NodeFuncDef function must return a value");
		Builder.CreateRetVoid();
	}

	if(F->back().empty() || !F->back().back().isTerminator())
	{
		if(funcInfo->retType == typeVoid)
			Builder.CreateRetVoid();
		else
			Builder.CreateRet(ConstantInt::get(getGlobalContext(), APInt(32, 0, true)));
	}

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
			if(curr->typeInfo->size == 0)
				ThrowError(CodeInfo::lastKnownStartPos, "ERROR: NodeFuncCall curr->typeInfo->size == 0");
			curr->CompileLLVM();
			if(!V)
				ThrowError(CodeInfo::lastKnownStartPos, "ERROR: NodeFuncCall V = NULL");
			//printf("%s\t", curr->typeInfo->GetFullTypeName());
			DUMP(V->getType());
			V = ConvertFirstToSecond(V, curr->typeInfo, *paramType);
			if(*paramType == typeFloat)
				V = Builder.CreateFPCast(V, (Type*)typeFloat->llvmType, "dtof");
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
		Builder.CreateStore(func, funcPtr);

		Value *ptr = Builder.CreateStructGEP(funcPtr, 1, "ptr_part");
		DUMP(ptr->getType());
		Value *context = Builder.CreateStructGEP(funcPtr, 0, "ctx_part");
		DUMP(context->getType());

		args.push_back(Builder.CreateLoad(context, "ctx_deref"));

		CalleeF = (Function*)Builder.CreateLoad(ptr, "func_deref");

		//ThrowError(CodeInfo::lastKnownStartPos, "ERROR: NodeFuncCall funcInfo = NULL");
	}else{
		CalleeF = (Function*)funcInfo->llvmFunction;// TheModule->getFunction(funcInfo->name);
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
				V = Builder.CreatePointerCast(V, last->getType());
				DUMP(V->getType());
			}
			args.push_back(V);
		}else{
			args.push_back(ConstantPointerNull::get(Type::getInt32PtrTy(getGlobalContext())));
		}
	}
	if(funcType->retType != typeVoid)
	{
		DUMP(CalleeF->getType());
		V = Builder.CreateCall(CalleeF, args.begin(), args.end());//, funcInfo->name);
	}else{
		Builder.CreateCall(CalleeF, args.begin(), args.end());
		V = NULL;
	}
	if(funcInfo && funcInfo->nameHash == GetStringHash("__newA"))
	{
		DUMP(V->getType());
		DUMP(((Type*)typeInfo->llvmType));

		Value *arrTemp = CreateEntryBlockAlloca(F, "newa_tmp", V->getType());
		DUMP(arrTemp->getType());
		Builder.CreateStore(V, arrTemp);

		DUMP(arrTemp->getType());
		DUMP(((Type*)typeInfo->llvmType));

		V = Builder.CreatePointerCast(arrTemp, PointerType::getUnqual((Type*)typeInfo->llvmType), "newa_rescast");
		DUMP(V->getType());
		V = Builder.CreateLoad(V, "newa_load");
		DUMP(V->getType());
	}
	if(funcInfo && funcInfo->nameHash == GetStringHash("__newS"))
	{
		DUMP(V->getType());
		DUMP(((Type*)typeInfo->llvmType));
		V = Builder.CreatePointerCast(V, (Type*)typeInfo->llvmType, "news_rescast");
	}
	if(funcInfo && funcInfo->nameHash == GetStringHash("__redirect"))
	{
		DUMP(V->getType());
		DUMP(((Type*)typeInfo->llvmType));

		Value *funcTemp = CreateEntryBlockAlloca(F, "redirect_tmp", V->getType());
		DUMP(funcTemp->getType());
		Builder.CreateStore(V, funcTemp);

		DUMP(funcTemp->getType());
		DUMP(((Type*)typeInfo->llvmType));

		V = Builder.CreatePointerCast(funcTemp, PointerType::getUnqual((Type*)typeInfo->llvmType), "redirect_cast");
		DUMP(V->getType());
		V = Builder.CreateLoad(V, "redirect_res");
		DUMP(V->getType());
	}
}

void NodeUnaryOp::CompileLLVM()
{
	CompileLLVMExtra();

	asmOperType aOT = operTypeForStackType[first->typeInfo->stackType];

	CmdID id = vmCmd.cmd;
	if(aOT == OTYPE_INT)
		id = vmCmd.cmd;
	else if(aOT == OTYPE_LONG)
		id = (CmdID)(vmCmd.cmd + 1);
	else
		id = (CmdID)(vmCmd.cmd + 2);

	// Child node computes value to V
	first->CompileLLVM();
	Value *zeroV = ConstantInt::get(getGlobalContext(), APInt(id == cmdLogNotL ? 64 : 32, 0, true));
	if(first->typeInfo == typeObject)
	{
		DUMP(V->getType());
		Value *aggr = CreateEntryBlockAlloca(F, "ref_tmp", (Type*)typeObject->llvmType);
		Builder.CreateStore(V, aggr);
		DUMP(aggr->getType());
		V = Builder.CreateStructGEP(aggr, 1, "tmp_ptr");
		DUMP(V->getType());
		V = Builder.CreateLoad(V, "tmp_ptr");
		DUMP(V->getType());
		V = Builder.CreatePtrToInt(V, (Type*)typeInt->llvmType, "as_int");
		DUMP(V->getType());
		id = cmdLogNot;
		//ThrowError(CodeInfo::lastKnownStartPos, "ERROR: !CalleeF");
	}else if(first->typeInfo->refLevel){
		DUMP(V->getType());
		zeroV = ConstantPointerNull::get((PointerType*)V->getType());
	}
	switch(id)
	{
	case cmdNeg:
	case cmdNegL:
	case cmdNegD:
		V = Builder.CreateNeg(V, "neg_tmp");
		break;

	case cmdBitNot:
	case cmdBitNotL:
		V = Builder.CreateNot(V, "not_tmp");
		break;
	case cmdLogNot:	// !	logical NOT
	case cmdLogNotL:
		// Compare that not equal to 0
		V = Builder.CreateICmpNE(V, zeroV, "to_bool");
		// Not 
		V = Builder.CreateNot(V, "lognot_tmp");
		V = Builder.CreateIntCast(V, Type::getInt32Ty(getGlobalContext()), true, "booltmp");
		break;
	case cmdPushTypeID:
	case cmdFuncAddr:
		V = ConstantInt::get((Type*)typeInt->llvmType, APInt(32, vmCmd.argument));
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
	//Builder.CreateIntToPtr
	V = (Value*)varInfo->llvmValue;//NamedValues[varInfo->nameHash];//Builder.CreateGEP(NamedValues[varInfo->nameHash], ConstantInt::get(Type::getInt32Ty(getGlobalContext()), APInt(32, 0)), "tmp_ptr");
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
		ThrowError(CodeInfo::lastKnownStartPos, "ERROR: NodeDereference closureFunc");
	}
	first->CompileLLVM();
	if(typeInfo != first->typeInfo->subType)
	{
		DUMP(V->getType());
		V = Builder.CreatePointerCast(V, PointerType::getUnqual((Type*)typeInfo->llvmType), "deref_cast");
		DUMP(V->getType());
	}
	if(!V)
		ThrowError(CodeInfo::lastKnownStartPos, "ERROR: NodeDereference V = NULL");
	V = Builder.CreateLoad(V, "temp_deref");
}

void MakeBinaryOp(Value *left, Value *right, CmdID cmdID, asmStackType fST)
{
	switch(cmdID)
	{
	case cmdAdd:
		V = Builder.CreateAdd(left, right, "add_tmp");
		break;		// a + b
	case cmdSub:
		V = Builder.CreateSub(left, right, "sub_tmp");
		break;		// a - b
	case cmdMul:
		V = Builder.CreateMul(left, right, "mul_tmp");
		break;		// a * b
	case cmdDiv:
		if(fST == STYPE_DOUBLE)
			V = Builder.CreateFDiv(left, right, "fdiv_tmp");
		else
			V = Builder.CreateSDiv(left, right, "idiv_tmp");
		break;		// a / b
	case cmdPow:
		if(fST == STYPE_INT)
			V = Builder.CreateCall2(TheModule->getFunction("llvmIntPow"), left, right, "ipow_tmp");
		else if(fST == STYPE_LONG)
			V = Builder.CreateCall2(TheModule->getFunction("llvmLongPow"), left, right, "lpow_tmp");
		else
			V = Builder.CreateCall2(TheModule->getFunction("llvmDoublePow"), left, right, "fpow_tmp");
		break;		// power(a, b) (**)
	case cmdMod:
		if(fST == STYPE_DOUBLE)
			V = Builder.CreateFRem(left, right, "frem_tmp");
		else
			V = Builder.CreateSRem(left, right, "irem_tmp");
		break;		// a % b
	case cmdLess:
		if(fST == STYPE_DOUBLE)
			V = Builder.CreateFCmpOLT(left, right);
		else
			V = Builder.CreateICmpSLT(left, right);
		V = Builder.CreateZExt(V, Type::getInt32Ty(getGlobalContext()), "to_int");
		break;	// a < b
	case cmdGreater:
		if(fST == STYPE_DOUBLE)
			V = Builder.CreateFCmpOGT(left, right);
		else
			V = Builder.CreateICmpSGT(left, right);
		V = Builder.CreateZExt(V, Type::getInt32Ty(getGlobalContext()), "to_int");
		break;	// a > b
	case cmdLEqual:
		if(fST == STYPE_DOUBLE)
			V = Builder.CreateFCmpOLE(left, right);
		else
			V = Builder.CreateICmpSLE(left, right);
		V = Builder.CreateZExt(V, Type::getInt32Ty(getGlobalContext()), "to_int");
		break;	// a <= b
	case cmdGEqual:
		if(fST == STYPE_DOUBLE)
			V = Builder.CreateFCmpOGE(left, right);
		else
			V = Builder.CreateICmpSGE(left, right);
		V = Builder.CreateZExt(V, Type::getInt32Ty(getGlobalContext()), "to_int");
		break;	// a >= b
	case cmdEqual:
		if(fST == STYPE_DOUBLE)
			V = Builder.CreateFCmpOEQ(left, right);
		else
			V = Builder.CreateICmpEQ(left, right);
		V = Builder.CreateZExt(V, Type::getInt32Ty(getGlobalContext()), "to_int");
		break;	// a == b
	case cmdNEqual:
		if(fST == STYPE_DOUBLE)
			V = Builder.CreateFCmpONE(left, right);
		else
			V = Builder.CreateICmpNE(left, right);
		V = Builder.CreateZExt(V, Type::getInt32Ty(getGlobalContext()), "to_int");
		break;	// a != b
	case cmdShl:
		V = Builder.CreateShl(left, right);
		break;		// a << b
	case cmdShr:
		V = Builder.CreateAShr(left, right);
		break;		// a >> b
	case cmdBitAnd:
		V = Builder.CreateBinOp(llvm::Instruction::And, left, right);
		break;
	case cmdBitOr:
		V = Builder.CreateBinOp(llvm::Instruction::Or, left, right);
		break;
	case cmdBitXor:
		V = Builder.CreateBinOp(llvm::Instruction::Xor, left, right);
		break;
	case cmdLogAnd:
		ThrowError(CodeInfo::lastKnownStartPos, "ERROR: MakeBinaryOp cmdLogAnd");
		break;
	case cmdLogOr:
		ThrowError(CodeInfo::lastKnownStartPos, "ERROR: MakeBinaryOp cmdLogOr");
		break;
	case cmdLogXor:
		left = Builder.CreateICmpNE(left, ConstantInt::get(getGlobalContext(), APInt(fST == STYPE_LONG ? 64 : 32, 0, true)), "to_bool_left");
		right = Builder.CreateICmpNE(right, ConstantInt::get(getGlobalContext(), APInt(fST == STYPE_LONG ? 64 : 32, 0, true)), "to_bool_rught");
		V = Builder.CreateBinOp(llvm::Instruction::Xor, left, right, "less_tmp");
		V = Builder.CreateIntCast(V, Type::getInt32Ty(getGlobalContext()), true, "to_int");
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
		if(fST == STYPE_LONG)
			left = Builder.CreateIntCast(left, (Type*)typeInt->llvmType, true, "sc_l");
		left = Builder.CreateICmpNE(left, ConstantInt::get(getGlobalContext(), APInt(32, 0)), "left_cond");

		BasicBlock *fastExit = BasicBlock::Create(getGlobalContext(), "fastExit", F);
		BasicBlock *slowExit = BasicBlock::Create(getGlobalContext(), "slowExit", F);
		BasicBlock *slowOneExit = BasicBlock::Create(getGlobalContext(), "slowOneExit", F);
		BasicBlock *slowZeroExit = BasicBlock::Create(getGlobalContext(), "slowZeroExit", F);
		BasicBlock *slowMergeExit = BasicBlock::Create(getGlobalContext(), "slowMergeExit", F);
		BasicBlock *mergeExit = BasicBlock::Create(getGlobalContext(), "mergeExit", F);

		Builder.CreateCondBr(left, cmdID == cmdLogOr ? fastExit : slowExit, cmdID == cmdLogOr ? slowExit : fastExit);

		Builder.SetInsertPoint(slowExit);
		second->CompileLLVM();
		Value *right = V;
		if(fST == STYPE_LONG)
			right = Builder.CreateIntCast(right, (Type*)typeInt->llvmType, true, "sc_l");
		right = Builder.CreateICmpNE(right, ConstantInt::get(getGlobalContext(), APInt(32, 0)), "left_cond");
		Builder.CreateCondBr(right, slowOneExit, slowZeroExit);

		Builder.SetInsertPoint(slowOneExit);
		Value *rightOne = ConstantInt::get(getGlobalContext(), APInt(32, 1));
		Builder.CreateBr(slowMergeExit);
		slowOneExit = Builder.GetInsertBlock();

		Builder.SetInsertPoint(slowZeroExit);
		Value *rightZero = ConstantInt::get(getGlobalContext(), APInt(32, 0));
		Builder.CreateBr(slowMergeExit);
		slowZeroExit = Builder.GetInsertBlock();

		Builder.SetInsertPoint(slowMergeExit);
		PHINode *PN = Builder.CreatePHI((Type*)typeInt->llvmType, "slow_merge_tmp");
		PN->addIncoming(rightOne, slowOneExit);
		PN->addIncoming(rightZero, slowZeroExit);
		Builder.CreateBr(mergeExit);
		slowMergeExit = Builder.GetInsertBlock();

		Value *leftUnknown = PN;

		Builder.SetInsertPoint(fastExit);
		Value *leftZero = ConstantInt::get(getGlobalContext(), APInt(32, cmdID == cmdLogOr ? 1 : 0));
		Builder.CreateBr(mergeExit);
		fastExit = Builder.GetInsertBlock();

		Builder.SetInsertPoint(mergeExit);
		PHINode *PN2 = Builder.CreatePHI((Type*)typeInt->llvmType, "merge_tmp");
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
		arrayIndexHelper[0] = ConstantInt::get(getGlobalContext(), APInt(64, 0, true));
		arrayIndexHelper[1] = ConstantInt::get(getGlobalContext(), APInt(32, 0));
		target = Builder.CreateGEP(target, &arrayIndexHelper[0], &arrayIndexHelper[2], "arr_begin");
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
		Function *setFunc = TheModule->getFunction(fName);
		assert(setFunc);
		DUMP(target->getType());
		DUMP(value->getType());
		Builder.CreateCall3(setFunc, target, value, ConstantInt::get((Type*)typeInt->llvmType, APInt(32u, elemCount, true)));
		V = NULL;
		return;
	}

	value = PromoteToStackType(value, second->typeInfo);
	value = ConvertFirstToSecond(value, GetStackType(second->typeInfo), first->typeInfo->subType);
	if(!value)
		ThrowError(CodeInfo::lastKnownStartPos, "ERROR: NodeVariableSet value = NULL after");
	DUMP(value->getType());
	Builder.CreateStore(value, target);
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
	Value *zeroV = ConstantInt::get(getGlobalContext(), APInt(32, 0, true));
	if(typeInfo == typeObject)
	{
		DUMP(V->getType());
		Value *arrTemp = CreateEntryBlockAlloca(F, "autoref_tmp", V->getType());
		DUMP(arrTemp->getType());
		Builder.CreateStore(V, arrTemp);

		V = Builder.CreateStructGEP(arrTemp, 1, "autoref_ptr");
		DUMP(V->getType());
		V = Builder.CreateLoad(V);
		DUMP(V->getType());
		V = Builder.CreatePtrToInt(V, Type::getInt32Ty(getGlobalContext()), "as_int");
		DUMP(V->getType());
	}else if(typeInfo->refLevel){
		DUMP(V->getType());
		zeroV = ConstantPointerNull::get((PointerType*)V->getType());
	}else if(typeInfo != typeInt){
		DUMP(V->getType());
		V = PromoteToStackType(V, typeInfo);
		DUMP(V->getType());
		V = Builder.CreateIntCast(V, Type::getInt32Ty(getGlobalContext()), true, "as_int");
		DUMP(V->getType());
	}
	return Builder.CreateICmpNE(V, zeroV, "cond_value");
}

void NodeIfElseExpr::CompileLLVM()
{
	CompileLLVMExtra();

	BasicBlock *trueBlock = BasicBlock::Create(getGlobalContext(), "trueBlock", F);
	BasicBlock *falseBlock = third ? BasicBlock::Create(getGlobalContext(), "falseBlock") : NULL;
	BasicBlock *exitBlock = BasicBlock::Create(getGlobalContext(), "exitBlock");
	
	Value *left = NULL, *right = NULL;

	// Compute condition
	first->CompileLLVM();
	Value *cond = GenerateCompareToZero(V, first->typeInfo);

	Builder.CreateCondBr(cond, trueBlock, third ? falseBlock : exitBlock);

	Builder.SetInsertPoint(trueBlock);

	second->CompileLLVM();
	left = V;
	trueBlock = Builder.GetInsertBlock();
	if(trueBlock->empty() || !trueBlock->back().isTerminator())//if(F->back().empty() || !F->back().back().isTerminator())
		Builder.CreateBr(exitBlock);
	
	if(third)
	{
		F->getBasicBlockList().push_back(falseBlock);
		Builder.SetInsertPoint(falseBlock);
		third->CompileLLVM();
		right = V;
		falseBlock = Builder.GetInsertBlock();
		if(falseBlock->empty() || !falseBlock->back().isTerminator())//F->back().empty() || !F->back().back().isTerminator())
			Builder.CreateBr(exitBlock);
		
	}
	F->getBasicBlockList().push_back(exitBlock);
	Builder.SetInsertPoint(exitBlock);

	V = NULL;
	if(typeInfo != typeVoid)
	{
		PHINode *PN2 = Builder.CreatePHI((Type*)typeInfo->llvmType, "merge_tmp");
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

	BasicBlock *condBlock = BasicBlock::Create(getGlobalContext(), "condBlock", F);
	BasicBlock *bodyBlock = BasicBlock::Create(getGlobalContext(), "bodyBlock");
	BasicBlock *incrementBlock = BasicBlock::Create(getGlobalContext(), "incrementBlock");
	BasicBlock *exitBlock = BasicBlock::Create(getGlobalContext(), "exitBlock");

	Builder.CreateBr(condBlock);
	Builder.SetInsertPoint(condBlock);

	second->CompileLLVM();
	Value *cond = GenerateCompareToZero(V, second->typeInfo);
	Builder.CreateCondBr(cond, bodyBlock, exitBlock);

	breakStack.push_back(exitBlock);
	continueStack.push_back(incrementBlock);

	F->getBasicBlockList().push_back(bodyBlock);
	Builder.SetInsertPoint(bodyBlock);
	fourth->CompileLLVM();
	Builder.CreateBr(incrementBlock);

	continueStack.pop_back();
	breakStack.pop_back();

	F->getBasicBlockList().push_back(incrementBlock);
	Builder.SetInsertPoint(incrementBlock);
	third->CompileLLVM();
	Builder.CreateBr(condBlock);

	F->getBasicBlockList().push_back(exitBlock);
	Builder.SetInsertPoint(exitBlock);
	V = NULL;
}

void NodeWhileExpr::CompileLLVM()
{
	CompileLLVMExtra();

	BasicBlock *condBlock = BasicBlock::Create(getGlobalContext(), "condBlock", F);
	BasicBlock *bodyBlock = BasicBlock::Create(getGlobalContext(), "bodyBlock", F);
	BasicBlock *exitBlock = BasicBlock::Create(getGlobalContext(), "exitBlock", F);

	Builder.CreateBr(condBlock);
	Builder.SetInsertPoint(condBlock);

	first->CompileLLVM();
	Value *cond = GenerateCompareToZero(V, first->typeInfo);
	Builder.CreateCondBr(cond, bodyBlock, exitBlock);

	breakStack.push_back(exitBlock);
	continueStack.push_back(condBlock);

	Builder.SetInsertPoint(bodyBlock);
	second->CompileLLVM();
	Builder.CreateBr(condBlock);

	continueStack.pop_back();
	breakStack.pop_back();

	Builder.SetInsertPoint(exitBlock);
	V = NULL;
}

void NodeDoWhileExpr::CompileLLVM()
{
	CompileLLVMExtra();

	BasicBlock *bodyBlock = BasicBlock::Create(getGlobalContext(), "bodyBlock", F);
	BasicBlock *condBlock = BasicBlock::Create(getGlobalContext(), "condBlock", F);
	BasicBlock *exitBlock = BasicBlock::Create(getGlobalContext(), "exitBlock", F);

	Builder.CreateBr(bodyBlock);
	Builder.SetInsertPoint(bodyBlock);

	breakStack.push_back(exitBlock);
	continueStack.push_back(condBlock);

	first->CompileLLVM();

	continueStack.pop_back();
	breakStack.pop_back();

	Builder.CreateBr(condBlock);
	Builder.SetInsertPoint(condBlock);

	second->CompileLLVM();
	Value *cond = GenerateCompareToZero(V, second->typeInfo);
	Builder.CreateCondBr(cond, bodyBlock, exitBlock);

	Builder.SetInsertPoint(exitBlock);
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
		V = Builder.CreateLoad(address, "incdecpre_tmp");
		DUMP(V->getType());
		//DUMP(V->getType());
		Value *mod = Builder.CreateAdd(V, ConvertFirstToSecond(ConstantInt::get(getGlobalContext(), APInt(32, incOp ? 1 : -1, true)), typeInt, typeInfo), "incdecpost_res");
		Builder.CreateStore(mod, address);
		V = mod;
	}else{
		// x++
		V = Builder.CreateLoad(address, "incdecpre_tmp");
		DUMP(V->getType());
		Value *mod = Builder.CreateAdd(V, ConvertFirstToSecond(ConstantInt::get(getGlobalContext(), APInt(32, incOp ? 1 : -1, true)), typeInt, typeInfo), "incdecpost_res");
		Builder.CreateStore(mod, address);
	}
}

void NodeVariableModify::CompileLLVM()
{
	CompileLLVMExtra();

	first->CompileLLVM();
	Value *address = V;

	second->CompileLLVM();
	Value *right = V;

	Value *left = Builder.CreateLoad(address, "modify_left");
	
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
	Builder.CreateStore(V, address);
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
		Builder.CreateStore(address, aggr);
		DUMP(aggr->getType());
		/*V = Builder.CreateStructGEP(aggr, 1, "tmp_arr");
		Value *arrSize = Builder.CreateLoad(V, "arr_size");
		// Check array size
		BasicBlock *failBlock = BasicBlock::Create(getGlobalContext(), "failBlock", F);
		BasicBlock *passBlock = BasicBlock::Create(getGlobalContext(), "passBlock", F);
		Value *check = Builder.CreateICmpULT(index, arrSize, "check_res");
		Builder.CreateCondBr(check, passBlock, failBlock);
		Builder.SetInsertPoint(failBlock);
		Builder.CreateBr(globalExit);
		Builder.SetInsertPoint(passBlock);*/
		// Access array
		V = Builder.CreateStructGEP(aggr, 0, "tmp_arr");
		Value *arrPtr = Builder.CreateLoad(V, "tmp_arrptr");
		arrayIndexHelper[0] = index;
		V = Builder.CreateGEP(arrPtr, &arrayIndexHelper[0], &arrayIndexHelper[1], "tmp_elem");
	}else{
		arrayIndexHelper[0] = ConstantInt::get(getGlobalContext(), APInt(64, 0, true));
		index = ConvertFirstToSecond(index, GetStackType(second->typeInfo), typeInt);
		arrayIndexHelper[1] = index;
		V = Builder.CreateGEP(address, &arrayIndexHelper[0], &arrayIndexHelper[2], "tmp_arr");
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
	arrayIndexHelper[0] = ConstantInt::get(getGlobalContext(), APInt(64, 0, true));
	arrayIndexHelper[1] = ConstantInt::get(getGlobalContext(), APInt(32, index, true));
	V = Builder.CreateGEP(address, &arrayIndexHelper[0], &arrayIndexHelper[2], "tmp_member");
}

void NodeBreakOp::CompileLLVM()
{
	CompileLLVMExtra();

	Builder.CreateBr(breakStack[breakStack.size() - breakDepth]);
	BasicBlock *afterBreak = BasicBlock::Create(getGlobalContext(), "afterBreak", F);
	Builder.SetInsertPoint(afterBreak);
	V = NULL;
}

void NodeContinueOp::CompileLLVM()
{
	CompileLLVMExtra();

	Builder.CreateBr(continueStack[breakStack.size() - continueDepth]);
	BasicBlock *afterContinue = BasicBlock::Create(getGlobalContext(), "afterContinue", F);
	Builder.SetInsertPoint(afterContinue);
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
		Value *ptr = Builder.CreatePointerCast(V, Type::getInt8PtrTy(getGlobalContext()), "ptr_cast");
		DUMP(ptr->getType());
		// Allocate space for auto ref
		Value *aggr = CreateEntryBlockAlloca(F, "tmp_autoref", (Type*)typeObject->llvmType);
		DUMP(aggr->getType());
		V = Builder.CreateStructGEP(aggr, 0, "tmp_arr");
		DUMP(V->getType());
		Builder.CreateStore(ConstantInt::get((Type*)typeInt->llvmType, APInt(32, first->typeInfo->subType->typeIndex)), V);
		V = Builder.CreateStructGEP(aggr, 1, "tmp_arr");
		DUMP(V->getType());
		Builder.CreateStore(ptr, V);
		V = Builder.CreateLoad(aggr, "autoref");
	}else{
		first->CompileLLVM();
		DUMP(V->getType());
		// Allocate space for auto ref
		Value *aggr = CreateEntryBlockAlloca(F, "tmp_autoref", (Type*)typeObject->llvmType);
		DUMP(aggr->getType());
		Builder.CreateStore(V, aggr);
		V = Builder.CreateStructGEP(aggr, 1, "tmp_ptr");
		DUMP(V->getType());
		V = Builder.CreateLoad(V, "ptr");
		DUMP(V->getType());
		DUMP(((Type*)typeInfo->llvmType));
		V = Builder.CreatePointerCast(V, (Type*)typeInfo->llvmType, "ptr_cast");
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
	Value *ptr = Builder.CreateStructGEP(aggr, 1, "ptr_part");
	DUMP(ptr->getType());
	Value *context = Builder.CreateStructGEP(aggr, 0, "ctx_part");
	DUMP(context->getType());

	const Type *functionType = ((llvm::StructType*)typeInfo->llvmType)->getTypeAtIndex(1u);//typeInfo->lastVariable->type;
	DUMP(functionType);

	Value *tmp = TheModule->getFunction(funcInfo->name);
	DUMP(tmp->getType());
	tmp = Builder.CreatePointerCast(tmp, functionType, "func_cast");
	Builder.CreateStore(tmp, ptr);

	const Type *contextType = ((llvm::StructType*)typeInfo->llvmType)->getTypeAtIndex(0u);//TypeInfo *contextType = typeInfo->firstVariable->type;
	DUMP(contextType);

	if(funcInfo->type == FunctionInfo::NORMAL)
	{
		Builder.CreateStore(ConstantPointerNull::get((PointerType*)contextType), context);
	}else{
		first->CompileLLVM();
		DUMP(V->getType());
		V = Builder.CreatePointerCast(V, Type::getInt8PtrTy(getGlobalContext()), "context_cast");
		//V = Builder.CreatePtrToInt(V, Type::getInt32Ty(getGlobalContext()), "int_cast");
		DUMP(V->getType());
		Builder.CreateStore(V, context);
	}

	V = Builder.CreateLoad(aggr, "func_res");
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
	V = Builder.CreateLoad(closure, "closure");
	DUMP(V->getType());
	// Cast it to char*
	V = Builder.CreatePointerCast(V, Type::getInt8PtrTy(getGlobalContext()));
	DUMP(V->getType());
	// Shift to offset
	Value *arrayIndexHelper[2];
	//arrayIndexHelper[0] = ConstantInt::get(getGlobalContext(), APInt(64, 0, true));
	arrayIndexHelper[0] = ConstantInt::get(getGlobalContext(), APInt(32, closureElem, true));
	V = Builder.CreateGEP(V, &arrayIndexHelper[0], &arrayIndexHelper[1]);
	DUMP(V->getType());
	// cast to result type
	V = Builder.CreatePointerCast(V, (Type*)typeInfo->llvmType);
	DUMP(V->getType());

	//cmdList.push_back(VMCmd(cmdPushPtr, ADDRESS_RELATIVE, (unsigned short)typeInfo->size, closurePos));
	//cmdList.push_back(VMCmd(cmdPushPtrStk, 0, (unsigned short)typeInfo->size, closureElem));

	//ThrowError(CodeInfo::lastKnownStartPos, "ERROR: NodeGetUpvalue");
}

void NodeBlock::CompileLLVM()
{
	CompileLLVMExtra();

	/*first->CompileLLVM();
	if(parentFunction->closeUpvals)
		cmdList.push_back(VMCmd(cmdCloseUpvals, (unsigned short)CodeInfo::FindFunctionByPtr(parentFunction), stackFrameShift));
*/
	ThrowError(CodeInfo::lastKnownStartPos, "ERROR: NodeBlock");
}

#else
void	NULLC_PreventLLVMWarning(){}
#endif
