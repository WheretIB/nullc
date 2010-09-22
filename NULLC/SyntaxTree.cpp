#include "stdafx.h"
#include "SyntaxTree.h"

#include "CodeInfo.h"

using CodeInfo::nodeList;
using CodeInfo::cmdList;
using CodeInfo::cmdInfoList;

NodeZeroOP*	TakeLastNode()
{
	NodeZeroOP* last = nodeList.back();
	nodeList.pop_back();
	return last;
}

const char* binCommandToText[] = { "+", "-", "*", "/", "**", "%", "<", ">", "<=", ">=", "==", "!=", "<<", ">>", "&", "|", "^", "&&", "||", "^^"};
const char* unaryCommandToText[] = { "-", "-", "-", "~", "~", "!", "!" };

//Functions for work with types

//This function converts a type according to result type of binary operation between types 'first' and 'second'
//For example,  int * double = double, so first operand will be transformed to double
//				double * int = double, no transformations
asmStackType	ConvertFirstForSecond(asmStackType first, asmStackType second)
{
	if(first == STYPE_INT && second == STYPE_DOUBLE)
	{
		cmdList.push_back(VMCmd(cmdItoD));
		return second;
	}
	if(first == STYPE_LONG && second == STYPE_DOUBLE)
	{
		cmdList.push_back(VMCmd(cmdLtoD));
		return second;
	}
	if(first == STYPE_INT && second == STYPE_LONG)
	{
		cmdList.push_back(VMCmd(cmdItoL));
		return second;
	}
	return first;
}

//This functions transforms first type to second one
void	ConvertFirstToSecond(asmStackType first, asmStackType second)
{
	if(second == STYPE_DOUBLE)
	{
		if(first == STYPE_INT)
			cmdList.push_back(VMCmd(cmdItoD));
		else if(first == STYPE_LONG)
			cmdList.push_back(VMCmd(cmdLtoD));
	}else if(second == STYPE_LONG){
		if(first == STYPE_INT)
			cmdList.push_back(VMCmd(cmdItoL));
		else if(first == STYPE_DOUBLE)
			cmdList.push_back(VMCmd(cmdDtoL));
	}else if(second == STYPE_INT){
		if(first == STYPE_DOUBLE)
			cmdList.push_back(VMCmd(cmdDtoI));
		else if(first == STYPE_LONG)
			cmdList.push_back(VMCmd(cmdLtoI));
	}
}

TypeInfo*	ChooseBinaryOpResultType(TypeInfo* a, TypeInfo* b)
{
	if(a->type == TypeInfo::TYPE_DOUBLE)
		return a;
	if(b->type == TypeInfo::TYPE_DOUBLE)
		return b;
	if(a->type == TypeInfo::TYPE_FLOAT)
		return a;
	if(b->type == TypeInfo::TYPE_FLOAT)
		return b;
	if(a->type == TypeInfo::TYPE_LONG)
		return a;
	if(b->type == TypeInfo::TYPE_LONG)
		return b;
	if(a->type == TypeInfo::TYPE_INT)
		return a;
	if(b->type == TypeInfo::TYPE_INT)
		return b;
	if(a->type == TypeInfo::TYPE_SHORT)
		return a;
	if(b->type == TypeInfo::TYPE_SHORT)
		return b;
	if(a->type == TypeInfo::TYPE_CHAR)
		return a;
	if(b->type == TypeInfo::TYPE_CHAR)
		return b;
	assert(false);
	return NULL;
}

// class implementation

//////////////////////////////////////////////////////////////////////////
// Node that doesn't have any child nodes

ChunkedStackPool<65532>	NodeZeroOP::nodePool;

NodeZeroOP::NodeZeroOP()
{
	typeInfo = typeVoid;
	sourcePos = NULL;
	prev = next = head = NULL;
	nodeType = typeNodeZeroOp;
}
NodeZeroOP::NodeZeroOP(TypeInfo* tinfo)
{
	typeInfo = tinfo;
	sourcePos = NULL;
	prev = next = head = NULL;
	nodeType = typeNodeZeroOp;
}
NodeZeroOP::~NodeZeroOP()
{
}

void NodeZeroOP::Compile()
{
}

void NodeZeroOP::SetCodeInfo(const char* newSourcePos)
{
	sourcePos = newSourcePos;
}

void NodeZeroOP::AddExtraNode()
{
	assert(nodeList.size() > 0);
	nodeList.back()->next = head;
	if(head)
		head->prev = nodeList.back();
	head = TakeLastNode();
}

void NodeZeroOP::CompileExtra()
{
	NodeZeroOP *curr = head;
	while(curr)
	{
		curr->Compile();
		curr = curr->next;
	}
}

//////////////////////////////////////////////////////////////////////////
// Node that have one child node

NodeOneOP::NodeOneOP()
{
	first = NULL;
	nodeType = typeNodeOneOp;
}
NodeOneOP::~NodeOneOP()
{
}

void NodeOneOP::Compile()
{
	CompileExtra();
	first->Compile();
}

//////////////////////////////////////////////////////////////////////////
// Node that have two child nodes

NodeTwoOP::NodeTwoOP()
{
	second = NULL;
	nodeType = typeNodeTwoOp;
}
NodeTwoOP::~NodeTwoOP()
{
}

void NodeTwoOP::Compile()
{
	CompileExtra();
	NodeOneOP::Compile();
	second->Compile();
}

//////////////////////////////////////////////////////////////////////////
// Node that have three child nodes

NodeThreeOP::NodeThreeOP()
{
	third = NULL;
	nodeType = typeNodeThreeOp;
}
NodeThreeOP::~NodeThreeOP()
{
}

void NodeThreeOP::Compile()
{
	CompileExtra();
	NodeTwoOP::Compile();
	third->Compile();
}

//////////////////////////////////////////////////////////////////////////
// Node that puts a number on top of the stack

void NodeNumber::Compile()
{
	assert(typeInfo->size <= 8);
	if(typeInfo->refLevel)
	{
		cmdList.push_back(VMCmd(cmdPushPtrImmt, num.quad.low));
	}else{
		if(typeInfo->stackType != STYPE_INT)
			cmdList.push_back(VMCmd(cmdPushImmt, num.quad.high));
		cmdList.push_back(VMCmd(cmdPushImmt, num.quad.low));
	}
}

bool NodeNumber::ConvertTo(TypeInfo *target)
{
	if(target == typeInt || target == typeShort || target == typeChar)
	{
		num.integer = GetInteger();
	}else if(target == typeDouble || target == typeFloat){
		num.real = GetDouble();
	}else if(target == typeLong){
		num.integer64 = GetLong();
	}else{
		return false;
	}
	typeInfo = target;
	return true;
}

//////////////////////////////////////////////////////////////////////////
// Node that removes value left on top of the stack by child node

NodePopOp::NodePopOp()
{
	first = TakeLastNode();
	nodeType = typeNodePopOp;
}
NodePopOp::~NodePopOp()
{
}

void NodePopOp::Compile()
{
	if(sourcePos)
		cmdInfoList.AddDescription(cmdList.size(), sourcePos);
	CompileExtra();

	// Child node computes value
	first->Compile();
	if(first->typeInfo != typeVoid && first->typeInfo->size)
	{
		// Removing it from top of the stack
		cmdList.push_back(VMCmd(cmdPop, first->typeInfo->type == TypeInfo::TYPE_COMPLEX ? first->typeInfo->size : stackTypeSize[first->typeInfo->stackType]));
	}
}

//////////////////////////////////////////////////////////////////////////
// Node that applies selected operation on value on top of the stack

NodeUnaryOp::NodeUnaryOp(CmdID cmd, unsigned int argument)
{
	// Unary operation
	vmCmd.cmd = cmd;
	vmCmd.argument = argument;

	first = TakeLastNode();
	// Resulting type is the same as source type with exception for logical NOT
	bool logicalOp = cmd == cmdLogNot;
	typeInfo = logicalOp ? typeInt : first->typeInfo;

	if(cmd != cmdCheckedRet && ((first->typeInfo->refLevel != 0 && !logicalOp) || (first->typeInfo->type == TypeInfo::TYPE_COMPLEX && first->typeInfo != typeObject)))
		ThrowError(CodeInfo::lastKnownStartPos, "ERROR: unary operation '%s' is not supported on '%s'", unaryCommandToText[cmd - cmdNeg], first->typeInfo->GetFullTypeName());

	nodeType = typeNodeUnaryOp;
}
NodeUnaryOp::~NodeUnaryOp()
{
}

void NodeUnaryOp::Compile()
{
	CompileExtra();

	asmOperType aOT = operTypeForStackType[first->typeInfo->stackType];

	// Child node computes value
	first->Compile();
	if(first->typeInfo == typeObject)
	{
		cmdList.push_back(VMCmd(cmdPop, 4)); // remove 'typeid', it's irrelevant
#ifdef _M_X64
		cmdList.push_back(VMCmd(cmdBitOr)); // cmdLogNot works on int, but we have long pointer
#endif
	}

	// Execute command
	if(aOT == OTYPE_INT || first->typeInfo == typeObject || vmCmd.cmd == cmdCheckedRet)
		cmdList.push_back(VMCmd((InstructionCode)(vmCmd.cmd + 0), vmCmd.argument));
	else if(aOT == OTYPE_LONG)
		cmdList.push_back(VMCmd((InstructionCode)(vmCmd.cmd + 1), vmCmd.argument));
	else
		cmdList.push_back(VMCmd((InstructionCode)(vmCmd.cmd + 2), vmCmd.argument));
}

//////////////////////////////////////////////////////////////////////////
// Node that returns from function or program

NodeReturnOp::NodeReturnOp(bool localRet, TypeInfo* tinfo, FunctionInfo* parentFunc, bool yield)
{
	localReturn = localRet;
	yieldResult = yield;
	parentFunction = parentFunc;

	// Result type is set from outside
	typeInfo = tinfo;

	first = TakeLastNode();
	if(first->nodeType == typeNodeNumber && first->typeInfo != typeInfo)
		((NodeNumber*)first)->ConvertTo(typeInfo);

	nodeType = typeNodeReturnOp;
}
NodeReturnOp::~NodeReturnOp()
{
}

void NodeReturnOp::Compile()
{
	if(sourcePos)
		cmdInfoList.AddDescription(cmdList.size(), sourcePos);

	CompileExtra();

	if(first->nodeType == typeNodeZeroOp && first->typeInfo != typeVoid)
	{
		// return zero'd object
		unsigned times = (first->typeInfo == typeFloat) ? 2 : ((first->typeInfo->size + 3) / 4);
		for(unsigned i = 0; i < times; i++)
			cmdList.push_back(VMCmd(cmdPushImmt, 0));
	}else{
		// Compute value that we're going to return
		first->Compile();
	}
	// Convert it to the return type of the function
	if(typeInfo)
		ConvertFirstToSecond(first->typeInfo->stackType, typeInfo->stackType);

	// Return from function or program
	TypeInfo *retType = typeInfo ? typeInfo : first->typeInfo;
	asmOperType operType = operTypeForStackType[retType->stackType];

	unsigned int retSize = retType == typeFloat ? 8 : retType->size;
	if(retSize != 0 && retSize < 4)
		retSize = 4;
	if(parentFunction && parentFunction->closeUpvals)
		cmdList.push_back(VMCmd(cmdCloseUpvals, (unsigned short)CodeInfo::FindFunctionByPtr(parentFunction), 0));
	// If return is from coroutine, we either need to reset jumpOffset to the beginning of a function, or set it to instruction after return
	if(parentFunction && parentFunction->type == FunctionInfo::COROUTINE)
		cmdList.push_back(VMCmd(cmdYield, 0, yieldResult, parentFunction->allParamSize));	// yieldResult == true means save state and yield
	cmdList.push_back(VMCmd(cmdReturn, (unsigned char)operType, (unsigned short)localReturn, retSize));
}

//////////////////////////////////////////////////////////////////////////

NodeBlock::NodeBlock(FunctionInfo* parentFunc, unsigned int shift)
{
	parentFunction = parentFunc;
	stackFrameShift = shift;

	first = TakeLastNode();

	nodeType = typeNodeBlockOp;
}
NodeBlock::~NodeBlock()
{
}

void NodeBlock::Compile()
{
	if(sourcePos)
		cmdInfoList.AddDescription(cmdList.size(), sourcePos);

	CompileExtra();

	// Compute value that we're going to return
	first->Compile();
	if(parentFunction->closeUpvals)
		cmdList.push_back(VMCmd(cmdCloseUpvals, (unsigned short)CodeInfo::FindFunctionByPtr(parentFunction), stackFrameShift));
}

//////////////////////////////////////////////////////////////////////////
// Nodes that compiles function

NodeFuncDef::NodeFuncDef(FunctionInfo *info, unsigned int varShift)
{
	// Function description
	funcInfo = info;
	// Size of all local variables
	shift = info->type == FunctionInfo::COROUTINE ? (funcInfo->allParamSize + NULLC_PTR_SIZE) : varShift;

	disabled = false;

	first = TakeLastNode();

	nodeType = typeNodeFuncDef;
}
NodeFuncDef::~NodeFuncDef()
{
}

void NodeFuncDef::Enable()
{
	disabled = false;
}
void NodeFuncDef::Disable()
{
	disabled = true;
}

void NodeFuncDef::Compile()
{
	if(disabled)
	{
		CompileExtra();
		return;
	}

	if(sourcePos)
		cmdInfoList.AddDescription(cmdList.size(), sourcePos);

	funcInfo->address = cmdList.size();

	assert(funcInfo->allParamSize + NULLC_PTR_SIZE < 65536);

	// Stack frame should remain aligned, so its size should multiple of 16
	unsigned int size = (shift + 0xf) & ~0xf;
	assert(size >= funcInfo->allParamSize + NULLC_PTR_SIZE);
	
	// Save previous stack frame, and expand current by shift bytes
	cmdList.push_back(VMCmd(cmdPushVTop, (unsigned short)(funcInfo->type == FunctionInfo::COROUTINE ? size : (funcInfo->allParamSize + NULLC_PTR_SIZE)), size));
	// At the beginning of a coroutine, we need to jump to saved instruction offset
	if(funcInfo->type == FunctionInfo::COROUTINE)
		cmdList.push_back(VMCmd(cmdYield, 1, 0, funcInfo->allParamSize));	// pass an offset to closure
	// Generate function code
	first->Compile();

	if(funcInfo->retType == typeVoid)
	{
		// If function returns void, this is implicit return
		cmdList.push_back(VMCmd(cmdReturn, 0, 1, 0));
	}else{
		// Stop program execution if user forgot the return statement
		cmdList.push_back(VMCmd(cmdReturn, bitRetError, 1, 0));
	}

	funcInfo->codeSize = cmdList.size() - funcInfo->address;
}

//////////////////////////////////////////////////////////////////////////
// Node that calls function

NodeFuncCall::NodeFuncCall(FunctionInfo *info, FunctionType *type)
{
	// Function description
	funcInfo = info;

	// Function type description
	funcType = type;

	// Result type is fetched from function type
	typeInfo = funcType->retType;

	if(funcInfo && (funcInfo->type == FunctionInfo::LOCAL || funcInfo->type == FunctionInfo::COROUTINE))
		first = TakeLastNode();

	if(!funcInfo)
		first = TakeLastNode();

	if(funcType->paramCount > 0)
	{
		paramHead = paramTail = TakeLastNode();
		if(paramHead->nodeType == typeNodeNumber && funcType->paramType[funcType->paramCount-1] != paramHead->typeInfo)
			((NodeNumber*)paramHead)->ConvertTo(funcType->paramType[funcType->paramCount-1]);
	}else{
		paramHead = paramTail = NULL;
	}

	// Take nodes for all parameters
	for(unsigned int i = 1; i < funcType->paramCount; i++)
	{
		paramTail->next = TakeLastNode();
		TypeInfo	*paramType = funcType->paramType[funcType->paramCount-i-1];
		if(paramTail->next->nodeType == typeNodeNumber && paramType != paramTail->next->typeInfo)
			((NodeNumber*)paramTail->next)->ConvertTo(paramType);
		paramTail->next->prev = paramTail;
		paramTail = paramTail->next;
	}

	if(funcInfo && funcInfo->type == FunctionInfo::THISCALL)
		first = TakeLastNode();

	nodeType = typeNodeFuncCall;
}
NodeFuncCall::~NodeFuncCall()
{
}

void NodeFuncCall::Compile()
{
	CompileExtra();

	// Find parameter values
	if(first)
	{
		first->Compile();
	}else if(funcInfo){
		NodeNumber nullPtr = NodeNumber(0, CodeInfo::GetReferenceType(typeVoid));
		nullPtr.Compile();
	}
	if(funcType->paramCount > 0)
	{
		NodeZeroOP	*curr = paramHead;
		TypeInfo	**paramType = funcType->paramType + funcType->paramCount - 1;
		do
		{
			if(curr->typeInfo->size == 0)
			{
				curr->Compile();
				cmdList.push_back(VMCmd(cmdPushImmt, 0));
			}else if(*paramType == typeFloat && curr->nodeType == typeNodeNumber){
				float num = (float)((NodeNumber*)curr)->GetDouble();
				cmdList.push_back(VMCmd(cmdPushImmt, *(int*)&num));
			}else{
				// Compute parameter value
				curr->Compile();
				// Convert it to type that function expects
				ConvertFirstToSecond(curr->typeInfo->stackType, (*paramType)->stackType);
				if(*paramType == typeFloat)
					cmdList.push_back(VMCmd(cmdDtoF));
			}
			curr = curr->next;
			paramType--;
		}while(curr);
	}
	unsigned int ID = CodeInfo::FindFunctionByPtr(funcInfo);
	unsigned short helper = (unsigned short)((typeInfo->type == TypeInfo::TYPE_COMPLEX || typeInfo->type == TypeInfo::TYPE_VOID) ? typeInfo->size : (bitRetSimple | operTypeForStackType[typeInfo->stackType]));
	if(funcInfo)
		cmdList.push_back(VMCmd(cmdCall, helper, ID));
	else
		cmdList.push_back(VMCmd(cmdCallPtr, helper, funcType->paramSize));
}

//////////////////////////////////////////////////////////////////////////
// Node that fetches variable value

NodeGetAddress::NodeGetAddress(VariableInfo* vInfo, int vAddress, bool absAddr, TypeInfo *retInfo)
{
	assert(retInfo);

	varInfo = vInfo;
	addressOriginal = varAddress = vAddress;
	absAddress = absAddr;
	trackAddress = false;

	typeOrig = retInfo;
	typeInfo = CodeInfo::GetReferenceType(typeOrig);

	nodeType = typeNodeGetAddress;
}

NodeGetAddress::~NodeGetAddress()
{
}

bool NodeGetAddress::IsAbsoluteAddress()
{
	return absAddress;
}
void NodeGetAddress::SetAddressTracking()
{
	trackAddress = true;
}

void NodeGetAddress::IndexArray(int shift)
{
	assert(typeOrig->arrLevel != 0);
	varAddress += typeOrig->subType->size * shift;
	typeOrig = typeOrig->subType;
	typeInfo = CodeInfo::GetReferenceType(typeOrig);
}

void NodeGetAddress::ShiftToMember(TypeInfo::MemberVariable *member)
{
	assert(member);
	varAddress += member->offset;
	typeOrig = member->type;
	typeInfo = CodeInfo::GetReferenceType(typeOrig);
}

void NodeGetAddress::Compile()
{
	if(sourcePos)
		cmdInfoList.AddDescription(cmdList.size(), sourcePos);

	CompileExtra();

	cmdList.push_back(VMCmd(cmdGetAddr, absAddress ? 0 : 1, trackAddress ? varInfo->pos : varAddress));
}

//////////////////////////////////////////////////////////////////////////
NodeGetUpvalue::NodeGetUpvalue(FunctionInfo* functionInfo, int closureOffset, int closureElement, TypeInfo *retInfo)
{
	closurePos = closureOffset;
	closureElem = closureElement;
	typeInfo = retInfo;
	parentFunc = functionInfo;

	nodeType = typeNodeGetUpvalue;
}

NodeGetUpvalue::~NodeGetUpvalue()
{
}

void NodeGetUpvalue::Compile()
{
	if(sourcePos)
		cmdInfoList.AddDescription(cmdList.size(), sourcePos);

	CompileExtra();

	cmdList.push_back(VMCmd(cmdPushPtr, ADDRESS_RELATIVE, (unsigned short)typeInfo->size, closurePos));
	cmdList.push_back(VMCmd(cmdPushPtrStk, 0, (unsigned short)typeInfo->size, closureElem));
}

//////////////////////////////////////////////////////////////////////////
NodeConvertPtr::NodeConvertPtr(TypeInfo *dstType)
{
	assert(dstType);

	typeInfo = dstType;

	first = TakeLastNode();
	
	nodeType = typeNodeConvertPtr;
}
NodeConvertPtr::~NodeConvertPtr()
{
}

void NodeConvertPtr::Compile()
{
	CompileExtra();

	first->Compile();
	if(typeInfo == typeObject || typeInfo == typeTypeid)
	{
		cmdList.push_back(VMCmd(cmdPushTypeID, first->typeInfo->subType->typeIndex));
	}else{
		cmdList.push_back(VMCmd(cmdConvertPtr, typeInfo->subType->typeIndex));
	}
}

//////////////////////////////////////////////////////////////////////////
// Node that sets value to the variable

NodeVariableSet::NodeVariableSet(TypeInfo* targetType, bool firstDefinition, bool swapNodes)
{
	assert(targetType);
	typeInfo = targetType->subType;

	if(swapNodes)
		second = TakeLastNode();

	// Address of the target variable
	first = TakeLastNode();
	assert(first->typeInfo->refLevel != 0);

	if(!swapNodes)
		second = TakeLastNode();

	if(typeInfo->arrLevel < 2 && typeInfo->refLevel == 0 && second->nodeType == typeNodeNumber)
		static_cast<NodeNumber*>(second)->ConvertTo(typeInfo);

	// If this is the first array definition and value is array sub-type, we set it to all array elements
	arrSetAll = (firstDefinition && typeInfo->arrLevel == 1 && typeInfo->arrSize != TypeInfo::UNSIZED_ARRAY && second->typeInfo->arrLevel == 0 && second->typeInfo->refLevel == 0 && typeInfo->subType->type != TypeInfo::TYPE_COMPLEX && second->typeInfo->type != TypeInfo::TYPE_COMPLEX);

	if(second->typeInfo == typeVoid)
		ThrowError(CodeInfo::lastKnownStartPos, "ERROR: cannot convert from void to %s", typeInfo->GetFullTypeName());
	if(typeInfo == typeVoid)
		ThrowError(CodeInfo::lastKnownStartPos, "ERROR: cannot convert from %s to void", second->typeInfo->GetFullTypeName());
	
	// If types don't match
	if(second->typeInfo != typeInfo)
	{
		// If it is not build-in basic types or if pointers point to different types
		if(typeInfo->type == TypeInfo::TYPE_COMPLEX || second->typeInfo->type == TypeInfo::TYPE_COMPLEX || typeInfo->subType != second->typeInfo->subType)
		{
			if(!(typeInfo->arrLevel != 0 && second->typeInfo->arrLevel == 0 && arrSetAll))
				ThrowError(CodeInfo::lastKnownStartPos, "ERROR: cannot convert '%s' to '%s'", second->typeInfo->GetFullTypeName(), typeInfo->GetFullTypeName());
		}
	}

	absAddress = true;
	knownAddress = false;
	addrShift = 0;

	if(first->nodeType == typeNodeGetAddress)
	{
		absAddress = static_cast<NodeGetAddress*>(first)->IsAbsoluteAddress();
		addrShift = static_cast<NodeGetAddress*>(first)->varAddress;
		knownAddress = true;
	}
#ifndef NULLC_ENABLE_C_TRANSLATION
	if(first->nodeType == typeNodeShiftAddress)
	{
		addrShift = static_cast<NodeShiftAddress*>(first)->memberShift;
		first = static_cast<NodeShiftAddress*>(first)->first;
	}
	if(first->nodeType == typeNodeArrayIndex && static_cast<NodeArrayIndex*>(first)->knownShift)
	{
		addrShift = static_cast<NodeArrayIndex*>(first)->shiftValue;
		first = static_cast<NodeArrayIndex*>(first)->first;
	}
#endif

	if(arrSetAll)
	{
		elemCount = typeInfo->size / typeInfo->subType->size;
		typeInfo = typeInfo->subType;
	}

	nodeType = typeNodeVariableSet;
}

NodeVariableSet::~NodeVariableSet()
{
}


void NodeVariableSet::Compile()
{
	if(sourcePos)
		cmdInfoList.AddDescription(cmdList.size(), sourcePos);

	CompileExtra();

	asmStackType asmST = typeInfo->stackType;
	asmDataType asmDT = typeInfo->dataType;

	second->Compile();
	ConvertFirstToSecond(second->typeInfo->stackType, asmST);

	if(!knownAddress)
		first->Compile();
	if(arrSetAll)
	{
		assert(knownAddress);
		cmdList.push_back(VMCmd(cmdPushImmt, elemCount));
		cmdList.push_back(VMCmd(cmdSetRange, absAddress ? ADDRESS_ABOLUTE : ADDRESS_RELATIVE, (unsigned short)(asmDT), addrShift));
	}else{
		if(asmDT == DTYPE_COMPLEX_TYPE && typeInfo->size == 8)
			asmDT = DTYPE_LONG;
		if(knownAddress)
		{
			cmdList.push_back(VMCmd(cmdMovType[asmDT>>2], absAddress ? ADDRESS_ABOLUTE : ADDRESS_RELATIVE, (unsigned short)typeInfo->size, addrShift));
		}else{
			cmdList.push_back(VMCmd(cmdMovTypeStk[asmDT>>2], asmST == STYPE_DOUBLE ? 1 : 0, (unsigned short)typeInfo->size, addrShift));
		}
	}
}

//////////////////////////////////////////////////////////////////////////
// Node to change variable value with following operations: += -= *= /= **=

NodeVariableModify::NodeVariableModify(TypeInfo* targetType, CmdID cmd)
{
	assert(targetType);
	typeInfo = targetType->subType;

	cmdID = cmd;

	second = TakeLastNode();

	// Address of the target variable
	first = TakeLastNode();
	assert(first->typeInfo->refLevel != 0);

	if(second->typeInfo == typeVoid)
		ThrowError(CodeInfo::lastKnownStartPos, "ERROR: cannot convert from void to %s", typeInfo->GetFullTypeName());
	if(typeInfo == typeVoid)
		ThrowError(CodeInfo::lastKnownStartPos, "ERROR: cannot convert from %s to void", second->typeInfo->GetFullTypeName());
	if(first->typeInfo->subType->refLevel != 0 || second->typeInfo->refLevel != 0 || typeInfo->type == TypeInfo::TYPE_COMPLEX || second->typeInfo->type == TypeInfo::TYPE_COMPLEX)
		ThrowError(CodeInfo::lastKnownStartPos, "ERROR: there is no build-in operator for types '%s' and '%s'", typeInfo->GetFullTypeName(), second->typeInfo->GetFullTypeName());

	// If types don't match
	if(second->typeInfo != typeInfo)
	{
		// If it is not build-in basic types or if pointers point to different types
		if(typeInfo->type == TypeInfo::TYPE_COMPLEX || second->typeInfo->type == TypeInfo::TYPE_COMPLEX || typeInfo->subType != second->typeInfo->subType)
			ThrowError(CodeInfo::lastKnownStartPos, "ERROR: cannot convert '%s' to '%s'", second->typeInfo->GetFullTypeName(), typeInfo->GetFullTypeName());
	}

	absAddress = true;
	knownAddress = false;
	addrShift = 0;

#ifndef NULLC_ENABLE_C_TRANSLATION
	if(first->nodeType == typeNodeGetAddress)
	{
		absAddress = static_cast<NodeGetAddress*>(first)->IsAbsoluteAddress();
		addrShift = static_cast<NodeGetAddress*>(first)->varAddress;
		knownAddress = true;
	}
	if(first->nodeType == typeNodeShiftAddress)
	{
		addrShift = static_cast<NodeShiftAddress*>(first)->memberShift;
		first = static_cast<NodeShiftAddress*>(first)->first;
	}
	if(first->nodeType == typeNodeArrayIndex && static_cast<NodeArrayIndex*>(first)->knownShift)
	{
		addrShift = static_cast<NodeArrayIndex*>(first)->shiftValue;
		first = static_cast<NodeArrayIndex*>(first)->first;
	}
#endif

	nodeType = typeNodeVariableModify;
}

NodeVariableModify::~NodeVariableModify()
{
}

void NodeVariableModify::Compile()
{
	if(sourcePos)
		cmdInfoList.AddDescription(cmdList.size(), sourcePos);

	CompileExtra();

	asmStackType asmSTfirst = typeInfo->stackType;
	asmDataType asmDT = typeInfo->dataType;

	asmStackType asmSTsecond = second->typeInfo->stackType;

	// Calculate address of the first operand, if it isn't known
	if(!knownAddress)
		first->Compile();

	// Put first operand on top of the stack
	if(knownAddress)
		cmdList.push_back(VMCmd(cmdPushType[asmDT>>2], absAddress ? ADDRESS_ABOLUTE : ADDRESS_RELATIVE, (unsigned short)typeInfo->size, addrShift));
	else
		cmdList.push_back(VMCmd(cmdPushTypeStk[asmDT>>2], asmDT == DTYPE_DOUBLE ? 1 : 0, (unsigned short)typeInfo->size, addrShift));

	// Convert it to the type that results from operation made between two operands.
	asmStackType asmSTresult = ConvertFirstForSecond(asmSTfirst, asmSTsecond);

	// Calculate second operand value
	second->Compile();

	// Convert it to the type that results from operation made between two operands.
	ConvertFirstForSecond(asmSTsecond, asmSTresult);

	// Make a binary operation of corresponding type
	if(asmSTresult == STYPE_INT)
		cmdList.push_back(VMCmd((InstructionCode)(cmdID)));
	else if(asmSTresult == STYPE_LONG)
		cmdList.push_back(VMCmd((InstructionCode)(cmdID - cmdAdd + cmdAddL)));
	else if(asmSTresult == STYPE_DOUBLE)
		cmdList.push_back(VMCmd((InstructionCode)(cmdID - cmdAdd + cmdAddD)));
	else
		assert(!"unknown operator type in NodeVariableModify");

	// Convert to the type of first operand
	ConvertFirstToSecond(asmSTresult, asmSTfirst);

	// Calculate address of the first operand, if it isn't known
	if(!knownAddress)
		first->Compile();

	if(asmDT == DTYPE_COMPLEX_TYPE && typeInfo->size == 8)
		asmDT = DTYPE_LONG;
	// Put first operand on top of the stack
	if(knownAddress)
	{
		cmdList.push_back(VMCmd(cmdMovType[asmDT>>2], absAddress ? ADDRESS_ABOLUTE : ADDRESS_RELATIVE, (unsigned short)typeInfo->size, addrShift));
	}else{
		cmdList.push_back(VMCmd(cmdMovTypeStk[asmDT>>2], asmDT == DTYPE_DOUBLE ? 1 : 0, (unsigned short)typeInfo->size, addrShift));
	}
}

//////////////////////////////////////////////////////////////////////////
// Node that calculates address of the array element

NodeArrayIndex::NodeArrayIndex(TypeInfo* parentType)
{
	assert(parentType);
	typeParent = parentType;
	typeInfo = CodeInfo::GetReferenceType(parentType->subType);

	// Node that calculates array index
	second = TakeLastNode();
	if(second->typeInfo->type == TypeInfo::TYPE_COMPLEX || second->typeInfo->type == TypeInfo::TYPE_VOID)
		ThrowError(CodeInfo::lastKnownStartPos, "ERROR: cannot index array with type '%s'", second->typeInfo->GetFullTypeName());

	// Node that calculates address of the first array element
	first = TakeLastNode();

	shiftValue = 0;
	knownShift = false;

	if(second->nodeType == typeNodeNumber && typeParent->arrSize != TypeInfo::UNSIZED_ARRAY)
	{
		shiftValue = static_cast<NodeNumber*>(second)->GetInteger();
		// Check bounds
		if(shiftValue < 0)
			ThrowError(CodeInfo::lastKnownStartPos, "ERROR: array index cannot be negative");
		if((unsigned int)shiftValue >= typeParent->arrSize)
			ThrowError(CodeInfo::lastKnownStartPos, "ERROR: array index out of bounds");
		shiftValue *= typeParent->subType->size;
		knownShift = true;
	}
	if(!knownShift && typeParent->subType->size > 65535)
		ThrowError(CodeInfo::lastKnownStartPos, "ERROR: cannot index array when sizeof(%s) exceeds 65535 bytes", typeParent->subType->GetFullTypeName());

	nodeType = typeNodeArrayIndex;
}

NodeArrayIndex::~NodeArrayIndex()
{
}

void NodeArrayIndex::Compile()
{
	if(sourcePos)
		cmdInfoList.AddDescription(cmdList.size(), sourcePos);

	CompileExtra();

	// Get address of the first array element
	first->Compile();

	if(knownShift)
	{
		cmdList.push_back(VMCmd(cmdPushImmt, shiftValue));
		// Add it to the address of the first element
		cmdList.push_back(VMCmd(cmdAdd));
	}else{
		// Compute index value
		second->Compile();
		// Convert it to integer and multiply by the size of the element
		if(second->typeInfo->stackType != STYPE_INT)
			cmdList.push_back(VMCmd(second->typeInfo->stackType == STYPE_DOUBLE ? cmdDtoI : cmdLtoI));
		cmdList.push_back(VMCmd(typeParent->arrSize == TypeInfo::UNSIZED_ARRAY ? cmdIndexStk : cmdIndex, (unsigned short)typeParent->subType->size, typeParent->arrSize));
	}
}

//////////////////////////////////////////////////////////////////////////
// Node to get value by address (dereference pointer)

NodeDereference::NodeDereference(FunctionInfo* setClosure, unsigned int offsetToPrevClosure, bool isReadonly)
{
	originalNode = first = TakeLastNode();
	assert(first->typeInfo);
	assert(first->typeInfo->subType);
	typeInfo = first->typeInfo->subType;

	absAddress = true;
	knownAddress = false;
	addrShift = 0;
	closureFunc = setClosure;
	offsetToPreviousClosure = offsetToPrevClosure;
	neutralized = false;
	readonly = isReadonly;

#ifndef NULLC_ENABLE_C_TRANSLATION
	if(first->nodeType == typeNodeGetAddress)
	{
		absAddress = static_cast<NodeGetAddress*>(first)->IsAbsoluteAddress();
		addrShift = static_cast<NodeGetAddress*>(first)->varAddress;
		knownAddress = true;
	}
	if(first->nodeType == typeNodeShiftAddress)
	{
		addrShift = static_cast<NodeShiftAddress*>(first)->memberShift;
		first = static_cast<NodeShiftAddress*>(first)->first;
	}
	if(first->nodeType == typeNodeArrayIndex && static_cast<NodeArrayIndex*>(first)->knownShift)
	{
		addrShift = static_cast<NodeArrayIndex*>(first)->shiftValue;
		first = static_cast<NodeArrayIndex*>(first)->first;
	}
#endif

	nodeType = typeNodeDereference;
}

NodeDereference::~NodeDereference()
{
}

void NodeDereference::Neutralize()
{
	if(readonly)
		ThrowError(CodeInfo::lastKnownStartPos, "ERROR: cannot take pointer to a read-only variable");
	neutralized = true;
	typeInfo = originalNode->typeInfo;
}

void NodeDereference::Compile()
{
	if(sourcePos)
		cmdInfoList.AddDescription(cmdList.size(), sourcePos);

	CompileExtra();

	if(typeInfo->size == 0)
		return;
	asmDataType asmDT = typeInfo->dataType;

	if(neutralized)
	{
		originalNode->Compile();
	}else{
		if(knownAddress || first != originalNode)
			originalNode->CompileExtra();
		if(closureFunc)
		{
			first->Compile();
			cmdList.push_back(VMCmd(cmdCreateClosure, (unsigned short)offsetToPreviousClosure, CodeInfo::FindFunctionByPtr(closureFunc)));
		}else{
			if(!knownAddress)
				first->Compile();

			if(asmDT == DTYPE_COMPLEX_TYPE && typeInfo->size == 8)
				asmDT = DTYPE_LONG;
			if(knownAddress)
				cmdList.push_back(VMCmd(cmdPushType[asmDT>>2], absAddress ? ADDRESS_ABOLUTE : ADDRESS_RELATIVE, (unsigned short)typeInfo->size, addrShift));
			else
				cmdList.push_back(VMCmd(cmdPushTypeStk[asmDT>>2], asmDT == DTYPE_DOUBLE ? 1 : 0, (unsigned short)typeInfo->size, addrShift));
		}
	}
}

//////////////////////////////////////////////////////////////////////////
// Node that shifts address to the class member

NodeShiftAddress::NodeShiftAddress(TypeInfo::MemberVariable *classMember)
{
	member = classMember;

	memberShift = member->offset;
	typeInfo = CodeInfo::GetReferenceType(member->type);

	first = TakeLastNode();

#ifndef NULLC_ENABLE_C_TRANSLATION
	if(first->nodeType == typeNodeShiftAddress)
	{
		memberShift += static_cast<NodeShiftAddress*>(first)->memberShift;
		first = static_cast<NodeShiftAddress*>(first)->first;
	}
#endif

	nodeType = typeNodeShiftAddress;
}

NodeShiftAddress::~NodeShiftAddress()
{
}


void NodeShiftAddress::Compile()
{
	if(sourcePos)
		cmdInfoList.AddDescription(cmdList.size(), sourcePos);

	CompileExtra();

	// Get variable address
	first->Compile();

	if(memberShift)
	{
		cmdList.push_back(VMCmd(cmdPushImmt, memberShift));
		// Add the shift to the address
		cmdList.push_back(VMCmd(cmdAdd));
	}
}

//////////////////////////////////////////////////////////////////////////
// Node for increment and decrement operations

NodePreOrPostOp::NodePreOrPostOp(bool isInc, bool preOp)
{
	first = TakeLastNode();
	assert(first->typeInfo->refLevel != 0);
	typeInfo = first->typeInfo->subType;

	incOp = isInc;

	if(typeInfo->type == TypeInfo::TYPE_COMPLEX || typeInfo->refLevel != 0)
		ThrowError(CodeInfo::lastKnownStartPos, "ERROR: %s is not supported on '%s'", (isInc ? "increment" : "decrement"), typeInfo->GetFullTypeName());

	prefixOp = preOp;

	optimised = false;

	absAddress = true;
	knownAddress = false;
	addrShift = 0;

#ifndef NULLC_ENABLE_C_TRANSLATION
	if(first->nodeType == typeNodeGetAddress)
	{
		absAddress = static_cast<NodeGetAddress*>(first)->IsAbsoluteAddress();
		addrShift = static_cast<NodeGetAddress*>(first)->varAddress;
		knownAddress = true;
	}
	if(first->nodeType == typeNodeShiftAddress)
	{
		addrShift = static_cast<NodeShiftAddress*>(first)->memberShift;
		first = static_cast<NodeShiftAddress*>(first)->first;
	}
	if(first->nodeType == typeNodeArrayIndex && static_cast<NodeArrayIndex*>(first)->knownShift)
	{
		addrShift = static_cast<NodeArrayIndex*>(first)->shiftValue;
		first = static_cast<NodeArrayIndex*>(first)->first;
	}
#endif

	nodeType = typeNodePreOrPostOp;
}

NodePreOrPostOp::~NodePreOrPostOp()
{
}


void NodePreOrPostOp::SetOptimised(bool doOptimisation)
{
	optimised = doOptimisation;
}


void NodePreOrPostOp::Compile()
{
	if(sourcePos)
		cmdInfoList.AddDescription(cmdList.size(), sourcePos);

	CompileExtra();

	asmStackType asmST = typeInfo->stackType;
	asmDataType asmDT = typeInfo->dataType;
	asmOperType aOT = operTypeForStackType[typeInfo->stackType];

	InstructionCode pushCmd = knownAddress ? cmdPushType[asmDT>>2] : cmdPushTypeStk[asmDT>>2];
	InstructionCode movCmd = knownAddress ? cmdMovType[asmDT>>2] : cmdMovTypeStk[asmDT>>2];
	if(!knownAddress)
		first->Compile();
	cmdList.push_back(VMCmd(pushCmd, absAddress ? ADDRESS_ABOLUTE : ADDRESS_RELATIVE, (unsigned short)typeInfo->size, addrShift));
	cmdList.push_back(VMCmd(incOp ? cmdIncType[aOT] : cmdDecType[aOT]));
	if(!knownAddress)
		first->Compile();
	cmdList.push_back(VMCmd(movCmd, absAddress ? ADDRESS_ABOLUTE : ADDRESS_RELATIVE, (unsigned short)typeInfo->size, addrShift));
	if(!prefixOp && !optimised)
		cmdList.push_back(VMCmd(!incOp ? cmdIncType[aOT] : cmdDecType[aOT]));
	if(optimised)
		cmdList.push_back(VMCmd(cmdPop, stackTypeSize[asmST]));
}

//////////////////////////////////////////////////////////////////////////
// Node that gets function address

NodeFunctionAddress::NodeFunctionAddress(FunctionInfo* functionInfo)
{
	funcInfo = functionInfo;
	typeInfo = funcInfo->funcType;

	if(funcInfo->type != FunctionInfo::NORMAL)
		first = TakeLastNode();

	nodeType = typeNodeFunctionAddress;
}

NodeFunctionAddress::~NodeFunctionAddress()
{
}


void NodeFunctionAddress::Compile()
{
	if(sourcePos)
		cmdInfoList.AddDescription(cmdList.size(), sourcePos);

	CompileExtra();

	unsigned int ID = CodeInfo::FindFunctionByPtr(funcInfo);
	cmdList.push_back(VMCmd(cmdFuncAddr, ID));

	if(funcInfo->type == FunctionInfo::NORMAL)
	{
		NodeNumber nullPtr = NodeNumber(0ll, CodeInfo::GetReferenceType(typeVoid));
		nullPtr.Compile();
	}else{
		first->Compile();
	}
}

//////////////////////////////////////////////////////////////////////////
// Node that applies binary operation on two values

NodeBinaryOp::NodeBinaryOp(CmdID cmd)
{
	// Binary operation
	cmdID = cmd;

	second = TakeLastNode();
	first = TakeLastNode();

	bool logicalOp = (cmd >= cmdLess && cmd <= cmdNEqual) || (cmd >= cmdLogAnd && cmd <= cmdLogXor);

	// Binary operations on complex types are not present at the moment
	if(first->typeInfo->type == TypeInfo::TYPE_COMPLEX || second->typeInfo->type == TypeInfo::TYPE_COMPLEX)
		ThrowError(CodeInfo::lastKnownStartPos, "ERROR: operation %s is not supported on '%s' and '%s'", binCommandToText[cmdID - cmdAdd], first->typeInfo->GetFullTypeName(), second->typeInfo->GetFullTypeName());
	if((first->typeInfo->refLevel != 0 || second->typeInfo->refLevel != 0) && !(first->typeInfo->refLevel == second->typeInfo->refLevel && logicalOp))
		ThrowError(CodeInfo::lastKnownStartPos, "ERROR: operation %s is not supported on '%s' and '%s'", binCommandToText[cmdID - cmdAdd], first->typeInfo->GetFullTypeName(), second->typeInfo->GetFullTypeName());
	
	if(first->typeInfo == typeVoid)
		ThrowError(CodeInfo::lastKnownStartPos, "ERROR: first operator returns void");
	if(second->typeInfo == typeVoid)
		ThrowError(CodeInfo::lastKnownStartPos, "ERROR: second operator returns void");

	if((first->typeInfo == typeDouble || first->typeInfo == typeFloat || second->typeInfo == typeDouble || second->typeInfo == typeFloat) && (cmd >= cmdShl && cmd <= cmdLogXor))
		ThrowError(CodeInfo::lastKnownStartPos, "ERROR: binary operations are not available on floating-point numbers");

	if(typeVoid->refType && first->typeInfo == typeVoid->refType)
	{
		first->typeInfo = second->typeInfo;
	}
	if(typeVoid->refType && second->typeInfo == typeVoid->refType)
	{
		second->typeInfo = first->typeInfo;
	}

	// Find the type or resulting value
	typeInfo = ChooseBinaryOpResultType(first->typeInfo, second->typeInfo);

	if(first->nodeType == typeNodeNumber && first->typeInfo != typeInfo)
		((NodeNumber*)first)->ConvertTo(typeInfo);
	if(second->nodeType == typeNodeNumber && second->typeInfo != typeInfo)
		((NodeNumber*)second)->ConvertTo(typeInfo);

	typeInfo = logicalOp ? typeInt : typeInfo;

	nodeType = typeNodeBinaryOp;
}
NodeBinaryOp::~NodeBinaryOp()
{
}

void NodeBinaryOp::Compile()
{
	asmStackType fST = first->typeInfo->stackType, sST = second->typeInfo->stackType;
	
	CompileExtra();

	if(cmdID == cmdLogOr || cmdID == cmdLogAnd)
	{
		first->Compile();
		// Convert long to int with | operation between parts of long (otherwise, we would've truncated 64 bit value)
		if(fST == STYPE_LONG)
			cmdList.push_back(VMCmd(cmdBitOr));

		// If it's operator || and first argument is true, jump to push 1 as result
		// If it's operator && and first argument is false, jump to push 0 as result
		cmdList.push_back(VMCmd(cmdID == cmdLogOr ? cmdJmpNZ : cmdJmpZ, ~0u));	// Jump address will be fixed later on
		unsigned int specialJmp1 = cmdList.size() - 1;

		second->Compile();
		if(sST == STYPE_LONG)
			cmdList.push_back(VMCmd(cmdBitOr));

		// If it's operator || and first argument is true, jump to push 1 as result
		// If it's operator && and first argument is false, jump to push 0 as result
		cmdList.push_back(VMCmd(cmdID == cmdLogOr ? cmdJmpNZ : cmdJmpZ, ~0u));	// Jump address will be fixed later on
		unsigned int specialJmp2 = cmdList.size() - 1;

		// If it's operator ||, result is zero, and if it's operator &&, result is 1
		cmdList.push_back(VMCmd(cmdPushImmt, cmdID == cmdLogOr ? 0 : 1));

		// Skip command that sets opposite result
		cmdList.push_back(VMCmd(cmdJmp, cmdList.size() + 2));
		// Fix up jumps
		cmdList[specialJmp1].argument = cmdList.size();
		cmdList[specialJmp2].argument = cmdList.size();
		// If it's early jump, for operator ||, result is one, and if it's operator &&, result is 0
		cmdList.push_back(VMCmd(cmdPushImmt, cmdID == cmdLogOr ? 1 : 0));
	}else{
		// Compute first value
		first->Compile();
		// Convert it to the resulting type
		fST = ConvertFirstForSecond(fST, sST);

		// Compute second value
		second->Compile();
		// Convert it to the result type
		sST = ConvertFirstForSecond(sST, fST);

		// Apply binary operation
		if(fST == STYPE_INT)
			cmdList.push_back(VMCmd((InstructionCode)(cmdID)));
		else if(fST == STYPE_LONG)
			cmdList.push_back(VMCmd((InstructionCode)(cmdID - cmdAdd + cmdAddL)));
		else if(fST == STYPE_DOUBLE)
			cmdList.push_back(VMCmd((InstructionCode)(cmdID - cmdAdd + cmdAddD)));
		else
			assert(!"unknown operator type in NodeTwoAndCmdOp");
	}
}

//////////////////////////////////////////////////////////////////////////
// Node for compilation of if(){}else{} statement and conditional operator ?:

NodeIfElseExpr::NodeIfElseExpr(bool haveElse, bool isTerm)
{
	// If else block is present
	if(haveElse)
		third = TakeLastNode();

	second = TakeLastNode();
	first = TakeLastNode();

	if((first->typeInfo->type == TypeInfo::TYPE_COMPLEX && first->typeInfo != typeObject) || first->typeInfo->type == TypeInfo::TYPE_VOID)
		ThrowError(CodeInfo::lastKnownStartPos, "ERROR: condition type cannot be '%s'", first->typeInfo->GetFullTypeName());
	// If it is a conditional operator, the there is a resulting type different than void
	if(isTerm)
		typeInfo = second->typeInfo != third->typeInfo ? ChooseBinaryOpResultType(second->typeInfo, third->typeInfo) : second->typeInfo;

	nodeType = typeNodeIfElseExpr;
}
NodeIfElseExpr::~NodeIfElseExpr()
{
}

void NodeIfElseExpr::Compile()
{
	if(sourcePos)
		cmdInfoList.AddDescription(cmdList.size(), sourcePos);

	CompileExtra();

	// Child node structure: if(first) second; else third;
	// Or, for conditional operator: first ? second : third;

	// Compute condition
	first->Compile();

	if(first->typeInfo == typeObject)
	{
		cmdList.push_back(VMCmd(cmdPop, 4)); // remove 'typeid', it's irrelevant
#ifdef _M_X64
		cmdList.push_back(VMCmd(cmdBitOr)); // cmdJmpZ checks int, but we have long pointer
#endif
	}else if(first->typeInfo->stackType != STYPE_INT){
		cmdList.push_back(VMCmd(first->typeInfo->stackType == STYPE_DOUBLE ? cmdDtoI : cmdBitOr));
	}
	// If false, jump to 'else' block, or out of statement, if there is no 'else'
	cmdList.push_back(VMCmd(cmdJmpZ, ~0u));	// Jump address will be fixed later on
	unsigned int jmpOnFalse = cmdList.size()-1;

	// Compile block for condition == true
	second->Compile();
	if(typeInfo != typeVoid)
		ConvertFirstForSecond(second->typeInfo->stackType, third->typeInfo->stackType);

	cmdList[jmpOnFalse].argument = cmdList.size();	// Fixup jump address
	// If 'else' block is present, compile it
	if(third)
	{
		// Put jump to exit statement at the end of main block
		cmdList.push_back(VMCmd(cmdJmp, ~0u));	// Jump address will be fixed later on
		unsigned int jmpToEnd = cmdList.size()-1;

		cmdList[jmpOnFalse].argument = cmdList.size();	// Fixup jump address

		// Compile block for condition == false
		third->Compile();
		if(typeInfo != typeVoid)
			ConvertFirstForSecond(third->typeInfo->stackType, second->typeInfo->stackType);

		cmdList[jmpToEnd].argument = cmdList.size();	// Fixup jump address
	}
}

//////////////////////////////////////////////////////////////////////////
// Nod for compilation of for(){}

unsigned int	currLoopDepth = 0;

NodeForExpr::NodeForExpr()
{
	fourth = TakeLastNode();
	third = TakeLastNode();
	second = TakeLastNode();
	first = TakeLastNode();

	if((second->typeInfo->type == TypeInfo::TYPE_COMPLEX && second->typeInfo != typeObject) || second->typeInfo->type == TypeInfo::TYPE_VOID)
		ThrowError(CodeInfo::lastKnownStartPos, "ERROR: condition type cannot be '%s'", second->typeInfo->GetFullTypeName());

	nodeType = typeNodeForExpr;
}
NodeForExpr::~NodeForExpr()
{
}

void NodeForExpr::Compile()
{
	if(sourcePos)
		cmdInfoList.AddDescription(cmdList.size(), sourcePos);

	CompileExtra();

	currLoopDepth++;

	// Child node structure: for(first, second, third) fourth;

	// Compile initialization node
	first->Compile();
	unsigned int posTestExpr = cmdList.size();

	// Compute condition value
	second->Compile();
	if(second->typeInfo == typeObject)
	{
		cmdList.push_back(VMCmd(cmdPop, 4)); // remove 'typeid', it's irrelevant
#ifdef _M_X64
		cmdList.push_back(VMCmd(cmdBitOr)); // cmdJmpZ checks int, but we have long pointer
#endif
	}else if(second->typeInfo->stackType != STYPE_INT){
		cmdList.push_back(VMCmd(second->typeInfo->stackType == STYPE_DOUBLE ? cmdDtoI : cmdBitOr));
	}

	// If condition == false, exit loop
	unsigned int exitJmp = cmdList.size();
	cmdList.push_back(VMCmd(cmdJmpZ, 0));

	// Compile loop contents
	fourth->Compile();

	unsigned int posPostOp = cmdList.size();
	// Compile operation, executed after each cycle
	third->Compile();
	// Jump to condition check
	cmdList.push_back(VMCmd(cmdJmp, posTestExpr));

	cmdList[exitJmp].argument = cmdList.size();
	NodeContinueOp::SatisfyJumps(posPostOp);
	NodeBreakOp::SatisfyJumps(cmdList.size());

	currLoopDepth--;
}

//////////////////////////////////////////////////////////////////////////
// Node for compilation of while(){}

NodeWhileExpr::NodeWhileExpr()
{
	second = TakeLastNode();
	first = TakeLastNode();

	if((first->typeInfo->type == TypeInfo::TYPE_COMPLEX && first->typeInfo != typeObject) || first->typeInfo->type == TypeInfo::TYPE_VOID)
		ThrowError(CodeInfo::lastKnownStartPos, "ERROR: condition type cannot be '%s'", first->typeInfo->GetFullTypeName());

	nodeType = typeNodeWhileExpr;
}
NodeWhileExpr::~NodeWhileExpr()
{
}

void NodeWhileExpr::Compile()
{
	if(sourcePos)
		cmdInfoList.AddDescription(cmdList.size(), sourcePos);

	CompileExtra();

	currLoopDepth++;
	// Child node structure: while(first) second;

	unsigned int posStart = cmdList.size();
	// Compute condition value
	first->Compile();

	// If branching by 'auto ref'
	if(first->typeInfo == typeObject)
	{
		cmdList.push_back(VMCmd(cmdPop, 4)); // remove 'typeid', it's irrelevant
#ifdef _M_X64
		cmdList.push_back(VMCmd(cmdBitOr)); // cmdJmpZ checks int, but we have long pointer
#endif
	}else if(first->typeInfo->stackType != STYPE_INT){
		cmdList.push_back(VMCmd(first->typeInfo->stackType == STYPE_DOUBLE ? cmdDtoI : cmdBitOr));
	}

	// If condition == false, exit loop
	unsigned int exitJmp = cmdList.size();
	cmdList.push_back(VMCmd(cmdJmpZ, 0));

	// Compile loop contents
	second->Compile();

	// Jump to condition check
	cmdList.push_back(VMCmd(cmdJmp, posStart));

	cmdList[exitJmp].argument = cmdList.size();
	NodeContinueOp::SatisfyJumps(posStart);
	NodeBreakOp::SatisfyJumps(cmdList.size());

	currLoopDepth--;
}

//////////////////////////////////////////////////////////////////////////
// Node for compilation of do{}while()

NodeDoWhileExpr::NodeDoWhileExpr()
{
	second = TakeLastNode();
	first = TakeLastNode();

	if((second->typeInfo->type == TypeInfo::TYPE_COMPLEX && second->typeInfo != typeObject) || second->typeInfo->type == TypeInfo::TYPE_VOID)
		ThrowError(CodeInfo::lastKnownStartPos, "ERROR: condition type cannot be '%s'", second->typeInfo->GetFullTypeName());

	nodeType = typeNodeDoWhileExpr;
}
NodeDoWhileExpr::~NodeDoWhileExpr()
{
}

void NodeDoWhileExpr::Compile()
{
	// Child node structure: do{ first; }while(second)

	CompileExtra();

	currLoopDepth++;

	unsigned int posStart = cmdList.size();
	// Compile loop contents
	first->Compile();

	if(sourcePos)
		cmdInfoList.AddDescription(cmdList.size(), sourcePos);

	unsigned int posCond = cmdList.size();
	// Compute condition value
	second->Compile();
	if(second->typeInfo == typeObject)
	{
		cmdList.push_back(VMCmd(cmdPop, 4)); // remove 'typeid', it's irrelevant
#ifdef _M_X64
		cmdList.push_back(VMCmd(cmdBitOr)); // cmdJmpZ checks int, but we have long pointer
#endif
	}else if(second->typeInfo->stackType != STYPE_INT){
		cmdList.push_back(VMCmd(second->typeInfo->stackType == STYPE_DOUBLE ? cmdDtoI : cmdBitOr));
	}

	// Jump to beginning if condition == true
	cmdList.push_back(VMCmd(cmdJmpNZ, posStart));

	NodeContinueOp::SatisfyJumps(posCond);
	NodeBreakOp::SatisfyJumps(cmdList.size());

	currLoopDepth--;
}

//////////////////////////////////////////////////////////////////////////
void SatisfyJumps(FastVector<unsigned int>& jumpList, unsigned int pos)
{
	for(unsigned int i = 0; i < jumpList.size();)
	{
		if(cmdList[jumpList[i]].argument == currLoopDepth)
		{
			// If level is equal to 1, replace it with jump position
			cmdList[jumpList[i]].argument = pos;
			// Remove element by replacing with the last one
			jumpList[i] = jumpList.back();
			jumpList.pop_back();
		}else{
			i++;
		}
	}
}

//////////////////////////////////////////////////////////////////////////
// Node for break operation

FastVector<unsigned int>	NodeBreakOp::fixQueue;

NodeBreakOp::NodeBreakOp(unsigned int brDepth)
{
	nodeType = typeNodeBreakOp;

	breakDepth = brDepth;
}
NodeBreakOp::~NodeBreakOp()
{
}

void NodeBreakOp::Compile()
{
	if(sourcePos)
		cmdInfoList.AddDescription(cmdList.size(), sourcePos);

	CompileExtra();

	// Break the loop
	fixQueue.push_back(cmdList.size());
	cmdList.push_back(VMCmd(cmdJmp, currLoopDepth - breakDepth + 1));
}

void NodeBreakOp::SatisfyJumps(unsigned int pos)
{
	::SatisfyJumps(fixQueue, pos);
}

//////////////////////////////////////////////////////////////////////////
// Node for continue operation

FastVector<unsigned int>	NodeContinueOp::fixQueue;

NodeContinueOp::NodeContinueOp(unsigned int contDepth)
{
	nodeType = typeNodeContinueOp;

	continueDepth = contDepth;
}
NodeContinueOp::~NodeContinueOp()
{
}

void NodeContinueOp::Compile()
{
	if(sourcePos)
		cmdInfoList.AddDescription(cmdList.size(), sourcePos);

	CompileExtra();

	// Continue the loop
	fixQueue.push_back(cmdList.size());
	cmdList.push_back(VMCmd(cmdJmp, currLoopDepth - continueDepth + 1));
}

void NodeContinueOp::SatisfyJumps(unsigned int pos)
{
	::SatisfyJumps(fixQueue, pos);
}

//////////////////////////////////////////////////////////////////////////
// Node for compilation of switch

FastVector<unsigned int>	NodeSwitchExpr::fixQueue;

NodeSwitchExpr::NodeSwitchExpr()
{
	// Take node with value
	first = TakeLastNode();

	if((first->typeInfo->type == TypeInfo::TYPE_COMPLEX && first->typeInfo != typeTypeid) || first->typeInfo->type == TypeInfo::TYPE_VOID)
		ThrowError(CodeInfo::lastKnownStartPos, "ERROR: condition type cannot be '%s'", first->typeInfo->GetFullTypeName());

	conditionHead = conditionTail = NULL;
	blockHead = blockTail = NULL;
	defaultCase = NULL;
	caseCount = 0;

	nodeType = typeNodeSwitchExpr;
}
NodeSwitchExpr::~NodeSwitchExpr()
{
}

void NodeSwitchExpr::AddCase()
{
	caseCount++;
	// Take case block from the top
	if(blockTail)
	{
		blockTail->next = TakeLastNode();
		blockTail->next->prev = blockTail;
		blockTail = blockTail->next;
	}else{
		blockHead = blockTail = TakeLastNode();
	}
	// Take case condition from the top
	if(conditionTail)
	{
		conditionTail->next = TakeLastNode();
		conditionTail->next->prev = conditionTail;
		conditionTail = conditionTail->next;
	}else{
		conditionHead = conditionTail = TakeLastNode();
	}
}

void NodeSwitchExpr::AddDefault()
{
	defaultCase = TakeLastNode();
}

void NodeSwitchExpr::Compile()
{
	CompileExtra();

	currLoopDepth++;

	asmStackType aST = first->typeInfo->stackType;
	asmOperType aOT = operTypeForStackType[aST];

	unsigned int queueStart = fixQueue.size(), queueCurr = queueStart;

	// Compute value
	first->Compile();
	if(first->typeInfo == typeTypeid)
	{
		aST = STYPE_INT;
		aOT = OTYPE_INT;
	}

	NodeZeroOP *curr, *currBlock;

	// Generate code for all cases
	for(curr = conditionHead, currBlock = blockHead; curr; curr = curr->next, currBlock = currBlock->next)
	{
		if(aOT == OTYPE_INT)
			cmdList.push_back(VMCmd(cmdCopyI));
		else
			cmdList.push_back(VMCmd(cmdCopyDorL));

		curr->Compile();
		// Compare for equality
		if(aOT == OTYPE_INT)
			cmdList.push_back(VMCmd(cmdEqual));
		else if(aOT == OTYPE_DOUBLE)
			cmdList.push_back(VMCmd(cmdEqualD));
		else
			cmdList.push_back(VMCmd(cmdEqualL));
		// If equal, jump to corresponding case block
		fixQueue.push_back(cmdList.size());
		cmdList.push_back(VMCmd(cmdJmpNZ, 0));
	}
	// Remove value by which we switched from stack
	cmdList.push_back(VMCmd(cmdPop, stackTypeSize[aST]));

	fixQueue.push_back(cmdList.size());
	cmdList.push_back(VMCmd(cmdJmp, 0));
	for(curr = blockHead; curr; curr = curr->next)
	{
		cmdList[fixQueue[queueCurr++]].argument = cmdList.size();
		// Remove value by which we switched from stack
		cmdList.push_back(VMCmd(cmdPop, stackTypeSize[aST]));
		curr->Compile();
		if(curr != blockTail)
			cmdList.push_back(VMCmd(cmdJmp, cmdList.size() + 2));
	}
	cmdList[fixQueue[queueCurr++]].argument = cmdList.size();
	if(defaultCase)
		defaultCase->Compile();

	for(unsigned int i = 0; i < NodeContinueOp::fixQueue.size(); i++)
	{
		if(cmdList[NodeContinueOp::fixQueue[i]].argument == 1)
			ThrowError(NULL, "ERROR: cannot continue inside switch");
	}
	fixQueue.shrink(queueStart);
	NodeBreakOp::SatisfyJumps(cmdList.size());

	currLoopDepth--;
}

//////////////////////////////////////////////////////////////////////////
// Node that contains list of expressions

NodeExpressionList::NodeExpressionList(TypeInfo *returnType)
{
	typeInfo = returnType;
	tail = first = TakeLastNode();

	nodeType = typeNodeExpressionList;
}
NodeExpressionList::~NodeExpressionList()
{
}

void NodeExpressionList::AddNode(bool reverse)
{
	// If reverse is set, add before the head
	if(reverse)
	{
		NodeZeroOP *firstNext = first;
		first = TakeLastNode();
		first->next = firstNext;
		first->next->prev = first;
	}else{
		tail->next = TakeLastNode();
		tail->next->prev = tail;
		tail = tail->next;
	}
}

NodeZeroOP* NodeExpressionList::GetFirstNode()
{
	assert(first);
	return first;
}

void NodeExpressionList::Compile()
{
	CompileExtra();

	NodeZeroOP	*curr = first;
	do 
	{
		curr->Compile();
		curr = curr->next;
	}while(curr);
}

void ResetTreeGlobals()
{
	currLoopDepth = 0;

#ifdef NULLC_ENABLE_C_TRANSLATION
	ResetTranslationState();
#endif

	NodeBreakOp::fixQueue.clear();
	NodeContinueOp::fixQueue.clear();
	NodeFuncCall::memoList.clear();
	NodeFuncCall::memoPool.Clear();
}
