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
	return b / a;
}

long long llvmLongMod(long long a, long long b)
{
	return b % a;
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
	return NULL;
}

llvm::Function *F = NULL;
void		StartLLVMGeneration()
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
	TheExecutionEngine->InstallLazyFunctionCreator(funcCreator);

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
	for(unsigned int i = 0; i < CodeInfo::typeInfo.size(); i++)
	{
		if(CodeInfo::typeInfo[i]->llvmType)
			continue;
		TypeInfo *type = CodeInfo::typeInfo[i];
		//printf("%s\n", type->GetFullTypeName());
		if(type->arrLevel && type->arrSize != TypeInfo::UNSIZED_ARRAY)
		{
			if(!type->subType->llvmType)
				ThrowError(CodeInfo::lastKnownStartPos, "ERROR: StartLLVMGeneration type->arrLevel 1 !type->subType->llvmType");
			type->llvmType = ArrayType::get((Type*)type->subType->llvmType, type->arrSize);
		}else if(type->arrLevel && type->arrSize == TypeInfo::UNSIZED_ARRAY){
			if(!type->subType->llvmType)
				ThrowError(CodeInfo::lastKnownStartPos, "ERROR: StartLLVMGeneration type->arrLevel 2 !type->subType->llvmType");
			type->llvmType = StructType::get(getGlobalContext(), PointerType::getUnqual((Type*)type->subType->llvmType), Type::getInt32Ty(getGlobalContext()), (Type*)NULL);
		}else if(type->refLevel){
			if(!type->subType->llvmType)
				ThrowError(CodeInfo::lastKnownStartPos, "ERROR: StartLLVMGeneration type->refLevel !type->subType->llvmType");
			type->llvmType = type->subType == typeVoid ? Type::getInt8PtrTy(getGlobalContext()) : PointerType::getUnqual((Type*)type->subType->llvmType);
		}else if(type->funcType){
			typeList.clear();
			for(unsigned int k = 0; k < type->funcType->paramCount; k++)
				typeList.push_back((Type*)type->funcType->paramType[k]->llvmType);
			Type *functionType = llvm::FunctionType::get((Type*)type->funcType->retType->llvmType, typeList, false);
			type->llvmType = StructType::get(getGlobalContext(), PointerType::getUnqual(functionType), Type::getInt32Ty(getGlobalContext()), (Type*)NULL);
		}else{
			typeList.clear();
			for(TypeInfo::MemberVariable *curr = type->firstVariable; curr; curr = curr->next)
			{
				if(!curr->type->llvmType)
					ThrowError(CodeInfo::lastKnownStartPos, "ERROR: StartLLVMGeneration !curr->type->llvmType");
				typeList.push_back((Type*)curr->type->llvmType);
			}
			type->llvmType = StructType::get(getGlobalContext(), typeList);
		}
	}

	// Generate global variables
	for(unsigned int i = 0; i < CodeInfo::varInfo.size(); i++)
	{
		InplaceStr name = CodeInfo::varInfo[i]->name;
		globals.push_back(new GlobalVariable(*TheModule, (Type*)CodeInfo::varInfo[i]->varType->llvmType, false, llvm::GlobalVariable::InternalLinkage, 0, std::string(name.begin, name.end)));
		CodeInfo::varInfo[i]->llvmValue = globals.back();
		globals.back()->setInitializer(Constant::getNullValue((Type*)CodeInfo::varInfo[i]->varType->llvmType));
	}

	Function::Create(TypeBuilder<types::i<32>(types::i<32>, types::i<32>), true>::get(getGlobalContext()), Function::ExternalLinkage, "llvmIntPow", TheModule);
	Function::Create(TypeBuilder<types::i<64>(types::i<64>, types::i<64>), true>::get(getGlobalContext()), Function::ExternalLinkage, "llvmLongPow", TheModule);
	Function::Create(TypeBuilder<types::ieee_double(types::ieee_double, types::ieee_double), true>::get(getGlobalContext()), Function::ExternalLinkage, "llvmDoublePow", TheModule);
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

#define DUMP(x) x->dump()
//#define DUMP(x) x

BasicBlock *globalExit = NULL;

void		StartGlobalCode()
{
	std::vector<const llvm::Type*> Empty;
	llvm::FunctionType *FT = llvm::FunctionType::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), Empty, false);
	F = llvm::Function::Create(FT, llvm::Function::ExternalLinkage, "Global", TheModule);

	llvm::BasicBlock *BB = llvm::BasicBlock::Create(llvm::getGlobalContext(), "global_entry", F);
	Builder.SetInsertPoint(BB);

	globalExit = BasicBlock::Create(getGlobalContext(), "globalReturn");
}

const char*	GetLLVMIR()
{
	if(F->back().empty() || !F->back().back().isTerminator())
		Builder.CreateRet(ConstantInt::get(getGlobalContext(), APInt(32, 0, true)));

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

	//LLVMContext &Context = getGlobalContext();
	//CreateFibFunction(TheModule, Context);
	//outs() << "verifying... ";
	DUMP(TheModule);
	//my_stream a;
	//TheModule->print(a, NULL);

	unsigned start = clock();

	verifyFunction(*F);
	verifyModule(*TheModule);

	unsigned verified = clock() - start;
	start = clock();

/*	std::vector<unsigned char> out;
	BitstreamWriter bw = BitstreamWriter(out);
	WriteBitcodeToStream(TheModule, bw);
	printf("%d\n", out.size());*/

	if(!F->getEntryBlock().empty())
	{
		void *FPtr = TheExecutionEngine->getPointerToFunction(F);
		unsigned compiled = clock() - start;
		start = clock();
		int (*FP)() = (int (*)())(intptr_t)FPtr;
		printf("Evaluated to %d (verification %d; compilation %d; execution %d)\n", FP(), verified, compiled, clock() - start);
	}

	return NULL;
}
llvm::Value *V;



void	EndLLVMGeneration()
{
	//for(unsigned int i = 0; i < globals.size(); i++)
	//	delete globals[i];
	//globals.clear();
	delete OurFPM;
	OurFPM = NULL;
	delete OurPM;
	OurPM = NULL;
	V = NULL;
	F = NULL;
	delete TheModule;
	TheModule = NULL;
	//for(unsigned int i = 0; i < globals.size(); i++)
	//	delete globals[i];
	//globals.clear();
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
		return Builder.CreateSIToFP(V, (Type*)secondType->llvmType, "tmp_itod");
	if(first == STYPE_LONG && second == STYPE_DOUBLE)
		return Builder.CreateSIToFP(V, (Type*)secondType->llvmType, "tmp_ltod");
	if(first == STYPE_INT && second == STYPE_LONG)
		return Builder.CreateIntCast(V, (Type*)secondType->llvmType, true, "tmp_itol");

	if(fDT != sDT)
	{
		if(fDT == DTYPE_FLOAT)
			return Builder.CreateFPCast(V, (Type*)typeDouble->llvmType, "tmp_ftod");
		if(fDT == DTYPE_CHAR || fDT == DTYPE_SHORT)
			return Builder.CreateIntCast(V, (Type*)typeInt->llvmType, true, "tmp_sx");
	}
	return V;
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

	if(typeInfo && first->typeInfo != typeInfo)
		ThrowError(CodeInfo::lastKnownStartPos, "ERROR: NodeReturnOp typeInfo && first->typeInfo != typeInfo");
	first->CompileLLVM();
	if(typeInfo != typeVoid && !V)
		ThrowError(CodeInfo::lastKnownStartPos, "ERROR: NodeReturnOp typeInfo != typeVoid && !V");
	if(!parentFunction && first->typeInfo != typeInt)
		ThrowError(CodeInfo::lastKnownStartPos, "ERROR: NodeReturnOp !typeInfo && first->typeInfo != typeInt");
	Builder.CreateRet(V);

	//Builder.CreateRetVoid();
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
	if(typeInfo->arrLevel && typeInfo->arrSize == TypeInfo::UNSIZED_ARRAY)
	{
		aggr = CreateEntryBlockAlloca(F, "arr_tmp", (Type*)typeInfo->llvmType);
	}else if(typeInfo != typeVoid)
		ThrowError(CodeInfo::lastKnownStartPos, "ERROR: NodeExpressionList typeInfo != typeVoid");
	int index = 1;
	NodeZeroOP	*curr = first;
	do 
	{
		curr->CompileLLVM();
		curr = curr->next;

		if(typeInfo->arrLevel && typeInfo->arrSize == TypeInfo::UNSIZED_ARRAY)
		{
			DUMP(V->getType());
			if(index == 0)
			{
				assert(typeInfo->subType->refType);
				V = Builder.CreatePointerCast(V, (Type*)typeInfo->subType->refType->llvmType, "ptr_any");
				DUMP(V->getType());
			}
			Value *arrayIndexHelper[2];
			DUMP(aggr->getType());
			arrayIndexHelper[0] = ConstantInt::get(getGlobalContext(), APInt(64, 0, true));
			arrayIndexHelper[1] = ConstantInt::get(getGlobalContext(), APInt(32, index--, true));
			Value *target = Builder.CreateGEP(aggr, &arrayIndexHelper[0], &arrayIndexHelper[2], "arr_part");
			DUMP(target->getType());
			Builder.CreateStore(V, target);
			
		}
	}while(curr);
	if(typeInfo->arrLevel && typeInfo->arrSize == TypeInfo::UNSIZED_ARRAY)
		V = Builder.CreateLoad(aggr, "arr_aggr");
}

void NodeFuncDef::CompileLLVM()
{
	if(disabled)
	{
		CompileLLVMExtra();
		return;
	}

	Function *CalleeF = TheModule->getFunction(funcInfo->name);
	if(CalleeF)
		ThrowError(CodeInfo::lastKnownStartPos, "ERROR: NodeFuncDef CalleeF");

	std::vector<const llvm::Type*> Arguments;
	VariableInfo *curr = NULL;
	for(curr = funcInfo->firstParam; curr; curr = curr->next)
		Arguments.push_back((Type*)curr->varType->llvmType);
	Arguments.push_back((Type*)funcInfo->extraParam->varType->llvmType);
	llvm::FunctionType *FT = llvm::FunctionType::get((Type*)funcInfo->retType->llvmType, Arguments, false);
	F = llvm::Function::Create(FT, llvm::Function::ExternalLinkage, funcInfo->name, TheModule);

	llvm::BasicBlock *BB = llvm::BasicBlock::Create(llvm::getGlobalContext(), "entry", F);
	Builder.SetInsertPoint(BB);

	Function::arg_iterator AI;
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

	//F->dump();
	verifyFunction(*F, llvm::PrintMessageAction);
	if(enableOptimization && !F->getEntryBlock().empty())
		OurFPM->run(*F);
	F = NULL;
}

void NodeFuncCall::CompileLLVM()
{
	CompileLLVMExtra();

	if(!funcInfo)
		ThrowError(CodeInfo::lastKnownStartPos, "ERROR: NodeFuncCall funcInfo = NULL");
	std::vector<llvm::Value*> args;
	printf("Function call\n");
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
			printf("%s\t", curr->typeInfo->GetFullTypeName());
			DUMP(V->getType());
			V = ConvertFirstToSecond(V, curr->typeInfo, *paramType);
			DUMP(V->getType());
			args.push_back(V);
			curr = curr->prev;
			paramType++;
		}while(curr);
	}
	if(first)
	{
		first->CompileLLVM();
		DUMP(V->getType());
		args.push_back(V);
	}else{
		args.push_back(ConstantPointerNull::get(Type::getInt32PtrTy(getGlobalContext())));
	}
	Function *CalleeF = TheModule->getFunction(funcInfo->name);
	if(!CalleeF)
		ThrowError(CodeInfo::lastKnownStartPos, "ERROR: !CalleeF");
	if(funcType->retType != typeVoid)
	{
		V = Builder.CreateCall(CalleeF, args.begin(), args.end(), funcInfo->name);
	}else{
		Builder.CreateCall(CalleeF, args.begin(), args.end());
		V = NULL;
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
		V = Builder.CreateICmpNE(V, ConstantInt::get(getGlobalContext(), APInt(id == cmdLogNotL ? 64 : 32, 0, true)), "to_bool");
		// Not 
		V = Builder.CreateNot(V, "lognot_tmp");
		V = Builder.CreateIntCast(V, Type::getInt32Ty(getGlobalContext()), true, "booltmp");
		break;
	}
}

void NodeGetAddress::CompileLLVM()
{
	CompileLLVMExtra();

	if(!varInfo)
		ThrowError(CodeInfo::lastKnownStartPos, "ERROR: NodeGetAddress varInfo = NULL");
	///*GetElementPtrInst *inst = */V = GetElementPtrInst::Create(NamedValues[varInfo->nameHash], NULL, "tmp_ptr");
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
	first->CompileLLVM();
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
			V = Builder.CreateFCmpOLT(left, right, "less_tmp");
		else
			V = Builder.CreateICmpSLT(left, right, "less_tmp");
		V = Builder.CreateIntCast(V, Type::getInt32Ty(getGlobalContext()), true, "to_int");
		break;	// a < b
	case cmdGreater:
		if(fST == STYPE_DOUBLE)
			V = Builder.CreateFCmpOGT(left, right, "less_tmp");
		else
			V = Builder.CreateICmpSGT(left, right, "less_tmp");
		V = Builder.CreateIntCast(V, Type::getInt32Ty(getGlobalContext()), true, "to_int");
		break;	// a > b
	case cmdLEqual:
		if(fST == STYPE_DOUBLE)
			V = Builder.CreateFCmpOLE(left, right, "less_tmp");
		else
			V = Builder.CreateICmpSLE(left, right, "less_tmp");
		V = Builder.CreateIntCast(V, Type::getInt32Ty(getGlobalContext()), true, "to_int");
		break;	// a <= b
	case cmdGEqual:
		if(fST == STYPE_DOUBLE)
			V = Builder.CreateFCmpOGE(left, right, "less_tmp");
		else
			V = Builder.CreateICmpSGE(left, right, "less_tmp");
		V = Builder.CreateIntCast(V, Type::getInt32Ty(getGlobalContext()), true, "to_int");
		break;	// a >= b
	case cmdEqual:
		if(fST == STYPE_DOUBLE)
			V = Builder.CreateFCmpOEQ(left, right, "less_tmp");
		else
			V = Builder.CreateICmpEQ(left, right, "less_tmp");
		V = Builder.CreateIntCast(V, Type::getInt32Ty(getGlobalContext()), true, "to_int");
		break;	// a == b
	case cmdNEqual:
		if(fST == STYPE_DOUBLE)
			V = Builder.CreateFCmpONE(left, right, "less_tmp");
		else
			V = Builder.CreateICmpNE(left, right, "less_tmp");
		V = Builder.CreateIntCast(V, Type::getInt32Ty(getGlobalContext()), true, "to_int");
		break;	// a != b
	case cmdShl:
		V = Builder.CreateShl(left, right, "less_tmp");
		break;		// a << b
	case cmdShr:
		V = Builder.CreateAShr(left, right, "less_tmp");
		break;		// a >> b
	case cmdBitAnd:
		V = Builder.CreateBinOp(llvm::Instruction::And, left, right, "less_tmp");
		break;	// a & b	binary AND/бинарное И
	case cmdBitOr:
		V = Builder.CreateBinOp(llvm::Instruction::Or, left, right, "less_tmp");
		break;	// a | b	binary OR/бинарное ИЛИ
	case cmdBitXor:
		V = Builder.CreateBinOp(llvm::Instruction::Xor, left, right, "less_tmp");
		break;	// a ^ b	binary XOR/бинарное Исключающее ИЛИ
	case cmdLogAnd:
		ThrowError(CodeInfo::lastKnownStartPos, "ERROR: MakeBinaryOp cmdLogAnd");
		break;	// a && b	logical AND/логическое И
	case cmdLogOr:
		ThrowError(CodeInfo::lastKnownStartPos, "ERROR: MakeBinaryOp cmdLogOr");
		break;	// a || b	logical OR/логическое ИЛИ
	case cmdLogXor:
		left = Builder.CreateICmpNE(left, ConstantInt::get(getGlobalContext(), APInt(fST == STYPE_LONG ? 64 : 32, 0, true)), "to_bool_left");
		right = Builder.CreateICmpNE(right, ConstantInt::get(getGlobalContext(), APInt(fST == STYPE_LONG ? 64 : 32, 0, true)), "to_bool_rught");
		V = Builder.CreateBinOp(llvm::Instruction::Xor, left, right, "less_tmp");
		V = Builder.CreateIntCast(V, Type::getInt32Ty(getGlobalContext()), true, "to_int");
		break;	// a ^^ b	logical XOR/логическое Исключающее ИЛИ
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
	left = ConvertFirstForSecond(left, first->typeInfo, second->typeInfo);
	right = ConvertFirstForSecond(right, second->typeInfo, first->typeInfo);
	DUMP(left->getType());
	DUMP(right->getType());
	MakeBinaryOp(left, right, cmdID, fST);
}

void NodeVariableSet::CompileLLVM()
{
	CompileLLVMExtra();

	if(arrSetAll)
		ThrowError(CodeInfo::lastKnownStartPos, "ERROR: NodeVariableSet arrSetAll");
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
	value = ConvertFirstToSecond(value, second->typeInfo, first->typeInfo->subType);
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

void NodeIfElseExpr::CompileLLVM()
{
	CompileLLVMExtra();

	BasicBlock *trueBlock = BasicBlock::Create(getGlobalContext(), "trueBlock", F);
	BasicBlock *falseBlock = third ? BasicBlock::Create(getGlobalContext(), "falseBlock", F) : NULL;
	BasicBlock *exitBlock = BasicBlock::Create(getGlobalContext(), "exitBlock", F);
	
	// Compute condition
	first->CompileLLVM();
	if(first->typeInfo != typeInt)
		ThrowError(CodeInfo::lastKnownStartPos, "ERROR: NodeIfElseExpr first->typeInfo != typeInt");
	Value *cond = Builder.CreateICmpNE(V, ConstantInt::get(getGlobalContext(), APInt(32, 0, true)), "cond_value");

	Builder.CreateCondBr(cond, trueBlock, third ? falseBlock : exitBlock);

	Builder.SetInsertPoint(trueBlock);

	second->CompileLLVM();
	if(second->nodeType != typeNodeReturnOp) // $$$ what if it's not the last?
		Builder.CreateBr(exitBlock);

	if(third)
	{
		Builder.SetInsertPoint(falseBlock);
		third->CompileLLVM();
		if(third->nodeType != typeNodeReturnOp)	// $$$ what if it's not the last?
			Builder.CreateBr(exitBlock);
	}
	Builder.SetInsertPoint(exitBlock);
	//Builder.CreateRet(V);
	V = NULL;
}

void NodeForExpr::CompileLLVM()
{
	CompileLLVMExtra();

	first->CompileLLVM();

	BasicBlock *condBlock = BasicBlock::Create(getGlobalContext(), "condBlock", F);
	BasicBlock *bodyBlock = BasicBlock::Create(getGlobalContext(), "bodyBlock", F);
	BasicBlock *incrementBlock = BasicBlock::Create(getGlobalContext(), "incrementBlock", F);
	BasicBlock *exitBlock = BasicBlock::Create(getGlobalContext(), "exitBlock", F);

	Builder.CreateBr(condBlock);
	Builder.SetInsertPoint(condBlock);

	second->CompileLLVM();
	if(second->typeInfo != typeInt)
		ThrowError(CodeInfo::lastKnownStartPos, "ERROR: NodeForExpr second->typeInfo != typeInt");
	Value *cond = Builder.CreateICmpNE(V, ConstantInt::get(getGlobalContext(), APInt(32, 0, true)), "cond_value");
	Builder.CreateCondBr(cond, bodyBlock, exitBlock);

	Builder.SetInsertPoint(bodyBlock);
	fourth->CompileLLVM();
	Builder.CreateBr(incrementBlock);

	Builder.SetInsertPoint(incrementBlock);
	third->CompileLLVM();
	Builder.CreateBr(condBlock);

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
	if(first->typeInfo != typeInt)
		ThrowError(CodeInfo::lastKnownStartPos, "ERROR: NodeWhileExpr first->typeInfo != typeInt");
	Value *cond = Builder.CreateICmpNE(V, ConstantInt::get(getGlobalContext(), APInt(32, 0, true)), "cond_value");
	Builder.CreateCondBr(cond, bodyBlock, exitBlock);

	Builder.SetInsertPoint(bodyBlock);
	second->CompileLLVM();
	Builder.CreateBr(condBlock);

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

	first->CompileLLVM();

	Builder.CreateBr(condBlock);
	Builder.SetInsertPoint(condBlock);

	second->CompileLLVM();
	if(second->typeInfo != typeInt)
		ThrowError(CodeInfo::lastKnownStartPos, "ERROR: NodeDoWhileExpr second->typeInfo != typeInt");
	Value *cond = Builder.CreateICmpNE(V, ConstantInt::get(getGlobalContext(), APInt(32, 0, true)), "cond_value");
	Builder.CreateCondBr(cond, bodyBlock, exitBlock);

	Builder.SetInsertPoint(exitBlock);
	V = NULL;
}

void NodePreOrPostOp::CompileLLVM()
{
	CompileLLVMExtra();

	first->CompileLLVM();
	Value *address = V;

	if(prefixOp)
	{
		// ++x
		V = Builder.CreateLoad(address, "incdecpre_tmp");
		Value *mod = Builder.CreateAdd(V, ConvertFirstToSecond(ConstantInt::get(getGlobalContext(), APInt(32, incOp ? 1 : -1, true)), typeInt, typeInfo), "incdecpost_res");
		Builder.CreateStore(mod, address);
		V = mod;
	}else{
		// x++
		V = Builder.CreateLoad(address, "incdecpre_tmp");
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
	
	left = ConvertFirstForSecond(left, first->typeInfo->subType, second->typeInfo);
	right = ConvertFirstForSecond(right, second->typeInfo, first->typeInfo->subType);
	MakeBinaryOp(left, right, cmdID, resType->stackType);

	V = ConvertFirstToSecond(V, resType, first->typeInfo->subType);
	V = Builder.CreateStore(V, address);
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

	ThrowError(CodeInfo::lastKnownStartPos, "ERROR: NodeBreakOp");
}

void NodeContinueOp::CompileLLVM()
{
	CompileLLVMExtra();

	ThrowError(CodeInfo::lastKnownStartPos, "ERROR: NodeContinueOp");
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
		ThrowError(CodeInfo::lastKnownStartPos, "ERROR: NodeConvertPtr auto ref -> type ref unsupported");
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

	ThrowError(CodeInfo::lastKnownStartPos, "ERROR: NodeFunctionAddress");
}

void NodeGetUpvalue::CompileLLVM()
{
	CompileLLVMExtra();

	ThrowError(CodeInfo::lastKnownStartPos, "ERROR: NodeGetUpvalue");
}

void NodeBlock::CompileLLVM()
{
	CompileLLVMExtra();

	ThrowError(CodeInfo::lastKnownStartPos, "ERROR: NodeBlock");
}

#endif
