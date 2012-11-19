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

#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/JIT.h"

#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/TypeBuilder.h"
#include "llvm/Support/ManagedStatic.h"

#include <cstdio>
#include <string>
#include <map>
#include <vector>
#include <iostream>

#pragma comment(lib, "LLVMCore.lib")
#pragma comment(lib, "LLVMSupport.lib")

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

	llvm::ExecutionEngine *TheExecutionEngine = NULL;

	std::vector<llvm::GlobalVariable*>	globals;

	FastVector<llvm::BasicBlock*>	breakStack;
	FastVector<llvm::BasicBlock*>	continueStack;

	llvm::Function *F = NULL;

	llvm::Type		*typeClosure = NULL;
	llvm::Function	*functionCloseUpvalues = NULL;
	llvm::Function	*functionIndexToFunction = NULL;
}

void		StartLLVMGeneration(unsigned functionsInModules)
{
	llvm::InitializeNativeTarget();

	delete module;
	module = 0;

	delete builder;
	builder = 0;

	delete ctx;

	ctx = new ContextHolder();

	builder = new llvm::IRBuilder<>(getContext());

	module = new llvm::Module("NULLCCode", getContext());

	std::string ErrStr;
	TheExecutionEngine = llvm::EngineBuilder(module).setErrorStr(&ErrStr).create();
	if(!TheExecutionEngine)
	{
		printf("Could not create ExecutionEngine: %s\n", ErrStr.c_str());
		exit(1);
	}

	for(unsigned int i = 0; i < CodeInfo::typeInfo.size(); i++)
		CodeInfo::typeInfo[i]->llvmType = NULL;

	// Generate types
	typeVoid->llvmType = llvm::Type::getVoidTy(getContext());
	typeBool->llvmType = llvm::Type::getInt1Ty(getContext());
	typeChar->llvmType = llvm::Type::getInt8Ty(getContext());
	typeShort->llvmType = llvm::Type::getInt16Ty(getContext());
	typeInt->llvmType = llvm::Type::getInt32Ty(getContext());
	typeLong->llvmType = llvm::Type::getInt64Ty(getContext());
	typeFloat->llvmType = llvm::Type::getFloatTy(getContext());
	typeDouble->llvmType = llvm::Type::getDoubleTy(getContext());
	typeObject->llvmType = llvm::StructType::get(typeInt->llvmType, llvm::Type::getInt8PtrTy(getContext()), (llvm::Type*)NULL);
	typeTypeid->llvmType = typeInt->llvmType;
	typeAutoArray->llvmType = llvm::StructType::get(typeInt->llvmType, llvm::Type::getInt8PtrTy(getContext()), typeInt->llvmType, (llvm::Type*)NULL);

	typeFunction->llvmType = llvm::Type::getInt32Ty(getContext());

	std::vector<llvm::Type*> typeList;
	// Construct abstract types for all classes
	for(unsigned int i = 0; i < CodeInfo::typeInfo.size(); i++)
	{
		TypeInfo *type = CodeInfo::typeInfo[i];
		if(type->llvmType || type->arrLevel || type->refLevel || type->funcType)
			continue;
		type->llvmType = llvm::StructType::create(getContext(), type->GetFullTypeName());
	}
	for(unsigned int i = 0; i < CodeInfo::typeInfo.size(); i++)
	{
		TypeInfo *type = CodeInfo::typeInfo[i];
		if(type->llvmType)
			continue;

		llvm::Type *subType = type->subType ? type->subType->llvmType : NULL;
		if(type->arrLevel && type->arrSize != TypeInfo::UNSIZED_ARRAY)
		{
			assert(subType);
			unsigned size = type->subType->size * type->arrSize;
			size = (size + 3) & ~3;
			type->llvmType = llvm::ArrayType::get(subType, type->subType->size ? size / type->subType->size : type->arrSize);
		}else if(type->arrLevel && type->arrSize == TypeInfo::UNSIZED_ARRAY){
			assert(subType);
			type->llvmType = llvm::StructType::get(llvm::PointerType::getUnqual(subType), llvm::Type::getInt32Ty(getContext()), (llvm::Type*)NULL);
		}else if(type->refLevel){
			assert(subType);
			type->llvmType = type->subType == typeVoid ? llvm::Type::getInt8PtrTy(getContext()) : llvm::PointerType::getUnqual(subType);
		}else if(type->funcType){
			typeList.clear();
			for(unsigned int k = 0; k < type->funcType->paramCount; k++)
			{
				TypeInfo *argTypeInfo = type->funcType->paramType[k];
				llvm::Type *argType = argTypeInfo->llvmType;
				assert(argType);
				typeList.push_back(argType);
			}
			typeList.push_back(llvm::Type::getInt8PtrTy(getContext()));
			llvm::Type *retType = type->funcType->retType->llvmType;
			assert(retType);
			type->funcType->llvmType = llvm::FunctionType::get(retType, llvm::ArrayRef<llvm::Type*>(typeList), false);

			type->llvmType = llvm::StructType::create(getContext(), type->GetFullTypeName());
			typeList.clear();
			typeList.push_back(llvm::Type::getInt8PtrTy(getContext()));
			typeList.push_back(llvm::Type::getInt32Ty(getContext()));
			((llvm::StructType*)type->llvmType)->setBody(llvm::ArrayRef<llvm::Type*>(typeList));
		}
	}
	for(unsigned int i = 0; i < CodeInfo::typeInfo.size(); i++)
	{
		TypeInfo *type = CodeInfo::typeInfo[i];

		if(type->arrLevel || type->refLevel || type->funcType || (type->type != TypeInfo::TYPE_COMPLEX && type->firstVariable == NULL) || type == typeObject || type == typeTypeid || type == typeAutoArray)
			continue;

		if(type->type != TypeInfo::TYPE_COMPLEX && type->firstVariable != NULL)
		{
			type->llvmType = llvm::Type::getInt32Ty(getContext());
			continue;
		}

		typeList.clear();
		for(TypeInfo::MemberVariable *curr = type->firstVariable; curr; curr = curr->next)
			typeList.push_back(curr->type->llvmType);

		((llvm::StructType*)type->llvmType)->setBody(llvm::ArrayRef<llvm::Type*>(typeList));
	}

	// Generate global variables
	for(unsigned int i = 0; i < CodeInfo::varInfo.size(); i++)
	{
		InplaceStr name = CodeInfo::varInfo[i]->name;

		llvm::GlobalVariable::LinkageTypes linkage = llvm::GlobalVariable::ExternalLinkage;
		if(CodeInfo::varInfo[i]->pos >> 24)
			linkage = llvm::GlobalVariable::ExternalWeakLinkage;
		else if(strstr(name.begin, "$temp") == name.begin || CodeInfo::varInfo[i]->blockDepth > 1)
			linkage = llvm::GlobalVariable::InternalLinkage;

		globals.push_back(new llvm::GlobalVariable(*module, CodeInfo::varInfo[i]->varType->llvmType, false, linkage, 0, std::string(name.begin, name.end)));
		CodeInfo::varInfo[i]->llvmValue = globals.back();
		globals.back()->setInitializer(llvm::Constant::getNullValue(CodeInfo::varInfo[i]->varType->llvmType));
	}

	typeClosure = llvm::StructType::create(getContext(), "#upvalue");
	typeList.clear();
	typeList.push_back(llvm::Type::getInt8PtrTy(getContext()));
	typeList.push_back(llvm::PointerType::getUnqual(typeClosure));
	typeList.push_back(llvm::Type::getInt32Ty(getContext()));
	llvm::cast<llvm::StructType>(typeClosure)->setBody(llvm::ArrayRef<llvm::Type*>(typeList));

	typeList.clear();
	typeList.push_back(llvm::PointerType::getUnqual(llvm::PointerType::getUnqual(typeClosure)));
	typeList.push_back(llvm::Type::getInt8PtrTy(getContext()));
	functionCloseUpvalues = llvm::Function::Create(llvm::FunctionType::get(llvm::Type::getVoidTy(getContext()), llvm::ArrayRef<llvm::Type*>(typeList), false), llvm::Function::ExternalLinkage, "llvmCloseUpvalue", module);

	llvm::Function::Create(llvm::TypeBuilder<llvm::types::i<32>(llvm::types::i<32>, llvm::types::i<32>), true>::get(getContext()), llvm::Function::ExternalLinkage, "llvmIntPow", module);
	llvm::Function::Create(llvm::TypeBuilder<llvm::types::i<64>(llvm::types::i<64>, llvm::types::i<64>), true>::get(getContext()), llvm::Function::ExternalLinkage, "llvmLongPow", module);
	llvm::Function::Create(llvm::TypeBuilder<llvm::types::ieee_double(llvm::types::ieee_double, llvm::types::ieee_double), true>::get(getContext()), llvm::Function::ExternalLinkage, "llvmDoublePow", module);

	std::vector<llvm::Type*> arrSetParams;
	arrSetParams.push_back(llvm::Type::getInt8PtrTy(getContext()));
	arrSetParams.push_back(llvm::Type::getInt8Ty(getContext()));
	arrSetParams.push_back(llvm::Type::getInt32Ty(getContext()));
	llvm::Function::Create(llvm::FunctionType::get(llvm::Type::getVoidTy(getContext()), arrSetParams, false), llvm::Function::ExternalLinkage, "__llvmSetArrayC", module);

	arrSetParams[0] = llvm::Type::getInt16PtrTy(getContext());
	arrSetParams[1] = llvm::Type::getInt16Ty(getContext());
	llvm::Function::Create(llvm::FunctionType::get(llvm::Type::getVoidTy(getContext()), arrSetParams, false), llvm::Function::ExternalLinkage, "__llvmSetArrayS", module);

	arrSetParams[0] = llvm::Type::getInt32PtrTy(getContext());
	arrSetParams[1] = llvm::Type::getInt32Ty(getContext());
	llvm::Function::Create(llvm::FunctionType::get(llvm::Type::getVoidTy(getContext()), arrSetParams, false), llvm::Function::ExternalLinkage, "__llvmSetArrayI", module);

	arrSetParams[0] = llvm::Type::getInt64PtrTy(getContext());
	arrSetParams[1] = llvm::Type::getInt64Ty(getContext());
	llvm::Function::Create(llvm::FunctionType::get(llvm::Type::getVoidTy(getContext()), arrSetParams, false), llvm::Function::ExternalLinkage, "__llvmSetArrayL", module);

	arrSetParams[0] = llvm::Type::getFloatPtrTy(getContext());
	arrSetParams[1] = llvm::Type::getFloatTy(getContext());
	llvm::Function::Create(llvm::FunctionType::get(llvm::Type::getVoidTy(getContext()), arrSetParams, false), llvm::Function::ExternalLinkage, "__llvmSetArrayF", module);

	arrSetParams[0] = llvm::Type::getDoublePtrTy(getContext());
	arrSetParams[1] = llvm::Type::getDoubleTy(getContext());
	llvm::Function::Create(llvm::FunctionType::get(llvm::Type::getVoidTy(getContext()), arrSetParams, false), llvm::Function::ExternalLinkage, "__llvmSetArrayD", module);

	llvm::Function::Create(llvm::TypeBuilder<void(llvm::types::i<32>), true>::get(getContext()), llvm::Function::ExternalLinkage, "llvmReturnInt", module);
	llvm::Function::Create(llvm::TypeBuilder<void(llvm::types::i<64>), true>::get(getContext()), llvm::Function::ExternalLinkage, "llvmReturnLong", module);
	llvm::Function::Create(llvm::TypeBuilder<void(llvm::types::ieee_double), true>::get(getContext()), llvm::Function::ExternalLinkage, "llvmReturnDouble", module);

	llvm::Type *pointerToFunction = llvm::PointerType::getUnqual(llvm::FunctionType::get(llvm::Type::getVoidTy(getContext()), false));

	typeList.clear();
	typeList.push_back(llvm::Type::getInt32Ty(getContext()));
	functionIndexToFunction = llvm::Function::Create(llvm::FunctionType::get(pointerToFunction, llvm::ArrayRef<llvm::Type*>(typeList), false), llvm::Function::ExternalLinkage, "__llvmIndexToFunction", module);

	std::vector<llvm::Type*> Arguments;
	for(unsigned int i = 0; i < CodeInfo::funcInfo.size(); i++)
	{
		FunctionInfo *funcInfo = CodeInfo::funcInfo[i];

		if(!funcInfo->retType || funcInfo->funcType->dependsOnGeneric || (funcInfo->parentClass && funcInfo->parentClass->genericInfo))
			continue;

		if((funcInfo->address & 0x80000000) && funcInfo->address != -1)
			continue;

		// There could be duplicate instances of generic functions coming from multiple modules, so skip the function if it's not in it's alloted spot
		if(i != unsigned(funcInfo->indexInArr))
			continue;

		Arguments.clear();
		VariableInfo *curr = NULL;
		for(curr = funcInfo->firstParam; curr; curr = curr->next)
			Arguments.push_back(curr->varType->llvmType);
		Arguments.push_back(funcInfo->extraParam ? funcInfo->extraParam->varType->llvmType : llvm::Type::getInt32PtrTy(getContext()));
		llvm::FunctionType *FT = llvm::FunctionType::get(funcInfo->retType->llvmType, llvm::ArrayRef<llvm::Type*>(Arguments), false);

		llvm::GlobalVariable::LinkageTypes linkage = llvm::GlobalVariable::ExternalLinkage;

		// Function from a different module
		if(i < functionsInModules)
			linkage = llvm::GlobalVariable::ExternalWeakLinkage;

		else if(funcInfo->genericBase || funcInfo->genericInstance || (funcInfo->parentClass && funcInfo->parentClass->genericBase))
			linkage = llvm::GlobalVariable::WeakODRLinkage;

		std::string decoratedName = std::string(funcInfo->name) + "#" + funcInfo->funcType->GetFullTypeName() + "#";

		funcInfo->llvmFunction = llvm::Function::Create(FT, linkage, decoratedName.c_str(), module);

		// Generate closures
		if(i >= functionsInModules)
		{
			char name[NULLC_MAX_VARIABLE_NAME_LENGTH];

			for(VariableInfo *local = funcInfo->firstParam; local; local = local->next)
			{
				if(local->usedAsExternal)
				{
					SafeSprintf(name, NULLC_MAX_VARIABLE_NAME_LENGTH, "#upvalue#%.*s", local->name.length(), local->name.begin);
					globals.push_back(new llvm::GlobalVariable(*module, llvm::PointerType::getUnqual(typeClosure), false, llvm::GlobalVariable::InternalLinkage, 0, name));
					local->llvmUpvalue = globals.back();
					globals.back()->setInitializer(llvm::Constant::getNullValue(llvm::PointerType::getUnqual(typeClosure)));
				}
			}
			if(funcInfo->closeUpvals)
			{
				SafeSprintf(name, NULLC_MAX_VARIABLE_NAME_LENGTH, "#upvalue#%s", funcInfo->name);
				globals.push_back(new llvm::GlobalVariable(*module, llvm::PointerType::getUnqual(typeClosure), false, llvm::GlobalVariable::InternalLinkage, 0, name));
				funcInfo->extraParam->llvmUpvalue = globals.back();
				globals.back()->setInitializer(llvm::Constant::getNullValue(llvm::PointerType::getUnqual(typeClosure)));
			}

			for(VariableInfo *local = funcInfo->firstLocal; local; local = local->next)
			{
				if(local->usedAsExternal)
				{
					SafeSprintf(name, NULLC_MAX_VARIABLE_NAME_LENGTH, "#upvalue#%.*s", local->name.length(), local->name.begin);
					globals.push_back(new llvm::GlobalVariable(*module, llvm::PointerType::getUnqual(typeClosure), false, llvm::GlobalVariable::InternalLinkage, 0, name));
					local->llvmUpvalue = globals.back();
					globals.back()->setInitializer(llvm::Constant::getNullValue(llvm::PointerType::getUnqual(typeClosure)));
				}
			}
		}
	}

	for(unsigned int i = 0; i < CodeInfo::funcInfo.size(); i++)
	{
		FunctionInfo *funcInfo = CodeInfo::funcInfo[i];

		if(funcInfo->retType && (funcInfo->address & 0x80000000) && funcInfo->address != -1)
			funcInfo->llvmFunction = CodeInfo::funcInfo[funcInfo->address & ~0x80000000]->llvmFunction;
	}

	breakStack.clear();
	continueStack.clear();
}

void	StartGlobalCode()
{
	F = llvm::Function::Create(llvm::TypeBuilder<void(), true>::get(getContext()), llvm::Function::ExternalLinkage, "Global", module);

	llvm::BasicBlock *BB = llvm::BasicBlock::Create(getContext(), "global_entry", F);
	builder->SetInsertPoint(BB);
}

std::string out;
const char*	GetLLVMIR(unsigned& length)
{
	builder->CreateRetVoid();

	//module->dump();
	verifyFunction(*F, llvm::PrintMessageAction);
	verifyModule(*module, llvm::PrintMessageAction);

	out.clear();
	llvm::raw_string_ostream os(out);
	WriteBitcodeToFile(module, os);
	os.flush();
	length = (unsigned)out.size();

	return (char*)&out[0];
}
llvm::Value *V;

void	EndLLVMGeneration()
{
	V = NULL;
	F = NULL;

	module = NULL;

	delete TheExecutionEngine;
	TheExecutionEngine = NULL;
}

llvm::Value*	PromoteToStackType(llvm::Value *V, TypeInfo *type)
{
	asmDataType	fDT = type->dataType;
	switch(fDT)
	{
	case DTYPE_CHAR:
		V = builder->CreateIntCast(V, llvm::Type::getInt32Ty(getContext()), type == typeBool ? false : true, "tmp_toint");
		break;
	case DTYPE_SHORT:
		V = builder->CreateIntCast(V, llvm::Type::getInt32Ty(getContext()), true, "tmp_toint");
		break;
	case DTYPE_FLOAT:
		V = builder->CreateFPCast(V, llvm::Type::getDoubleTy(getContext()), "tmp_ftod");
		break;
	}
	return V;
}

llvm::Value*	ConvertFirstToSecond(llvm::Value *V, TypeInfo *firstType, TypeInfo *secondType)
{
	asmDataType	fDT = firstType->dataType, sDT = secondType->dataType;
	asmStackType first = stackTypeForDataType(fDT), second = stackTypeForDataType(sDT);
	if(second == STYPE_DOUBLE)
	{
		if(first == STYPE_INT)
			return builder->CreateSIToFP(V, secondType->llvmType, "tmp_itod");
		else if(first == STYPE_LONG)
			return builder->CreateSIToFP(V, secondType->llvmType, "tmp_ltod");
		else if(firstType != secondType)
			return builder->CreateFPCast(V, secondType->llvmType, "tmp_fpcast");
	}else if(second == STYPE_LONG){
		if(first == STYPE_INT)
			return builder->CreateIntCast(V, secondType->llvmType, true, "tmp_itol");
		else if(first == STYPE_DOUBLE)
			return builder->CreateFPToSI(V, secondType->llvmType, "tmp_dtol");
		else if(firstType != secondType)
			return builder->CreateIntCast(V, secondType->llvmType, firstType == typeBool ? false : true, "tmp_lcast");
	}else if(second == STYPE_INT){
		if(first == STYPE_DOUBLE)
			return builder->CreateFPToSI(V, secondType->llvmType, "tmp_dtoi");
		else if(first == STYPE_LONG)
			return builder->CreateIntCast(V, secondType->llvmType, true, "tmp_ltoi");
		else if(firstType != secondType)
			return builder->CreateIntCast(V, secondType->llvmType, firstType == typeBool ? false : true, "tmp_icast");
	}
	return V;
}

llvm::Value*	ConvertFirstForSecond(llvm::Value *V, TypeInfo *firstType, TypeInfo *secondType)
{
	asmDataType	fDT = firstType->dataType, sDT = secondType->dataType;
	asmStackType first = stackTypeForDataType(fDT), second = stackTypeForDataType(sDT);
	if(first == STYPE_INT && second == STYPE_DOUBLE)
		return builder->CreateSIToFP(V, llvm::Type::getDoubleTy(getContext()), "tmp_itod");
	if(first == STYPE_LONG && second == STYPE_DOUBLE)
		return builder->CreateSIToFP(V, llvm::Type::getDoubleTy(getContext()), "tmp_ltod");
	if(first == STYPE_INT && second == STYPE_LONG)
		return builder->CreateIntCast(V, llvm::Type::getInt64Ty(getContext()), true, "tmp_itol");

	return V;
}

TypeInfo*	GetStackType(TypeInfo* type)
{
	if(type == typeFloat)
		return typeDouble;
	if(type == typeChar || type == typeShort || type == typeBool)
		return typeInt;
	return type;
}

llvm::Value* GetTypeIndex(unsigned localIndex)
{
	char buf[32];
	sprintf(buf, "^type_index_%d", localIndex);

	llvm::GlobalVariable *typeIndexValue = module->getGlobalVariable(buf, true);
	if(!typeIndexValue)
	{
		typeIndexValue = new llvm::GlobalVariable(*module, llvm::Type::getInt32Ty(getContext()), true, llvm::GlobalValue::PrivateLinkage, 0, buf);
		typeIndexValue->setInitializer(llvm::Constant::getNullValue(llvm::Type::getInt32Ty(getContext())));
	}

	return builder->CreateLoad(typeIndexValue, "type_index");
}

llvm::Value* GetFunctionIndex(unsigned localIndex)
{
	char buf[32];
	sprintf(buf, "^func_index_%d", localIndex);

	llvm::GlobalVariable *funcIndexValue = module->getGlobalVariable(buf, true);
	if(!funcIndexValue)
	{
		funcIndexValue = new llvm::GlobalVariable(*module, llvm::Type::getInt32Ty(getContext()), true, llvm::Function::PrivateLinkage, 0, buf);
		funcIndexValue->setInitializer(llvm::Constant::getNullValue(llvm::Type::getInt32Ty(getContext())));
	}

	return builder->CreateLoad(funcIndexValue, "func_index");
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
		if(typeInfo == typeBool)
			V = llvm::ConstantInt::get(getContext(), llvm::APInt(1, num.integer));
		else
			V = llvm::ConstantInt::get(getContext(), llvm::APInt(8, num.integer, true));
		break;
	case DTYPE_SHORT:
		V = llvm::ConstantInt::get(getContext(), llvm::APInt(16, num.integer, true));
		break;
	case DTYPE_INT:
		if(typeInfo->refLevel && num.integer == 0)
			V = llvm::ConstantPointerNull::get((llvm::PointerType*)typeInfo->llvmType);
		else
			V = llvm::ConstantInt::get(getContext(), llvm::APInt(32, num.integer, true));
		break;
	case DTYPE_LONG:
		if(typeInfo->refLevel && num.integer64 == 0)
			V = llvm::ConstantPointerNull::get((llvm::PointerType*)typeInfo->llvmType);
		else
			V = llvm::ConstantInt::get(getContext(), llvm::APInt(64, num.integer64, true));
		break;
	case DTYPE_FLOAT:
		V = llvm::ConstantFP::get(getContext(), llvm::APFloat((float)num.real));
		break;
	case DTYPE_DOUBLE:
		V = llvm::ConstantFP::get(getContext(), llvm::APFloat(num.real));
		break;
	default:
		assert(!"unsupported data type");
	}
}

void NodeReturnOp::CompileLLVM()
{
	CompileLLVMExtra();

	if(first->nodeType == typeNodeZeroOp && first->typeInfo != typeVoid)
		V = llvm::Constant::getNullValue(first->typeInfo->llvmType);
	else
		first->CompileLLVM();
	if(typeInfo != typeVoid && !V)
		ThrowError(CodeInfo::lastKnownStartPos, "ERROR: NodeReturnOp typeInfo != typeVoid && !V");
	if(typeInfo != typeVoid)
	{
		V = PromoteToStackType(V, first->typeInfo);
		V = ConvertFirstToSecond(V, GetStackType(first->typeInfo), typeInfo);
	}

	llvm::BasicBlock *returnBlock = llvm::BasicBlock::Create(getContext(), "return_block", F);
	llvm::BasicBlock *afterReturn = llvm::BasicBlock::Create(getContext(), "after_break");

	builder->CreateBr(returnBlock);
	builder->SetInsertPoint(returnBlock);
	
	if(!parentFunction)
	{
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
		F->getBasicBlockList().push_back(afterReturn);
		builder->SetInsertPoint(afterReturn);
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
					builder->CreateCall2(functionCloseUpvalues, curr->llvmUpvalue, builder->CreatePointerCast(curr->llvmValue, llvm::Type::getInt8PtrTy(getContext())));
			}
			if(parentFunction->firstParam)
				parentFunction->lastParam->next = NULL;
			if(parentFunction->firstLocal)
				parentFunction->lastLocal->next = NULL;
		}

		if(typeInfo == typeBool)
			V = builder->CreateIntCast(V, typeInfo->llvmType, true, "tmp_to_bool");

		// If return is from coroutine, we either need to reset jumpOffset to the beginning of a function, or set it to instruction after return
		if(parentFunction && parentFunction->type == FunctionInfo::COROUTINE)
		{
			assert(parentFunction->extraParam);

			// Get function closure
			llvm::Value *closure = builder->CreateLoad(parentFunction->extraParam->llvmValue, "closure");
			// Cast to int**
			closure = builder->CreatePointerCast(closure, llvm::PointerType::getUnqual(llvm::Type::getInt32PtrTy(getContext())));
			// Get pointer to value
			closure = builder->CreateLoad(closure, "ptr_to_label_id");
			// Store label ID to closure
			builder->CreateStore(llvm::ConstantInt::get(getContext(), llvm::APInt(32, yieldResult ? (parentFunction->yieldCount + 1) : 0, true)), closure);
		}

		builder->CreateRet(V);

		if(yieldResult && parentFunction && parentFunction->type == FunctionInfo::COROUTINE)
		{
			builder->SetInsertPoint(parentFunction->llvmYields[parentFunction->yieldCount + 1]);
			parentFunction->yieldCount++;
		}else{
			F->getBasicBlockList().push_back(afterReturn);
			builder->SetInsertPoint(afterReturn);
		}
	}
}

// CreateEntryBlockAlloca - Create an alloca instruction in the entry block of the function.  This is used for mutable variables etc.
llvm::AllocaInst* CreateEntryBlockAlloca(llvm::Function *TheFunction, const std::string &VarName, llvm::Type* type)
{
	llvm::IRBuilder<> TmpB(&TheFunction->getEntryBlock(), TheFunction->getEntryBlock().begin());
	return TmpB.CreateAlloca(type, 0, VarName.c_str());
}

void NodeExpressionList::CompileLLVM()
{
	llvm::Value *aggr = NULL;
	if(typeInfo->arrLevel && typeInfo->arrSize != TypeInfo::UNSIZED_ARRAY)
	{
		aggr = CreateEntryBlockAlloca(F, "arr_tmp", typeInfo->llvmType);
	}
	if(typeInfo == typeAutoArray)
	{
		aggr = CreateEntryBlockAlloca(F, "tmp", typeInfo->llvmType);
	}

	int index = 0;
	if(typeInfo->arrLevel && typeInfo->arrSize != TypeInfo::UNSIZED_ARRAY)
		index = typeInfo->subType == typeChar ? ((typeInfo->arrSize + 3) / 4) - 1 : typeInfo->arrSize - 1;
	NodeZeroOP	*curr = first;
	do 
	{
		curr->CompileLLVM();
		if(typeInfo != typeVoid && curr->typeInfo == typeInfo)
		{
			llvm::Value *retVal = V;
			while(curr->next)
			{
				curr->next->CompileLLVM();
				assert(curr->next->typeInfo == typeVoid);
				curr = curr->next;
			}
			V = retVal;
			return;
		}
		if(V && (typeInfo->arrLevel && typeInfo->arrSize != TypeInfo::UNSIZED_ARRAY) && curr->nodeType != typeNodeZeroOp)
		{
			V = PromoteToStackType(V, curr->typeInfo);
			llvm::Value *arrayIndexHelper[2];
			arrayIndexHelper[0] = llvm::ConstantInt::get(getContext(), llvm::APInt(64, 0, true));
			arrayIndexHelper[1] = llvm::ConstantInt::get(getContext(), llvm::APInt(32, typeInfo->subType == typeChar ? index * 4 : index, true));
			index--;
			llvm::Value *target = builder->CreateGEP(aggr, llvm::ArrayRef<llvm::Value*>(&arrayIndexHelper[0], 2), "arr_part");
			if(typeInfo->subType == typeChar)
			{
				target = builder->CreatePointerCast(target, llvm::Type::getInt32PtrTy(getContext()), "ptr_any");
			}
			builder->CreateStore(V, target);
		}else if(V && typeInfo == typeAutoArray){
			llvm::Value *arrayStruct = CreateEntryBlockAlloca(F, "array_struct", curr->typeInfo->llvmType);
			builder->CreateStore(V, arrayStruct);

			assert(curr->next);
			curr->next->CompileLLVM();
			llvm::Value *arrayType = V;
			llvm::Value *arrayPtr = builder->CreateStructGEP(arrayStruct, 0, "array_ptr");
			arrayPtr = builder->CreateLoad(arrayPtr);
			arrayPtr = builder->CreatePointerCast(arrayPtr, llvm::Type::getInt8PtrTy(getContext()), "ptr_any");
			llvm::Value *arrayLength = builder->CreateStructGEP(arrayStruct, 1, "array_length");
			arrayLength = builder->CreateLoad(arrayLength);

			llvm::Value *autoType = builder->CreateStructGEP(aggr, 0, "auto_arr_type");
			llvm::Value *autoPtr = builder->CreateStructGEP(aggr, 1, "auto_arr_ptr");
			llvm::Value *autoLength = builder->CreateStructGEP(aggr, 2, "auto_arr_length");

			builder->CreateStore(arrayType, autoType);
			builder->CreateStore(arrayPtr, autoPtr);
			builder->CreateStore(arrayLength, autoLength);
			break;
		}
		curr = curr->next;
	}while(curr);
	if(aggr)
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

	F = funcInfo->llvmFunction;
	funcInfo->llvmImplemented = true;

	llvm::BasicBlock *BB = llvm::BasicBlock::Create(getContext(), "entry", F);
	builder->SetInsertPoint(BB);

	llvm::Function::arg_iterator AI;
	VariableInfo *curr = NULL;
	for(AI = F->arg_begin(), curr = funcInfo->firstParam; AI != F->arg_end() && curr; AI++, curr = curr->next)
	{
		AI->setName(std::string(curr->name.begin, curr->name.end));
		llvm::AllocaInst *Alloca = CreateEntryBlockAlloca(F, std::string(curr->name.begin, curr->name.end), curr->varType->llvmType);
		builder->CreateStore(AI, Alloca);
		curr->llvmValue = Alloca;
	}
	AI->setName(std::string(funcInfo->extraParam->name.begin, funcInfo->extraParam->name.end));
	llvm::AllocaInst *Alloca = CreateEntryBlockAlloca(F, std::string(funcInfo->extraParam->name.begin, funcInfo->extraParam->name.end), funcInfo->extraParam->varType->llvmType);
	builder->CreateStore(AI, Alloca);
	funcInfo->extraParam->llvmValue = Alloca;

	for(curr = funcInfo->firstLocal; curr; curr = curr->next)
		curr->llvmValue = CreateEntryBlockAlloca(F, std::string(curr->name.begin, curr->name.end), curr->varType->llvmType);

	if(funcInfo->type == FunctionInfo::COROUTINE)
	{
		funcInfo->llvmYields = new llvm::BasicBlock*[funcInfo->yieldCount + 1];
		llvm::Constant **yieldValues = new llvm::Constant*[funcInfo->yieldCount + 1];

		for(unsigned i = 0; i < funcInfo->yieldCount + 1; i++)
		{
			// Create block for every entry point
			char name[64];
			sprintf(name, "entrypoint_%d_", i);

			funcInfo->llvmYields[i] = llvm::BasicBlock::Create(getContext(), name, F);
			yieldValues[i] = llvm::BlockAddress::get(F, funcInfo->llvmYields[i]);
		}
		// Create label array
		llvm::ArrayType *arrayType = llvm::ArrayType::get(llvm::Type::getInt8PtrTy(getContext()), funcInfo->yieldCount + 1);
		llvm::GlobalVariable *labelArray = new llvm::GlobalVariable(*module, arrayType, false, llvm::GlobalVariable::InternalLinkage, 0, "yields");
		
		labelArray->setInitializer(llvm::ConstantArray::get(arrayType, llvm::ArrayRef<llvm::Constant*>(yieldValues, funcInfo->yieldCount + 1)));

		// Get function closure
		llvm::Value *closure = builder->CreateLoad(funcInfo->extraParam->llvmValue, "closure");
		// Cast to int**
		closure = builder->CreatePointerCast(closure, llvm::PointerType::getUnqual(llvm::Type::getInt32PtrTy(getContext())));
		// Get pointer to value
		closure = builder->CreateLoad(closure, "ptr_to_label_id");

		// Get label ID
		llvm::Value *arrayIndexHelper[2];
		arrayIndexHelper[0] = llvm::ConstantInt::get(getContext(), llvm::APInt(64, 0, true));
		arrayIndexHelper[1] = builder->CreateLoad(closure, "label_id");
		llvm::Value *address = builder->CreateGEP(labelArray, llvm::ArrayRef<llvm::Value*>(&arrayIndexHelper[0], 2), "pointer_to_label_address");
		// Create indirect branch
		llvm::IndirectBrInst *indirectBranch = builder->CreateIndirectBr(builder->CreateLoad(address, "label_address"), funcInfo->yieldCount + 1);
		// Add possible destinations
		for(unsigned i = 0; i < funcInfo->yieldCount + 1; i++)
			indirectBranch->addDestination(funcInfo->llvmYields[i]);
		// Reset counter
		funcInfo->yieldCount = 0;
		// Set insert block to first destination
		builder->SetInsertPoint(funcInfo->llvmYields[0]);

		delete[] yieldValues;
	}

	first->CompileLLVM();
	if(first->nodeType == typeNodeZeroOp)
	{
		if(funcInfo->retType != typeVoid)
			ThrowError(CodeInfo::lastKnownStartPos, "ERROR: NodeFuncDef function must return a value");
	}

	delete[] funcInfo->llvmYields;
	funcInfo->llvmYields = 0;

	if(funcInfo->retType == typeVoid)
		builder->CreateRetVoid();
	else
		builder->CreateUnreachable();

	verifyFunction(*F, llvm::PrintMessageAction);

	F = NULL;
}

void NodeFuncCall::CompileLLVM()
{
	CompileLLVMExtra();

	std::vector<llvm::Value*> args;

	if(funcType->paramCount)
	{
		NodeZeroOP	*curr = paramTail;
		TypeInfo	**paramType = funcType->paramType;
		do
		{
			curr->CompileLLVM();
			if(!V)
				ThrowError(CodeInfo::lastKnownStartPos, "ERROR: NodeFuncCall V = NULL");

			V = ConvertFirstToSecond(V, curr->typeInfo, *paramType);
			if(*paramType == typeFloat)
				V = builder->CreateFPCast(V, typeFloat->llvmType, "dtof");

			args.push_back(V);
			curr = curr->prev;
			paramType++;
		}while(curr);
	}
	llvm::Function *CalleeF = NULL;
	if(!funcInfo)
	{
		first->CompileLLVM();
		llvm::Value *func = V;

		llvm::Value *funcPtr = CreateEntryBlockAlloca(F, "func_tmp", first->typeInfo->llvmType);

		builder->CreateStore(func, funcPtr);

		llvm::Value *ptr = builder->CreateStructGEP(funcPtr, 1, "ptr_part");
		llvm::Value *functionIndex = builder->CreateLoad(ptr, "func_index");

		llvm::Value *context = builder->CreateStructGEP(funcPtr, 0, "ctx_part");

		args.push_back(builder->CreateLoad(context, "ctx_deref"));

		llvm::Value	*function = builder->CreateCall(functionIndexToFunction, functionIndex);

		CalleeF = (llvm::Function*)builder->CreatePointerCast(function, llvm::PointerType::getUnqual(funcType->llvmType));
	}else{
		CalleeF = funcInfo->llvmFunction;
		if(!CalleeF)
			ThrowError(CodeInfo::lastKnownStartPos, "ERROR: !CalleeF");
		if(first)
		{
			first->CompileLLVM();

			llvm::Argument *last = (--CalleeF->arg_end());

			if(V->getType() != last->getType())
			{
				V = builder->CreatePointerCast(V, last->getType());
			}
			args.push_back(V);
		}else{
			args.push_back(llvm::ConstantPointerNull::get(llvm::Type::getInt32PtrTy(getContext())));
		}
	}
	if(funcType->retType != typeVoid)
	{
		V = builder->CreateCall(CalleeF, llvm::ArrayRef<llvm::Value*>(args));
	}else{
		builder->CreateCall(CalleeF, llvm::ArrayRef<llvm::Value*>(args));
		V = NULL;
	}
	if(funcInfo && funcInfo->nameHash == GetStringHash("__newA"))
	{
		llvm::Value *arrTemp = CreateEntryBlockAlloca(F, "newa_tmp", V->getType());
		builder->CreateStore(V, arrTemp);

		V = builder->CreatePointerCast(arrTemp, llvm::PointerType::getUnqual(typeInfo->llvmType), "newa_rescast");
		V = builder->CreateLoad(V, "newa_load");
	}
	if(funcInfo && funcInfo->nameHash == GetStringHash("__newS"))
	{
		V = builder->CreatePointerCast(V, typeInfo->llvmType, "news_rescast");
	}
	if(funcInfo && (funcInfo->nameHash == GetStringHash("__redirect") || funcInfo->nameHash == GetStringHash("__redirect_ptr")))
	{
		llvm::Value *funcTemp = CreateEntryBlockAlloca(F, "redirect_tmp", V->getType());
		builder->CreateStore(V, funcTemp);

		V = builder->CreatePointerCast(funcTemp, llvm::PointerType::getUnqual(typeInfo->llvmType), "redirect_cast");
		V = builder->CreateLoad(V, "redirect_res");
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
	llvm::Value *zeroV = llvm::ConstantInt::get(getContext(), llvm::APInt(id == cmdLogNotL ? 64 : 32, 0, true));
	if(first->typeInfo == typeObject)
	{
		llvm::Value *aggr = CreateEntryBlockAlloca(F, "ref_tmp", typeObject->llvmType);
		builder->CreateStore(V, aggr);
		V = builder->CreateStructGEP(aggr, 1, "tmp_ptr");
		V = builder->CreateLoad(V, "tmp_ptr");
		V = builder->CreatePtrToInt(V, typeInt->llvmType, "as_int");
		id = cmdLogNot;
	}else if(first->typeInfo->refLevel){
		zeroV = llvm::ConstantPointerNull::get((llvm::PointerType*)V->getType());
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
		V = builder->CreateIntCast(V, llvm::Type::getInt32Ty(getContext()), true, "booltmp");
		break;
	case cmdPushTypeID:
		V = GetTypeIndex(vmCmd.argument);
		break;
	case cmdFuncAddr:
		V = GetFunctionIndex(vmCmd.argument);
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

	V = varInfo->llvmValue;
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

		llvm::Value	*targetClosure = 0;

		if(closure)
		{
			targetClosure = closure->llvmValue;
		}else{
			((NodeOneOP*)first)->GetFirstNode()->CompileLLVM();
			targetClosure = V;
		}

		// Allocate closure
		llvm::Value *typeSize = llvm::ConstantInt::get(getContext(), llvm::APInt(32, first->typeInfo->subType->size, true));
		llvm::Value *value = builder->CreateCall3(module->getFunction("__newS#void ref ref(int,int)#"), typeSize, GetTypeIndex(first->typeInfo->subType->typeIndex), llvm::ConstantPointerNull::get(llvm::Type::getInt32PtrTy(getContext())));
		
		llvm::Value *closureStart = value;

		value = builder->CreatePointerCast(value, llvm::PointerType::getUnqual(typeInfo->llvmType), "news_rescast");

		builder->CreateStore(value, targetClosure);

		unsigned int pos = 0;
		for(FunctionInfo::ExternalInfo *curr = closureFunc->firstExternal; curr; curr = curr->next)
		{
			llvm::Value *gep[2];
			gep[0] = llvm::ConstantInt::get(getContext(), llvm::APInt(32, pos, true));
			llvm::Value *closureCurr = builder->CreateGEP(closureStart, llvm::ArrayRef<llvm::Value*>(gep[0]));

			closureCurr = builder->CreatePointerCast(closureCurr, llvm::PointerType::getUnqual(typeClosure));

			llvm::Value* closurePtr = builder->CreateStructGEP(closureCurr, 0, "cls_ptr");
			
			if(curr->targetPos == ~0u)
			{
				// Immediately close upvalue
				llvm::Value *closure = builder->CreateLoad(targetClosure, "closure"); // Get function closure
				closure = builder->CreatePointerCast(closure, llvm::Type::getInt8PtrTy(getContext())); // Cast to char*
				closure = builder->CreateGEP(closure, llvm::ArrayRef<llvm::Value*>(llvm::ConstantInt::get(getContext(), llvm::APInt(32, pos + NULLC_PTR_SIZE, true)))); // Offset to target upvalue
				builder->CreateStore(closure, closurePtr); // Store in upvalue
			}else{
				if(curr->targetLocal)
				{
					builder->CreateStore(builder->CreatePointerCast(curr->variable->llvmValue, llvm::Type::getInt8PtrTy(getContext())), closurePtr);
				}else{
					assert(closureFunc->parentFunc);
					assert(closureFunc->parentFunc->extraParam);

					// Get function closure
					llvm::Value *closure = builder->CreateLoad(closureFunc->parentFunc->extraParam->llvmValue, "closure");
					// Cast to char*
					closure = builder->CreatePointerCast(closure, llvm::Type::getInt8PtrTy(getContext()));
					// Offset to target upvalue
					closure = builder->CreateGEP(closure, llvm::ArrayRef<llvm::Value*>(llvm::ConstantInt::get(getContext(), llvm::APInt(32, curr->targetPos, true))));
					// Convert char* to type**
					closure = builder->CreatePointerCast(closure, llvm::PointerType::getUnqual(llvm::PointerType::getUnqual(curr->variable->varType->llvmType)));
					// Get pointer to value
					closure = builder->CreateLoad(closure, "closure_deref");
					// Cast to char*
					closure = builder->CreatePointerCast(closure, llvm::Type::getInt8PtrTy(getContext()));
					// Store in upvalue
					builder->CreateStore(closure, closurePtr);
				}

				llvm::Value* closureUpvalue = builder->CreateStructGEP(closureCurr, 1, "cls_upvalue");

				builder->CreateStore(builder->CreateLoad(curr->variable->llvmUpvalue), closureUpvalue);

				llvm::Value* closureSize = builder->CreateStructGEP(closureCurr, 2, "cls_size");

				builder->CreateStore(llvm::ConstantInt::get(getContext(), llvm::APInt(32, curr->variable->varType->size, true)), closureSize);

				builder->CreateStore(closureCurr, curr->variable->llvmUpvalue);
			}

			pos += NULLC_PTR_SIZE + (curr->variable->varType->size > (NULLC_PTR_SIZE + 4) ? curr->variable->varType->size : (NULLC_PTR_SIZE + 4));
		}
	}else{
		first->CompileLLVM();
		if(typeInfo != first->typeInfo->subType)
		{
			V = builder->CreatePointerCast(V, llvm::PointerType::getUnqual(typeInfo->llvmType), "deref_cast");
		}
		if(!V)
			ThrowError(CodeInfo::lastKnownStartPos, "ERROR: NodeDereference V = NULL");
		V = builder->CreateLoad(V, "temp_deref");
	}
}

void MakeBinaryOp(llvm::Value *left, llvm::Value *right, CmdID cmdID, asmStackType fST)
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
		V = builder->CreateZExt(V, llvm::Type::getInt32Ty(getContext()), "to_int");
		break;	// a < b
	case cmdGreater:
		if(fST == STYPE_DOUBLE)
			V = builder->CreateFCmpOGT(left, right);
		else
			V = builder->CreateICmpSGT(left, right);
		V = builder->CreateZExt(V, llvm::Type::getInt32Ty(getContext()), "to_int");
		break;	// a > b
	case cmdLEqual:
		if(fST == STYPE_DOUBLE)
			V = builder->CreateFCmpOLE(left, right);
		else
			V = builder->CreateICmpSLE(left, right);
		V = builder->CreateZExt(V, llvm::Type::getInt32Ty(getContext()), "to_int");
		break;	// a <= b
	case cmdGEqual:
		if(fST == STYPE_DOUBLE)
			V = builder->CreateFCmpOGE(left, right);
		else
			V = builder->CreateICmpSGE(left, right);
		V = builder->CreateZExt(V, llvm::Type::getInt32Ty(getContext()), "to_int");
		break;	// a >= b
	case cmdEqual:
		if(fST == STYPE_DOUBLE)
			V = builder->CreateFCmpOEQ(left, right);
		else
			V = builder->CreateICmpEQ(left, right);
		V = builder->CreateZExt(V, llvm::Type::getInt32Ty(getContext()), "to_int");
		break;	// a == b
	case cmdNEqual:
		if(fST == STYPE_DOUBLE)
			V = builder->CreateFCmpONE(left, right);
		else
			V = builder->CreateICmpNE(left, right);
		V = builder->CreateZExt(V, llvm::Type::getInt32Ty(getContext()), "to_int");
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
		left = builder->CreateICmpNE(left, llvm::ConstantInt::get(getContext(), llvm::APInt(fST == STYPE_LONG ? 64 : 32, 0, true)), "to_bool_left");
		right = builder->CreateICmpNE(right, llvm::ConstantInt::get(getContext(), llvm::APInt(fST == STYPE_LONG ? 64 : 32, 0, true)), "to_bool_rught");
		V = builder->CreateBinOp(llvm::Instruction::Xor, left, right, "less_tmp");
		V = builder->CreateIntCast(V, llvm::Type::getInt32Ty(getContext()), true, "to_int");
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
		llvm::Value *left = V;
		left = PromoteToStackType(left, first->typeInfo);
		if(fST == STYPE_LONG)
			left = builder->CreateIntCast(left, typeInt->llvmType, true, "sc_l");
		left = builder->CreateICmpNE(left, llvm::ConstantInt::get(getContext(), llvm::APInt(32, 0)), "left_cond");

		llvm::BasicBlock *fastExit = llvm::BasicBlock::Create(getContext(), "fastExit", F);
		llvm::BasicBlock *slowExit = llvm::BasicBlock::Create(getContext(), "slowExit", F);
		llvm::BasicBlock *slowOneExit = llvm::BasicBlock::Create(getContext(), "slowOneExit", F);
		llvm::BasicBlock *slowZeroExit = llvm::BasicBlock::Create(getContext(), "slowZeroExit", F);
		llvm::BasicBlock *slowMergeExit = llvm::BasicBlock::Create(getContext(), "slowMergeExit", F);
		llvm::BasicBlock *mergeExit = llvm::BasicBlock::Create(getContext(), "mergeExit", F);

		builder->CreateCondBr(left, cmdID == cmdLogOr ? fastExit : slowExit, cmdID == cmdLogOr ? slowExit : fastExit);

		builder->SetInsertPoint(slowExit);
		second->CompileLLVM();
		llvm::Value *right = V;
		right = PromoteToStackType(right, second->typeInfo);
		if(fST == STYPE_LONG)
			right = builder->CreateIntCast(right, typeInt->llvmType, true, "sc_l");
		right = builder->CreateICmpNE(right, llvm::ConstantInt::get(getContext(), llvm::APInt(32, 0)), "left_cond");
		builder->CreateCondBr(right, slowOneExit, slowZeroExit);

		builder->SetInsertPoint(slowOneExit);
		llvm::Value *rightOne = llvm::ConstantInt::get(getContext(), llvm::APInt(32, 1));
		builder->CreateBr(slowMergeExit);
		slowOneExit = builder->GetInsertBlock();

		builder->SetInsertPoint(slowZeroExit);
		llvm::Value *rightZero = llvm::ConstantInt::get(getContext(), llvm::APInt(32, 0));
		builder->CreateBr(slowMergeExit);
		slowZeroExit = builder->GetInsertBlock();

		builder->SetInsertPoint(slowMergeExit);
		llvm::PHINode *PN = builder->CreatePHI(typeInt->llvmType, 2, "slow_merge_tmp");
		PN->addIncoming(rightOne, slowOneExit);
		PN->addIncoming(rightZero, slowZeroExit);
		builder->CreateBr(mergeExit);
		slowMergeExit = builder->GetInsertBlock();

		llvm::Value *leftUnknown = PN;

		builder->SetInsertPoint(fastExit);
		llvm::Value *leftZero = llvm::ConstantInt::get(getContext(), llvm::APInt(32, cmdID == cmdLogOr ? 1 : 0));
		builder->CreateBr(mergeExit);
		fastExit = builder->GetInsertBlock();

		builder->SetInsertPoint(mergeExit);
		llvm::PHINode *PN2 = builder->CreatePHI(typeInt->llvmType, 2, "merge_tmp");
		PN2->addIncoming(leftUnknown, slowMergeExit);
		PN2->addIncoming(leftZero, fastExit);

		V = PN2;
		return;
	}

	first->CompileLLVM();
	llvm::Value *left = V;
	second->CompileLLVM();
	llvm::Value *right = V;

	left = PromoteToStackType(left, first->typeInfo);
	right = PromoteToStackType(right, second->typeInfo);

	left = ConvertFirstForSecond(left, first->typeInfo, second->typeInfo);
	right = ConvertFirstForSecond(right, second->typeInfo, first->typeInfo);

	MakeBinaryOp(left, right, cmdID, fST);
}

void NodeVariableSet::CompileLLVM()
{
	CompileLLVMExtra();

	first->CompileLLVM();
	llvm::Value *target = V;
	if(!target)
		ThrowError(CodeInfo::lastKnownStartPos, "ERROR: NodeVariableSet target = NULL");
	second->CompileLLVM();
	llvm::Value *value = V;
	if(!value)
		ThrowError(CodeInfo::lastKnownStartPos, "ERROR: NodeVariableSet value = NULL");

	if(arrSetAll)
	{
		char fName[20];
		llvm::Value *arrayIndexHelper[2];
		arrayIndexHelper[0] = llvm::ConstantInt::get(getContext(), llvm::APInt(64, 0, true));
		arrayIndexHelper[1] = llvm::ConstantInt::get(getContext(), llvm::APInt(32, 0));
		target = builder->CreateGEP(target, llvm::ArrayRef<llvm::Value*>(&arrayIndexHelper[0], 2), "arr_begin");
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
		llvm::Function *setFunc = module->getFunction(fName);
		assert(setFunc);

		builder->CreateCall3(setFunc, target, value, llvm::ConstantInt::get(typeInt->llvmType, llvm::APInt(32u, elemCount, true)));
		V = NULL;
		return;
	}

	value = PromoteToStackType(value, second->typeInfo);
	value = ConvertFirstToSecond(value, GetStackType(second->typeInfo), first->typeInfo->subType);
	if(!value)
		ThrowError(CodeInfo::lastKnownStartPos, "ERROR: NodeVariableSet value = NULL after");

	builder->CreateStore(value, target);
	V = value;
}

void NodePopOp::CompileLLVM()
{
	CompileLLVMExtra();

	first->CompileLLVM();
	V = NULL;
}

llvm::Value* GenerateCompareToZero(llvm::Value *V, TypeInfo *typeInfo)
{
	llvm::Value *zeroV = llvm::ConstantInt::get(getContext(), llvm::APInt(32, 0, true));
	if(typeInfo == typeObject)
	{
		llvm::Value *arrTemp = CreateEntryBlockAlloca(F, "autoref_tmp", V->getType());
		builder->CreateStore(V, arrTemp);

		V = builder->CreateStructGEP(arrTemp, 1, "autoref_ptr");
		V = builder->CreateLoad(V);
		V = builder->CreatePtrToInt(V, llvm::Type::getInt32Ty(getContext()), "as_int");
	}else if(typeInfo->refLevel){
		zeroV = llvm::ConstantPointerNull::get((llvm::PointerType*)V->getType());
	}else if(typeInfo != typeInt){
		V = PromoteToStackType(V, typeInfo);
		V = builder->CreateIntCast(V, llvm::Type::getInt32Ty(getContext()), true, "as_int");
	}
	return builder->CreateICmpNE(V, zeroV, "cond_value");
}

void NodeIfElseExpr::CompileLLVM()
{
	CompileLLVMExtra();

	llvm::BasicBlock *trueBlock = llvm::BasicBlock::Create(getContext(), "trueBlock", F);
	llvm::BasicBlock *falseBlock = third ? llvm::BasicBlock::Create(getContext(), "falseBlock") : NULL;
	llvm::BasicBlock *exitBlock = llvm::BasicBlock::Create(getContext(), "exitBlock");
	
	llvm::Value *left = NULL, *right = NULL;

	// Compute condition
	first->CompileLLVM();
	llvm::Value *cond = GenerateCompareToZero(V, first->typeInfo);

	builder->CreateCondBr(cond, trueBlock, third ? falseBlock : exitBlock);

	builder->SetInsertPoint(trueBlock);

	second->CompileLLVM();
	left = V;
	if(typeInfo != typeVoid)
	{
		left = ConvertFirstToSecond(left, second->typeInfo, typeInfo);
		if(typeInfo == typeBool)
			left = builder->CreateIntCast(left, typeBool->llvmType, false, "left_to_bool");
	}
	
	trueBlock = builder->GetInsertBlock();
	if(trueBlock->empty() || !trueBlock->back().isTerminator())
		builder->CreateBr(exitBlock);
	
	if(third)
	{
		F->getBasicBlockList().push_back(falseBlock);
		builder->SetInsertPoint(falseBlock);
		third->CompileLLVM();
		right = V;
		if(typeInfo != typeVoid)
		{
			right = ConvertFirstToSecond(right, third->typeInfo, typeInfo);
			if(typeInfo == typeBool)
				right = builder->CreateIntCast(right, typeBool->llvmType, false, "right_to_bool");
		}

		falseBlock = builder->GetInsertBlock();
		if(falseBlock->empty() || !falseBlock->back().isTerminator())
			builder->CreateBr(exitBlock);
		
	}
	F->getBasicBlockList().push_back(exitBlock);
	builder->SetInsertPoint(exitBlock);

	V = NULL;
	if(typeInfo != typeVoid)
	{
		llvm::PHINode *PN2 = builder->CreatePHI(typeInfo->llvmType, 2, "merge_tmp");

		PN2->addIncoming(left, trueBlock);
		PN2->addIncoming(right, falseBlock);

		V = PN2;
	}
}

void NodeForExpr::CompileLLVM()
{
	CompileLLVMExtra();

	first->CompileLLVM();

	llvm::BasicBlock *condBlock = llvm::BasicBlock::Create(getContext(), "condBlock", F);
	llvm::BasicBlock *bodyBlock = llvm::BasicBlock::Create(getContext(), "bodyBlock");
	llvm::BasicBlock *incrementBlock = llvm::BasicBlock::Create(getContext(), "incrementBlock");
	llvm::BasicBlock *exitBlock = llvm::BasicBlock::Create(getContext(), "exitBlock");

	builder->CreateBr(condBlock);
	builder->SetInsertPoint(condBlock);

	second->CompileLLVM();
	llvm::Value *cond = GenerateCompareToZero(V, second->typeInfo);
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

	llvm::BasicBlock *condBlock = llvm::BasicBlock::Create(getContext(), "condBlock", F);
	llvm::BasicBlock *bodyBlock = llvm::BasicBlock::Create(getContext(), "bodyBlock", F);
	llvm::BasicBlock *exitBlock = llvm::BasicBlock::Create(getContext(), "exitBlock", F);

	builder->CreateBr(condBlock);
	builder->SetInsertPoint(condBlock);

	first->CompileLLVM();
	llvm::Value *cond = GenerateCompareToZero(V, first->typeInfo);
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

	llvm::BasicBlock *bodyBlock = llvm::BasicBlock::Create(getContext(), "bodyBlock", F);
	llvm::BasicBlock *condBlock = llvm::BasicBlock::Create(getContext(), "condBlock", F);
	llvm::BasicBlock *exitBlock = llvm::BasicBlock::Create(getContext(), "exitBlock", F);

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
	llvm::Value *cond = GenerateCompareToZero(V, second->typeInfo);
	builder->CreateCondBr(cond, bodyBlock, exitBlock);

	builder->SetInsertPoint(exitBlock);
	V = NULL;
}

void NodePreOrPostOp::CompileLLVM()
{
	CompileLLVMExtra();

	first->CompileLLVM();
	llvm::Value *address = V;

	if(prefixOp)
	{
		// ++x
		V = builder->CreateLoad(address, "incdecpre_tmp");

		llvm::Value *mod = 0;
		if(typeInfo->stackType == STYPE_DOUBLE)
			mod = builder->CreateFAdd(V, ConvertFirstToSecond(llvm::ConstantInt::get(getContext(), llvm::APInt(32, incOp ? 1 : -1, true)), typeInt, typeInfo), "pre_res");
		else
			mod = builder->CreateAdd(V, ConvertFirstToSecond(llvm::ConstantInt::get(getContext(), llvm::APInt(32, incOp ? 1 : -1, true)), typeInt, typeInfo), "pre_res");
		builder->CreateStore(mod, address);
		V = mod;
	}else{
		// x++
		V = builder->CreateLoad(address, "incdecpre_tmp");

		llvm::Value *mod = 0;
		if(typeInfo->stackType == STYPE_DOUBLE)
			mod = builder->CreateFAdd(V, ConvertFirstToSecond(llvm::ConstantInt::get(getContext(), llvm::APInt(32, incOp ? 1 : -1, true)), typeInt, typeInfo), "post_res");
		else
			mod = builder->CreateAdd(V, ConvertFirstToSecond(llvm::ConstantInt::get(getContext(), llvm::APInt(32, incOp ? 1 : -1, true)), typeInt, typeInfo), "post_res");
		builder->CreateStore(mod, address);
	}
}

void NodeVariableModify::CompileLLVM()
{
	CompileLLVMExtra();

	first->CompileLLVM();
	llvm::Value *address = V;

	second->CompileLLVM();
	llvm::Value *right = V;

	llvm::Value *left = builder->CreateLoad(address, "modify_left");
	
	TypeInfo *resType = ChooseBinaryOpResultType(first->typeInfo->subType, second->typeInfo);
	
	left = PromoteToStackType(left, first->typeInfo->subType);
	right = PromoteToStackType(right, second->typeInfo);

	left = ConvertFirstForSecond(left, first->typeInfo->subType, second->typeInfo);
	right = ConvertFirstForSecond(right, second->typeInfo, first->typeInfo->subType);

	MakeBinaryOp(left, right, cmdID, resType->stackType);

	V = ConvertFirstToSecond(V, GetStackType(resType), typeInfo);

	builder->CreateStore(V, address);
}

void NodeArrayIndex::CompileLLVM()
{
	CompileLLVMExtra();

	first->CompileLLVM();
	llvm::Value *address = V;

	second->CompileLLVM();
	llvm::Value *index = V;

	llvm::Value *arrayIndexHelper[2];
	if(typeParent->arrSize == TypeInfo::UNSIZED_ARRAY)
	{
		llvm::Value *aggr = CreateEntryBlockAlloca(F, "arr_tmp", typeParent->llvmType);
		builder->CreateStore(address, aggr);

		// Access array
		V = builder->CreateStructGEP(aggr, 0, "tmp_arr");
		llvm::Value *arrPtr = builder->CreateLoad(V, "tmp_arrptr");
		arrayIndexHelper[0] = index;
		V = builder->CreateGEP(arrPtr, llvm::ArrayRef<llvm::Value*>(&arrayIndexHelper[0], 1), "tmp_elem");
	}else{
		arrayIndexHelper[0] = llvm::ConstantInt::get(getContext(), llvm::APInt(64, 0, true));
		index = ConvertFirstToSecond(index, GetStackType(second->typeInfo), typeInt);
		arrayIndexHelper[1] = index;
		V = builder->CreateGEP(address, llvm::ArrayRef<llvm::Value*>(&arrayIndexHelper[0], 2), "tmp_arr");
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
	llvm::Value *address = V;
	llvm::Value *arrayIndexHelper[2];
	arrayIndexHelper[0] = llvm::ConstantInt::get(getContext(), llvm::APInt(64, 0, true));
	arrayIndexHelper[1] = llvm::ConstantInt::get(getContext(), llvm::APInt(32, index, true));
	V = builder->CreateGEP(address, llvm::ArrayRef<llvm::Value*>(&arrayIndexHelper[0], 2), "tmp_member");
}

void NodeBreakOp::CompileLLVM()
{
	CompileLLVMExtra();

	builder->CreateBr(breakStack[breakStack.size() - breakDepth]);
	llvm::BasicBlock *afterBreak = llvm::BasicBlock::Create(getContext(), "afterBreak", F);
	builder->SetInsertPoint(afterBreak);
	V = NULL;
}

void NodeContinueOp::CompileLLVM()
{
	CompileLLVMExtra();

	builder->CreateBr(continueStack[breakStack.size() - continueDepth]);
	llvm::BasicBlock *afterContinue = llvm::BasicBlock::Create(getContext(), "afterContinue", F);
	builder->SetInsertPoint(afterContinue);
	V = NULL;
}

void NodeSwitchExpr::CompileLLVM()
{
	CompileLLVMExtra();

	bool intOptimization = true;

	if(!first || first->typeInfo != typeInt)
		intOptimization = false;
	for(NodeZeroOP *currCondition = conditionHead; currCondition; currCondition = currCondition->next)
	{
		if(currCondition->typeInfo != typeInt)
			intOptimization = false;
	}

	if(intOptimization)
	{
		llvm::BasicBlock *defaultBlock = llvm::BasicBlock::Create(getContext(), "defaultBlock", F);
		llvm::BasicBlock *exitBlock = llvm::BasicBlock::Create(getContext(), "exitBlock", F);

		breakStack.push_back(exitBlock);

		first->CompileLLVM();
		llvm::Value *condition = V;

		llvm::SwitchInst *swichInst = builder->CreateSwitch(condition, defaultBlock);

		// Generate code for all cases
		llvm::BasicBlock *nextBlock = NULL;
		for(NodeZeroOP *currCondition = conditionHead, *currBlock = blockHead; currCondition; currCondition = currCondition->next, currBlock = currBlock->next)
		{
			currCondition->CompileLLVM();
			llvm::Value *conditionValue = V;

			llvm::ConstantInt *conditionConstant = llvm::dyn_cast<llvm::ConstantInt>(conditionValue);

			llvm::BasicBlock *caseBlock = nextBlock ? nextBlock : llvm::BasicBlock::Create(getContext(), "caseBlock", F);
			builder->SetInsertPoint(caseBlock);
			currBlock->CompileLLVM();

			nextBlock = currBlock->next ? llvm::BasicBlock::Create(getContext(), "caseBlock", F) : defaultBlock;
			builder->CreateBr(nextBlock);

			swichInst->addCase(conditionConstant, caseBlock);
		}

		// Create default block
		builder->SetInsertPoint(defaultBlock);
		if(defaultCase)
			defaultCase->CompileLLVM();
		builder->CreateBr(exitBlock);

		builder->SetInsertPoint(exitBlock);
		V = NULL;
	}else{
		if(first)
			first->CompileLLVM();
		else if(second)
			second->CompileLLVM();
		llvm::Value *condition = V;

		FastVector<llvm::BasicBlock*, true, true>	conditionBlocks;
		FastVector<llvm::BasicBlock*, true, true>	caseBlocks;

		// Generate blocks for all condiions
		for(NodeZeroOP *currCondition = conditionHead; currCondition; currCondition = currCondition->next)
			conditionBlocks.push_back(llvm::BasicBlock::Create(getContext(), "condBlock", F));

		// Generate blocks for all cases
		for(NodeZeroOP *currBlock = blockHead; currBlock; currBlock = currBlock->next)
			caseBlocks.push_back(llvm::BasicBlock::Create(getContext(), "caseBlock", F));

		llvm::BasicBlock *defaultBlock = llvm::BasicBlock::Create(getContext(), "defaultBlock", F);
		llvm::BasicBlock *exitBlock = llvm::BasicBlock::Create(getContext(), "exitBlock", F);

		builder->CreateBr(conditionBlocks[0]);

		unsigned i;

		// Generate code for all conditions
		i = 0;
		for(NodeZeroOP *currCondition = conditionHead; currCondition; currCondition = currCondition->next, i++)
		{
			builder->SetInsertPoint(conditionBlocks[i]);
			currCondition->CompileLLVM();
			llvm::Value *conditionValue = V;

			if(!second)
			{
				TypeInfo *leftType = first->typeInfo == typeTypeid ? typeInt : first->typeInfo;
				TypeInfo *rightType = currCondition->typeInfo == typeTypeid ? typeInt : currCondition->typeInfo;

				asmStackType fST = ChooseBinaryOpResultType(leftType, rightType)->stackType;

				// Make a copy of the condition
				llvm::Value *conditionOption = CreateEntryBlockAlloca(F, "case_cond_tmp", first->typeInfo->llvmType);
				builder->CreateStore(condition, conditionOption, false);
				conditionOption = builder->CreateLoad(conditionOption, "cond_tmp");

				conditionOption = PromoteToStackType(conditionOption, leftType);
				conditionValue = PromoteToStackType(conditionValue, rightType);

				conditionOption = ConvertFirstForSecond(conditionOption, leftType, rightType);
				conditionValue = ConvertFirstForSecond(conditionValue, rightType, leftType);

				MakeBinaryOp(conditionOption, conditionValue, cmdEqual, fST);
			}

			if(V->getType() != typeBool->llvmType)
				V = builder->CreateICmpNE(V, llvm::ConstantInt::get(getContext(), llvm::APInt(32, 0, true)), "cond_value");

			builder->CreateCondBr(V, caseBlocks[i], currCondition->next ? conditionBlocks[i + 1] : defaultBlock);
		}

		breakStack.push_back(exitBlock);

		// Generate code for all cases
		i = 0;
		for(NodeZeroOP *currBlock = blockHead; currBlock; currBlock = currBlock->next, i++)
		{
			builder->SetInsertPoint(caseBlocks[i]);
			currBlock->CompileLLVM();

			builder->CreateBr(currBlock->next ? caseBlocks[i + 1] : defaultBlock);
		}

		// Create default block
		builder->SetInsertPoint(defaultBlock);
		if(defaultCase)
			defaultCase->CompileLLVM();
		builder->CreateBr(exitBlock);

		builder->SetInsertPoint(exitBlock);
		V = NULL;
	}
}

void NodeConvertPtr::CompileLLVM()
{
	CompileLLVMExtra();

	if(typeInfo == typeTypeid)
	{
		V = GetTypeIndex(first->typeInfo->subType->typeIndex);
	}else if(typeInfo == typeObject){
		first->CompileLLVM();
		llvm::Value *ptr = builder->CreatePointerCast(V, llvm::Type::getInt8PtrTy(getContext()), "ptr_cast");
		// Allocate space for auto ref
		llvm::Value *aggr = CreateEntryBlockAlloca(F, "tmp_autoref", typeObject->llvmType);
		V = builder->CreateStructGEP(aggr, 0, "tmp_arr");

		TypeInfo *type = first->typeInfo->subType;
		if(handleBaseClass && type->firstVariable && type->firstVariable->nameHash == GetStringHash("$typeid"))
		{
			llvm::Value *typeID = builder->CreatePointerCast(ptr, llvm::Type::getInt32PtrTy(getContext()), "typeid_cast");
			builder->CreateStore(builder->CreateLoad(typeID), V);
		}else{
			builder->CreateStore(GetTypeIndex(type->typeIndex), V);
		}

		V = builder->CreateStructGEP(aggr, 1, "tmp_arr");
		builder->CreateStore(ptr, V);
		V = builder->CreateLoad(aggr, "autoref");
	}else{
		first->CompileLLVM();
		// Allocate space for auto ref
		llvm::Value *aggr = CreateEntryBlockAlloca(F, "tmp_autoref", typeObject->llvmType);
		builder->CreateStore(V, aggr);
		V = builder->CreateStructGEP(aggr, 1, "tmp_ptr");
		V = builder->CreateLoad(V, "ptr");
		V = builder->CreatePointerCast(V, typeInfo->llvmType, "ptr_cast");
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

	llvm::Value *aggr = CreateEntryBlockAlloca(F, "func_tmp", typeInfo->llvmType);
	llvm::Value *ptr = builder->CreateStructGEP(aggr, 1, "ptr_part");
	llvm::Value *context = builder->CreateStructGEP(aggr, 0, "ctx_part");

	builder->CreateStore(GetFunctionIndex(funcInfo->indexInArr), ptr);

	llvm::Type *contextType = ((llvm::StructType*)typeInfo->llvmType)->getTypeAtIndex(0u);

	if(funcInfo->type == FunctionInfo::NORMAL)
	{
		builder->CreateStore(llvm::ConstantPointerNull::get((llvm::PointerType*)contextType), context);
	}else{
		first->CompileLLVM();
		V = builder->CreatePointerCast(V, llvm::Type::getInt8PtrTy(getContext()), "context_cast");
		builder->CreateStore(V, context);
	}

	V = builder->CreateLoad(aggr, "func_res");
}

void NodeGetUpvalue::CompileLLVM()
{
	CompileLLVMExtra();

	assert(parentFunc->extraParam);
	assert(parentFunc->extraParam->llvmValue);

	llvm::Value *closure = parentFunc->extraParam->llvmValue;

	// Load pointer to closure into a variable
	V = builder->CreateLoad(closure, "closure");
	// Cast it to char*
	V = builder->CreatePointerCast(V, llvm::Type::getInt8PtrTy(getContext()));
	// Shift to offset
	llvm::Value *arrayIndexHelper[2];
	arrayIndexHelper[0] = llvm::ConstantInt::get(getContext(), llvm::APInt(32, closureElem, true));
	V = builder->CreateGEP(V, llvm::ArrayRef<llvm::Value*>(&arrayIndexHelper[0], 1));
	// cast to result type
	V = builder->CreatePointerCast(V, llvm::PointerType::getUnqual(typeInfo->llvmType));

	V = builder->CreateLoad(V, "closure_deref");
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

	V = builder->CreatePointerCast(V, typeInfo->llvmType, "ptr_cast");
}


void NodeGetFunctionContext::CompileLLVM()
{
	CompileLLVMExtra();

	first->CompileLLVM();

	V = builder->CreateStructGEP(V, 0, "ptr_to_context");
	V = builder->CreateLoad(V, "context");
}

void NodeGetCoroutineState::CompileLLVM()
{
	CompileLLVMExtra();

	first->CompileLLVM();

	V = builder->CreatePointerCast(V, llvm::PointerType::getUnqual(llvm::Type::getInt32PtrTy(getContext())), "ptr_to_ptr_to_state");
	V = builder->CreateLoad(V, "ptr_to_state");
	V = builder->CreateLoad(V, "state");
}


void NodeCreateUnsizedArray::CompileLLVM()
{
	CompileLLVMExtra();

	llvm::Value *arrayTmp = CreateEntryBlockAlloca(F, "array_tmp", typeInfo->llvmType);
	
	first->CompileLLVM();
	V = builder->CreatePointerCast(V, llvm::PointerType::getUnqual(typeInfo->subType->llvmType), "ptr_any");

	llvm::Value *arrayPtr = builder->CreateStructGEP(arrayTmp, 0, "array_ptr");
	builder->CreateStore(V, arrayPtr);

	second->CompileLLVM();
	llvm::Value *arraySize = builder->CreateStructGEP(arrayTmp, 1, "array_size");
	builder->CreateStore(V, arraySize);

	V = builder->CreateLoad(arrayTmp, "unsized_array");
}

#else
void	NULLC_PreventLLVMWarning(){}
#endif
