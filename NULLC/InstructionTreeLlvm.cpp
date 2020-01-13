#include "InstructionTreeLlvm.h"

#include "nullcdef.h"

#if defined(NULLC_LLVM_SUPPORT)
#include "llvm-c/Analysis.h"
#include "llvm-c/BitWriter.h"
#include "llvm-c/Core.h"
#include "llvm-c/Target.h"

#include "llvm-c/Transforms/Scalar.h"

#pragma comment(lib, "llvm-c/lib/LLVMAnalysis.lib")
#pragma comment(lib, "llvm-c/lib/LLVMBinaryFormat.lib")
#pragma comment(lib, "llvm-c/lib/LLVMBitReader.lib")
#pragma comment(lib, "llvm-c/lib/LLVMBitWriter.lib")
#pragma comment(lib, "llvm-c/lib/LLVMCore.lib")
#pragma comment(lib, "llvm-c/lib/LLVMMC.lib")
#pragma comment(lib, "llvm-c/lib/LLVMMCParser.lib")
#pragma comment(lib, "llvm-c/lib/LLVMObject.lib")
#pragma comment(lib, "llvm-c/lib/LLVMProfileData.lib")
#pragma comment(lib, "llvm-c/lib/LLVMSupport.lib")

#pragma comment(lib, "llvm-c/lib/LLVMInstCombine.lib")
#pragma comment(lib, "llvm-c/lib/LLVMScalarOpts.lib")
#pragma comment(lib, "llvm-c/lib/LLVMTransformUtils.lib")

#else

// Stub
struct LLVMContextRefOpaque{};
struct LLVMModuleRefOpaque{};
struct LLVMBuilderRefOpaque{};
struct LLVMPassManagerRefOpaque{};
struct LLVMTypeRefOpaque{};
struct LLVMValueRefOpaque{};
struct LLVMBasicBlockRefOpaque{};
struct LLVMMemoryBufferRefOpaque{};
struct LLVMAttributeRefOpaque{};

typedef LLVMContextRefOpaque* LLVMContextRef;
typedef LLVMModuleRefOpaque* LLVMModuleRef;
typedef LLVMBuilderRefOpaque* LLVMBuilderRef;
typedef LLVMPassManagerRefOpaque* LLVMPassManagerRef;
typedef LLVMTypeRefOpaque* LLVMTypeRef;
typedef LLVMValueRefOpaque* LLVMValueRef;
typedef LLVMBasicBlockRefOpaque* LLVMBasicBlockRef;
typedef LLVMMemoryBufferRefOpaque* LLVMMemoryBufferRef;
typedef LLVMAttributeRefOpaque* LLVMAttributeRef;

static LLVMTypeRefOpaque placeholderType;
static LLVMValueRefOpaque placeholderValue;
static LLVMBasicBlockRefOpaque placeholderBasicBlock;
static LLVMAttributeRefOpaque placeholderAttribute;

typedef unsigned LLVMAttributeIndex;

LLVMTypeRef LLVMVoidTypeInContext(LLVMContextRef){ return &placeholderType; }
LLVMTypeRef LLVMInt1TypeInContext(LLVMContextRef){ return &placeholderType; }
LLVMTypeRef LLVMInt8TypeInContext(LLVMContextRef){ return &placeholderType; }
LLVMTypeRef LLVMInt16TypeInContext(LLVMContextRef){ return &placeholderType; }
LLVMTypeRef LLVMInt32TypeInContext(LLVMContextRef){ return &placeholderType; }
LLVMTypeRef LLVMInt64TypeInContext(LLVMContextRef){ return &placeholderType; }
LLVMTypeRef LLVMFloatTypeInContext(LLVMContextRef){ return &placeholderType; }
LLVMTypeRef LLVMDoubleTypeInContext(LLVMContextRef){ return &placeholderType; }

LLVMTypeRef LLVMPointerType(LLVMTypeRef, unsigned){ return &placeholderType; }
LLVMTypeRef LLVMStructTypeInContext(LLVMContextRef, LLVMTypeRef*, unsigned, bool){ return &placeholderType; }
LLVMTypeRef LLVMArrayType(LLVMTypeRef, unsigned){ return &placeholderType; }
LLVMTypeRef LLVMStructCreateNamed(LLVMContextRef, const char*){ return &placeholderType; }
void LLVMStructSetBody(LLVMTypeRef, LLVMTypeRef*, unsigned, bool){}
LLVMTypeRef LLVMFunctionType(LLVMTypeRef, LLVMTypeRef*, unsigned, bool){ return &placeholderType; }

static int LLVMIntNE = 0;
static int LLVMRealUNE = 0;
static int LLVMIntEQ = 0;
static int LLVMIntSLT = 0;
static int LLVMRealULT = 0;
static int LLVMIntSLE = 0;
static int LLVMRealULE = 0;
static int LLVMIntSGT = 0;
static int LLVMRealUGT = 0;
static int LLVMIntSGE = 0;
static int LLVMRealUGE = 0;
static int LLVMRealUEQ = 0;

LLVMValueRef LLVMBuildSExt(LLVMBuilderRef, LLVMValueRef, LLVMTypeRef, const char*){ return &placeholderValue; }
LLVMValueRef LLVMBuildFPCast(LLVMBuilderRef, LLVMValueRef, LLVMTypeRef, const char*){ return &placeholderValue; }
LLVMValueRef LLVMBuildTrunc(LLVMBuilderRef, LLVMValueRef, LLVMTypeRef, const char*){ return &placeholderValue; }
LLVMValueRef LLVMBuildFPToSI(LLVMBuilderRef, LLVMValueRef, LLVMTypeRef, const char*){ return &placeholderValue; }
LLVMValueRef LLVMBuildSIToFP(LLVMBuilderRef, LLVMValueRef, LLVMTypeRef, const char*){ return &placeholderValue; }
LLVMValueRef LLVMBuildAlloca(LLVMBuilderRef, LLVMTypeRef, const char*){ return &placeholderValue; }
LLVMValueRef LLVMBuildGEP(LLVMBuilderRef, LLVMValueRef, LLVMValueRef*, unsigned, const char*){ return &placeholderValue; }
LLVMValueRef LLVMBuildStore(LLVMBuilderRef, LLVMValueRef, LLVMValueRef){ return &placeholderValue; }
LLVMValueRef LLVMBuildLoad(LLVMBuilderRef, LLVMValueRef, const char*){ return &placeholderValue; }
LLVMValueRef LLVMBuildAdd(LLVMBuilderRef, LLVMValueRef, LLVMValueRef, const char*){ return &placeholderValue; }
LLVMValueRef LLVMBuildFAdd(LLVMBuilderRef, LLVMValueRef, LLVMValueRef, const char*){ return &placeholderValue; }
LLVMValueRef LLVMBuildZExt(LLVMBuilderRef, LLVMValueRef, LLVMTypeRef, const char*){ return &placeholderValue; }
LLVMValueRef LLVMBuildICmp(LLVMBuilderRef, int, LLVMValueRef, LLVMValueRef, const char*){ return &placeholderValue; }
LLVMValueRef LLVMBuildFCmp(LLVMBuilderRef, int, LLVMValueRef, LLVMValueRef, const char*){ return &placeholderValue; }
LLVMValueRef LLVMBuildExtractValue(LLVMBuilderRef, LLVMValueRef, unsigned, const char*){ return &placeholderValue; }
LLVMValueRef LLVMBuildInsertValue(LLVMBuilderRef, LLVMValueRef, LLVMValueRef, unsigned, const char*){ return &placeholderValue; }
LLVMValueRef LLVMBuildPointerCast(LLVMBuilderRef, LLVMValueRef, LLVMTypeRef, const char*){ return &placeholderValue; }
LLVMValueRef LLVMBuildCall(LLVMBuilderRef, LLVMValueRef, LLVMValueRef*, unsigned, const char*){ return &placeholderValue; }
LLVMValueRef LLVMBuildNeg(LLVMBuilderRef, LLVMValueRef, const char*){ return &placeholderValue; }
LLVMValueRef LLVMBuildFNeg(LLVMBuilderRef, LLVMValueRef, const char*){ return &placeholderValue; }
LLVMValueRef LLVMBuildNot(LLVMBuilderRef, LLVMValueRef, const char*){ return &placeholderValue; }
LLVMValueRef LLVMBuildCondBr(LLVMBuilderRef, LLVMValueRef, LLVMBasicBlockRef, LLVMBasicBlockRef){ return &placeholderValue; }
LLVMValueRef LLVMBuildBr(LLVMBuilderRef, LLVMBasicBlockRef){ return &placeholderValue; }
LLVMValueRef LLVMBuildPhi(LLVMBuilderRef, LLVMTypeRef, const char*){ return &placeholderValue; }
LLVMValueRef LLVMBuildSub(LLVMBuilderRef, LLVMValueRef, LLVMValueRef, const char*){ return &placeholderValue; }
LLVMValueRef LLVMBuildFSub(LLVMBuilderRef, LLVMValueRef, LLVMValueRef, const char*){ return &placeholderValue; }
LLVMValueRef LLVMBuildMul(LLVMBuilderRef, LLVMValueRef, LLVMValueRef, const char*){ return &placeholderValue; }
LLVMValueRef LLVMBuildFMul(LLVMBuilderRef, LLVMValueRef, LLVMValueRef, const char*){ return &placeholderValue; }
LLVMValueRef LLVMBuildSDiv(LLVMBuilderRef, LLVMValueRef, LLVMValueRef, const char*){ return &placeholderValue; }
LLVMValueRef LLVMBuildFDiv(LLVMBuilderRef, LLVMValueRef, LLVMValueRef, const char*){ return &placeholderValue; }
LLVMValueRef LLVMBuildSRem(LLVMBuilderRef, LLVMValueRef, LLVMValueRef, const char*){ return &placeholderValue; }
LLVMValueRef LLVMBuildFRem(LLVMBuilderRef, LLVMValueRef, LLVMValueRef, const char*){ return &placeholderValue; }
LLVMValueRef LLVMBuildShl(LLVMBuilderRef, LLVMValueRef, LLVMValueRef, const char*){ return &placeholderValue; }
LLVMValueRef LLVMBuildAShr(LLVMBuilderRef, LLVMValueRef, LLVMValueRef, const char*){ return &placeholderValue; }
LLVMValueRef LLVMBuildAnd(LLVMBuilderRef, LLVMValueRef, LLVMValueRef, const char*){ return &placeholderValue; }
LLVMValueRef LLVMBuildOr(LLVMBuilderRef, LLVMValueRef, LLVMValueRef, const char*){ return &placeholderValue; }
LLVMValueRef LLVMBuildXor(LLVMBuilderRef, LLVMValueRef, LLVMValueRef, const char*){ return &placeholderValue; }
LLVMValueRef LLVMBuildRetVoid(LLVMBuilderRef){ return &placeholderValue; }
LLVMValueRef LLVMBuildRet(LLVMBuilderRef, LLVMValueRef){ return &placeholderValue; }
LLVMValueRef LLVMBuildSwitch(LLVMBuilderRef, LLVMValueRef, LLVMBasicBlockRef, unsigned){ return &placeholderValue; }

LLVMValueRef LLVMConstInt(LLVMTypeRef, unsigned long long, bool){ return &placeholderValue; }
LLVMValueRef LLVMConstReal(LLVMTypeRef, double){ return &placeholderValue; }
LLVMValueRef LLVMConstStringInContext(LLVMContextRef, char*, unsigned, bool){ return &placeholderValue; }
LLVMValueRef LLVMConstNamedStruct(LLVMTypeRef, LLVMValueRef*, unsigned){ return &placeholderValue; }
LLVMValueRef LLVMConstPointerNull(LLVMTypeRef){ return &placeholderValue; }
LLVMValueRef LLVMConstNull(LLVMTypeRef){ return &placeholderValue; }

LLVMBasicBlockRef LLVMAppendBasicBlockInContext(LLVMContextRef, LLVMValueRef, const char*){ return &placeholderBasicBlock; }
void LLVMPositionBuilderAtEnd(LLVMBuilderRef, LLVMBasicBlockRef){}
void LLVMAddIncoming(LLVMValueRef, LLVMValueRef*, LLVMBasicBlockRef*, unsigned){}
void LLVMAddCase(LLVMValueRef, LLVMValueRef, LLVMBasicBlockRef){}

LLVMValueRef LLVMAddFunction(LLVMModuleRef, const char*, LLVMTypeRef){ return &placeholderValue; }
LLVMValueRef LLVMGetNamedFunction(LLVMModuleRef, const char*){ return &placeholderValue; }
LLVMValueRef LLVMGetParam(LLVMValueRef, unsigned){ return &placeholderValue; }
LLVMValueRef LLVMAddGlobal(LLVMModuleRef, LLVMTypeRef, const char*){ return &placeholderValue; }

LLVMTypeRef LLVMTypeOf(LLVMValueRef){ return &placeholderType; }
void LLVMDumpType(LLVMTypeRef){}
void LLVMDumpValue(LLVMValueRef){}

static int LLVMReturnStatusAction = 0;
int LLVMVerifyFunction(LLVMValueRef, int){ return 0; }

int LLVMVerifyModule(LLVMModuleRef, int, char**){ return 0; }

void LLVMDisposeMessage(const char*){}

int LLVMRunFunctionPassManager(LLVMPassManagerRef, LLVMValueRef){ return 0; }

LLVMContextRef LLVMContextCreate(){ return 0; }
LLVMModuleRef LLVMModuleCreateWithNameInContext(const char*, LLVMContextRef){ return 0; }
LLVMBuilderRef LLVMCreateBuilderInContext(LLVMContextRef){ return 0; }
LLVMPassManagerRef LLVMCreateFunctionPassManagerForModule(LLVMModuleRef){ return 0; }

void LLVMAddBasicAliasAnalysisPass(LLVMPassManagerRef){}
void LLVMAddScalarReplAggregatesPass(LLVMPassManagerRef){}
void LLVMAddInstructionCombiningPass(LLVMPassManagerRef){}
void LLVMAddEarlyCSEPass(LLVMPassManagerRef){}
void LLVMAddReassociatePass(LLVMPassManagerRef){}
void LLVMAddGVNPass(LLVMPassManagerRef){}
void LLVMAddConstantPropagationPass(LLVMPassManagerRef){}
void LLVMAddCFGSimplificationPass(LLVMPassManagerRef){}
void LLVMAddAggressiveDCEPass(LLVMPassManagerRef){}

void LLVMInitializeFunctionPassManager(LLVMPassManagerRef){}
void LLVMFinalizeFunctionPassManager(LLVMPassManagerRef){}

void LLVMDisposePassManager(LLVMPassManagerRef){}
void LLVMDisposeBuilder(LLVMBuilderRef){}
void LLVMDisposeModule(LLVMModuleRef){}
void LLVMContextDispose(LLVMContextRef){}

LLVMMemoryBufferRef LLVMWriteBitcodeToMemoryBuffer(LLVMModuleRef){ return 0; }
unsigned LLVMGetBufferSize(LLVMMemoryBufferRef){ return 0; }
const char* LLVMGetBufferStart(LLVMMemoryBufferRef){ return 0; }
void LLVMDisposeMemoryBuffer(LLVMMemoryBufferRef){}

int LLVMExternalLinkage = 0;
void LLVMSetLinkage(LLVMValueRef, int) {}
void LLVMSetAlignment(LLVMValueRef, int){}
void LLVMSetInitializer(LLVMValueRef, LLVMValueRef){}

unsigned LLVMGetEnumAttributeKindForName(const char*, unsigned){ return 0; }
LLVMAttributeRef LLVMCreateEnumAttribute(LLVMContextRef, unsigned, unsigned){ return &placeholderAttribute; }
void LLVMAddCallSiteAttribute(LLVMValueRef, LLVMAttributeIndex, LLVMAttributeRef){}
void LLVMAddAttributeAtIndex(LLVMValueRef, LLVMAttributeIndex, LLVMAttributeRef){}

#endif

#include "ExpressionTree.h"
#include "DenseMap.h"

struct LlvmCompilationContext
{
	LlvmCompilationContext(ExpressionContext &ctx): ctx(ctx), types(ctx.allocator), functions(ctx.allocator), allocator(ctx.allocator)
	{
		enableOptimization = false;

		context = NULL;

		module = NULL;

		builder = NULL;

		functionPassManager = NULL;

		skipFunctionDefinitions = false;

		currentFunction = NULL;
		currentFunctionSource = NULL;
		currentFunctionGlobal = false;

		currentNextRestoreBlock = 0;
	}

	ExpressionContext &ctx;

	bool enableOptimization;

	LLVMContextRef context;

	LLVMModuleRef module;

	LLVMBuilderRef builder;

	LLVMPassManagerRef functionPassManager;

	SmallArray<LLVMTypeRef, 128> types;
	SmallArray<LLVMValueRef, 128> functions;

	SmallDenseMap<unsigned, LLVMValueRef, SmallDenseMapUnsignedHasher, 128> variables;

	bool skipFunctionDefinitions;

	LLVMValueRef currentFunction;
	FunctionData *currentFunctionSource;
	bool currentFunctionGlobal;

	unsigned currentNextRestoreBlock;
	SmallArray<LLVMBasicBlockRef, 16> currentRestoreBlocks;

	struct LoopInfo
	{
		LoopInfo(): breakBlock(NULL), continueBlock(NULL)
		{
		}

		LoopInfo(LLVMBasicBlockRef breakBlock, LLVMBasicBlockRef continueBlock): breakBlock(breakBlock), continueBlock(continueBlock)
		{
		}

		LLVMBasicBlockRef breakBlock;
		LLVMBasicBlockRef continueBlock;
	};

	SmallArray<LoopInfo, 32> loopInfo;

	// Memory pool
	Allocator *allocator;

	template<typename T>
	T* get()
	{
		return (T*)allocator->alloc(sizeof(T));
	}
};

LLVMValueRef CompileLlvm(LlvmCompilationContext &ctx, ExprBase *expression);

char* CreateLlvmName(LlvmCompilationContext &ctx, InplaceStr str)
{
	char *name = (char*)ctx.allocator->alloc(str.length() + 1);

	memcpy(name, str.begin, str.length());
	name[str.length()] = 0;

	return name;
}

char* CreateLlvmFunctionName(LlvmCompilationContext &ctx, FunctionData *function)
{
	unsigned length = function->name->name.length();
	length++; // '#'
	length += function->type->name.length();

	if(function->generics.head)
	{
		length++; // '$'

		for(MatchData *curr = function->generics.head; curr; curr = curr->next)
		{
			length += curr->type->name.length();

			if(curr->next)
				length++; // ','
		}
	}

	char *name = (char*)ctx.allocator->alloc(length + 1);

	char *pos = name;

	memcpy(pos, function->name->name.begin, function->name->name.length());
	pos += function->name->name.length();

	*pos++ = '#';

	memcpy(pos, function->type->name.begin, function->type->name.length());
	pos += function->type->name.length();

	if(function->generics.head)
	{
		*pos++ = '$';

		for(MatchData *curr = function->generics.head; curr; curr = curr->next)
		{
			memcpy(pos, curr->type->name.begin, curr->type->name.length());
			pos += curr->type->name.length();

			if(curr->next)
				*pos++ = ',';
		}
	}

	*pos = 0;

	return name;
}

LLVMTypeRef CompileLlvmType(LlvmCompilationContext &ctx, TypeBase *type)
{
	if(LLVMTypeRef llvmType = ctx.types[type->typeIndex])
		return llvmType;

	if(isType<TypeVoid>(type))
	{
		ctx.types[type->typeIndex] = LLVMVoidTypeInContext(ctx.context);
	}
	else if(isType<TypeBool>(type))
	{
		ctx.types[type->typeIndex] = LLVMInt1TypeInContext(ctx.context);
	}
	else if(isType<TypeChar>(type))
	{
		ctx.types[type->typeIndex] = LLVMInt8TypeInContext(ctx.context);
	}
	else if(isType<TypeShort>(type))
	{
		ctx.types[type->typeIndex] = LLVMInt16TypeInContext(ctx.context);
	}
	else if(isType<TypeInt>(type))
	{
		ctx.types[type->typeIndex] = LLVMInt32TypeInContext(ctx.context);
	}
	else if(isType<TypeLong>(type))
	{
		ctx.types[type->typeIndex] = LLVMInt64TypeInContext(ctx.context);
	}
	else if(isType<TypeFloat>(type))
	{
		ctx.types[type->typeIndex] = LLVMFloatTypeInContext(ctx.context);
	}
	else if(isType<TypeDouble>(type))
	{
		ctx.types[type->typeIndex] = LLVMDoubleTypeInContext(ctx.context);
	}
	else if(isType<TypeTypeID>(type))
	{
		ctx.types[type->typeIndex] = LLVMInt32TypeInContext(ctx.context);
	}
	else if(isType<TypeFunctionID>(type))
	{
		ctx.types[type->typeIndex] = LLVMInt32TypeInContext(ctx.context);
	}
	else if(isType<TypeNullptr>(type))
	{
		ctx.types[type->typeIndex] = LLVMPointerType(CompileLlvmType(ctx, ctx.ctx.typeChar), 0);
	}
	else if(isType<TypeAutoRef>(type))
	{
		LLVMTypeRef members[2] = { LLVMInt32TypeInContext(ctx.context), LLVMPointerType(CompileLlvmType(ctx, ctx.ctx.typeChar), 0) };

		ctx.types[type->typeIndex] = LLVMStructTypeInContext(ctx.context, members, 2, true);
	}
	else if(isType<TypeAutoArray>(type))
	{
		LLVMTypeRef members[3] = { LLVMInt32TypeInContext(ctx.context), LLVMPointerType(CompileLlvmType(ctx, ctx.ctx.typeChar), 0), LLVMInt32TypeInContext(ctx.context) };

		ctx.types[type->typeIndex] = LLVMStructTypeInContext(ctx.context, members, 3, true);
	}
	else if(TypeRef *typeRef = getType<TypeRef>(type))
	{
		ctx.types[type->typeIndex] = LLVMPointerType(CompileLlvmType(ctx, typeRef->subType == ctx.ctx.typeVoid ? ctx.ctx.typeChar : typeRef->subType), 0);
	}
	else if(TypeArray *typeArray = getType<TypeArray>(type))
	{
		if(typeArray->subType == ctx.ctx.typeChar)
			ctx.types[type->typeIndex] = LLVMArrayType(CompileLlvmType(ctx, typeArray->subType), ((unsigned)typeArray->length + 3u) & ~3u);
		else
			ctx.types[type->typeIndex] = LLVMArrayType(CompileLlvmType(ctx, typeArray->subType), (unsigned)typeArray->length);
	}
	else if(TypeUnsizedArray *typeUnsizedArray = getType<TypeUnsizedArray>(type))
	{
		LLVMTypeRef members[2] = { LLVMPointerType(CompileLlvmType(ctx, typeUnsizedArray->subType), 0), LLVMInt32TypeInContext(ctx.context) };

		ctx.types[type->typeIndex] = LLVMStructTypeInContext(ctx.context, members, 2, true);
	}
	else if(isType<TypeFunction>(type))
	{
		// TODO: use function indices and remap before execution
		//LLVMTypeRef members[2] = { LLVMPointerType(CompileLlvmType(ctx, ctx.ctx.typeChar), 0), LLVMInt32TypeInContext(ctx.context) };
		LLVMTypeRef members[2] = { CompileLlvmType(ctx, ctx.ctx.typeNullPtr), CompileLlvmType(ctx, ctx.ctx.typeNullPtr) };

		ctx.types[type->typeIndex] = LLVMStructTypeInContext(ctx.context, members, 2, true);
	}
	else if(TypeClass *typeClass = getType<TypeClass>(type))
	{
		ctx.types[type->typeIndex] = LLVMStructCreateNamed(ctx.context, CreateLlvmName(ctx, typeClass->name));

		SmallArray<LLVMTypeRef, 32> members(ctx.allocator);

		for(MemberHandle *curr = typeClass->members.head; curr; curr = curr->next)
			members.push_back(CompileLlvmType(ctx, curr->variable->type));

		// TODO: create packed type with custom padding
		LLVMStructSetBody(ctx.types[type->typeIndex], members.data, members.count, false);
	}
	else if(isType<TypeEnum>(type))
	{
		ctx.types[type->typeIndex] = LLVMInt32TypeInContext(ctx.context);
	}
	else
	{
		assert(!"unknown type");
	}

	return ctx.types[type->typeIndex];
}

bool IsStructReturnType(TypeBase *type)
{
#if defined(_WIN64)
	if(isType<TypeVoid>(type))
		return false;

	if(TypeStruct *typeStruct = getType<TypeStruct>(type))
	{
		for(MemberHandle *curr = typeStruct->members.head; curr; curr = curr->next)
		{
			if(IsStructReturnType(curr->variable->type))
				return true;
		}
	}
	return type->size != 1 && type->size != 2 && type->size != 4 && type->size != 8;
#else
	(void)type;

	return false;
#endif
}

bool IsStructArgumentType(TypeBase *type)
{
#if defined(_WIN64)
	if(isType<TypeVoid>(type))
		return false;

	return type->size != 1 && type->size != 2 && type->size != 4 && type->size != 8;
#else
	(void)type;

	return false;
#endif
}

LLVMTypeRef CompileLlvmFunctionType(LlvmCompilationContext& ctx, TypeBase *returnType, ArrayView<TypeBase*> argumentTypes)
{
	bool isStructReturnType = IsStructReturnType(returnType);

	SmallArray<LLVMTypeRef, 32> arguments(ctx.allocator);

	if (isStructReturnType)
	{
		TypeBase* newType = ctx.ctx.GetReferenceType(returnType);

		if (newType->typeIndex == ~0u)
		{
			assert(newType == ctx.ctx.types.back());

			ctx.ctx.types.back()->typeIndex = ctx.ctx.types.size() - 1;
			ctx.types.push_back(NULL);
		}

		arguments.push_back(CompileLlvmType(ctx, newType));
	}

	for (unsigned i = 0; i < argumentTypes.size(); i++)
	{
		TypeBase* argumentType = argumentTypes[i];

		if (IsStructArgumentType(argumentType))
		{
			TypeBase* newType = ctx.ctx.GetReferenceType(argumentType);

			if (newType->typeIndex == ~0u)
			{
				assert(newType == ctx.ctx.types.back());

				ctx.ctx.types.back()->typeIndex = ctx.ctx.types.size() - 1;
				ctx.types.push_back(NULL);
			}

			arguments.push_back(CompileLlvmType(ctx, newType));
		}
		else
		{
			arguments.push_back(CompileLlvmType(ctx, argumentType));
		}
	}

	LLVMTypeRef resultType = isStructReturnType ? LLVMVoidTypeInContext(ctx.context) : CompileLlvmType(ctx, returnType);

	return LLVMFunctionType(resultType, arguments.data, arguments.count, false);
}

LLVMTypeRef CompileLlvmFunctionType(LlvmCompilationContext &ctx, TypeFunction *functionType)
{
	SmallArray<TypeBase*, 32> argumentTypes(ctx.allocator);

	for(TypeHandle *curr = functionType->arguments.head; curr; curr = curr->next)
		argumentTypes.push_back(curr->type);

	argumentTypes.push_back(ctx.ctx.typeNullPtr);

	return CompileLlvmFunctionType(ctx, functionType->returnType, argumentTypes);
}

LLVMValueRef ConvertToStackType(LlvmCompilationContext &ctx, LLVMValueRef value, TypeBase *valueType)
{
	if(valueType == ctx.ctx.typeBool)
		return LLVMBuildSExt(ctx.builder, value, CompileLlvmType(ctx, ctx.ctx.typeInt), "");

	if(valueType == ctx.ctx.typeChar)
		return LLVMBuildSExt(ctx.builder, value, CompileLlvmType(ctx, ctx.ctx.typeInt), "");

	if(valueType == ctx.ctx.typeShort)
		return LLVMBuildSExt(ctx.builder, value, CompileLlvmType(ctx, ctx.ctx.typeInt), "");

	if(valueType == ctx.ctx.typeFloat)
		return LLVMBuildFPCast(ctx.builder, value, CompileLlvmType(ctx, ctx.ctx.typeDouble), "");

	return value;
}

TypeBase* GetStackType(LlvmCompilationContext &ctx, TypeBase *valueType)
{
	if(valueType == ctx.ctx.typeBool)
		return ctx.ctx.typeInt;

	if(valueType == ctx.ctx.typeChar)
		return ctx.ctx.typeInt;

	if(valueType == ctx.ctx.typeShort)
		return ctx.ctx.typeInt;

	if(valueType == ctx.ctx.typeFloat)
		return ctx.ctx.typeDouble;

	return valueType;
}

LLVMValueRef ConvertToDataType(LlvmCompilationContext &ctx, LLVMValueRef value, TypeBase *valueType, TypeBase *targetType)
{
	if(targetType == valueType)
		return value;

	if(valueType == ctx.ctx.typeInt)
	{
		if(targetType == ctx.ctx.typeBool)
			return LLVMBuildTrunc(ctx.builder, value, CompileLlvmType(ctx, targetType), "");

		if(targetType == ctx.ctx.typeChar)
			return LLVMBuildTrunc(ctx.builder, value, CompileLlvmType(ctx, targetType), "");

		if(targetType == ctx.ctx.typeShort)
			return LLVMBuildTrunc(ctx.builder, value, CompileLlvmType(ctx, targetType), "");
	}

	if(valueType == ctx.ctx.typeDouble)
	{
		if(targetType == ctx.ctx.typeFloat)
			return LLVMBuildFPCast(ctx.builder, value, CompileLlvmType(ctx, targetType), "");
	}

	return value;
}

LLVMValueRef CompileArgument(LlvmCompilationContext& ctx, TypeBase *argumentType, LLVMValueRef argument)
{
	if (IsStructArgumentType(argumentType))
	{
		LLVMValueRef storage = LLVMBuildAlloca(ctx.builder, CompileLlvmType(ctx, argumentType), "");

		LLVMSetAlignment(storage, 16);

		LLVMBuildStore(ctx.builder, argument, storage);

		return storage;
	}

	return argument;
}

LLVMValueRef CompileArgument(LlvmCompilationContext& ctx, TypeBase* argumentType, ExprBase* value)
{
	LLVMValueRef argument = CompileLlvm(ctx, value);

	argument = ConvertToDataType(ctx, argument, GetStackType(ctx, value->type), value->type);

	return CompileArgument(ctx, argumentType, argument);
}

LLVMValueRef CheckType(LlvmCompilationContext &ctx, ExprBase *node, LLVMValueRef value)
{
	if(!value)
	{
		assert(node->type == ctx.ctx.typeVoid);

		return NULL;
	}

	LLVMTypeRef valueType = LLVMTypeOf(value);
	LLVMTypeRef expectedType = CompileLlvmType(ctx, GetStackType(ctx, node->type));

	if(valueType != expectedType)
	{
		printf("Wrong result type\nExpected: ");
		LLVMDumpType(expectedType);
		printf("\n     Got: ");
		LLVMDumpType(valueType);
		printf("\n");

		assert(!"wrong result type");
	}

	return value;
}

void CheckFunction(LlvmCompilationContext &ctx, LLVMValueRef function, InplaceStr name)
{
	(void)ctx;
	(void)function;
	(void)name;

#if !defined(NDEBUG)
	// Check result
	if(LLVMVerifyFunction(function, LLVMReturnStatusAction))
	{
		LLVMDumpValue(function);

		printf("LLVM function '%.*s' verification failed\n", name.length(), name.begin);

		char *error = NULL;

		if(LLVMVerifyModule(ctx.module, LLVMReturnStatusAction, &error))
			printf("%s\n", error);

		LLVMDisposeMessage(error);
	}
#endif
}

LLVMBasicBlockRef GetLoopContinueBlock(LlvmCompilationContext &ctx, unsigned depth)
{
	unsigned pos = ctx.loopInfo.size();

	for(unsigned i = 0; i < depth; i++)
	{
		pos--;

		while(!ctx.loopInfo[pos].continueBlock)
			pos--;
	}

	return ctx.loopInfo[pos].continueBlock;
}

LLVMValueRef CompileLlvmVoid(LlvmCompilationContext &ctx, ExprVoid *node)
{
	return CheckType(ctx, node, NULL);
}

LLVMValueRef CompileLlvmBoolLiteral(LlvmCompilationContext &ctx, ExprBoolLiteral *node)
{
	return CheckType(ctx, node, LLVMConstInt(CompileLlvmType(ctx, ctx.ctx.typeInt), node->value ? 1 : 0, true));
}

LLVMValueRef CompileLlvmCharacterLiteral(LlvmCompilationContext &ctx, ExprCharacterLiteral *node)
{
	return CheckType(ctx, node, LLVMConstInt(CompileLlvmType(ctx, ctx.ctx.typeInt), node->value, true));
}

LLVMValueRef CompileLlvmStringLiteral(LlvmCompilationContext &ctx, ExprStringLiteral *node)
{
	unsigned size = node->length + 1;

	// Align to 4
	size = (size + 3) & ~3;

	char *value = (char*)ctx.allocator->alloc(size);
	memset(value, 0, size);

	for(unsigned i = 0; i < node->length; i++)
		value[i] = node->value[i];

	LLVMValueRef result = LLVMConstStringInContext(ctx.context, value, size, true);

	return CheckType(ctx, node, result);
}

LLVMValueRef CompileLlvmIntegerLiteral(LlvmCompilationContext &ctx, ExprIntegerLiteral *node)
{
	return CheckType(ctx, node, LLVMConstInt(CompileLlvmType(ctx, GetStackType(ctx, node->type)), node->value, true));
}

LLVMValueRef CompileLlvmRationalLiteral(LlvmCompilationContext &ctx, ExprRationalLiteral *node)
{
	return CheckType(ctx, node, LLVMConstReal(CompileLlvmType(ctx, ctx.ctx.typeDouble), node->value));
}

LLVMValueRef CompileLlvmTypeLiteral(LlvmCompilationContext &ctx, ExprTypeLiteral *node)
{
	// TODO: use global type index values for a later remap
	return CheckType(ctx, node, LLVMConstInt(CompileLlvmType(ctx, node->type), node->value->typeIndex, true));
}

LLVMValueRef CompileLlvmNullptrLiteral(LlvmCompilationContext &ctx, ExprNullptrLiteral *node)
{
	return CheckType(ctx, node, LLVMConstPointerNull(CompileLlvmType(ctx, node->type)));
}

LLVMValueRef CompileLlvmFunctionIndexLiteral(LlvmCompilationContext &ctx, ExprFunctionIndexLiteral *node)
{
	// TODO: use global function index values for a later remap
	return CheckType(ctx, node, LLVMConstInt(CompileLlvmType(ctx, node->type), node->function->functionIndex, true));
}

LLVMValueRef CompileLlvmPassthrough(LlvmCompilationContext &ctx, ExprPassthrough *node)
{
	return CheckType(ctx, node, CompileLlvm(ctx, node->value));
}

LLVMValueRef CompileLlvmArray(LlvmCompilationContext &ctx, ExprArray *node)
{
	LLVMValueRef storage = LLVMBuildAlloca(ctx.builder, CompileLlvmType(ctx, node->type), "arr_lit");

	unsigned i = 0;

	for(ExprBase *value = node->values.head; value; value = value->next)
	{
		LLVMValueRef element = CompileLlvm(ctx, value);

		LLVMValueRef indices[] = { LLVMConstInt(LLVMInt32TypeInContext(ctx.context), 0, true), LLVMConstInt(LLVMInt32TypeInContext(ctx.context), i, true) };

		LLVMValueRef address = LLVMBuildGEP(ctx.builder, storage, indices, 2, "");

		element = ConvertToDataType(ctx, element, GetStackType(ctx, value->type), value->type);

		LLVMBuildStore(ctx.builder, element, address);

		i++;
	}

	LLVMValueRef result = LLVMBuildLoad(ctx.builder, storage, "");

	return CheckType(ctx, node, result);
}

LLVMValueRef CompileLlvmPreModify(LlvmCompilationContext &ctx, ExprPreModify *node)
{
	LLVMValueRef address = CompileLlvm(ctx, node->value);

	TypeRef *refType = getType<TypeRef>(node->value->type);

	assert(refType);

	LLVMValueRef value = ConvertToStackType(ctx, LLVMBuildLoad(ctx.builder, address, ""), refType->subType);

	TypeBase *stackType = GetStackType(ctx, refType->subType);

	if(ctx.ctx.IsIntegerType(stackType))
		value = LLVMBuildAdd(ctx.builder, value, LLVMConstInt(CompileLlvmType(ctx, stackType), node->isIncrement ? 1 : -1, true), "");
	else
		value = LLVMBuildFAdd(ctx.builder, value, LLVMConstReal(CompileLlvmType(ctx, stackType), node->isIncrement ? 1.0 : -1.0), "");

	LLVMBuildStore(ctx.builder, ConvertToDataType(ctx, value, stackType, refType->subType), address);

	return CheckType(ctx, node, value);
}

LLVMValueRef CompileLlvmPostModify(LlvmCompilationContext &ctx, ExprPostModify *node)
{
	LLVMValueRef address = CompileLlvm(ctx, node->value);

	TypeRef *refType = getType<TypeRef>(node->value->type);

	assert(refType);

	LLVMValueRef value = ConvertToStackType(ctx, LLVMBuildLoad(ctx.builder, address, ""), refType->subType);
	LLVMValueRef result = value;

	TypeBase *stackType = GetStackType(ctx, refType->subType);

	if(ctx.ctx.IsIntegerType(stackType))
		value = LLVMBuildAdd(ctx.builder, value, LLVMConstInt(CompileLlvmType(ctx, stackType), node->isIncrement ? 1 : -1, true), "");
	else
		value = LLVMBuildFAdd(ctx.builder, value, LLVMConstReal(CompileLlvmType(ctx, stackType), node->isIncrement ? 1.0 : -1.0), "");

	LLVMBuildStore(ctx.builder, ConvertToDataType(ctx, value, stackType, refType->subType), address);

	return CheckType(ctx, node, result);
}

LLVMValueRef CompileLlvmTypeCast(LlvmCompilationContext &ctx, ExprTypeCast *node)
{
	LLVMValueRef value = CompileLlvm(ctx, node->value);

	switch(node->category)
	{
	case EXPR_CAST_NUMERICAL:
		if(node->type == ctx.ctx.typeBool)
		{
			if(ctx.ctx.IsIntegerType(node->value->type))
				return CheckType(ctx, node, LLVMBuildZExt(ctx.builder, LLVMBuildICmp(ctx.builder, LLVMIntNE, value, LLVMConstInt(CompileLlvmType(ctx, GetStackType(ctx, node->value->type)), 0, true), ""), CompileLlvmType(ctx, ctx.ctx.typeInt), ""));

			return CheckType(ctx, node, LLVMBuildZExt(ctx.builder, LLVMBuildFCmp(ctx.builder, LLVMRealUNE, value, LLVMConstReal(CompileLlvmType(ctx, GetStackType(ctx, node->value->type)), 0.0), ""), CompileLlvmType(ctx, ctx.ctx.typeInt), ""));
		}

		if(TypeBase *resultStackType = GetStackType(ctx, node->type))
		{
			if(resultStackType == node->value->type)
				return CheckType(ctx, node, value);

			if(resultStackType == ctx.ctx.typeInt)
			{
				if(node->value->type == ctx.ctx.typeLong)
					return CheckType(ctx, node, LLVMBuildTrunc(ctx.builder, value, CompileLlvmType(ctx, resultStackType), ""));

				if(node->value->type == ctx.ctx.typeBool || node->value->type == ctx.ctx.typeChar || node->value->type == ctx.ctx.typeShort)
					return CheckType(ctx, node, LLVMBuildSExt(ctx.builder, value, CompileLlvmType(ctx, resultStackType), ""));

				if(node->value->type == ctx.ctx.typeFloat || node->value->type == ctx.ctx.typeDouble)
					return CheckType(ctx, node, LLVMBuildFPToSI(ctx.builder, value, CompileLlvmType(ctx, resultStackType), ""));
			}
			else if(resultStackType == ctx.ctx.typeLong)
			{
				if(node->value->type == ctx.ctx.typeBool || node->value->type == ctx.ctx.typeChar || node->value->type == ctx.ctx.typeShort || node->value->type == ctx.ctx.typeInt)
					return CheckType(ctx, node, LLVMBuildSExt(ctx.builder, value, CompileLlvmType(ctx, resultStackType), ""));

				if(node->value->type == ctx.ctx.typeFloat || node->value->type == ctx.ctx.typeDouble)
					return CheckType(ctx, node, LLVMBuildFPToSI(ctx.builder, value, CompileLlvmType(ctx, resultStackType), ""));
			}
			else if(resultStackType == ctx.ctx.typeDouble)
			{
				if(node->value->type == ctx.ctx.typeBool || node->value->type == ctx.ctx.typeChar || node->value->type == ctx.ctx.typeShort || node->value->type == ctx.ctx.typeInt || node->value->type == ctx.ctx.typeLong)
					return CheckType(ctx, node, LLVMBuildSIToFP(ctx.builder, value, CompileLlvmType(ctx, resultStackType), ""));

				if(node->value->type == ctx.ctx.typeFloat || node->value->type == ctx.ctx.typeDouble)
					return CheckType(ctx, node, LLVMBuildFPCast(ctx.builder, value, CompileLlvmType(ctx, resultStackType), ""));
			}

			assert(!"unknown cast");
		}

		return CheckType(ctx, node, NULL);
	case EXPR_CAST_PTR_TO_BOOL:
		return CheckType(ctx, node, LLVMBuildZExt(ctx.builder, LLVMBuildICmp(ctx.builder, LLVMIntNE, value, LLVMConstPointerNull(CompileLlvmType(ctx, node->value->type)), ""), CompileLlvmType(ctx, ctx.ctx.typeInt), ""));
	case EXPR_CAST_UNSIZED_TO_BOOL:
		if(TypeUnsizedArray *unsizedArrType = getType<TypeUnsizedArray>(node->value->type))
		{
			LLVMValueRef ptr = LLVMBuildExtractValue(ctx.builder, value, 0, "arr_ptr");

			return CheckType(ctx, node, LLVMBuildZExt(ctx.builder, LLVMBuildICmp(ctx.builder, LLVMIntNE, ptr, LLVMConstPointerNull(LLVMPointerType(CompileLlvmType(ctx, unsizedArrType->subType), 0)), ""), CompileLlvmType(ctx, ctx.ctx.typeInt), ""));
		}

		break;
	case EXPR_CAST_FUNCTION_TO_BOOL:
		// TODO: replace with a function index comparison
		if(LLVMValueRef ptr = LLVMBuildExtractValue(ctx.builder, value, 1, "func_ptr"))
		{
			return CheckType(ctx, node, LLVMBuildZExt(ctx.builder, LLVMBuildICmp(ctx.builder, LLVMIntNE, ptr, LLVMConstPointerNull(CompileLlvmType(ctx, ctx.ctx.typeNullPtr)), ""), CompileLlvmType(ctx, ctx.ctx.typeInt), ""));
		}

		break;
	case EXPR_CAST_NULL_TO_PTR:
		return CheckType(ctx, node, LLVMBuildPointerCast(ctx.builder, value, CompileLlvmType(ctx, node->type), "null_to_ptr"));
	case EXPR_CAST_NULL_TO_AUTO_PTR:
	{
		LLVMValueRef constants[] = { LLVMConstInt(CompileLlvmType(ctx, ctx.ctx.typeInt), 0, true), LLVMConstPointerNull(CompileLlvmType(ctx, ctx.ctx.typeNullPtr)) };

		return CheckType(ctx, node, LLVMConstNamedStruct(CompileLlvmType(ctx, node->type), constants, 2));
	}
	case EXPR_CAST_NULL_TO_UNSIZED:
	{
		LLVMValueRef constants[] = { LLVMConstPointerNull(CompileLlvmType(ctx, ctx.ctx.typeNullPtr)), LLVMConstInt(CompileLlvmType(ctx, ctx.ctx.typeInt), 0, true) };

		return CheckType(ctx, node, LLVMConstNamedStruct(CompileLlvmType(ctx, node->type), constants, 2));
	}
	case EXPR_CAST_NULL_TO_AUTO_ARRAY:
	{
		LLVMValueRef constants[] = { LLVMConstInt(CompileLlvmType(ctx, ctx.ctx.typeInt), 0, true), LLVMConstPointerNull(CompileLlvmType(ctx, ctx.ctx.typeNullPtr)), LLVMConstInt(CompileLlvmType(ctx, ctx.ctx.typeInt), 0, true) };

		return CheckType(ctx, node, LLVMConstNamedStruct(CompileLlvmType(ctx, node->type), constants, 3));
	}
	case EXPR_CAST_NULL_TO_FUNCTION:
	{
		// TODO: replace with a function index
		LLVMValueRef constants[] = { LLVMConstPointerNull(CompileLlvmType(ctx, ctx.ctx.typeNullPtr)), LLVMConstPointerNull(CompileLlvmType(ctx, ctx.ctx.typeNullPtr)) };

		return CheckType(ctx, node, LLVMConstNamedStruct(CompileLlvmType(ctx, node->type), constants, 2));
	}
	case EXPR_CAST_ARRAY_PTR_TO_UNSIZED:
		if(TypeRef *refType = getType<TypeRef>(node->value->type))
		{
			TypeArray *arrType = getType<TypeArray>(refType->subType);

			assert(arrType);
			assert(unsigned(arrType->length) == arrType->length);

			LLVMValueRef constants[] = { LLVMConstPointerNull(LLVMPointerType(CompileLlvmType(ctx, arrType->subType), 0)), LLVMConstInt(CompileLlvmType(ctx, ctx.ctx.typeInt), unsigned(arrType->length), true) };

			LLVMValueRef result = LLVMConstNamedStruct(CompileLlvmType(ctx, node->type), constants, 2);

			LLVMValueRef ptr = LLVMBuildPointerCast(ctx.builder, value, LLVMPointerType(CompileLlvmType(ctx, arrType->subType), 0), "arr_to_unsized");

			result = LLVMBuildInsertValue(ctx.builder, result, ptr, 0, "");

			return CheckType(ctx, node, result);
		}

		break;
	case EXPR_CAST_PTR_TO_AUTO_PTR:
		if(TypeRef *refType = getType<TypeRef>(node->value->type))
		{
			TypeClass *classType = getType<TypeClass>(refType->subType);

			LLVMValueRef constants[] = { LLVMConstPointerNull(CompileLlvmType(ctx, ctx.ctx.typeNullPtr)), LLVMConstPointerNull(CompileLlvmType(ctx, ctx.ctx.typeNullPtr)) };

			LLVMValueRef result = LLVMConstNamedStruct(CompileLlvmType(ctx, node->type), constants, 2);

			if(classType && (classType->extendable || classType->baseClass))
			{
				LLVMValueRef indices[] = { LLVMConstInt(LLVMInt32TypeInContext(ctx.context), 0, true), LLVMConstInt(LLVMInt32TypeInContext(ctx.context), 0, true) };

				result = LLVMBuildInsertValue(ctx.builder, result, LLVMBuildLoad(ctx.builder, LLVMBuildGEP(ctx.builder, value, indices, 2, ""), "$typeid"), 0, "");
			}
			else
			{
				// TODO: use global type index values for a later remap
				result = LLVMBuildInsertValue(ctx.builder, result, LLVMConstInt(CompileLlvmType(ctx, ctx.ctx.typeTypeID), refType->subType->typeIndex, true), 0, "");
			}

			result = LLVMBuildInsertValue(ctx.builder, result, LLVMBuildPointerCast(ctx.builder, value, CompileLlvmType(ctx, ctx.ctx.typeNullPtr), "ptr_to_auto_ptr"), 1, "");

			return CheckType(ctx, node, result);
		}
		break;
	case EXPR_CAST_AUTO_PTR_TO_PTR:
		if(TypeRef *refType = getType<TypeRef>(node->type))
		{
			// TODO: use global type index values for a later remap
			SmallArray<LLVMValueRef, 2> arguments;

			arguments.push_back(CompileArgument(ctx, ctx.ctx.typeAutoRef, value));
			arguments.push_back(CompileArgument(ctx, ctx.ctx.typeTypeID, LLVMConstInt(CompileLlvmType(ctx, ctx.ctx.typeTypeID), refType->subType->typeIndex, true)));

			LLVMValueRef result = LLVMBuildCall(ctx.builder, LLVMGetNamedFunction(ctx.module, "__llvmConvertPtr"), arguments.data, arguments.count, "");

			return CheckType(ctx, node, LLVMBuildPointerCast(ctx.builder, result, CompileLlvmType(ctx, node->type), "auto_ptr_to_ptr"));
		}

		break;
	case EXPR_CAST_UNSIZED_TO_AUTO_ARRAY:
		if(TypeUnsizedArray *unsizedType = getType<TypeUnsizedArray>(node->value->type))
		{
			LLVMValueRef ptr = LLVMBuildExtractValue(ctx.builder, value, 0, "arr_ptr");
			LLVMValueRef size = LLVMBuildExtractValue(ctx.builder, value, 1, "arr_size");

			LLVMValueRef constants[] = { LLVMConstInt(CompileLlvmType(ctx, ctx.ctx.typeTypeID), 0, true), LLVMConstPointerNull(CompileLlvmType(ctx, ctx.ctx.typeNullPtr)), LLVMConstInt(CompileLlvmType(ctx, ctx.ctx.typeInt), 0, true) };

			LLVMValueRef result = LLVMConstNamedStruct(CompileLlvmType(ctx, node->type), constants, 3);

			// TODO: use global type index values for a later remap
			result = LLVMBuildInsertValue(ctx.builder, result, LLVMConstInt(CompileLlvmType(ctx, ctx.ctx.typeTypeID), unsizedType->subType->typeIndex, true), 0, "");
			result = LLVMBuildInsertValue(ctx.builder, result, LLVMBuildPointerCast(ctx.builder, ptr, CompileLlvmType(ctx, ctx.ctx.typeNullPtr), "unsized_to_auto_array"), 1, "");
			result = LLVMBuildInsertValue(ctx.builder, result, size, 2, "");

			return CheckType(ctx, node, result);
		}

		break;
	case EXPR_CAST_REINTERPRET:
		if(node->type == node->value->type)
			return CheckType(ctx, node, value);

		if(isType<TypeUnsizedArray>(node->type) && isType<TypeUnsizedArray>(node->value->type))
		{
			TypeUnsizedArray *typeUnsizedArray = getType<TypeUnsizedArray>(node->type);

			LLVMTypeRef targetPointerType = LLVMPointerType(CompileLlvmType(ctx, typeUnsizedArray->subType), 0);

			LLVMValueRef constants[] = { LLVMConstPointerNull(CompileLlvmType(ctx, ctx.ctx.typeNullPtr)), LLVMConstInt(CompileLlvmType(ctx, ctx.ctx.typeInt), 0, true) };

			LLVMValueRef result = LLVMConstNamedStruct(CompileLlvmType(ctx, node->type), constants, 2);

			result = LLVMBuildInsertValue(ctx.builder, result, LLVMBuildPointerCast(ctx.builder, LLVMBuildExtractValue(ctx.builder, value, 0, ""), targetPointerType, "unsized_reinterpret"), 0, "");
			result = LLVMBuildInsertValue(ctx.builder, result, LLVMBuildExtractValue(ctx.builder, value, 1, ""), 1, "");

			return CheckType(ctx, node, result);
		}
		else if(isType<TypeRef>(node->type) && isType<TypeRef>(node->value->type))
		{
			return CheckType(ctx, node, LLVMBuildPointerCast(ctx.builder, value, CompileLlvmType(ctx, node->type), "ref_reinterpret"));
		}
		else if(isType<TypeFunction>(node->type) && isType<TypeFunction>(node->value->type))
		{
			return CheckType(ctx, node, value);
		}
		else if(node->type == ctx.ctx.typeInt && node->value->type == ctx.ctx.typeTypeID)
		{
			return CheckType(ctx, node, value);
		}
		else if(node->type == ctx.ctx.typeInt && isType<TypeEnum>(node->value->type))
		{
			return CheckType(ctx, node, value);
		}
		else if(isType<TypeEnum>(node->type) && node->value->type == ctx.ctx.typeInt)
		{
			return CheckType(ctx, node, value);
		}

		assert(!"unknown cast");

		return CheckType(ctx, node, value);
	default:
		assert(!"unknown cast");
	}

	return CheckType(ctx, node, value);
}

LLVMValueRef CompileLlvmUnaryOp(LlvmCompilationContext &ctx, ExprUnaryOp *node)
{
	LLVMValueRef value = CompileLlvm(ctx, node->value);

	LLVMValueRef result = NULL;

	switch(node->op)
	{
	case SYN_UNARY_OP_UNKNOWN:
		break;
	case SYN_UNARY_OP_PLUS:
		result = value;
		break;
	case SYN_UNARY_OP_NEGATE:
		if(ctx.ctx.IsIntegerType(node->value->type))
			result = LLVMBuildNeg(ctx.builder, value, "");
		else
			result = LLVMBuildFNeg(ctx.builder, value, "");
		break;
	case SYN_UNARY_OP_BIT_NOT:
		result = LLVMBuildNot(ctx.builder, value, "");
		break;
	case SYN_UNARY_OP_LOGICAL_NOT:
		if(node->value->type == ctx.ctx.typeAutoRef)
		{
			LLVMValueRef ptr = LLVMBuildExtractValue(ctx.builder, value, 1, "ref_ptr");

			LLVMValueRef rhs = LLVMConstPointerNull(CompileLlvmType(ctx, ctx.ctx.typeNullPtr));

			result = LLVMBuildZExt(ctx.builder, LLVMBuildICmp(ctx.builder, LLVMIntEQ, ptr, rhs, ""), CompileLlvmType(ctx, ctx.ctx.typeInt), "");
		}
		else if(isType<TypeRef>(node->value->type))
		{
			LLVMValueRef rhs = LLVMConstPointerNull(CompileLlvmType(ctx, GetStackType(ctx, node->value->type)));

			result = LLVMBuildZExt(ctx.builder, LLVMBuildICmp(ctx.builder, LLVMIntEQ, value, rhs, ""), CompileLlvmType(ctx, ctx.ctx.typeInt), "");
		}
		else
		{
			LLVMValueRef rhs = LLVMConstInt(CompileLlvmType(ctx, GetStackType(ctx, node->value->type)), 0, true);

			result = LLVMBuildZExt(ctx.builder, LLVMBuildICmp(ctx.builder, LLVMIntEQ, value, rhs, ""), CompileLlvmType(ctx, ctx.ctx.typeInt), "");
		}
		break;
	}

	assert(result);

	return CheckType(ctx, node, result);
}

LLVMValueRef CompileLlvmBinaryOp(LlvmCompilationContext &ctx, ExprBinaryOp *node)
{
	LLVMValueRef lhs = CompileLlvm(ctx, node->lhs);

	if(node->op == SYN_BINARY_OP_LOGICAL_AND)
	{
		LLVMBasicBlockRef checkRhsBlock = LLVMAppendBasicBlockInContext(ctx.context, ctx.currentFunction, "land_check_rhs");
		LLVMBasicBlockRef storeOneBlock = LLVMAppendBasicBlockInContext(ctx.context, ctx.currentFunction, "land_store_1");
		LLVMBasicBlockRef storeZeroBlock = LLVMAppendBasicBlockInContext(ctx.context, ctx.currentFunction, "land_store_0");
		LLVMBasicBlockRef exitBlock = LLVMAppendBasicBlockInContext(ctx.context, ctx.currentFunction, "land_exit");

		lhs = LLVMBuildICmp(ctx.builder, LLVMIntNE, lhs, LLVMConstInt(CompileLlvmType(ctx, GetStackType(ctx, node->lhs->type)), 0, true), "");

		LLVMBuildCondBr(ctx.builder, lhs, checkRhsBlock, storeZeroBlock);

		LLVMPositionBuilderAtEnd(ctx.builder, checkRhsBlock);

		LLVMValueRef rhs = CompileLlvm(ctx, node->rhs);

		rhs = LLVMBuildICmp(ctx.builder, LLVMIntNE, rhs, LLVMConstInt(CompileLlvmType(ctx, GetStackType(ctx, node->rhs->type)), 0, true), "");

		LLVMBuildCondBr(ctx.builder, lhs, storeOneBlock, storeZeroBlock);

		LLVMPositionBuilderAtEnd(ctx.builder, storeOneBlock);

		LLVMBuildBr(ctx.builder, exitBlock);

		LLVMPositionBuilderAtEnd(ctx.builder, storeZeroBlock);

		LLVMBuildBr(ctx.builder, exitBlock);

		LLVMPositionBuilderAtEnd(ctx.builder, exitBlock);

		LLVMValueRef phi = LLVMBuildPhi(ctx.builder, CompileLlvmType(ctx, ctx.ctx.typeInt), "land");

		LLVMValueRef values[] = { LLVMConstInt(CompileLlvmType(ctx, ctx.ctx.typeInt), 1, true), LLVMConstInt(CompileLlvmType(ctx, ctx.ctx.typeInt), 0, true) };
		LLVMBasicBlockRef blocks[] = { storeOneBlock, storeZeroBlock };

		LLVMAddIncoming(phi, values, blocks, 2);

		return CheckType(ctx, node, phi);
	}

	if(node->op == SYN_BINARY_OP_LOGICAL_OR)
	{
		LLVMBasicBlockRef checkRhsBlock = LLVMAppendBasicBlockInContext(ctx.context, ctx.currentFunction, "lor_check_rhs");
		LLVMBasicBlockRef storeOneBlock = LLVMAppendBasicBlockInContext(ctx.context, ctx.currentFunction, "lor_store_1");
		LLVMBasicBlockRef storeZeroBlock = LLVMAppendBasicBlockInContext(ctx.context, ctx.currentFunction, "lor_store_0");
		LLVMBasicBlockRef exitBlock = LLVMAppendBasicBlockInContext(ctx.context, ctx.currentFunction, "lor_exit");

		lhs = LLVMBuildICmp(ctx.builder, LLVMIntNE, lhs, LLVMConstInt(CompileLlvmType(ctx, GetStackType(ctx, node->lhs->type)), 0, true), "");

		LLVMBuildCondBr(ctx.builder, lhs, storeOneBlock, checkRhsBlock);

		LLVMPositionBuilderAtEnd(ctx.builder, checkRhsBlock);

		LLVMValueRef rhs = CompileLlvm(ctx, node->rhs);

		rhs = LLVMBuildICmp(ctx.builder, LLVMIntNE, rhs, LLVMConstInt(CompileLlvmType(ctx, GetStackType(ctx, node->rhs->type)), 0, true), "");

		LLVMBuildCondBr(ctx.builder, lhs, storeOneBlock, storeZeroBlock);

		LLVMPositionBuilderAtEnd(ctx.builder, storeOneBlock);

		LLVMBuildBr(ctx.builder, exitBlock);

		LLVMPositionBuilderAtEnd(ctx.builder, storeZeroBlock);

		LLVMBuildBr(ctx.builder, exitBlock);

		LLVMPositionBuilderAtEnd(ctx.builder, exitBlock);

		LLVMValueRef phi = LLVMBuildPhi(ctx.builder, CompileLlvmType(ctx, ctx.ctx.typeInt), "lor");

		LLVMValueRef values[] = { LLVMConstInt(CompileLlvmType(ctx, ctx.ctx.typeInt), 1, true), LLVMConstInt(CompileLlvmType(ctx, ctx.ctx.typeInt), 0, true) };
		LLVMBasicBlockRef blocks[] = { storeOneBlock, storeZeroBlock };

		LLVMAddIncoming(phi, values, blocks, 2);

		return CheckType(ctx, node, phi);
	}

	LLVMValueRef rhs = CompileLlvm(ctx, node->rhs);

	LLVMValueRef result = NULL;

	switch(node->op)
	{
	case SYN_BINARY_OP_ADD:
		if(ctx.ctx.IsIntegerType(node->lhs->type) || isType<TypeEnum>(node->lhs->type))
			result = LLVMBuildAdd(ctx.builder, lhs, rhs, "");
		else
			result = LLVMBuildFAdd(ctx.builder, lhs, rhs, "");
		break;
	case SYN_BINARY_OP_SUB:
		if(ctx.ctx.IsIntegerType(node->lhs->type) || isType<TypeEnum>(node->lhs->type))
			result = LLVMBuildSub(ctx.builder, lhs, rhs, "");
		else
			result = LLVMBuildFSub(ctx.builder, lhs, rhs, "");
		break;
	case SYN_BINARY_OP_MUL:
		if(ctx.ctx.IsIntegerType(node->lhs->type) || isType<TypeEnum>(node->lhs->type))
			result = LLVMBuildMul(ctx.builder, lhs, rhs, "");
		else
			result = LLVMBuildFMul(ctx.builder, lhs, rhs, "");
		break;
	case SYN_BINARY_OP_DIV:
		if(ctx.ctx.IsIntegerType(node->lhs->type) || isType<TypeEnum>(node->lhs->type))
			result = LLVMBuildSDiv(ctx.builder, lhs, rhs, "");
		else
			result = LLVMBuildFDiv(ctx.builder, lhs, rhs, "");
		break;
	case SYN_BINARY_OP_MOD:
		if(ctx.ctx.IsIntegerType(node->lhs->type) || isType<TypeEnum>(node->lhs->type))
			result = LLVMBuildSRem(ctx.builder, lhs, rhs, "");
		else
			result = LLVMBuildFRem(ctx.builder, lhs, rhs, "");
		break;
	case SYN_BINARY_OP_POW:
		if(GetStackType(ctx, node->type) == ctx.ctx.typeInt || isType<TypeEnum>(node->type))
		{
			LLVMValueRef arguments[] = { lhs, rhs };

			result = LLVMBuildCall(ctx.builder, LLVMGetNamedFunction(ctx.module, "__llvmPowInt"), arguments, 2, "");
		}
		else if(GetStackType(ctx, node->type) == ctx.ctx.typeLong)
		{
			LLVMValueRef arguments[] = { lhs, rhs };

			result = LLVMBuildCall(ctx.builder, LLVMGetNamedFunction(ctx.module, "__llvmPowLong"), arguments, 2, "");
		}
		else if(GetStackType(ctx, node->type) == ctx.ctx.typeDouble)
		{
			LLVMValueRef arguments[] = { lhs, rhs };

			result = LLVMBuildCall(ctx.builder, LLVMGetNamedFunction(ctx.module, "__llvmPowDouble"), arguments, 2, "");
		}
		else
		{
			assert(!"unknown operand type");
		}
		break;
	case SYN_BINARY_OP_SHL:
		result = LLVMBuildShl(ctx.builder, lhs, rhs, "");
		break;
	case SYN_BINARY_OP_SHR:
		result = LLVMBuildAShr(ctx.builder, lhs, rhs, "");
		break;
	case SYN_BINARY_OP_LESS:
		if(ctx.ctx.IsIntegerType(node->lhs->type) || isType<TypeEnum>(node->lhs->type))
			result = LLVMBuildZExt(ctx.builder, LLVMBuildICmp(ctx.builder, LLVMIntSLT, lhs, rhs, ""), CompileLlvmType(ctx, ctx.ctx.typeInt), "");
		else
			result = LLVMBuildZExt(ctx.builder, LLVMBuildFCmp(ctx.builder, LLVMRealULT, lhs, rhs, ""), CompileLlvmType(ctx, ctx.ctx.typeInt), "");
		break;
	case SYN_BINARY_OP_LESS_EQUAL:
		if(ctx.ctx.IsIntegerType(node->lhs->type) || isType<TypeEnum>(node->lhs->type))
			result = LLVMBuildZExt(ctx.builder, LLVMBuildICmp(ctx.builder, LLVMIntSLE, lhs, rhs, ""), CompileLlvmType(ctx, ctx.ctx.typeInt), "");
		else
			result = LLVMBuildZExt(ctx.builder, LLVMBuildFCmp(ctx.builder, LLVMRealULE, lhs, rhs, ""), CompileLlvmType(ctx, ctx.ctx.typeInt), "");
		break;
	case SYN_BINARY_OP_GREATER:
		if(ctx.ctx.IsIntegerType(node->lhs->type) || isType<TypeEnum>(node->lhs->type))
			result = LLVMBuildZExt(ctx.builder, LLVMBuildICmp(ctx.builder, LLVMIntSGT, lhs, rhs, ""), CompileLlvmType(ctx, ctx.ctx.typeInt), "");
		else
			result = LLVMBuildZExt(ctx.builder, LLVMBuildFCmp(ctx.builder, LLVMRealUGT, lhs, rhs, ""), CompileLlvmType(ctx, ctx.ctx.typeInt), "");
		break;
	case SYN_BINARY_OP_GREATER_EQUAL:
		if(ctx.ctx.IsIntegerType(node->lhs->type) || isType<TypeEnum>(node->lhs->type))
			result = LLVMBuildZExt(ctx.builder, LLVMBuildICmp(ctx.builder, LLVMIntSGE, lhs, rhs, ""), CompileLlvmType(ctx, ctx.ctx.typeInt), "");
		else
			result = LLVMBuildZExt(ctx.builder, LLVMBuildFCmp(ctx.builder, LLVMRealUGE, lhs, rhs, ""), CompileLlvmType(ctx, ctx.ctx.typeInt), "");
		break;
	case SYN_BINARY_OP_EQUAL:
		if(isType<TypeRef>(node->lhs->type) || ctx.ctx.IsIntegerType(node->lhs->type) || isType<TypeEnum>(node->lhs->type) || node->lhs->type == ctx.ctx.typeTypeID)
			result = LLVMBuildZExt(ctx.builder, LLVMBuildICmp(ctx.builder, LLVMIntEQ, lhs, rhs, ""), CompileLlvmType(ctx, ctx.ctx.typeInt), "");
		else
			result = LLVMBuildZExt(ctx.builder, LLVMBuildFCmp(ctx.builder, LLVMRealUEQ, lhs, rhs, ""), CompileLlvmType(ctx, ctx.ctx.typeInt), "");
		break;
	case SYN_BINARY_OP_NOT_EQUAL:
		if(isType<TypeRef>(node->lhs->type) || ctx.ctx.IsIntegerType(node->lhs->type) || isType<TypeEnum>(node->lhs->type) || node->lhs->type == ctx.ctx.typeTypeID)
			result = LLVMBuildZExt(ctx.builder, LLVMBuildICmp(ctx.builder, LLVMIntNE, lhs, rhs, ""), CompileLlvmType(ctx, ctx.ctx.typeInt), "");
		else
			result = LLVMBuildZExt(ctx.builder, LLVMBuildFCmp(ctx.builder, LLVMRealUNE, lhs, rhs, ""), CompileLlvmType(ctx, ctx.ctx.typeInt), "");
		break;
	case SYN_BINARY_OP_BIT_AND:
		result = LLVMBuildAnd(ctx.builder, lhs, rhs, "");
		break;
	case SYN_BINARY_OP_BIT_OR:
		result = LLVMBuildOr(ctx.builder, lhs, rhs, "");
		break;
	case SYN_BINARY_OP_BIT_XOR:
		result = LLVMBuildXor(ctx.builder, lhs, rhs, "");
		break;
	case SYN_BINARY_OP_LOGICAL_XOR:
		// (lhs != 0) != (rhs != 0)
		lhs = LLVMBuildICmp(ctx.builder, LLVMIntNE, lhs, LLVMConstInt(CompileLlvmType(ctx, GetStackType(ctx, node->lhs->type)), 0, true), "");
		rhs = LLVMBuildICmp(ctx.builder, LLVMIntNE, rhs, LLVMConstInt(CompileLlvmType(ctx, GetStackType(ctx, node->rhs->type)), 0, true), "");

		result = LLVMBuildZExt(ctx.builder, LLVMBuildICmp(ctx.builder, LLVMIntNE, lhs, rhs, ""), CompileLlvmType(ctx, ctx.ctx.typeInt), "");
		break;
	default:
		break;
	}

	assert(result);

	return CheckType(ctx, node, result);
}

LLVMValueRef CompileLlvmGetAddress(LlvmCompilationContext &ctx, ExprGetAddress *node)
{
	LLVMValueRef *value = ctx.variables.find(node->variable->variable->uniqueId);

	assert(value);

	return CheckType(ctx, node, *value);
}

LLVMValueRef CompileLlvmDereference(LlvmCompilationContext &ctx, ExprDereference *node)
{
	LLVMValueRef value = CompileLlvm(ctx, node->value);

	return CheckType(ctx, node, ConvertToStackType(ctx, LLVMBuildLoad(ctx.builder, value, ""), node->type));
}

LLVMValueRef CompileLlvmUnboxing(LlvmCompilationContext &ctx, ExprUnboxing *node)
{
	LLVMValueRef value = CompileLlvm(ctx, node->value);

	return CheckType(ctx, node, value);
}

LLVMValueRef CompileLlvmConditional(LlvmCompilationContext &ctx, ExprConditional *node)
{
	LLVMValueRef condition = CompileLlvm(ctx, node->condition);

	condition = LLVMBuildICmp(ctx.builder, LLVMIntNE, condition, LLVMConstInt(CompileLlvmType(ctx, GetStackType(ctx, node->condition->type)), 0, true), "");

	LLVMBasicBlockRef trueBlock = LLVMAppendBasicBlockInContext(ctx.context, ctx.currentFunction, "cond_true");
	LLVMBasicBlockRef falseBlock = LLVMAppendBasicBlockInContext(ctx.context, ctx.currentFunction, "cond_false");
	LLVMBasicBlockRef exitBlock = LLVMAppendBasicBlockInContext(ctx.context, ctx.currentFunction, "cond_exit");

	LLVMValueRef result = LLVMBuildAlloca(ctx.builder, CompileLlvmType(ctx, node->type), "cond");

	LLVMBuildCondBr(ctx.builder, condition, trueBlock, falseBlock);

	LLVMPositionBuilderAtEnd(ctx.builder, trueBlock);

	LLVMBuildStore(ctx.builder, ConvertToDataType(ctx, CompileLlvm(ctx, node->trueBlock), GetStackType(ctx, node->trueBlock->type), node->trueBlock->type), result);

	LLVMBuildBr(ctx.builder, exitBlock);

	LLVMPositionBuilderAtEnd(ctx.builder, falseBlock);

	LLVMBuildStore(ctx.builder, ConvertToDataType(ctx, CompileLlvm(ctx, node->falseBlock), GetStackType(ctx, node->falseBlock->type), node->falseBlock->type), result);

	LLVMBuildBr(ctx.builder, exitBlock);

	LLVMPositionBuilderAtEnd(ctx.builder, exitBlock);

	return CheckType(ctx, node, ConvertToStackType(ctx, LLVMBuildLoad(ctx.builder, result, ""), node->type));
}

LLVMValueRef CompileLlvmAssignment(LlvmCompilationContext &ctx, ExprAssignment *node)
{
	LLVMValueRef address = CompileLlvm(ctx, node->lhs);

	LLVMValueRef initializer = CompileLlvm(ctx, node->rhs);

	LLVMBuildStore(ctx.builder, ConvertToDataType(ctx, initializer, GetStackType(ctx, node->rhs->type), node->rhs->type), address);

	return CheckType(ctx, node, initializer);
}

LLVMValueRef CompileLlvmMemberAccess(LlvmCompilationContext &ctx, ExprMemberAccess *node)
{
	LLVMValueRef value = CompileLlvm(ctx, node->value);

	TypeRef *typeRef = getType<TypeRef>(node->value->type);
	TypeStruct *typeStruct = getType<TypeStruct>(typeRef->subType);

	unsigned memberIndex = ~0u;

	unsigned currMember = 0;

	// Unsized array types have a hidden member (pointer) at index 0
	if(isType<TypeUnsizedArray>(typeStruct))
		currMember++;

	for(MemberHandle *curr = typeStruct->members.head; curr; curr = curr->next)
	{
		if(curr->variable == node->member->variable)
		{
			memberIndex = currMember;
			break;
		}

		currMember++;
	}

	assert(memberIndex != ~0u);

	LLVMValueRef indices[] = { LLVMConstInt(LLVMInt32TypeInContext(ctx.context), 0, true), LLVMConstInt(LLVMInt32TypeInContext(ctx.context), memberIndex, true) };

	return CheckType(ctx, node, LLVMBuildGEP(ctx.builder, value, indices, 2, ""));
}

LLVMValueRef CompileLlvmArrayIndex(LlvmCompilationContext &ctx, ExprArrayIndex *node)
{
	LLVMValueRef value = CompileLlvm(ctx, node->value);
	LLVMValueRef index = CompileLlvm(ctx, node->index);

	if(isType<TypeUnsizedArray>(node->value->type))
	{
		// TODO: bounds checking
		LLVMValueRef start = LLVMBuildExtractValue(ctx.builder, value, 0, "arr_ptr");

		LLVMValueRef indices[] = { index };

		return CheckType(ctx, node, LLVMBuildGEP(ctx.builder, start, indices, 1, ""));
	}

	// TODO: bounds checking
	LLVMValueRef indices[] = { LLVMConstInt(LLVMInt32TypeInContext(ctx.context), 0, true), index };

	return CheckType(ctx, node, LLVMBuildGEP(ctx.builder, value, indices, 2, ""));
}

LLVMValueRef CompileLlvmReturn(LlvmCompilationContext &ctx, ExprReturn *node)
{
	LLVMValueRef value = CompileLlvm(ctx, node->value);

	if(node->coroutineStateUpdate)
		CompileLlvm(ctx, node->coroutineStateUpdate);

	if(node->closures)
		CompileLlvm(ctx, node->closures);

	if(node->value->type == ctx.ctx.typeVoid)
	{
		assert(!ctx.currentFunctionGlobal);

		LLVMBuildRetVoid(ctx.builder);
	}
	else
	{
		if(ctx.currentFunctionGlobal)
		{
			if(GetStackType(ctx, node->value->type) == ctx.ctx.typeInt || isType<TypeEnum>(node->value->type))
			{
				LLVMValueRef arguments[] = { value };

				LLVMBuildCall(ctx.builder, LLVMGetNamedFunction(ctx.module, "__llvmReturnInt"), arguments, 1, "");

				LLVMBuildRetVoid(ctx.builder);
			}
			else if(GetStackType(ctx, node->value->type) == ctx.ctx.typeLong)
			{
				LLVMValueRef arguments[] = { value };

				LLVMBuildCall(ctx.builder, LLVMGetNamedFunction(ctx.module, "__llvmReturnLong"), arguments, 1, "");

				LLVMBuildRetVoid(ctx.builder);
			}
			else if(GetStackType(ctx, node->value->type) == ctx.ctx.typeDouble)
			{
				LLVMValueRef arguments[] = { value };

				LLVMBuildCall(ctx.builder, LLVMGetNamedFunction(ctx.module, "__llvmReturnDouble"), arguments, 1, "");

				LLVMBuildRetVoid(ctx.builder);
			}
			else
			{
				assert(!"unknown global return type");
			}
		}
		else
		{
			value = ConvertToDataType(ctx, value, GetStackType(ctx, node->value->type), node->value->type);

			if(IsStructReturnType(ctx.currentFunctionSource->type->returnType))
			{
				LLVMBuildStore(ctx.builder, value, LLVMGetParam(ctx.currentFunction, 0));

				LLVMBuildRetVoid(ctx.builder);
			}
			else
			{
				LLVMBuildRet(ctx.builder, value);
			}
		}
	}

	LLVMBasicBlockRef afterReturn = LLVMAppendBasicBlockInContext(ctx.context, ctx.currentFunction, "after_return");

	LLVMPositionBuilderAtEnd(ctx.builder, afterReturn);

	return CheckType(ctx, node, NULL);
}

LLVMValueRef CompileLlvmYield(LlvmCompilationContext &ctx, ExprYield *node)
{
	LLVMValueRef value = CompileLlvm(ctx, node->value);

	if(node->coroutineStateUpdate)
		CompileLlvm(ctx, node->coroutineStateUpdate);

	if(node->closures)
		CompileLlvm(ctx, node->closures);

	LLVMBasicBlockRef block = ctx.currentRestoreBlocks[ctx.currentNextRestoreBlock++];

	if(node->value->type == ctx.ctx.typeVoid)
	{
		LLVMBuildRetVoid(ctx.builder);
	}
	else
	{
		value = ConvertToDataType(ctx, value, GetStackType(ctx, node->value->type), node->value->type);

		if(IsStructReturnType(ctx.currentFunctionSource->type->returnType))
		{
			LLVMBuildStore(ctx.builder, value, LLVMGetParam(ctx.currentFunction, 0));

			LLVMBuildRetVoid(ctx.builder);
		}
		else
		{
			LLVMBuildRet(ctx.builder, value);
		}
	}

	LLVMPositionBuilderAtEnd(ctx.builder, block);

	return CheckType(ctx, node, NULL);
}

LLVMValueRef CompileLlvmVariableDefinition(LlvmCompilationContext &ctx, ExprVariableDefinition *node)
{
	VariableData *variable = node->variable->variable;

	LLVMValueRef storage = LLVMBuildAlloca(ctx.builder, CompileLlvmType(ctx, variable->type), CreateLlvmName(ctx, variable->name->name));

	if(!ctx.variables.find(variable->uniqueId))
		ctx.variables.insert(variable->uniqueId, storage);
	else if(!ctx.currentFunctionGlobal)
		assert(!"duplicate variable definition");

	if(node->initializer)
		CompileLlvm(ctx, node->initializer);

	return CheckType(ctx, node, NULL);
}

LLVMValueRef CompileLlvmArraySetup(LlvmCompilationContext &ctx, ExprArraySetup *node)
{
	TypeRef *refType = getType<TypeRef>(node->lhs->type);

	assert(refType);

	TypeArray *arrayType = getType<TypeArray>(refType->subType);

	assert(arrayType);

	LLVMValueRef initializer = CompileLlvm(ctx, node->initializer);

	initializer = ConvertToDataType(ctx, initializer, GetStackType(ctx, node->initializer->type), node->initializer->type);

	LLVMValueRef address = CompileLlvm(ctx, node->lhs);

	LLVMValueRef offsetPtr = LLVMBuildAlloca(ctx.builder, CompileLlvmType(ctx, ctx.ctx.typeInt), "arr_it");

	LLVMBasicBlockRef conditionBlock = LLVMAppendBasicBlockInContext(ctx.context, ctx.currentFunction, "arr_setup_cond");
	LLVMBasicBlockRef bodyBlock = LLVMAppendBasicBlockInContext(ctx.context, ctx.currentFunction, "arr_setup_body");
	LLVMBasicBlockRef exitBlock = LLVMAppendBasicBlockInContext(ctx.context, ctx.currentFunction, "arr_setup_exit");

	LLVMBuildStore(ctx.builder, LLVMConstInt(CompileLlvmType(ctx, ctx.ctx.typeInt), 0, true), offsetPtr);

	LLVMBuildBr(ctx.builder, conditionBlock);

	LLVMPositionBuilderAtEnd(ctx.builder, conditionBlock);

	// Offset will move in element size steps, so it will reach the full size of the array
	assert(int(arrayType->length * arrayType->subType->size) == arrayType->length * arrayType->subType->size);

	// While offset is less than array size
	LLVMValueRef condition = LLVMBuildICmp(ctx.builder, LLVMIntSLT, LLVMBuildLoad(ctx.builder, offsetPtr, ""), LLVMConstInt(CompileLlvmType(ctx, ctx.ctx.typeInt), int(arrayType->length), true), "");

	LLVMBuildCondBr(ctx.builder, condition, bodyBlock, exitBlock);

	LLVMPositionBuilderAtEnd(ctx.builder, bodyBlock);

	LLVMValueRef offset = LLVMBuildLoad(ctx.builder, offsetPtr, "");

	LLVMValueRef indices[] = { LLVMConstInt(LLVMInt32TypeInContext(ctx.context), 0, true), offset };

	LLVMBuildStore(ctx.builder, initializer, LLVMBuildGEP(ctx.builder, address, indices, 2, ""));
	LLVMBuildStore(ctx.builder, LLVMBuildAdd(ctx.builder, offset, LLVMConstInt(CompileLlvmType(ctx, ctx.ctx.typeInt), 1, true), ""), offsetPtr);

	LLVMBuildBr(ctx.builder, conditionBlock);

	LLVMPositionBuilderAtEnd(ctx.builder, exitBlock);

	return CheckType(ctx, node, NULL);
}

LLVMValueRef CompileLlvmVariableDefinitions(LlvmCompilationContext &ctx, ExprVariableDefinitions *node)
{
	for(ExprBase *value = node->definitions.head; value; value = value->next)
		CompileLlvm(ctx, value);

	return CheckType(ctx, node, NULL);
}

LLVMValueRef CompileLlvmVariableAccess(LlvmCompilationContext &ctx, ExprVariableAccess *node)
{
	LLVMValueRef *value = ctx.variables.find(node->variable->uniqueId);

	assert(value);

	return CheckType(ctx, node, ConvertToStackType(ctx, LLVMBuildLoad(ctx.builder, *value, CreateLlvmName(ctx, node->variable->name->name)), node->variable->type));
}

LLVMValueRef CompileLlvmFunctionContextAccess(LlvmCompilationContext &ctx, ExprFunctionContextAccess *node)
{
	TypeRef *refType = getType<TypeRef>(node->function->contextType);

	assert(refType);

	TypeClass *classType = getType<TypeClass>(refType->subType);

	assert(classType);

	LLVMValueRef value = NULL;

	if(classType->size == 0)
	{
		value = LLVMConstPointerNull(CompileLlvmType(ctx, node->type));
	}
	else
	{
		LLVMValueRef *variable = ctx.variables.find(node->function->contextVariable->uniqueId);

		assert(variable);

		LLVMValueRef address = *variable;

		value = LLVMBuildLoad(ctx.builder, address, CreateLlvmName(ctx, node->function->contextVariable->name->name));
	}

	return CheckType(ctx, node, value);
}

LLVMValueRef CompileLlvmFunctionDefinition(LlvmCompilationContext &ctx, ExprFunctionDefinition *node)
{
	LLVMValueRef function = ctx.functions[node->function->functionIndex];

	if(ctx.skipFunctionDefinitions)
	{
		LLVMValueRef function = LLVMBuildPointerCast(ctx.builder, ctx.functions[node->function->functionIndex], CompileLlvmType(ctx, ctx.ctx.typeNullPtr), "func_ptr");

		LLVMValueRef constants[] = { LLVMConstPointerNull(CompileLlvmType(ctx, ctx.ctx.typeNullPtr)), LLVMConstPointerNull(CompileLlvmType(ctx, ctx.ctx.typeNullPtr)) };

		LLVMValueRef result = LLVMConstNamedStruct(CompileLlvmType(ctx, node->type), constants, 2);

		result = LLVMBuildInsertValue(ctx.builder, result, function, 1, "");

		return CheckType(ctx, node, result);
	}

	if(node->function->isPrototype)
		return NULL;

	ctx.skipFunctionDefinitions = true;

	// Store state
	LLVMValueRef currentFunction = ctx.currentFunction;
	FunctionData *currentFunctionSource = ctx.currentFunctionSource;

	// Switch to new function
	ctx.currentFunction = function;
	ctx.currentFunctionSource = node->function;

	assert(ctx.currentNextRestoreBlock == 0);
	assert(ctx.currentRestoreBlocks.empty());

	LLVMBasicBlockRef block = LLVMAppendBasicBlockInContext(ctx.context, ctx.currentFunction, "start");

	LLVMPositionBuilderAtEnd(ctx.builder, block);

	// Allocate all arguments
	unsigned argIndex = 0;

	if(IsStructReturnType(node->function->type->returnType))
		argIndex++;

	for(VariableHandle *curr = node->function->argumentVariables.head; curr; curr = curr->next)
	{
		VariableData *variable = curr->variable;

		if(IsStructArgumentType(variable->type))
		{
			LLVMValueRef argument = LLVMGetParam(function, argIndex++);

			assert(!ctx.variables.find(variable->uniqueId));

			ctx.variables.insert(variable->uniqueId, argument);
		}
		else
		{
			LLVMValueRef storage = LLVMBuildAlloca(ctx.builder, CompileLlvmType(ctx, variable->type), CreateLlvmName(ctx, variable->name->name));

			LLVMValueRef argument = LLVMGetParam(function, argIndex++);

			LLVMBuildStore(ctx.builder, argument, storage);

			assert(!ctx.variables.find(variable->uniqueId));

			ctx.variables.insert(variable->uniqueId, storage);
		}
	}

	if(VariableData *variable = node->function->contextArgument)
	{
		LLVMValueRef storage = LLVMBuildAlloca(ctx.builder, CompileLlvmType(ctx, variable->type), CreateLlvmName(ctx, variable->name->name));

		LLVMValueRef argument = LLVMGetParam(function, argIndex++);

		argument = LLVMBuildPointerCast(ctx.builder, argument, CompileLlvmType(ctx, node->function->contextType), "context_reinterpret");

		LLVMBuildStore(ctx.builder, argument, storage);

		assert(!ctx.variables.find(variable->uniqueId));

		ctx.variables.insert(variable->uniqueId, storage);
	}

	if(node->function->coroutine)
	{
		LLVMValueRef state = CompileLlvm(ctx, node->coroutineStateRead);

		LLVMBasicBlockRef startBlock = LLVMAppendBasicBlockInContext(ctx.context, ctx.currentFunction, "start");

		LLVMValueRef switchInst = LLVMBuildSwitch(ctx.builder, state, startBlock, node->function->yieldCount);

		LLVMPositionBuilderAtEnd(ctx.builder, startBlock);

		for(unsigned i = 0; i < node->function->yieldCount; i++)
		{
			LLVMBasicBlockRef restoreBlock = LLVMAppendBasicBlockInContext(ctx.context, ctx.currentFunction, "restore");

			LLVMAddCase(switchInst, LLVMConstInt(CompileLlvmType(ctx, ctx.ctx.typeInt), i + 1, true), restoreBlock);

			ctx.currentRestoreBlocks.push_back(restoreBlock);
		}
	}

	for(ExprBase *value = node->expressions.head; value; value = value->next)
		CompileLlvm(ctx, value);

	LLVMBuildCall(ctx.builder, LLVMGetNamedFunction(ctx.module, "__llvmAbortNoReturn"), NULL, 0, "");

	if(node->function->type->returnType == ctx.ctx.typeVoid || IsStructReturnType(node->function->type->returnType))
		LLVMBuildRetVoid(ctx.builder);
	else
		LLVMBuildRet(ctx.builder, LLVMConstNull(CompileLlvmType(ctx, node->function->type->returnType)));

	CheckFunction(ctx, ctx.currentFunction, node->function->name->name);

	if(LLVMRunFunctionPassManager(ctx.functionPassManager, function))
		LLVMRunFunctionPassManager(ctx.functionPassManager, function);

	// Restore state
	ctx.currentFunction = currentFunction;
	ctx.currentFunctionSource = currentFunctionSource;

	ctx.currentNextRestoreBlock = 0;
	ctx.currentRestoreBlocks.clear();

	ctx.skipFunctionDefinitions = false;

	return NULL;
}

LLVMValueRef CompileLlvmGenericFunctionPrototype(LlvmCompilationContext &ctx, ExprGenericFunctionPrototype *node)
{
	for(ExprBase *expr = node->contextVariables.head; expr; expr = expr->next)
		CompileLlvm(ctx, expr);

	return NULL;
}

LLVMValueRef CompileLlvmFunctionAccess(LlvmCompilationContext &ctx, ExprFunctionAccess *node)
{
	assert(ctx.functions[node->function->functionIndex]);

	LLVMValueRef context = node->context ? CompileLlvm(ctx, node->context) : LLVMConstPointerNull(CompileLlvmType(ctx, ctx.ctx.typeNullPtr));
	LLVMValueRef function = ctx.functions[node->function->functionIndex];

	context = LLVMBuildPointerCast(ctx.builder, context, CompileLlvmType(ctx, ctx.ctx.typeNullPtr), "context_ptr");
	function = LLVMBuildPointerCast(ctx.builder, function, CompileLlvmType(ctx, ctx.ctx.typeNullPtr), "func_ptr");

	LLVMValueRef constants[] = { LLVMConstPointerNull(CompileLlvmType(ctx, ctx.ctx.typeNullPtr)), LLVMConstPointerNull(CompileLlvmType(ctx, ctx.ctx.typeNullPtr)) };

	LLVMValueRef result = LLVMConstNamedStruct(CompileLlvmType(ctx, node->type), constants, 2);

	result = LLVMBuildInsertValue(ctx.builder, result, context, 0, "");
	result = LLVMBuildInsertValue(ctx.builder, result, function, 1, "");

	return CheckType(ctx, node, result);
}

LLVMValueRef CompileLlvmFunctionCall(LlvmCompilationContext &ctx, ExprFunctionCall *node)
{
	TypeFunction *typeFunction = getType<TypeFunction>(node->function->type);

	LLVMValueRef function = CompileLlvm(ctx, node->function);

	SmallArray<LLVMValueRef, 32> arguments(ctx.allocator);

	bool isStructReturnType = IsStructReturnType(typeFunction->returnType);
	LLVMValueRef resultStorage = NULL;

	if(isStructReturnType)
	{
		resultStorage = LLVMBuildAlloca(ctx.builder, CompileLlvmType(ctx, typeFunction->returnType), "call_return_storage");

		arguments.push_back(resultStorage);
	}

	TypeHandle *argumentType = typeFunction->arguments.head;

	for(ExprBase *value = node->arguments.head; value; value = value->next)
	{
		arguments.push_back(CompileArgument(ctx, argumentType->type, value));

		argumentType = argumentType->next;
	}

	arguments.push_back(LLVMBuildExtractValue(ctx.builder, function, 0, "context"));

	LLVMValueRef functionRef = LLVMBuildExtractValue(ctx.builder, function, 1, "function");

	LLVMTypeRef functionType = LLVMPointerType(CompileLlvmFunctionType(ctx, typeFunction), 0);

	functionRef = LLVMBuildPointerCast(ctx.builder, functionRef, functionType, "to_func_type");

	LLVMValueRef result = LLVMBuildCall(ctx.builder, functionRef, arguments.data, arguments.count, "");

	if(isStructReturnType)
	{
		LLVMAddCallSiteAttribute(result, 1, LLVMCreateEnumAttribute(ctx.context, LLVMGetEnumAttributeKindForName("sret", unsigned(strlen("sret"))), 0));

		result = LLVMBuildLoad(ctx.builder, resultStorage, "");
	}

	return CheckType(ctx, node, ConvertToStackType(ctx, result, node->type));
}

LLVMValueRef CompileLlvmAliasDefinition(LlvmCompilationContext &ctx, ExprAliasDefinition *node)
{
	return CheckType(ctx, node, NULL);
}

LLVMValueRef CompileLlvmClassPrototype(LlvmCompilationContext &ctx, ExprClassPrototype *node)
{
	return CheckType(ctx, node, NULL);
}

LLVMValueRef CompileLlvmGenericClassPrototype(LlvmCompilationContext &ctx, ExprGenericClassPrototype *node)
{
	return CheckType(ctx, node, NULL);
}

LLVMValueRef CompileLlvmClassDefinition(LlvmCompilationContext &ctx, ExprClassDefinition *node)
{
	return CheckType(ctx, node, NULL);
}

LLVMValueRef CompileLlvmEnumDefinition(LlvmCompilationContext &ctx, ExprEnumDefinition *node)
{
	return CheckType(ctx, node, NULL);
}

LLVMValueRef CompileLlvmIfElse(LlvmCompilationContext &ctx, ExprIfElse *node)
{
	LLVMValueRef condition = CompileLlvm(ctx, node->condition);

	condition = LLVMBuildICmp(ctx.builder, LLVMIntNE, condition, LLVMConstInt(CompileLlvmType(ctx, GetStackType(ctx, node->condition->type)), 0, true), "");

	LLVMBasicBlockRef trueBlock = LLVMAppendBasicBlockInContext(ctx.context, ctx.currentFunction, "if_true");
	LLVMBasicBlockRef falseBlock = LLVMAppendBasicBlockInContext(ctx.context, ctx.currentFunction, "if_false");
	LLVMBasicBlockRef exitBlock = LLVMAppendBasicBlockInContext(ctx.context, ctx.currentFunction, "if_exit");

	if(node->falseBlock)
		LLVMBuildCondBr(ctx.builder, condition, trueBlock, falseBlock);
	else
		LLVMBuildCondBr(ctx.builder, condition, trueBlock, exitBlock);

	LLVMPositionBuilderAtEnd(ctx.builder, trueBlock);

	CompileLlvm(ctx, node->trueBlock);

	LLVMBuildBr(ctx.builder, exitBlock);

	LLVMPositionBuilderAtEnd(ctx.builder, falseBlock);

	if(node->falseBlock)
		CompileLlvm(ctx, node->falseBlock);

	LLVMBuildBr(ctx.builder, exitBlock);

	LLVMPositionBuilderAtEnd(ctx.builder, exitBlock);

	return CheckType(ctx, node, NULL);
}

LLVMValueRef CompileLlvmFor(LlvmCompilationContext &ctx, ExprFor *node)
{
	CompileLlvm(ctx, node->initializer);

	LLVMBasicBlockRef conditionBlock = LLVMAppendBasicBlockInContext(ctx.context, ctx.currentFunction, "for_cond");
	LLVMBasicBlockRef bodyBlock = LLVMAppendBasicBlockInContext(ctx.context, ctx.currentFunction, "for_body");
	LLVMBasicBlockRef iterationBlock = LLVMAppendBasicBlockInContext(ctx.context, ctx.currentFunction, "for_iter");
	LLVMBasicBlockRef exitBlock = LLVMAppendBasicBlockInContext(ctx.context, ctx.currentFunction, "for_exit");

	ctx.loopInfo.push_back(LlvmCompilationContext::LoopInfo(exitBlock, iterationBlock));

	LLVMBuildBr(ctx.builder, conditionBlock);

	LLVMPositionBuilderAtEnd(ctx.builder, conditionBlock);

	LLVMValueRef condition = CompileLlvm(ctx, node->condition);

	condition = LLVMBuildICmp(ctx.builder, LLVMIntNE, condition, LLVMConstInt(CompileLlvmType(ctx, GetStackType(ctx, node->condition->type)), 0, true), "");

	LLVMBuildCondBr(ctx.builder, condition, bodyBlock, exitBlock);

	LLVMPositionBuilderAtEnd(ctx.builder, bodyBlock);

	CompileLlvm(ctx, node->body);

	LLVMBuildBr(ctx.builder, iterationBlock);

	LLVMPositionBuilderAtEnd(ctx.builder, iterationBlock);

	CompileLlvm(ctx, node->increment);

	LLVMBuildBr(ctx.builder, conditionBlock);

	LLVMPositionBuilderAtEnd(ctx.builder, exitBlock);

	ctx.loopInfo.pop_back();

	return CheckType(ctx, node, NULL);
}

LLVMValueRef CompileLlvmWhile(LlvmCompilationContext &ctx, ExprWhile *node)
{
	LLVMBasicBlockRef conditionBlock = LLVMAppendBasicBlockInContext(ctx.context, ctx.currentFunction, "while_cond");
	LLVMBasicBlockRef bodyBlock = LLVMAppendBasicBlockInContext(ctx.context, ctx.currentFunction, "while_body");
	LLVMBasicBlockRef exitBlock = LLVMAppendBasicBlockInContext(ctx.context, ctx.currentFunction, "while_exit");

	ctx.loopInfo.push_back(LlvmCompilationContext::LoopInfo(exitBlock, conditionBlock));

	LLVMBuildBr(ctx.builder, conditionBlock);

	LLVMPositionBuilderAtEnd(ctx.builder, conditionBlock);

	LLVMValueRef condition = CompileLlvm(ctx, node->condition);

	condition = LLVMBuildICmp(ctx.builder, LLVMIntNE, condition, LLVMConstInt(CompileLlvmType(ctx, GetStackType(ctx, node->condition->type)), 0, true), "");

	LLVMBuildCondBr(ctx.builder, condition, bodyBlock, exitBlock);

	LLVMPositionBuilderAtEnd(ctx.builder, bodyBlock);

	CompileLlvm(ctx, node->body);

	LLVMBuildBr(ctx.builder, conditionBlock);

	LLVMPositionBuilderAtEnd(ctx.builder, exitBlock);

	ctx.loopInfo.pop_back();

	return CheckType(ctx, node, NULL);
}

LLVMValueRef CompileLlvmDoWhile(LlvmCompilationContext &ctx, ExprDoWhile *node)
{
	LLVMBasicBlockRef bodyBlock = LLVMAppendBasicBlockInContext(ctx.context, ctx.currentFunction, "do_body");
	LLVMBasicBlockRef condBlock = LLVMAppendBasicBlockInContext(ctx.context, ctx.currentFunction, "do_cond");
	LLVMBasicBlockRef exitBlock = LLVMAppendBasicBlockInContext(ctx.context, ctx.currentFunction, "do_exit");

	LLVMBuildBr(ctx.builder, bodyBlock);

	LLVMPositionBuilderAtEnd(ctx.builder, bodyBlock);

	ctx.loopInfo.push_back(LlvmCompilationContext::LoopInfo(exitBlock, condBlock));

	CompileLlvm(ctx, node->body);

	LLVMBuildBr(ctx.builder, condBlock);

	LLVMPositionBuilderAtEnd(ctx.builder, condBlock);

	LLVMValueRef condition = CompileLlvm(ctx, node->condition);

	condition = LLVMBuildICmp(ctx.builder, LLVMIntNE, condition, LLVMConstInt(CompileLlvmType(ctx, GetStackType(ctx, node->condition->type)), 0, true), "");

	LLVMBuildCondBr(ctx.builder, condition, bodyBlock, exitBlock);

	LLVMPositionBuilderAtEnd(ctx.builder, exitBlock);

	ctx.loopInfo.pop_back();

	return CheckType(ctx, node, NULL);
}

LLVMValueRef CompileLlvmSwitch(LlvmCompilationContext &ctx, ExprSwitch *node)
{
	CompileLlvm(ctx, node->condition);

	SmallArray<LLVMBasicBlockRef, 16> conditionBlocks(ctx.allocator);
	SmallArray<LLVMBasicBlockRef, 16> caseBlocks(ctx.allocator);

	// Generate blocks for all cases
	for(ExprBase *curr = node->cases.head; curr; curr = curr->next)
		conditionBlocks.push_back(LLVMAppendBasicBlockInContext(ctx.context, ctx.currentFunction, "switch_case"));

	// Generate blocks for all cases
	for(ExprBase *curr = node->blocks.head; curr; curr = curr->next)
		caseBlocks.push_back(LLVMAppendBasicBlockInContext(ctx.context, ctx.currentFunction, "case_block"));

	LLVMBasicBlockRef defaultBlock = LLVMAppendBasicBlockInContext(ctx.context, ctx.currentFunction, "default_block");
	LLVMBasicBlockRef exitBlock = LLVMAppendBasicBlockInContext(ctx.context, ctx.currentFunction, "switch_exit");

	LLVMBuildBr(ctx.builder, conditionBlocks.empty() ? defaultBlock : conditionBlocks[0]);

	unsigned i;

	// Generate code for all conditions
	i = 0;
	for(ExprBase *curr = node->cases.head; curr; curr = curr->next, i++)
	{
		LLVMPositionBuilderAtEnd(ctx.builder, conditionBlocks[i]);

		LLVMValueRef condition = CompileLlvm(ctx, curr);

		condition = LLVMBuildICmp(ctx.builder, LLVMIntNE, condition, LLVMConstInt(CompileLlvmType(ctx, GetStackType(ctx, curr->type)), 0, true), "");

		LLVMBuildCondBr(ctx.builder, condition, caseBlocks[i], curr->next ? conditionBlocks[i + 1] : defaultBlock);
	}

	ctx.loopInfo.push_back(LlvmCompilationContext::LoopInfo(exitBlock, NULL));

	// Generate code for all cases
	i = 0;
	for(ExprBase *curr = node->blocks.head; curr; curr = curr->next, i++)
	{
		LLVMPositionBuilderAtEnd(ctx.builder, caseBlocks[i]);

		CompileLlvm(ctx, curr);

		LLVMBuildBr(ctx.builder, curr->next ? caseBlocks[i + 1] : defaultBlock);
	}

	// Create default block
	LLVMPositionBuilderAtEnd(ctx.builder, defaultBlock);

	if(node->defaultBlock)
		CompileLlvm(ctx, node->defaultBlock);

	LLVMBuildBr(ctx.builder, exitBlock);

	LLVMPositionBuilderAtEnd(ctx.builder, exitBlock);

	ctx.loopInfo.pop_back();

	return CheckType(ctx, node, NULL);
}

LLVMValueRef CompileLlvmBreak(LlvmCompilationContext &ctx, ExprBreak *node)
{
	if(node->closures)
		CompileLlvm(ctx, node->closures);

	LLVMBasicBlockRef target = ctx.loopInfo[ctx.loopInfo.size() - node->depth].breakBlock;

	LLVMBuildBr(ctx.builder, target);

	LLVMBasicBlockRef afterReturn = LLVMAppendBasicBlockInContext(ctx.context, ctx.currentFunction, "after_break");

	LLVMPositionBuilderAtEnd(ctx.builder, afterReturn);

	return CheckType(ctx, node, NULL);
}

LLVMValueRef CompileLlvmContinue(LlvmCompilationContext &ctx, ExprContinue *node)
{
	if(node->closures)
		CompileLlvm(ctx, node->closures);

	LLVMBasicBlockRef target = GetLoopContinueBlock(ctx, node->depth);

	LLVMBuildBr(ctx.builder, target);

	LLVMBasicBlockRef afterReturn = LLVMAppendBasicBlockInContext(ctx.context, ctx.currentFunction, "after_continue");

	LLVMPositionBuilderAtEnd(ctx.builder, afterReturn);

	return CheckType(ctx, node, NULL);
}

LLVMValueRef CompileLlvmBlock(LlvmCompilationContext &ctx, ExprBlock *node)
{
	for(ExprBase *value = node->expressions.head; value; value = value->next)
		CompileLlvm(ctx, value);

	if(node->closures)
		CompileLlvm(ctx, node->closures);

	return CheckType(ctx, node, NULL);
}

LLVMValueRef CompileLlvmSequence(LlvmCompilationContext &ctx, ExprSequence *node)
{
	LLVMValueRef result = NULL;

	for(ExprBase *value = node->expressions.head; value; value = value->next)
		result = CompileLlvm(ctx, value);

	return CheckType(ctx, node, result);
}

LLVMValueRef CompileLlvm(LlvmCompilationContext &ctx, ExprBase *expression)
{
	if(ExprVoid *node = getType<ExprVoid>(expression))
		return CompileLlvmVoid(ctx, node);

	if(ExprBoolLiteral *node = getType<ExprBoolLiteral>(expression))
		return CompileLlvmBoolLiteral(ctx, node);

	if(ExprCharacterLiteral *node = getType<ExprCharacterLiteral>(expression))
		return CompileLlvmCharacterLiteral(ctx, node);

	if(ExprStringLiteral *node = getType<ExprStringLiteral>(expression))
		return CompileLlvmStringLiteral(ctx, node);

	if(ExprIntegerLiteral *node = getType<ExprIntegerLiteral>(expression))
		return CompileLlvmIntegerLiteral(ctx, node);

	if(ExprRationalLiteral *node = getType<ExprRationalLiteral>(expression))
		return CompileLlvmRationalLiteral(ctx, node);

	if(ExprTypeLiteral *node = getType<ExprTypeLiteral>(expression))
		return CompileLlvmTypeLiteral(ctx, node);

	if(ExprNullptrLiteral *node = getType<ExprNullptrLiteral>(expression))
		return CompileLlvmNullptrLiteral(ctx, node);

	if(ExprFunctionIndexLiteral *node = getType<ExprFunctionIndexLiteral>(expression))
		return CompileLlvmFunctionIndexLiteral(ctx, node);

	if(ExprPassthrough *node = getType<ExprPassthrough>(expression))
		return CompileLlvmPassthrough(ctx, node);

	if(ExprArray *node = getType<ExprArray>(expression))
		return CompileLlvmArray(ctx, node);

	if(ExprPreModify *node = getType<ExprPreModify>(expression))
		return CompileLlvmPreModify(ctx, node);

	if(ExprPostModify *node = getType<ExprPostModify>(expression))
		return CompileLlvmPostModify(ctx, node);	

	if(ExprTypeCast *node = getType<ExprTypeCast>(expression))
		return CompileLlvmTypeCast(ctx, node);

	if(ExprUnaryOp *node = getType<ExprUnaryOp>(expression))
		return CompileLlvmUnaryOp(ctx, node);

	if(ExprBinaryOp *node = getType<ExprBinaryOp>(expression))
		return CompileLlvmBinaryOp(ctx, node);

	if(ExprGetAddress *node = getType<ExprGetAddress>(expression))
		return CompileLlvmGetAddress(ctx, node);

	if(ExprDereference *node = getType<ExprDereference>(expression))
		return CompileLlvmDereference(ctx, node);

	if(ExprUnboxing *node = getType<ExprUnboxing>(expression))
		return CompileLlvmUnboxing(ctx, node);

	if(ExprConditional *node = getType<ExprConditional>(expression))
		return CompileLlvmConditional(ctx, node);

	if(ExprAssignment *node = getType<ExprAssignment>(expression))
		return CompileLlvmAssignment(ctx, node);

	if(ExprMemberAccess *node = getType<ExprMemberAccess>(expression))
		return CompileLlvmMemberAccess(ctx, node);

	if(ExprArrayIndex *node = getType<ExprArrayIndex>(expression))
		return CompileLlvmArrayIndex(ctx, node);

	if(ExprReturn *node = getType<ExprReturn>(expression))
		return CompileLlvmReturn(ctx, node);

	if(ExprYield *node = getType<ExprYield>(expression))
		return CompileLlvmYield(ctx, node);

	if(ExprVariableDefinition *node = getType<ExprVariableDefinition>(expression))
		return CompileLlvmVariableDefinition(ctx, node);

	if(ExprArraySetup *node = getType<ExprArraySetup>(expression))
		return CompileLlvmArraySetup(ctx, node);

	if(ExprVariableDefinitions *node = getType<ExprVariableDefinitions>(expression))
		return CompileLlvmVariableDefinitions(ctx, node);

	if(ExprVariableAccess *node = getType<ExprVariableAccess>(expression))
		return CompileLlvmVariableAccess(ctx, node);

	if(ExprFunctionContextAccess *node = getType<ExprFunctionContextAccess>(expression))
		return CompileLlvmFunctionContextAccess(ctx, node);

	if(ExprFunctionDefinition *node = getType<ExprFunctionDefinition>(expression))
		return CompileLlvmFunctionDefinition(ctx, node);

	if(ExprGenericFunctionPrototype *node = getType<ExprGenericFunctionPrototype>(expression))
		return CompileLlvmGenericFunctionPrototype(ctx, node);

	if(ExprFunctionAccess *node = getType<ExprFunctionAccess>(expression))
		return CompileLlvmFunctionAccess(ctx, node);

	if(ExprFunctionCall *node = getType<ExprFunctionCall>(expression))
		return CompileLlvmFunctionCall(ctx, node);

	if(ExprAliasDefinition *node = getType<ExprAliasDefinition>(expression))
		return CompileLlvmAliasDefinition(ctx, node);

	if(ExprClassPrototype *node = getType<ExprClassPrototype>(expression))
		return CompileLlvmClassPrototype(ctx, node);

	if(ExprGenericClassPrototype *node = getType<ExprGenericClassPrototype>(expression))
		return CompileLlvmGenericClassPrototype(ctx, node);

	if(ExprClassDefinition *node = getType<ExprClassDefinition>(expression))
		return CompileLlvmClassDefinition(ctx, node);

	if(ExprEnumDefinition *node = getType<ExprEnumDefinition>(expression))
		return CompileLlvmEnumDefinition(ctx, node);

	if(ExprIfElse *node = getType<ExprIfElse>(expression))
		return CompileLlvmIfElse(ctx, node);

	if(ExprFor *node = getType<ExprFor>(expression))
		return CompileLlvmFor(ctx, node);

	if(ExprWhile *node = getType<ExprWhile>(expression))
		return CompileLlvmWhile(ctx, node);

	if(ExprDoWhile *node = getType<ExprDoWhile>(expression))
		return CompileLlvmDoWhile(ctx, node);

	if(ExprSwitch *node = getType<ExprSwitch>(expression))
		return CompileLlvmSwitch(ctx, node);

	if(ExprBreak *node = getType<ExprBreak>(expression))
		return CompileLlvmBreak(ctx, node);

	if(ExprContinue *node = getType<ExprContinue>(expression))
		return CompileLlvmContinue(ctx, node);

	if(ExprBlock *node = getType<ExprBlock>(expression))
		return CompileLlvmBlock(ctx, node);

	if(ExprSequence *node = getType<ExprSequence>(expression))
		return CompileLlvmSequence(ctx, node);

	if(!expression)
		return NULL;

	assert(!"unknown type");

	return NULL;
}

LlvmModule* CompileLlvm(ExpressionContext &exprCtx, ExprModule *expression)
{
	TRACE_SCOPE("InstructionTreeLlvm", "CompileLlvm");

	LlvmCompilationContext ctx(exprCtx);

	ctx.context = LLVMContextCreate();

	ctx.module = LLVMModuleCreateWithNameInContext("module", ctx.context);

	ctx.builder = LLVMCreateBuilderInContext(ctx.context);

	ctx.functionPassManager = LLVMCreateFunctionPassManagerForModule(ctx.module);

	LLVMAddBasicAliasAnalysisPass(ctx.functionPassManager);

	if(ctx.enableOptimization)
	{
		LLVMAddScalarReplAggregatesPass(ctx.functionPassManager);
		LLVMAddInstructionCombiningPass(ctx.functionPassManager);
		LLVMAddEarlyCSEPass(ctx.functionPassManager);
		LLVMAddReassociatePass(ctx.functionPassManager);
		LLVMAddGVNPass(ctx.functionPassManager);
		LLVMAddConstantPropagationPass(ctx.functionPassManager);
	}

	LLVMAddCFGSimplificationPass(ctx.functionPassManager);
	LLVMAddAggressiveDCEPass(ctx.functionPassManager);

	LLVMInitializeFunctionPassManager(ctx.functionPassManager);

	// Generate type indexes
	for(unsigned i = 0; i < ctx.ctx.types.size(); i++)
		ctx.ctx.types[i]->typeIndex = i;

	// Generate function indexes
	for(unsigned i = 0; i < ctx.ctx.functions.size(); i++)
		ctx.ctx.functions[i]->functionIndex = i;

	// Reserve types, generate as required
	ctx.types.resize(ctx.ctx.types.size());
	memset(ctx.types.data, 0, ctx.types.count * sizeof(ctx.types[0]));

	// Reserve functions
	ctx.functions.resize(ctx.ctx.functions.size());
	memset(ctx.functions.data, 0, ctx.functions.count * sizeof(ctx.functions[0]));

	// Create runtime support functions
	{
		LLVMValueRef function = LLVMAddFunction(ctx.module, "__llvmAbortNoReturn", CompileLlvmFunctionType(ctx, ctx.ctx.typeVoid, ArrayView<TypeBase*>()));
		LLVMSetLinkage(function, LLVMExternalLinkage);
	}

	{
		TypeBase* arguments[] = { ctx.ctx.typeAutoRef, ctx.ctx.typeTypeID };

		LLVMValueRef function = LLVMAddFunction(ctx.module, "__llvmConvertPtr", CompileLlvmFunctionType(ctx, ctx.ctx.typeNullPtr, arguments));
		LLVMSetLinkage(function, LLVMExternalLinkage);
	}

	{
		TypeBase* arguments[] = { ctx.ctx.typeInt, ctx.ctx.typeInt };

		LLVMValueRef function = LLVMAddFunction(ctx.module, "__llvmPowInt", CompileLlvmFunctionType(ctx, ctx.ctx.typeInt, arguments));
		LLVMSetLinkage(function, LLVMExternalLinkage);
	}

	{
		TypeBase* arguments[] = { ctx.ctx.typeLong, ctx.ctx.typeLong };

		LLVMValueRef function = LLVMAddFunction(ctx.module, "__llvmPowLong", CompileLlvmFunctionType(ctx, ctx.ctx.typeLong, arguments));
		LLVMSetLinkage(function, LLVMExternalLinkage);
	}

	{
		TypeBase* arguments[] = { ctx.ctx.typeDouble, ctx.ctx.typeDouble };

		LLVMValueRef function = LLVMAddFunction(ctx.module, "__llvmPowDouble", CompileLlvmFunctionType(ctx, ctx.ctx.typeDouble, arguments));
		LLVMSetLinkage(function, LLVMExternalLinkage);
	}

	{
		TypeBase* arguments[] = { ctx.ctx.typeInt };

		LLVMValueRef function = LLVMAddFunction(ctx.module, "__llvmReturnInt", CompileLlvmFunctionType(ctx, ctx.ctx.typeVoid, arguments));
		LLVMSetLinkage(function, LLVMExternalLinkage);
	}

	{
		TypeBase* arguments[] = { ctx.ctx.typeLong };

		LLVMValueRef function = LLVMAddFunction(ctx.module, "__llvmReturnLong", CompileLlvmFunctionType(ctx, ctx.ctx.typeVoid, arguments));
		LLVMSetLinkage(function, LLVMExternalLinkage);
	}

	{
		TypeBase* arguments[] = { ctx.ctx.typeDouble };

		LLVMValueRef function = LLVMAddFunction(ctx.module, "__llvmReturnDouble", CompileLlvmFunctionType(ctx, ctx.ctx.typeVoid, arguments));
		LLVMSetLinkage(function, LLVMExternalLinkage);
	}

	// Generate functions
	for(unsigned i = 0; i < ctx.ctx.functions.size(); i++)
	{
		FunctionData *function = ctx.ctx.functions[i];

		if(ctx.ctx.IsGenericFunction(function))
			continue;

		// Skip prototypes that will have an implementation later
		if(function->isPrototype && function->implementation)
			continue;

		// Skip if function is already ready
		if(ctx.functions[function->functionIndex])
			continue;

		LLVMValueRef llvmFunction = LLVMAddFunction(ctx.module, CreateLlvmFunctionName(ctx, function), CompileLlvmFunctionType(ctx, function->type));

		if(IsStructReturnType(function->type->returnType))
		{
			LLVMAddAttributeAtIndex(llvmFunction, 1, LLVMCreateEnumAttribute(ctx.context, LLVMGetEnumAttributeKindForName("nonnull", unsigned(strlen("nonnull"))), 0));
			LLVMAddAttributeAtIndex(llvmFunction, 1, LLVMCreateEnumAttribute(ctx.context, LLVMGetEnumAttributeKindForName("sret", unsigned(strlen("sret"))), 0));
		}

		ctx.functions[function->functionIndex] = llvmFunction;
	}

	for(unsigned i = 0; i < ctx.ctx.functions.size(); i++)
	{
		FunctionData *function = ctx.ctx.functions[i];

		if(function->isPrototype && function->implementation)
			ctx.functions[function->functionIndex] = ctx.functions[function->implementation->functionIndex];
	}

	// Generate global valriables
	for(unsigned i = 0; i < ctx.ctx.variables.size(); i++)
	{
		VariableData *variable = ctx.ctx.variables[i];

		if(ctx.ctx.GlobalScopeFrom(variable->scope))
		{
			LLVMValueRef value = LLVMAddGlobal(ctx.module, CompileLlvmType(ctx, variable->type), CreateLlvmName(ctx, variable->name->name));

			if(!variable->importModule)
				LLVMSetInitializer(value, LLVMConstNull(CompileLlvmType(ctx, variable->type)));

			assert(!ctx.variables.find(variable->uniqueId));

			ctx.variables.insert(variable->uniqueId, value);
		}
	}

	for(unsigned i = 0; i < expression->definitions.size(); i++)
		CompileLlvm(ctx, expression->definitions[i]);

	ctx.skipFunctionDefinitions = true;

	// Generate global function
	{
		LLVMValueRef function = LLVMAddFunction(ctx.module, "__llvmEntry", LLVMFunctionType(LLVMVoidTypeInContext(ctx.context), NULL, 0, false));

		LLVMSetLinkage(function, LLVMExternalLinkage);

		// Setup global function
		ctx.currentFunction = function;
		ctx.currentFunctionGlobal = true;

		LLVMBasicBlockRef block = LLVMAppendBasicBlockInContext(ctx.context, ctx.currentFunction, "start");

		LLVMPositionBuilderAtEnd(ctx.builder, block);

		for(ExprBase *value = expression->setup.head; value; value = value->next)
			CompileLlvm(ctx, value);

		for(ExprBase *value = expression->expressions.head; value; value = value->next)
			CompileLlvm(ctx, value);

		LLVMBuildRetVoid(ctx.builder);

		CheckFunction(ctx, ctx.currentFunction, InplaceStr("global"));

		if(LLVMRunFunctionPassManager(ctx.functionPassManager, function))
			LLVMRunFunctionPassManager(ctx.functionPassManager, function);

		ctx.currentFunction = NULL;
		ctx.currentFunctionGlobal = false;
	}

	LLVMFinalizeFunctionPassManager(ctx.functionPassManager);

	char *error = NULL;

	if(LLVMVerifyModule(ctx.module, LLVMReturnStatusAction, &error))
	{
		printf("LLVM module verification failed with:\n%s\n", error);

		LLVMDisposeMessage(error);

		LLVMDisposePassManager(ctx.functionPassManager);

		LLVMDisposeBuilder(ctx.builder);

		LLVMDisposeModule(ctx.module);

		LLVMContextDispose(ctx.context);

		return NULL;
	}

	LLVMDisposeMessage(error);

	LlvmModule *module = new (ctx.get<LlvmModule>()) LlvmModule();

	LLVMMemoryBufferRef buffer = LLVMWriteBitcodeToMemoryBuffer(ctx.module);

	module->moduleSize = (unsigned)LLVMGetBufferSize(buffer);

	module->moduleData = (char*)ctx.allocator->alloc(module->moduleSize);

	if(const char *start = LLVMGetBufferStart(buffer))
		memcpy(module->moduleData, start, module->moduleSize);

	LLVMDisposeMemoryBuffer(buffer);

	LLVMDisposePassManager(ctx.functionPassManager);

	LLVMDisposeBuilder(ctx.builder);

	LLVMDisposeModule(ctx.module);

	LLVMContextDispose(ctx.context);

	return module;
}
