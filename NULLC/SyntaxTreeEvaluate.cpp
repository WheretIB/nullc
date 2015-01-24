#include "SyntaxTree.h"
#include "CodeInfo.h"
#include "ConstantFold.h"

NodeNumber* NodeZeroOP::Evaluate(char* memory, unsigned int size)
{
	(void)memory;
	(void)size;
	return NULL;	// by default, node evaluation is unknown
}

NodeNumber* NodeNumber::Evaluate(char *memory, unsigned int size)
{
	(void)memory;
	(void)size;
	return new NodeNumber(*this);
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

NodeNumber* NodeBlock::Evaluate(char *memory, unsigned int size)
{
	if(head)
		return NULL;
	if(parentFunction->closeUpvals)
		return NULL;
	return first->Evaluate(memory, size);
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

	unsigned oldBaseShift = NodeFuncCall::baseShift;
	NodeFuncCall::baseShift = size;
	// Evaluate function code
	NodeNumber *result = first->Evaluate(memory, memSize);
	NodeFuncCall::baseShift = oldBaseShift;

	return result;
}

unsigned int NodeFuncCall::callCount = 0;
unsigned int NodeFuncCall::baseShift = 0;
ChunkedStackPool<4092> NodeFuncCall::memoPool;
FastVector<NodeFuncCall::CallMemo> NodeFuncCall::memoList;

NodeNumber* NodeFuncCall::Evaluate(char *memory, unsigned int size)
{
	// Extra nodes disable evaluation, as also does indirect function call
	if(head || first || !funcInfo || !funcInfo->pure)
		return NULL;

	// Check that we have enough place for parameters
	if(funcInfo->allParamSize > size || funcType->paramCount > 16)
		return NULL;

	// Limit call count
	if(callCount++ > 256)
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
				// If this is first function call (from AddFunctionCallNode), and parameter is not a known number, exit immediately.
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

		unsigned size = paramValue[i]->typeInfo->size;
		if(size && size < 4)
			size = 4;
		offset -= size;
	}

	// Find old result
	for(unsigned int i = 0; i < memoList.size(); i++)
	{
		if(memoList[i].func == funcInfo && memcmp(memory, memoList[i].arguments, funcInfo->allParamSize) == 0)
			return new NodeNumber(*memoList[i].value);
	}

	// Call function
	NodeNumber *result = ((NodeFuncDef*)funcInfo->functionNode)->Evaluate(memory, size);
	if(result && result->typeInfo != funcInfo->retType)
		result = NULL;

	// Memoization
	if(result)
	{
		memoList.push_back(CallMemo());
		memoList.back().arguments = (char*)memoPool.Allocate(funcInfo->allParamSize);
		memcpy(memoList.back().arguments, memory, funcInfo->allParamSize);
		memoList.back().func = funcInfo;
		memoList.back().value = result;

		return result;
	}

	return NULL;
}

NodeNumber* NodeGetAddress::Evaluate(char *memory, unsigned int size)
{
	(void)memory;
	(void)size;
	if(head)
		return NULL;
	if(varInfo->isGlobal)
		return NULL;
	return new NodeNumber(int(trackAddress ? varInfo->pos : varAddress), typeInt);
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
			*(char*)(memory + addrShift) = *(char*)(memory + addrShift) + 1 * (incOp ? 1 : -1);
		else if(typeInfo == typeShort)
			*(short*)(memory + addrShift) = *(short*)(memory + addrShift) + 1 * (incOp ? 1 : -1);
		else if(typeInfo == typeInt)
			*(int*)(memory + addrShift) = *(int*)(memory + addrShift) + 1 * (incOp ? 1 : -1);
		else if(typeInfo == typeLong)
			*(long long*)(memory + addrShift) = *(long long*)(memory + addrShift) + 1ll * (incOp ? 1 : -1);
		else if(typeInfo == typeFloat)
			*(float*)(memory + addrShift) = *(float*)(memory + addrShift) + 1.0f * (incOp ? 1 : -1);
		else if(typeInfo == typeDouble)
			*(double*)(memory + addrShift) = *(double*)(memory + addrShift) + 1.0 * (incOp ? 1 : -1);
		else
			return NULL;
	}
	// Take number
	NodeNumber *value = NULL;
	if(!optimised)
	{
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
	}
	if(!prefixOp)
	{
		if(typeInfo == typeChar)
			*(char*)(memory + addrShift) = *(char*)(memory + addrShift) + 1 * (incOp ? 1 : -1);
		else if(typeInfo == typeShort)
			*(short*)(memory + addrShift) = *(short*)(memory + addrShift) + 1 * (incOp ? 1 : -1);
		else if(typeInfo == typeInt)
			*(int*)(memory + addrShift) = *(int*)(memory + addrShift) + 1 * (incOp ? 1 : -1);
		else if(typeInfo == typeLong)
			*(long long*)(memory + addrShift) = *(long long*)(memory + addrShift) + 1ll * (incOp ? 1 : -1);
		else if(typeInfo == typeFloat)
			*(float*)(memory + addrShift) = *(float*)(memory + addrShift) + 1.0f * (incOp ? 1 : -1);
		else if(typeInfo == typeDouble)
			*(double*)(memory + addrShift) = *(double*)(memory + addrShift) + 1.0 * (incOp ? 1 : -1);
		else
			return NULL;
	}
	return optimised ? new NodeNumber(0, typeVoid) : value;
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
		
		if(valueLeft->GetInteger() && cmdID == cmdLogOr)
			return new NodeNumber(1, typeInt);
		if(!valueLeft->GetInteger() && cmdID == cmdLogAnd)
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

unsigned int	evaluateLoopDepth = 0;

NodeNumber* NodeForExpr::Evaluate(char *memory, unsigned int size)
{
	if(head || evaluateLoopDepth)
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
		evaluateLoopDepth++;
		NodeNumber *body = fourth->Evaluate(memory, size);
		evaluateLoopDepth--;
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

#pragma warning(disable: 4702) // unreachable code
NodeNumber*	NodeFunctionProxy::Evaluate(char *memory, unsigned int size)
{
	(void)memory; (void)size;
	ThrowError(codePos, "ERROR: internal compiler error at NodeFunctionProxy::Evaluate");
	return NULL;
}
#pragma warning(default: 4702)

NodeNumber* NodePointerCast::Evaluate(char *memory, unsigned int size)
{
	if(head)
		return NULL;

	return first->Evaluate(memory, size);
}

NodeNumber* NodeGetFunctionContext::Evaluate(char *memory, unsigned int size)
{
	(void)memory;
	(void)size;

	return NULL;
}

NodeNumber* NodeGetCoroutineState::Evaluate(char *memory, unsigned int size)
{
	(void)memory;
	(void)size;

	return NULL;
}

NodeNumber* NodeCreateUnsizedArray::Evaluate(char *memory, unsigned int size)
{
	(void)memory;
	(void)size;

	return NULL;
}

NodeNumber* NodeCreateAutoArray::Evaluate(char *memory, unsigned int size)
{
	(void)memory;
	(void)size;

	return NULL;
}
