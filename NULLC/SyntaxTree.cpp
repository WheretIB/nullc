#include "stdafx.h"
#include "SyntaxTree.h"

#include "CodeInfo.h"
#include "ConstantFold.h"

using CodeInfo::nodeList;
using CodeInfo::cmdList;
using CodeInfo::cmdInfoList;

NodeZeroOP*	TakeLastNode()
{
	NodeZeroOP* last = nodeList.back();
	nodeList.pop_back();
	return last;
}

static const char* binCommandToText[] = { "+", "-", "*", "/", "**", "%", "<", ">", "<=", ">=", "==", "!=", "<<", ">>", "&", "|", "^", "&&", "||", "^^"};
static const char* unaryCommandToText[] = { "-", "-", "-", "~", "~", "!", "!" };

//////////////////////////////////////////////////////////////////////////

unsigned int indentDepth = 1;
void OutputIdent(FILE *fOut)
{
	for(unsigned int i = 0; i < indentDepth; i++)
		fprintf(fOut, "\t");
}

void OutputCFunctionName(FILE *fOut, FunctionInfo *funcInfo)
{
	const char *namePrefix = *funcInfo->name == '$' ? "__" : "";
	unsigned int nameShift = *funcInfo->name == '$' ? 1 : 0;
	char fName[NULLC_MAX_VARIABLE_NAME_LENGTH + 32];
	SafeSprintf(fName, NULLC_MAX_VARIABLE_NAME_LENGTH + 32, (funcInfo->type == FunctionInfo::LOCAL || !funcInfo->visible) ? "%s%s_%d" : "%s%s", namePrefix, funcInfo->name + nameShift, CodeInfo::FindFunctionByPtr(funcInfo));
	if(const char *opName = funcInfo->GetOperatorName())
	{
		strcpy(fName, opName);
	}else{
		for(unsigned int k = 0; k < funcInfo->nameLength; k++)
		{
			if(fName[k] == ':' || fName[k] == '$' || fName[k] == '[' || fName[k] == ']')
				fName[k] = '_';
		}
		TypeInfo **type = CodeInfo::classMap.find(funcInfo->nameHash);
		if(type)
			strcat(fName, "__");
	}
	unsigned int length = (unsigned int)strlen(fName);
	if(fName[length-1] == '$')
		fName[length-1] = '_';
	fprintf(fOut, "%s", fName);
}

int	level = 0;
char	linePrefix[256];
unsigned int prefixSize = 2;

bool preNeedChange = false;
void GoDown()
{
	if(prefixSize >= 256 || prefixSize < 2)
		return;
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
	if(prefixSize >= 256 || prefixSize < 5)
		return;
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
void NodeZeroOP::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s ZeroOp\r\n", typeInfo->GetFullTypeName());
}
void NodeZeroOP::TranslateToC(FILE *fOut)
{
	OutputIdent(fOut);
	fprintf(fOut, "/* node translation unknown */\r\n");
}
NodeNumber* NodeZeroOP::Evaluate(char* memory, unsigned int size)
{
	(void)memory;
	(void)size;
	return NULL;	// by default, node evaluation is unknown
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
void NodeZeroOP::LogToStreamExtra(FILE *fGraph)
{
	NodeZeroOP *curr = head;
	while(curr)
	{
		curr->LogToStream(fGraph);
		curr = curr->next;
	}
}
void NodeZeroOP::TranslateToCExtra(FILE *fOut)
{
	NodeZeroOP *curr = head;
	while(curr)
	{
		curr->TranslateToC(fOut);
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
void NodeOneOP::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s OneOP :\r\n", typeInfo->GetFullTypeName());
	GoDown();
	LogToStreamExtra(fGraph);
	first->LogToStream(fGraph);
	GoUp();
}
void NodeOneOP::TranslateToC(FILE *fOut)
{
	first->TranslateToC(fOut);
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
void NodeTwoOP::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s TwoOp :\r\n", typeInfo->GetFullTypeName());
	GoDown();
	LogToStreamExtra(fGraph);
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
	CompileExtra();
	NodeTwoOP::Compile();
	third->Compile();
}
void NodeThreeOP::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s ThreeOp :\r\n", typeInfo->GetFullTypeName());
	GoDown();
	LogToStreamExtra(fGraph);
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
	if(typeInfo->refLevel)
	{
		cmdList.push_back(VMCmd(cmdPushPtrImmt, num.quad.low));
	}else{
		if(typeInfo->stackType != STYPE_INT)
			cmdList.push_back(VMCmd(cmdPushImmt, num.quad.high));
		cmdList.push_back(VMCmd(cmdPushImmt, num.quad.low));
	}
}
void NodeNumber::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s Number\r\n", typeInfo->GetFullTypeName());
}
void NodeNumber::TranslateToC(FILE *fOut)
{
	if(typeInfo->refLevel)
		fprintf(fOut, "(void*)(%d)", num.integer);
	else if(typeInfo == typeChar || typeInfo == typeShort || typeInfo == typeInt)
		fprintf(fOut, "%d", num.integer);
	else if(typeInfo == typeDouble)
		fprintf(fOut, "%f", num.real);
	else if(typeInfo == typeFloat)
		fprintf(fOut, "%ff", num.real);
	else if(typeInfo == typeLong)
		fprintf(fOut, "%lldLL", num.integer64);
	else
		fprintf(fOut, "%%unknown_number%%");
}
NodeNumber* NodeNumber::Evaluate(char *memory, unsigned int size)
{
	(void)memory;
	(void)size;
	return this;
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
void NodePopOp::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s PopOp :\r\n", typeInfo->GetFullTypeName());
	GoDownB();
	LogToStreamExtra(fGraph);
	first->LogToStream(fGraph);
	GoUp();
}
void NodePopOp::TranslateToC(FILE *fOut)
{
	OutputIdent(fOut);
	first->TranslateToC(fOut);
	fprintf(fOut, ";\r\n");
}
NodeNumber* NodePopOp::Evaluate(char *memory, unsigned int size)
{
	if(head)
		return NULL;
	NodeNumber *value = first->Evaluate(memory, size);
	if(!value)
		return NULL;
	return new NodeNumber(0, typeVoid);
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

	if((first->typeInfo->refLevel != 0 && !logicalOp) || (first->typeInfo->type == TypeInfo::TYPE_COMPLEX && first->typeInfo != typeObject))
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
		cmdList.push_back(VMCmd(cmdPop, 4));

	// Execute command
	if(aOT == OTYPE_INT || first->typeInfo == typeObject)
		cmdList.push_back(VMCmd((InstructionCode)(vmCmd.cmd + 0), vmCmd.argument));
	else if(aOT == OTYPE_LONG)
		cmdList.push_back(VMCmd((InstructionCode)(vmCmd.cmd + 1), vmCmd.argument));
	else
		cmdList.push_back(VMCmd((InstructionCode)(vmCmd.cmd + 2), vmCmd.argument));
}
void NodeUnaryOp::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s UnaryOp :\r\n", typeInfo->GetFullTypeName());
	GoDownB();
	LogToStreamExtra(fGraph);
	first->LogToStream(fGraph);
	GoUp();
}
void NodeUnaryOp::TranslateToC(FILE *fOut)
{
	switch(vmCmd.cmd)
	{
	case cmdNeg:
		fprintf(fOut, "-");
		break;
	case cmdBitNot:
		fprintf(fOut, "~");
		break;
	case cmdLogNot:
		fprintf(fOut, "!");
		break;
	default:
		fprintf(fOut, "%%unknown_unary_command%%");
	}
	first->TranslateToC(fOut);
}
NodeNumber* NodeUnaryOp::Evaluate(char *memory, unsigned int size)
{
	NodeNumber *value = first->Evaluate(memory, size);
	if(!value)
		return NULL;
	switch(vmCmd.cmd)
	{
	case cmdNeg:
		if(value->typeInfo == typeInt)
			return new NodeNumber(-value->GetInteger(), typeInt);
		if(value->typeInfo == typeLong)
			return new NodeNumber(-value->GetLong(), typeLong);
		if(value->typeInfo == typeDouble)
			return new NodeNumber(-value->GetDouble(), typeDouble);
		break;
	case cmdBitNot:
		if(value->typeInfo == typeInt)
			return new NodeNumber(~value->GetInteger(), typeInt);
		if(value->typeInfo == typeLong)
			return new NodeNumber(~value->GetLong(), typeLong);
		break;
	case cmdLogNot:
		if(value->typeInfo == typeInt)
			return new NodeNumber(!value->GetInteger(), typeInt);
		if(value->typeInfo == typeLong)
			return new NodeNumber((int)!value->GetLong(), typeInt);
		break;
	}
	return NULL;
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
	LogToStreamExtra(fGraph);
	first->LogToStream(fGraph);
	GoUp();
}
void NodeReturnOp::TranslateToC(FILE *fOut)
{
	static unsigned int retVarID = 0;

	TranslateToCExtra(fOut);
	if(parentFunction && parentFunction->closeUpvals)
	{
		OutputIdent(fOut);
		typeInfo->OutputCType(fOut, "");
		fprintf(fOut, "__nullcRetVar%d = ", retVarID);
		if(typeInfo != first->typeInfo)
		{
			fprintf(fOut, "(");
			typeInfo->OutputCType(fOut, "");
			fprintf(fOut, ")(");
		}
		first->TranslateToC(fOut);
		if(typeInfo != first->typeInfo)
			fprintf(fOut, ")");
		fprintf(fOut, ";\r\n");

		char name[NULLC_MAX_VARIABLE_NAME_LENGTH];
		// Glue together parameter list, extra parameter and local list. Every list could be empty.
		VariableInfo *curr = parentFunction->firstParam ? parentFunction->firstParam : (parentFunction->firstLocal ? parentFunction->firstLocal : parentFunction->extraParam);
		if(parentFunction->firstParam)
			parentFunction->lastParam->next = (parentFunction->firstLocal ? parentFunction->firstLocal : parentFunction->extraParam);
		if(parentFunction->firstLocal)
			parentFunction->lastLocal->next = parentFunction->extraParam;
		unsigned int hashThis = GetStringHash("this");
		for(; curr; curr = curr->next)
		{
			if(curr->usedAsExternal)
			{
				const char *namePrefix = *curr->name.begin == '$' ? "__" : "";
				unsigned int nameShift = *curr->name.begin == '$' ? 1 : 0;
				sprintf(name, "%s%.*s_%d", namePrefix, int(curr->name.end - curr->name.begin) -nameShift, curr->name.begin+nameShift, curr->pos);
			
				OutputIdent(fOut);
				if(curr->nameHash == hashThis)
					fprintf(fOut, "__nullcCloseUpvalue(__upvalue_%d___context, &__context);\r\n", CodeInfo::FindFunctionByPtr(curr->parentFunction));
				else
					fprintf(fOut, "__nullcCloseUpvalue(__upvalue_%d_%s, &%s);\r\n", CodeInfo::FindFunctionByPtr(curr->parentFunction), name, name);
			}
		}
		if(parentFunction->firstParam)
			parentFunction->lastParam->next = NULL;
		if(parentFunction->firstLocal)
			parentFunction->lastLocal->next = NULL;
		OutputIdent(fOut);
		fprintf(fOut, "return __nullcRetVar%d;\r\n", retVarID++);
		return;
	}
	OutputIdent(fOut);
	if(typeInfo == typeVoid || first->typeInfo == typeVoid)
	{
		fprintf(fOut, "return;\r\n");
	}else{
		fprintf(fOut, "return ");
		if(typeInfo != first->typeInfo)
		{
			fprintf(fOut, "(");
			typeInfo->OutputCType(fOut, "");
			fprintf(fOut, ")(");
		}
		first->TranslateToC(fOut);
		if(typeInfo != first->typeInfo)
			fprintf(fOut, ")");
		fprintf(fOut, ";\r\n");
	}
}
NodeNumber* NodeReturnOp::Evaluate(char *memory, unsigned int size)
{
	if(head)
		return NULL;
	if(parentFunction && parentFunction->closeUpvals)
		return NULL;
	// Compute value that we're going to return
	NodeNumber *value = first->Evaluate(memory, size);
	if(!value)
		return NULL;
	// Convert it to the return type of the function
	if(typeInfo)
		value->ConvertTo(typeInfo);
	return value;
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

void NodeBlock::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s BlockOp (close upvalues from offset %d of function %s) %s:\r\n", first->typeInfo->GetFullTypeName(), stackFrameShift, parentFunction->name, parentFunction->closeUpvals ? "yes" : "no");
	GoDownB();
	LogToStreamExtra(fGraph);
	first->LogToStream(fGraph);
	GoUp();
}
void NodeBlock::TranslateToC(FILE *fOut)
{
	first->TranslateToC(fOut);
	char name[NULLC_MAX_VARIABLE_NAME_LENGTH];
	// Glue together parameter list, extra parameter and local list. Every list could be empty.
	VariableInfo *curr = parentFunction->firstParam ? parentFunction->firstParam : (parentFunction->firstLocal ? parentFunction->firstLocal : parentFunction->extraParam);
	if(parentFunction->firstParam)
		parentFunction->lastParam->next = (parentFunction->firstLocal ? parentFunction->firstLocal : parentFunction->extraParam);
	if(parentFunction->firstLocal)
		parentFunction->lastLocal->next = parentFunction->extraParam;
	unsigned int hashThis = GetStringHash("this");
	for(; curr; curr = curr->next)
	{
		if(curr->usedAsExternal)
		{
			const char *namePrefix = *curr->name.begin == '$' ? "__" : "";
			unsigned int nameShift = *curr->name.begin == '$' ? 1 : 0;
			sprintf(name, "%s%.*s_%d", namePrefix, int(curr->name.end - curr->name.begin) - nameShift, curr->name.begin+nameShift, curr->pos);
		
			OutputIdent(fOut);
			if(curr->nameHash == hashThis)
				fprintf(fOut, "__nullcCloseUpvalue(__upvalue_%d___context, &__context);\r\n", CodeInfo::FindFunctionByPtr(curr->parentFunction));
			else
				fprintf(fOut, "__nullcCloseUpvalue(__upvalue_%d_%s, &%s);\r\n", CodeInfo::FindFunctionByPtr(curr->parentFunction), name, name);
		}
	}
	if(parentFunction->firstParam)
		parentFunction->lastParam->next = NULL;
	if(parentFunction->firstLocal)
		parentFunction->lastLocal->next = NULL;
}
NodeNumber* NodeBlock::Evaluate(char *memory, unsigned int size)
{
	if(head)
		return NULL;
	if(parentFunction->closeUpvals)
		return NULL;
	return first->Evaluate(memory, size);
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
	cmdList.push_back(VMCmd(cmdPushVTop, (unsigned short)(funcInfo->allParamSize + NULLC_PTR_SIZE), size));
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
	}else{
		GoDownB();
		LogToStreamExtra(fGraph);
		GoUp();
	}
}
void NodeFuncDef::TranslateToC(FILE *fOut)
{
	unsigned int oldIndent = indentDepth;
	indentDepth = 0;
	if(!disabled)
	{
		funcInfo->retType->OutputCType(fOut, " ");
		OutputCFunctionName(fOut, funcInfo);
		fprintf(fOut, "(");

		char name[NULLC_MAX_VARIABLE_NAME_LENGTH];
		VariableInfo *param = funcInfo->firstParam;
		for(; param; param = param->next)
		{
			sprintf(name, "%.*s_%d", int(param->name.end - param->name.begin), param->name.begin, param->pos);
			param->varType->OutputCType(fOut, name);
			fprintf(fOut, ", ");
		}
		if(funcInfo->type == FunctionInfo::THISCALL)
		{
			funcInfo->parentClass->OutputCType(fOut, "* __context");
		}else if(funcInfo->type == FunctionInfo::LOCAL){
			fprintf(fOut, "void* __");
			OutputCFunctionName(fOut, funcInfo);
			fprintf(fOut, "_ext_%d", funcInfo->allParamSize);
		}else{
			fprintf(fOut, "void* unused");
		}

		fprintf(fOut, ")\r\n{\r\n");
		indentDepth++;
		VariableInfo *local = funcInfo->firstLocal;
		for(; local; local = local->next)
		{
			OutputIdent(fOut);
			const char *namePrefix = *local->name.begin == '$' ? "__" : "";
			unsigned int nameShift = *local->name.begin == '$' ? 1 : 0;
			unsigned int length = sprintf(name, "%s%.*s_%d", namePrefix, int(local->name.end - local->name.begin) - nameShift, local->name.begin+nameShift, local->pos);
			for(unsigned int k = 0; k < length; k++)
			{
				if(name[k] == ':' || name[k] == '$')
					name[k] = '_';
			}
			local->varType->OutputCType(fOut, name);
			fprintf(fOut, ";\r\n");
		}
		first->TranslateToC(fOut);
		indentDepth--;
		fprintf(fOut, "}\r\n");
	}else{
		indentDepth++;
		TranslateToCExtra(fOut);
		indentDepth--;
	}
	indentDepth = oldIndent;
}
NodeNumber* NodeFuncDef::Evaluate(char *memory, unsigned int memSize)
{
	if(head)
		return NULL;
	// Stack frame should remain aligned, so its size should multiple of 16
	unsigned int size = (shift + 0xf) & ~0xf;
	if(memSize < size)
		return NULL;
	// Clear stack frame
	memset(memory + funcInfo->allParamSize, 0, size - funcInfo->allParamSize);
	NodeFuncCall::baseShift = size;
	// Evaluate function code
	return first->Evaluate(memory, memSize);
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
void NodeFuncCall::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s FuncCall '%s' %d\r\n", typeInfo->GetFullTypeName(), (funcInfo ? funcInfo->name : "$ptr"), funcType->paramCount);
	GoDown();
	LogToStreamExtra(fGraph);
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
void NodeFuncCall::TranslateToC(FILE *fOut)
{
	TranslateToCExtra(fOut);
	if(funcInfo)
		OutputCFunctionName(fOut, funcInfo);
	if(!funcInfo)
	{
		fprintf(fOut, "((");
		funcType->retType->OutputCType(fOut, "");
		fprintf(fOut, "(*)(");
		NodeZeroOP	*curr = paramTail;
		TypeInfo	**paramType = funcType->paramType;
		while(curr)
		{
			(*paramType)->OutputCType(fOut, "");
			fprintf(fOut, ", ");
			curr = curr->prev;
			paramType++;
		}
		fprintf(fOut, "void*))");

		fprintf(fOut, "(");
		first->TranslateToC(fOut);
		fprintf(fOut, ").ptr)");
	}
	fprintf(fOut, "(");
	NodeZeroOP	*curr = paramTail;
	TypeInfo	**paramType = funcType->paramType;
	while(curr)
	{
		if(*paramType != curr->typeInfo)
		{
			fprintf(fOut, "(");
			(*paramType)->OutputCType(fOut, "");
			fprintf(fOut, ")(");
		}
		curr->TranslateToC(fOut);
		if(*paramType != curr->typeInfo)
			fprintf(fOut, ")");
		fprintf(fOut, ", ");
		curr = curr->prev;
		paramType++;
	}
	if(!funcInfo)
		fprintf(fOut, "(");
	if(first)
		first->TranslateToC(fOut);
	else if(funcInfo)
		fprintf(fOut, "(void*)0");
	if(!funcInfo)
		fprintf(fOut, ").context");
	fprintf(fOut, ")");
}

unsigned int NodeFuncCall::baseShift = 0;
ChunkedStackPool<4092> NodeFuncCall::memoPool;
FastVector<NodeFuncCall::CallMemo> NodeFuncCall::memoList(128);

NodeNumber* NodeFuncCall::Evaluate(char *memory, unsigned int size)
{
	// Extra nodes disable evaluation, as also does indirect function call
	if(head || first || !funcInfo || !funcInfo->pure)
		return NULL;
	// Check that we have enough place for parameters
	if(funcInfo->allParamSize > size || funcType->paramCount > 16)
		return NULL;
	unsigned int nextFrameOffset = baseShift;
	NodeNumber *paramValue[16];
	if(funcType->paramCount > 0)
	{
		unsigned int argument = 0;
		NodeZeroOP	*curr = paramHead;
		TypeInfo	**paramType = funcType->paramType + funcType->paramCount - 1;
		do
		{
			if(curr->typeInfo->size == 0)
			{
				return NULL;	// cannot evaluate call with empty classes
			}else{
				// If this is first function call (from AddFunctionCallNode), and parameter is not a knpwn number, exit immediately.
				if(!nextFrameOffset && curr->nodeType != typeNodeNumber)
					return NULL;
				// Evaluate parameter value
				paramValue[argument] = curr->Evaluate(nextFrameOffset ? memory : NULL, nextFrameOffset ? size : 0);
				if(!paramValue[argument])
					return NULL;
				// Convert it to type that function expects
				paramValue[argument]->ConvertTo(*paramType);
			}
			curr = curr->next;
			paramType--;
			argument++;
		}while(curr);
	}
	// Shift stack frame
	memory += nextFrameOffset;
	size -= nextFrameOffset;
	if(funcInfo->allParamSize + NULLC_PTR_SIZE > size)
		return NULL;
	// Copy arguments into stack frame
	unsigned int offset = funcInfo->allParamSize;
	for(unsigned int i = 0; i < funcType->paramCount; i++)
	{
		if(paramValue[i]->typeInfo == typeFloat)
			*(float*)(memory + offset - 4) = (float)paramValue[i]->GetDouble();
		else if(paramValue[i]->typeInfo == typeInt || paramValue[i]->typeInfo == typeChar || paramValue[i]->typeInfo == typeShort)
			*(int*)(memory + offset - 4) = paramValue[i]->GetInteger();
		else if(paramValue[i]->typeInfo == typeLong)
			*(long long*)(memory + offset - 8) = paramValue[i]->GetLong();
		else if(paramValue[i]->typeInfo == typeDouble)
			*(double*)(memory + offset - 8) = paramValue[i]->GetDouble();
		else
			return NULL;
		offset -= paramValue[i]->typeInfo->size;
	}
	// Find old result
	for(unsigned int i = 0; i < memoList.size(); i++)
	{
		if(memoList[i].func == funcInfo && memcmp(memory, memoList[i].arguments, funcInfo->allParamSize) == 0)
			return memoList[i].value;
	}
	// Call function
	NodeNumber *result = ((NodeFuncDef*)funcInfo->functionNode)->Evaluate(memory, size);
	// Memoization
	if(result)
	{
		memoList.push_back(CallMemo());
		memoList.back().arguments = (char*)memoPool.Allocate(funcInfo->allParamSize);
		memcpy(memoList.back().arguments, memory, funcInfo->allParamSize);
		memoList.back().func = funcInfo;
		memoList.back().value = result;
	}
	return result;
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

void NodeGetAddress::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s GetAddress ", typeInfo->GetFullTypeName());
	if(varInfo)
		fprintf(fGraph, "%s '%.*s'", varInfo->varType->GetFullTypeName(), int(varInfo->name.end-varInfo->name.begin), varInfo->name.begin);
	else
		fprintf(fGraph, "$$$");
	fprintf(fGraph, " (%d %s)\r\n", varInfo ? varInfo->pos : varAddress, (absAddress ? " absolute" : " relative"));
	LogToStreamExtra(fGraph);
}
void NodeGetAddress::TranslateToC(FILE *fOut)
{
	if(head)
		fprintf(fOut, "(");
	NodeZeroOP *curr = head;
	while(curr)
	{
		assert(curr->nodeType == typeNodePopOp);
		((NodePopOp*)curr)->GetFirstNode()->TranslateToC(fOut);
		fprintf(fOut, ", ");
		curr = curr->next;
	}
	TranslateToCEx(fOut, true);
	if(head)
		fprintf(fOut, ")");
}
void NodeGetAddress::TranslateToCEx(FILE *fOut, bool takeAddress)
{
	if(takeAddress)
		fprintf(fOut, "&");
	if(varInfo && varInfo->nameHash != GetStringHash("this"))
	{
		const char *namePrefix = *varInfo->name.begin == '$' ? "__" : "";
		unsigned int nameShift = *varInfo->name.begin == '$' ? 1 : 0;
		fprintf(fOut, varAddress - addressOriginal ? "%s%.*s%+d" : "%s%.*s", namePrefix, int(varInfo->name.end-varInfo->name.begin) - nameShift, varInfo->name.begin + nameShift, (varAddress - addressOriginal) / (typeOrig->size ? typeOrig->size : 1));
		if(varInfo->blockDepth > 1)
			fprintf(fOut, "_%d", varInfo->pos);
	}else{
		fprintf(fOut, "__context");
	}
}
NodeNumber* NodeGetAddress::Evaluate(char *memory, unsigned int size)
{
	(void)memory;
	(void)size;
	if(head)
		return NULL;
	if(absAddress)
		return NULL;
	return new NodeNumber(int(trackAddress ? varInfo->pos : varAddress), typeInt);
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

void NodeGetUpvalue::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s GetUpvalue (base + %d)[%d]\r\n", typeInfo->GetFullTypeName(), closurePos, closureElem);
	LogToStreamExtra(fGraph);
}
void NodeGetUpvalue::TranslateToC(FILE *fOut)
{
	fprintf(fOut, "(");
	typeInfo->OutputCType(fOut, "");
	fprintf(fOut, ")");
	fprintf(fOut, "((__nullcUpvalue*)((char*)__");
	OutputCFunctionName(fOut, parentFunc);
	fprintf(fOut, "_ext_%d + %d))->ptr", closurePos, closureElem);
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
void NodeConvertPtr::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s ConvertPtr :\r\n", typeInfo->GetFullTypeName());
	GoDownB();
	LogToStreamExtra(fGraph);
	first->LogToStream(fGraph);
	GoUp();
}
void NodeConvertPtr::TranslateToC(FILE *fOut)
{
	if(typeInfo == typeTypeid)
	{
		fprintf(fOut, "%d", first->typeInfo->subType->typeIndex);
		return;
	}
	TranslateToCExtra(fOut);
	
	if(typeInfo == typeObject || typeInfo == typeTypeid)
	{
		fprintf(fOut, "__nullcMakeAutoRef((void*)");
		first->TranslateToC(fOut);
		fprintf(fOut, ", __nullcTR[%d])", first->typeInfo->subType->typeIndex);
	}else{
		fprintf(fOut, "(");
		typeInfo->OutputCType(fOut, "");
		fprintf(fOut, ")");
		fprintf(fOut, "__nullcGetAutoRef(");
		first->TranslateToC(fOut);
		fprintf(fOut, ", __nullcTR[%d])", typeInfo->subType->typeIndex);
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
	LogToStreamExtra(fGraph);
	first->LogToStream(fGraph);
	GoUp();
	GoDownB();
	second->LogToStream(fGraph);
	GoUp();
}
void NodeVariableSet::TranslateToC(FILE *fOut)
{
	TranslateToCExtra(fOut);
	if(arrSetAll)
	{
		if(typeInfo == typeChar)
			fprintf(fOut, "memset(*(");
		else
			fprintf(fOut, "__nullcSetArray(*(");
		first->TranslateToC(fOut);
		fprintf(fOut, ".ptr), ");
		second->TranslateToC(fOut);
		fprintf(fOut, ", %d)", elemCount);
	}else{
		if(second->nodeType == typeNodeExpressionList && second->typeInfo->subType == typeChar && second->typeInfo->arrSize != TypeInfo::UNSIZED_ARRAY)
		{
			fprintf(fOut, "memcpy((");
			first->TranslateToC(fOut);
			fprintf(fOut, ")->ptr, ");
			second->TranslateToC(fOut);
			fprintf(fOut, ", %d)", first->typeInfo->subType->size);
		}else{
			fprintf(fOut, "*(");
			first->TranslateToC(fOut);
			fprintf(fOut, ") = ");
			if(first->typeInfo->subType != second->typeInfo || (first->typeInfo->subType->refLevel && second->nodeType == typeNodeFuncCall))
			{
				fprintf(fOut, "(");
				first->typeInfo->subType->OutputCType(fOut, "");
				fprintf(fOut, ")");
			}
			second->TranslateToC(fOut);
		}
	}
}
NodeNumber* NodeVariableSet::Evaluate(char *memory, unsigned int size)
{
	if(head)
		return NULL;

	NodeNumber *right = second->Evaluate(memory, size);
	if(!right)
		return NULL;
	right->ConvertTo(typeInfo);

	unsigned int address = addrShift;
	if(!knownAddress)
	{
		NodeNumber *pointer = first->Evaluate(memory, size);
		if(!pointer)
			return NULL;
		address = pointer->GetInteger();
	}
	if(arrSetAll)
	{
		return NULL;
	}else{
		if(typeInfo == typeChar)
			*(char*)(memory + address) = (char)right->GetInteger();
		if(typeInfo == typeShort)
			*(short*)(memory + address) = (short)right->GetInteger();
		if(typeInfo == typeInt)
			*(int*)(memory + address) = right->GetInteger();
		if(typeInfo == typeLong)
			*(long long*)(memory + address) = right->GetLong();
		if(typeInfo == typeFloat)
			*(float*)(memory + address) = (float)right->GetDouble();
		if(typeInfo == typeDouble)
			*(double*)(memory + address) = right->GetDouble();
		return right;
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
	LogToStreamExtra(fGraph);
	first->LogToStream(fGraph);
	GoUp();
	GoDownB();
	second->LogToStream(fGraph);
	GoUp();
}
void NodeVariableModify::TranslateToC(FILE *fOut)
{
	TranslateToCExtra(fOut);
	if(cmdID == cmdPow)
	{
		fprintf(fOut, "__nullcPowSet(");
		first->TranslateToC(fOut);
		fprintf(fOut, ", ");
		second->TranslateToC(fOut);
		fprintf(fOut, ")");
	}else{
		const char *operation = "???";
		switch(cmdID)
		{
		case cmdAdd:
			operation = "+=";
			break;
		case cmdSub:
			operation = "-=";
			break;
		case cmdMul:
			operation = "*=";
			break;
		case cmdDiv:
			operation = "/=";
			break;
		}
		fprintf(fOut, "*(");
		first->TranslateToC(fOut);
		fprintf(fOut, ") %s ", operation);
		second->TranslateToC(fOut);
	}
}
NodeNumber* NodeVariableModify::Evaluate(char *memory, unsigned int size)
{
	NodeZeroOP	*curr = head;
	while(curr)
	{
		NodeNumber *value = curr->Evaluate(memory, size);
		if(!value)
			return NULL;
		curr = curr->next;
	}

	unsigned int address = addrShift;
	if(!knownAddress)
	{
		NodeNumber *pointer = first->Evaluate(memory, size);
		if(!pointer)
			return NULL;
		address = pointer->GetInteger();
	}else if(absAddress){
		return NULL;
	}

	TypeInfo *midType = ChooseBinaryOpResultType(typeInfo, second->typeInfo);

	// First operand
	NodeNumber *valueLeft = NULL;
	if(typeInfo == typeChar)
		valueLeft = new NodeNumber(*(char*)(memory + address), typeInt);
	else if(typeInfo == typeShort)
		valueLeft = new NodeNumber(*(short*)(memory + address), typeInt);
	else if(typeInfo == typeInt)
		valueLeft = new NodeNumber(*(int*)(memory + address), typeInt);
	else if(typeInfo == typeLong)
		valueLeft = new NodeNumber(*(long long*)(memory + address), typeLong);
	else if(typeInfo == typeFloat)
		valueLeft = new NodeNumber(*(float*)(memory + address), typeDouble);
	else if(typeInfo == typeDouble)
		valueLeft = new NodeNumber(*(double*)(memory + address), typeDouble);
	else
		return NULL;

	// Convert it to the resulting type
	if(midType == typeDouble || midType == typeFloat)
		valueLeft->ConvertTo(typeDouble);
	else if(midType == typeLong)
		valueLeft->ConvertTo(typeLong);
	else if(midType == typeInt || midType == typeShort || midType == typeChar)
		valueLeft->ConvertTo(typeInt);

	// Compute second value
	NodeNumber *valueRight = second->Evaluate(memory, size);
	if(!valueRight)
		return NULL;
	// Convert it to the result type
	if(midType == typeDouble || midType == typeFloat)
		valueRight->ConvertTo(typeDouble);
	else if(midType == typeLong)
		valueRight->ConvertTo(typeLong);
	else if(midType == typeInt || midType == typeShort || midType == typeChar)
		valueRight->ConvertTo(typeInt);

	// Apply binary operation
	if(midType == typeInt || midType == typeShort || midType == typeChar)
	{
		int result = optDoOperation(cmdID, valueLeft->GetInteger(), valueRight->GetInteger());
		*valueLeft = NodeNumber(result, typeInt);
	}else if(midType == typeLong){
		long long result = optDoOperation(cmdID, valueLeft->GetLong(), valueRight->GetLong());
		*valueLeft = NodeNumber(result, typeLong);
	}else if(midType == typeDouble || midType == typeFloat){
		double result = optDoOperation(cmdID, valueLeft->GetDouble(), valueRight->GetDouble());
		*valueLeft = NodeNumber(result, typeDouble);
	}else{
		return NULL;
	}
	if(valueLeft)
	{
		valueLeft->ConvertTo(typeInfo);
		// Save value to memory
		if(typeInfo == typeChar)
			*(char*)(memory + address) = (char)valueLeft->GetInteger();
		if(typeInfo == typeShort)
			*(short*)(memory + address) = (short)valueLeft->GetInteger();
		if(typeInfo == typeInt)
			*(int*)(memory + address) = valueLeft->GetInteger();
		if(typeInfo == typeLong)
			*(long long*)(memory + address) = valueLeft->GetLong();
		if(typeInfo == typeFloat)
			*(float*)(memory + address) = (float)valueLeft->GetDouble();
		if(typeInfo == typeDouble)
			*(double*)(memory + address) = valueLeft->GetDouble();
		return valueLeft;
	}
	return NULL;
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

void NodeArrayIndex::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s ArrayIndex %s known: %d shiftval: %d\r\n", typeInfo->GetFullTypeName(), typeParent->GetFullTypeName(), knownShift, shiftValue);
	GoDown();
	LogToStreamExtra(fGraph);
	first->LogToStream(fGraph);
	GoUp();
	GoDownB();
	second->LogToStream(fGraph);
	GoUp();
}
void NodeArrayIndex::TranslateToC(FILE *fOut)
{
	if(first->typeInfo->arrSize == TypeInfo::UNSIZED_ARRAY)
	{
		fprintf(fOut, "(");
		fprintf(fOut, "(");
		typeInfo->OutputCType(fOut, "");
		fprintf(fOut, ")(");
		first->TranslateToC(fOut);
		fprintf(fOut, ").ptr + __nullcIndex(");
		if(second->typeInfo != typeInt)
			fprintf(fOut, "(unsigned)(");
		second->TranslateToC(fOut);
		if(second->typeInfo != typeInt)
			fprintf(fOut, ")");

		fprintf(fOut, ", (");
		first->TranslateToC(fOut);
		fprintf(fOut, ").size)");

		fprintf(fOut, ")");
	}else{
		fprintf(fOut, "&(");
		first->TranslateToC(fOut);
		fprintf(fOut, ")");
		fprintf(fOut, "->ptr");
		fprintf(fOut, "[__nullcIndex(");
		if(second->typeInfo != typeInt)
			fprintf(fOut, "(unsigned)(");
		second->TranslateToC(fOut);
		if(second->typeInfo != typeInt)
			fprintf(fOut, ")");
		fprintf(fOut, ", %uu)]", first->typeInfo->subType->arrSize);
	}
}
NodeNumber* NodeArrayIndex::Evaluate(char *memory, unsigned int size)
{
	if(head)
		return NULL;

	// Get address of the first array element
	NodeNumber *pointer = first->Evaluate(memory, size);
	if(!pointer)
		return NULL;

	if(knownShift)
	{
		return new NodeNumber(pointer->GetInteger() + shiftValue, typeInt);
	}else{
		NodeNumber *index = second->Evaluate(memory, size);
		index->ConvertTo(typeInt);
		// Check bounds
		if(index->GetInteger() < 0 || (unsigned int)index->GetInteger() >= typeParent->arrSize)
			return NULL;
		return new NodeNumber(int(pointer->GetInteger() + typeParent->subType->size * index->GetInteger()), typeInt);
	}
}

//////////////////////////////////////////////////////////////////////////
// Node to get value by address (dereference pointer)

NodeDereference::NodeDereference(FunctionInfo* setClosure, unsigned int offsetToPrevClosure)
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

			if(knownAddress)
				cmdList.push_back(VMCmd(cmdPushType[asmDT>>2], absAddress ? ADDRESS_ABOLUTE : ADDRESS_RELATIVE, (unsigned short)typeInfo->size, addrShift));
			else
				cmdList.push_back(VMCmd(cmdPushTypeStk[asmDT>>2], asmDT == DTYPE_DOUBLE ? 1 : 0, (unsigned short)typeInfo->size, addrShift));
		}
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
	LogToStreamExtra(fGraph);
	first->LogToStream(fGraph);
	GoUp();
}
bool nodeDereferenceEndInComma = false;
void NodeDereference::TranslateToC(FILE *fOut)
{
	TranslateToCExtra(fOut);

	if(closureFunc)
	{
		OutputIdent(fOut);
		assert(first->nodeType == typeNodeVariableSet);
		assert(((NodeOneOP*)first)->GetFirstNode()->nodeType == typeNodeGetAddress);

		VariableInfo *closure = ((NodeGetAddress*)((NodeOneOP*)first)->GetFirstNode())->varInfo;

		char closureName[NULLC_MAX_VARIABLE_NAME_LENGTH];
		const char *namePrefix = *closure->name.begin == '$' ? "__" : "";
		unsigned int nameShift = *closure->name.begin == '$' ? 1 : 0;
		unsigned int length = sprintf(closureName, "%s%.*s_%d", namePrefix, int(closure->name.end - closure->name.begin) - nameShift, closure->name.begin + nameShift, closure->pos);
		for(unsigned int k = 0; k < length; k++)
		{
			if(closureName[k] == ':' || closureName[k] == '$')
				closureName[k] = '_';
		}

		fprintf(fOut, "(%s = (", closureName);
		closure->varType->OutputCType(fOut, "");
		fprintf(fOut, ")__newS(%d, (void*)0)),\r\n", closure->varType->subType->size);

		unsigned int pos = 0;
		for(FunctionInfo::ExternalInfo *curr = closureFunc->firstExternal; curr; curr = curr->next)
		{
			OutputIdent(fOut);

			fprintf(fOut, "(((int**)%s)[%d] = ", closureName, pos);
			VariableInfo *varInfo = curr->variable;
			char variableName[NULLC_MAX_VARIABLE_NAME_LENGTH];
			if(varInfo->nameHash == GetStringHash("this"))
			{
				strcpy(variableName, "__context");
			}else{
				namePrefix = *varInfo->name.begin == '$' ? "__" : "";
				nameShift = *varInfo->name.begin == '$' ? 1 : 0;
				sprintf(variableName, "%s%.*s_%d", namePrefix, int(varInfo->name.end-varInfo->name.begin) - nameShift, varInfo->name.begin + nameShift, varInfo->pos);
			}

			if(curr->targetLocal)
			{
				fprintf(fOut, "(int*)&%s", variableName);
			}else{
				assert(closureFunc->parentFunc);
				fprintf(fOut, "((int**)__%s_%d_ext_%d)[%d]", closureFunc->parentFunc->name, CodeInfo::FindFunctionByPtr(closureFunc->parentFunc), closureFunc->parentFunc->allParamSize, curr->targetPos >> 2);
			}
			fprintf(fOut, "),\r\n");
			OutputIdent(fOut);
			fprintf(fOut, "(((int**)%s)[%d] = (int*)__upvalue_%d_%s),\r\n", closureName, pos+1, CodeInfo::FindFunctionByPtr(varInfo->parentFunction), variableName);
			OutputIdent(fOut);
			fprintf(fOut, "(((int*)%s)[%d] = %d),\r\n", closureName, pos+2, curr->variable->varType->size);
			OutputIdent(fOut);
			fprintf(fOut, "(__upvalue_%d_%s = (__nullcUpvalue*)((int*)%s + %d))", CodeInfo::FindFunctionByPtr(varInfo->parentFunction), variableName, closureName, pos);
			if(curr->next)
				fprintf(fOut, ",\r\n");
			pos += ((varInfo->varType->size >> 2) < 3 ? 3 : 1 + (varInfo->varType->size >> 2));
		}
		if(nodeDereferenceEndInComma)
			fprintf(fOut, ",\r\n");
		else
			fprintf(fOut, ";\r\n");
	}else{
		if(!neutralized)
			fprintf(fOut, "*(");
		first->TranslateToC(fOut);
		if(!neutralized)
			fprintf(fOut, ")");
	}
}
NodeNumber* NodeDereference::Evaluate(char *memory, unsigned int size)
{
	if(!memory)
		return NULL;
	if(head)
		return NULL;
	if(typeInfo->size == 0)
		return NULL;

	if(neutralized)
	{
		return originalNode->Evaluate(memory, size);
	}else{
		// $$ original node extra nodes
		if(closureFunc)
		{
			return NULL;
		}else{
			unsigned int address = addrShift;
			if(!knownAddress)
			{
				NodeNumber *pointer = first->Evaluate(memory, size);
				if(!pointer)
					return NULL;
				address = pointer->GetInteger();
			}else if(absAddress){
				return NULL;
			}

			if(typeInfo == typeChar)
				return new NodeNumber(*(char*)(memory + address), typeInt);
			if(typeInfo == typeShort)
				return new NodeNumber(*(short*)(memory + address), typeInt);
			if(typeInfo == typeInt)
				return new NodeNumber(*(int*)(memory + address), typeInt);
			if(typeInfo == typeLong)
				return new NodeNumber(*(long long*)(memory + address), typeLong);
			if(typeInfo == typeFloat)
				return new NodeNumber(*(float*)(memory + address), typeDouble);
			if(typeInfo == typeDouble)
				return new NodeNumber(*(double*)(memory + address), typeDouble);
			return NULL;
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

void NodeShiftAddress::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s ShiftAddress [+%d]\r\n", typeInfo->GetFullTypeName(), memberShift);
	GoDownB();
	LogToStreamExtra(fGraph);
	first->LogToStream(fGraph);
	GoUp();
}
void NodeShiftAddress::TranslateToC(FILE *fOut)
{
	fprintf(fOut, "&(", member->name);
	first->TranslateToC(fOut);
	fprintf(fOut, ")->%s", member->name);
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

void NodePreOrPostOp::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s PreOrPostOp %s\r\n", typeInfo->GetFullTypeName(), (prefixOp ? "prefix" : "postfix"));
	GoDownB();
	LogToStreamExtra(fGraph);
	first->LogToStream(fGraph);
	GoUp();
}
void NodePreOrPostOp::TranslateToC(FILE *fOut)
{
	if(head)
		fprintf(fOut, "(");
	NodeZeroOP *curr = head;
	while(curr)
	{
		assert(curr->nodeType == typeNodePopOp);
		((NodePopOp*)curr)->GetFirstNode()->TranslateToC(fOut);
		fprintf(fOut, ", ");
		curr = curr->next;
	}
	if(typeInfo == typeDouble || typeInfo == typeFloat)
	{
		if(optimised)
		{
			OutputIdent(fOut);
			fprintf(fOut, "*(");
			first->TranslateToC(fOut);
			fprintf(fOut, incOp ? ") += 1.0" : ") -= 1.0");
		}else{
			if(prefixOp)
			{
				fprintf(fOut, "(*(");
				first->TranslateToC(fOut);
				fprintf(fOut, incOp ? ") += 1.0)" : ") -= 1.0)");
			}else{
				fprintf(fOut, "((*(");
				first->TranslateToC(fOut);
				fprintf(fOut, incOp ? ") += 1.0) - 1.0)" : ") -= 1.0) + 1.0)");
			}
		}
	}else{
		if(optimised)
			OutputIdent(fOut);
		if(prefixOp)
			fprintf(fOut, incOp ? "++" : "--");

		fprintf(fOut, "(*(");
		first->TranslateToC(fOut);
		fprintf(fOut, "))");

		if(!prefixOp)
			fprintf(fOut, incOp ? "++" : "--");
	}
	if(head)
		fprintf(fOut, ")");
	if(optimised)
		fprintf(fOut, ";\r\n");
}
NodeNumber* NodePreOrPostOp::Evaluate(char *memory, unsigned int size)
{
	(void)size;
	if(head)
		return NULL;
	if(!knownAddress || absAddress)
		return NULL;
	if(prefixOp)
	{
		if(typeInfo == typeChar)
			*(char*)(memory + addrShift) = *(char*)(memory + addrShift) + 1;
		else if(typeInfo == typeShort)
			*(short*)(memory + addrShift) = *(short*)(memory + addrShift) + 1;
		else if(typeInfo == typeInt)
			*(int*)(memory + addrShift) = *(int*)(memory + addrShift) + 1;
		else if(typeInfo == typeLong)
			*(long long*)(memory + addrShift) = *(long long*)(memory + addrShift) + 1ll;
		else if(typeInfo == typeFloat)
			*(float*)(memory + addrShift) = *(float*)(memory + addrShift) + 1.0f;
		else if(typeInfo == typeDouble)
			*(double*)(memory + addrShift) = *(double*)(memory + addrShift) + 1.0;
		else
			return NULL;
	}
	// Take number
	NodeNumber *value = NULL;
	if(typeInfo == typeChar)
		value = new NodeNumber(*(char*)(memory + addrShift), typeInt);
	else if(typeInfo == typeShort)
		value = new NodeNumber(*(short*)(memory + addrShift), typeInt);
	else if(typeInfo == typeInt)
		value = new NodeNumber(*(int*)(memory + addrShift), typeInt);
	else if(typeInfo == typeLong)
		value = new NodeNumber(*(long long*)(memory + addrShift), typeLong);
	else if(typeInfo == typeFloat)
		value = new NodeNumber(*(float*)(memory + addrShift), typeDouble);
	else if(typeInfo == typeDouble)
		value = new NodeNumber(*(double*)(memory + addrShift), typeDouble);
	if(!value)
		return NULL;
	if(!prefixOp)
	{
		if(typeInfo == typeChar)
			*(char*)(memory + addrShift) = *(char*)(memory + addrShift) + 1;
		else if(typeInfo == typeShort)
			*(short*)(memory + addrShift) = *(short*)(memory + addrShift) + 1;
		else if(typeInfo == typeInt)
			*(int*)(memory + addrShift) = *(int*)(memory + addrShift) + 1;
		else if(typeInfo == typeLong)
			*(long long*)(memory + addrShift) = *(long long*)(memory + addrShift) + 1ll;
		else if(typeInfo == typeFloat)
			*(float*)(memory + addrShift) = *(float*)(memory + addrShift) + 1.0f;
		else if(typeInfo == typeDouble)
			*(double*)(memory + addrShift) = *(double*)(memory + addrShift) + 1.0;
		else
			return NULL;
	}
	return value;
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

	CompileExtra();

	unsigned int ID = CodeInfo::FindFunctionByPtr(funcInfo);
	cmdList.push_back(VMCmd(cmdFuncAddr, ID));

	if(funcInfo->type == FunctionInfo::NORMAL)
	{
		NodeNumber nullPtr = NodeNumber(0ll, CodeInfo::GetReferenceType(typeVoid));
		nullPtr.Compile();
	}else if(funcInfo->type == FunctionInfo::LOCAL || funcInfo->type == FunctionInfo::THISCALL){
		first->Compile();
	}
}

void NodeFunctionAddress::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s FunctionAddress %s %s\r\n", typeInfo->GetFullTypeName(), funcInfo->name, (funcInfo->funcPtr ? " external" : ""));
	LogToStreamExtra(fGraph);
	if(first)
	{
		GoDownB();
		first->LogToStream(fGraph);
		GoUp();
	}
}
void NodeFunctionAddress::TranslateToC(FILE *fOut)
{
	fprintf(fOut, "(");
	nodeDereferenceEndInComma = true;
	TranslateToCExtra(fOut);
	nodeDereferenceEndInComma = false;
	OutputIdent(fOut);
	fprintf(fOut, "__nullcMakeFunction((void*)");
	OutputCFunctionName(fOut, funcInfo);
	fprintf(fOut, ", ");
	if(funcInfo->type == FunctionInfo::NORMAL)
	{
		fprintf(fOut, "(void*)%uu", funcInfo->funcPtr ? ~0u : 0u);
	}else if(funcInfo->type == FunctionInfo::LOCAL || funcInfo->type == FunctionInfo::THISCALL){
		if(first->nodeType == typeNodeDereference)
		{
			VariableInfo *closure = ((NodeGetAddress*)((NodeOneOP*)first)->GetFirstNode())->varInfo;

			if(closure->nameHash == GetStringHash("this"))
			{
				fprintf(fOut, "__context");
			}else{
				char closureName[NULLC_MAX_VARIABLE_NAME_LENGTH];
				const char *namePrefix = *closure->name.begin == '$' ? "__" : "";
				unsigned int nameShift = *closure->name.begin == '$' ? 1 : 0;
				unsigned int length = sprintf(closureName, "%s%.*s_%d", namePrefix, int(closure->name.end - closure->name.begin) - nameShift, closure->name.begin + nameShift, closure->pos);
				for(unsigned int k = 0; k < length; k++)
				{
					if(closureName[k] == ':' || closureName[k] == '$')
						closureName[k] = '_';
				}
				fprintf(fOut, "%s", closureName);
			}
		}else{
			first->TranslateToC(fOut);
		}
	}
	fprintf(fOut, "))");
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
		// If it's operator ^^ and first argument is false, jump to push 0 as result
		cmdList.push_back(VMCmd(cmdID == cmdLogOr ? cmdJmpNZ : cmdJmpZ, ~0u));	// Jump address will be fixed later on
		unsigned int specialJmp1 = cmdList.size() - 1;

		second->Compile();
		if(sST == STYPE_LONG)
			cmdList.push_back(VMCmd(cmdBitOr));

		// If it's operator || and first argument is true, jump to push 1 as result
		// If it's operator ^^ and first argument is false, jump to push 0 as result
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

void NodeBinaryOp::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s NodeBinaryOp<%s> :\r\n", typeInfo->GetFullTypeName(), binCommandToText[cmdID-cmdAdd]);
	assert(cmdID >= cmdAdd);
	assert(cmdID <= cmdNEqualD);
	GoDown();
	LogToStreamExtra(fGraph);
	first->LogToStream(fGraph);
	GoUp();
	GoDownB();
	second->LogToStream(fGraph);
	GoUp();
}
void NodeBinaryOp::TranslateToC(FILE *fOut)
{
	TypeInfo *tmpType = ChooseBinaryOpResultType(first->typeInfo, second->typeInfo);
	if(cmdID == cmdPow)
		fprintf(fOut, "__nullcPow");
	if(cmdID == cmdMod && typeInfo == typeDouble)
		fprintf(fOut, "__nullcMod");

	fprintf(fOut, "(");
	
	if(cmdID == cmdLogXor)
		fprintf(fOut, "!!");
	if(tmpType != first->typeInfo)
	{
		fprintf(fOut, "(");
		tmpType->OutputCType(fOut, "");
		fprintf(fOut, ")");
	}
	first->TranslateToC(fOut);
	if(!(cmdID == cmdPow || (cmdID == cmdMod && typeInfo == typeDouble)))
		fprintf(fOut, ")");
	fprintf(fOut, " %s ", cmdID == cmdLogXor ? "!=" : ((cmdID == cmdPow || (cmdID == cmdMod && typeInfo == typeDouble)) ? "," : binCommandToText[cmdID-cmdAdd]));
	if(!(cmdID == cmdPow || (cmdID == cmdMod && typeInfo == typeDouble)))
		fprintf(fOut, "(");
	if(cmdID == cmdLogXor)
		fprintf(fOut, "!!");
	if(tmpType != second->typeInfo)
	{
		fprintf(fOut, "(");
		tmpType->OutputCType(fOut, "");
		fprintf(fOut, ")");
	}
	second->TranslateToC(fOut);

	fprintf(fOut, ")");
}
NodeNumber* NodeBinaryOp::Evaluate(char *memory, unsigned int size)
{
	if(head)
		return NULL;

	if(cmdID == cmdLogOr || cmdID == cmdLogAnd)
	{
		NodeNumber *valueLeft = first->Evaluate(memory, size);
		if(!valueLeft)
			return NULL;
		// Convert long to int
		if(valueLeft->typeInfo == typeLong)
			valueLeft = new NodeNumber(valueLeft->GetLong() ? 1 : 0, typeInt);
		
		if(valueLeft->GetInteger() && cmdLogOr)
			return new NodeNumber(1, typeInt);
		if(!valueLeft->GetInteger() && cmdLogAnd)
			return new NodeNumber(0, typeInt);

		NodeNumber *valueRight = second->Evaluate(memory, size);
		if(!valueRight)
			return NULL;
		// Convert long to int
		if(valueRight->typeInfo == typeLong)
			valueRight = new NodeNumber(valueRight->GetLong() ? 1 : 0, typeInt);

		return new NodeNumber(valueRight->GetInteger() ? 1 : 0, typeInt);
	}else{
		TypeInfo *midType = ChooseBinaryOpResultType(first->typeInfo, second->typeInfo);

		// Compute first value
		NodeNumber *valueLeft = first->Evaluate(memory, size);
		if(!valueLeft)
			return NULL;

		// Convert it to the resulting type
		if(midType == typeDouble || midType == typeFloat)
			valueLeft->ConvertTo(typeDouble);
		else if(midType == typeLong)
			valueLeft->ConvertTo(typeLong);
		else if(midType == typeInt || midType == typeShort || midType == typeChar)
			valueLeft->ConvertTo(typeInt);

		// Compute second value
		NodeNumber *valueRight = second->Evaluate(memory, size);
		if(!valueRight)
			return NULL;
		// Convert it to the result type
		if(midType == typeDouble || midType == typeFloat)
			valueRight->ConvertTo(typeDouble);
		else if(midType == typeLong)
			valueRight->ConvertTo(typeLong);
		else if(midType == typeInt || midType == typeShort || midType == typeChar)
			valueRight->ConvertTo(typeInt);

		// Apply binary operation
		NodeNumber *value = NULL;
		if(midType == typeInt || midType == typeShort || midType == typeChar)
		{
			int result = optDoOperation(cmdID, valueLeft->GetInteger(), valueRight->GetInteger());
			value = new NodeNumber(result, typeInt);
		}else if(midType == typeLong){
			long long result = optDoOperation(cmdID, valueLeft->GetLong(), valueRight->GetLong());
			value = new NodeNumber(result, typeLong);
		}else if(midType == typeDouble || midType == typeFloat){
			double result = optDoOperation(cmdID, valueLeft->GetDouble(), valueRight->GetDouble());
			value = new NodeNumber(result, typeDouble);
		}
		if(value)
		{
			value->ConvertTo(typeInfo);
			return value;
		}
		return NULL;
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
		cmdList.push_back(VMCmd(cmdPop, 4));
	else if(first->typeInfo->stackType != STYPE_INT)
		cmdList.push_back(VMCmd(first->typeInfo->stackType == STYPE_DOUBLE ? cmdDtoI : cmdBitOr));
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
void NodeIfElseExpr::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s IfExpression :\r\n", typeInfo->GetFullTypeName());
	GoDown();
	LogToStreamExtra(fGraph);
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
void NodeIfElseExpr::TranslateToC(FILE *fOut)
{
	if(typeInfo != typeVoid)
	{
		first->TranslateToC(fOut);
		fprintf(fOut, " ? ");
		second->TranslateToC(fOut);
		fprintf(fOut, " : ");
		third->TranslateToC(fOut);
	}else{
		OutputIdent(fOut);
		fprintf(fOut, "if(");
		first->TranslateToC(fOut);
		fprintf(fOut, ")\r\n");
		OutputIdent(fOut);
		fprintf(fOut, "{\r\n");
		indentDepth++;
		second->TranslateToC(fOut);
		indentDepth--;
		if(third)
		{
			OutputIdent(fOut);
			fprintf(fOut, "}else{\r\n");
			indentDepth++;
			third->TranslateToC(fOut);
			indentDepth--;
		}
		OutputIdent(fOut);
		fprintf(fOut, "}\r\n");
	}
}
NodeNumber* NodeIfElseExpr::Evaluate(char *memory, unsigned int size)
{
	if(head)
		return NULL;

	// Compute condition
	NodeNumber *cond = first->Evaluate(memory, size);
	if(!cond)
		return NULL;

	cond->ConvertTo(typeInt);

	if(cond->GetInteger())
	{
		NodeNumber *value = second->Evaluate(memory, size);
		if(!value)
			return NULL;
		if(typeInfo != typeVoid && value->typeInfo != typeVoid)
			value->ConvertTo(ChooseBinaryOpResultType(second->typeInfo, third->typeInfo));
		return value;
	}else if(third){
		NodeNumber *value = third->Evaluate(memory, size);
		if(!value)
			return NULL;
		if(typeInfo != typeVoid && value->typeInfo != typeVoid)
			value->ConvertTo(ChooseBinaryOpResultType(second->typeInfo, third->typeInfo));
		return value;
	}
	return new NodeNumber(0, typeVoid);
}

//////////////////////////////////////////////////////////////////////////
// Nod for compilation of for(){}

unsigned int	currLoopDepth = 0;
const unsigned int TRANSLATE_MAX_LOOP_DEPTH = 64;
unsigned int	currLoopID[TRANSLATE_MAX_LOOP_DEPTH];

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
		cmdList.push_back(VMCmd(cmdPop, 4));
	else if(second->typeInfo->stackType != STYPE_INT)
		cmdList.push_back(VMCmd(second->typeInfo->stackType == STYPE_DOUBLE ? cmdDtoI : cmdBitOr));

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
void NodeForExpr::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s ForExpression :\r\n", typeInfo->GetFullTypeName());
	GoDown();
	LogToStreamExtra(fGraph);
	first->LogToStream(fGraph);
	second->LogToStream(fGraph);
	third->LogToStream(fGraph);
	GoUp();
	GoDownB(); 
	fourth->LogToStream(fGraph);
	GoUp();
}
void NodeForExpr::TranslateToC(FILE *fOut)
{
	currLoopDepth++;
	first->TranslateToC(fOut);
	OutputIdent(fOut); fprintf(fOut, "while(");
	second->TranslateToC(fOut);
	fprintf(fOut, ")\r\n");
	OutputIdent(fOut); fprintf(fOut, "{\r\n");
	indentDepth++;
	fourth->TranslateToC(fOut);
	OutputIdent(fOut); fprintf(fOut, "continue%d_%d:1;\r\n", currLoopID[currLoopDepth-1], currLoopDepth);
	third->TranslateToC(fOut);
	indentDepth--;
	OutputIdent(fOut); fprintf(fOut, "}\r\n");
	OutputIdent(fOut); fprintf(fOut, "break%d_%d:1;\r\n", currLoopID[currLoopDepth-1], currLoopDepth);
	currLoopDepth--;
	assert(currLoopDepth < TRANSLATE_MAX_LOOP_DEPTH);
	currLoopID[currLoopDepth]++;
}
NodeNumber* NodeForExpr::Evaluate(char *memory, unsigned int size)
{
	if(head || currLoopDepth)
		return NULL;

	// Compile initialization node
	NodeNumber *init = first->Evaluate(memory, size);
	if(!init)
		return NULL;
	
	NodeNumber *condition = second->Evaluate(memory, size);
	if(!condition)
		return NULL;
	unsigned int iteration = 0;
	while(condition->GetInteger())
	{
		currLoopDepth++;
		NodeNumber *body = fourth->Evaluate(memory, size);
		currLoopDepth--;
		if(!body)
			return NULL;
		NodeNumber *increment = third->Evaluate(memory, size);
		if(!increment)
			return NULL;
		condition = second->Evaluate(memory, size);
		if(!condition)
			return NULL;
		if(iteration++ > 128)
			return NULL;
	}
	return new NodeNumber(0, typeVoid);
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

	if(first->typeInfo == typeObject)
		cmdList.push_back(VMCmd(cmdPop, 4));
	else if(first->typeInfo->stackType != STYPE_INT)
		cmdList.push_back(VMCmd(first->typeInfo->stackType == STYPE_DOUBLE ? cmdDtoI : cmdBitOr));

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
void NodeWhileExpr::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s WhileExpression :\r\n", typeInfo->GetFullTypeName());
	GoDown();
	LogToStreamExtra(fGraph);
	first->LogToStream(fGraph);
	GoUp();
	GoDownB(); 
	second->LogToStream(fGraph);
	GoUp();
}
void NodeWhileExpr::TranslateToC(FILE *fOut)
{
	currLoopDepth++;
	OutputIdent(fOut); fprintf(fOut, "while(");
	first->TranslateToC(fOut);
	fprintf(fOut, ")\r\n");
	OutputIdent(fOut); fprintf(fOut, "{\r\n");
	indentDepth++;
	second->TranslateToC(fOut);
	OutputIdent(fOut); fprintf(fOut, "continue%d_%d:1;\r\n", currLoopID[currLoopDepth-1], currLoopDepth);
	indentDepth--;
	OutputIdent(fOut); fprintf(fOut, "}\r\n");
	OutputIdent(fOut); fprintf(fOut, "break%d_%d:1;\r\n", currLoopID[currLoopDepth-1], currLoopDepth);
	currLoopDepth--;
	assert(currLoopDepth < TRANSLATE_MAX_LOOP_DEPTH);
	currLoopID[currLoopDepth]++;
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
		cmdList.push_back(VMCmd(cmdPop, 4));
	else if(second->typeInfo->stackType != STYPE_INT)
		cmdList.push_back(VMCmd(second->typeInfo->stackType == STYPE_DOUBLE ? cmdDtoI : cmdBitOr));

	// Jump to beginning if condition == true
	cmdList.push_back(VMCmd(cmdJmpNZ, posStart));

	NodeContinueOp::SatisfyJumps(posCond);
	NodeBreakOp::SatisfyJumps(cmdList.size());

	currLoopDepth--;
}
void NodeDoWhileExpr::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s DoWhileExpression :\r\n", typeInfo->GetFullTypeName());
	GoDown();
	LogToStreamExtra(fGraph);
	first->LogToStream(fGraph);
	GoUp();
	GoDownB();
	second->LogToStream(fGraph);
	GoUp();
}
void NodeDoWhileExpr::TranslateToC(FILE *fOut)
{
	currLoopDepth++;
	OutputIdent(fOut); fprintf(fOut, "do\r\n");
	OutputIdent(fOut); fprintf(fOut, "{\r\n");
	indentDepth++;
	first->TranslateToC(fOut);
	OutputIdent(fOut); fprintf(fOut, "continue%d_%d:1;\r\n", currLoopID[currLoopDepth-1], currLoopDepth);
	indentDepth--;
	OutputIdent(fOut); fprintf(fOut, "} while(");
	second->TranslateToC(fOut);
	fprintf(fOut, ");\r\n");
	OutputIdent(fOut);
	fprintf(fOut, "break%d_%d:1;\r\n", currLoopID[currLoopDepth-1], currLoopDepth);
	currLoopDepth--;
	assert(currLoopDepth < TRANSLATE_MAX_LOOP_DEPTH);
	currLoopID[currLoopDepth]++;
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
void NodeBreakOp::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s BreakExpression\r\n", typeInfo->GetFullTypeName());
	LogToStreamExtra(fGraph);
}
void NodeBreakOp::TranslateToC(FILE *fOut)
{
	OutputIdent(fOut);
	if(breakDepth == 1)
		fprintf(fOut, "break;\r\n");
	else
		fprintf(fOut, "goto break%d_%d;\r\n", currLoopID[currLoopDepth-breakDepth], currLoopDepth - breakDepth + 1);
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
void NodeContinueOp::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s ContinueOp\r\n", typeInfo->GetFullTypeName());
	LogToStreamExtra(fGraph);
}
void NodeContinueOp::TranslateToC(FILE *fOut)
{
	OutputIdent(fOut);
	fprintf(fOut, "goto continue%d_%d;\r\n", currLoopID[currLoopDepth-continueDepth], currLoopDepth - continueDepth + 1);
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
void NodeSwitchExpr::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s SwitchExpression :\r\n", typeInfo->GetFullTypeName());
	GoDown();
	LogToStreamExtra(fGraph);
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
void NodeSwitchExpr::TranslateToC(FILE *fOut)
{
	static int switchNum = 0;
	int myNum = switchNum++;
	OutputIdent(fOut);
	fprintf(fOut, "do\r\n");
	OutputIdent(fOut);
	fprintf(fOut, "{\r\n");
	indentDepth++;
	char buf[64];
	sprintf(buf, "switchVar");
	OutputIdent(fOut);
	first->typeInfo->OutputCType(fOut, buf);
	fprintf(fOut, " = ");
	first->TranslateToC(fOut);
	fprintf(fOut, ";\r\n");
	unsigned int i = 0;
	for(NodeZeroOP *curr = conditionHead; curr; curr = curr->next, i++)
	{
		OutputIdent(fOut);
		fprintf(fOut, "if(switchVar == ");
		curr->TranslateToC(fOut);
		fprintf(fOut, ")\r\n");
		OutputIdent(fOut);
		fprintf(fOut, "\tgoto case%d_%d;\r\n", myNum, i);
	}
	OutputIdent(fOut);
	fprintf(fOut, "goto defaultCase_%d;\r\n", myNum);
	i = 0;
	for(NodeZeroOP *block = blockHead; block; block = block->next, i++)
	{
		OutputIdent(fOut);
		fprintf(fOut, "case%d_%d:\r\n", myNum, i);
		block->TranslateToC(fOut);
	}
	OutputIdent(fOut);
	fprintf(fOut, "defaultCase_%d:\r\n", myNum);
	if(defaultCase)
	{
		defaultCase->TranslateToC(fOut);
	}else{
		OutputIdent(fOut);
		fprintf(fOut, "0;\r\n");
	}
	indentDepth--;
	OutputIdent(fOut);
	fprintf(fOut, "}while(0);\r\n");
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
void NodeExpressionList::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s NodeExpressionList :\r\n", typeInfo->GetFullTypeName());
	GoDown();
	LogToStreamExtra(fGraph);
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
void NodeExpressionList::TranslateToC(FILE *fOut)
{
	if(typeInfo->arrLevel && typeInfo->arrSize != TypeInfo::UNSIZED_ARRAY && typeInfo->subType == typeChar)
	{
		fprintf(fOut, "\"");
		NodeZeroOP	*curr = tail->prev;
		do 
		{
			assert(curr->nodeType == typeNodeNumber);
			NodeNumber *dword = (NodeNumber*)curr;
			for(unsigned int i = 0; i < 4; i++)
			{
				unsigned char ch = (unsigned char)((dword->GetInteger() >> (i * 8)) & 0xff);
				if(ch >= ' ' && ch <= 128 && ch != '\"' && ch != '\\' && ch != '\'')
					fprintf(fOut, "%c", ch);
				else
					fprintf(fOut, "\\x%x", ch);
			}
			curr = curr->prev;
		}while(curr);
		fprintf(fOut, "\"");
		return;
	}else if(typeInfo != typeVoid){
		NodeZeroOP *end = first;
		if(first->nodeType == typeNodePopOp)
		{
			fprintf(fOut, "(");
			((NodePopOp*)first)->GetFirstNode()->TranslateToC(fOut);
			fprintf(fOut, ", ");
			end = first->next;
		}
		if(typeInfo->arrLevel && typeInfo->arrSize == TypeInfo::UNSIZED_ARRAY)
			fprintf(fOut, "__makeNullcArray(");
		else if(first->nodeType != typeNodePopOp){
			typeInfo->OutputCType(fOut, "()");
			end = first->next;
		}

		NodeZeroOP	*curr = tail;
		unsigned int id = 0;
		do 
		{
			if(typeInfo->arrLevel && typeInfo->arrSize != TypeInfo::UNSIZED_ARRAY)
				fprintf(fOut, ".set(%d, ", id++);
			curr->TranslateToC(fOut);
			if(typeInfo->arrLevel && typeInfo->arrSize != TypeInfo::UNSIZED_ARRAY)
				fprintf(fOut, ")");
			else if(curr != end)
				fprintf(fOut, ", ");
			curr = curr->prev;
		}while(curr != end->prev);

		if(typeInfo->arrLevel && typeInfo->arrSize == TypeInfo::UNSIZED_ARRAY)
			fprintf(fOut, ")");

		if(first->nodeType == typeNodePopOp)
			fprintf(fOut, ")");
	}else{
		NodeZeroOP	*curr = first;
		do 
		{
			curr->TranslateToC(fOut);
			if(curr != tail && typeInfo != typeVoid)
				fprintf(fOut, ", ");
			curr = curr->next;
		}while(curr);
	}
}
NodeNumber* NodeExpressionList::Evaluate(char *memory, unsigned int size)
{
	if(head)
		return NULL;
	if(typeInfo != typeVoid)
		return NULL;

	NodeZeroOP	*curr = first;
	NodeNumber	*value = NULL;
	do 
	{
		value = curr->Evaluate(memory, size);
		if(!value)
			return NULL;
		if(value && value->typeInfo != typeVoid)
			return value;
		curr = curr->next;
	}while(curr);
	return value;
}

void ResetTreeGlobals()
{
	currLoopDepth = 0;
	memset(currLoopID, 0, sizeof(unsigned int) * TRANSLATE_MAX_LOOP_DEPTH);
	nodeDereferenceEndInComma = false;
	NodeBreakOp::fixQueue.clear();
	NodeContinueOp::fixQueue.clear();
	NodeFuncCall::memoList.clear();
	NodeFuncCall::memoPool.Clear();
}
