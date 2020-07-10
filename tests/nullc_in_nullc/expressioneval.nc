import std.memory;
import std.vector;
import std.hashmap;
import expressiontree;
import expressioncontext;

void memcpy(char[] dst, int offset, @T ref value, int size)
{
	assert(sizeof(T) == size);

	if(T == char)
		memory.write(dst, offset, *value);
	else if(T == short)
		memory.write(dst, offset, *value);
	else if(T == int)
		memory.write(dst, offset, *value);
	else if(T == long)
		memory.write(dst, offset, *value);
	else if(T == float)
		memory.write(dst, offset, *value);
	else if(T == double)
		memory.write(dst, offset, *value);
	else
		assert(false, "unknown type");
}

void memcpy(char[] dst, @T ref value, int size)
{
	memcpy(dst, 0, value, size);
}

void memcpy(@T ref value, char[] src, int offset, int size)
{
	assert(sizeof(T) == size);

	if(T == char)
		*value = memory.read_char(src, offset);
	else if(T == short)
		*value = memory.read_short(src, offset);
	else if(T == int)
		*value = memory.read_int(src, offset);
	else if(T == long)
		*value = memory.read_long(src, offset);
	else if(T == float)
		*value = memory.read_float(src, offset);
	else if(T == double)
		*value = memory.read_double(src, offset);
	else
		assert(false, "unknown type");
}

void memcpy(@T ref value, char[] src, int size)
{
	memcpy(value, src, 0, size);
}

void memcpy(char[] dst, char[] src, int size)
{
	memory.copy(dst, 0, src, 0, size);
}

void memset(char[] dst, int offset, int value, int size)
{
	assert(value == 0);
	
	memory.set(dst, offset, value, size);
}

void memset(char[] dst, int value, int size)
{
	memset(dst, 0, value, size);
}

int memcmp(char[] a, char[] b, int size)
{
	return memory.compare(a, 0, b, 0, size);
}

class StackVariable
{
	void StackVariable(VariableData ref variable, ExprPointerLiteral ref ptr)
	{
		this.variable = variable;
		this.ptr = ptr;
	}

	VariableData ref variable;
	ExprPointerLiteral ref ptr;
}

class StackFrame
{
	void StackFrame(FunctionData ref owner)
	{
		this.owner = owner;
	}

	FunctionData ref owner;

	vector<StackVariable> variables;

	ExprBase ref returnValue;

	int targetYield;

	int breakDepth;
	int continueDepth;
}

class ExpressionEvalContext
{
	void ExpressionEvalContext(ExpressionContext ref ctx)
	{
		this.ctx = ctx;

		errorBuf = nullptr;
		errorCritical = false;

		globalFrame = nullptr;

		emulateKnownExternals = false;

		stackDepthLimit = 64;

		variableMemoryLimit = 8 * 1024;

		totalMemory = 0;
		totalMemoryLimit = 64 * 1024;

		instruction = 0;
		instructionsLimit = 64 * 1024;

		expressionDepth = 0;
		expressionDepthLimit = 2048;
	}

	ExpressionContext ref ctx;

	// Error info
	char[] errorBuf;
	bool errorCritical; // If error wasn't a limitation of the evaluation engine, but was an error in the program itself

	StackFrame ref globalFrame;

	vector<StackFrame ref> stackFrames;
	int stackDepthLimit;

	bool emulateKnownExternals;

	vector<ExprPointerLiteral ref> abandonedMemory;

	int variableMemoryLimit;

	int totalMemory;
	int totalMemoryLimit;

	int instruction;
	int instructionsLimit;

	int expressionDepth;
	int expressionDepthLimit;

	vector<EvalMemoryBlock> memoryBlocks;
}

EvalMemoryBlock AllocateMemory(ExpressionEvalContext ref ctx, int size)
{
	char[] buffer = new char[size];

	if(ctx.memoryBlocks.size() == 0)
		ctx.memoryBlocks.push_back(EvalMemoryBlock(1, buffer));
	else
		ctx.memoryBlocks.push_back(EvalMemoryBlock(ctx.memoryBlocks.back().address + ctx.memoryBlocks.back().buffer.size, buffer));

	return *ctx.memoryBlocks.back();
}

int GetMemoryAddress(ExpressionEvalContext ref ctx, EvalMemoryBlock block, int offset)
{
	return block.address + offset;
}

EvalMemoryBlock GetMemoryBuffer(ExpressionEvalContext ref ctx, int address, int ref offset)
{
	assert(address != 0);
	assert(ctx.memoryBlocks.size() != 0);

	int lowerBound = 0;
	int upperBound = ctx.memoryBlocks.size() - 1;
	int index = 0;

	while(lowerBound <= upperBound)
	{
		index = (lowerBound + upperBound) >> 1;

		if(address < ctx.memoryBlocks[index].address)
			upperBound = index - 1;
		else if(address >= ctx.memoryBlocks[index].address + ctx.memoryBlocks[index].buffer.size)
			lowerBound = index + 1;
		else
			break;
	}

	*offset = address - ctx.memoryBlocks[index].address;

	assert(*offset < ctx.memoryBlocks[index].buffer.size);

	return ctx.memoryBlocks[index];
}

ExprBase ref Evaluate(ExpressionEvalContext ref ctx, ExprBase ref expression);

ExprBase ref Report(ExpressionEvalContext ref ctx, char[] msg, auto ref[] args)
{
	if(ctx.errorBuf)
	{
		/*va_list args;
		va_start(args, msg);

		vsnprintf(ctx.errorBuf, ctx.errorBufSize, msg, args);

		va_end(args);

		ctx.errorBuf[ctx.errorBufSize - 1] = '\0';*/
	}

	ctx.errorCritical = false;

	return nullptr;
}

ExprBase ref ReportCritical(ExpressionEvalContext ref ctx, char[] msg, auto ref[] args)
{
	if(ctx.errorBuf)
	{
		/*va_list args;
		va_start(args, msg);

		vsnprintf(ctx.errorBuf, ctx.errorBufSize, msg, args);

		va_end(args);

		ctx.errorBuf[ctx.errorBufSize - 1] = '\0';*/
	}

	ctx.errorCritical = true;

	return nullptr;
}

bool AddInstruction(ExpressionEvalContext ref ctx)
{
	if(ctx.instruction < ctx.instructionsLimit)
	{
		ctx.instruction++;
		return true;
	}

	Report(ctx, "ERROR: instruction limit reached");

	return false;
}

ExprPointerLiteral ref AllocateTypeStorage(ExpressionEvalContext ref ctx, SynBase ref source, TypeBase ref type)
{
	if(isType with<TypeError>(type))
		return nullptr;

	for(int i = 0; i < ctx.abandonedMemory.size(); i++)
	{
		ExprPointerLiteral ref ptr = ctx.abandonedMemory[i];

		if(ptr.end - ptr.start == type.size)
		{
			ptr.type = ctx.ctx.GetReferenceType(type);

			memset(ptr.ptr.buffer, ptr.start, 0, int(type.size));

			ctx.abandonedMemory[i] = *ctx.abandonedMemory.back();
			ctx.abandonedMemory.pop_back();

			return ptr;
		}
	}

	if(type.size > ctx.variableMemoryLimit)
		return (ExprPointerLiteral ref)(Report(ctx, "ERROR: single variable memory limit"));

	if(ctx.totalMemory + type.size > ctx.totalMemoryLimit)
		return (ExprPointerLiteral ref)(Report(ctx, "ERROR: total variable memory limit"));

	ctx.totalMemory += int(type.size);

	auto mem = AllocateMemory(ctx, type.size);

	return new ExprPointerLiteral(source, ctx.ctx.GetReferenceType(type), mem, 0, type.size);
}

void FreeMemoryLiteral(ExpressionEvalContext ref ctx, ExprMemoryLiteral ref mem)
{
	ctx.abandonedMemory.push_back(mem.ptr);
}

bool CreateStore(ExpressionEvalContext ref ctx, ExprBase ref target, ExprBase ref value)
{
	// No side-effects while coroutine is skipping to target node
	if(!ctx.stackFrames.empty())
		assert(ctx.stackFrames.back().targetYield == 0);

	if(isType with<ExprNullptrLiteral>(target))
	{
		Report(ctx, "ERROR: store to null pointer");

		return false;
	}

	ExprPointerLiteral ref ptr = getType with<ExprPointerLiteral>(target);

	assert(ptr != nullptr);
	assert(ptr.start + value.type.size <= ptr.end);

	if(ExprBoolLiteral ref expr = getType with<ExprBoolLiteral>(value))
	{
		memcpy(ptr.ptr.buffer, ptr.start, &expr.value, int(value.type.size));
		return true;
	}

	if(ExprCharacterLiteral ref expr = getType with<ExprCharacterLiteral>(value))
	{
		memcpy(ptr.ptr.buffer, ptr.start, &expr.value, int(value.type.size));
		return true;
	}

	if(ExprStringLiteral ref expr = getType with<ExprStringLiteral>(value))
	{
		memcpy(ptr.ptr.buffer, ptr.start, expr.value, int(value.type.size));
		return true;
	}

	if(ExprIntegerLiteral ref expr = getType with<ExprIntegerLiteral>(value))
	{
		if(value.type.size == 1)
		{
			char tmp = expr.value;
			memcpy(ptr.ptr.buffer, ptr.start, &tmp, int(value.type.size));
		}
		else if(value.type.size == 2)
		{
			short tmp = expr.value;
			memcpy(ptr.ptr.buffer, ptr.start, &tmp, int(value.type.size));
		}
		else if(value.type.size == 4)
		{
			int tmp = expr.value;
			memcpy(ptr.ptr.buffer, ptr.start, &tmp, int(value.type.size));
		}
		else if(value.type.size == 8)
		{
			memcpy(ptr.ptr.buffer, ptr.start, &expr.value, int(value.type.size));
		}
		else
		{
			assert(false, "unknown ExprIntegerLiteral type");
		}

		return true;
	}

	if(ExprRationalLiteral ref expr = getType with<ExprRationalLiteral>(value))
	{
		if(expr.type == ctx.ctx.typeFloat)
		{
			float tmp = float(expr.value);
			memcpy(ptr.ptr.buffer, ptr.start, &tmp, int(value.type.size));
			return true;
		}

		memcpy(ptr.ptr.buffer, ptr.start, &expr.value, int(value.type.size));
		return true;
	}

	if(ExprTypeLiteral ref expr = getType with<ExprTypeLiteral>(value))
	{
		if(isType with<TypeError>(expr.value))
			return false;

		if(isType with<TypeArgumentSet>(expr.value) || isType with<TypeMemberSet>(expr.value))
			return false;

		int index = ctx.ctx.GetTypeIndex(expr.value);
		memcpy(ptr.ptr.buffer, ptr.start, &index, int(value.type.size));
		return true;
	}

	if(isType with<ExprNullptrLiteral>(value))
	{
		memset(ptr.ptr.buffer, ptr.start, 0, int(value.type.size));
		return true;
	}

	if(ExprFunctionIndexLiteral ref expr = getType with<ExprFunctionIndexLiteral>(value))
	{
		int index = expr.function ? ctx.ctx.GetFunctionIndex(expr.function) + 1 : 0;
		memcpy(ptr.ptr.buffer, ptr.start, &index, int(value.type.size));
		return true;
	}

	if(ExprFunctionLiteral ref expr = getType with<ExprFunctionLiteral>(value))
	{
		int index = expr.data ? ctx.ctx.GetFunctionIndex(expr.data) + 1 : 0;
		memcpy(ptr.ptr.buffer, ptr.start, &index, sizeof(int));

		if(isType with<ExprNullptrLiteral>(expr.context))
			memset(ptr.ptr.buffer, ptr.start + 4, 0, sizeof(void ref));
		else if(ExprPointerLiteral ref context = getType with<ExprPointerLiteral>(expr.context))
			memcpy(ptr.ptr.buffer, ptr.start + 4, GetMemoryAddress(ctx, context.ptr, context.start), sizeof(void ref));
		else
			return false;

		return true;
	}

	if(ExprPointerLiteral ref expr = getType with<ExprPointerLiteral>(value))
	{
		TypeRef ref ptrType = getType with<TypeRef>(expr.type);

		assert(ptrType != nullptr);
		assert(expr.start + ptrType.subType.size <= expr.end);

		memcpy(ptr.ptr.buffer, ptr.start, GetMemoryAddress(ctx, expr.ptr, expr.start), int(value.type.size));
		return true;
	}

	if(ExprMemoryLiteral ref expr = getType with<ExprMemoryLiteral>(value))
	{
		memory.copy(ptr.ptr.buffer, ptr.start, expr.ptr.ptr.buffer, expr.ptr.start, int(value.type.size));
		return true;
	}

	Report(ctx, "ERROR: unknown store type");

	return false;
}

ExprBase ref CreateLoad(ExpressionEvalContext ref ctx, ExprBase ref target)
{
	// No side-effects while coroutine is skipping to target node
	if(!ctx.stackFrames.empty())
		assert(ctx.stackFrames.back().targetYield == 0);

	if(isType with<ExprNullptrLiteral>(target))
	{
		Report(ctx, "ERROR: load from null pointer");

		return nullptr;
	}

	ExprPointerLiteral ref ptr = getType with<ExprPointerLiteral>(target);

	assert(ptr != nullptr);

	TypeRef ref refType = getType with<TypeRef>(target.type);

	assert(refType != nullptr);

	TypeBase ref type = refType.subType;

	assert(ptr.start + type.size <= ptr.end);

	if(type == ctx.ctx.typeBool)
	{
		bool value;
		assert(type.size == sizeof(value));
		memcpy(&value, ptr.ptr.buffer, ptr.start, int(type.size));

		return new ExprBoolLiteral(target.source, type, value);
	}

	if(type == ctx.ctx.typeChar)
	{
		char value;
		assert(type.size == sizeof(value));
		memcpy(&value, ptr.ptr.buffer, ptr.start, int(type.size));

		return new ExprCharacterLiteral(target.source, type, value);
	}

	if(type == ctx.ctx.typeShort)
	{
		short value;
		assert(type.size == sizeof(value));
		memcpy(&value, ptr.ptr.buffer, ptr.start, int(type.size));

		return new ExprIntegerLiteral(target.source, type, value);
	}

	if(type == ctx.ctx.typeInt)
	{
		int value;
		assert(type.size == sizeof(value));
		memcpy(&value, ptr.ptr.buffer, ptr.start, int(type.size));

		return new ExprIntegerLiteral(target.source, type, value);
	}

	if(type == ctx.ctx.typeLong)
	{
		long value;
		assert(type.size == sizeof(value));
		memcpy(&value, ptr.ptr.buffer, ptr.start, int(type.size));

		return new ExprIntegerLiteral(target.source, type, value);
	}

	if(type == ctx.ctx.typeFloat)
	{
		float value;
		assert(type.size == sizeof(value));
		memcpy(&value, ptr.ptr.buffer, ptr.start, int(type.size));

		return new ExprRationalLiteral(target.source, type, value);
	}

	if(type == ctx.ctx.typeDouble)
	{
		double value;
		assert(type.size == sizeof(value));
		memcpy(&value, ptr.ptr.buffer, ptr.start, int(type.size));

		return new ExprRationalLiteral(target.source, type, value);
	}

	if(type == ctx.ctx.typeTypeID)
	{
		int index = 0;
		memcpy(&index, ptr.ptr.buffer, ptr.start, sizeof(int));

		TypeBase ref data = ctx.ctx.types[index];

		return new ExprTypeLiteral(target.source, type, data);
	}

	if(type == ctx.ctx.typeFunctionID)
	{
		int index = 0;
		memcpy(&index, ptr.ptr.buffer, ptr.start, sizeof(int));

		FunctionData ref data = index != 0 ? ctx.ctx.functions[index - 1] : nullptr;

		return new ExprFunctionIndexLiteral(target.source, type, data);
	}

	if(type == ctx.ctx.typeNullPtr)
	{
		return new ExprNullptrLiteral(target.source, type);
	}

	if(isType with<TypeFunction>(type))
	{
		int index = 0;
		memcpy(&index, ptr.ptr.buffer, ptr.start, sizeof(int));

		FunctionData ref data = index != 0 ? ctx.ctx.functions[index - 1] : nullptr;

		int value = 0;
		memcpy(&value, ptr.ptr.buffer, ptr.start + 4, sizeof(value));
		
		if(!data)
		{
			assert(value == 0);

			return new ExprFunctionLiteral(target.source, type, nullptr, new ExprNullptrLiteral(target.source, ctx.ctx.GetReferenceType(ctx.ctx.typeVoid)));
		}

		TypeRef ref ptrType = getType with<TypeRef>(data.contextType);

		assert(ptrType != nullptr);

		ExprBase ref context = nullptr;

		if(value == 0)
		{
			context = new ExprNullptrLiteral(target.source, ptrType);
		}
		else
		{
			int blockOffset;
			auto block = GetMemoryBuffer(ctx, value, &blockOffset);
			context = new ExprPointerLiteral(target.source, ptrType, block, blockOffset, ptrType.subType.size);
		}

		return new ExprFunctionLiteral(target.source, type, data, context);
	}

	if(TypeRef ref ptrType = getType with<TypeRef>(type))
	{
		int value;
		assert(type.size == sizeof(value));
		memcpy(&value, ptr.ptr.buffer, ptr.start, int(type.size));
		
		if(value == 0)
			return new ExprNullptrLiteral(target.source, type);

		int blockOffset;
		auto block = GetMemoryBuffer(ctx, value, &blockOffset);

		return new ExprPointerLiteral(target.source, type, block, blockOffset, ptrType.subType.size);
	}

	ExprPointerLiteral ref storage = AllocateTypeStorage(ctx, target.source, type);

	if(!storage)
		return nullptr;

	memory.copy(storage.ptr.buffer, storage.start, ptr.ptr.buffer, ptr.start, int(type.size));

	return new ExprMemoryLiteral(target.source, type, storage);
}

bool CreateInsert(ExpressionEvalContext ref ctx, ExprMemoryLiteral ref mem, int offset, ExprBase ref value)
{
	assert(mem.ptr.start + value.type.size <= mem.ptr.end);

	ExprPointerLiteral addr = ExprPointerLiteral(mem.source, ctx.ctx.GetReferenceType(value.type), mem.ptr.ptr, mem.ptr.start + offset, mem.ptr.start + offset + value.type.size);

	if(!CreateStore(ctx, &addr, value))
		return false;

	return true;
}

ExprBase ref CreateExtract(ExpressionEvalContext ref ctx, ExprMemoryLiteral ref mem, int offset, TypeBase ref type)
{
	assert(mem.ptr.start + type.size <= mem.ptr.end);

	ExprPointerLiteral addr = ExprPointerLiteral(mem.source, ctx.ctx.GetReferenceType(type), mem.ptr.ptr, mem.ptr.start + offset, mem.ptr.start + offset + type.size);

	return CreateLoad(ctx, &addr);
}

ExprMemoryLiteral ref CreateConstruct(ExpressionEvalContext ref ctx, TypeBase ref type, ExprBase ref el0, ExprBase ref el1, ExprBase ref el2)
{
	long size = 0;
	
	if(el0)
		size += el0.type.size;

	if(el1)
		size += el1.type.size;

	if(el2)
		size += el2.type.size;

	assert(type.size == size);

	ExprPointerLiteral ref storage = AllocateTypeStorage(ctx, el0.source, type);

	if(!storage)
		return nullptr;

	ExprMemoryLiteral ref mem = new ExprMemoryLiteral(el0.source, type, storage);

	int offset = 0;

	if(el0 && !CreateInsert(ctx, mem, offset, el0))
		return nullptr;
	else if(el0)
		offset += int(el0.type.size);

	if(el1 && !CreateInsert(ctx, mem, offset, el1))
		return nullptr;
	else if(el1)
		offset += int(el1.type.size);

	if(el2 && !CreateInsert(ctx, mem, offset, el2))
		return nullptr;
	else if(el2)
		offset += int(el2.type.size);

	return mem;
}

ExprPointerLiteral ref FindVariableStorage(ExpressionEvalContext ref ctx, VariableData ref data)
{
	if(ctx.stackFrames.empty())
		return (ExprPointerLiteral ref)(Report(ctx, "ERROR: no stack frame"));

	StackFrame ref frame = *ctx.stackFrames.back();

	for(int i = 0; i < frame.variables.size(); i++)
	{
		StackVariable ref variable = &frame.variables[i];

		if(variable.variable == data)
			return variable.ptr;
	}

	if(ctx.globalFrame)
	{
		for(int i = 0; i < ctx.globalFrame.variables.size(); i++)
		{
			StackVariable ref variable = &ctx.globalFrame.variables[i];

			if(variable.variable == data)
				return variable.ptr;
		}
	}

	if(data.importModule != nullptr)
		return (ExprPointerLiteral ref)(Report(ctx, "ERROR: can't access external variable '%.*s'", FMT_ISTR(data.name.name)));

	return (ExprPointerLiteral ref)(Report(ctx, "ERROR: variable '%.*s' not found", FMT_ISTR(data.name.name)));
}

bool TryTakeLong(ExprBase ref expression, long ref result)
{
	if(ExprBoolLiteral ref expr = getType with<ExprBoolLiteral>(expression))
	{
		*result = expr.value ? 1 : 0;
		return true;
	}

	if(ExprCharacterLiteral ref expr = getType with<ExprCharacterLiteral>(expression))
	{
		*result = expr.value;
		return true;
	}

	if(ExprIntegerLiteral ref expr = getType with<ExprIntegerLiteral>(expression))
	{
		*result = expr.value;
		return true;
	}

	if(ExprRationalLiteral ref expr = getType with<ExprRationalLiteral>(expression))
	{
		*result = expr.value;
		return true;
	}

	return false;
}

bool TryTakeDouble(ExprBase ref expression, double ref result)
{
	if(ExprBoolLiteral ref expr = getType with<ExprBoolLiteral>(expression))
	{
		*result = expr.value ? 1.0 : 0.0;
		return true;
	}

	if(ExprCharacterLiteral ref expr = getType with<ExprCharacterLiteral>(expression))
	{
		*result = expr.value;
		return true;
	}

	if(ExprIntegerLiteral ref expr = getType with<ExprIntegerLiteral>(expression))
	{
		*result = expr.value;
		return true;
	}

	if(ExprRationalLiteral ref expr = getType with<ExprRationalLiteral>(expression))
	{
		*result = expr.value;
		return true;
	}

	return false;
}

bool TryTakeTypeId(ExprBase ref expression, TypeBase ref ref result)
{
	if(ExprTypeLiteral ref expr = getType with<ExprTypeLiteral>(expression))
	{
		*result = expr.value;
		return true;
	}

	return false;
}

bool TryTakePointer(ExpressionEvalContext ref ctx, ExprBase ref expression, int ref result)
{
	if(isType with<ExprNullptrLiteral>(expression))
	{
		*result = 0;
		return true;
	}
	else if(ExprPointerLiteral ref expr = getType with<ExprPointerLiteral>(expression))
	{
		*result = GetMemoryAddress(ctx, expr.ptr, expr.start);
		return true;
	}

	return false;
}

ExprBase ref CreateBinaryOp(ExpressionEvalContext ref ctx, SynBase ref source, ExprBase ref lhs, ExprBase ref unevaluatedRhs, SynBinaryOpType op)
{
	if(isType with<TypeError>(lhs.type) || isType with<TypeError>(unevaluatedRhs.type))
		return Report(ctx, "ERROR: encountered an error node");

	assert(lhs.type == unevaluatedRhs.type);

	if((ctx.ctx.IsIntegerType(lhs.type) || isType with<TypeEnum>(lhs.type)) && (ctx.ctx.IsIntegerType(unevaluatedRhs.type) || isType with<TypeEnum>(unevaluatedRhs.type)))
	{
		long lhsValue = 0;
		long rhsValue = 0;

		// Short-circuit behaviour
		if(op == SynBinaryOpType.SYN_BINARY_OP_LOGICAL_AND)
		{
			if(TryTakeLong(lhs, lhsValue))
			{
				if(lhsValue == 0)
					return new ExprBoolLiteral(source, ctx.ctx.typeBool, false);

				ExprBase ref rhs = Evaluate(ctx, unevaluatedRhs);

				if(!rhs)
					return nullptr;

				if(TryTakeLong(rhs, rhsValue))
					return new ExprBoolLiteral(source, ctx.ctx.typeBool, rhsValue != 0);
			}

			return nullptr;
		}

		if(op == SynBinaryOpType.SYN_BINARY_OP_LOGICAL_OR)
		{
			if(TryTakeLong(lhs, lhsValue))
			{
				if(lhsValue == 1)
					return new ExprBoolLiteral(source, ctx.ctx.typeBool, true);

				ExprBase ref rhs = Evaluate(ctx, unevaluatedRhs);

				if(!rhs)
					return nullptr;

				if(TryTakeLong(rhs, rhsValue))
					return new ExprBoolLiteral(source, ctx.ctx.typeBool, rhsValue != 0);
			}

			return nullptr;
		}

		ExprBase ref rhs = Evaluate(ctx, unevaluatedRhs);

		if(!rhs)
			return nullptr;

		assert(lhs.type == rhs.type);

		TypeBase ref resultType = ctx.ctx.typeInt;

		if(lhs.type == ctx.ctx.typeLong || rhs.type == ctx.ctx.typeLong)
			resultType = ctx.ctx.typeLong;

		if(TryTakeLong(lhs, lhsValue) && TryTakeLong(rhs, rhsValue))
		{
			if(resultType == ctx.ctx.typeInt)
			{
				int lhsValueInt = int(lhsValue);
				int rhsValueInt = int(rhsValue);

				switch(op)
				{
				case SynBinaryOpType.SYN_BINARY_OP_ADD:
					return new ExprIntegerLiteral(source, lhs.type, lhsValueInt + rhsValueInt);
				case SynBinaryOpType.SYN_BINARY_OP_SUB:
					return new ExprIntegerLiteral(source, lhs.type, lhsValueInt - rhsValueInt);
				case SynBinaryOpType.SYN_BINARY_OP_MUL:
					return new ExprIntegerLiteral(source, lhs.type, int(rhsValueInt));
				case SynBinaryOpType.SYN_BINARY_OP_DIV:
					if(rhsValueInt == 0)
						return ReportCritical(ctx, "ERROR: division by zero during constant folding");

					return new ExprIntegerLiteral(source, lhs.type, lhsValueInt / rhsValueInt);
				case SynBinaryOpType.SYN_BINARY_OP_MOD:
					if(rhsValueInt == 0)
						return ReportCritical(ctx, "ERROR: modulus division by zero during constant folding");

					return new ExprIntegerLiteral(source, lhs.type, lhsValueInt % rhsValueInt);
				case SynBinaryOpType.SYN_BINARY_OP_POW:
					if(rhsValueInt < 0)
						return ReportCritical(ctx, "ERROR: negative power on integer number in exponentiation during constant folding");

					int result, power;

					result = 1;
					power = rhsValueInt;

					while(power)
					{
						if(power & 1)
						{
							result *= lhsValueInt;
							power--;
						}
						lhsValueInt *= lhsValueInt;
						power >>= 1;
					}

					return new ExprIntegerLiteral(source, lhs.type, result);
				case SynBinaryOpType.SYN_BINARY_OP_SHL:
					if(rhsValueInt < 0)
						return ReportCritical(ctx, "ERROR: negative shift value");

					return new ExprIntegerLiteral(source, lhs.type, lhsValueInt << rhsValueInt);
				case SynBinaryOpType.SYN_BINARY_OP_SHR:
					if(rhsValueInt < 0)
						return ReportCritical(ctx, "ERROR: negative shift value");

					return new ExprIntegerLiteral(source, lhs.type, lhsValueInt >> rhsValueInt);
				case SynBinaryOpType.SYN_BINARY_OP_LESS:
					return new ExprBoolLiteral(source, ctx.ctx.typeBool, lhsValueInt < rhsValueInt);
				case SynBinaryOpType.SYN_BINARY_OP_LESS_EQUAL:
					return new ExprBoolLiteral(source, ctx.ctx.typeBool, lhsValueInt <= rhsValueInt);
				case SynBinaryOpType.SYN_BINARY_OP_GREATER:
					return new ExprBoolLiteral(source, ctx.ctx.typeBool, lhsValueInt > rhsValueInt);
				case SynBinaryOpType.SYN_BINARY_OP_GREATER_EQUAL:
					return new ExprBoolLiteral(source, ctx.ctx.typeBool, lhsValueInt >= rhsValueInt);
				case SynBinaryOpType.SYN_BINARY_OP_EQUAL:
					return new ExprBoolLiteral(source, ctx.ctx.typeBool, lhsValueInt == rhsValueInt);
				case SynBinaryOpType.SYN_BINARY_OP_NOT_EQUAL:
					return new ExprBoolLiteral(source, ctx.ctx.typeBool, lhsValueInt != rhsValueInt);
				case SynBinaryOpType.SYN_BINARY_OP_BIT_AND:
					return new ExprIntegerLiteral(source, lhs.type, lhsValueInt & rhsValueInt);
				case SynBinaryOpType.SYN_BINARY_OP_BIT_OR:
					return new ExprIntegerLiteral(source, lhs.type, lhsValueInt | rhsValueInt);
				case SynBinaryOpType.SYN_BINARY_OP_BIT_XOR:
					return new ExprIntegerLiteral(source, lhs.type, lhsValueInt ^ rhsValueInt);
				case SynBinaryOpType.SYN_BINARY_OP_LOGICAL_XOR:
					return new ExprBoolLiteral(source, ctx.ctx.typeBool, !!lhsValueInt != !!rhsValueInt);
				default:
					assert(false, "unexpected type");
					break;
				}
			}
			else
			{
				switch(op)
				{
				case SynBinaryOpType.SYN_BINARY_OP_ADD:
					return new ExprIntegerLiteral(source, lhs.type, lhsValue + rhsValue);
				case SynBinaryOpType.SYN_BINARY_OP_SUB:
					return new ExprIntegerLiteral(source, lhs.type, lhsValue - rhsValue);
				case SynBinaryOpType.SYN_BINARY_OP_MUL:
					return new ExprIntegerLiteral(source, lhs.type, rhsValue);
				case SynBinaryOpType.SYN_BINARY_OP_DIV:
					if(rhsValue == 0)
						return ReportCritical(ctx, "ERROR: division by zero during constant folding");

					return new ExprIntegerLiteral(source, lhs.type, lhsValue / rhsValue);
				case SynBinaryOpType.SYN_BINARY_OP_MOD:
					if(rhsValue == 0)
						return ReportCritical(ctx, "ERROR: modulus division by zero during constant folding");

					return new ExprIntegerLiteral(source, lhs.type, lhsValue % rhsValue);
				case SynBinaryOpType.SYN_BINARY_OP_POW:
					if(rhsValue < 0)
						return ReportCritical(ctx, "ERROR: negative power on integer number in exponentiation during constant folding");

					long result, power;

					result = 1;
					power = rhsValue;

					while(power)
					{
						if(power & 1)
						{
							result *= lhsValue;
							power--;
						}
						lhsValue *= lhsValue;
						power >>= 1;
					}

					return new ExprIntegerLiteral(source, lhs.type, result);
				case SynBinaryOpType.SYN_BINARY_OP_SHL:
					if(rhsValue < 0)
						return ReportCritical(ctx, "ERROR: negative shift value");

					return new ExprIntegerLiteral(source, lhs.type, lhsValue << rhsValue);
				case SynBinaryOpType.SYN_BINARY_OP_SHR:
					if(rhsValue < 0)
						return ReportCritical(ctx, "ERROR: negative shift value");

					return new ExprIntegerLiteral(source, lhs.type, lhsValue >> rhsValue);
				case SynBinaryOpType.SYN_BINARY_OP_LESS:
					return new ExprBoolLiteral(source, ctx.ctx.typeBool, lhsValue < rhsValue);
				case SynBinaryOpType.SYN_BINARY_OP_LESS_EQUAL:
					return new ExprBoolLiteral(source, ctx.ctx.typeBool, lhsValue <= rhsValue);
				case SynBinaryOpType.SYN_BINARY_OP_GREATER:
					return new ExprBoolLiteral(source, ctx.ctx.typeBool, lhsValue > rhsValue);
				case SynBinaryOpType.SYN_BINARY_OP_GREATER_EQUAL:
					return new ExprBoolLiteral(source, ctx.ctx.typeBool, lhsValue >= rhsValue);
				case SynBinaryOpType.SYN_BINARY_OP_EQUAL:
					return new ExprBoolLiteral(source, ctx.ctx.typeBool, lhsValue == rhsValue);
				case SynBinaryOpType.SYN_BINARY_OP_NOT_EQUAL:
					return new ExprBoolLiteral(source, ctx.ctx.typeBool, lhsValue != rhsValue);
				case SynBinaryOpType.SYN_BINARY_OP_BIT_AND:
					return new ExprIntegerLiteral(source, lhs.type, lhsValue & rhsValue);
				case SynBinaryOpType.SYN_BINARY_OP_BIT_OR:
					return new ExprIntegerLiteral(source, lhs.type, lhsValue | rhsValue);
				case SynBinaryOpType.SYN_BINARY_OP_BIT_XOR:
					return new ExprIntegerLiteral(source, lhs.type, lhsValue ^ rhsValue);
				case SynBinaryOpType.SYN_BINARY_OP_LOGICAL_XOR:
					return new ExprBoolLiteral(source, ctx.ctx.typeBool, !!lhsValue != !!rhsValue);
				default:
					assert(false, "unexpected type");
					break;
				}
			}
		}

		return nullptr;
	}

	ExprBase ref rhs = Evaluate(ctx, unevaluatedRhs);

	if(!rhs)
		return nullptr;

	if(ctx.ctx.IsFloatingPointType(lhs.type) && ctx.ctx.IsFloatingPointType(rhs.type))
	{
		assert(lhs.type == rhs.type);

		double lhsValue = 0;
		double rhsValue = 0;

		if(TryTakeDouble(lhs, lhsValue) && TryTakeDouble(rhs, rhsValue))
		{
			switch(op)
			{
			case SynBinaryOpType.SYN_BINARY_OP_ADD:
				return new ExprRationalLiteral(source, lhs.type, lhsValue + rhsValue);
			case SynBinaryOpType.SYN_BINARY_OP_SUB:
				return new ExprRationalLiteral(source, lhs.type, lhsValue - rhsValue);
			case SynBinaryOpType.SYN_BINARY_OP_MUL:
				return new ExprRationalLiteral(source, lhs.type, lhsValue * rhsValue);
			case SynBinaryOpType.SYN_BINARY_OP_DIV:
				return new ExprRationalLiteral(source, lhs.type, lhsValue / rhsValue);
			/*case SynBinaryOpType.SYN_BINARY_OP_MOD:
				return new ExprRationalLiteral(source, lhs.type, fmod(lhsValue, rhsValue));
			case SynBinaryOpType.SYN_BINARY_OP_POW:
				return new ExprRationalLiteral(source, lhs.type, pow(lhsValue, rhsValue));*/
			case SynBinaryOpType.SYN_BINARY_OP_LESS:
				return new ExprBoolLiteral(source, ctx.ctx.typeBool, lhsValue < rhsValue);
			case SynBinaryOpType.SYN_BINARY_OP_LESS_EQUAL:
				return new ExprBoolLiteral(source, ctx.ctx.typeBool, lhsValue <= rhsValue);
			case SynBinaryOpType.SYN_BINARY_OP_GREATER:
				return new ExprBoolLiteral(source, ctx.ctx.typeBool, lhsValue > rhsValue);
			case SynBinaryOpType.SYN_BINARY_OP_GREATER_EQUAL:
				return new ExprBoolLiteral(source, ctx.ctx.typeBool, lhsValue >= rhsValue);
			case SynBinaryOpType.SYN_BINARY_OP_EQUAL:
				return new ExprBoolLiteral(source, ctx.ctx.typeBool, lhsValue == rhsValue);
			case SynBinaryOpType.SYN_BINARY_OP_NOT_EQUAL:
				return new ExprBoolLiteral(source, ctx.ctx.typeBool, lhsValue != rhsValue);
			default:
				assert(false, "unexpected type");
				break;
			}
		}
	}
	else if(lhs.type == ctx.ctx.typeTypeID && rhs.type == ctx.ctx.typeTypeID)
	{
		TypeBase ref lhsValue = nullptr;
		TypeBase ref rhsValue = nullptr;

		if(TryTakeTypeId(lhs, &lhsValue) && TryTakeTypeId(rhs, &rhsValue))
		{
			if(op == SynBinaryOpType.SYN_BINARY_OP_EQUAL)
				return new ExprBoolLiteral(source, ctx.ctx.typeBool, lhsValue == rhsValue);

			if(op == SynBinaryOpType.SYN_BINARY_OP_NOT_EQUAL)
				return new ExprBoolLiteral(source, ctx.ctx.typeBool, lhsValue != rhsValue);
		}
	}
	else if(isType with<TypeRef>(lhs.type) && isType with<TypeRef>(rhs.type))
	{
		assert(lhs.type == rhs.type);

		int lPtr = 0;

		if(isType with<ExprNullptrLiteral>(lhs))
			lPtr = 0;
		else if(ExprPointerLiteral ref value = getType with<ExprPointerLiteral>(lhs))
			lPtr = GetMemoryAddress(ctx, value.ptr, value.start);
		else
			assert(false, "unknown type");

		int rPtr = 0;

		if(isType with<ExprNullptrLiteral>(rhs))
			rPtr = 0;
		else if(ExprPointerLiteral ref value = getType with<ExprPointerLiteral>(rhs))
			rPtr = GetMemoryAddress(ctx, value.ptr, value.start);
		else
			assert(false, "unknown type");

		if(op == SynBinaryOpType.SYN_BINARY_OP_EQUAL)
			return new ExprBoolLiteral(source, ctx.ctx.typeBool, lPtr == rPtr);

		if(op == SynBinaryOpType.SYN_BINARY_OP_NOT_EQUAL)
			return new ExprBoolLiteral(source, ctx.ctx.typeBool, lPtr != rPtr);
	}

	return Report(ctx, "ERROR: failed to eval binary op");
}

ExprBase ref CheckType(ExprBase ref expression, ExprBase ref value)
{
	if(isType with<TypeError>(expression.type))
		return nullptr;

	assert(expression.type == value.type);

	return value;
}

ExprBase ref EvaluateVoid(ExpressionEvalContext ref ctx, ExprBase ref expression)
{
	return CheckType(expression, new ExprVoid(expression.source, ctx.ctx.typeVoid));
}

ExprBase ref EvaluateBoolLiteral(ExpressionEvalContext ref ctx, ExprBoolLiteral ref expression)
{
	return CheckType(expression, new ExprBoolLiteral(expression.source, expression.type, expression.value));
}

ExprBase ref EvaluateCharacterLiteral(ExpressionEvalContext ref ctx, ExprCharacterLiteral ref expression)
{
	return CheckType(expression, new ExprCharacterLiteral(expression.source, expression.type, expression.value));
}

ExprBase ref EvaluateStringLiteral(ExpressionEvalContext ref ctx, ExprStringLiteral ref expression)
{
	return CheckType(expression, new ExprStringLiteral(expression.source, expression.type, expression.value, expression.length));
}

ExprBase ref EvaluateIntegerLiteral(ExpressionEvalContext ref ctx, ExprIntegerLiteral ref expression)
{
	return CheckType(expression, new ExprIntegerLiteral(expression.source, expression.type, expression.value));
}

ExprBase ref EvaluateRationalLiteral(ExpressionEvalContext ref ctx, ExprRationalLiteral ref expression)
{
	return CheckType(expression, new ExprRationalLiteral(expression.source, expression.type, expression.value));
}

ExprBase ref EvaluateTypeLiteral(ExpressionEvalContext ref ctx, ExprTypeLiteral ref expression)
{
	return CheckType(expression, new ExprTypeLiteral(expression.source, expression.type, expression.value));
}

ExprBase ref EvaluateNullptrLiteral(ExpressionEvalContext ref ctx, ExprNullptrLiteral ref expression)
{
	return CheckType(expression, new ExprNullptrLiteral(expression.source, expression.type));
}

ExprBase ref EvaluateFunctionIndexLiteral(ExpressionEvalContext ref ctx, ExprFunctionIndexLiteral ref expression)
{
	return CheckType(expression, new ExprFunctionIndexLiteral(expression.source, expression.type, expression.function));
}

ExprBase ref EvaluateArray(ExpressionEvalContext ref ctx, ExprArray ref expression)
{
	if(!ctx.stackFrames.empty() && ctx.stackFrames.back().targetYield)
		return new ExprVoid(expression.source, ctx.ctx.typeVoid);

	if(!AddInstruction(ctx))
		return nullptr;

	ExprPointerLiteral ref storage = AllocateTypeStorage(ctx, expression.source, expression.type);

	if(!storage)
		return nullptr;

	TypeArray ref arrayType = getType with<TypeArray>(expression.type);

	assert(arrayType != nullptr);

	int offset = 0;

	for(ExprBase ref value = expression.values.head; value; value = value.next)
	{
		ExprBase ref element = Evaluate(ctx, value);

		if(!element)
			return nullptr;

		assert(storage.start + offset + arrayType.subType.size <= storage.end);

		ExprPointerLiteral ref target = new ExprPointerLiteral(expression.source, ctx.ctx.GetReferenceType(arrayType.subType), storage.ptr, storage.start + offset, storage.start + offset + arrayType.subType.size);

		if(!CreateStore(ctx, target, element))
			return nullptr;

		offset += int(arrayType.subType.size);
	}

	ExprBase ref load = CreateLoad(ctx, storage);

	if(!load)
		return nullptr;

	return CheckType(expression, load);
}

ExprBase ref EvaluatePreModify(ExpressionEvalContext ref ctx, ExprPreModify ref expression)
{
	if(!ctx.stackFrames.empty() && ctx.stackFrames.back().targetYield)
		return new ExprVoid(expression.source, ctx.ctx.typeVoid);

	if(!AddInstruction(ctx))
		return nullptr;

	ExprBase ref ptr = Evaluate(ctx, expression.value);

	if(!ptr)
		return nullptr;

	ExprBase ref value = CreateLoad(ctx, ptr);

	if(!value)
		return nullptr;

	ExprBase ref modified = CreateBinaryOp(ctx, expression.source, value, new ExprIntegerLiteral(expression.source, value.type, 1), expression.isIncrement ? SynBinaryOpType.SYN_BINARY_OP_ADD : SynBinaryOpType.SYN_BINARY_OP_SUB);

	if(!modified)
		return nullptr;

	if(!CreateStore(ctx, ptr, modified))
		return nullptr;

	return CheckType(expression, modified);
}

ExprBase ref EvaluatePostModify(ExpressionEvalContext ref ctx, ExprPostModify ref expression)
{
	if(!ctx.stackFrames.empty() && ctx.stackFrames.back().targetYield)
		return new ExprVoid(expression.source, ctx.ctx.typeVoid);

	if(!AddInstruction(ctx))
		return nullptr;

	ExprBase ref ptr = Evaluate(ctx, expression.value);

	if(!ptr)
		return nullptr;

	ExprBase ref value = CreateLoad(ctx, ptr);

	if(!value)
		return nullptr;

	ExprBase ref modified = CreateBinaryOp(ctx, expression.source, value, new ExprIntegerLiteral(expression.source, value.type, 1), expression.isIncrement ? SynBinaryOpType.SYN_BINARY_OP_ADD : SynBinaryOpType.SYN_BINARY_OP_SUB);

	if(!modified)
		return nullptr;

	if(!CreateStore(ctx, ptr, modified))
		return nullptr;

	return CheckType(expression, value);
}

ExprBase ref EvaluateCast(ExpressionEvalContext ref ctx, ExprTypeCast ref expression)
{
	if(!AddInstruction(ctx))
		return nullptr;

	ExprBase ref value = Evaluate(ctx, expression.value);

	if(!value)
		return nullptr;

	switch(expression.category)
	{
	case ExprTypeCastCategory.EXPR_CAST_NUMERICAL:
		if(ctx.ctx.IsIntegerType(expression.type))
		{
			long result = 0;

			if(ExprRationalLiteral ref expr = getType with<ExprRationalLiteral>(value))
			{
				if(expression.type == ctx.ctx.typeBool)
					return CheckType(expression, new ExprBoolLiteral(expression.source, ctx.ctx.typeBool, expr.value != 0.0));

				if(expression.type == ctx.ctx.typeChar)
					return CheckType(expression, new ExprCharacterLiteral(expression.source, ctx.ctx.typeChar, expr.value));

				if(expression.type == ctx.ctx.typeShort)
					return CheckType(expression, new ExprIntegerLiteral(expression.source, ctx.ctx.typeShort, expr.value));

				if(expression.type == ctx.ctx.typeInt)
					return CheckType(expression, new ExprIntegerLiteral(expression.source, ctx.ctx.typeInt, expr.value));

				if(expression.type == ctx.ctx.typeLong)
					return CheckType(expression, new ExprIntegerLiteral(expression.source, ctx.ctx.typeLong, expr.value));
			}
			else if(TryTakeLong(value, result))
			{
				if(expression.type == ctx.ctx.typeBool)
					return CheckType(expression, new ExprBoolLiteral(expression.source, ctx.ctx.typeBool, result != 0));

				if(expression.type == ctx.ctx.typeChar)
					return CheckType(expression, new ExprCharacterLiteral(expression.source, ctx.ctx.typeChar, result));

				if(expression.type == ctx.ctx.typeShort)
					return CheckType(expression, new ExprIntegerLiteral(expression.source, ctx.ctx.typeShort, result));

				if(expression.type == ctx.ctx.typeInt)
					return CheckType(expression, new ExprIntegerLiteral(expression.source, ctx.ctx.typeInt, result));

				if(expression.type == ctx.ctx.typeLong)
					return CheckType(expression, new ExprIntegerLiteral(expression.source, ctx.ctx.typeLong, result));
			}
		}
		else if(ctx.ctx.IsFloatingPointType(expression.type))
		{
			double result = 0.0;

			if(TryTakeDouble(value, result))
			{
				if(expression.type == ctx.ctx.typeFloat)
					return CheckType(expression, new ExprRationalLiteral(expression.source, ctx.ctx.typeFloat, result));

				if(expression.type == ctx.ctx.typeDouble)
					return CheckType(expression, new ExprRationalLiteral(expression.source, ctx.ctx.typeDouble, result));
			}
		}
		break;
	case ExprTypeCastCategory.EXPR_CAST_PTR_TO_BOOL:
		return CheckType(expression, new ExprBoolLiteral(expression.source, ctx.ctx.typeBool, !isType with<ExprNullptrLiteral>(value)));
	case ExprTypeCastCategory.EXPR_CAST_UNSIZED_TO_BOOL:
		{
			ExprMemoryLiteral ref memLiteral = getType with<ExprMemoryLiteral>(value);

			ExprBase ref ptr = CreateExtract(ctx, memLiteral, 0, ctx.ctx.GetReferenceType(ctx.ctx.typeVoid));

			return CheckType(expression, new ExprBoolLiteral(expression.source, ctx.ctx.typeBool, !isType with<ExprNullptrLiteral>(ptr)));
		}
		break;
	case ExprTypeCastCategory.EXPR_CAST_FUNCTION_TO_BOOL:
		{
			ExprFunctionLiteral ref funcLiteral = getType with<ExprFunctionLiteral>(value);

			return CheckType(expression, new ExprBoolLiteral(expression.source, ctx.ctx.typeBool, funcLiteral.data != nullptr));
		}
		break;
	case ExprTypeCastCategory.EXPR_CAST_NULL_TO_PTR:
		return CheckType(expression, new ExprNullptrLiteral(expression.source, expression.type));
	case ExprTypeCastCategory.EXPR_CAST_NULL_TO_AUTO_PTR:
		{
			ExprBase ref typeId = new ExprTypeLiteral(expression.source, ctx.ctx.typeTypeID, ctx.ctx.typeVoid);
			ExprBase ref ptr = new ExprNullptrLiteral(expression.source, ctx.ctx.GetReferenceType(ctx.ctx.typeVoid));

			ExprBase ref result = CreateConstruct(ctx, expression.type, typeId, ptr, nullptr);

			if(!result)
				return nullptr;

			return CheckType(expression, result);
		}
		break;
	case ExprTypeCastCategory.EXPR_CAST_NULL_TO_UNSIZED:
		{
			ExprBase ref ptr = new ExprNullptrLiteral(expression.source, ctx.ctx.GetReferenceType(ctx.ctx.typeVoid));
			ExprBase ref size = new ExprIntegerLiteral(expression.source, ctx.ctx.typeInt, 0);

			ExprBase ref result = CreateConstruct(ctx, expression.type, ptr, size, nullptr);

			if(!result)
				return nullptr;

			return CheckType(expression, result);
		}
		break;
	case ExprTypeCastCategory.EXPR_CAST_NULL_TO_AUTO_ARRAY:
		{
			ExprBase ref typeId = new ExprTypeLiteral(expression.source, ctx.ctx.typeTypeID, ctx.ctx.typeVoid);
			ExprBase ref ptr = new ExprNullptrLiteral(expression.source, ctx.ctx.GetReferenceType(ctx.ctx.typeVoid));
			ExprBase ref length = new ExprIntegerLiteral(expression.source, ctx.ctx.typeInt, 0);

			ExprBase ref result = CreateConstruct(ctx, expression.type, typeId, ptr, length);

			if(!result)
				return nullptr;

			return CheckType(expression, result);
		}
		break;
	case ExprTypeCastCategory.EXPR_CAST_NULL_TO_FUNCTION:
		{
			ExprBase ref context = new ExprNullptrLiteral(expression.source, ctx.ctx.GetReferenceType(ctx.ctx.typeVoid));

			ExprFunctionLiteral ref result = new ExprFunctionLiteral(expression.source, expression.type, nullptr, context);

			return CheckType(expression, result);
		}
		break;
	case ExprTypeCastCategory.EXPR_CAST_ARRAY_PTR_TO_UNSIZED:
		{
			TypeRef ref refType = getType with<TypeRef>(value.type);

			assert(refType != nullptr);

			TypeArray ref arrType = getType with<TypeArray>(refType.subType);

			assert(arrType != nullptr);
			assert(int(arrType.length) == arrType.length);

			ExprBase ref result = CreateConstruct(ctx, expression.type, value, new ExprIntegerLiteral(expression.source, ctx.ctx.typeInt, arrType.length), nullptr);

			if(!result)
				return nullptr;

			return CheckType(expression, result);
		}
		break;
	case ExprTypeCastCategory.EXPR_CAST_PTR_TO_AUTO_PTR:
		{
			TypeRef ref refType = getType with<TypeRef>(value.type);

			assert(refType != nullptr);

			TypeClass ref classType = getType with<TypeClass>(refType.subType);

			ExprBase ref typeId = nullptr;

			if(classType && (classType.isExtendable || classType.baseClass))
			{
				if(isType with<ExprNullptrLiteral>(value))
					return Report(ctx, "ERROR: null pointer access");

				ExprPointerLiteral ref ptr = getType with<ExprPointerLiteral>(value);

				assert(ptr != nullptr);
				assert(ptr.end - ptr.start >= 4);

				typeId = CreateLoad(ctx, new ExprPointerLiteral(expression.source, ctx.ctx.GetReferenceType(ctx.ctx.typeTypeID), ptr.ptr, ptr.start, ptr.start + 4));
			}
			else
			{
				typeId = new ExprTypeLiteral(expression.source, ctx.ctx.typeTypeID, refType.subType);
			}

			ExprBase ref result = CreateConstruct(ctx, expression.type, typeId, value, nullptr);

			if(!result)
				return nullptr;

			return CheckType(expression, result);
		}
		break;
	case ExprTypeCastCategory.EXPR_CAST_AUTO_PTR_TO_PTR:
		{
			TypeRef ref refType = getType with<TypeRef>(expression.type);

			assert(refType != nullptr);

			ExprMemoryLiteral ref memLiteral = getType with<ExprMemoryLiteral>(value);

			ExprTypeLiteral ref typeId = getType with<ExprTypeLiteral>(CreateExtract(ctx, memLiteral, 0, ctx.ctx.typeTypeID));

			if(typeId.value != refType.subType)
				return Report(ctx, "ERROR: failed to cast '%.*s' to '%.*s'", FMT_ISTR(value.type.name), FMT_ISTR(expression.type.name));

			ExprBase ref ptr = CreateExtract(ctx, memLiteral, 4, refType);

			if(!ptr)
				return nullptr;

			return CheckType(expression, ptr);
		}
		break;
	case ExprTypeCastCategory.EXPR_CAST_UNSIZED_TO_AUTO_ARRAY:
		{
			TypeUnsizedArray ref arrType = getType with<TypeUnsizedArray>(value.type);

			assert(arrType != nullptr);

			ExprMemoryLiteral ref memLiteral = getType with<ExprMemoryLiteral>(value);

			ExprBase ref typeId = new ExprTypeLiteral(expression.source, ctx.ctx.typeTypeID, arrType.subType);
			ExprBase ref ptr = CreateExtract(ctx, memLiteral, 0, ctx.ctx.GetReferenceType(ctx.ctx.typeVoid));
			ExprBase ref length = CreateExtract(ctx, memLiteral, sizeof(void ref), ctx.ctx.typeInt);

			ExprBase ref result = CreateConstruct(ctx, expression.type, typeId, ptr, length);

			if(!result)
				return nullptr;

			return CheckType(expression, result);
		}
		break;
	case ExprTypeCastCategory.EXPR_CAST_REINTERPRET:
		if(expression.type == ctx.ctx.typeInt && value.type == ctx.ctx.typeTypeID)
		{
			ExprTypeLiteral ref typeLiteral = getType with<ExprTypeLiteral>(value);

			if(isType with<TypeError>(typeLiteral.value))
				return nullptr;

			if(isType with<TypeArgumentSet>(typeLiteral.value) || isType with<TypeMemberSet>(typeLiteral.value))
				return nullptr;

			int index = ctx.ctx.GetTypeIndex(typeLiteral.value);

			return CheckType(expression, new ExprIntegerLiteral(expression.source, ctx.ctx.typeInt, index));
		}
		else if(isType with<TypeRef>(expression.type) && isType with<TypeRef>(value.type))
		{
			TypeRef ref refType = getType with<TypeRef>(expression.type);

			if(isType with<ExprNullptrLiteral>(value))
				return CheckType(expression, new ExprNullptrLiteral(expression.source, expression.type));
			
			if(ExprPointerLiteral ref tmp = getType with<ExprPointerLiteral>(value))
			{
				assert(tmp.end - tmp.start >= refType.subType.size);

				return CheckType(expression, new ExprPointerLiteral(expression.source, expression.type, tmp.ptr, tmp.start, tmp.end));
			}
		}
		else if(isType with<TypeUnsizedArray>(expression.type) && isType with<TypeUnsizedArray>(value.type))
		{
			ExprMemoryLiteral ref memLiteral = getType with<ExprMemoryLiteral>(value);

			return CheckType(expression, new ExprMemoryLiteral(expression.source, expression.type, memLiteral.ptr));
		}
		else if(isType with<TypeFunction>(expression.type) && isType with<TypeFunction>(value.type))
		{
			ExprFunctionLiteral ref funcLiteral = getType with<ExprFunctionLiteral>(value);

			return CheckType(expression, new ExprFunctionLiteral(expression.source, expression.type, funcLiteral.data, funcLiteral.context));
		}
		break;
	}

	return Report(ctx, "ERROR: failed to cast '%.*s' to '%.*s'", FMT_ISTR(value.type.name), FMT_ISTR(expression.type.name));
}

ExprBase ref EvaluateUnaryOp(ExpressionEvalContext ref ctx, ExprUnaryOp ref expression)
{
	if(!ctx.stackFrames.empty() && ctx.stackFrames.back().targetYield)
		return new ExprVoid(expression.source, ctx.ctx.typeVoid);

	if(!AddInstruction(ctx))
		return nullptr;

	ExprBase ref value = Evaluate(ctx, expression.value);

	if(!value)
		return nullptr;

	if(value.type == ctx.ctx.typeBool)
	{
		if(ExprBoolLiteral ref expr = getType with<ExprBoolLiteral>(value))
		{
			if(expression.op == SynUnaryOpType.SYN_UNARY_OP_LOGICAL_NOT)
				return CheckType(expression, new ExprBoolLiteral(expression.source, expression.type, !expr.value));
		}
		else if(ExprIntegerLiteral ref expr = getType with<ExprIntegerLiteral>(value))
		{
			if(expression.op == SynUnaryOpType.SYN_UNARY_OP_LOGICAL_NOT)
				return CheckType(expression, new ExprBoolLiteral(expression.source, expression.type, !expr.value));
		}
	}
	else if(ctx.ctx.IsIntegerType(value.type))
	{
		long result = 0;

		if(TryTakeLong(value, result))
		{
			switch(expression.op)
			{
			case SynUnaryOpType.SYN_UNARY_OP_PLUS:
				return CheckType(expression, new ExprIntegerLiteral(expression.source, expression.type, result));
			case SynUnaryOpType.SYN_UNARY_OP_NEGATE:
				return CheckType(expression, new ExprIntegerLiteral(expression.source, expression.type, -result));
			case SynUnaryOpType.SYN_UNARY_OP_BIT_NOT:
				return CheckType(expression, new ExprIntegerLiteral(expression.source, expression.type, ~result));
			case SynUnaryOpType.SYN_UNARY_OP_LOGICAL_NOT:
				return CheckType(expression, new ExprIntegerLiteral(expression.source, expression.type, !result));
			default:
				assert(false, "unknown unary operation");
			}
		}
	}
	else if(ctx.ctx.IsFloatingPointType(value.type))
	{
		double result = 0.0;

		if(TryTakeDouble(value, result))
		{
			switch(expression.op)
			{
			case SynUnaryOpType.SYN_UNARY_OP_PLUS:
				return CheckType(expression, new ExprRationalLiteral(expression.source, expression.type, result));
			case SynUnaryOpType.SYN_UNARY_OP_NEGATE:
				return CheckType(expression, new ExprRationalLiteral(expression.source, expression.type, -result));
			case SynUnaryOpType.SYN_UNARY_OP_BIT_NOT:
			case SynUnaryOpType.SYN_UNARY_OP_LOGICAL_NOT:
				return nullptr;
			default:
				assert(false, "unknown unary operation");
			}
		}
	}
	else if(isType with<TypeRef>(value.type))
	{
		int lPtr = 0;

		if(isType with<ExprNullptrLiteral>(value))
			lPtr = 0;
		else if(ExprPointerLiteral ref tmp = getType with<ExprPointerLiteral>(value))
			lPtr = GetMemoryAddress(ctx, tmp.ptr, tmp.start);
		else
			assert(false, "unknown type");

		if(expression.op == SynUnaryOpType.SYN_UNARY_OP_LOGICAL_NOT)
			return CheckType(expression, new ExprBoolLiteral(expression.source, ctx.ctx.typeBool, !lPtr));
	}
	else if(value.type == ctx.ctx.typeAutoRef)
	{
		ExprMemoryLiteral ref memLiteral = getType with<ExprMemoryLiteral>(value);

		int lPtr = 0;
		if(!TryTakePointer(ctx, CreateExtract(ctx, memLiteral, 4, ctx.ctx.GetReferenceType(ctx.ctx.typeVoid)), lPtr))
			return Report(ctx, "ERROR: failed to evaluate auto ref value");

		return CheckType(expression, new ExprBoolLiteral(expression.source, ctx.ctx.typeBool, !lPtr));

	}

	return Report(ctx, "ERROR: failed to eval unary op");
}

ExprBase ref EvaluateBinaryOp(ExpressionEvalContext ref ctx, ExprBinaryOp ref expression)
{
	if(!ctx.stackFrames.empty() && ctx.stackFrames.back().targetYield)
		return new ExprVoid(expression.source, ctx.ctx.typeVoid);

	if(!AddInstruction(ctx))
		return nullptr;

	ExprBase ref lhs = Evaluate(ctx, expression.lhs);

	if(!lhs)
		return nullptr;

	// rhs remain unevaluated
	ExprBase ref result = CreateBinaryOp(ctx, expression.source, lhs, expression.rhs, expression.op);

	if(!result)
		return result;

	return CheckType(expression, result);
}

ExprBase ref EvaluateGetAddress(ExpressionEvalContext ref ctx, ExprGetAddress ref expression)
{
	if(!ctx.stackFrames.empty() && ctx.stackFrames.back().targetYield)
		return new ExprVoid(expression.source, ctx.ctx.typeVoid);

	if(!AddInstruction(ctx))
		return nullptr;

	ExprPointerLiteral ref ptr = FindVariableStorage(ctx, expression.variable.variable);

	if(!ptr)
		return nullptr;

	return CheckType(expression, ptr);
}

ExprBase ref EvaluateDereference(ExpressionEvalContext ref ctx, ExprDereference ref expression)
{
	if(!ctx.stackFrames.empty() && ctx.stackFrames.back().targetYield)
		return new ExprVoid(expression.source, ctx.ctx.typeVoid);

	if(!AddInstruction(ctx))
		return nullptr;

	ExprBase ref ptr = Evaluate(ctx, expression.value);

	if(!ptr)
		return nullptr;

	ExprBase ref value = CreateLoad(ctx, ptr);

	if(!value)
		return nullptr;

	return CheckType(expression, value);
}

ExprBase ref EvaluateConditional(ExpressionEvalContext ref ctx, ExprConditional ref expression)
{
	if(!ctx.stackFrames.empty() && ctx.stackFrames.back().targetYield)
		return new ExprVoid(expression.source, ctx.ctx.typeVoid);

	if(!AddInstruction(ctx))
		return nullptr;

	ExprBase ref condition = Evaluate(ctx, expression.condition);

	if(!condition)
		return nullptr;

	long result;
	if(!TryTakeLong(condition, result))
		return Report(ctx, "ERROR: failed to evaluate ternary operator condition");

	ExprBase ref value = Evaluate(ctx, result ? expression.trueBlock : expression.falseBlock);

	if(!value)
		return nullptr;

	return CheckType(expression, value);
}

ExprBase ref EvaluateAssignment(ExpressionEvalContext ref ctx, ExprAssignment ref expression)
{
	if(!ctx.stackFrames.empty() && ctx.stackFrames.back().targetYield)
		return new ExprVoid(expression.source, ctx.ctx.typeVoid);

	if(!AddInstruction(ctx))
		return nullptr;

	ExprBase ref lhs = Evaluate(ctx, expression.lhs);
	ExprBase ref rhs = Evaluate(ctx, expression.rhs);

	if(!lhs || !rhs)
		return nullptr;

	if(!CreateStore(ctx, lhs, rhs))
		return nullptr;

	return CheckType(expression, rhs);
}

ExprBase ref EvaluateMemberAccess(ExpressionEvalContext ref ctx, ExprMemberAccess ref expression)
{
	if(!ctx.stackFrames.empty() && ctx.stackFrames.back().targetYield)
		return new ExprVoid(expression.source, ctx.ctx.typeVoid);

	if(!AddInstruction(ctx))
		return nullptr;

	ExprBase ref value = Evaluate(ctx, expression.value);

	if(!value)
		return nullptr;

	if(isType with<ExprNullptrLiteral>(value))
		return Report(ctx, "ERROR: member access of null pointer");

	ExprPointerLiteral ref ptr = getType with<ExprPointerLiteral>(value);

	assert(ptr != nullptr);
	assert(ptr.start + expression.member.variable.offset + expression.member.variable.type.size <= ptr.end);

	ExprPointerLiteral ref shifted = new ExprPointerLiteral(expression.source, ctx.ctx.GetReferenceType(expression.member.variable.type), ptr.ptr, ptr.start + expression.member.variable.offset, ptr.start + expression.member.variable.offset + expression.member.variable.type.size);

	return CheckType(expression, shifted);
}

ExprBase ref EvaluateArrayIndex(ExpressionEvalContext ref ctx, ExprArrayIndex ref expression)
{
	if(!ctx.stackFrames.empty() && ctx.stackFrames.back().targetYield)
		return new ExprVoid(expression.source, ctx.ctx.typeVoid);

	if(!AddInstruction(ctx))
		return nullptr;

	ExprBase ref value = Evaluate(ctx, expression.value);

	if(!value)
		return nullptr;

	ExprBase ref index = Evaluate(ctx, expression.index);

	if(!index)
		return nullptr;

	long result;
	if(!TryTakeLong(index, result))
		return Report(ctx, "ERROR: failed to evaluate array index");

	if(TypeUnsizedArray ref arrayType = getType with<TypeUnsizedArray>(value.type))
	{
		ExprMemoryLiteral ref mem = getType with<ExprMemoryLiteral>(value);

		assert(mem != nullptr);

		ExprBase ref value = CreateExtract(ctx, mem, 0, ctx.ctx.GetReferenceType(arrayType.subType));

		if(!value)
			return nullptr;

		if(isType with<ExprNullptrLiteral>(value))
			return Report(ctx, "ERROR: array index of a null array");

		ExprIntegerLiteral ref size = getType with<ExprIntegerLiteral>(CreateExtract(ctx, mem, sizeof(void ref), ctx.ctx.typeInt));

		if(!size)
			return nullptr;

		if(result < 0 || result >= size.value)
			return ReportCritical(ctx, "ERROR: array index out of bounds");

		ExprPointerLiteral ref ptr = getType with<ExprPointerLiteral>(value);

		assert(ptr != nullptr);

		int targetOffset = ptr.start + result * arrayType.subType.size;

		ExprPointerLiteral ref shifted = new ExprPointerLiteral(expression.source, ctx.ctx.GetReferenceType(arrayType.subType), ptr.ptr, targetOffset, targetOffset + arrayType.subType.size);

		return CheckType(expression, shifted);
	}

	TypeRef ref refType = getType with<TypeRef>(value.type);

	assert(refType != nullptr);

	TypeArray ref arrayType = getType with<TypeArray>(refType.subType);

	assert(arrayType != nullptr);

	if(isType with<ExprNullptrLiteral>(value))
		return Report(ctx, "ERROR: array index of a null array");

	if(result < 0 || result >= arrayType.length)
		return ReportCritical(ctx, "ERROR: array index out of bounds");

	ExprPointerLiteral ref ptr = getType with<ExprPointerLiteral>(value);

	assert(ptr != nullptr);
	assert(ptr.start + result * arrayType.subType.size + arrayType.subType.size <= ptr.end);

	int targetOffset = ptr.start + result * arrayType.subType.size;

	ExprPointerLiteral ref shifted = new ExprPointerLiteral(expression.source, ctx.ctx.GetReferenceType(arrayType.subType), ptr.ptr, targetOffset, targetOffset + arrayType.subType.size);

	return CheckType(expression, shifted);
}

ExprBase ref EvaluateReturn(ExpressionEvalContext ref ctx, ExprReturn ref expression)
{
	if(!AddInstruction(ctx))
		return nullptr;

	StackFrame ref frame = *ctx.stackFrames.back();

	if(frame.targetYield)
		return CheckType(expression, new ExprVoid(expression.source, ctx.ctx.typeVoid));

	ExprBase ref value = Evaluate(ctx, expression.value);

	if(!value)
		return nullptr;

	if(ctx.stackFrames.empty())
		return Report(ctx, "ERROR: no stack frame to return from");

	frame.returnValue = value;

	if(expression.coroutineStateUpdate)
	{
		if(!Evaluate(ctx, expression.coroutineStateUpdate))
			return nullptr;
	}

	if(expression.closures)
	{
		if(!Evaluate(ctx, expression.closures))
			return nullptr;
	}

	return CheckType(expression, new ExprVoid(expression.source, ctx.ctx.typeVoid));
}

ExprBase ref EvaluateYield(ExpressionEvalContext ref ctx, ExprYield ref expression)
{
	if(!AddInstruction(ctx))
		return nullptr;

	StackFrame ref frame = *ctx.stackFrames.back();

	// Check if we reached target yield
	if(frame.targetYield == expression.order)
	{
		frame.targetYield = 0;

		return CheckType(expression, new ExprVoid(expression.source, ctx.ctx.typeVoid));
	}

	if(frame.targetYield)
		return CheckType(expression, new ExprVoid(expression.source, ctx.ctx.typeVoid));

	ExprBase ref value = Evaluate(ctx, expression.value);

	if(!value)
		return nullptr;

	if(ctx.stackFrames.empty())
		return Report(ctx, "ERROR: no stack frame to return from");

	frame.returnValue = value;

	if(expression.coroutineStateUpdate)
	{
		if(!Evaluate(ctx, expression.coroutineStateUpdate))
			return nullptr;
	}

	if(expression.closures)
	{
		if(!Evaluate(ctx, expression.closures))
			return nullptr;
	}

	return CheckType(expression, new ExprVoid(expression.source, ctx.ctx.typeVoid));
}

ExprBase ref EvaluateVariableDefinition(ExpressionEvalContext ref ctx, ExprVariableDefinition ref expression)
{
	if(!AddInstruction(ctx))
		return nullptr;

	StackFrame ref frame = *ctx.stackFrames.back();

	if(FindVariableStorage(ctx, expression.variable.variable) == nullptr)
	{
		TypeBase ref type = expression.variable.variable.type;

		ExprPointerLiteral ref storage = AllocateTypeStorage(ctx, expression.source, type);

		if(!storage)
			return nullptr;

		frame.variables.push_back(StackVariable(expression.variable.variable, storage));
	}

	if(!frame.targetYield)
	{
		if(expression.initializer)
		{
			if(!Evaluate(ctx, expression.initializer))
				return nullptr;
		}
	}

	return CheckType(expression, new ExprVoid(expression.source, ctx.ctx.typeVoid));
}

ExprBase ref EvaluateArraySetup(ExpressionEvalContext ref ctx, ExprArraySetup ref expression)
{
	if(!AddInstruction(ctx))
		return nullptr;

	TypeRef ref refType = getType with<TypeRef>(expression.lhs.type);

	assert(refType != nullptr);

	TypeArray ref arrayType = getType with<TypeArray>(refType.subType);

	assert(arrayType != nullptr);

	ExprBase ref initializer = Evaluate(ctx, expression.initializer);

	if(!initializer)
		return nullptr;

	ExprPointerLiteral ref ptr = getType with<ExprPointerLiteral>(Evaluate(ctx, expression.lhs));

	if(!ptr)
		return nullptr;

	for(int i = 0; i < int(arrayType.length); i++)
	{
		if(!AddInstruction(ctx))
			return nullptr;

		assert(ptr != nullptr);
		assert(ptr.start + i * arrayType.subType.size + arrayType.subType.size <= ptr.end);

		int targetOffset = ptr.start + i * arrayType.subType.size;

		ExprPointerLiteral ref shifted = new ExprPointerLiteral(expression.source, ctx.ctx.GetReferenceType(arrayType.subType), ptr.ptr, targetOffset, targetOffset + arrayType.subType.size);

		if(!CreateStore(ctx, shifted, initializer))
			return nullptr;
	}

	return CheckType(expression, new ExprVoid(expression.source, ctx.ctx.typeVoid));
}

ExprBase ref EvaluateVariableDefinitions(ExpressionEvalContext ref ctx, ExprVariableDefinitions ref expression)
{
	if(!AddInstruction(ctx))
		return nullptr;

	for(ExprBase ref definition = expression.definitions.head; definition; definition = definition.next)
	{
		if(!Evaluate(ctx, definition))
			return nullptr;
	}

	return CheckType(expression, new ExprVoid(expression.source, ctx.ctx.typeVoid));
}

ExprBase ref EvaluateVariableAccess(ExpressionEvalContext ref ctx, ExprVariableAccess ref expression)
{
	if(!ctx.stackFrames.empty() && ctx.stackFrames.back().targetYield)
		return new ExprVoid(expression.source, ctx.ctx.typeVoid);

	if(!AddInstruction(ctx))
		return nullptr;

	ExprPointerLiteral ref ptr = FindVariableStorage(ctx, expression.variable);

	if(!ptr)
		return nullptr;

	ExprBase ref value = CreateLoad(ctx, ptr);

	if(!value)
		return nullptr;

	return CheckType(expression, value);
}

ExprBase ref EvaluateFunctionContextAccess(ExpressionEvalContext ref ctx, ExprFunctionContextAccess ref expression)
{
	if(!ctx.stackFrames.empty() && ctx.stackFrames.back().targetYield)
		return new ExprVoid(expression.source, ctx.ctx.typeVoid);

	if(!AddInstruction(ctx))
		return nullptr;

	ExprPointerLiteral ref ptr = FindVariableStorage(ctx, expression.contextVariable);

	if(!ptr)
		return nullptr;

	ExprBase ref value = nullptr;

	TypeRef ref refType = getType with<TypeRef>(expression.function.contextType);

	assert(refType != nullptr);

	TypeClass ref classType = getType with<TypeClass>(refType.subType);

	assert(classType != nullptr);

	if(classType.members.empty())
		value = new ExprNullptrLiteral(expression.source, expression.function.contextType);
	else
		value = CreateLoad(ctx, ptr);

	if(!value)
		return nullptr;

	return CheckType(expression, value);
}

ExprBase ref EvaluateFunctionDefinition(ExpressionEvalContext ref ctx, ExprFunctionDefinition ref expression)
{
	if(!ctx.stackFrames.empty() && ctx.stackFrames.back().targetYield)
		return new ExprVoid(expression.source, ctx.ctx.typeVoid);

	if(!AddInstruction(ctx))
		return nullptr;

	ExprBase ref context = new ExprNullptrLiteral(expression.source, ctx.ctx.GetReferenceType(ctx.ctx.typeVoid));

	return CheckType(expression, new ExprFunctionLiteral(expression.source, expression.function.type, expression.function, context));
}

ExprBase ref EvaluateGenericFunctionPrototype(ExpressionEvalContext ref ctx, ExprGenericFunctionPrototype ref expression)
{
	if(!ctx.stackFrames.empty() && ctx.stackFrames.back().targetYield)
		return new ExprVoid(expression.source, ctx.ctx.typeVoid);

	if(!AddInstruction(ctx))
		return nullptr;

	for(ExprBase ref expr = expression.contextVariables.head; expr; expr = expr.next)
	{
		if(!Evaluate(ctx, expr))
			return nullptr;
	}

	return new ExprVoid(expression.source, ctx.ctx.typeVoid);
}

ExprBase ref EvaluateFunction(ExpressionEvalContext ref ctx, ExprFunctionDefinition ref expression, ExprBase ref context, ArrayView<ExprBase ref> arguments)
{
	if(!AddInstruction(ctx))
		return nullptr;

	if(ctx.stackFrames.size() >= ctx.stackDepthLimit)
		return Report(ctx, "ERROR: stack depth limit");

	ctx.stackFrames.push_back(new StackFrame(expression.function));

	StackFrame ref frame = *ctx.stackFrames.back();

	if(ExprVariableDefinition ref curr = expression.contextArgument)
	{
		ExprPointerLiteral ref storage = AllocateTypeStorage(ctx, curr.source, curr.variable.variable.type);

		if(!storage)
			return nullptr;

		frame.variables.push_back(StackVariable(curr.variable.variable, storage));

		if(!CreateStore(ctx, storage, context))
			return nullptr;
	}

	int pos = 0;

	for(ExprVariableDefinition ref curr = expression.arguments.head; curr; curr = getType with<ExprVariableDefinition>(curr.next))
	{
		ExprPointerLiteral ref storage = AllocateTypeStorage(ctx, curr.source, curr.variable.variable.type);

		if(!storage)
			return nullptr;

		frame.variables.push_back(StackVariable(curr.variable.variable, storage));

		if(!CreateStore(ctx, storage, arguments[pos]))
			return nullptr;

		pos++;
	}

	if(expression.coroutineStateRead)
	{
		if(ExprIntegerLiteral ref jmpOffset = getType with<ExprIntegerLiteral>(Evaluate(ctx, expression.coroutineStateRead)))
		{
			frame.targetYield = int(jmpOffset.value);
		}
		else
		{
			return nullptr;
		}
	}

	for(ExprBase ref value = expression.expressions.head; value; value = value.next)
	{
		if(!Evaluate(ctx, value))
			return nullptr;

		assert(frame.breakDepth == 0 && frame.continueDepth == 0);

		if(frame.returnValue)
			break;
	}

	ExprBase ref result = frame.returnValue;

	if(!result)
		return ReportCritical(ctx, "ERROR: function didn't return a value");

	ctx.stackFrames.pop_back();

	return result;
}

ExprBase ref EvaluateFunctionAccess(ExpressionEvalContext ref ctx, ExprFunctionAccess ref expression)
{
	if(!ctx.stackFrames.empty() && ctx.stackFrames.back().targetYield)
		return new ExprVoid(expression.source, ctx.ctx.typeVoid);

	if(!AddInstruction(ctx))
		return nullptr;

	ExprBase ref context = Evaluate(ctx, expression.context);

	if(!context)
		return nullptr;

	FunctionData ref function = expression.function;

	if(function.implementation)
		function = function.implementation;

	return CheckType(expression, new ExprFunctionLiteral(expression.source, function.type, function, context));
}

ExprBase ref EvaluateKnownExternalFunctionCall(ExpressionEvalContext ref ctx, ExprFunctionCall ref expression, ExprFunctionLiteral ref ptr, ArrayView<ExprBase ref> arguments)
{
	if(ptr.data.name.name == InplaceStr("assert") && arguments.size() == 1 && arguments[0].type == ctx.ctx.typeInt)
	{
		long value;
		if(!TryTakeLong(arguments[0], value))
			return Report(ctx, "ERROR: failed to evaluate value");

		if(value == 0)
			return Report(ctx, "ERROR: Assertion failed");

		return CheckType(expression, new ExprVoid(expression.source, ctx.ctx.typeVoid));
	}
	else if(ptr.data.name.name == InplaceStr("assert") && arguments.size() == 2 && arguments[0].type == ctx.ctx.typeInt && arguments[1].type == ctx.ctx.GetUnsizedArrayType(ctx.ctx.typeChar))
	{
		long value;
		if(!TryTakeLong(arguments[0], value))
			return Report(ctx, "ERROR: failed to evaluate value");

		ExprMemoryLiteral ref mem = getType with<ExprMemoryLiteral>(arguments[1]);

		ExprPointerLiteral ref str = getType with<ExprPointerLiteral>(CreateExtract(ctx, mem, 0, ctx.ctx.GetReferenceType(ctx.ctx.typeChar)));
		ExprIntegerLiteral ref length = getType with<ExprIntegerLiteral>(CreateExtract(ctx, mem, sizeof(void ref), ctx.ctx.typeInt));

		if(!str)
			return Report(ctx, "ERROR: null pointer access");

		assert(length != nullptr);

		if(value == 0)
			return Report(ctx, "ERROR: %.*s", int(length.value), str.ptr);

		return CheckType(expression, new ExprVoid(expression.source, ctx.ctx.typeVoid));
	}
	else if(ptr.data.name.name == InplaceStr("bool") && arguments.size() == 1 && arguments[0].type == ctx.ctx.typeBool)
	{
		long value;
		if(!TryTakeLong(arguments[0], value))
			return Report(ctx, "ERROR: failed to evaluate value");

		return CheckType(expression, new ExprBoolLiteral(expression.source, ctx.ctx.typeBool, value != 0));
	}
	else if(ptr.data.name.name == InplaceStr("char") && arguments.size() == 1 && arguments[0].type == ctx.ctx.typeChar)
	{
		long value;
		if(!TryTakeLong(arguments[0], value))
			return Report(ctx, "ERROR: failed to evaluate value");

		return CheckType(expression, new ExprIntegerLiteral(expression.source, ctx.ctx.typeChar, char(value)));
	}
	else if(ptr.data.name.name == InplaceStr("short") && arguments.size() == 1 && arguments[0].type == ctx.ctx.typeShort)
	{
		long value;
		if(!TryTakeLong(arguments[0], value))
			return Report(ctx, "ERROR: failed to evaluate value");

		return CheckType(expression, new ExprIntegerLiteral(expression.source, ctx.ctx.typeShort, short(value)));
	}
	else if(ptr.data.name.name == InplaceStr("int") && arguments.size() == 1 && arguments[0].type == ctx.ctx.typeInt)
	{
		long value;
		if(!TryTakeLong(arguments[0], value))
			return Report(ctx, "ERROR: failed to evaluate value");

		return CheckType(expression, new ExprIntegerLiteral(expression.source, ctx.ctx.typeInt, int(value)));
	}
	else if(ptr.data.name.name == InplaceStr("long") && arguments.size() == 1 && arguments[0].type == ctx.ctx.typeLong)
	{
		long value;
		if(!TryTakeLong(arguments[0], value))
			return Report(ctx, "ERROR: failed to evaluate value");

		return CheckType(expression, new ExprIntegerLiteral(expression.source, ctx.ctx.typeLong, value));
	}
	else if(ptr.data.name.name == InplaceStr("float") && arguments.size() == 1 && arguments[0].type == ctx.ctx.typeFloat)
	{
		double value;
		if(!TryTakeDouble(arguments[0], value))
			return Report(ctx, "ERROR: failed to evaluate value");

		return CheckType(expression, new ExprRationalLiteral(expression.source, ctx.ctx.typeFloat, value));
	}
	else if(ptr.data.name.name == InplaceStr("double") && arguments.size() == 1 && arguments[0].type == ctx.ctx.typeDouble)
	{
		double value;
		if(!TryTakeDouble(arguments[0], value))
			return Report(ctx, "ERROR: failed to evaluate value");

		return CheckType(expression, new ExprRationalLiteral(expression.source, ctx.ctx.typeDouble, value));
	}
	else if(ptr.data.name.name == InplaceStr("bool::bool") && ptr.context && arguments.size() == 1 && arguments[0].type == ctx.ctx.typeBool)
	{
		if(!CreateStore(ctx, ptr.context, arguments[0]))
			return nullptr;

		return CheckType(expression, new ExprVoid(expression.source, ctx.ctx.typeVoid));
	}
	else if(ptr.data.name.name == InplaceStr("char::char") && ptr.context && arguments.size() == 1 && arguments[0].type == ctx.ctx.typeChar)
	{
		if(!CreateStore(ctx, ptr.context, arguments[0]))
			return nullptr;

		return CheckType(expression, new ExprVoid(expression.source, ctx.ctx.typeVoid));
	}
	else if(ptr.data.name.name == InplaceStr("short::short") && ptr.context && arguments.size() == 1 && arguments[0].type == ctx.ctx.typeShort)
	{
		if(!CreateStore(ctx, ptr.context, arguments[0]))
			return nullptr;

		return CheckType(expression, new ExprVoid(expression.source, ctx.ctx.typeVoid));
	}
	else if(ptr.data.name.name == InplaceStr("int::int") && ptr.context && arguments.size() == 1 && arguments[0].type == ctx.ctx.typeInt)
	{
		if(!CreateStore(ctx, ptr.context, arguments[0]))
			return nullptr;

		return CheckType(expression, new ExprVoid(expression.source, ctx.ctx.typeVoid));
	}
	else if(ptr.data.name.name == InplaceStr("long::long") && ptr.context && arguments.size() == 1 && arguments[0].type == ctx.ctx.typeLong)
	{
		if(!CreateStore(ctx, ptr.context, arguments[0]))
			return nullptr;

		return CheckType(expression, new ExprVoid(expression.source, ctx.ctx.typeVoid));
	}
	else if(ptr.data.name.name == InplaceStr("float::float") && ptr.context && arguments.size() == 1 && arguments[0].type == ctx.ctx.typeFloat)
	{
		if(!CreateStore(ctx, ptr.context, arguments[0]))
			return nullptr;

		return CheckType(expression, new ExprVoid(expression.source, ctx.ctx.typeVoid));
	}
	else if(ptr.data.name.name == InplaceStr("double::double") && ptr.context && arguments.size() == 1 && arguments[0].type == ctx.ctx.typeDouble)
	{
		if(!CreateStore(ctx, ptr.context, arguments[0]))
			return nullptr;

		return CheckType(expression, new ExprVoid(expression.source, ctx.ctx.typeVoid));
	}
	else if(ptr.data.name.name == InplaceStr("__newS"))
	{
		long size;
		if(!TryTakeLong(arguments[0], size))
			return Report(ctx, "ERROR: failed to evaluate type size");

		long type;
		if(!TryTakeLong(arguments[1], type))
			return Report(ctx, "ERROR: failed to evaluate type ID");

		TypeBase ref target = ctx.ctx.types[int(type)];

		assert(target.size == size);

		ExprPointerLiteral ref storage = AllocateTypeStorage(ctx, expression.source, target);

		if(!storage)
			return nullptr;

		return CheckType(expression, new ExprPointerLiteral(expression.source, ctx.ctx.GetReferenceType(ctx.ctx.typeVoid), storage.ptr, storage.start, storage.end));
	}
	else if(ptr.data.name.name == InplaceStr("__newA"))
	{
		long size;
		if(!TryTakeLong(arguments[0], size))
			return Report(ctx, "ERROR: failed to evaluate type size");

		long count;
		if(!TryTakeLong(arguments[1], count))
			return Report(ctx, "ERROR: failed to evaluate element count");

		long type;
		if(!TryTakeLong(arguments[2], type))
			return Report(ctx, "ERROR: failed to evaluate type ID");

		TypeBase ref target = ctx.ctx.types[int(type)];

		assert(target.size == size);

		if(target.size * int(count) > ctx.variableMemoryLimit)
			return Report(ctx, "ERROR: single variable memory limit");

		ExprPointerLiteral ref storage = AllocateTypeStorage(ctx, expression.source, ctx.ctx.GetArrayType(target, count));

		if(!storage)
			return nullptr;

		ExprBase ref result = CreateConstruct(ctx, ctx.ctx.GetUnsizedArrayType(ctx.ctx.typeInt), storage, new ExprIntegerLiteral(expression.source, ctx.ctx.typeInt, count), nullptr);

		if(!result)
			return nullptr;

		return CheckType(expression, result);
	}
	else if(ptr.data.name.name == InplaceStr("__rcomp"))
	{
		ExprMemoryLiteral ref a = getType with<ExprMemoryLiteral>(arguments[0]);
		ExprMemoryLiteral ref b = getType with<ExprMemoryLiteral>(arguments[1]);

		assert(a && b);

		int lPtr = 0;
		if(!TryTakePointer(ctx, CreateExtract(ctx, a, 4, ctx.ctx.GetReferenceType(ctx.ctx.typeVoid)), lPtr))
			return Report(ctx, "ERROR: failed to evaluate first argument");

		int rPtr = 0;
		if(!TryTakePointer(ctx, CreateExtract(ctx, b, 4, ctx.ctx.GetReferenceType(ctx.ctx.typeVoid)), rPtr))
			return Report(ctx, "ERROR: failed to evaluate second argument");

		return CheckType(expression, new ExprIntegerLiteral(expression.source, ctx.ctx.typeInt, lPtr == rPtr));
	}
	else if(ptr.data.name.name == InplaceStr("__rncomp"))
	{
		ExprMemoryLiteral ref a = getType with<ExprMemoryLiteral>(arguments[0]);
		ExprMemoryLiteral ref b = getType with<ExprMemoryLiteral>(arguments[1]);

		assert(a && b);

		int lPtr = 0;
		if(!TryTakePointer(ctx, CreateExtract(ctx, a, 4, ctx.ctx.GetReferenceType(ctx.ctx.typeVoid)), lPtr))
			return Report(ctx, "ERROR: failed to evaluate first argument");

		int rPtr = 0;
		if(!TryTakePointer(ctx, CreateExtract(ctx, b, 4, ctx.ctx.GetReferenceType(ctx.ctx.typeVoid)), rPtr))
			return Report(ctx, "ERROR: failed to evaluate second argument");

		return CheckType(expression, new ExprIntegerLiteral(expression.source, ctx.ctx.typeInt, lPtr != rPtr));
	}
	else if(ptr.data.name.name == InplaceStr("__pcomp"))
	{
		ExprFunctionLiteral ref a = getType with<ExprFunctionLiteral>(arguments[0]);
		ExprFunctionLiteral ref b = getType with<ExprFunctionLiteral>(arguments[1]);

		assert(a && b);

		int aContext = 0;
		if(a.context && !TryTakePointer(ctx, a.context, aContext))
			return Report(ctx, "ERROR: failed to evaluate first argument");

		int bContext = 0;
		if(b.context && !TryTakePointer(ctx, b.context, bContext))
			return Report(ctx, "ERROR: failed to evaluate second argument");

		return CheckType(expression, new ExprIntegerLiteral(expression.source, ctx.ctx.typeInt, a.data == b.data && aContext == bContext));
	}
	else if(ptr.data.name.name == InplaceStr("__pncomp"))
	{
		ExprFunctionLiteral ref a = getType with<ExprFunctionLiteral>(arguments[0]);
		ExprFunctionLiteral ref b = getType with<ExprFunctionLiteral>(arguments[1]);

		assert(a && b);

		int aContext = 0;
		if(a.context && !TryTakePointer(ctx, a.context, aContext))
			return Report(ctx, "ERROR: failed to evaluate first argument");

		int bContext = 0;
		if(b.context && !TryTakePointer(ctx, b.context, bContext))
			return Report(ctx, "ERROR: failed to evaluate second argument");

		return CheckType(expression, new ExprIntegerLiteral(expression.source, ctx.ctx.typeInt, a.data != b.data || aContext != bContext));
	}
	else if(ptr.data.name.name == InplaceStr("__acomp"))
	{
		ExprMemoryLiteral ref a = getType with<ExprMemoryLiteral>(arguments[0]);
		ExprMemoryLiteral ref b = getType with<ExprMemoryLiteral>(arguments[1]);

		assert(a && b);
		assert(a.type.size == b.type.size);

		return CheckType(expression, new ExprIntegerLiteral(expression.source, ctx.ctx.typeInt, memcmp(a.ptr.ptr.buffer, b.ptr.ptr.buffer, int(a.type.size)) == 0));
	}
	else if(ptr.data.name.name == InplaceStr("__ancomp"))
	{
		ExprMemoryLiteral ref a = getType with<ExprMemoryLiteral>(arguments[0]);
		ExprMemoryLiteral ref b = getType with<ExprMemoryLiteral>(arguments[1]);

		assert(a && b);
		assert(a.type.size == b.type.size);

		return CheckType(expression, new ExprIntegerLiteral(expression.source, ctx.ctx.typeInt, memcmp(a.ptr.ptr.buffer, b.ptr.ptr.buffer, int(a.type.size)) != 0));
	}
	else if(ptr.data.name.name == InplaceStr("__typeCount"))
	{
		return CheckType(expression, new ExprIntegerLiteral(expression.source, ctx.ctx.typeInt, ctx.ctx.types.size()));
	}
	else if(ptr.data.name.name == InplaceStr("__redirect") || ptr.data.name.name == InplaceStr("__redirect_ptr"))
	{
		ExprMemoryLiteral ref autoRef = getType with<ExprMemoryLiteral>(arguments[0]);
		ExprPointerLiteral ref tableRef = getType with<ExprPointerLiteral>(arguments[1]);

		if(!tableRef)
			return Report(ctx, "ERROR: null pointer access");

		ExprTypeLiteral ref typeID = getType with<ExprTypeLiteral>(CreateExtract(ctx, autoRef, 0, ctx.ctx.typeTypeID));

		assert(typeID != nullptr);

		int typeIndex = ctx.ctx.GetTypeIndex(typeID.value);

		ExprBase ref context = CreateExtract(ctx, autoRef, 4, ctx.ctx.GetReferenceType(ctx.ctx.types[typeIndex]));

		assert(context != nullptr);

		ExprBase ref tableRefLoad = CreateLoad(ctx, tableRef);

		if(!tableRefLoad)
			return nullptr;

		ExprMemoryLiteral ref table = getType with<ExprMemoryLiteral>(tableRefLoad);

		ExprPointerLiteral ref tableArray = getType with<ExprPointerLiteral>(CreateExtract(ctx, table, 0, ctx.ctx.GetReferenceType(ctx.ctx.typeFunctionID)));
		ExprIntegerLiteral ref tableSize = getType with<ExprIntegerLiteral>(CreateExtract(ctx, table, sizeof(void ref), ctx.ctx.typeInt));

		assert(tableArray && tableSize);

		if(typeIndex >= tableSize.value)
			return Report(ctx, "ERROR: type index is out of bounds of redirection table");

		int index = 0;
		memcpy(&index, tableArray.ptr.buffer, typeIndex * ctx.ctx.typeTypeID.size, sizeof(int));

		FunctionData ref data = index != 0 ? ctx.ctx.functions[index - 1] : nullptr;

		if(!data)
		{
			if(ptr.data.name.name == InplaceStr("__redirect_ptr"))
				return CheckType(expression, new ExprFunctionLiteral(expression.source, expression.type, nullptr, new ExprNullptrLiteral(expression.source, ctx.ctx.GetReferenceType(ctx.ctx.types[typeIndex]))));

			return Report(ctx, "ERROR: type '%.*s' doesn't implement method", FMT_ISTR(ctx.ctx.types[typeIndex].name));
		}

		return CheckType(expression, new ExprFunctionLiteral(expression.source, expression.type, data, context));
	}
	else if(ptr.data.name.name == InplaceStr("duplicate") && arguments.size() == 1 && arguments[0].type == ctx.ctx.typeAutoRef)
	{
		ExprMemoryLiteral ref object = getType with<ExprMemoryLiteral>(arguments[0]);

		assert(object != nullptr);

		ExprTypeLiteral ref ptrTypeID = getType with<ExprTypeLiteral>(CreateExtract(ctx, object, 0, ctx.ctx.typeTypeID));
		ExprPointerLiteral ref ptrPtr = getType with<ExprPointerLiteral>(CreateExtract(ctx, object, 4, ctx.ctx.GetReferenceType(ptrTypeID.value)));

		ExprPointerLiteral ref storage = AllocateTypeStorage(ctx, expression.source, ctx.ctx.typeAutoRef);

		if(!storage)
			return nullptr;

		ExprMemoryLiteral ref result = new ExprMemoryLiteral(expression.source, ctx.ctx.typeAutoRef, storage);

		CreateInsert(ctx, result, 0, new ExprTypeLiteral(expression.source, ctx.ctx.typeTypeID, ptrTypeID.value));

		if(!ptrPtr)
		{
			CreateInsert(ctx, result, 4, new ExprNullptrLiteral(expression.source, ctx.ctx.GetReferenceType(ptrTypeID.value)));

			return CheckType(expression, result);
		}

		ExprPointerLiteral ref resultPtr = AllocateTypeStorage(ctx, expression.source, ptrTypeID.value);

		if(!resultPtr)
			return nullptr;

		CreateInsert(ctx, result, 4, resultPtr);

		ExprBase ref ptrPtrLoad = CreateLoad(ctx, ptrPtr);

		if(!ptrPtrLoad)
			return nullptr;

		if(!CreateStore(ctx, resultPtr, ptrPtrLoad))
			return nullptr;

		return CheckType(expression, result);
	}
	else if(ptr.data.name.name == InplaceStr("duplicate") && arguments.size() == 1 && arguments[0].type == ctx.ctx.typeAutoArray)
	{
		ExprMemoryLiteral ref arr = getType with<ExprMemoryLiteral>(arguments[0]);

		assert(arr != nullptr);

		ExprTypeLiteral ref arrTypeID = getType with<ExprTypeLiteral>(CreateExtract(ctx, arr, 0, ctx.ctx.typeTypeID));
		ExprIntegerLiteral ref arrLen = getType with<ExprIntegerLiteral>(CreateExtract(ctx, arr, 4 + sizeof(void ref), ctx.ctx.typeInt));
		ExprPointerLiteral ref arrPtr = getType with<ExprPointerLiteral>(CreateExtract(ctx, arr, 4, ctx.ctx.GetReferenceType(ctx.ctx.GetArrayType(arrTypeID.value, arrLen.value))));

		ExprPointerLiteral ref storage = AllocateTypeStorage(ctx, expression.source, ctx.ctx.typeAutoArray);

		if(!storage)
			return nullptr;

		ExprMemoryLiteral ref result = new ExprMemoryLiteral(expression.source, ctx.ctx.typeAutoArray, storage);

		CreateInsert(ctx, result, 0, new ExprTypeLiteral(expression.source, ctx.ctx.typeTypeID, arrTypeID.value));
		CreateInsert(ctx, result, 4 + sizeof(void ref), new ExprIntegerLiteral(expression.source, ctx.ctx.typeInt, arrLen.value));

		if(!arrPtr)
		{
			CreateInsert(ctx, result, 4, new ExprNullptrLiteral(expression.source, ctx.ctx.GetReferenceType(ctx.ctx.typeVoid)));

			return CheckType(expression, result);
		}

		ExprPointerLiteral ref resultPtr = AllocateTypeStorage(ctx, expression.source, ctx.ctx.GetArrayType(arrTypeID.value, arrLen.value));

		if(!resultPtr)
			return nullptr;

		CreateInsert(ctx, result, 4, resultPtr);

		ExprBase ref ptrPtrLoad = CreateLoad(ctx, arrPtr);

		if(!ptrPtrLoad)
			return nullptr;

		if(!CreateStore(ctx, resultPtr, ptrPtrLoad))
			return nullptr;

		return CheckType(expression, result);
	}
	else if(ptr.data.name.name == InplaceStr("typeid") && arguments.size() == 1 && arguments[0].type == ctx.ctx.typeAutoRef)
	{
		ExprMemoryLiteral ref reference = getType with<ExprMemoryLiteral>(arguments[0]);

		assert(reference != nullptr);

		ExprTypeLiteral ref typeID = getType with<ExprTypeLiteral>(CreateExtract(ctx, reference, 0, ctx.ctx.typeTypeID));

		return CheckType(expression, typeID);
	}
	else if(ptr.data.name.name == InplaceStr("auto_array") && arguments.size() == 2 && arguments[0].type == ctx.ctx.typeTypeID && arguments[1].type == ctx.ctx.typeInt)
	{
		ExprTypeLiteral ref type = getType with<ExprTypeLiteral>(arguments[0]);
		ExprIntegerLiteral ref count = getType with<ExprIntegerLiteral>(arguments[1]);

		assert(type && count);

		ExprPointerLiteral ref storage = AllocateTypeStorage(ctx, expression.source, ctx.ctx.typeAutoArray);

		if(!storage)
			return nullptr;

		ExprMemoryLiteral ref result = new ExprMemoryLiteral(expression.source, ctx.ctx.typeAutoArray, storage);

		CreateInsert(ctx, result, 0, new ExprTypeLiteral(expression.source, ctx.ctx.typeTypeID, type.value));

		ExprPointerLiteral ref resultPtr = AllocateTypeStorage(ctx, expression.source, ctx.ctx.GetArrayType(type.value, count.value));

		if(!resultPtr)
			return nullptr;

		CreateInsert(ctx, result, 4, resultPtr);
		CreateInsert(ctx, result, 4 + sizeof(void ref), new ExprIntegerLiteral(expression.source, ctx.ctx.typeInt, count.value));

		return CheckType(expression, result);
	}
	else if(ptr.data.name.name == InplaceStr("array_copy") && arguments.size() == 2 && arguments[0].type == ctx.ctx.typeAutoArray && arguments[1].type == ctx.ctx.typeAutoArray)
	{
		ExprMemoryLiteral ref dst = getType with<ExprMemoryLiteral>(arguments[0]);
		ExprMemoryLiteral ref src = getType with<ExprMemoryLiteral>(arguments[1]);

		assert(dst && src);

		ExprTypeLiteral ref dstTypeID = getType with<ExprTypeLiteral>(CreateExtract(ctx, dst, 0, ctx.ctx.typeTypeID));
		ExprPointerLiteral ref dstPtr = getType with<ExprPointerLiteral>(CreateExtract(ctx, dst, 4, ctx.ctx.GetReferenceType(ctx.ctx.typeVoid)));
		ExprIntegerLiteral ref dstLen = getType with<ExprIntegerLiteral>(CreateExtract(ctx, dst, 4 + sizeof(void ref), ctx.ctx.typeInt));

		ExprTypeLiteral ref srcTypeID = getType with<ExprTypeLiteral>(CreateExtract(ctx, src, 0, ctx.ctx.typeTypeID));
		ExprPointerLiteral ref srcPtr = getType with<ExprPointerLiteral>(CreateExtract(ctx, src, 4, ctx.ctx.GetReferenceType(ctx.ctx.typeVoid)));
		ExprIntegerLiteral ref srcLen = getType with<ExprIntegerLiteral>(CreateExtract(ctx, src, 4 + sizeof(void ref), ctx.ctx.typeInt));

		if(!dstPtr && !srcPtr)
			return CheckType(expression, new ExprVoid(expression.source, ctx.ctx.typeVoid));

		if(!srcPtr || dstPtr.ptr.buffer == srcPtr.ptr.buffer)
			return CheckType(expression, new ExprVoid(expression.source, ctx.ctx.typeVoid));

		if(dstTypeID.value != srcTypeID.value)
			return Report(ctx, "ERROR: destination element type '%.*s' doesn't match source element type '%.*s'", FMT_ISTR(dstTypeID.value.name), FMT_ISTR(srcTypeID.value.name));

		if(dstLen.value < srcLen.value)
			return Report(ctx, "ERROR: destination array size '%d' is smaller than source array size '%d'", int(dstLen.value), int(srcLen.value));

		memcpy(dstPtr.ptr.buffer, srcPtr.ptr.buffer, int(dstTypeID.value.size * srcLen.value));

		return CheckType(expression, new ExprVoid(expression.source, ctx.ctx.typeVoid));
	}
	else if(ptr.data.name.name == InplaceStr("[]") && arguments.size() == 2 && arguments[0].type == ctx.ctx.GetReferenceType(ctx.ctx.typeAutoArray) && arguments[1].type == ctx.ctx.typeInt)
	{
		// Get arguments
		ExprPointerLiteral ref arrPtrArg = getType with<ExprPointerLiteral>(arguments[0]);

		if(!arrPtrArg)
			return Report(ctx, "ERROR: null pointer access");

		ExprIntegerLiteral ref indexArg = getType with<ExprIntegerLiteral>(arguments[1]);

		assert(indexArg != nullptr);

		ExprBase ref arrPtrLoad = CreateLoad(ctx, arrPtrArg);

		if(!arrPtrLoad)
			return nullptr;

		ExprMemoryLiteral ref arr = getType with<ExprMemoryLiteral>(arrPtrLoad);

		assert(arr != nullptr);

		// Check index
		ExprIntegerLiteral ref arrLen = getType with<ExprIntegerLiteral>(CreateExtract(ctx, arr, 4 + sizeof(void ref), ctx.ctx.typeInt));

		if(int(indexArg.value) >= arrLen.value)
			return Report(ctx, "ERROR: array index out of bounds");

		// Load auto[] array type and pointer members
		ExprTypeLiteral ref arrTypeID = getType with<ExprTypeLiteral>(CreateExtract(ctx, arr, 0, ctx.ctx.typeTypeID));
		ExprPointerLiteral ref arrPtr = getType with<ExprPointerLiteral>(CreateExtract(ctx, arr, 4, ctx.ctx.GetReferenceType(ctx.ctx.GetArrayType(arrTypeID.value, arrLen.value))));

		// Create storage for result
		ExprPointerLiteral ref storage = AllocateTypeStorage(ctx, expression.source, ctx.ctx.typeAutoRef);

		if(!storage)
			return nullptr;

		// Create result in that storage
		ExprMemoryLiteral ref result = new ExprMemoryLiteral(expression.source, ctx.ctx.typeAutoRef, storage);

		// Save typeid
		CreateInsert(ctx, result, 0, new ExprTypeLiteral(expression.source, ctx.ctx.typeTypeID, arrTypeID.value));

		// Save pointer to array element
		assert(arrPtr.start + indexArg.value * arrTypeID.value.size + arrTypeID.value.size <= arrPtr.end);

		int targetOffset = arrPtr.start + indexArg.value * arrTypeID.value.size;

		ExprPointerLiteral ref shifted = new ExprPointerLiteral(expression.source, ctx.ctx.GetReferenceType(arrTypeID.value), arrPtr.ptr, targetOffset, targetOffset + arrTypeID.value.size);

		CreateInsert(ctx, result, 4, shifted);

		return CheckType(expression, result);
	}
	else if(ptr.data.name.name == InplaceStr("__assertCoroutine") && arguments.size() == 1 && arguments[0].type == ctx.ctx.typeAutoRef)
	{
		ExprMemoryLiteral ref functionPtr = getType with<ExprMemoryLiteral>(arguments[0]);

		assert(functionPtr != nullptr);

		ExprTypeLiteral ref ptrTypeID = getType with<ExprTypeLiteral>(CreateExtract(ctx, functionPtr, 0, ctx.ctx.typeTypeID));

		if(!isType with<TypeFunction>(ptrTypeID.value))
			return Report(ctx, "ERROR: '%.*s' is not a function'", FMT_ISTR(ptrTypeID.value.name));

		ExprPointerLiteral ref ptrPtr = getType with<ExprPointerLiteral>(CreateExtract(ctx, functionPtr, 4, ctx.ctx.GetReferenceType(ptrTypeID.value)));

		assert(ptrPtr != nullptr);

		ExprFunctionLiteral ref function = getType with<ExprFunctionLiteral>(CreateLoad(ctx, ptrPtr));

		if(!function.data)
			return Report(ctx, "ERROR: function is not a coroutine'");

		if(!function.data.isCoroutine)
			return Report(ctx, "ERROR: '%.*s' is not a coroutine'", FMT_ISTR(function.data.name.name));

		return CheckType(expression, new ExprVoid(expression.source, ctx.ctx.typeVoid));
	}
	else if(ptr.data.name.name == InplaceStr("isCoroutineReset") && arguments.size() == 1 && arguments[0].type == ctx.ctx.typeAutoRef)
	{
		ExprMemoryLiteral ref functionPtr = getType with<ExprMemoryLiteral>(arguments[0]);

		assert(functionPtr != nullptr);

		ExprTypeLiteral ref ptrTypeID = getType with<ExprTypeLiteral>(CreateExtract(ctx, functionPtr, 0, ctx.ctx.typeTypeID));

		if(!isType with<TypeFunction>(ptrTypeID.value))
			return Report(ctx, "ERROR: '%.*s' is not a function'", FMT_ISTR(ptrTypeID.value.name));

		ExprPointerLiteral ref ptrPtr = getType with<ExprPointerLiteral>(CreateExtract(ctx, functionPtr, 4, ctx.ctx.GetReferenceType(ptrTypeID.value)));

		assert(ptrPtr != nullptr);

		ExprFunctionLiteral ref function = getType with<ExprFunctionLiteral>(CreateLoad(ctx, ptrPtr));

		if(!function.data.isCoroutine)
			return Report(ctx, "ERROR: '%.*s' is not a coroutine'", FMT_ISTR(function.data.name.name));

		ExprBase ref contextLoad = CreateLoad(ctx, function.context);

		if(!contextLoad)
			return nullptr;

		ExprMemoryLiteral ref context = getType with<ExprMemoryLiteral>(contextLoad);

		// TODO: remove this check, all coroutines must have a context
		if(!context)
			return Report(ctx, "ERROR: '%.*s' coroutine has no context'", FMT_ISTR(function.data.name.name));

		ExprIntegerLiteral ref jmpOffset = getType with<ExprIntegerLiteral>(CreateExtract(ctx, context, 0, ctx.ctx.typeInt));

		return CheckType(expression, new ExprIntegerLiteral(expression.source, ctx.ctx.typeInt, jmpOffset.value == 0));
	}
	else if(ptr.data.name.name == InplaceStr("assert_derived_from_base") && arguments.size() == 2 && arguments[0].type == ctx.ctx.GetReferenceType(ctx.ctx.typeVoid) && arguments[1].type == ctx.ctx.typeTypeID)
	{
		ExprPointerLiteral ref object = getType with<ExprPointerLiteral>(arguments[0]);
		ExprTypeLiteral ref base = getType with<ExprTypeLiteral>(arguments[1]);

		if(!object)
			return CheckType(expression, new ExprNullptrLiteral(expression.source, ctx.ctx.GetReferenceType(ctx.ctx.typeVoid)));

		assert(object.end - object.start >= sizeof(int));

		ExprTypeLiteral ref derived = getType with<ExprTypeLiteral>(CreateLoad(ctx, new ExprPointerLiteral(expression.source, ctx.ctx.GetReferenceType(ctx.ctx.typeTypeID), object.ptr, object.start, object.end)));

		assert(derived != nullptr);

		TypeBase ref curr = derived.value;

		while(curr)
		{
			if(curr == base.value)
				return new ExprPointerLiteral(expression.source, ctx.ctx.GetReferenceType(curr), object.ptr, object.start, object.start + curr.size);

			if(TypeClass ref classType = getType with<TypeClass>(curr))
				curr = classType.baseClass;
			else
				curr = nullptr;
		}

		return Report(ctx, "ERROR: cannot convert from '%.*s' to '%.*s'", FMT_ISTR(derived.value.name), FMT_ISTR(base.value.name));
	}
	else if(ptr.data.name.name == InplaceStr("__closeUpvalue") && arguments.size() == 4)
	{
		ExprPointerLiteral ref upvalueListLocation = getType with<ExprPointerLiteral>(arguments[0]);
		ExprPointerLiteral ref variableLocation = getType with<ExprPointerLiteral>(arguments[1]);
		ExprIntegerLiteral ref offsetToCopy = getType with<ExprIntegerLiteral>(arguments[2]);
		ExprIntegerLiteral ref copySize = getType with<ExprIntegerLiteral>(arguments[3]);

		assert(upvalueListLocation != nullptr);
		assert(variableLocation != nullptr);
		assert(offsetToCopy != nullptr);
		assert(copySize != nullptr);

		ExprBase ref upvalueListHeadBase = CreateLoad(ctx, upvalueListLocation);

		// Nothing to close if the list is empty
		if(getType with<ExprNullptrLiteral>(upvalueListHeadBase))
			return CheckType(expression, new ExprVoid(expression.source, ctx.ctx.typeVoid));

		ExprPointerLiteral ref upvalueListHead = getType with<ExprPointerLiteral>(upvalueListHeadBase);

		assert(upvalueListHead != nullptr);

		assert(false, "upvalues are not implemented");

		/*struct Upvalue
		{
			void ref target;
			Upvalue ref next;
		};

		Upvalue ref upvalue = (Upvalue ref)upvalueListHead.ptr;

		assert(upvalue != nullptr);

		while (upvalue && ReadVmMemoryPointer(&upvalue.target) == variableLocation.ptr)
		{
			Upvalue ref next = (Upvalue ref)ReadVmMemoryPointer(&upvalue.next);

			int char ref copy = (int char*)upvalue + offsetToCopy.value;
			memcpy(copy, variableLocation.ptr, int(copySize.value));
			WriteVmMemoryPointer(&upvalue.target, copy);
			WriteVmMemoryPointer(&upvalue.next, nullptr);

			upvalue = next;
		}

		if(!CreateStore(ctx, upvalueListLocation, new ExprPointerLiteral(expression.source, ctx.ctx.GetReferenceType(ctx.ctx.typeVoid), (int char*)upvalue, (int char*)upvalue + NULLC_PTR_SIZE)))
			return nullptr;*/

		return CheckType(expression, new ExprVoid(expression.source, ctx.ctx.typeVoid));
	}

	return nullptr;
}

ExprBase ref EvaluateFunctionCall(ExpressionEvalContext ref ctx, ExprFunctionCall ref expression)
{
	if(!ctx.stackFrames.empty() && ctx.stackFrames.back().targetYield)
		return new ExprVoid(expression.source, ctx.ctx.typeVoid);

	if(!AddInstruction(ctx))
		return nullptr;
	
	ExprBase ref function = Evaluate(ctx, expression.function);

	if(!function)
		return nullptr;

	vector<ExprBase ref> arguments;

	for(ExprBase ref curr = expression.arguments.head; curr; curr = curr.next)
	{
		ExprBase ref value = Evaluate(ctx, curr);

		if(!value)
			return nullptr;

		arguments.push_back(value);
	}

	ExprFunctionLiteral ref ptr = getType with<ExprFunctionLiteral>(function);

	if(!ptr)
		return nullptr;

	if(!ptr.data)
		return Report(ctx, "ERROR: null function pointer call");

	if(!ptr.data.declaration)
	{
		if(ctx.emulateKnownExternals && ctx.ctx.GetFunctionIndex(ptr.data) < ctx.ctx.baseModuleFunctionCount)
		{
			if(ExprBase ref result = EvaluateKnownExternalFunctionCall(ctx, expression, ptr, ViewOf(arguments)))
				return result;
		}

		return Report(ctx, "ERROR: function '%.*s' has no source", FMT_ISTR(ptr.data.name.name));
	}

	if(ptr.data.isPrototype)
		return Report(ctx, "ERROR: function '%.*s' has no source", FMT_ISTR(ptr.data.name.name));

	ExprFunctionDefinition ref declaration = getType with<ExprFunctionDefinition>(ptr.data.declaration);

	assert(declaration != nullptr);

	if(declaration.arguments.size() != arguments.size())
		return nullptr;

	ExprBase ref call = EvaluateFunction(ctx, declaration, ptr.context, ViewOf(arguments));

	if(!call)
		return nullptr;

	return CheckType(expression, call);
}

ExprBase ref EvaluateIfElse(ExpressionEvalContext ref ctx, ExprIfElse ref expression)
{
	if(ctx.stackFrames.back().targetYield)
		return CheckType(expression, new ExprVoid(expression.source, ctx.ctx.typeVoid));

	if(!AddInstruction(ctx))
		return nullptr;

	ExprBase ref condition = Evaluate(ctx, expression.condition);

	if(!condition)
		return nullptr;

	long result;
	if(!TryTakeLong(condition, result))
		return Report(ctx, "ERROR: failed to evaluate 'if' condition");

	if(result)
	{
		if(!Evaluate(ctx, expression.trueBlock))
			return nullptr;
	}
	else if(expression.falseBlock)
	{
		if(!Evaluate(ctx, expression.falseBlock))
			return nullptr;
	}

	return CheckType(expression, new ExprVoid(expression.source, ctx.ctx.typeVoid));
}

ExprBase ref EvaluateFor(ExpressionEvalContext ref ctx, ExprFor ref expression)
{
	if(!AddInstruction(ctx))
		return nullptr;

	StackFrame ref frame = *ctx.stackFrames.back();

	if(!frame.targetYield)
	{
		if(!Evaluate(ctx, expression.initializer))
			return nullptr;
	}

	for(;1;)
	{
		if(!AddInstruction(ctx))
			return nullptr;

		if(!frame.targetYield)
		{
			if(!expression.condition)
				return nullptr;

			ExprBase ref condition = Evaluate(ctx, expression.condition);

			if(!condition)
				return nullptr;

			long result;
			if(!TryTakeLong(condition, result))
				return Report(ctx, "ERROR: failed to evaluate 'for' condition");

			if(!result)
				break;
		}

		if(!Evaluate(ctx, expression.body))
			return nullptr;

		// On break, decrease depth and exit
		if(frame.breakDepth)
		{
			frame.breakDepth--;
			break;
		}

		// On continue, decrease depth and proceed to next iteration, unless it's a multi-level continue
		if(frame.continueDepth)
		{
			frame.continueDepth--;

			if(frame.continueDepth)
				break;
		}

		if(frame.returnValue)
			break;

		if(!frame.targetYield)
		{
			if(!Evaluate(ctx, expression.increment))
				return nullptr;
		}
	}

	return CheckType(expression, new ExprVoid(expression.source, ctx.ctx.typeVoid));
}

ExprBase ref EvaluateWhile(ExpressionEvalContext ref ctx, ExprWhile ref expression)
{
	StackFrame ref frame = *ctx.stackFrames.back();

	for(;1;)
	{
		if(!AddInstruction(ctx))
			return nullptr;

		if(!frame.targetYield)
		{
			ExprBase ref condition = Evaluate(ctx, expression.condition);

			if(!condition)
				return nullptr;

			long result;
			if(!TryTakeLong(condition, result))
				return Report(ctx, "ERROR: failed to evaluate 'while' condition");

			if(!result)
				break;
		}

		if(!Evaluate(ctx, expression.body))
			return nullptr;

		// On break, decrease depth and exit
		if(frame.breakDepth)
		{
			frame.breakDepth--;
			break;
		}

		// On continue, decrease depth and proceed to next iteration, unless it's a multi-level continue
		if(frame.continueDepth)
		{
			frame.continueDepth--;

			if(frame.continueDepth)
				break;
		}

		if(frame.returnValue)
			break;
	}

	return CheckType(expression, new ExprVoid(expression.source, ctx.ctx.typeVoid));
}

ExprBase ref EvaluateDoWhile(ExpressionEvalContext ref ctx, ExprDoWhile ref expression)
{
	StackFrame ref frame = *ctx.stackFrames.back();

	for(;1;)
	{
		if(!AddInstruction(ctx))
			return nullptr;

		if(!Evaluate(ctx, expression.body))
			return nullptr;

		// On break, decrease depth and exit
		if(frame.breakDepth)
		{
			frame.breakDepth--;
			break;
		}

		// On continue, decrease depth and proceed to next iteration, unless it's a multi-level continue
		if(frame.continueDepth)
		{
			frame.continueDepth--;

			if(frame.continueDepth)
				break;
		}

		if(frame.returnValue)
			break;

		if(!frame.targetYield)
		{
			ExprBase ref condition = Evaluate(ctx, expression.condition);

			if(!condition)
				return nullptr;

			long result;
			if(!TryTakeLong(condition, result))
				return Report(ctx, "ERROR: failed to evaluate 'do' condition");

			if(!result)
				break;
		}
	}

	return CheckType(expression, new ExprVoid(expression.source, ctx.ctx.typeVoid));
}

ExprBase ref EvaluateSwitch(ExpressionEvalContext ref ctx, ExprSwitch ref expression)
{
	if(!AddInstruction(ctx))
		return nullptr;

	StackFrame ref frame = *ctx.stackFrames.back();

	if(frame.targetYield)
		return Report(ctx, "ERROR: can't yield back into a switch statement");

	ExprBase ref condition = Evaluate(ctx, expression.condition);

	if(!condition)
		return nullptr;

	bool matched = false;

	for(ExprBase ref currCase = expression.cases.head, currBlock = expression.blocks.head; currCase && currBlock; { currCase = currCase.next; currBlock = currBlock.next; })
	{
		if(!AddInstruction(ctx))
			return nullptr;

		if(!matched)
		{
			ExprBase ref value = Evaluate(ctx, currCase);

			if(!value)
				return nullptr;

			long result;
			if(!TryTakeLong(value, result))
				return Report(ctx, "ERROR: failed to evaluate 'case' value");

			// Try next case
			if(!result)
				continue;

			matched = true;
		}

		if(!Evaluate(ctx, currBlock))
			return nullptr;

		// On break, decrease depth and exit
		if(frame.breakDepth)
		{
			frame.breakDepth--;

			return CheckType(expression, new ExprVoid(expression.source, ctx.ctx.typeVoid));
		}
	}

	if(expression.defaultBlock)
	{
		if(!Evaluate(ctx, expression.defaultBlock))
			return nullptr;

		// On break, decrease depth and exit
		if(frame.breakDepth)
		{
			frame.breakDepth--;

			return CheckType(expression, new ExprVoid(expression.source, ctx.ctx.typeVoid));
		}
	}

	return CheckType(expression, new ExprVoid(expression.source, ctx.ctx.typeVoid));
}

ExprBase ref EvaluateBreak(ExpressionEvalContext ref ctx, ExprBreak ref expression)
{
	StackFrame ref frame = *ctx.stackFrames.back();

	if(!frame.targetYield)
	{
		assert(frame.breakDepth == 0);

		frame.breakDepth = expression.depth;
	}

	if(expression.closures)
	{
		if(!Evaluate(ctx, expression.closures))
			return nullptr;
	}

	return CheckType(expression, new ExprVoid(expression.source, ctx.ctx.typeVoid));
}

ExprBase ref EvaluateContinue(ExpressionEvalContext ref ctx, ExprContinue ref expression)
{
	StackFrame ref frame = *ctx.stackFrames.back();

	if(!frame.targetYield)
	{
		assert(frame.continueDepth == 0);

		frame.continueDepth = expression.depth;
	}

	if(expression.closures)
	{
		if(!Evaluate(ctx, expression.closures))
			return nullptr;
	}

	return CheckType(expression, new ExprVoid(expression.source, ctx.ctx.typeVoid));
}

ExprBase ref EvaluateBlock(ExpressionEvalContext ref ctx, ExprBlock ref expression)
{
	if(!AddInstruction(ctx))
		return nullptr;

	StackFrame ref frame = *ctx.stackFrames.back();

	for(ExprBase ref value = expression.expressions.head; value; value = value.next)
	{
		if(!Evaluate(ctx, value))
			return nullptr;

		if(frame.continueDepth || frame.breakDepth || frame.returnValue)
			return CheckType(expression, new ExprVoid(expression.source, ctx.ctx.typeVoid));
	}

	if(expression.closures)
	{
		if(!Evaluate(ctx, expression.closures))
			return nullptr;
	}

	return CheckType(expression, new ExprVoid(expression.source, ctx.ctx.typeVoid));
}

ExprBase ref EvaluateSequence(ExpressionEvalContext ref ctx, ExprSequence ref expression)
{
	if(!AddInstruction(ctx))
		return nullptr;

	ExprBase ref result = new ExprVoid(expression.source, ctx.ctx.typeVoid);

	for(int i = 0; i < expression.expressions.size(); i++)
	{
		result = Evaluate(ctx, expression.expressions[i]);

		if(!result)
			return nullptr;
	}

	if(!ctx.stackFrames.empty() && ctx.stackFrames.back().targetYield)
		return new ExprVoid(expression.source, ctx.ctx.typeVoid);

	return CheckType(expression, result);
}

ExprBase ref EvaluateModule(ExpressionEvalContext ref ctx, ExprModule ref expression)
{
	ctx.globalFrame = new StackFrame(nullptr);
	ctx.stackFrames.push_back(ctx.globalFrame);

	StackFrame ref frame = *ctx.stackFrames.back();

	for(ExprBase ref value = expression.setup.head; value; value = value.next)
	{
		if(!Evaluate(ctx, value))
			return nullptr;
	}

	for(ExprBase ref value = expression.expressions.head; value; value = value.next)
	{
		if(!Evaluate(ctx, value))
			return nullptr;

		assert(frame.breakDepth == 0 && frame.continueDepth == 0);

		if(frame.returnValue)
			return frame.returnValue;
	}

	ctx.stackFrames.pop_back();

	assert(ctx.stackFrames.empty());

	return nullptr;
}

ExprBase ref Evaluate(ExpressionEvalContext ref ctx, ExprBase ref expression)
{
	if(isType with<TypeError>(expression.type))
		return nullptr;

	ctx.expressionDepth++;

	if(ctx.expressionDepth > ctx.expressionDepthLimit)
		return Report(ctx, "ERROR: expression depth limit reached");

	ExprBase ref result = nullptr;

	switch(typeid(expression))
	{
	case ExprError:
		result = Report(ctx, "ERROR: invalid expression");
		break;
	case ExprErrorTypeMemberAccess:
		result = Report(ctx, "ERROR: invalid expression");
		break;
	case ExprVoid:
		result = EvaluateVoid(ctx, expression);
		break;
	case ExprBoolLiteral:
		result = EvaluateBoolLiteral(ctx, (ExprBoolLiteral ref)(expression));
		break;
	case ExprCharacterLiteral:
		result = EvaluateCharacterLiteral(ctx, (ExprCharacterLiteral ref)(expression));
		break;
	case ExprStringLiteral:
		result = EvaluateStringLiteral(ctx, (ExprStringLiteral ref)(expression));
		break;
	case ExprIntegerLiteral:
		result = EvaluateIntegerLiteral(ctx, (ExprIntegerLiteral ref)(expression));
		break;
	case ExprRationalLiteral:
		result = EvaluateRationalLiteral(ctx, (ExprRationalLiteral ref)(expression));
		break;
	case ExprTypeLiteral:
		result = EvaluateTypeLiteral(ctx, (ExprTypeLiteral ref)(expression));
		break;
	case ExprNullptrLiteral:
		result = EvaluateNullptrLiteral(ctx, (ExprNullptrLiteral ref)(expression));
		break;
	case ExprFunctionIndexLiteral:
		result = EvaluateFunctionIndexLiteral(ctx, (ExprFunctionIndexLiteral ref)(expression));
		break;
	case ExprPassthrough:
		result = Evaluate(ctx, ((ExprPassthrough ref)(expression)).value);
		break;
	case ExprArray:
		result = EvaluateArray(ctx, (ExprArray ref)(expression));
		break;
	case ExprPreModify:
		result = EvaluatePreModify(ctx, (ExprPreModify ref)(expression));
		break;
	case ExprPostModify:
		result = EvaluatePostModify(ctx, (ExprPostModify ref)(expression));
		break;
	case ExprTypeCast:
		result = EvaluateCast(ctx, (ExprTypeCast ref)(expression));
		break;
	case ExprUnaryOp:
		result = EvaluateUnaryOp(ctx, (ExprUnaryOp ref)(expression));
		break;
	case ExprBinaryOp:
		result = EvaluateBinaryOp(ctx, (ExprBinaryOp ref)(expression));
		break;
	case ExprGetAddress:
		result = EvaluateGetAddress(ctx, (ExprGetAddress ref)(expression));
		break;
	case ExprDereference:
		result = EvaluateDereference(ctx, (ExprDereference ref)(expression));
		break;
	case ExprUnboxing:
		result = Evaluate(ctx, ((ExprUnboxing ref)(expression)).value);
		break;
	case ExprConditional:
		result = EvaluateConditional(ctx, (ExprConditional ref)(expression));
		break;
	case ExprAssignment:
		result = EvaluateAssignment(ctx, (ExprAssignment ref)(expression));
		break;
	case ExprMemberAccess:
		result = EvaluateMemberAccess(ctx, (ExprMemberAccess ref)(expression));
		break;
	case ExprArrayIndex:
		result = EvaluateArrayIndex(ctx, (ExprArrayIndex ref)(expression));
		break;
	case ExprReturn:
		result = EvaluateReturn(ctx, (ExprReturn ref)(expression));
		break;
	case ExprYield:
		result = EvaluateYield(ctx, (ExprYield ref)(expression));
		break;
	case ExprVariableDefinition:
		result = EvaluateVariableDefinition(ctx, (ExprVariableDefinition ref)(expression));
		break;
	case ExprArraySetup:
		result = EvaluateArraySetup(ctx, (ExprArraySetup ref)(expression));
		break;
	case ExprVariableDefinitions:
		result = EvaluateVariableDefinitions(ctx, (ExprVariableDefinitions ref)(expression));
		break;
	case ExprVariableAccess:
		result = EvaluateVariableAccess(ctx, (ExprVariableAccess ref)(expression));
		break;
	case ExprFunctionContextAccess:
		result = EvaluateFunctionContextAccess(ctx, (ExprFunctionContextAccess ref)(expression));
		break;
	case ExprFunctionDefinition:
		result = EvaluateFunctionDefinition(ctx, (ExprFunctionDefinition ref)(expression));
		break;
	case ExprGenericFunctionPrototype:
		result = EvaluateGenericFunctionPrototype(ctx, (ExprGenericFunctionPrototype ref)(expression));
		break;
	case ExprFunctionAccess:
		result = EvaluateFunctionAccess(ctx, (ExprFunctionAccess ref)(expression));
		break;
	case ExprFunctionOverloadSet:
		result = nullptr;
		break;
	case ExprShortFunctionOverloadSet:
		result = nullptr;
		break;
	case ExprFunctionCall:
		result = EvaluateFunctionCall(ctx, (ExprFunctionCall ref)(expression));
		break;
	case ExprAliasDefinition:
		result = EvaluateVoid(ctx, expression);
		break;
	case ExprClassPrototype:
		result = EvaluateVoid(ctx, expression);
		break;
	case ExprGenericClassPrototype:
		result = EvaluateVoid(ctx, expression);
		break;
	case ExprClassDefinition:
		result = EvaluateVoid(ctx, expression);
		break;
	case ExprEnumDefinition:
		result = EvaluateVoid(ctx, expression);
		break;
	case ExprIfElse:
		result = EvaluateIfElse(ctx, (ExprIfElse ref)(expression));
		break;
	case ExprFor:
		result = EvaluateFor(ctx, (ExprFor ref)(expression));
		break;
	case ExprWhile:
		result = EvaluateWhile(ctx, (ExprWhile ref)(expression));
		break;
	case ExprDoWhile:
		result = EvaluateDoWhile(ctx, (ExprDoWhile ref)(expression));
		break;
	case ExprSwitch:
		result = EvaluateSwitch(ctx, (ExprSwitch ref)(expression));
		break;
	case ExprBreak:
		result = EvaluateBreak(ctx, (ExprBreak ref)(expression));
		break;
	case ExprContinue:
		result = EvaluateContinue(ctx, (ExprContinue ref)(expression));
		break;
	case ExprBlock:
		result = EvaluateBlock(ctx, (ExprBlock ref)(expression));
		break;
	case ExprSequence:
		result = EvaluateSequence(ctx, (ExprSequence ref)(expression));
		break;
	case ExprModule:
		result = EvaluateModule(ctx, (ExprModule ref)(expression));
		break;
	default:
		assert(false, "unknown type");
		break;
	}

	ctx.expressionDepth--;

	return result;
}

char[] EvaluateToBuffer(ExpressionEvalContext ref ctx, ExprBase ref expression)
{
	if(ExprBase ref value = Evaluate(ctx, expression))
	{
		if(ExprBoolLiteral ref result = getType with<ExprBoolLiteral>(value))
		{
			return (result.value ? 1 : 0).str();
		}
		else if(ExprCharacterLiteral ref result = getType with<ExprCharacterLiteral>(value))
		{
			char[] str = new char[2];
			str[0] = result.value;
			return str;
		}
		else if(ExprIntegerLiteral ref result = getType with<ExprIntegerLiteral>(value))
		{
			if(result.type == ctx.ctx.typeLong)
				return result.value.str();
			else
				return int(result.value).str();
		}
		else if(ExprRationalLiteral ref result = getType with<ExprRationalLiteral>(value))
		{
			return result.value.str();
		}
		else
		{
			return "unknown";
		}
	}

	return nullptr;
}

char[] TestEvaluation(ExpressionContext ref ctx, ExprBase ref expression, char[] errorBuf)
{
	if(!expression)
		return nullptr;

	ExpressionEvalContext evalCtx = ExpressionEvalContext(ctx);

	evalCtx.errorBuf = errorBuf;

	evalCtx.emulateKnownExternals = true;

	return EvaluateToBuffer(evalCtx, expression);
}
