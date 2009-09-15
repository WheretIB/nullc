#include "stdafx.h"
#include "SyntaxTree.h"

#include "CodeInfo.h"
using namespace CodeInfo;

unsigned int GetFuncIndexByPtr(FunctionInfo* funcInfo)
{
	for(unsigned int i = 0; i < CodeInfo::funcInfo.size(); i++)
		if(CodeInfo::funcInfo[i] == funcInfo)
			return i;

	return ~0u;
}

NodeZeroOP*	TakeLastNode()
{
	NodeZeroOP* last = nodeList.back();
	nodeList.pop_back();
	return last;
}

FastVector<unsigned int>	breakAddr(64);
FastVector<unsigned int>	continueAddr(64);

static char* binCommandToText[] = { "+", "-", "*", "/", "^", "%", "<", ">", "<=", ">=", "==", "!=", "<<", ">>", "&", "|", "^", "and", "or", "xor"};

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

unsigned int ConvertFirstForSecondSize(asmStackType first, asmStackType second)
{
	if((first == STYPE_INT || first == STYPE_LONG) && second == STYPE_DOUBLE)
		return 1;
	if(first == STYPE_INT && second == STYPE_LONG)
		return 1;
	return 0;
}
asmStackType	ConvertFirstForSecondType(asmStackType first, asmStackType second)
{
	if((first == STYPE_INT || first == STYPE_LONG) && second == STYPE_DOUBLE)
		return STYPE_DOUBLE;
	if(first == STYPE_INT && second == STYPE_LONG)
		return STYPE_LONG;
	return first;
}

unsigned int	ConvertFirstToSecondSize(asmStackType first, asmStackType second)
{
	if(second == STYPE_DOUBLE)
	{
		if(first == STYPE_INT || first == STYPE_LONG)
			return 1;
	}else if(second == STYPE_LONG){
		if(first == STYPE_INT)
			return 1;
		else if(first == STYPE_DOUBLE)
			return 1;
	}else if(second == STYPE_INT){
		if(first == STYPE_DOUBLE)
			return 1;
		else if(first == STYPE_LONG)
			return 1;
	}
	return 0;
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
	codeSize = 0;
	nodeType = typeNodeZeroOp;
}
NodeZeroOP::NodeZeroOP(TypeInfo* tinfo)
{
	typeInfo = tinfo;
	sourcePos = NULL;
	prev = next = NULL;
	codeSize = 0;
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
	unsigned int startCmdSize = cmdList.size();

	first->Compile();

	assert((cmdList.size()-startCmdSize) == codeSize);
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
	unsigned int startCmdSize = cmdList.size();

	NodeOneOP::Compile();
	second->Compile();

	assert((cmdList.size()-startCmdSize) == codeSize);
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
	unsigned int startCmdSize = cmdList.size();

	NodeTwoOP::Compile();
	third->Compile();

	assert((cmdList.size()-startCmdSize) == codeSize);
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
	if(codeSize == 2)
		cmdList.push_back(VMCmd(cmdPushImmt, quad.high));
	cmdList.push_back(VMCmd(cmdPushImmt, quad.low));
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
		integer = GetInteger();
		codeSize = 1;
	}else if(target == typeDouble || target == typeFloat){
		real = GetDouble();
		codeSize = 2;
	}else if(target == typeLong){
		integer64 = GetLong();
		codeSize = 2;
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
	codeSize = first->codeSize;
	if(first->typeInfo != typeVoid)
		codeSize += 1;
	nodeType = typeNodePopOp;
}
NodePopOp::~NodePopOp()
{
}

void NodePopOp::Compile()
{
	unsigned int startCmdSize = cmdList.size();

	if(sourcePos)
		cmdInfoList.AddDescription(cmdList.size(), sourcePos);

	// Child node computes value
	first->Compile();
	if(first->typeInfo != typeVoid)
	{
		// Removing it from top of the stack
		cmdList.push_back(VMCmd(cmdPop, first->typeInfo->type == TypeInfo::TYPE_COMPLEX ? first->typeInfo->size : stackTypeSize[first->typeInfo->stackType]));
	}

	assert((cmdList.size()-startCmdSize) == codeSize);
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

	codeSize = first->codeSize + 1;
	nodeType = typeNodeUnaryOp;
}
NodeUnaryOp::~NodeUnaryOp()
{
}

void NodeUnaryOp::Compile()
{
	unsigned int startCmdSize = cmdList.size();

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

	assert((cmdList.size()-startCmdSize) == codeSize);
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

NodeReturnOp::NodeReturnOp(int localRet, TypeInfo* tinfo)
{
	localReturn = localRet;
	// Result type is set from outside
	typeInfo = tinfo;

	first = TakeLastNode();

	codeSize = first->codeSize + 1 + (typeInfo ? ConvertFirstToSecondSize(first->typeInfo->stackType, typeInfo->stackType) : 0);
	nodeType = typeNodeReturnOp;
}
NodeReturnOp::~NodeReturnOp()
{
}

void NodeReturnOp::Compile()
{
	unsigned int startCmdSize = cmdList.size();

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
	cmdList.push_back(VMCmd(cmdReturn, (unsigned char)operType, (unsigned short)localReturn, retSize));

	assert((cmdList.size()-startCmdSize) == codeSize);
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
// Nodes that compiles function

NodeFuncDef::NodeFuncDef(FunctionInfo *info, unsigned int varShift)
{
	// Function description
	funcInfo = info;
	// Size of all local variables
	shift = varShift;

	disabled = false;

	first = TakeLastNode();

	codeSize = first->codeSize + 2;
	nodeType = typeNodeFuncDef;
}
NodeFuncDef::~NodeFuncDef()
{
}

void NodeFuncDef::Disable()
{
	codeSize = 0;
	disabled = true;
}

void NodeFuncDef::Compile()
{
	if(disabled)
		return;
	unsigned int startCmdSize = cmdList.size();

	funcInfo->address = cmdList.size();

	// Save previous stack frame, and expand current by shift bytes
	cmdList.push_back(VMCmd(cmdPushVTop, shift));
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

	assert((cmdList.size()-startCmdSize) == codeSize);
}
void NodeFuncDef::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s FuncDef %s %s\r\n", typeInfo->GetFullTypeName(), funcInfo->name, (disabled ? " disabled" : ""));
	GoDownB();
	first->LogToStream(fGraph);
	GoUp();
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
		second = TakeLastNode();

	if(!funcInfo)
		first = TakeLastNode();

	if(funcType->paramCount > 0)
		paramHead = paramTail = TakeLastNode();
	else
		paramHead = paramTail = NULL;

	codeSize = 0;
	bool onlyStackTypes = true;
	TypeInfo	**paramType = funcType->paramType;
	if(*paramType == typeChar || *paramType == typeShort || *paramType == typeFloat)
		onlyStackTypes = false;
	if(funcInfo && funcInfo->address == -1 && *paramType == typeFloat)
		codeSize += 1;

	// Take nodes for all parameters
	for(unsigned int i = 1; i < funcType->paramCount; i++)
	{
		paramType++;
		if(*paramType == typeChar || *paramType == typeShort || *paramType == typeFloat)
			onlyStackTypes = false;
		if(funcInfo && funcInfo->address == -1 && *paramType == typeFloat)
			codeSize += 1;
		paramTail->next = TakeLastNode();
		paramTail->next->prev = paramTail;
		paramTail = paramTail->next;
	}

	if(funcInfo && funcInfo->type == FunctionInfo::THISCALL)
		second = TakeLastNode();
	
	unsigned int paramSize = ((!funcInfo || second) ? 4 : 0);

	if(funcType->paramCount > 0)
	{
		NodeZeroOP	*curr = paramTail;
		TypeInfo	**paramType = funcType->paramType;
		do
		{
			paramSize += (*paramType)->size;

			codeSize += curr->codeSize;
			codeSize += ConvertFirstToSecondSize(curr->typeInfo->stackType, (*paramType)->stackType);
			curr = curr->prev;
			paramType++;
		}while(curr);
	}
	if(!funcInfo || second)
	{
		if(second)
			codeSize += second->codeSize;
		else
			codeSize += first->codeSize;
		if(!onlyStackTypes)
			codeSize += 1;
	}
	
	if(funcInfo && funcInfo->address == -1)
	{
		codeSize += 1;
	}else{
		if(onlyStackTypes)
			codeSize += (paramSize ? 3 : 1);
		else
			codeSize += (paramSize ? 2 : 1) + (unsigned int)(funcType->paramCount);
	}
	nodeType = typeNodeFuncCall;
}
NodeFuncCall::~NodeFuncCall()
{
}

void NodeFuncCall::Compile()
{
	unsigned int startCmdSize = cmdList.size();

	// Find parameter values
	bool onlyStackTypes = true;
	if(funcInfo && funcInfo->address == -1)
	{
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
	}else{
		if(!funcInfo || second)
		{
			if(second)
				second->Compile();
			else
				first->Compile();
		}
		if(funcType->paramCount > 0)
		{
			NodeZeroOP	*curr = paramTail;
			TypeInfo	**paramType = funcType->paramType;
			do
			{
				// Compute parameter value
				curr->Compile();
				// Convert it to type that function expects
				ConvertFirstToSecond(curr->typeInfo->stackType, (*paramType)->stackType);
				if(*paramType == typeChar || *paramType == typeShort || *paramType == typeFloat)
					onlyStackTypes = false;
				curr = curr->prev;
				paramType++;
			}while(curr);
		}
	}
	if(funcInfo && funcInfo->address == -1)		// If the function is build-in or external
	{
		// Call it by function index
		unsigned int ID = GetFuncIndexByPtr(funcInfo);
		cmdList.push_back(VMCmd(cmdCallStd, ID));
	}else{					// If the function is defined by user
		// Lets move parameters to function local variables
		unsigned int paramSize = 0;
		for(unsigned int i = 0; i < funcType->paramCount; i++)
			paramSize += funcType->paramType[i]->size;
		paramSize += ((!funcInfo || second) ? 4 : 0);
		if(paramSize)
			cmdList.push_back(VMCmd(cmdReserveV, paramSize));

		unsigned int addr = 0;
		if(!onlyStackTypes)
		{
			for(int i = funcType->paramCount-1; i >= 0; i--)
			{
				asmDataType newDT = funcType->paramType[i]->dataType;
				cmdList.push_back(VMCmd(cmdPopTypeTop[newDT>>2], newDT == DTYPE_DOUBLE ? 1 : 0, (unsigned short)funcType->paramType[i]->size, addr));
				addr += funcType->paramType[i]->size;
			}
		}

		if(!funcInfo || second)
		{
			if(!onlyStackTypes)
				cmdList.push_back(VMCmd(cmdPopIntTop, 4, addr));
		}
		if(onlyStackTypes && paramSize != 0)
		{
			if(paramSize == 4)
				cmdList.push_back(VMCmd(cmdPopTypeTop[DTYPE_INT>>2], 0, (unsigned short)paramSize, addr));
			else if(paramSize == 8)
				cmdList.push_back(VMCmd(cmdPopTypeTop[DTYPE_LONG>>2], 0, (unsigned short)paramSize, addr));
			else
				cmdList.push_back(VMCmd(cmdPopTypeTop[DTYPE_COMPLEX_TYPE>>2], 0, (unsigned short)paramSize, addr));
		}


		// Call by function address in bytecode
		unsigned int ID = GetFuncIndexByPtr(funcInfo);
		unsigned short helper = (unsigned short)((typeInfo->type == TypeInfo::TYPE_COMPLEX || typeInfo->type == TypeInfo::TYPE_VOID) ? typeInfo->size : (bitRetSimple | operTypeForStackType[typeInfo->stackType]));
		cmdList.push_back(VMCmd(cmdCall, helper, funcInfo ? ID : -1));
	}

	assert((cmdList.size()-startCmdSize) == codeSize);
}
void NodeFuncCall::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s FuncCall '%s' %d\r\n", typeInfo->GetFullTypeName(), (funcInfo ? funcInfo->name : "$ptr"), funcType->paramCount);
	GoDown();
	if(first)
		first->LogToStream(fGraph);
	if(second)
		second->LogToStream(fGraph);
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
	typeInfo = GetReferenceType(typeOrig);

	codeSize = 1;
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
	typeInfo = GetReferenceType(typeOrig);
}

void NodeGetAddress::ShiftToMember(TypeInfo::MemberVariable *member)
{
	assert(member);
	varAddress += member->offset;
	typeOrig = member->type;
	typeInfo = GetReferenceType(typeOrig);
}

void NodeGetAddress::Compile()
{
	unsigned int startCmdSize = cmdList.size();
	if(sourcePos)
		cmdInfoList.AddDescription(cmdList.size(), sourcePos);

	if(absAddress)
		cmdList.push_back(VMCmd(cmdPushImmt, varAddress));
	else
		cmdList.push_back(VMCmd(cmdGetAddr, varAddress));

	assert((cmdList.size()-startCmdSize) == codeSize);
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
// Node that sets value to the variable

NodeVariableSet::NodeVariableSet(TypeInfo* targetType, bool firstDefinition, bool swapNodes)
{
	assert(targetType);
	typeInfo = targetType;

	if(swapNodes)
		second = TakeLastNode();

	// Address of the target variable
	first = TakeLastNode();
	assert(first->typeInfo->refLevel != 0);

	if(!swapNodes)
		second = TakeLastNode();

	// If this is the first array definition and value is array sub-type, we set it to all array elements
	arrSetAll = (firstDefinition && typeInfo->arrLevel != 0 && second->typeInfo->arrLevel == 0 && typeInfo->subType->type != TypeInfo::TYPE_COMPLEX && second->typeInfo->type != TypeInfo::TYPE_COMPLEX);

	if(second->typeInfo == typeVoid)
	{
		char	errBuf[128];
		_snprintf(errBuf, 128, "ERROR: cannot convert from void to %s", typeInfo->GetFullTypeName());
		lastError = CompilerError(errBuf, lastKnownStartPos);
		return;
	}
	if(typeInfo == typeVoid)
	{
		char	errBuf[128];
		_snprintf(errBuf, 128, "ERROR: cannot convert from %s to void", second->typeInfo->GetFullTypeName());
		lastError = CompilerError(errBuf, lastKnownStartPos);
		return;
	}

	if(second->nodeType == typeNodeNumber)
		static_cast<NodeNumber*>(second)->ConvertTo(typeInfo);

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
			{
				char	errBuf[128];
				_snprintf(errBuf, 128, "ERROR: Cannot convert '%s' to '%s'", second->typeInfo->GetFullTypeName(), typeInfo->GetFullTypeName());
				lastError = CompilerError(errBuf, lastKnownStartPos);
				return;
			}
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

	codeSize = second->codeSize;
	if(!knownAddress)
		codeSize += first->codeSize;
	codeSize += ConvertFirstToSecondSize(second->typeInfo->stackType, typeInfo->stackType);
	if(arrSetAll)
		codeSize += 2;
	else
		codeSize += 1;
	nodeType = typeNodeVariableSet;
}

NodeVariableSet::~NodeVariableSet()
{
}


void NodeVariableSet::Compile()
{
	unsigned int startCmdSize = cmdList.size();
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

	assert((cmdList.size()-startCmdSize) == codeSize);
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
	typeInfo = targetType;

	cmdID = cmd;

	second = TakeLastNode();

	// Address of the target variable
	first = TakeLastNode();
	assert(first->typeInfo->refLevel != 0);

	if(second->typeInfo == typeVoid)
	{
		char	errBuf[128];
		_snprintf(errBuf, 128, "ERROR: cannot convert from void to %s", typeInfo->GetFullTypeName());
		lastError = CompilerError(errBuf, lastKnownStartPos);
		return;
	}
	if(typeInfo == typeVoid)
	{
		char	errBuf[128];
		_snprintf(errBuf, 128, "ERROR: cannot convert from %s to void", second->typeInfo->GetFullTypeName());
		lastError = CompilerError(errBuf, lastKnownStartPos);
		return;
	}

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
			char	errBuf[128];
			_snprintf(errBuf, 128, "ERROR: Cannot convert '%s' to '%s'", second->typeInfo->GetFullTypeName(), typeInfo->GetFullTypeName());
			lastError = CompilerError(errBuf, lastKnownStartPos);
			return;
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

	asmStackType asmSTfirst = typeInfo->stackType;
	asmStackType asmSTsecond = second->typeInfo->stackType;

	codeSize = second->codeSize;
	if(!knownAddress)
		codeSize += 2 * first->codeSize;
	codeSize += ConvertFirstForSecondSize(asmSTfirst, asmSTsecond);
	asmStackType asmSTresult = ConvertFirstForSecondType(asmSTfirst, asmSTsecond);
	codeSize += ConvertFirstForSecondSize(asmSTsecond, asmSTresult);
	codeSize += ConvertFirstToSecondSize(asmSTresult, asmSTfirst);
	codeSize += 3;
	nodeType = typeNodeVariableModify;
}

NodeVariableModify::~NodeVariableModify()
{
}

void NodeVariableModify::Compile()
{
	unsigned int startCmdSize = cmdList.size();
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

	// Произведём операцию со значениями
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

	assert((cmdList.size()-startCmdSize) == codeSize);
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
	typeInfo = GetReferenceType(parentType->subType);

	// Node that calculates array index
	second = TakeLastNode();

	// Node that calculates address of the first array element
	first = TakeLastNode();

	shiftValue = 0;
	knownShift = false;

	if(second->nodeType == typeNodeNumber)
	{
		shiftValue = typeParent->subType->size * static_cast<NodeNumber*>(second)->GetInteger();
		knownShift = true;
	}

	codeSize = first->codeSize + 1;
	if(knownShift)
		codeSize += 1;
	else
		codeSize += second->codeSize + (second->typeInfo->stackType != STYPE_INT ? 1 : 0) + (typeParent->subType->size != 1 ? 1 : 0);
	nodeType = typeNodeArrayIndex;
}

NodeArrayIndex::~NodeArrayIndex()
{
}

void NodeArrayIndex::Compile()
{
	unsigned int startCmdSize = cmdList.size();
	if(sourcePos)
		cmdInfoList.AddDescription(cmdList.size(), sourcePos);

	// Get address of the first array element
	first->Compile();

	if(knownShift)
	{
		cmdList.push_back(VMCmd(cmdPushImmt, shiftValue));
	}else{
		// Compute index value
		second->Compile();
		// Convert it to integer and multiply by the size of the element
		if(second->typeInfo->stackType != STYPE_INT)
			cmdList.push_back(VMCmd(second->typeInfo->stackType == STYPE_DOUBLE ? cmdDtoI : cmdLtoI));
		if(typeParent->subType->size != 1)
			cmdList.push_back(VMCmd(cmdImmtMul, typeParent->subType->size));
	}
	// Add it to the address of the first element
	cmdList.push_back(VMCmd(cmdAdd));

	assert((cmdList.size()-startCmdSize) == codeSize);
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

NodeDereference::NodeDereference(TypeInfo* type)
{
	assert(type);
	typeInfo = type;

	first = TakeLastNode();
	assert(first->typeInfo->refLevel != 0);

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

	codeSize = (!knownAddress ? first->codeSize : 0) + 1;
	nodeType = typeNodeDereference;
}

NodeDereference::~NodeDereference()
{
}


void NodeDereference::Compile()
{
	unsigned int startCmdSize = cmdList.size();
	if(sourcePos)
		cmdInfoList.AddDescription(cmdList.size(), sourcePos);

	asmDataType asmDT = typeInfo->dataType;
	
	if(!knownAddress)
		first->Compile();

	if(knownAddress)
	{
		cmdList.push_back(VMCmd(cmdPushType[asmDT>>2], absAddress ? ADDRESS_ABOLUTE : ADDRESS_RELATIVE, (unsigned short)typeInfo->size, addrShift));
	}else{
		cmdList.push_back(VMCmd(cmdPushTypeStk[asmDT>>2], asmDT == DTYPE_DOUBLE ? 1 : 0, (unsigned short)typeInfo->size, addrShift));
	}

	assert((cmdList.size()-startCmdSize) == codeSize);
}

void NodeDereference::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s Dereference\r\n", typeInfo->GetFullTypeName());
	GoDownB();
	first->LogToStream(fGraph);
	GoUp();
}

//////////////////////////////////////////////////////////////////////////
// Node that shifts address to the class member

NodeShiftAddress::NodeShiftAddress(unsigned int shift, TypeInfo* resType)
{
	memberShift = shift;
	typeInfo = GetReferenceType(resType);

	first = TakeLastNode();

	codeSize = first->codeSize;
	if(memberShift)
		codeSize += 2;
	nodeType = typeNodeShiftAddress;
}

NodeShiftAddress::~NodeShiftAddress()
{
}


void NodeShiftAddress::Compile()
{
	unsigned int startCmdSize = cmdList.size();
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

	assert((cmdList.size()-startCmdSize) == codeSize);
}

void NodeShiftAddress::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s ShiftAddress\r\n", typeInfo->GetFullTypeName());
	GoDownB();
	first->LogToStream(fGraph);
	GoUp();
}

//////////////////////////////////////////////////////////////////////////
// Node for increment and decrement operations

NodePreOrPostOp::NodePreOrPostOp(TypeInfo* resType, bool isInc, bool preOp)
{
	assert(resType);
	typeInfo = resType;

	first = TakeLastNode();
	assert(first->typeInfo->refLevel != 0);

	incOp = isInc;

	if(typeInfo->type == TypeInfo::TYPE_COMPLEX || typeInfo->refLevel != 0)
	{
		char	errBuf[128];
		_snprintf(errBuf, 128, "ERROR: %s is not supported on '%s'", (isInc ? "Increment" : "Decrement"), typeInfo->GetFullTypeName());
		lastError = CompilerError(errBuf, lastKnownStartPos);
		return;
	}

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

	codeSize = (!knownAddress ? first->codeSize : 0);
	if(knownAddress)
	{
		codeSize += 3;
		if(!prefixOp)
			codeSize++;
	}else{
		codeSize++;
	}
	nodeType = typeNodePreOrPostOp;
}

NodePreOrPostOp::~NodePreOrPostOp()
{
}


void NodePreOrPostOp::SetOptimised(bool doOptimisation)
{
	if(prefixOp)
	{
		if(!optimised && doOptimisation)
			codeSize++;
		if(optimised && !doOptimisation)
			codeSize--;
	}
	optimised = doOptimisation;
}


void NodePreOrPostOp::Compile()
{
	unsigned int startCmdSize = cmdList.size();
	if(sourcePos)
		cmdInfoList.AddDescription(cmdList.size(), sourcePos);

	asmStackType asmST = typeInfo->stackType;
	asmDataType asmDT = typeInfo->dataType;
	asmOperType aOT = operTypeForStackType[typeInfo->stackType];
	
	if(!knownAddress)
		first->Compile();

	if(knownAddress)
	{
		cmdList.push_back(VMCmd(cmdPushType[asmDT>>2], absAddress ? ADDRESS_ABOLUTE : ADDRESS_RELATIVE, (unsigned short)typeInfo->size, addrShift));
		cmdList.push_back(VMCmd(incOp ? cmdIncType[aOT] : cmdDecType[aOT]));
		cmdList.push_back(VMCmd(cmdMovType[asmDT>>2], absAddress ? ADDRESS_ABOLUTE : ADDRESS_RELATIVE, (unsigned short)typeInfo->size, addrShift));
		if(!prefixOp && !optimised)
			cmdList.push_back(VMCmd(!incOp ? cmdIncType[aOT] : cmdDecType[aOT]));
		if(optimised)
			cmdList.push_back(VMCmd(cmdPop, stackTypeSize[asmST]));
	}else{
		cmdList.push_back(VMCmd(cmdAddAtTypeStk[asmDT>>2], optimised ? 0 : (prefixOp ? bitPushAfter : bitPushBefore), incOp ? 1 : -1, addrShift));
	}

	assert((cmdList.size()-startCmdSize) == codeSize);
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

	codeSize = 1;
	if(funcInfo->type == FunctionInfo::NORMAL)
		codeSize += 1;
	else if(funcInfo->type == FunctionInfo::LOCAL || funcInfo->type == FunctionInfo::THISCALL)
		codeSize += first->codeSize;
	nodeType = typeNodeFunctionAddress;
}

NodeFunctionAddress::~NodeFunctionAddress()
{
}


void NodeFunctionAddress::Compile()
{
	unsigned int startCmdSize = cmdList.size();
	if(sourcePos)
		cmdInfoList.AddDescription(cmdList.size(), sourcePos);

	unsigned int ID = GetFuncIndexByPtr(funcInfo);
	cmdList.push_back(VMCmd(cmdFuncAddr, ID));

	if(funcInfo->type == FunctionInfo::NORMAL)
	{
		cmdList.push_back(VMCmd(cmdPushImmt, 0));
	}else if(funcInfo->type == FunctionInfo::LOCAL || funcInfo->type == FunctionInfo::THISCALL){
		first->Compile();
	}

	assert((cmdList.size()-startCmdSize) == codeSize);
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

	// Binary operations on complex types are not present at the moment
	if(first->typeInfo->refLevel != 0 || second->typeInfo->refLevel != 0 || first->typeInfo->type == TypeInfo::TYPE_COMPLEX || second->typeInfo->type == TypeInfo::TYPE_COMPLEX)
	{
		char	errBuf[128];
		_snprintf(errBuf, 128, "ERROR: Operation %s is not supported on '%s' and '%s'", binCommandToText[cmdID - cmdAdd], first->typeInfo->GetFullTypeName(), second->typeInfo->GetFullTypeName());
		lastError = CompilerError(errBuf, lastKnownStartPos);
		return;
	}
	if(first->typeInfo == typeVoid)
	{
		lastError = CompilerError("ERROR: first operator returns void", lastKnownStartPos);
		return;
	}
	if(second->typeInfo == typeVoid)
	{
		lastError = CompilerError("ERROR: second operator returns void", lastKnownStartPos);
		return;
	}

	if((first->typeInfo == typeDouble || first->typeInfo == typeFloat || second->typeInfo == typeDouble || second->typeInfo == typeFloat) && (cmd >= cmdShl && cmd <= cmdLogXor))
	{
		lastError = CompilerError("ERROR: binary operations are not available on floating-point numbers", lastKnownStartPos);
		return;
	}

	bool logicalOp = (cmd >= cmdLess && cmd <= cmdNEqual) || (cmd >= cmdLogAnd && cmd <= cmdLogXor);

	// Find the type or resulting value
	typeInfo = logicalOp ? typeInt : ChooseBinaryOpResultType(first->typeInfo, second->typeInfo);

	asmStackType fST = first->typeInfo->stackType, sST = second->typeInfo->stackType;
	codeSize = ConvertFirstForSecondSize(fST, sST);
	fST = ConvertFirstForSecondType(fST, sST);
	codeSize += ConvertFirstForSecondSize(sST, fST);
	codeSize += first->codeSize + second->codeSize + 1;
	nodeType = typeNodeBinaryOp;
}
NodeBinaryOp::~NodeBinaryOp()
{
}

void NodeBinaryOp::Compile()
{
	unsigned int startCmdSize = cmdList.size();

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

	assert((cmdList.size()-startCmdSize) == codeSize);
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

	codeSize = first->codeSize + second->codeSize + 1;
	if(third)
		codeSize += third->codeSize + 1 + (second->typeInfo != third->typeInfo ? 1 : 0);
	if(first->typeInfo->stackType != STYPE_INT)
		codeSize++;
	nodeType = typeNodeIfElseExpr;
}
NodeIfElseExpr::~NodeIfElseExpr()
{
}

void NodeIfElseExpr::Compile()
{
	unsigned int startCmdSize = cmdList.size();

	if(sourcePos)
		cmdInfoList.AddDescription(cmdList.size(), sourcePos);

	// Child node structure: if(first) second; else third;
	// Or, for conditional operator: first ? second : third;

	// Compute condition
	first->Compile();

	if(first->typeInfo->stackType != STYPE_INT)
		cmdList.push_back(VMCmd(first->typeInfo->stackType == STYPE_DOUBLE ? cmdDtoI : cmdLtoI));
	// If false, jump to 'else' block, or out of statement, if there is no 'else'
	cmdList.push_back(VMCmd(cmdJmpZ, 0));	// Jump address will be fixed later on
	VMCmd *jmpOnFalse = &cmdList.back(), *jmpToEnd = NULL;

	// Compile block for condition == true
	second->Compile();
	if(typeInfo != typeVoid)
		ConvertFirstForSecond(second->typeInfo->stackType, third->typeInfo->stackType);

	jmpOnFalse->argument = cmdList.size();	// Fixup jump address
	// If 'else' block is present, compile it
	if(third)
	{
		// Put jump to exit statement at the end of main block
		cmdList.push_back(VMCmd(cmdJmp, 0));	// Jump address will be fixed later on
		jmpToEnd = &cmdList.back();

		jmpOnFalse->argument = cmdList.size(); // Fixup jump address

		// Compile block for condition == false
		third->Compile();
		if(typeInfo != typeVoid)
			ConvertFirstForSecond(third->typeInfo->stackType, second->typeInfo->stackType);

		jmpToEnd->argument = cmdList.size(); // Fixup jump address
	}

	assert((cmdList.size()-startCmdSize) == codeSize);
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

	codeSize = first->codeSize + second->codeSize + third->codeSize + fourth->codeSize + 2;
	if(second->typeInfo->stackType != STYPE_INT)
		codeSize++;
	nodeType = typeNodeForExpr;
}
NodeForExpr::~NodeForExpr()
{
}

void NodeForExpr::Compile()
{
	unsigned int startCmdSize = cmdList.size();

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

	// Save exit address for break operator
	breakAddr.push_back(cmdList.size() + 1 + third->codeSize + fourth->codeSize + 1);

	// If condition == false, exit loop
	cmdList.push_back(VMCmd(cmdJmpZ, breakAddr.back()));

	// Save address for continue operator
	continueAddr.push_back(cmdList.size()+fourth->codeSize);

	// Compile loop contents
	fourth->Compile();
	// Compile operation, executed after each cycle
	third->Compile();
	// Jump to condition check
	cmdList.push_back(VMCmd(cmdJmp, posTestExpr));

	breakAddr.pop_back();
	continueAddr.pop_back();

	assert((cmdList.size()-startCmdSize) == codeSize);
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

	codeSize = first->codeSize + second->codeSize + 2;
	if(first->typeInfo->stackType != STYPE_INT)
		codeSize++;
	nodeType = typeNodeWhileExpr;
}
NodeWhileExpr::~NodeWhileExpr()
{
}

void NodeWhileExpr::Compile()
{
	unsigned int startCmdSize = cmdList.size();

	// Child node structure: while(first) second;

	unsigned int posStart = cmdList.size();
	// Compute condition value
	first->Compile();

	if(first->typeInfo->stackType != STYPE_INT)
		cmdList.push_back(VMCmd(first->typeInfo->stackType == STYPE_DOUBLE ? cmdDtoI : cmdLtoI));

	// Save exit address for break operator
	breakAddr.push_back(cmdList.size() + 1 + second->codeSize + 1);

	// If condition == false, exit loop
	cmdList.push_back(VMCmd(cmdJmpZ, breakAddr.back()));

	// Save address for continue operator
	continueAddr.push_back(cmdList.size() + second->codeSize);

	// Compile loop contents
	second->Compile();
	// Jump to condition check
	cmdList.push_back(VMCmd(cmdJmp, posStart));

	breakAddr.pop_back();
	continueAddr.pop_back();

	assert((cmdList.size()-startCmdSize) == codeSize);
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

	codeSize = first->codeSize + second->codeSize + 1;
	if(second->typeInfo->stackType != STYPE_INT)
		codeSize++;
	nodeType = typeNodeDoWhileExpr;
}
NodeDoWhileExpr::~NodeDoWhileExpr()
{
}

void NodeDoWhileExpr::Compile()
{
	unsigned int startCmdSize = cmdList.size();

	// Child node structure: do{ first; }while(second)

	unsigned int posStart = cmdList.size();
	// Save exit address for break operator
	breakAddr.push_back(cmdList.size() + first->codeSize + second->codeSize + 1);

	// Save address for continue operator
	continueAddr.push_back(cmdList.size() + first->codeSize);

	// Compile loop contents
	first->Compile();
	// Compute condition value
	second->Compile();
	if(second->typeInfo->stackType != STYPE_INT)
		cmdList.push_back(VMCmd(second->typeInfo->stackType == STYPE_DOUBLE ? cmdDtoI : cmdLtoI));

	// Jump to beginning if condition == true
	cmdList.push_back(VMCmd(cmdJmpNZ, posStart));

	breakAddr.pop_back();
	continueAddr.pop_back();

	assert((cmdList.size()-startCmdSize) == codeSize);
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
// Node for break operation

NodeBreakOp::NodeBreakOp(unsigned int brDepth)
{
	codeSize = 1;
	nodeType = typeNodeBreakOp;

	breakDepth = brDepth;
}
NodeBreakOp::~NodeBreakOp()
{
}

void NodeBreakOp::Compile()
{
	unsigned int startCmdSize = cmdList.size();
	if(sourcePos)
		cmdInfoList.AddDescription(cmdList.size(), sourcePos);

	// Break the loop
	cmdList.push_back(VMCmd(cmdJmp, breakAddr[breakAddr.size()-breakDepth]));

	assert((cmdList.size()-startCmdSize) == codeSize);
}
void NodeBreakOp::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s BreakExpression\r\n", typeInfo->GetFullTypeName());
}

//////////////////////////////////////////////////////////////////////////
// Node for continue operation

NodeContinueOp::NodeContinueOp(unsigned int contDepth)
{
	codeSize = 1;
	nodeType = typeNodeContinueOp;

	continueDepth = contDepth;
}
NodeContinueOp::~NodeContinueOp()
{
}

void NodeContinueOp::Compile()
{
	unsigned int startCmdSize = cmdList.size();
	if(sourcePos)
		cmdInfoList.AddDescription(cmdList.size(), sourcePos);

	// Continue the loop
	cmdList.push_back(VMCmd(cmdJmp, continueAddr[continueAddr.size()-continueDepth]));

	assert((cmdList.size()-startCmdSize) == codeSize);
}
void NodeContinueOp::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s ContinueOp\r\n", typeInfo->GetFullTypeName());
}

//////////////////////////////////////////////////////////////////////////
// Node for compilation of switch

NodeSwitchExpr::NodeSwitchExpr()
{
	// Take node with value
	first = TakeLastNode();
	conditionHead = conditionTail = NULL;
	blockHead = blockTail = NULL;
	caseCount = 0;

	codeSize = first->codeSize - 1;
	codeSize += 2;
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
	codeSize += conditionTail->codeSize;
	codeSize += blockTail->codeSize + 2;
	codeSize += 3;
}

void NodeSwitchExpr::Compile()
{
	unsigned int startCmdSize = cmdList.size();

	asmStackType aST = first->typeInfo->stackType;
	asmOperType aOT = operTypeForStackType[aST];

	// Compute value
	first->Compile();

	NodeZeroOP *curr, *currBlock;

	// Find address of the end of the switch
	unsigned int switchEnd = cmdList.size() + 2 + caseCount * 3;
	for(curr = conditionHead; curr; curr = curr->next)
		switchEnd += curr->codeSize;
	unsigned int condEnd = switchEnd;
	for(curr = blockHead; curr; curr = curr->next)
		switchEnd += curr->codeSize + 1 + (curr != blockTail ? 1 : 0);

	// Save exit address form break operator
	breakAddr.push_back(switchEnd);

	// Generate code for all cases
	unsigned int caseAddr = condEnd;
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
		cmdList.push_back(VMCmd(cmdJmpNZ, caseAddr));
		caseAddr += currBlock->codeSize + 2;
	}
	// Remove value by which we switched from stack
	cmdList.push_back(VMCmd(cmdPop, stackTypeSize[aST]));

	cmdList.push_back(VMCmd(cmdJmp, switchEnd));
	for(curr = blockHead; curr; curr = curr->next)
	{
		// Remove value by which we switched from stack
		cmdList.push_back(VMCmd(cmdPop, stackTypeSize[aST]));
		curr->Compile();
		if(curr != blockTail)
		{
			cmdList.push_back(VMCmd(cmdJmp, cmdList.size() + 2));
		}
	}

	assert(switchEnd == cmdList.size());

	breakAddr.pop_back();

	assert((cmdList.size()-startCmdSize) == codeSize);
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

	codeSize = tail->codeSize;
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
		codeSize += first->nodeType == typeNodeFuncDef ? 0 : first->codeSize;
		first->next = firstNext;
		first->next->prev = first;
	}else{
		tail->next = TakeLastNode();
		tail->next->prev = tail;
		tail = tail->next;
		codeSize += tail->nodeType == typeNodeFuncDef ? 0 : tail->codeSize;
	}
}

NodeZeroOP* NodeExpressionList::GetFirstNode()
{
	assert(first);
	return first;
}

void NodeExpressionList::Compile()
{
	unsigned int startCmdSize = cmdList.size();

	NodeZeroOP	*curr = first;
	do 
	{
		curr->Compile();
		curr = curr->next;
	}while(curr);

	assert((cmdList.size()-startCmdSize) == codeSize);
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
