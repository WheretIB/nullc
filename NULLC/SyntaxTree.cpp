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

static char* binCommandToText[] = { "+", "-", "*", "/", "^", "%", "<", ">", "<=", ">=", "==", "!=", "<<", ">>", "&", "|", "^", "and", "or", "xor"};
static char* unaryCommandToText[] = { "-", "-", "-", "~", "~", "!", "!" };

//////////////////////////////////////////////////////////////////////////

int	level = 0;
char	linePrefix[256];
unsigned int prefixSize = 2;

bool preNeedChange = false;
void GoDown()
{
	level++;
	prefixSize -= 2;
	linePrefix[prefixSize] = 0;
	sprintf(linePrefix + prefixSize, "  |__");
	prefixSize += 5;
}
void GoDownB()
{
	GoDown();
	preNeedChange = true;
}
void GoUp()
{
	level--;
	prefixSize -= 5;
	linePrefix[prefixSize] = 0;
	sprintf(linePrefix + prefixSize, "__");
	prefixSize += 2;
}
void DrawLine(FILE *fGraph)
{
	fprintf(fGraph, "%s", linePrefix);
	if(preNeedChange)
	{
		preNeedChange = false;
		GoUp();
		level++;

		prefixSize -= 2;
		linePrefix[prefixSize] = 0;
		sprintf(linePrefix + prefixSize, "   __");
		prefixSize += 5;
	}
}

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

ChunkedStackPool<4092>	NodeZeroOP::nodePool;

NodeZeroOP::NodeZeroOP()
{
	typeInfo = typeVoid;
	sourcePos = NULL;
	prev = next = NULL;
	nodeType = typeNodeZeroOp;
}
NodeZeroOP::NodeZeroOP(TypeInfo* tinfo)
{
	typeInfo = tinfo;
	sourcePos = NULL;
	prev = next = NULL;
	nodeType = typeNodeZeroOp;
}
NodeZeroOP::~NodeZeroOP()
{
}

void NodeZeroOP::Compile()
{
}
void NodeZeroOP::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s ZeroOp\r\n", typeInfo->GetFullTypeName());
}

void NodeZeroOP::SetCodeInfo(const char* newSourcePos)
{
	sourcePos = newSourcePos;
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
	first->Compile();
}
void NodeOneOP::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s OneOP :\r\n", typeInfo->GetFullTypeName());
	GoDown();
	first->LogToStream(fGraph);
	GoUp();
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
	NodeOneOP::Compile();
	second->Compile();
}
void NodeTwoOP::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s TwoOp :\r\n", typeInfo->GetFullTypeName());
	GoDown();
	first->LogToStream(fGraph);
	second->LogToStream(fGraph);
	GoUp();
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
	NodeTwoOP::Compile();
	third->Compile();
}
void NodeThreeOP::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s ThreeOp :\r\n", typeInfo->GetFullTypeName());
	GoDown();
	first->LogToStream(fGraph);
	second->LogToStream(fGraph);
	third->LogToStream(fGraph);
	GoUp();
}

//////////////////////////////////////////////////////////////////////////
// Node that puts a number on top of the stack

void NodeNumber::Compile()
{
	assert(typeInfo->size <= 8);
	if(typeInfo->stackType != STYPE_INT)
		cmdList.push_back(VMCmd(cmdPushImmt, num.quad.high));
	cmdList.push_back(VMCmd(cmdPushImmt, num.quad.low));
}
void NodeNumber::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s Number\r\n", typeInfo->GetFullTypeName());
}

bool NodeNumber::ConvertTo(TypeInfo *target)
{
	if(target == typeInt)
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

	// Child node computes value
	first->Compile();
	if(first->typeInfo != typeVoid)
	{
		// Removing it from top of the stack
		cmdList.push_back(VMCmd(cmdPop, first->typeInfo->type == TypeInfo::TYPE_COMPLEX ? first->typeInfo->size : stackTypeSize[first->typeInfo->stackType]));
	}
}
void NodePopOp::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s PopOp :\r\n", typeInfo->GetFullTypeName());
	GoDownB();
	first->LogToStream(fGraph);
	GoUp();
}

//////////////////////////////////////////////////////////////////////////
// Node that applies selected operation on value on top of the stack

NodeUnaryOp::NodeUnaryOp(CmdID cmd)
{
	// Unary operation
	cmdID = cmd;

	first = TakeLastNode();
	// Resulting type is the same as source type with exception for logical NOT
	typeInfo = cmd == cmdLogNot ? typeInt : first->typeInfo;

	if(first->typeInfo->refLevel != 0 || first->typeInfo->type == TypeInfo::TYPE_COMPLEX)
		ThrowError(CodeInfo::lastKnownStartPos, "ERROR: Unary operation '%s' is not supported on '%s'", unaryCommandToText[cmdID - cmdNeg], first->typeInfo->GetFullTypeName());

	nodeType = typeNodeUnaryOp;
}
NodeUnaryOp::~NodeUnaryOp()
{
}

void NodeUnaryOp::Compile()
{
	asmOperType aOT = operTypeForStackType[first->typeInfo->stackType];

	// Child node computes value
	first->Compile();
	// Execute command
	if(aOT == OTYPE_INT)
		cmdList.push_back(VMCmd((InstructionCode)cmdID));
	else if(aOT == OTYPE_LONG)
		cmdList.push_back(VMCmd((InstructionCode)(cmdID + 1)));
	else
		cmdList.push_back(VMCmd((InstructionCode)(cmdID + 2)));
}
void NodeUnaryOp::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s UnaryOp :\r\n", typeInfo->GetFullTypeName());
	GoDownB();
	first->LogToStream(fGraph);
	GoUp();
}

//////////////////////////////////////////////////////////////////////////
// Node that returns from function or program

NodeReturnOp::NodeReturnOp(bool localRet, TypeInfo* tinfo, FunctionInfo* parentFunc)
{
	localReturn = localRet;
	parentFunction = parentFunc;

	// Result type is set from outside
	typeInfo = tinfo;

	first = TakeLastNode();

	nodeType = typeNodeReturnOp;
}
NodeReturnOp::~NodeReturnOp()
{
}

void NodeReturnOp::Compile()
{
	if(sourcePos)
		cmdInfoList.AddDescription(cmdList.size(), sourcePos);

	// Compute value that we're going to return
	first->Compile();
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
	cmdList.push_back(VMCmd(cmdReturn, (unsigned char)operType, (unsigned short)localReturn, retSize));
}
void NodeReturnOp::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	if(typeInfo)
		fprintf(fGraph, "%s ReturnOp :\r\n", typeInfo->GetFullTypeName());
	else
		fprintf(fGraph, "%s ReturnOp :\r\n", first->typeInfo->GetFullTypeName());
	GoDownB();
	first->LogToStream(fGraph);
	GoUp();
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

	// Compute value that we're going to return
	first->Compile();
	if(parentFunction->closeUpvals)
		cmdList.push_back(VMCmd(cmdCloseUpvals, (unsigned short)CodeInfo::FindFunctionByPtr(parentFunction), stackFrameShift));
}

void NodeBlock::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s BlockOp (close upvalues from offset %d of function %s) %s:\r\n", first->typeInfo->GetFullTypeName(), stackFrameShift, parentFunction->name, parentFunction->closeUpvals ? "yes" : "no");
	GoDownB();
	first->LogToStream(fGraph);
	GoUp();
}
//////////////////////////////////////////////////////////////////////////
// Nodes that compiles function

NodeFuncDef::NodeFuncDef(FunctionInfo *info, unsigned int varShift)
{
	// Function description
	funcInfo = info;
	// Size of all local variables
	shift = varShift;

	disabled = false;

	first = TakeLastNode();

	nodeType = typeNodeFuncDef;
}
NodeFuncDef::~NodeFuncDef()
{
}

void NodeFuncDef::Disable()
{
	disabled = true;
}

void NodeFuncDef::Compile()
{
	if(disabled)
		return;
	if(sourcePos)
		cmdInfoList.AddDescription(cmdList.size(), sourcePos);

	funcInfo->address = cmdList.size();

	assert(funcInfo->allParamSize + 4 < 65536);
	// Save previous stack frame, and expand current by shift bytes
	cmdList.push_back(VMCmd(cmdPushVTop, (unsigned short)(funcInfo->allParamSize + 4), shift));
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
void NodeFuncDef::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s FuncDef %s %s\r\n", typeInfo->GetFullTypeName(), funcInfo->name, (disabled ? " disabled" : ""));
	if(!disabled)
	{
		GoDownB();
		first->LogToStream(fGraph);
		GoUp();
	}
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

	if(funcInfo && funcInfo->type == FunctionInfo::LOCAL)
		first = TakeLastNode();

	if(!funcInfo)
		first = TakeLastNode();

	if(funcType->paramCount > 0)
		paramHead = paramTail = TakeLastNode();
	else
		paramHead = paramTail = NULL;

	// Take nodes for all parameters
	for(unsigned int i = 1; i < funcType->paramCount; i++)
	{
		paramTail->next = TakeLastNode();
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
	// Find parameter values
	if(first)
		first->Compile();
	if(funcType->paramCount > 0)
	{
		NodeZeroOP	*curr = paramHead;
		TypeInfo	**paramType = funcType->paramType + funcType->paramCount - 1;
		do
		{
			// Compute parameter value
			curr->Compile();
			// Convert it to type that function expects
			ConvertFirstToSecond(curr->typeInfo->stackType, (*paramType)->stackType);
			if(*paramType == typeFloat)
				cmdList.push_back(VMCmd(cmdDtoF));
			curr = curr->next;
			paramType--;
		}while(curr);
	}
	if(funcInfo && funcInfo->address == -1)		// If the function is build-in or external
	{
		// Call it by function index
		unsigned int ID = CodeInfo::FindFunctionByPtr(funcInfo);
		cmdList.push_back(VMCmd(cmdCallStd, ID));
	}else{					// If the function is defined by user
		// Lets move parameters to function local variables
		unsigned int paramSize = 0;
		for(unsigned int i = 0; i < funcType->paramCount; i++)
			paramSize += funcType->paramType[i]->size < 4 ? 4 : funcType->paramType[i]->size;
		paramSize += first ? 4 : 0;
		if(paramSize)
			cmdList.push_back(VMCmd(cmdReserveV, paramSize));

		// Call by function address in bytecode
		unsigned int ID = CodeInfo::FindFunctionByPtr(funcInfo);
		unsigned short helper = (unsigned short)((typeInfo->type == TypeInfo::TYPE_COMPLEX || typeInfo->type == TypeInfo::TYPE_VOID) ? typeInfo->size : (bitRetSimple | operTypeForStackType[typeInfo->stackType]));
		cmdList.push_back(VMCmd(cmdCall, helper, funcInfo ? ID : CALL_BY_POINTER));
	}
}
void NodeFuncCall::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s FuncCall '%s' %d\r\n", typeInfo->GetFullTypeName(), (funcInfo ? funcInfo->name : "$ptr"), funcType->paramCount);
	GoDown();
	if(first)
		first->LogToStream(fGraph);
	NodeZeroOP	*curr = paramTail;
	while(curr)
	{
		if(curr == paramHead)
		{
			GoUp();
			GoDownB();
		}
		curr->LogToStream(fGraph);
		curr = curr->prev;
	}
	GoUp();
}

//////////////////////////////////////////////////////////////////////////
// Node that fetches variable value

NodeGetAddress::NodeGetAddress(VariableInfo* vInfo, int vAddress, bool absAddr, TypeInfo *retInfo)
{
	assert(retInfo);

	varInfo = vInfo;
	varAddress = vAddress;
	absAddress = absAddr;

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

	cmdList.push_back(VMCmd(cmdGetAddr, absAddress ? 0 : 1, varAddress));
}

void NodeGetAddress::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s GetAddress ", typeInfo->GetFullTypeName());
	if(varInfo)
		fprintf(fGraph, "%s%s '%.*s'", (varInfo->isConst ? "const " : ""), varInfo->varType->GetFullTypeName(), varInfo->name.end-varInfo->name.begin, varInfo->name.begin);
	else
		fprintf(fGraph, "$$$");
	fprintf(fGraph, " (%d %s)\r\n", (int)varAddress, (absAddress ? " absolute" : " relative"));
}

//////////////////////////////////////////////////////////////////////////
NodeGetUpvalue::NodeGetUpvalue(int closureOffset, int closureElement, TypeInfo *retInfo)
{
	closurePos = closureOffset;
	closureElem = closureElement;
	typeInfo = retInfo;

	nodeType = typeNodeGetUpvalue;
}

NodeGetUpvalue::~NodeGetUpvalue()
{
}

void NodeGetUpvalue::Compile()
{
	if(sourcePos)
		cmdInfoList.AddDescription(cmdList.size(), sourcePos);

	cmdList.push_back(VMCmd(cmdPushInt, ADDRESS_RELATIVE, (unsigned short)typeInfo->size, closurePos));
	cmdList.push_back(VMCmd(cmdPushIntStk, 0, (unsigned short)typeInfo->size, closureElem));
}

void NodeGetUpvalue::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s GetUpvalue (base + %d)[%d]\r\n", typeInfo->GetFullTypeName(), closurePos, closureElem);
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
	first->Compile();
	if(typeInfo == typeObject)
	{
		cmdList.push_back(VMCmd(cmdPushTypeID, first->typeInfo->subType->typeIndex));
	}else{
		cmdList.push_back(VMCmd(cmdConvertPtr, typeInfo->subType->typeIndex));
	}
}
void NodeConvertPtr::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s ConvertPtr :\r\n", typeInfo->GetFullTypeName());
	GoDownB();
	first->LogToStream(fGraph);
	GoUp();
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
	arrSetAll = (firstDefinition && typeInfo->arrLevel == 1 && second->typeInfo->arrLevel == 0 && second->typeInfo->refLevel == 0 && typeInfo->subType->type != TypeInfo::TYPE_COMPLEX && second->typeInfo->type != TypeInfo::TYPE_COMPLEX);

	if(second->typeInfo == typeVoid)
		ThrowError(CodeInfo::lastKnownStartPos, "ERROR: Cannot convert from void to %s", typeInfo->GetFullTypeName());
	if(typeInfo == typeVoid)
		ThrowError(CodeInfo::lastKnownStartPos, "ERROR: Cannot convert from %s to void", second->typeInfo->GetFullTypeName());
	
	// If types don't match
	if(second->typeInfo != typeInfo)
	{
		// If it is not build-in basic types
		// Or if array dimension differ and it is not special array definition
		// Or if the pointer depths aren't the same
		// Or if pointers point to different types
		if(!(typeInfo->type != TypeInfo::TYPE_COMPLEX && second->typeInfo->type != TypeInfo::TYPE_COMPLEX) ||
			(typeInfo->arrLevel != second->typeInfo->arrLevel && !arrSetAll) ||
			(typeInfo->refLevel != second->typeInfo->refLevel) ||
			(typeInfo->refLevel && typeInfo->refLevel == second->typeInfo->refLevel && typeInfo->subType != second->typeInfo->subType))
		{
			if(!(typeInfo->arrLevel != 0 && second->typeInfo->arrLevel == 0 && arrSetAll))
				ThrowError(CodeInfo::lastKnownStartPos, "ERROR: Cannot convert '%s' to '%s'", second->typeInfo->GetFullTypeName(), typeInfo->GetFullTypeName());
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
	if(first->nodeType == typeNodeShiftAddress)
	{
		addrShift = static_cast<NodeShiftAddress*>(first)->memberShift;
		NodeZeroOP	*oldFirst = first;
		first = static_cast<NodeShiftAddress*>(first)->first;
		static_cast<NodeShiftAddress*>(oldFirst)->first = NULL;
	}
	if(first->nodeType == typeNodeArrayIndex && static_cast<NodeArrayIndex*>(first)->knownShift)
	{
		addrShift = static_cast<NodeArrayIndex*>(first)->shiftValue;
		NodeZeroOP	*oldFirst = first;
		first = static_cast<NodeArrayIndex*>(first)->first;
		static_cast<NodeArrayIndex*>(oldFirst)->first = NULL;
	}

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
		cmdList.push_back(VMCmd(cmdSetRange, (unsigned short)(asmDT), addrShift));
	}else{
		if(knownAddress)
		{
			cmdList.push_back(VMCmd(cmdMovType[asmDT>>2], absAddress ? ADDRESS_ABOLUTE : ADDRESS_RELATIVE, (unsigned short)typeInfo->size, addrShift));
		}else{
			cmdList.push_back(VMCmd(cmdMovTypeStk[asmDT>>2], asmST == STYPE_DOUBLE ? 1 : 0, (unsigned short)typeInfo->size, addrShift));
		}
	}
}

void NodeVariableSet::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s VariableSet %s\r\n", typeInfo->GetFullTypeName(), (arrSetAll ? "set all elements" : ""));
	GoDown();
	first->LogToStream(fGraph);
	GoUp();
	GoDownB();
	second->LogToStream(fGraph);
	GoUp();
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
		ThrowError(CodeInfo::lastKnownStartPos, "ERROR: Cannot convert from void to %s", typeInfo->GetFullTypeName());
	if(typeInfo == typeVoid)
		ThrowError(CodeInfo::lastKnownStartPos, "ERROR: Cannot convert from %s to void", second->typeInfo->GetFullTypeName());
	if(first->typeInfo->subType->refLevel != 0 || second->typeInfo->refLevel != 0 || typeInfo->type == TypeInfo::TYPE_COMPLEX || second->typeInfo->type == TypeInfo::TYPE_COMPLEX)
		ThrowError(CodeInfo::lastKnownStartPos, "ERROR: There is no build-in operator for types '%s' and '%s'", typeInfo->GetFullTypeName(), second->typeInfo->GetFullTypeName());

	// If types don't match
	if(second->typeInfo != typeInfo)
	{
		// If it is not build-in basic types
		// Or if array dimension differ
		// Or if the pointer depths aren't the same
		// Or if pointers point to different types
		if(!(typeInfo->type != TypeInfo::TYPE_COMPLEX && second->typeInfo->type != TypeInfo::TYPE_COMPLEX) ||
			(typeInfo->arrLevel != second->typeInfo->arrLevel) ||
			(typeInfo->refLevel != second->typeInfo->refLevel) ||
			(typeInfo->refLevel && typeInfo->refLevel == second->typeInfo->refLevel && typeInfo->subType != second->typeInfo->subType))
		{
			ThrowError(CodeInfo::lastKnownStartPos, "ERROR: Cannot convert '%s' to '%s'", second->typeInfo->GetFullTypeName(), typeInfo->GetFullTypeName());
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
	if(first->nodeType == typeNodeShiftAddress)
	{
		addrShift = static_cast<NodeShiftAddress*>(first)->memberShift;
		NodeZeroOP	*oldFirst = first;
		first = static_cast<NodeShiftAddress*>(first)->first;
		static_cast<NodeShiftAddress*>(oldFirst)->first = NULL;
	}
	if(first->nodeType == typeNodeArrayIndex && static_cast<NodeArrayIndex*>(first)->knownShift)
	{
		addrShift = static_cast<NodeArrayIndex*>(first)->shiftValue;
		NodeZeroOP	*oldFirst = first;
		first = static_cast<NodeArrayIndex*>(first)->first;
		static_cast<NodeArrayIndex*>(oldFirst)->first = NULL;
	}

	nodeType = typeNodeVariableModify;
}

NodeVariableModify::~NodeVariableModify()
{
}

void NodeVariableModify::Compile()
{
	if(sourcePos)
		cmdInfoList.AddDescription(cmdList.size(), sourcePos);

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

	// Put first operand on top of the stack
	if(knownAddress)
	{
		cmdList.push_back(VMCmd(cmdMovType[asmDT>>2], absAddress ? ADDRESS_ABOLUTE : ADDRESS_RELATIVE, (unsigned short)typeInfo->size, addrShift));
	}else{
		cmdList.push_back(VMCmd(cmdMovTypeStk[asmDT>>2], asmDT == DTYPE_DOUBLE ? 1 : 0, (unsigned short)typeInfo->size, addrShift));
	}
}

void NodeVariableModify::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s VariableModify\r\n", typeInfo->GetFullTypeName());
	GoDown();
	first->LogToStream(fGraph);
	GoUp();
	GoDownB();
	second->LogToStream(fGraph);
	GoUp();
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

	// Node that calculates address of the first array element
	first = TakeLastNode();

	shiftValue = 0;
	knownShift = false;

	if(second->nodeType == typeNodeNumber && typeParent->arrSize != TypeInfo::UNSIZED_ARRAY)
	{
		shiftValue = typeParent->subType->size * static_cast<NodeNumber*>(second)->GetInteger();
		knownShift = true;
	}

	nodeType = typeNodeArrayIndex;
}

NodeArrayIndex::~NodeArrayIndex()
{
}

void NodeArrayIndex::Compile()
{
	if(sourcePos)
		cmdInfoList.AddDescription(cmdList.size(), sourcePos);

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

void NodeArrayIndex::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s ArrayIndex %s known: %d shiftval: %d\r\n", typeInfo->GetFullTypeName(), typeParent->GetFullTypeName(), knownShift, shiftValue);
	GoDown();
	first->LogToStream(fGraph);
	GoUp();
	GoDownB();
	second->LogToStream(fGraph);
	GoUp();
}

//////////////////////////////////////////////////////////////////////////
// Node to get value by address (dereference pointer)

NodeDereference::NodeDereference(FunctionInfo* setClosure, unsigned int offsetToPrevClosure)
{
	first = TakeLastNode();
	assert(first->typeInfo);
	assert(first->typeInfo->subType);
	typeInfo = first->typeInfo->subType;

	absAddress = true;
	knownAddress = false;
	addrShift = 0;
	closureFunc = setClosure;
	offsetToPreviousClosure = offsetToPrevClosure;

	if(first->nodeType == typeNodeGetAddress)
	{
		absAddress = static_cast<NodeGetAddress*>(first)->IsAbsoluteAddress();
		addrShift = static_cast<NodeGetAddress*>(first)->varAddress;
		knownAddress = true;
	}
	if(first->nodeType == typeNodeShiftAddress)
	{
		addrShift = static_cast<NodeShiftAddress*>(first)->memberShift;
		NodeZeroOP	*oldFirst = first;
		first = static_cast<NodeShiftAddress*>(first)->first;
		static_cast<NodeShiftAddress*>(oldFirst)->first = NULL;
	}
	if(first->nodeType == typeNodeArrayIndex && static_cast<NodeArrayIndex*>(first)->knownShift)
	{
		addrShift = static_cast<NodeArrayIndex*>(first)->shiftValue;
		NodeZeroOP	*oldFirst = first;
		first = static_cast<NodeArrayIndex*>(first)->first;
		static_cast<NodeArrayIndex*>(oldFirst)->first = NULL;
	}

	nodeType = typeNodeDereference;
}

NodeDereference::~NodeDereference()
{
}


void NodeDereference::Compile()
{
	if(sourcePos)
		cmdInfoList.AddDescription(cmdList.size(), sourcePos);

	asmDataType asmDT = typeInfo->dataType;

	if(closureFunc)
	{
		first->Compile();
		cmdList.push_back(VMCmd(cmdCreateClosure, (unsigned short)offsetToPreviousClosure, CodeInfo::FindFunctionByPtr(closureFunc)));
	}else{
		if(!knownAddress)
			first->Compile();

		if(knownAddress)
			cmdList.push_back(VMCmd(cmdPushType[asmDT>>2], absAddress ? ADDRESS_ABOLUTE : ADDRESS_RELATIVE, (unsigned short)typeInfo->size, addrShift));
		else
			cmdList.push_back(VMCmd(cmdPushTypeStk[asmDT>>2], asmDT == DTYPE_DOUBLE ? 1 : 0, (unsigned short)typeInfo->size, addrShift));
	}
}

void NodeDereference::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s Dereference%s", typeInfo->GetFullTypeName(), closureFunc ? " and create closure" : "");
	if(knownAddress)
		fprintf(fGraph, " at known address [%s%d]\r\n", absAddress ? "" : "base+", addrShift);
	else
		fprintf(fGraph, " at [ptr+%d]\r\n", addrShift);
	GoDownB();
	first->LogToStream(fGraph);
	GoUp();
}

//////////////////////////////////////////////////////////////////////////
// Node that shifts address to the class member

NodeShiftAddress::NodeShiftAddress(unsigned int shift, TypeInfo* resType)
{
	memberShift = shift;
	typeInfo = CodeInfo::GetReferenceType(resType);

	first = TakeLastNode();

	if(first->nodeType == typeNodeShiftAddress)
	{
		memberShift += static_cast<NodeShiftAddress*>(first)->memberShift;
		NodeZeroOP	*oldFirst = first;
		first = static_cast<NodeShiftAddress*>(first)->first;
		static_cast<NodeShiftAddress*>(oldFirst)->first = NULL;
	}

	nodeType = typeNodeShiftAddress;
}

NodeShiftAddress::~NodeShiftAddress()
{
}


void NodeShiftAddress::Compile()
{
	if(sourcePos)
		cmdInfoList.AddDescription(cmdList.size(), sourcePos);

	// Get variable address
	first->Compile();

	if(memberShift)
	{
		cmdList.push_back(VMCmd(cmdPushImmt, memberShift));
		// Add the shift to the address
		cmdList.push_back(VMCmd(cmdAdd));
	}
}

void NodeShiftAddress::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s ShiftAddress [+%d]\r\n", typeInfo->GetFullTypeName(), memberShift);
	GoDownB();
	first->LogToStream(fGraph);
	GoUp();
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
		ThrowError(CodeInfo::lastKnownStartPos, "ERROR: %s is not supported on '%s'", (isInc ? "Increment" : "Decrement"), typeInfo->GetFullTypeName());

	prefixOp = preOp;

	optimised = false;

	absAddress = true;
	knownAddress = false;
	addrShift = 0;

	if(first->nodeType == typeNodeGetAddress)
	{
		absAddress = static_cast<NodeGetAddress*>(first)->IsAbsoluteAddress();
		addrShift = static_cast<NodeGetAddress*>(first)->varAddress;
		knownAddress = true;
	}
	if(first->nodeType == typeNodeShiftAddress)
	{
		addrShift = static_cast<NodeShiftAddress*>(first)->memberShift;
		NodeZeroOP	*oldFirst = first;
		first = static_cast<NodeShiftAddress*>(first)->first;
		static_cast<NodeShiftAddress*>(oldFirst)->first = NULL;
	}
	if(first->nodeType == typeNodeArrayIndex && static_cast<NodeArrayIndex*>(first)->knownShift)
	{
		addrShift = static_cast<NodeArrayIndex*>(first)->shiftValue;
		NodeZeroOP	*oldFirst = first;
		first = static_cast<NodeArrayIndex*>(first)->first;
		static_cast<NodeArrayIndex*>(oldFirst)->first = NULL;
	}

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

void NodePreOrPostOp::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s PreOrPostOp %s\r\n", typeInfo->GetFullTypeName(), (prefixOp ? "prefix" : "postfix"));
	GoDownB();
	first->LogToStream(fGraph);
	GoUp();
}

//////////////////////////////////////////////////////////////////////////
// Node that gets function address

NodeFunctionAddress::NodeFunctionAddress(FunctionInfo* functionInfo)
{
	funcInfo = functionInfo;
	typeInfo = funcInfo->funcType;

	if(funcInfo->type == FunctionInfo::LOCAL || funcInfo->type == FunctionInfo::THISCALL)
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

	unsigned int ID = CodeInfo::FindFunctionByPtr(funcInfo);
	cmdList.push_back(VMCmd(cmdFuncAddr, ID));

	if(funcInfo->type == FunctionInfo::NORMAL)
	{
		cmdList.push_back(VMCmd(cmdPushImmt, funcInfo->funcPtr ? ~0ul : 0));
	}else if(funcInfo->type == FunctionInfo::LOCAL || funcInfo->type == FunctionInfo::THISCALL){
		first->Compile();
	}
}

void NodeFunctionAddress::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s FunctionAddress %s %s\r\n", typeInfo->GetFullTypeName(), funcInfo->name, (funcInfo->funcPtr ? " external" : ""));
	if(first)
	{
		GoDownB();
		first->LogToStream(fGraph);
		GoUp();
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
	if(first->typeInfo->refLevel != 0 || second->typeInfo->refLevel != 0 || first->typeInfo->type == TypeInfo::TYPE_COMPLEX || second->typeInfo->type == TypeInfo::TYPE_COMPLEX)
	{
		if(!(first->typeInfo->refLevel == second->typeInfo->refLevel && logicalOp))
			ThrowError(CodeInfo::lastKnownStartPos, "ERROR: Operation %s is not supported on '%s' and '%s'", binCommandToText[cmdID - cmdAdd], first->typeInfo->GetFullTypeName(), second->typeInfo->GetFullTypeName());
	}
	if(first->typeInfo == typeVoid)
		ThrowError(CodeInfo::lastKnownStartPos, "ERROR: first operator returns void");
	if(second->typeInfo == typeVoid)
		ThrowError(CodeInfo::lastKnownStartPos, "ERROR: second operator returns void");

	if((first->typeInfo == typeDouble || first->typeInfo == typeFloat || second->typeInfo == typeDouble || second->typeInfo == typeFloat) && (cmd >= cmdShl && cmd <= cmdLogXor))
		ThrowError(CodeInfo::lastKnownStartPos, "ERROR: binary operations are not available on floating-point numbers");

	// Find the type or resulting value
	typeInfo = logicalOp ? typeInt : ChooseBinaryOpResultType(first->typeInfo, second->typeInfo);

	nodeType = typeNodeBinaryOp;
}
NodeBinaryOp::~NodeBinaryOp()
{
}

void NodeBinaryOp::Compile()
{
	asmStackType fST = first->typeInfo->stackType, sST = second->typeInfo->stackType;
	
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
void NodeBinaryOp::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s NodeBinaryOp<%s> :\r\n", typeInfo->GetFullTypeName(), binCommandToText[cmdID-cmdAdd]);
	assert(cmdID >= cmdAdd);
	assert(cmdID <= cmdNEqualD);
	GoDown();
	first->LogToStream(fGraph);
	GoUp();
	GoDownB();
	second->LogToStream(fGraph);
	GoUp();
}

//////////////////////////////////////////////////////////////////////////
// Node for compilation of if(){}else{} statement and conditional operator ?:

NodeIfElseExpr::NodeIfElseExpr(bool haveElse, bool isTerm)
{
	// If else block is present
	if(haveElse)
	{
		third = TakeLastNode();
	}
	second = TakeLastNode();
	first = TakeLastNode();
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

	// Child node structure: if(first) second; else third;
	// Or, for conditional operator: first ? second : third;

	// Compute condition
	first->Compile();

	if(first->typeInfo->stackType != STYPE_INT)
		cmdList.push_back(VMCmd(first->typeInfo->stackType == STYPE_DOUBLE ? cmdDtoI : cmdLtoI));
	// If false, jump to 'else' block, or out of statement, if there is no 'else'
	cmdList.push_back(VMCmd(cmdJmpZ, ~0ul));	// Jump address will be fixed later on
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
		cmdList.push_back(VMCmd(cmdJmp, ~0ul));	// Jump address will be fixed later on
		unsigned int jmpToEnd = cmdList.size()-1;

		cmdList[jmpOnFalse].argument = cmdList.size();	// Fixup jump address

		// Compile block for condition == false
		third->Compile();
		if(typeInfo != typeVoid)
			ConvertFirstForSecond(third->typeInfo->stackType, second->typeInfo->stackType);

		cmdList[jmpToEnd].argument = cmdList.size();	// Fixup jump address
	}
}
void NodeIfElseExpr::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s IfExpression :\r\n", typeInfo->GetFullTypeName());
	GoDown();
	first->LogToStream(fGraph);
	if(!third)
	{
		GoUp();
		GoDownB();
	}
	second->LogToStream(fGraph);
	if(third)
	{
		GoUp();
		GoDownB();
		third->LogToStream(fGraph);
	}
	GoUp();
}

//////////////////////////////////////////////////////////////////////////
// Nod for compilation of for(){}

NodeForExpr::NodeForExpr()
{
	fourth = TakeLastNode();
	third = TakeLastNode();
	second = TakeLastNode();
	first = TakeLastNode();

	nodeType = typeNodeForExpr;
}
NodeForExpr::~NodeForExpr()
{
}

void NodeForExpr::Compile()
{
	if(sourcePos)
		cmdInfoList.AddDescription(cmdList.size(), sourcePos);

	// Child node structure: for(first, second, third) fourth;

	// Compile initialization node
	first->Compile();
	unsigned int posTestExpr = cmdList.size();

	// Compute condition value
	second->Compile();

	if(second->typeInfo->stackType != STYPE_INT)
		cmdList.push_back(VMCmd(second->typeInfo->stackType == STYPE_DOUBLE ? cmdDtoI : cmdLtoI));

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
}
void NodeForExpr::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s ForExpression :\r\n", typeInfo->GetFullTypeName());
	GoDown();
	first->LogToStream(fGraph);
	second->LogToStream(fGraph);
	third->LogToStream(fGraph);
	GoUp();
	GoDownB(); 
	fourth->LogToStream(fGraph);
	GoUp();
}

//////////////////////////////////////////////////////////////////////////
// Node for compilation of while(){}

NodeWhileExpr::NodeWhileExpr()
{
	second = TakeLastNode();
	first = TakeLastNode();

	nodeType = typeNodeWhileExpr;
}
NodeWhileExpr::~NodeWhileExpr()
{
}

void NodeWhileExpr::Compile()
{
	// Child node structure: while(first) second;

	unsigned int posStart = cmdList.size();
	// Compute condition value
	first->Compile();

	if(first->typeInfo->stackType != STYPE_INT)
		cmdList.push_back(VMCmd(first->typeInfo->stackType == STYPE_DOUBLE ? cmdDtoI : cmdLtoI));

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
}
void NodeWhileExpr::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s WhileExpression :\r\n", typeInfo->GetFullTypeName());
	GoDown();
	first->LogToStream(fGraph);
	GoUp();
	GoDownB(); 
	second->LogToStream(fGraph);
	GoUp();
}

//////////////////////////////////////////////////////////////////////////
// Node for compilation of do{}while()

NodeDoWhileExpr::NodeDoWhileExpr()
{
	second = TakeLastNode();
	first = TakeLastNode();

	nodeType = typeNodeDoWhileExpr;
}
NodeDoWhileExpr::~NodeDoWhileExpr()
{
}

void NodeDoWhileExpr::Compile()
{
	// Child node structure: do{ first; }while(second)

	unsigned int posStart = cmdList.size();
	// Compile loop contents
	first->Compile();

	unsigned int posCond = cmdList.size();
	// Compute condition value
	second->Compile();
	if(second->typeInfo->stackType != STYPE_INT)
		cmdList.push_back(VMCmd(second->typeInfo->stackType == STYPE_DOUBLE ? cmdDtoI : cmdLtoI));

	// Jump to beginning if condition == true
	cmdList.push_back(VMCmd(cmdJmpNZ, posStart));

	NodeContinueOp::SatisfyJumps(posCond);
	NodeBreakOp::SatisfyJumps(cmdList.size());
}
void NodeDoWhileExpr::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s DoWhileExpression :\r\n", typeInfo->GetFullTypeName());
	GoDown();
	first->LogToStream(fGraph);
	GoUp();
	GoDownB();
	second->LogToStream(fGraph);
	GoUp();
}

//////////////////////////////////////////////////////////////////////////
void SatisfyJumps(FastVector<unsigned int>& jumpList, unsigned int pos)
{
	for(unsigned int i = 0; i < jumpList.size();)
	{
		if(cmdList[jumpList[i]].argument == 1)//jumpList[i]->argument == 1)
		{
			// If level is equal to 1, replace it with jump position
			cmdList[jumpList[i]].argument = pos;
			// Remove element by replacing with the last one
			jumpList[i] = jumpList.back();
			jumpList.pop_back();
		}else{
			// Otherwise, change level
			cmdList[jumpList[i]].argument--;
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

	// Break the loop
	fixQueue.push_back(cmdList.size());
	cmdList.push_back(VMCmd(cmdJmp, breakDepth));
}
void NodeBreakOp::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s BreakExpression\r\n", typeInfo->GetFullTypeName());
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

	// Continue the loop
	fixQueue.push_back(cmdList.size());
	cmdList.push_back(VMCmd(cmdJmp, continueDepth));
}
void NodeContinueOp::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s ContinueOp\r\n", typeInfo->GetFullTypeName());
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
	asmStackType aST = first->typeInfo->stackType;
	asmOperType aOT = operTypeForStackType[aST];

	unsigned int queueStart = fixQueue.size(), queueCurr = queueStart;

	// Compute value
	first->Compile();

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
}
void NodeSwitchExpr::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s SwitchExpression :\r\n", typeInfo->GetFullTypeName());
	GoDown();
	first->LogToStream(fGraph);
	for(NodeZeroOP *curr = conditionHead, *block = blockHead; curr; curr = curr->next, block = block->next)
	{
		curr->LogToStream(fGraph);
		if(curr == conditionTail)
		{
			GoUp();
			GoDownB();
		}
		block->LogToStream(fGraph);
	}
	GoUp();
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
	NodeZeroOP	*curr = first;
	do 
	{
		curr->Compile();
		curr = curr->next;
	}while(curr);
}
void NodeExpressionList::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s NodeExpressionList :\r\n", typeInfo->GetFullTypeName());
	GoDown();
	NodeZeroOP	*curr = first;
	do 
	{
		if(curr == tail)
		{
			GoUp();
			GoDownB();
		}
		curr->LogToStream(fGraph);
		curr = curr->next;
	}while(curr);
	GoUp();
}
