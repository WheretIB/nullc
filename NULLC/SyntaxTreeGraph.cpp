#include "SyntaxTree.h"

namespace
{
	const unsigned prefixLimit = 512;
	char	linePrefix[prefixLimit];

	int	level = 0;
	unsigned int prefixSize = 2;

	bool preNeedChange = false;
}

void StartGraphGeneration()
{
	memset(linePrefix, 0, prefixLimit);
	level = 0;
	prefixSize = 2;
	preNeedChange = false;
}

void RemovePrefix(unsigned length)
{
	prefixSize -= length;

	if(prefixSize < prefixLimit + 1)
		linePrefix[prefixSize] = 0;
}

void AddPrefix(const char *str)
{
	unsigned length = unsigned(strlen(str));
	for(unsigned i = 0; i < length; i++)
	{
		if(prefixSize < prefixLimit + 1)
			linePrefix[prefixSize++] = str[i];
	}
}

void GoDown()
{
	level++;

	RemovePrefix(2);
	AddPrefix("  |__");
}

void GoDownB()
{
	GoDown();
	preNeedChange = true;
}

void GoUp()
{
	if(level == 0)
		return;

	level--;

	RemovePrefix(5);
	AddPrefix("__");
}

void EndGraphGeneration()
{
}

void DrawLine(FILE *fGraph)
{
	fprintf(fGraph, "%s", linePrefix);
	if(preNeedChange)
	{
		preNeedChange = false;
		GoUp();

		level++;

		RemovePrefix(2);
		AddPrefix("   __");
	}
}

void NodeZeroOP::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s ZeroOp\r\n", typeInfo->GetFullTypeName());
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

void NodeOneOP::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s OneOP :\r\n", typeInfo->GetFullTypeName());
	GoDown();
	LogToStreamExtra(fGraph);
	first->LogToStream(fGraph);
	GoUp();
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

void NodeNumber::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s Number :\r\n", typeInfo->GetFullTypeName());
	GoDown();
	DrawLine(fGraph);
	if(typeInfo == typeInt)
		fprintf(fGraph, "%d\r\n", num.integer);
	else if(typeInfo == typeFloat || typeInfo == typeDouble)
		fprintf(fGraph, "%f\r\n", num.real);
	else if(typeInfo == typeLong)
		fprintf(fGraph, "%lld\r\n", num.integer64);
	else if(typeInfo->refLevel)
		fprintf(fGraph, "0x%08x\r\n", num.integer);
	else
		fprintf(fGraph, "int(%d) or double(%f) or long(%lld)\r\n", num.integer, num.real, num.integer64);
	GoUp();
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

void NodeUnaryOp::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s UnaryOp :\r\n", typeInfo->GetFullTypeName());
	GoDownB();
	LogToStreamExtra(fGraph);
	first->LogToStream(fGraph);
	GoUp();
}

void NodeReturnOp::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	TypeInfo *tInfo = typeInfo ? typeInfo : first->typeInfo;
	fprintf(fGraph, "%s ReturnOp (yield: %d yield result: %d):\r\n", tInfo->GetFullTypeName(), parentFunction ? parentFunction->type == FunctionInfo::COROUTINE : false, yieldResult);
	GoDownB();
	LogToStreamExtra(fGraph);
	first->LogToStream(fGraph);
	GoUp();
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

void NodeFuncCall::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s FuncCall '%s' id: %d parameters: %d\r\n", typeInfo->GetFullTypeName(), (funcInfo ? funcInfo->name : "$ptr"), (funcInfo ? funcInfo->indexInArr : -1), funcType->paramCount);
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

void NodeGetAddress::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s GetAddress ", typeInfo->GetFullTypeName());
	if(varInfo)
		fprintf(fGraph, "%s '%.*s'", varInfo->varType->GetFullTypeName(), int(varInfo->name.end-varInfo->name.begin), varInfo->name.begin);
	else
		fprintf(fGraph, "$$$");
	fprintf(fGraph, " (%d %s)\r\n", varInfo ? varInfo->pos : varAddress, (varInfo->isGlobal ? " absolute" : " relative"));
	LogToStreamExtra(fGraph);
}

void NodeGetUpvalue::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s GetUpvalue (base + %d)[%d]\r\n", typeInfo->GetFullTypeName(), closurePos, closureElem);
	LogToStreamExtra(fGraph);
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

void NodeShiftAddress::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s ShiftAddress [+%d]\r\n", typeInfo->GetFullTypeName(), memberShift);
	GoDownB();
	LogToStreamExtra(fGraph);
	first->LogToStream(fGraph);
	GoUp();
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

void NodeBreakOp::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s BreakExpression\r\n", typeInfo->GetFullTypeName());
	LogToStreamExtra(fGraph);
}

void NodeContinueOp::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s ContinueOp\r\n", typeInfo->GetFullTypeName());
	LogToStreamExtra(fGraph);
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

void NodeFunctionProxy::LogToStream(FILE *fGraph)
{
	(void)fGraph;
}

void NodePointerCast::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s NodePointerCast :\r\n", typeInfo->GetFullTypeName());
	GoDownB();
	LogToStreamExtra(fGraph);
	first->LogToStream(fGraph);
	GoUp();
}

void NodeGetFunctionContext::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s NodeGetFunctionContext :\r\n", typeInfo->GetFullTypeName());
	GoDownB();
	LogToStreamExtra(fGraph);
	first->LogToStream(fGraph);
	GoUp();
}

void NodeGetCoroutineState::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s NodeGetCoroutineState :\r\n", typeInfo->GetFullTypeName());
	GoDownB();
	LogToStreamExtra(fGraph);
	first->LogToStream(fGraph);
	GoUp();
}

void NodeCreateUnsizedArray::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s NodeCreateUnsizedArray :\r\n", typeInfo->GetFullTypeName());
	GoDown();
	LogToStreamExtra(fGraph);
	first->LogToStream(fGraph);
	GoUp();
	GoDownB();
	second->LogToStream(fGraph);
	GoUp();
}

void NodeCreateAutoArray::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s NodeCreateAutoArray :\r\n", typeInfo->GetFullTypeName());
	GoDown();
	LogToStreamExtra(fGraph);
	first->LogToStream(fGraph);
	GoUp();
	GoDownB();
	second->LogToStream(fGraph);
	GoUp();
}
