import stringutil;
import expressiontree;
import parser;
import typetreehelpers;
import expressioncontext;
import expressioneval;
import bytecode;
import binarycache;
import std.error;
import std.string;

void string::string(char[] right, int start, int end)
{
	assert(start <= end);
	assert(start >= 0 && start < right.size);
	assert(end >= 0 && end < right.size);

	data = new char[end - start + 1];

	for(int i = 0; i < data.size; i++)
		data[i] = right[start + i];
}

void string::string(StringRef right)
{
	int length;

	while(right.string[right.pos + length])
		length++;

	data = new char[length + 1];

	for(int i = 0; i < data.size; i++)
		data[i] = right.string[right.pos + i];
}

void string::string(char[] right, int pos)
{
	int length;

	while(right[pos + length])
		length++;

	data = new char[length + 1];

	for(int i = 0; i < data.size; i++)
		data[i] = right[pos + i];
}

void string::string(InplaceStr right)
{
	data = new char[right.end - right.begin + 1];

	for(int i = 0; i < data.size; i++)
		data[i] = right[i];
}

void InplaceStr::InplaceStr(StringRef str)
{
	data = str.string;
	begin = str.pos;
	end = str.pos;

	while(data[end])
		end++;
}

/*InplaceStr FindModuleNameWithSourceLocation(ExpressionContext ref ctx, char[] code, int position);
char[] FindModuleCodeWithSourceLocation(ExpressionContext ref ctx, char[] code, int position);*/

class AnalyzerError
{
}

void ReportAt(ExpressionContext ref ctx, SynBase ref source, StringRef pos, char[] msg, auto ref[] args)
{
	if(ctx.errorBuf)
	{
		if(ctx.errorCount == 0)
		{
			ctx.errorPos = pos;
		}

		*ctx.errorBuf += msg;
		/*const char ref messageStart = ctx.errorBufLocation;

		vsnprintf(ctx.errorBufLocation, ctx.errorBufSize - int(ctx.errorBufLocation - ctx.errorBuf), msg, args);
		ctx.errorBuf[ctx.errorBufSize - 1] = '\0';

		// Shouldn't report errors that are caused by error recovery
		assert(strstr(ctx.errorBufLocation, "%error-type%") == nullptr);

		ctx.errorBufLocation += strlen(ctx.errorBufLocation);

		const char ref messageEnd = ctx.errorBufLocation;

		ctx.errorInfo.push_back(new ErrorInfo(messageStart, messageEnd, source.begin, source.end, pos));

		if(const char ref code = FindModuleCodeWithSourceLocation(ctx, pos))
		{
			AddErrorLocationInfo(code, pos, ctx.errorBufLocation, ctx.errorBufSize - int(ctx.errorBufLocation - ctx.errorBuf));

			ctx.errorBufLocation += strlen(ctx.errorBufLocation);

			if(code != ctx.code)
			{
				InplaceStr parentModule = FindModuleNameWithSourceLocation(ctx, pos);

				if(!parentModule.empty())
				{
					NULLC::SafeSprintf(ctx.errorBufLocation, ctx.errorBufSize - int(ctx.errorBufLocation - ctx.errorBuf), " [in module '%.*s']\n", FMT_ISTR(parentModule));

					ctx.errorBufLocation += strlen(ctx.errorBufLocation);
				}
			}
		}*/
	}

	if(ctx.errorHandlerNested)
	{
		assert(ctx.errorHandlerActive);

		throw(new AnalyzerError());
	}

	ctx.errorCount++;

	if(ctx.errorCount == 100)
	{
		/*NULLC::SafeSprintf(ctx.errorBufLocation, ctx.errorBufSize - int(ctx.errorBufLocation - ctx.errorBuf), "ERROR: error limit reached");

		ctx.errorBufLocation += strlen(ctx.errorBufLocation);*/

		assert(ctx.errorHandlerActive);

		throw(new AnalyzerError());
	}
}

void Report(ExpressionContext ref ctx, SynBase ref source, char[] msg, auto ref[] args)
{
	//ReportAt(ctx, source, source.pos.begin, msg, args);

	io.out << msg << io.endl;

	for(arg in args)
	{
		if(arg.type == char[])
		{
			char[] str = arg;
			
			io.out << "argument: " << str << io.endl;
		}
		else if(arg.type == InplaceStr)
		{
			InplaceStr str = arg;
			
			io.out << "argument: ";

			for(int k = str.begin; k < str.end; k++)
				io.out << str.data[k];

			io.out << io.endl;
		}
	}

	for(int i = 0; i < 16 && i + source.begin.pos.pos < source.begin.pos.string.size; i++)
		io.out << source.begin.pos[i];
	io.out << io.endl;
}

ExprError ref ReportExpected(ExpressionContext ref ctx, SynBase ref source, TypeBase ref type, char[] msg, auto ref[] args)
{
	//ReportAt(ctx, source, source.pos.begin, msg, args);

	io.out << msg << io.endl;

	for(arg in args)
	{
		if(arg.type == char[])
		{
			char[] str = arg;
			
			io.out << "argument: " << str << io.endl;
		}
		else if(arg.type == InplaceStr)
		{
			InplaceStr str = arg;
			
			io.out << "argument: ";

			for(int k = str.begin; k < str.end; k++)
				io.out << str.data[k];

			io.out << io.endl;
		}
	}

	for(int i = 0; i < 16 && i + source.begin.pos.pos < source.begin.pos.string.size; i++)
		io.out << source.begin.pos[i];
	io.out << io.endl;

	return new ExprError(source, type);
}

void StopAt(ExpressionContext ref ctx, SynBase ref source, StringRef pos, char[] msg, auto ref[] args)
{
	ReportAt(ctx, source, pos, msg, args);

	assert(ctx.errorHandlerActive);

	io.out << msg << io.endl;

	for(arg in args)
	{
		if(arg.type == int)
		{
			int value = arg;

			io.out << "argument: " << value << io.endl;
		}
		else if(arg.type == char[])
		{
			char[] str = arg;

			io.out << "argument: " << str << io.endl;
		}
		else if(arg.type == InplaceStr)
		{
			InplaceStr str = arg;

			io.out << "argument: ";

			for(int k = str.begin; k < str.end; k++)
				io.out << str.data[k];

			io.out << io.endl;
		}
		else if(arg.type == StringRef)
		{
			StringRef str = arg;

			int strPos = 0;
			while(str.string[str.pos + strPos])
				io.out << str.string[str.pos + strPos++];

			io.out << io.endl;
		}
	}

	for(int i = 0; i < 16 && i + pos.pos < pos.string.size; i++)
		io.out << pos[i];
	io.out << io.endl;

	throw(new AnalyzerError());
}

void Stop(ExpressionContext ref ctx, SynBase ref source, char[] msg, auto ref[] args)
{
	StopAt(ctx, source, StringRef(source.pos), msg, args);
}

/*void OnMemoryLimitHit(void ref context)
{
	ExpressionContext ref ctx = (ExpressionContext ref)context;

	ctx.allocator.clear_limit();

	if(ctx.scope.ownerFunction)
		Stop(ctx, ctx.scope.ownerFunction.source, "ERROR: memory limit (%u) reached during compilation (analyze stage)", ctx.memoryLimit);
	else
		Stop(ctx, ctx.typeAutoRef.members.head.source, "ERROR: memory limit (%u) reached during compilation (analyze stage)", ctx.memoryLimit);
}*/

char ParseEscapeSequence(ExpressionContext ref ctx, SynBase ref source, StringRef str)
{
	assert(str[0] == '\\');

	switch(str[1])
	{
	case 'n':
		return '\n';
	case 'r':
		return '\r';
	case 't':
		return '\t';
	case '0':
		return '\0';
	case '\'':
		return '\'';
	case '\"':
		return '\"';
	case '\\':
		return '\\';
	}

	ReportAt(ctx, source, str, "ERROR: unknown escape sequence");

	return 0;
}

int ParseInteger(ExpressionContext ref ctx, StringRef str)
{
	int a = 0;

	int digit = str[0] - '0';

	while(digit >= 0 && digit < 10)
	{
		a = a * 10 + digit;
		str.advance();
		digit = str[0] - '0';
	}

	return a;
}

long ParseLong(ExpressionContext ref ctx, SynBase ref source, StringRef s, StringRef e, int base)
{
	long res = 0;

	for(StringRef p = s; p < e; p.advance())
	{
		int digit = ((p.curr() >= '0' && p.curr() <= '9') ? p.curr() - '0' : (p.curr() & ~0x20) - 'A' + 10);

		if(digit < 0 || digit >= base)
			ReportAt(ctx, source, p, "ERROR: digit %d is not allowed in base %d", digit, base);

		long prev = res;

		res = res * base + digit;

		if(res < prev)
			StopAt(ctx, source, s, "ERROR: overflow in integer constant");
	}

	return res;
}

double ParseDouble(ExpressionContext ref ctx, StringRef str)
{
	double integer = 0.0;

	int digit = str.curr() - '0';

	while(digit >= 0 && digit < 10)
	{
		integer = integer * 10.0 + digit;
		str.advance();
		digit = str.curr() - '0';
	}

	double fractional = 0.0;

	if(str.curr() == '.')
	{
		double power = 0.1;
		str.advance();

		digit = str.curr() - '0';

		while(digit >= 0 && digit < 10)
		{
			fractional = fractional + power * digit;
			power /= 10.0;
			str.advance();
			digit = str.curr() - '0';
		}
	}

	if(str.curr() == 'e' || str.curr() == 'E')
	{
		str.advance();

		assert(false, "not implemented");
		/*if(str.curr() == '-')
			return (integer + fractional) * pow(10.0, -ParseInteger(ctx, str + 1));
		else
			return (integer + fractional) * pow(10.0, ParseInteger(ctx, str));*/
	}

	return integer + fractional;
}

bool IsBinaryOp(SynUnaryOpType type)
{
	return type == SynUnaryOpType.SYN_UNARY_OP_BIT_NOT;
}

bool IsLogicalOp(SynUnaryOpType type)
{
	return type == SynUnaryOpType.SYN_UNARY_OP_LOGICAL_NOT;
}

bool IsBinaryOp(SynBinaryOpType type)
{
	switch(type)
	{
	case SynBinaryOpType.SYN_BINARY_OP_SHL:
	case SynBinaryOpType.SYN_BINARY_OP_SHR:
	case SynBinaryOpType.SYN_BINARY_OP_BIT_AND:
	case SynBinaryOpType.SYN_BINARY_OP_BIT_OR:
	case SynBinaryOpType.SYN_BINARY_OP_BIT_XOR:
		return true;
	default:
		break;
	}

	return false;
}

bool IsComparisonOp(SynBinaryOpType type)
{
	switch(type)
	{
	case SynBinaryOpType.SYN_BINARY_OP_LESS:
	case SynBinaryOpType.SYN_BINARY_OP_LESS_EQUAL:
	case SynBinaryOpType.SYN_BINARY_OP_GREATER:
	case SynBinaryOpType.SYN_BINARY_OP_GREATER_EQUAL:
	case SynBinaryOpType.SYN_BINARY_OP_EQUAL:
	case SynBinaryOpType.SYN_BINARY_OP_NOT_EQUAL:
		return true;
	default:
		break;
	}

	return false;
}

bool IsLogicalOp(SynBinaryOpType type)
{
	switch(type)
	{
	case SynBinaryOpType.SYN_BINARY_OP_LOGICAL_AND:
	case SynBinaryOpType.SYN_BINARY_OP_LOGICAL_OR:
	case SynBinaryOpType.SYN_BINARY_OP_LOGICAL_XOR:
		return true;
	default:
		break;
	}

	return false;
}

SynBinaryOpType GetBinaryOpType(SynModifyAssignType type)
{
	switch(type)
	{
	case SynModifyAssignType.SYN_MODIFY_ASSIGN_ADD:
		return SynBinaryOpType.SYN_BINARY_OP_ADD;
	case SynModifyAssignType.SYN_MODIFY_ASSIGN_SUB:
		return SynBinaryOpType.SYN_BINARY_OP_SUB;
	case SynModifyAssignType.SYN_MODIFY_ASSIGN_MUL:
		return SynBinaryOpType.SYN_BINARY_OP_MUL;
	case SynModifyAssignType.SYN_MODIFY_ASSIGN_DIV:
		return SynBinaryOpType.SYN_BINARY_OP_DIV;
	case SynModifyAssignType.SYN_MODIFY_ASSIGN_POW:
		return SynBinaryOpType.SYN_BINARY_OP_POW;
	case SynModifyAssignType.SYN_MODIFY_ASSIGN_MOD:
		return SynBinaryOpType.SYN_BINARY_OP_MOD;
	case SynModifyAssignType.SYN_MODIFY_ASSIGN_SHL:
		return SynBinaryOpType.SYN_BINARY_OP_SHL;
	case SynModifyAssignType.SYN_MODIFY_ASSIGN_SHR:
		return SynBinaryOpType.SYN_BINARY_OP_SHR;
	case SynModifyAssignType.SYN_MODIFY_ASSIGN_BIT_AND:
		return SynBinaryOpType.SYN_BINARY_OP_BIT_AND;
	case SynModifyAssignType.SYN_MODIFY_ASSIGN_BIT_OR:
		return SynBinaryOpType.SYN_BINARY_OP_BIT_OR;
	case SynModifyAssignType.SYN_MODIFY_ASSIGN_BIT_XOR:
		return SynBinaryOpType.SYN_BINARY_OP_BIT_XOR;
	default:
		break;
	}

	return SynBinaryOpType.SYN_BINARY_OP_UNKNOWN;
}

ScopeData ref NamedOrGlobalScopeFrom(ScopeData ref scope)
{
	if(!scope || scope.ownerNamespace || scope.scope == nullptr)
		return scope;

	return NamedOrGlobalScopeFrom(scope.scope);
}

TypeBase ref FindNextTypeFromScope(ScopeData ref scope)
{
	if(!scope)
		return nullptr;

	if(scope.ownerType)
		return scope.ownerType;

	return FindNextTypeFromScope(scope.scope);
}

int AllocateGlobalVariable(ExpressionContext ref ctx, SynBase ref source, int alignment, long size)
{
	assert((alignment & (alignment - 1)) == 0 && alignment <= 16);

	ScopeData ref scope = ctx.globalScope;

	scope.dataSize += GetAlignmentOffset(scope.dataSize, alignment);

	int result = int(scope.dataSize);

	if(result + size > (1 << 24))
		Stop(ctx, source, "ERROR: variable size limit exceeded");

	scope.dataSize += size;

	return result;
}

int AllocateVariableInScope(ExpressionContext ref ctx, SynBase ref source, int alignment, long size)
{
	assert((alignment & (alignment - 1)) == 0 && alignment <= 16);

	ScopeData ref scope = ctx.scope;

	while(scope.scope)
	{
		// Don't let allocations escape the temporary scope
		if(scope.type == ScopeType.SCOPE_TEMPORARY)
		{
			return 0;
		}

		if(scope.ownerFunction)
		{
			scope.dataSize += GetAlignmentOffset(scope.dataSize, alignment);

			int result = int(scope.dataSize);

			if(result + size > (1 << 24))
				Stop(ctx, source, "ERROR: variable size limit exceeded");

			scope.dataSize += size;

			scope.ownerFunction.stackSize = scope.dataSize;

			return result;
		}

		if(scope.ownerType)
		{
			scope.dataSize += GetAlignmentOffset(scope.dataSize, alignment);

			int result = int(scope.dataSize);

			if(result + size > (1 << 24))
				Stop(ctx, source, "ERROR: variable size limit exceeded");

			scope.dataSize += size;

			scope.ownerType.size = scope.dataSize;

			return result;
		}

		scope = scope.scope;
	}

	assert(scope == ctx.globalScope);

	return AllocateGlobalVariable(ctx, source, alignment, size);
}

int AllocateVariableInScope(ExpressionContext ref ctx, SynBase ref source, int alignment, TypeBase ref type)
{
	if(TypeClass ref typeClass = getType with<TypeClass>(type))
	{
		if(!typeClass.completed)
			Stop(ctx, source, "ERROR: type '%.*s' is not fully defined", FMT_ISTR(type.name));
	}

	return AllocateVariableInScope(ctx, source, alignment, type.size);
}

int AllocateArgumentInScope(ExpressionContext ref ctx, SynBase ref source, int alignment, TypeBase ref type)
{
	if(TypeClass ref typeClass = getType with<TypeClass>(type))
	{
		if(!typeClass.completed)
			Stop(ctx, source, "ERROR: type '%.*s' is not fully defined", FMT_ISTR(type.name));
	}

	return AllocateVariableInScope(ctx, source, alignment, type.size >= 4 ? type.size : 4);
}

NamespaceData ref FindNamespaceInCurrentScope(ExpressionContext ref ctx, InplaceStr name)
{
	ArrayView<NamespaceData ref> namespaces;

	if(NamespaceData ref ns = ctx.GetCurrentNamespace(ctx.scope))
		namespaces = ns.children;
	else
		namespaces = ctx.globalNamespaces;

	for(int i = 0; i < namespaces.size(); i++)
	{
		if(namespaces[i].name.name == name)
			return namespaces[i];
	}

	return nullptr;
}

bool CheckVariableConflict(ExpressionContext ref ctx, SynBase ref source, InplaceStr name)
{
	if(ctx.typeMap.find(name.hash()))
	{
		Report(ctx, source, "ERROR: name '%.*s' is already taken for a type", FMT_ISTR(name));
		return true;
	}

	if(VariableData ref ref variable = ctx.variableMap.find(name.hash()))
	{
		if((*variable).scope == ctx.scope)
		{
			Report(ctx, source, "ERROR: name '%.*s' is already taken for a variable in current scope", FMT_ISTR(name));
			return true;
		}
	}

	if(FunctionData ref ref functions = ctx.functionMap.find(name.hash()))
	{
		if((*functions).scope == ctx.scope)
		{
			Report(ctx, source, "ERROR: name '%.*s' is already taken for a function", FMT_ISTR(name));
			return true;
		}
	}

	if(FindNamespaceInCurrentScope(ctx, name))
	{
		Report(ctx, source, "ERROR: name '%.*s' is already taken for a namespace", FMT_ISTR(name));
		return true;
	}

	return false;
}

void CheckFunctionConflict(ExpressionContext ref ctx, SynBase ref source, InplaceStr name)
{
	if(FunctionData ref ref function = ctx.functionMap.find(name.hash()))
	{
		if((*function).isInternal)
			Report(ctx, source, "ERROR: function '%.*s' is reserved", FMT_ISTR(name));
	}
}

void CheckTypeConflict(ExpressionContext ref ctx, SynBase ref source, InplaceStr name)
{
	if(VariableData ref ref variable = ctx.variableMap.find(name.hash()))
	{
		if((*variable).scope == ctx.scope)
			Report(ctx, source, "ERROR: name '%.*s' is already taken for a variable in current scope", FMT_ISTR(name));
	}

	if(FindNamespaceInCurrentScope(ctx, name))
		Report(ctx, source, "ERROR: name '%.*s' is already taken for a namespace", FMT_ISTR(name));
}

void CheckNamespaceConflict(ExpressionContext ref ctx, SynBase ref source, NamespaceData ref ns)
{
	if(ctx.typeMap.find(ns.fullNameHash))
		Report(ctx, source, "ERROR: name '%.*s' is already taken for a type", FMT_ISTR(ns.name.name));

	if(VariableData ref ref variable = ctx.variableMap.find(ns.nameHash))
	{
		if((*variable).scope == ctx.scope)
			Report(ctx, source, "ERROR: name '%.*s' is already taken for a variable in current scope", FMT_ISTR(ns.name.name));
	}

	if(FunctionData ref ref functions = ctx.functionMap.find(ns.nameHash))
	{
		if((*functions).scope == ctx.scope)
			Report(ctx, source, "ERROR: name '%.*s' is already taken for a function", FMT_ISTR(ns.name.name));
	}
}

bool IsArgumentVariable(FunctionData ref function, VariableData ref data)
{
	for(VariableHandle ref curr = function.argumentVariables.head; curr; curr = curr.next)
	{
		if(data == curr.variable)
			return true;
	}

	if(data == function.contextArgument)
		return true;

	return false;
}

bool IsLookupOnlyVariable(ExpressionContext ref ctx, VariableData ref variable)
{
	FunctionData ref currentFunction = ctx.GetCurrentFunction(ctx.scope);
	FunctionData ref variableFunctionOwner = ctx.GetFunctionOwner(variable.scope);

	if(currentFunction && variableFunctionOwner)
	{
		if(variableFunctionOwner != currentFunction)
			return true;

		if(currentFunction.isCoroutine && !IsArgumentVariable(currentFunction, variable))
			return true;
	}

	return false;
}

VariableData ref AllocateClassMember(ExpressionContext ref ctx, SynBase ref source, int alignment, TypeBase ref type, InplaceStr name, bool readonly, int uniqueId)
{
	if(alignment == 0)
		alignment = type.alignment;

	int offset = AllocateVariableInScope(ctx, source, alignment, type);

	assert(!type.isGeneric);

	VariableData ref variable = new VariableData(source, ctx.scope, alignment, type, new SynIdentifier(name), offset, uniqueId);

	variable.isReadonly = readonly;

	ctx.AddVariable(variable, true);

	return variable;
}

VariableData ref AllocateTemporary(ExpressionContext ref ctx, SynBase ref source, TypeBase ref type)
{
	InplaceStr name = GetTemporaryName(ctx, ctx.unnamedVariableCount++, nullptr);

	assert(!type.isGeneric);

	VariableData ref variable = new VariableData(source, ctx.scope, type.alignment, type, new SynIdentifier(name), 0, ctx.uniqueVariableId++);

	if (IsLookupOnlyVariable(ctx, variable))
		variable.lookupOnly = true;

	variable.isAlloca = true;
	variable.offset = -1;

	ctx.AddVariable(variable, false);

	return variable;
}

void FinalizeAlignment(TypeStruct ref type)
{
	int maximumAlignment = 0;

	// Additional padding may apply to preserve the alignment of members
	for(MemberHandle ref curr = type.members.head; curr; curr = curr.next)
	{
		maximumAlignment = maximumAlignment > curr.variable.alignment ? maximumAlignment : curr.variable.alignment;

		if(curr.variable.type.hasPointers)
			type.hasPointers = true;
	}

	// If explicit alignment is not specified, then class must be aligned to the maximum alignment of the members
	if(type.alignment == 0)
		type.alignment = maximumAlignment;

	// In NULLC, all classes have sizes multiple of 4, so add additional padding if necessary
	maximumAlignment = type.alignment < 4 ? 4 : type.alignment;

	if(type.size % maximumAlignment != 0)
	{
		type.padding = maximumAlignment - (type.size % maximumAlignment);

		type.size += type.padding;
		type.typeScope.dataSize += type.padding;
	}
}

FunctionData ref ImplementPrototype(ExpressionContext ref ctx, FunctionData ref function)
{
	ArrayView<FunctionData ref> functions = ctx.scope.functions;

	for(int i = 0, e = functions.count; i < e; i++)
	{
		FunctionData ref curr = functions.data[i];

		// Skip current function
		if(curr == function)
			continue;

		// TODO: generic function list

		if(curr.isPrototype && curr.type == function.type && curr.name.name == function.name.name)
		{
			curr.implementation = function;

			ctx.HideFunction(curr);

			return curr;
		}
	}

	if(function.scope.ownerType)
	{
		auto curr = ctx.functionMap.first(function.nameHash);

		while(curr)
		{
			// Skip current function
			if(curr.value == function)
			{
				curr = ctx.functionMap.next(curr);
				continue;
			}

			if(curr.value.isPrototype && /*SameGenerics(curr.value.generics, function.generics) &&*/ curr.value.type == function.type)
			{
				curr.value.implementation = function;

				ctx.HideFunction(curr.value);

				return curr.value;
			}

			curr = ctx.functionMap.next(curr);
		}
	}

	return nullptr;
}

bool SameGenerics(RefList<MatchData> a, RefList<TypeHandle> b)
{
	if(a.size() != b.size())
		return false;

	MatchData ref ca = a.head;
	TypeHandle ref cb = b.head;

	for(; ca && cb; {ca = ca.next; cb = cb.next; })
	{
		if(ca.type != cb.type)
			return false;
	}

	return true;
}

bool SameGenerics(RefList<MatchData> a, RefList<MatchData> b)
{
	if(a.size() != b.size())
		return false;

	MatchData ref ca = a.head;
	MatchData ref cb = b.head;

	for(; ca && cb; { ca = ca.next; cb = cb.next; })
	{
		if(ca.type != cb.type)
			return false;
	}

	return true;
}

bool SameArguments(TypeFunction ref a, TypeFunction ref b)
{
	TypeHandle ref ca = a.arguments.head;
	TypeHandle ref cb = b.arguments.head;

	for(; ca && cb; { ca = ca.next; cb = cb.next; })
	{
		if(ca.type != cb.type)
			return false;
	}

	return ca == cb;
}

FunctionData ref CheckUniqueness(ExpressionContext ref ctx, FunctionData ref function)
{
   auto curr = ctx.functionMap.first(function.nameHash);

	while(curr)
	{
		// Skip current function
		if(curr.value == function)
		{
			curr = ctx.functionMap.next(curr);
			continue;
		}

		if(SameGenerics(curr.value.generics, function.generics) && curr.value.type == function.type && curr.value.scope.ownerType == function.scope.ownerType)
			return curr.value;

		curr = ctx.functionMap.next(curr);
	}

	return nullptr;
}

int GetDerivedFromDepth(TypeClass ref type, TypeClass ref target)
{
	int depth = 0;

	while(type)
	{
		if(target == type)
			return depth;

		depth++;

		type = type.baseClass;
	}

	return -1;
}

bool IsDerivedFrom(TypeClass ref type, TypeClass ref target)
{
	return GetDerivedFromDepth(type, target) != -1;
}

ExprBase ref EvaluateExpression(ExpressionContext ref ctx, SynBase ref source, ExprBase ref expression)
{
	ExpressionEvalContext evalCtx = ExpressionEvalContext(ctx);

	if(ctx.errorBuf)
	{
		evalCtx.errorBuf = ctx.errorBuf;
	}

	evalCtx.globalFrame = new StackFrame(nullptr);
	evalCtx.stackFrames.push_back(evalCtx.globalFrame);

	ExprBase ref result = Evaluate(evalCtx, expression);

	if(evalCtx.errorCritical)
	{
		/*if(ctx.errorBuf && ctx.errorBufSize)
		{
			if(ctx.errorCount == 0)
			{
				ctx.errorPos = source.pos.begin;
				ctx.errorBufLocation = ctx.errorBuf;
			}

			const char ref messageStart = ctx.errorBufLocation;

			ctx.errorBufLocation += strlen(ctx.errorBufLocation);

			const char ref messageEnd = ctx.errorBufLocation;

			ctx.errorInfo.push_back(new ErrorInfo(messageStart, messageEnd, source.begin, source.end, ctx.errorPos));

			if(const char ref code = FindModuleCodeWithSourceLocation(ctx, ctx.errorPos))
			{
				AddErrorLocationInfo(code, ctx.errorPos, ctx.errorBufLocation, ctx.errorBufSize - int(ctx.errorBufLocation - ctx.errorBuf));

				ctx.errorBufLocation += strlen(ctx.errorBufLocation);
			}
		}*/

		throw(new AnalyzerError());
	}
	else if(!result)
	{
		// Remove non-critical error from buffer
		if(ctx.errorBuf)
			evalCtx.errorBuf.clear();
	}

	return result;
}

ExprBase ref AnalyzeNumber(ExpressionContext ref ctx, SynNumber ref syntax);
ExprBase ref AnalyzeExpression(ExpressionContext ref ctx, SynBase ref syntax);
ExprBase ref AnalyzeStatement(ExpressionContext ref ctx, SynBase ref syntax);
ExprBlock ref AnalyzeBlock(ExpressionContext ref ctx, SynBlock ref syntax, bool createScope);
ExprAliasDefinition ref AnalyzeTypedef(ExpressionContext ref ctx, SynTypedef ref syntax);
ExprBase ref AnalyzeClassDefinition(ExpressionContext ref ctx, SynClassDefinition ref syntax, TypeGenericClassProto ref proto, RefList<TypeHandle> generics);
void AnalyzeClassBaseElements(ExpressionContext ref ctx, ExprClassDefinition ref classDefinition, SynClassElements ref syntax);
void AnalyzeClassFunctionElements(ExpressionContext ref ctx, ExprClassDefinition ref classDefinition, SynClassElements ref syntax);
void AnalyzeClassElements(ExpressionContext ref ctx, ExprClassDefinition ref classDefinition, SynClassElements ref syntax);
ExprBase ref AnalyzeFunctionDefinition(ExpressionContext ref ctx, SynFunctionDefinition ref syntax, FunctionData ref genericProto, TypeFunction ref instance, TypeBase ref instanceParent, RefList<MatchData> matches, bool createAccess, bool isLocal, bool checkParent);
ExprBase ref AnalyzeShortFunctionDefinition(ExpressionContext ref ctx, SynShortFunctionDefinition ref syntax, FunctionData ref genericProto, TypeFunction ref argumentType);
ExprBase ref AnalyzeShortFunctionDefinition(ExpressionContext ref ctx, SynShortFunctionDefinition ref syntax, TypeBase ref type, ArrayView<ArgumentData> arguments, RefList<MatchData> aliases);
ExprBase ref AnalyzeShortFunctionDefinition(ExpressionContext ref ctx, SynShortFunctionDefinition ref syntax, FunctionData ref genericProto, TypeFunction ref argumentType, RefList<MatchData> argCasts, ArrayView<ArgumentData> argData);

ExprBase ref CreateTypeidMemberAccess(ExpressionContext ref ctx, SynBase ref source, TypeBase ref type, SynIdentifier ref member);

ExprBase ref CreateBinaryOp(ExpressionContext ref ctx, SynBase ref source, SynBinaryOpType op, ExprBase ref lhs, ExprBase ref rhs);

ExprBase ref CreateVariableAccess(ExpressionContext ref ctx, SynBase ref source, VariableData ref variable, bool handleReference);
ExprBase ref CreateVariableAccess(ExpressionContext ref ctx, SynBase ref source, RefList<SynIdentifier> path, InplaceStr name, bool allowInternal);

ExprBase ref CreateGetAddress(ExpressionContext ref ctx, SynBase ref source, ExprBase ref value);

ExprBase ref CreateMemberAccess(ExpressionContext ref ctx, SynBase ref source, ExprBase ref value, SynIdentifier ref member, bool allowFailure);

ExprBase ref CreateAssignment(ExpressionContext ref ctx, SynBase ref source, ExprBase ref lhs, ExprBase ref rhs);

ExprBase ref CreateReturn(ExpressionContext ref ctx, SynBase ref source, ExprBase ref result);

bool AssertResolvableType(ExpressionContext ref ctx, SynBase ref source, TypeBase ref type, bool allowGeneric);
bool AssertResolvableTypeLiteral(ExpressionContext ref ctx, SynBase ref source, ExprBase ref expr);
bool AssertValueExpression(ExpressionContext ref ctx, SynBase ref source, ExprBase ref expr);

InplaceStr GetTypeConstructorName(TypeClass ref classType);
bool GetTypeConstructorFunctions(ExpressionContext ref ctx, TypeBase ref type, bool noArguments, vector<FunctionData ref> ref functions);
ExprBase ref CreateConstructorAccess(ExpressionContext ref ctx, SynBase ref source, ArrayView<FunctionData ref> functions, ExprBase ref context);
ExprBase ref CreateConstructorAccess(ExpressionContext ref ctx, SynBase ref source, TypeBase ref type, bool noArguments, ExprBase ref context);
bool HasDefaultConstructor(ExpressionContext ref ctx, SynBase ref source, TypeBase ref type);
ExprBase ref CreateDefaultConstructorCall(ExpressionContext ref ctx, SynBase ref source, TypeBase ref type, ExprBase ref pointer);
void CreateDefaultConstructorCode(ExpressionContext ref ctx, SynBase ref source, TypeClass ref classType, RefList<ExprBase> ref expressions);

InplaceStr GetTemporaryFunctionName(ExpressionContext ref ctx);
InplaceStr GetDefaultArgumentWrapperFunctionName(ExpressionContext ref ctx, FunctionData ref function, InplaceStr argumentName);
TypeBase ref CreateFunctionContextType(ExpressionContext ref ctx, SynBase ref source, InplaceStr functionName);
ExprVariableDefinition ref CreateFunctionContextArgument(ExpressionContext ref ctx, SynBase ref source, FunctionData ref function);
VariableData ref CreateFunctionContextVariable(ExpressionContext ref ctx, SynBase ref source, FunctionData ref function, FunctionData ref prototype);
ExprVariableDefinition ref CreateFunctionContextVariableDefinition(ExpressionContext ref ctx, SynBase ref source, FunctionData ref function, FunctionData ref prototype, VariableData ref context);
ExprBase ref CreateFunctionContextAccess(ExpressionContext ref ctx, SynBase ref source, FunctionData ref function);
ExprBase ref CreateFunctionAccess(ExpressionContext ref ctx, SynBase ref source, hashmap_node<int, FunctionData ref> ref function, ExprBase ref context);
ExprBase ref CreateFunctionCoroutineStateUpdate(ExpressionContext ref ctx, SynBase ref source, FunctionData ref function, int state);

TypeBase ref MatchGenericType(ExpressionContext ref ctx, SynBase ref source, TypeBase ref matchType, TypeBase ref argType, RefList<MatchData> ref aliases, bool strict);
TypeBase ref ResolveGenericTypeAliases(ExpressionContext ref ctx, SynBase ref source, TypeBase ref type, RefList<MatchData> aliases);

FunctionValue SelectBestFunction(ExpressionContext ref ctx, SynBase ref source, ArrayView<FunctionValue> functions, RefList<TypeHandle> generics, ArrayView<ArgumentData> arguments, vector<int> ref ratings);
FunctionValue CreateGenericFunctionInstance(ExpressionContext ref ctx, SynBase ref source, FunctionValue proto, RefList<TypeHandle> generics, ArrayView<ArgumentData> arguments, bool standalone);
void GetNodeFunctions(ExpressionContext ref ctx, SynBase ref source, ExprBase ref function, vector<FunctionValue> ref functions);
void ReportOnFunctionSelectError(ExpressionContext ref ctx, SynBase ref source, string ref errorBuf, int messageStart, ArrayView<FunctionValue> functions);
void ReportOnFunctionSelectError(ExpressionContext ref ctx, SynBase ref source, string ref errorBuf, int messageStart, InplaceStr functionName, ArrayView<FunctionValue> functions, RefList<TypeHandle> generics, ArrayView<ArgumentData> arguments, ArrayView<int> ratings, int bestRating, bool showInstanceInfo);
ExprBase ref CreateFunctionCall0(ExpressionContext ref ctx, SynBase ref source, InplaceStr name, bool allowFailure, bool allowInternal, bool allowFastLookup);
ExprBase ref CreateFunctionCall1(ExpressionContext ref ctx, SynBase ref source, InplaceStr name, ExprBase ref arg0, bool allowFailure, bool allowInternal, bool allowFastLookup);
ExprBase ref CreateFunctionCall2(ExpressionContext ref ctx, SynBase ref source, InplaceStr name, ExprBase ref arg0, ExprBase ref arg1, bool allowFailure, bool allowInternal, bool allowFastLookup);
ExprBase ref CreateFunctionCall3(ExpressionContext ref ctx, SynBase ref source, InplaceStr name, ExprBase ref arg0, ExprBase ref arg1, ExprBase ref arg2, bool allowFailure, bool allowInternal, bool allowFastLookup);
ExprBase ref CreateFunctionCall4(ExpressionContext ref ctx, SynBase ref source, InplaceStr name, ExprBase ref arg0, ExprBase ref arg1, ExprBase ref arg2, ExprBase ref arg3, bool allowFailure, bool allowInternal, bool allowFastLookup);
ExprBase ref CreateFunctionCallByName(ExpressionContext ref ctx, SynBase ref source, InplaceStr name, ArrayView<ArgumentData> arguments, bool allowFailure, bool allowInternal, bool allowFastLookup);
ExprBase ref CreateFunctionCall(ExpressionContext ref ctx, SynBase ref source, ExprBase ref value, ArrayView<ArgumentData> arguments, bool allowFailure);
ExprBase ref CreateFunctionCall(ExpressionContext ref ctx, SynBase ref source, ExprBase ref value, RefList<TypeHandle> generics, SynCallArgument ref argumentHead, bool allowFailure);
ExprBase ref CreateFunctionCallOverloaded(ExpressionContext ref ctx, SynBase ref source, ExprBase ref value, ArrayView<FunctionValue> functions, RefList<TypeHandle> generics, SynCallArgument ref argumentHead, bool allowFailure);
ExprBase ref CreateFunctionCallFinal(ExpressionContext ref ctx, SynBase ref source, ExprBase ref value, ArrayView<FunctionValue> functions, RefList<TypeHandle> generics, ArrayView<ArgumentData> arguments, bool allowFailure);
ExprBase ref CreateObjectAllocation(ExpressionContext ref ctx, SynBase ref source, TypeBase ref type);
ExprBase ref CreateArrayAllocation(ExpressionContext ref ctx, SynBase ref source, TypeBase ref type, ExprBase ref count);

bool RestoreParentTypeScope(ExpressionContext ref ctx, SynBase ref source, TypeBase ref parentType);
ExprBase ref CreateFunctionDefinition(ExpressionContext ref ctx, SynBase ref source, bool prototype, bool isCoroutine, TypeBase ref parentType, bool accessor, TypeBase ref returnType, bool isOperator, SynIdentifier ref name, RefList<SynIdentifier> aliases, RefList<SynFunctionArgument> arguments, RefList<SynBase> expressions, FunctionData ref genericProto, TypeFunction ref instance, RefList<MatchData> matches);

FunctionValue GetFunctionForType(ExpressionContext ref ctx, SynBase ref source, ExprBase ref value, TypeFunction ref type)
{
	// Collect a set of available functions
	vector<FunctionValue> functions;

	GetNodeFunctions(ctx, source, value, functions);

	if(!functions.empty())
	{
		FunctionValue bestMatch;
		TypeFunction ref bestMatchTarget = nullptr;

		FunctionValue bestGenericMatch;
		TypeFunction ref bestGenericMatchTarget = nullptr;

		for(int i = 0; i < functions.size(); i++)
		{
			TypeFunction ref functionType = functions[i].function.type;

			if(type.arguments.size() != functionType.arguments.size())
				continue;

			if(type.isGeneric)
			{
				RefList<MatchData> aliases;

				TypeBase ref returnType = MatchGenericType(ctx, source, type.returnType, functionType.returnType, aliases, true);
				RefList<TypeHandle> arguments;

				for(TypeHandle ref lhs = type.arguments.head, rhs = functionType.arguments.head; lhs && rhs; { lhs = lhs.next; rhs = rhs.next; })
				{
					TypeBase ref match = MatchGenericType(ctx, source, lhs.type, rhs.type, aliases, true);

					if(match && !match.isGeneric)
						arguments.push_back(new TypeHandle(match));
				}

				if(returnType && arguments.size() == type.arguments.size())
				{
					if(bestGenericMatch)
						return FunctionValue();

					bestGenericMatch = functions[i];
					bestGenericMatchTarget = ctx.GetFunctionType(source, returnType, arguments);
				}
			}
			else if(functionType.isGeneric)
			{
				int matches = 0;

				RefList<MatchData> aliases;

				for(TypeHandle ref lhs = functionType.arguments.head, rhs = type.arguments.head; lhs && rhs; { lhs = lhs.next; rhs = rhs.next; })
				{
					TypeBase ref match = MatchGenericType(ctx, source, lhs.type, rhs.type, aliases, true);

					if(match && !match.isGeneric)
						matches++;
				}

				if(functionType.returnType == ctx.typeAuto || functionType.returnType == type.returnType)
					matches++;

				if(matches == type.arguments.size() + 1)
				{
					if(bestGenericMatch)
						return FunctionValue();

					bestGenericMatch = functions[i];
					bestGenericMatchTarget = type;
				}
			}
			else if(functionType == type)
			{
				if(bestMatch)
					return FunctionValue();

				bestMatch = functions[i];
				bestMatchTarget = type;
			}
		}

		FunctionValue bestOverload = bestMatch ? bestMatch : bestGenericMatch;
		TypeFunction ref bestTarget = bestMatch ? bestMatchTarget : bestGenericMatchTarget;

		if(bestOverload)
		{
			vector<ArgumentData> arguments;

			for(TypeHandle ref curr = bestTarget.arguments.head; curr; curr = curr.next)
				arguments.push_back(ArgumentData(source, false, nullptr, curr.type, nullptr));

			FunctionData ref function = bestOverload.function;

			if(ctx.IsGenericFunction(function))
				bestOverload = CreateGenericFunctionInstance(ctx, source, bestOverload, RefList<TypeHandle>(), ArrayView<ArgumentData>(arguments), false);

			if(bestOverload)
			{
				if(bestTarget.returnType == ctx.typeAuto)
					bestTarget = ctx.GetFunctionType(source, bestOverload.function.type.returnType, bestTarget.arguments);

				if(bestOverload.function.type == bestTarget)
					return bestOverload;
			}
		}
	}

	return FunctionValue();
}

ExprBase ref CreateSequence(ExpressionContext ref ctx, SynBase ref source, ExprBase ref first, ExprBase ref second)
{
	vector<ExprBase ref> expressions;

	expressions.push_back(first);
	expressions.push_back(second);

	return new ExprSequence(source, second.type, ArrayView<ExprBase ref>(expressions));
}

ExprBase ref CreateSequence(ExpressionContext ref ctx, SynBase ref source, ExprBase ref first, ExprBase ref second, ExprBase ref third)
{
	vector<ExprBase ref> expressions;

	expressions.push_back(first);
	expressions.push_back(second);
	expressions.push_back(third);

	return new ExprSequence(source, third.type, ArrayView<ExprBase ref>(expressions));
}

ExprBase ref CreateLiteralCopy(ExpressionContext ref ctx, SynBase ref source, ExprBase ref value)
{
	if(ExprBoolLiteral ref node = getType with<ExprBoolLiteral>(value))
		return new ExprBoolLiteral(source, node.type, node.value);

	if(ExprCharacterLiteral ref node = getType with<ExprCharacterLiteral>(value))
		return new ExprCharacterLiteral(source, node.type, node.value);

	if(ExprIntegerLiteral ref node = getType with<ExprIntegerLiteral>(value))
		return new ExprIntegerLiteral(source, node.type, node.value);

	if(ExprRationalLiteral ref node = getType with<ExprRationalLiteral>(value))
		return new ExprRationalLiteral(source, node.type, node.value);

	Stop(ctx, source, "ERROR: unknown literal type");

	return nullptr;
}

ExprBase ref CreateFunctionPointer(ExpressionContext ref ctx, SynBase ref source, ExprFunctionDefinition ref definition, bool hideFunction)
{
	if(hideFunction)
	{
		ctx.HideFunction(definition.function);

		definition.function.isHidden = true;
	}

	vector<ExprBase ref> expressions;

	expressions.push_back(definition);

	if(definition.contextVariableDefinition)
		expressions.push_back(definition.contextVariableDefinition);

	expressions.push_back(new ExprFunctionAccess(ctx.MakeInternal(source), definition.function.type, definition.function, CreateFunctionContextAccess(ctx, ctx.MakeInternal(source), definition.function)));

	return new ExprSequence(source, definition.function.type, ArrayView<ExprBase ref>(expressions));
}

ExprBase ref CreateCast(ExpressionContext ref ctx, SynBase ref source, ExprBase ref value, TypeBase ref type, bool isFunctionArgument)
{
	if(isType with<TypeError>(value.type))
		return new ExprTypeCast(source, type, value, ExprTypeCastCategory.EXPR_CAST_REINTERPRET);

	// When function is used as value, hide its visibility immediately after use
	if(ExprFunctionDefinition ref definition = getType with<ExprFunctionDefinition>(value))
		return CreateCast(ctx, source, CreateFunctionPointer(ctx, source, definition, true), type, isFunctionArgument);

	if(value.type == type)
	{
		AssertValueExpression(ctx, source, value);

		return value;
	}

	if(TypeFunction ref target = getType with<TypeFunction>(type))
	{
		if(FunctionValue function = GetFunctionForType(ctx, source, value, target))
		{
			if(isType with<TypeAutoRef>(function.context.type))
				Stop(ctx, source, "ERROR: can't convert dynamic function set to '%.*s'", FMT_ISTR(type.name));

			ExprBase ref access = new ExprFunctionAccess(function.source, type, function.function, function.context);

			if(isType with<ExprFunctionDefinition>(value) || isType with<ExprGenericFunctionPrototype>(value))
				return CreateSequence(ctx, source, value, access);

			return access;
		}

		if(value.type.isGeneric)
			Stop(ctx, source, "ERROR: can't resolve generic type '%.*s' instance for '%.*s'", FMT_ISTR(value.type.name), FMT_ISTR(type.name));
	}

	if(!AssertValueExpression(ctx, source, value))
		return new ExprTypeCast(source, type, value, ExprTypeCastCategory.EXPR_CAST_REINTERPRET);

	if(ctx.IsNumericType(value.type) && ctx.IsNumericType(type))
		return new ExprTypeCast(source, type, value, ExprTypeCastCategory.EXPR_CAST_NUMERICAL);

	if(type == ctx.typeBool)
	{
		if(isType with<TypeRef>(value.type))
			return new ExprTypeCast(source, type, value, ExprTypeCastCategory.EXPR_CAST_PTR_TO_BOOL);

		if(isType with<TypeUnsizedArray>(value.type))
			return new ExprTypeCast(source, type, value, ExprTypeCastCategory.EXPR_CAST_UNSIZED_TO_BOOL);

		if(isType with<TypeFunction>(value.type))
			return new ExprTypeCast(source, type, value, ExprTypeCastCategory.EXPR_CAST_FUNCTION_TO_BOOL);
	}

	if(value.type == ctx.typeNullPtr)
	{
		// nullptr to type ref conversion
		if(isType with<TypeRef>(type))
			return new ExprTypeCast(source, type, value, ExprTypeCastCategory.EXPR_CAST_NULL_TO_PTR);

		// nullptr to auto ref conversion
		if(type == (TypeBase ref)(ctx.typeAutoRef))
			return new ExprTypeCast(source, type, value, ExprTypeCastCategory.EXPR_CAST_NULL_TO_AUTO_PTR);

		// nullptr to type[] conversion
		if(isType with<TypeUnsizedArray>(type))
			return new ExprTypeCast(source, type, value, ExprTypeCastCategory.EXPR_CAST_NULL_TO_UNSIZED);

		// nullptr to auto[] conversion
		if(type == (TypeBase ref)(ctx.typeAutoArray))
			return new ExprTypeCast(source, type, value, ExprTypeCastCategory.EXPR_CAST_NULL_TO_AUTO_ARRAY);

		// nullptr to function type conversion
		if(isType with<TypeFunction>(type))
			return new ExprTypeCast(source, type, value, ExprTypeCastCategory.EXPR_CAST_NULL_TO_FUNCTION);
	}

	if(TypeUnsizedArray ref target = getType with<TypeUnsizedArray>(type))
	{
		// type[N] to type[] conversion
		if(TypeArray ref valueType = getType with<TypeArray>(value.type))
		{
			if(target.subType == valueType.subType)
			{
				if(ExprVariableAccess ref node = getType with<ExprVariableAccess>(value))
				{
					ExprBase ref address = new ExprGetAddress(source, ctx.GetReferenceType(value.type), new VariableHandle(node.source, node.variable));

					return new ExprTypeCast(source, type, address, ExprTypeCastCategory.EXPR_CAST_ARRAY_PTR_TO_UNSIZED);
				}
				else if(ExprDereference ref node = getType with<ExprDereference>(value))
				{
					return new ExprTypeCast(source, type, node.value, ExprTypeCastCategory.EXPR_CAST_ARRAY_PTR_TO_UNSIZED);
				}

				// Allocate storage in heap and copy literal data into it
				SynBase ref sourceInternal = ctx.MakeInternal(source);

				VariableData ref storage = AllocateTemporary(ctx, sourceInternal, ctx.GetReferenceType(valueType));

				ExprBase ref alloc = CreateObjectAllocation(ctx, sourceInternal, valueType);

				ExprBase ref definition = new ExprVariableDefinition(sourceInternal, ctx.typeVoid, new VariableHandle(nullptr, storage), CreateAssignment(ctx, sourceInternal, CreateVariableAccess(ctx, sourceInternal, storage, false), alloc));

				ExprBase ref assignment = CreateAssignment(ctx, sourceInternal, new ExprDereference(sourceInternal, valueType, CreateVariableAccess(ctx, sourceInternal, storage, false)), value);

				ExprBase ref result = new ExprTypeCast(sourceInternal, type, CreateVariableAccess(ctx, sourceInternal, storage, false), ExprTypeCastCategory.EXPR_CAST_ARRAY_PTR_TO_UNSIZED);

				return CreateSequence(ctx, source, definition, assignment, result);
			}
		}
	}

	if(TypeRef ref target = getType with<TypeRef>(type))
	{
		if(TypeRef ref valueType = getType with<TypeRef>(value.type))
		{
			// type[N] ref to type[] ref conversion
			if(isType with<TypeUnsizedArray>(target.subType) && isType with<TypeArray>(valueType.subType))
			{
				TypeUnsizedArray ref targetSub = getType with<TypeUnsizedArray>(target.subType);
				TypeArray ref sourceSub = getType with<TypeArray>(valueType.subType);

				if(targetSub.subType == sourceSub.subType)
				{
					SynBase ref sourceInternal = ctx.MakeInternal(source);

					VariableData ref storage = AllocateTemporary(ctx, sourceInternal, targetSub);

					ExprBase ref assignment = CreateAssignment(ctx, sourceInternal, CreateVariableAccess(ctx, sourceInternal, storage, false), new ExprTypeCast(sourceInternal, targetSub, value, ExprTypeCastCategory.EXPR_CAST_ARRAY_PTR_TO_UNSIZED));

					ExprBase ref definition = new ExprVariableDefinition(sourceInternal, ctx.typeVoid, new VariableHandle(nullptr, storage), assignment);

					ExprBase ref result = CreateGetAddress(ctx, sourceInternal, CreateVariableAccess(ctx, sourceInternal, storage, false));

					return CreateSequence(ctx, source, definition, result);
				}
			}

			if(isType with<TypeClass>(target.subType) && isType with<TypeClass>(valueType.subType))
			{
				TypeClass ref targetClass = getType with<TypeClass>(target.subType);
				TypeClass ref valueClass = getType with<TypeClass>(valueType.subType);

				if(IsDerivedFrom(valueClass, targetClass))
					return new ExprTypeCast(source, type, value, ExprTypeCastCategory.EXPR_CAST_REINTERPRET);

				if(IsDerivedFrom(targetClass, valueClass))
				{
					ExprBase ref untyped = new ExprTypeCast(source, ctx.GetReferenceType(ctx.typeVoid), value, ExprTypeCastCategory.EXPR_CAST_REINTERPRET);
					ExprBase ref typeID = new ExprTypeLiteral(source, ctx.typeTypeID, targetClass);

					ExprBase ref checked = CreateFunctionCall2(ctx, source, InplaceStr("assert_derived_from_base"), untyped, typeID, false, false, true);

					return new ExprTypeCast(source, type, checked, ExprTypeCastCategory.EXPR_CAST_REINTERPRET);
				}
			}
		}
		else if(value.type == (TypeBase ref)(ctx.typeAutoRef))
		{
			return new ExprTypeCast(source, type, value, ExprTypeCastCategory.EXPR_CAST_AUTO_PTR_TO_PTR);
		}
		else if(isFunctionArgument && value.type == target.subType)
		{
			// type to type ref conversion
			if(ExprVariableAccess ref node = getType with<ExprVariableAccess>(value))
			{
				ExprBase ref address = new ExprGetAddress(source, ctx.GetReferenceType(value.type), new VariableHandle(node.source, node.variable));

				return address;
			}
			else if(ExprDereference ref node = getType with<ExprDereference>(value))
			{
				return node.value;
			}
			else if(target.subType == value.type)
			{
				SynBase ref sourceInternal = ctx.MakeInternal(source);

				VariableData ref storage = AllocateTemporary(ctx, sourceInternal, target.subType);

				ExprBase ref assignment = new ExprAssignment(sourceInternal, storage.type, CreateGetAddress(ctx, sourceInternal, CreateVariableAccess(ctx, sourceInternal, storage, false)), value);

				ExprBase ref definition = new ExprVariableDefinition(sourceInternal, ctx.typeVoid, new VariableHandle(nullptr, storage), assignment);

				ExprBase ref result = CreateGetAddress(ctx, sourceInternal, CreateVariableAccess(ctx, sourceInternal, storage, false));

				return CreateSequence(ctx, source, definition, result);
			}
		}
	}

	if(type == (TypeBase ref)(ctx.typeAutoRef) && value.type != ctx.typeVoid)
	{
		// type ref to auto ref conversion
		if(isType with<TypeRef>(value.type))
			return new ExprTypeCast(source, type, value, ExprTypeCastCategory.EXPR_CAST_PTR_TO_AUTO_PTR);

		ExprTypeCast ref typeCast = nullptr;

		// type to auto ref conversion
		if(ExprVariableAccess ref node = getType with<ExprVariableAccess>(value))
		{
			ExprBase ref address = new ExprGetAddress(source, ctx.GetReferenceType(value.type), new VariableHandle(node.source, node.variable));

			typeCast = new ExprTypeCast(source, type, address, ExprTypeCastCategory.EXPR_CAST_PTR_TO_AUTO_PTR);
		}
		else if(ExprDereference ref node = getType with<ExprDereference>(value))
		{
			typeCast = new ExprTypeCast(source, type, node.value, ExprTypeCastCategory.EXPR_CAST_PTR_TO_AUTO_PTR);
		}
		else
		{
			typeCast = new ExprTypeCast(source, type, CreateCast(ctx, source, value, ctx.GetReferenceType(value.type), true), ExprTypeCastCategory.EXPR_CAST_PTR_TO_AUTO_PTR);
		}

		if(isFunctionArgument)
			return typeCast;

		// type to auto ref conversion (boxing)
		return CreateFunctionCall1(ctx, source, InplaceStr("duplicate"), typeCast, false, false, true);
	}

	if(type == (TypeBase ref)(ctx.typeAutoArray))
	{
		// type[] to auto[] conversion
		if(isType with<TypeUnsizedArray>(value.type))
			return new ExprTypeCast(source, type, value, ExprTypeCastCategory.EXPR_CAST_UNSIZED_TO_AUTO_ARRAY);
		
		if(TypeArray ref valueType = getType with<TypeArray>(value.type))
		{
			ExprBase ref unsized = CreateCast(ctx, source, value, ctx.GetUnsizedArrayType(valueType.subType), false);

			return CreateCast(ctx, source, unsized, type, false);
		}
	}

	if(value.type == (TypeBase ref)(ctx.typeAutoRef))
	{
		// auto ref to type (unboxing)
		if(!isType with<TypeRef>(type) && type != ctx.typeVoid)
		{
			ExprBase ref ptr = CreateCast(ctx, source, value, ctx.GetReferenceType(type), false);

			return new ExprDereference(source, type, ptr);
		}
	}

	if(TypeClass ref target = getType with<TypeClass>(type))
	{
		if(IsDerivedFrom(getType with<TypeClass>(value.type), target))
		{
			SynBase ref sourceInternal = ctx.MakeInternal(source);

			VariableData ref storage = AllocateTemporary(ctx, sourceInternal, value.type);

			ExprBase ref assignment = new ExprAssignment(sourceInternal, storage.type, CreateGetAddress(ctx, sourceInternal, CreateVariableAccess(ctx, sourceInternal, storage, false)), value);

			ExprBase ref definition = new ExprVariableDefinition(sourceInternal, ctx.typeVoid, new VariableHandle(nullptr, storage), assignment);

			ExprBase ref result = new ExprDereference(sourceInternal, type, new ExprTypeCast(sourceInternal, ctx.GetReferenceType(type), CreateGetAddress(ctx, sourceInternal, CreateVariableAccess(ctx, sourceInternal, storage, false)), ExprTypeCastCategory.EXPR_CAST_REINTERPRET));

			return CreateSequence(ctx, source, definition, result);
		}
	}

	return ReportExpected(ctx, source, type, "ERROR: cannot convert '%.*s' to '%.*s'", FMT_ISTR(value.type.name), FMT_ISTR(type.name));
}

ExprBase ref CreateConditionCast(ExpressionContext ref ctx, SynBase ref source, ExprBase ref value)
{
	if(isType with<TypeError>(value.type))
		return new ExprTypeCast(source, ctx.typeBool, value, ExprTypeCastCategory.EXPR_CAST_REINTERPRET);

	if(!ctx.IsIntegerType(value.type) && !value.type.isGeneric)
	{
		// TODO: function overload

		if(ctx.IsFloatingPointType(value.type))
			return CreateCast(ctx, source, value, ctx.typeBool, false);

		if(isType with<TypeRef>(value.type))
			return CreateCast(ctx, source, value, ctx.typeBool, false);

		if(isType with<TypeUnsizedArray>(value.type))
			return CreateCast(ctx, source, value, ctx.typeBool, false);

		if(isType with<TypeFunction>(value.type))
			return CreateCast(ctx, source, value, ctx.typeBool, false);

		if(value.type == (TypeBase ref)(ctx.typeAutoRef))
		{
			ExprBase ref nullPtr = new ExprNullptrLiteral(value.source, ctx.typeNullPtr);

			return CreateBinaryOp(ctx, source, SynBinaryOpType.SYN_BINARY_OP_NOT_EQUAL, value, nullPtr);
		}
		else
		{
			if(ExprBase ref call = CreateFunctionCall1(ctx, source, InplaceStr("bool"), value, true, false, false))
				return call;

			return ReportExpected(ctx, source, ctx.typeBool, "ERROR: condition type cannot be '%.*s' and function for conversion to bool is undefined", FMT_ISTR(value.type.name));
		}
	}

	AssertValueExpression(ctx, source, value);

	if(value.type == ctx.typeLong)
		value = CreateCast(ctx, source, value, ctx.typeBool, false);

	return value;
}

ExprBase ref CreateAssignment(ExpressionContext ref ctx, SynBase ref source, ExprBase ref lhs, ExprBase ref rhs)
{
	if(isType with<TypeError>(lhs.type) || isType with<TypeError>(rhs.type))
		return new ExprAssignment(source, ctx.GetErrorType(), lhs, rhs);

	if(isType with<ExprUnboxing>(lhs))
	{
		if(!AssertValueExpression(ctx, source, rhs))
			return new ExprAssignment(source, ctx.GetErrorType(), lhs, rhs);

		lhs = CreateCast(ctx, source, lhs, ctx.GetReferenceType(rhs.type), false);
		lhs = new ExprDereference(source, rhs.type, lhs);
	}

	ExprBase ref wrapped = lhs;

	if(ExprVariableAccess ref node = getType with<ExprVariableAccess>(lhs))
	{
		wrapped = new ExprGetAddress(lhs.source, ctx.GetReferenceType(lhs.type), new VariableHandle(node.source, node.variable));
	}
	else if(ExprDereference ref node = getType with<ExprDereference>(lhs))
	{
		wrapped = node.value;
	}
	else if(ExprFunctionCall ref node = getType with<ExprFunctionCall>(lhs))
	{
		// Try to transform 'get' accessor to 'set'
		if(ExprFunctionAccess ref access = getType with<ExprFunctionAccess>(node.function))
		{
			if(access.function.accessor)
			{
				vector<ArgumentData> arguments;
				arguments.push_back(ArgumentData(rhs.source, false, nullptr, rhs.type, rhs));

				if(auto function = ctx.functionMap.first(access.function.nameHash))
				{
					ExprBase ref overloads = CreateFunctionAccess(ctx, source, function, access.context);

					if(ExprBase ref call = CreateFunctionCall(ctx, source, overloads, ArrayView<ArgumentData>(arguments), true))
						return call;
				}

				if(FunctionData ref proto = access.function.proto)
				{
					if(auto function = ctx.functionMap.first(proto.nameHash))
					{
						ExprBase ref overloads = CreateFunctionAccess(ctx, source, function, access.context);

						if(ExprBase ref call = CreateFunctionCall(ctx, source, overloads, ArrayView<ArgumentData>(arguments), true))
							return call;
					}
				}
			}
		}

		if(TypeRef ref refType = getType with<TypeRef>(lhs.type))
			lhs = new ExprDereference(source, refType.subType, lhs);
	}
	else if(TypeRef ref refType = getType with<TypeRef>(lhs.type))
	{
		lhs = new ExprDereference(source, refType.subType, lhs);
	}

	if(!isType with<TypeRef>(wrapped.type))
		return ReportExpected(ctx, source, ctx.GetErrorType(), "ERROR: cannot change immutable value of type %.*s", FMT_ISTR(lhs.type.name));

	if(rhs.type == ctx.typeVoid)
		return ReportExpected(ctx, source, lhs.type, "ERROR: cannot convert from 'void' to '%.*s'", FMT_ISTR(lhs.type.name));

	if(lhs.type == ctx.typeVoid)
		return ReportExpected(ctx, source, lhs.type, "ERROR: cannot convert from '%.*s' to 'void'", FMT_ISTR(rhs.type.name));

	TypePair typePair = TypePair(wrapped.type, rhs.type);

	if(!ctx.noAssignmentOperatorForTypePair.contains(typePair))
	{
		if(ExprBase ref result = CreateFunctionCall2(ctx, source, InplaceStr("="), wrapped, rhs, true, false, true))
			return result;
		else
			ctx.noAssignmentOperatorForTypePair.insert(typePair, true);
	}

	if(ExprBase ref result = CreateFunctionCall2(ctx, source, InplaceStr("default_assign$_"), wrapped, rhs, true, false, true))
		return result;

	if((isType with<TypeArray>(lhs.type) || isType with<TypeUnsizedArray>(lhs.type)) && rhs.type == (TypeBase ref)(ctx.typeAutoArray))
		return CreateFunctionCall2(ctx, source, InplaceStr("__aaassignrev"), wrapped, rhs, false, true, true);

	rhs = CreateCast(ctx, source, rhs, lhs.type, false);

	return new ExprAssignment(source, lhs.type, wrapped, rhs);
}

ExprBase ref GetFunctionUpvalue(ExpressionContext ref ctx, SynBase ref source, VariableData ref target)
{
	InplaceStr upvalueName = GetFunctionVariableUpvalueName(ctx, target);

	if(VariableData ref ref variable = ctx.upvalueMap.find(upvalueName))
	{
		return new ExprVariableAccess(source, (*variable).type, *variable);
	}

	TypeBase ref type = ctx.GetReferenceType(ctx.typeVoid);

	int offset = AllocateGlobalVariable(ctx, source, type.alignment, type.size);
	VariableData ref variable = new VariableData(source, ctx.globalScope, type.alignment, type, new SynIdentifier(upvalueName), offset, ctx.uniqueVariableId++);

	ctx.globalScope.variables.push_back(variable);
	ctx.globalScope.allVariables.push_back(variable);

	ctx.variables.push_back(variable);

	ctx.upvalues.push_back(variable);
	ctx.upvalueMap.insert(upvalueName, variable);

	return new ExprVariableAccess(source, variable.type, variable);
}

ExprBase ref CreateUpvalueClose(ExpressionContext ref ctx, SynBase ref source, VariableData ref variable)
{
	ExprBase ref upvalueAddress = CreateGetAddress(ctx, source, GetFunctionUpvalue(ctx, source, variable));

	ExprBase ref variableAddress = CreateGetAddress(ctx, source, CreateVariableAccess(ctx, source, variable, false));

	variableAddress = new ExprTypeCast(source, ctx.GetReferenceType(ctx.typeVoid), variableAddress, ExprTypeCastCategory.EXPR_CAST_REINTERPRET);

	// Two pointers before data
	int offset = NULLC_PTR_SIZE + NULLC_PTR_SIZE;

	offset += GetAlignmentOffset(offset, variable.type.alignment);

	ExprBase ref copyOffset = new ExprIntegerLiteral(source, ctx.typeInt, offset);

	ExprBase ref copySize = new ExprIntegerLiteral(source, ctx.typeInt, variable.type.size);

	return CreateFunctionCall4(ctx, source, InplaceStr("__closeUpvalue"), upvalueAddress, variableAddress, copyOffset, copySize, false, true, true);
}

ExprBase ref CreateFunctionUpvalueClose(ExpressionContext ref ctx, SynBase ref source, FunctionData ref onwerFunction, ScopeData ref fromScope)
{
	if(!onwerFunction)
		return nullptr;

	ExprSequence ref holder = new ExprSequence(source, ctx.typeVoid, ArrayView<ExprBase ref>());

	onwerFunction.closeUpvalues.push_back(new CloseUpvaluesData(holder, CloseUpvaluesType.CLOSE_UPVALUES_FUNCTION, source, fromScope, 0));

	return holder;
}

ExprBase ref CreateBlockUpvalueClose(ExpressionContext ref ctx, SynBase ref source, FunctionData ref onwerFunction, ScopeData ref scope)
{
	ExprSequence ref holder = new ExprSequence(source, ctx.typeVoid, ArrayView<ExprBase ref>());

	RefList<CloseUpvaluesData> ref closeUpvalues = onwerFunction ? &onwerFunction.closeUpvalues : &ctx.globalCloseUpvalues;

	closeUpvalues.push_back(new CloseUpvaluesData(holder, CloseUpvaluesType.CLOSE_UPVALUES_BLOCK, source, scope, 0));

	return holder;
}

ExprBase ref CreateBreakUpvalueClose(ExpressionContext ref ctx, SynBase ref source, FunctionData ref onwerFunction, ScopeData ref fromScope, int depth)
{
	ExprSequence ref holder = new ExprSequence(source, ctx.typeVoid, ArrayView<ExprBase ref>());

	RefList<CloseUpvaluesData> ref closeUpvalues = onwerFunction ? &onwerFunction.closeUpvalues : &ctx.globalCloseUpvalues;

	closeUpvalues.push_back(new CloseUpvaluesData(holder, CloseUpvaluesType.CLOSE_UPVALUES_BREAK, source, fromScope, depth));

	return holder;
}

ExprBase ref CreateContinueUpvalueClose(ExpressionContext ref ctx, SynBase ref source, FunctionData ref onwerFunction, ScopeData ref fromScope, int depth)
{
	ExprSequence ref holder = new ExprSequence(source, ctx.typeVoid, ArrayView<ExprBase ref>());

	RefList<CloseUpvaluesData> ref closeUpvalues = onwerFunction ? &onwerFunction.closeUpvalues : &ctx.globalCloseUpvalues;

	closeUpvalues.push_back(new CloseUpvaluesData(holder, CloseUpvaluesType.CLOSE_UPVALUES_CONTINUE, source, fromScope, depth));

	return holder;
}

ExprBase ref CreateArgumentUpvalueClose(ExpressionContext ref ctx, SynBase ref source, FunctionData ref onwerFunction)
{
	if(!onwerFunction)
		return nullptr;

	ExprSequence ref holder = new ExprSequence(source, ctx.typeVoid, ArrayView<ExprBase ref>());

	onwerFunction.closeUpvalues.push_back(new CloseUpvaluesData(holder, CloseUpvaluesType.CLOSE_UPVALUES_ARGUMENT, source, nullptr, 0));

	return holder;
}

void ClosePendingUpvalues(ExpressionContext ref ctx, FunctionData ref function)
{
	RefList<CloseUpvaluesData> ref closeUpvalues = function ? &function.closeUpvalues : &ctx.globalCloseUpvalues;

	assert(function == ctx.GetCurrentFunction(ctx.scope));

	for(CloseUpvaluesData ref curr = closeUpvalues.head; curr; curr = curr.next)
	{
		CloseUpvaluesData ref data = curr;

		switch(data.type)
		{
		case CloseUpvaluesType.CLOSE_UPVALUES_FUNCTION:
			assert(function != nullptr);

			for(ScopeData ref scope = data.scope; scope; scope = scope.scope)
			{
				for(int i = 0; i < scope.variables.size(); i++)
				{
					VariableData ref variable = scope.variables[i];

					if(variable.usedAsExternal)
						data.expr.expressions.push_back(CreateUpvalueClose(ctx, ctx.MakeInternal(data.source), variable));
				}

				if(scope.ownerFunction)
					break;
			}
			break;
		case CloseUpvaluesType.CLOSE_UPVALUES_BLOCK:
			for(int i = 0; i < data.scope.variables.size(); i++)
			{
				VariableData ref variable = data.scope.variables[i];

				if(variable.usedAsExternal)
					data.expr.expressions.push_back(CreateUpvalueClose(ctx, ctx.MakeInternal(data.source), variable));
			}
			break;
		case CloseUpvaluesType.CLOSE_UPVALUES_BREAK:
			for(ScopeData ref scope = data.scope; scope; scope = scope.scope)
			{
				if(scope.breakDepth == data.scope.breakDepth - data.depth)
					break;

				for(int i = 0; i < scope.variables.size(); i++)
				{
					VariableData ref variable = scope.variables[i];

					if(variable.usedAsExternal)
						data.expr.expressions.push_back(CreateUpvalueClose(ctx, ctx.MakeInternal(data.source), variable));
				}
			}
			break;
		case CloseUpvaluesType.CLOSE_UPVALUES_CONTINUE:
			for(ScopeData ref scope = data.scope; scope; scope = scope.scope)
			{
				if(scope.contiueDepth == data.scope.contiueDepth - data.depth)
					break;

				for(int i = 0; i < scope.variables.size(); i++)
				{
					VariableData ref variable = scope.variables[i];

					if(variable.usedAsExternal)
						data.expr.expressions.push_back(CreateUpvalueClose(ctx, ctx.MakeInternal(data.source), variable));
				}
			}
			break;
		case CloseUpvaluesType.CLOSE_UPVALUES_ARGUMENT:
			assert(function != nullptr);

			for(VariableHandle ref curr = function.argumentVariables.head; curr; curr = curr.next)
			{
				if(curr.variable.usedAsExternal)
					data.expr.expressions.push_back(CreateUpvalueClose(ctx, ctx.MakeInternal(data.source), curr.variable));
			}

			if(VariableData ref variable = function.contextArgument)
			{
				if(variable.usedAsExternal)
					data.expr.expressions.push_back(CreateUpvalueClose(ctx, ctx.MakeInternal(data.source), variable));
			}
		}
	}
}

ExprBase ref CreateValueFunctionWrapper(ExpressionContext ref ctx, SynBase ref source, SynBase ref synValue, ExprBase ref exprValue, InplaceStr functionName)
{
	if(!AssertValueExpression(ctx, source, exprValue))
		return new ExprError(source, ctx.GetErrorType(), exprValue);

	vector<ArgumentData> arguments;

	TypeBase ref contextRefType = nullptr;

	if(ctx.scope == ctx.globalScope || ctx.scope.ownerNamespace)
		contextRefType = ctx.GetReferenceType(ctx.typeVoid);
	else
		contextRefType = ctx.GetReferenceType(CreateFunctionContextType(ctx, source, functionName));

	SynIdentifier ref functionNameIdentifier = new SynIdentifier(source.begin, source.end, functionName);

	TypeFunction ref typeFunction = ctx.GetFunctionType(source, exprValue ? exprValue.type : ctx.typeAuto, ArrayView<ArgumentData>(arguments));

	FunctionData ref function = new FunctionData(source, ctx.scope, false, false, false, typeFunction, contextRefType, functionNameIdentifier, RefList<MatchData>(), ctx.uniqueFunctionId++);

	CheckFunctionConflict(ctx, source, function.name.name);

	ctx.AddFunction(function);

	ctx.PushScope(function);

	function.functionScope = ctx.scope;

	ExprVariableDefinition ref contextArgumentDefinition = CreateFunctionContextArgument(ctx, ctx.MakeInternal(source), function);

	function.argumentsSize = function.functionScope.dataSize;

	RefList<ExprBase> expressions;

	if (synValue)
		expressions.push_back(CreateReturn(ctx, source, AnalyzeExpression(ctx, synValue)));
	else
		expressions.push_back(new ExprReturn(source, ctx.typeVoid, exprValue, nullptr, CreateFunctionUpvalueClose(ctx, ctx.MakeInternal(source), function, ctx.scope)));

	ClosePendingUpvalues(ctx, function);

	ctx.PopScope(ScopeType.SCOPE_FUNCTION);

	VariableData ref contextVariable = nullptr;
	ExprVariableDefinition ref contextVariableDefinition = nullptr;

	if(ctx.scope != ctx.globalScope && !ctx.scope.ownerNamespace)
	{
		contextVariable = CreateFunctionContextVariable(ctx, source, function, nullptr);
		contextVariableDefinition = CreateFunctionContextVariableDefinition(ctx, source, function, nullptr, contextVariable);
	}

	function.declaration = new ExprFunctionDefinition(source, function.type, function, contextArgumentDefinition, RefList<ExprVariableDefinition>(), nullptr, expressions, contextVariableDefinition, contextVariable);

	ctx.definitions.push_back(function.declaration);

	ExprBase ref access = new ExprFunctionAccess(source, function.type, function, CreateFunctionContextAccess(ctx, ctx.MakeInternal(source), function));

	if(!contextVariableDefinition)
		return access;

	return CreateSequence(ctx, source, contextVariableDefinition, access);
}

ExprBase ref CreateBinaryOp(ExpressionContext ref ctx, SynBase ref source, SynBinaryOpType op, ExprBase ref lhs, ExprBase ref rhs)
{
	if(isType with<TypeError>(lhs.type) || isType with<TypeError>(rhs.type))
		return new ExprBinaryOp(source, ctx.GetErrorType(), op, lhs, rhs);

	if(op == SynBinaryOpType.SYN_BINARY_OP_IN)
		return CreateFunctionCall2(ctx, source, InplaceStr("in"), lhs, rhs, false, false, false);

	bool skipOverload = false;

	// Built-in comparisons
	if(op == SynBinaryOpType.SYN_BINARY_OP_EQUAL || op == SynBinaryOpType.SYN_BINARY_OP_NOT_EQUAL)
	{
		if(lhs.type != rhs.type)
		{
			if(lhs.type == ctx.typeNullPtr)
				lhs = CreateCast(ctx, source, lhs, rhs.type, false);

			if(rhs.type == ctx.typeNullPtr)
				rhs = CreateCast(ctx, source, rhs, lhs.type, false);
		}

		if(lhs.type == (TypeBase ref)(ctx.typeAutoRef) && lhs.type == rhs.type)
		{
			return CreateFunctionCall2(ctx, source, InplaceStr(op == SynBinaryOpType.SYN_BINARY_OP_EQUAL ? "__rcomp" : "__rncomp"), lhs, rhs, false, true, true);
		}

		if(isType with<TypeFunction>(lhs.type) && lhs.type == rhs.type)
		{
			RefList<TypeHandle> types;
			types.push_back(new TypeHandle(ctx.typeInt));
			TypeBase ref type = ctx.GetFunctionType(source, ctx.typeVoid, types);

			lhs = new ExprTypeCast(lhs.source, type, lhs, ExprTypeCastCategory.EXPR_CAST_REINTERPRET);
			rhs = new ExprTypeCast(rhs.source, type, rhs, ExprTypeCastCategory.EXPR_CAST_REINTERPRET);

			return CreateFunctionCall2(ctx, source, InplaceStr(op == SynBinaryOpType.SYN_BINARY_OP_EQUAL ? "__pcomp" : "__pncomp"), lhs, rhs, false, true, true);
		}

		if(isType with<TypeUnsizedArray>(lhs.type) && lhs.type == rhs.type)
		{
			if(ExprBase ref result = CreateFunctionCall2(ctx, source, InplaceStr(GetOpName(op)), lhs, rhs, true, false, true))
				return result;

			return CreateFunctionCall2(ctx, source, InplaceStr(op == SynBinaryOpType.SYN_BINARY_OP_EQUAL ? "__acomp" : "__ancomp"), lhs, rhs, false, true, true);
		}

		if(lhs.type == ctx.typeTypeID && rhs.type == ctx.typeTypeID)
			skipOverload = true;
	}

	if(!skipOverload)
	{
		// For && and || try to find a function that accepts a wrapped right-hand-side evaluation
		if((op == SynBinaryOpType.SYN_BINARY_OP_LOGICAL_AND || op == SynBinaryOpType.SYN_BINARY_OP_LOGICAL_OR) && isType with<TypeClass>(lhs.type))
		{
			if(ExprBase ref result = CreateFunctionCall2(ctx, source, InplaceStr(GetOpName(op)), lhs, CreateValueFunctionWrapper(ctx, source, nullptr, rhs, GetTemporaryFunctionName(ctx)), true, false, true))
				return result;
		}

		// For ^^ try to find a function before generic condition casts to bool
		if(op == SynBinaryOpType.SYN_BINARY_OP_LOGICAL_XOR && isType with<TypeClass>(lhs.type))
		{
			if(ExprBase ref result = CreateFunctionCall2(ctx, source, InplaceStr(GetOpName(op)), lhs, rhs, true, false, true))
				return result;
		}
	}

	// Promotion to bool for some types
	if(op == SynBinaryOpType.SYN_BINARY_OP_LOGICAL_AND || op == SynBinaryOpType.SYN_BINARY_OP_LOGICAL_OR || op == SynBinaryOpType.SYN_BINARY_OP_LOGICAL_XOR)
	{
		lhs = CreateConditionCast(ctx, lhs.source, lhs);
		rhs = CreateConditionCast(ctx, rhs.source, rhs);
	}

	if(lhs.type == ctx.typeVoid)
		return ReportExpected(ctx, source, ctx.GetErrorType(), "ERROR: first operand type is 'void'");

	if(rhs.type == ctx.typeVoid)
		return ReportExpected(ctx, source, ctx.GetErrorType(), "ERROR: second operand type is 'void'");

	bool hasBuiltIn = false;

	hasBuiltIn |= ctx.IsNumericType(lhs.type) && ctx.IsNumericType(rhs.type);
	hasBuiltIn |= lhs.type == ctx.typeTypeID && rhs.type == ctx.typeTypeID && (op == SynBinaryOpType.SYN_BINARY_OP_EQUAL || op == SynBinaryOpType.SYN_BINARY_OP_NOT_EQUAL);
	hasBuiltIn |= isType with<TypeRef>(lhs.type) && lhs.type == rhs.type && (op == SynBinaryOpType.SYN_BINARY_OP_EQUAL || op == SynBinaryOpType.SYN_BINARY_OP_NOT_EQUAL);
	hasBuiltIn |= isType with<TypeEnum>(lhs.type) && lhs.type == rhs.type;

	if(!skipOverload)
	{
		if(ExprBase ref result = CreateFunctionCall2(ctx, source, InplaceStr(GetOpName(op)), lhs, rhs, hasBuiltIn, false, true))
			return result;
	}

	AssertValueExpression(ctx, lhs.source, lhs);
	AssertValueExpression(ctx, rhs.source, rhs);

	if(!hasBuiltIn)
		return ReportExpected(ctx, source, lhs.type, "ERROR: operation %s is not supported on '%.*s' and '%.*s'", GetOpName(op), FMT_ISTR(lhs.type.name), FMT_ISTR(rhs.type.name));

	bool binaryOp = IsBinaryOp(op);
	bool comparisonOp = IsComparisonOp(op);
	bool logicalOp = IsLogicalOp(op);

	if(ctx.IsFloatingPointType(lhs.type) || ctx.IsFloatingPointType(rhs.type))
	{
		if(logicalOp || binaryOp)
			return ReportExpected(ctx, source, lhs.type, "ERROR: operation %s is not supported on '%.*s' and '%.*s'", GetOpName(op), FMT_ISTR(lhs.type.name), FMT_ISTR(rhs.type.name));
	}

	if(logicalOp)
	{
		// Logical operations require both operands to be 'bool'
		lhs = CreateCast(ctx, source, lhs, ctx.typeBool, false);
		rhs = CreateCast(ctx, source, rhs, ctx.typeBool, false);
	}
	else if(ctx.IsNumericType(lhs.type) && ctx.IsNumericType(rhs.type))
	{
		// Numeric operations promote both operands to a common type
		TypeBase ref commonType = ctx.GetBinaryOpResultType(lhs.type, rhs.type);

		lhs = CreateCast(ctx, source, lhs, commonType, false);
		rhs = CreateCast(ctx, source, rhs, commonType, false);
	}

	if(lhs.type != rhs.type)
		return ReportExpected(ctx, source, lhs.type, "ERROR: operation %s is not supported on '%.*s' and '%.*s'", GetOpName(op), FMT_ISTR(lhs.type.name), FMT_ISTR(rhs.type.name));

	TypeBase ref resultType = nullptr;

	if(comparisonOp || logicalOp)
		resultType = ctx.typeBool;
	else
		resultType = lhs.type;

	ExprBase ref result = new ExprBinaryOp(source, resultType, op, lhs, rhs);

	// Arithmetic operation on bool results in an int
	if(lhs.type == ctx.typeBool && !binaryOp && !comparisonOp && !logicalOp)
		return CreateCast(ctx, source, result, ctx.typeInt, false);

	return result;
}

// Apply in reverse order
TypeBase ref ApplyArraySizesToType(ExpressionContext ref ctx, TypeBase ref type, SynBase ref sizes)
{
	SynBase ref size = sizes;

	if(isType with<SynNothing>(size))
		size = nullptr;

	if(sizes.next)
		type = ApplyArraySizesToType(ctx, type, sizes.next);

	if(isType with<TypeAuto>(type))
	{
		if(size)
			Stop(ctx, size, "ERROR: cannot specify array size for auto");

		return ctx.typeAutoArray;
	}

	if(type == ctx.typeVoid)
		Stop(ctx, sizes, "ERROR: cannot specify array size for void");

	if(!size)
	{
		if(type.size >= 64 * 1024)
			Stop(ctx, sizes, "ERROR: array element size cannot exceed 65535 bytes");

		return ctx.GetUnsizedArrayType(type);
	}

	ExprBase ref sizeValue = AnalyzeExpression(ctx, size);

	if(ExprIntegerLiteral ref number = getType with<ExprIntegerLiteral>(EvaluateExpression(ctx, size, CreateCast(ctx, size, sizeValue, ctx.typeLong, false))))
	{
		if(number.value <= 0)
			Stop(ctx, size, "ERROR: array size can't be negative or zero");

		if(TypeClass ref typeClass = getType with<TypeClass>(type))
		{
			if(!typeClass.completed)
				Stop(ctx, size, "ERROR: type '%.*s' is not fully defined", FMT_ISTR(type.name));

			if(typeClass.hasFinalizer)
				Stop(ctx, size, "ERROR: class '%.*s' implements 'finalize' so only an unsized array type can be created", FMT_ISTR(type.name));
		}

		if(type.size >= 64 * 1024)
			Stop(ctx, size, "ERROR: array element size cannot exceed 65535 bytes");

		if(!AssertResolvableType(ctx, size, type, true))
			return ctx.GetErrorType();

		return ctx.GetArrayType(type, number.value);
	}

	Stop(ctx, size, "ERROR: array size cannot be evaluated");

	return nullptr;
}

TypeBase ref CreateGenericTypeInstance(ExpressionContext ref ctx, SynBase ref source, TypeGenericClassProto ref proto, RefList<TypeHandle> ref types)
{
	InplaceStr className = GetGenericClassTypeName(ctx, proto, *types);

	// Check if type already exists
	if(TypeClass ref ref prev = ctx.genericTypeMap.find(className.hash()))
		return *prev;

	// Switch to original type scope
	ScopeData ref scope = ctx.scope;

	ctx.SwitchToScopeAtPoint(proto.scope, proto.source);

	ExprBase ref result = nullptr;

	bool prevErrorHandlerNested = ctx.errorHandlerNested;
	ctx.errorHandlerNested = true;

	ctx.classInstanceDepth++;

	if(ctx.classInstanceDepth > NULLC_MAX_GENERIC_INSTANCE_DEPTH)
		Stop(ctx, source, "ERROR: reached maximum generic type instance depth (%d)", NULLC_MAX_GENERIC_INSTANCE_DEPTH);

	//int traceDepth = NULLC::TraceGetDepth();

	auto tryResult = try(auto(){ return AnalyzeClassDefinition(ctx, proto.definition, proto, *types); });

	if(tryResult)
	{
		result = tryResult.value;
	}
	else
	{
		//NULLC::TraceLeaveTo(traceDepth);

		ctx.classInstanceDepth--;

		// Restore old scope
		ctx.SwitchToScopeAtPoint(scope, nullptr);

		// Additional error info
		/*if(ctx.errorBuf)
		{
			char ref errorCurr = ctx.errorBuf + strlen(ctx.errorBuf);

			const char ref messageStart = errorCurr;

			errorCurr += NULLC::SafeSprintf(errorCurr, ctx.errorBufSize - int(errorCurr - ctx.errorBuf), "while instantiating generic type %.*s<", FMT_ISTR(proto.name));

			for(TypeHandle ref curr = types.head; curr; curr = curr.next)
			{
				TypeBase ref type = curr.type;

				errorCurr += NULLC::SafeSprintf(errorCurr, ctx.errorBufSize - int(errorCurr - ctx.errorBuf), "%s%.*s", curr != types.head ? ", " : "", FMT_ISTR(type.name));
			}

			errorCurr += NULLC::SafeSprintf(errorCurr, ctx.errorBufSize - int(errorCurr - ctx.errorBuf), ">");

			const char ref messageEnd = errorCurr;

			ctx.errorInfo.back().related.push_back(new ErrorInfo(messageStart, messageEnd, source.begin, source.end, source.pos.begin));

			AddErrorLocationInfo(FindModuleCodeWithSourceLocation(ctx, source.pos.begin), source.pos.begin, ctx.errorBuf, ctx.errorBufSize);

			ctx.errorBufLocation += strlen(ctx.errorBufLocation);
		}*/

		ctx.errorHandlerNested = prevErrorHandlerNested;

		tryResult.rethrow();
	}

	ctx.classInstanceDepth--;

	// Restore old scope
	ctx.SwitchToScopeAtPoint(scope, nullptr);

	ctx.errorHandlerNested = prevErrorHandlerNested;

	if(ExprClassDefinition ref definition = getType with<ExprClassDefinition>(result))
	{
		proto.instances.push_back(result);

		return definition.classType;
	}

	Stop(ctx, source, "ERROR: type '%.*s' couldn't be instantiated", FMT_ISTR(proto.name));

	return nullptr;
}

TypeBase ref AnalyzeType(ExpressionContext ref ctx, SynBase ref syntax, bool onlyType = true, bool ref failed = nullptr)
{
	if(isType with<SynTypeAuto>(syntax))
	{
		return ctx.typeAuto;
	}

	if(isType with<SynTypeGeneric>(syntax))
	{
		return ctx.typeGeneric;
	}

	if(SynTypeAlias ref node = getType with<SynTypeAlias>(syntax))
	{
		return ctx.GetGenericAliasType(node.name);
	}

	if(SynTypeReference ref node = getType with<SynTypeReference>(syntax))
	{
		TypeBase ref type = AnalyzeType(ctx, node.type, true, failed);

		if(isType with<TypeAuto>(type))
			return ctx.typeAutoRef;

		if(isType with<TypeError>(type))
			return ctx.GetErrorType();

		if(!AssertResolvableType(ctx, syntax, type, true))
			return ctx.GetErrorType();

		return ctx.GetReferenceType(type);
	}

	if(SynTypeArray ref node = getType with<SynTypeArray>(syntax))
	{
		TypeBase ref type = AnalyzeType(ctx, node.type, onlyType, failed);

		if(!onlyType && !type)
			return nullptr;

		if(isType with<TypeError>(type))
			return ctx.GetErrorType();

		return ApplyArraySizesToType(ctx, type, node.sizes.head);
	}

	if(SynArrayIndex ref node = getType with<SynArrayIndex>(syntax))
	{
		TypeBase ref type = AnalyzeType(ctx, node.value, onlyType, failed);

		if(!onlyType && !type)
			return nullptr;

		if(isType with<TypeVoid>(type))
		{
			Report(ctx, syntax, "ERROR: cannot define an array of 'void'");

			return ctx.GetErrorType();
		}

		if(isType with<TypeAuto>(type))
		{
			if(!node.arguments.empty())
				Stop(ctx, syntax, "ERROR: cannot specify array size for auto");

			return ctx.typeAutoArray;
		}

		if(isType with<TypeError>(type))
			return ctx.GetErrorType();

		if(node.arguments.empty())
		{
			if(type.size >= 64 * 1024)
				Stop(ctx, syntax, "ERROR: array element size cannot exceed 65535 bytes");

			if(!AssertResolvableType(ctx, syntax, type, true))
				return ctx.GetErrorType();

			return ctx.GetUnsizedArrayType(type);
		}

		if(node.arguments.size() > 1)
			Stop(ctx, syntax, "ERROR: ',' is not expected in array type size");

		SynCallArgument ref argument = node.arguments.head;

		if(argument.name)
			Stop(ctx, syntax, "ERROR: named argument not expected in array type size");

		ExprBase ref size = AnalyzeExpression(ctx, argument.value);

		if(isType with<TypeError>(size.type))
			return ctx.GetErrorType();

		if(ExprIntegerLiteral ref number = getType with<ExprIntegerLiteral>(EvaluateExpression(ctx, syntax, CreateCast(ctx, node, size, ctx.typeLong, false))))
		{
			if(TypeArgumentSet ref lhs = getType with<TypeArgumentSet>(type))
			{
				if(number.value < 0)
					Stop(ctx, syntax, "ERROR: argument index can't be negative");

				if(lhs.types.empty())
					Stop(ctx, syntax, "ERROR: function argument set is empty");

				if(number.value >= lhs.types.size())
					Stop(ctx, syntax, "ERROR: this function type '%.*s' has only %d argument(s)", FMT_ISTR(type.name), lhs.types.size());

				return lhs.types[int(number.value)].type;
			}

			if(number.value <= 0)
				Stop(ctx, syntax, "ERROR: array size can't be negative or zero");

			if(TypeClass ref typeClass = getType with<TypeClass>(type))
			{
				if(typeClass.hasFinalizer)
					Stop(ctx, syntax, "ERROR: class '%.*s' implements 'finalize' so only an unsized array type can be created", FMT_ISTR(type.name));
			}

			if(type.size >= 64 * 1024)
				Stop(ctx, syntax, "ERROR: array element size cannot exceed 65535 bytes");

			if(!AssertResolvableType(ctx, syntax, type, true))
				return ctx.GetErrorType();

			return ctx.GetArrayType(type, number.value);
		}

		if(!onlyType)
			return nullptr;

		Stop(ctx, syntax, "ERROR: index must be a constant expression");
	}

	if(SynTypeFunction ref node = getType with<SynTypeFunction>(syntax))
	{
		TypeBase ref returnType = AnalyzeType(ctx, node.returnType, onlyType, failed);

		if(!onlyType && !returnType)
			return nullptr;

		if(isType with<TypeError>(returnType))
			return ctx.GetErrorType();

		if(!AssertResolvableType(ctx, syntax, returnType, true))
			return ctx.GetErrorType();

		if(returnType == ctx.typeAuto)
			Stop(ctx, syntax, "ERROR: return type of a function type cannot be auto");

		RefList<TypeHandle> arguments;

		for(SynBase ref el = node.arguments.head; el; el = el.next)
		{
			TypeBase ref argType = AnalyzeType(ctx, el, onlyType, failed);

			if(!onlyType && !argType)
				return nullptr;

			if(isType with<TypeError>(argType))
				return ctx.GetErrorType();

			if(!AssertResolvableType(ctx, syntax, argType, true))
				return ctx.GetErrorType();

			if(argType == ctx.typeAuto)
				Stop(ctx, syntax, "ERROR: function argument cannot be an auto type");

			if(argType == ctx.typeVoid)
				Stop(ctx, syntax, "ERROR: function argument cannot be a void type");

			arguments.push_back(new TypeHandle(argType));
		}

		return ctx.GetFunctionType(syntax, returnType, arguments);
	}

	if(SynTypeof ref node = getType with<SynTypeof>(syntax))
	{
		bool prevErrorHandlerNested = ctx.errorHandlerNested;
		ctx.errorHandlerNested = true;

		string ref errorBuf = ctx.errorBuf;

		if(failed)
		{
			ctx.errorBuf = nullptr;
		}

		// Remember current scope
		ScopeData ref scope = ctx.scope;

		//int traceDepth = NULLC::TraceGetDepth();

		auto tryResult = try(auto(){
			TypeBase ref type = AnalyzeType(ctx, node.value, false);

			if(!type)
			{
				ExprBase ref value = AnalyzeExpression(ctx, node.value);

				if(value.type == ctx.typeAuto)
					Stop(ctx, syntax, "ERROR: cannot take typeid from auto type");

				AssertValueExpression(ctx, syntax, value);

				type = value.type;
			}

			return type;
		});

		if(tryResult)
		{
			TypeBase ref type = tryResult.value;

			ctx.errorHandlerNested = prevErrorHandlerNested;

			ctx.errorBuf = errorBuf;

			if(type)
			{
				if(!AssertResolvableType(ctx, node.value, type, false))
					return ctx.GetErrorType();

				return type;
			}
		}
		else
		{
			//NULLC::TraceLeaveTo(traceDepth);

			// Restore original scope
			if(ctx.scope != scope)
				ctx.SwitchToScopeAtPoint(scope, nullptr);

			ctx.errorHandlerNested = prevErrorHandlerNested;

			ctx.errorBuf = errorBuf;

			if(failed)
			{
				*failed = true;
				return ctx.typeGeneric;
			}

			tryResult.rethrow();
		}
	}

	if(SynTypeSimple ref node = getType with<SynTypeSimple>(syntax))
	{
		TypeBase ref ref type = nullptr;

		for(ScopeData ref nsScope = NamedOrGlobalScopeFrom(ctx.scope); nsScope; nsScope = NamedOrGlobalScopeFrom(nsScope.scope))
		{
			int hash = nsScope.ownerNamespace ? StringHashContinue(nsScope.ownerNamespace.fullNameHash, ".") : GetStringHash("");

			for(SynIdentifier ref part = node.path.head; part; part = getType with<SynIdentifier>(part.next))
			{
				hash = StringHashContinue(hash, part.name);
				hash = StringHashContinue(hash, ".");
			}

			hash = StringHashContinue(hash, node.name);

			type = ctx.typeMap.find(hash);

			if(type)
				return *type;
		}

		// Might be a variable
		if(!onlyType)
			return nullptr;

		Report(ctx, syntax, "ERROR: '%.*s' is not a known type name", FMT_ISTR(node.name));

		return ctx.GetErrorType();
	}

	if(SynMemberAccess ref node = getType with<SynMemberAccess>(syntax))
	{
		TypeBase ref value = AnalyzeType(ctx, node.value, onlyType, failed);

		if(!onlyType && !value)
			return nullptr;

		if(isType with<TypeGeneric>(value))
			return ctx.typeGeneric;

		if(isType with<TypeError>(value))
			return ctx.GetErrorType();

		ExprBase ref result = CreateTypeidMemberAccess(ctx, syntax, value, node.member);

		if(ExprTypeLiteral ref typeLiteral = getType with<ExprTypeLiteral>(result))
			return typeLiteral.value;

		// [n]

		if(!onlyType)
			return nullptr;

		// isReference/isArray/isFunction/arraySize/hasMember(x)/class member/class typedef

		Stop(ctx, syntax, "ERROR: typeof expression result is not a type");

		return nullptr;
	}

	if(SynTypeGenericInstance ref node = getType with<SynTypeGenericInstance>(syntax))
	{
		TypeBase ref baseType = AnalyzeType(ctx, node.baseType, true, failed);

		if(isType with<TypeError>(baseType))
			return ctx.GetErrorType();

		// TODO: overloads with a different number of generic arguments

		if(TypeGenericClassProto ref proto = getType with<TypeGenericClassProto>(baseType))
		{
			RefList<SynIdentifier> aliases = proto.definition.aliases;

			if(node.types.size() < aliases.size())
				Stop(ctx, syntax, "ERROR: there where only '%d' argument(s) to a generic type that expects '%d'", node.types.size(), aliases.size());

			if(node.types.size() > aliases.size())
				Stop(ctx, syntax, "ERROR: type has only '%d' generic argument(s) while '%d' specified", aliases.size(), node.types.size());

			bool isGeneric = false;
			RefList<TypeHandle> types;

			for(SynBase ref el = node.types.head; el; el = el.next)
			{
				TypeBase ref type = AnalyzeType(ctx, el, true, failed);

				if(type == ctx.typeAuto)
					Stop(ctx, syntax, "ERROR: 'auto' type cannot be used as template argument");

				if(isType with<TypeError>(type))
					return ctx.GetErrorType();

				isGeneric |= type.isGeneric;

				types.push_back(new TypeHandle(type));
			}

			if(isGeneric)
				return ctx.GetGenericClassType(syntax, proto, types);
			
			return CreateGenericTypeInstance(ctx, syntax, proto, types);
		}

		Stop(ctx, syntax, "ERROR: type '%.*s' can't have generic arguments", FMT_ISTR(baseType.name));
	}

	if(isType with<SynError>(syntax))
		return ctx.GetErrorType();

	if(!onlyType)
		return nullptr;

	Stop(ctx, syntax, "ERROR: unknown type");

	return nullptr;
}

int AnalyzeAlignment(ExpressionContext ref ctx, SynAlign ref syntax)
{
	// noalign
	if(!syntax.value)
		return 1;

	ExprBase ref alignment = AnalyzeExpression(ctx, syntax.value);

	// Some info about aignment expression tree is lost
	if(isType with<TypeError>(alignment.type))
		return 0;

	if(ExprIntegerLiteral ref alignValue = getType with<ExprIntegerLiteral>(EvaluateExpression(ctx, syntax, CreateCast(ctx, syntax, alignment, ctx.typeLong, false))))
	{
		if(alignValue.value > 16)
		{
			Report(ctx, syntax, "ERROR: alignment must be less than 16 bytes");

			return 0;
		}

		if(alignValue.value & (alignValue.value - 1))
		{
			Report(ctx, syntax, "ERROR: alignment must be power of two");

			return 0;
		}

		return int(alignValue.value);
	}

	Report(ctx, syntax, "ERROR: alignment must be a constant expression");

	return 0;
}

ExprBase ref AnalyzeNumber(ExpressionContext ref ctx, SynNumber ref syntax)
{
	InplaceStr value = syntax.value;

	// Hexadecimal
	if(value.length() > 1 && value[1] == 'x')
	{
		if(value.length() == 2)
			ReportAt(ctx, syntax, value.begin_ref + 2, "ERROR: '0x' must be followed by number");

		// Skip 0x
		int pos = 2;

		// Skip leading zeros
		while(value[pos] == '0')
			pos++;

		if(int(value.length() - pos) > 16)
			ReportAt(ctx, syntax, value.begin_ref, "ERROR: overflow in hexadecimal constant");

		long num = ParseLong(ctx, syntax, value.begin_ref + pos, value.end_ref, 16);

		// If number overflows integer number, create long number
		if(int(num) == num)
			return new ExprIntegerLiteral(syntax, ctx.typeInt, num);

		return new ExprIntegerLiteral(syntax, ctx.typeLong, num);
	}

	bool isFP = false;

	for(int i = 0; i < value.length(); i++)
	{
		if(value[i] == '.' || value[i] == 'e')
			isFP = true;
	}

	if(!isFP)
	{
		if(syntax.suffix == InplaceStr("b"))
		{
			int pos = 0;

			// Skip leading zeros
			while(value[pos] == '0')
				pos++;

			if(int(value.length() - pos) > 64)
				ReportAt(ctx, syntax, value.begin_ref, "ERROR: overflow in binary constant");

			long num = ParseLong(ctx, syntax, value.begin_ref + pos, value.end_ref, 2);

			// If number overflows integer number, create long number
			if(int(num) == num)
				return new ExprIntegerLiteral(syntax, ctx.typeInt, num);

			return new ExprIntegerLiteral(syntax, ctx.typeLong, num);
		}
		else if(syntax.suffix == InplaceStr("l"))
		{
			long num = ParseLong(ctx, syntax, value.begin_ref, value.end_ref, 10);

			if(num > 9223372036854775807l)
				StopAt(ctx, syntax, value.begin_ref, "ERROR: overflow in integer constant");

			return new ExprIntegerLiteral(syntax, ctx.typeLong, num);
		}
		else if(!syntax.suffix.empty())
		{
			ReportAt(ctx, syntax, syntax.suffix.begin_ref, "ERROR: unknown number suffix '%.*s'", syntax.suffix.length(), syntax.suffix.begin);
		}

		if(value.length() > 1 && value[0] == '0' && isDigit(value[1]))
		{
			int pos = 0;

			// Skip leading zeros
			while(value[pos] == '0')
				pos++;

			if(int(value.length() - pos) > 22 || (int(value.length() - pos) > 21 && value[pos] != '1'))
				ReportAt(ctx, syntax, value.begin_ref, "ERROR: overflow in octal constant");

			long num = ParseLong(ctx,syntax,  value.begin_ref, value.end_ref, 8);

			// If number overflows integer number, create long number
			if(int(num) == num)
				return new ExprIntegerLiteral(syntax, ctx.typeInt, num);

			return new ExprIntegerLiteral(syntax, ctx.typeLong, num);
		}

		long num = ParseLong(ctx, syntax, value.begin_ref, value.end_ref, 10);

		if(int(num) != num)
			StopAt(ctx, syntax, value.begin_ref, "ERROR: overflow in integer constant");

		return new ExprIntegerLiteral(syntax, ctx.typeInt, num);
	}

	if(syntax.suffix == InplaceStr("f"))
	{
		double num = ParseDouble(ctx, value.begin_ref);

		return new ExprRationalLiteral(syntax, ctx.typeFloat, float(num));
	}
	else if(!syntax.suffix.empty())
	{
		ReportAt(ctx, syntax, syntax.suffix.begin_ref, "ERROR: unknown number suffix '%.*s'", syntax.suffix.length(), syntax.suffix.begin);
	}

	double num = ParseDouble(ctx, value.begin_ref);

	return new ExprRationalLiteral(syntax, ctx.typeDouble, num);
}

ExprBase ref AnalyzeArray(ExpressionContext ref ctx, SynArray ref syntax)
{
	assert(syntax.values.head != nullptr);

	vector<ExprBase ref> raw;

	TypeBase ref nestedUnsizedType = nullptr;

	for(SynBase ref el = syntax.values.head; el; el = el.next)
	{
		ExprBase ref value = AnalyzeExpression(ctx, el);

		if(!raw.empty() && raw[0].type != value.type)
		{
			if(TypeArray ref arrayType = getType with<TypeArray>(raw[0].type))
				nestedUnsizedType = ctx.GetUnsizedArrayType(arrayType.subType);
		}

		raw.push_back(value);
	}

	// First value type is required to complete array definition
	if(!raw.empty() && isType with<TypeError>(raw[0].type))
	{
		RefList<ExprBase> values;

		for(int i = 0; i < raw.size(); i++)
			values.push_back(raw[i]);

		return new ExprArray(syntax, ctx.GetErrorType(), values);
	}

	RefList<ExprBase> values;

	TypeBase ref subType = nullptr;

	for(int i = 0; i < raw.size(); i++)
	{
		ExprBase ref value = raw[i];

		if(nestedUnsizedType)
			value = CreateCast(ctx, value.source, value, nestedUnsizedType, false);

		if(subType == nullptr)
		{
			subType = value.type;
		}
		else if(subType != value.type)
		{
			// Allow numeric promotion
			if(ctx.IsIntegerType(value.type) && ctx.IsFloatingPointType(subType))
				value = CreateCast(ctx, value.source, value, subType, false);
			else if(ctx.IsIntegerType(value.type) && ctx.IsIntegerType(subType) && subType.size > value.type.size)
				value = CreateCast(ctx, value.source, value, subType, false);
			else if(ctx.IsFloatingPointType(value.type) && ctx.IsFloatingPointType(subType) && subType.size > value.type.size)
				value = CreateCast(ctx, value.source, value, subType, false);
			else if(!isType with<TypeError>(value.type))
				value = ReportExpected(ctx, value.source, value.type, "ERROR: array element %d type '%.*s' doesn't match '%.*s'", i + 1, FMT_ISTR(value.type.name), FMT_ISTR(subType.name));
		}

		if(value.type == ctx.typeVoid)
			Stop(ctx, value.source, "ERROR: array cannot be constructed from void type elements");

		if(!AssertValueExpression(ctx, value.source, value))
			value = new ExprError(syntax, ctx.GetErrorType(), value);

		values.push_back(value);
	}

	if(!values.empty() && isType with<TypeError>(values[0].type))
		return new ExprArray(syntax, ctx.GetErrorType(), values);

	if(TypeClass ref typeClass = getType with<TypeClass>(subType))
	{
		if(typeClass.hasFinalizer)
			Stop(ctx, syntax, "ERROR: class '%.*s' implements 'finalize' so only an unsized array type can be created", FMT_ISTR(subType.name));
	}

	if(subType.size >= 64 * 1024)
		Stop(ctx, syntax, "ERROR: array element size cannot exceed 65535 bytes");

	return new ExprArray(syntax, ctx.GetArrayType(subType, values.size()), values);
}

ExprBase ref CreateFunctionContextAccess(ExpressionContext ref ctx, SynBase ref source, FunctionData ref function)
{
	if(function.scope.ownerType)
		return ReportExpected(ctx, source, function.contextType, "ERROR: member function can't be called without a class instance");

	bool inFunctionScope = false;

	// Walk up, but if we reach a type owner, stop - we're not in a context of a function
	for(ScopeData ref curr = ctx.scope; curr; curr = curr.scope)
	{
		if(curr.ownerType)
			break;

		if(curr.ownerFunction == function)
		{
			inFunctionScope = true;
			break;
		}
	}

	ExprBase ref context = nullptr;

	if(inFunctionScope)
	{
		context = CreateVariableAccess(ctx, source, function.contextArgument, true);
	}
	else if(ExprFunctionDefinition ref definition = getType with<ExprFunctionDefinition>(function.declaration))
	{
		if(definition.contextVariable)
		{
			VariableData ref contextVariable = definition.contextVariable;

			context = CreateVariableAccess(ctx, source, contextVariable, true);

			if(ExprVariableAccess ref access = getType with<ExprVariableAccess>(context))
			{
				assert(access.variable == contextVariable);

				context = new ExprFunctionContextAccess(source, access.type, function, contextVariable);
			}
		}
		else
		{
			context = new ExprNullptrLiteral(source, function.contextType);
		}
	}
	else if(function.isCoroutine)
	{
		int functionIndex = ctx.GetFunctionIndex(function);

		if(function.importModule)
			functionIndex = functionIndex - function.importModule.startingFunctionIndex + function.importModule.importedFunctionCount;

		InplaceStr contextVariableName = GetFunctionContextVariableName(ctx, function, functionIndex);

		if(VariableData ref ref variable = ctx.variableMap.find(contextVariableName.hash()))
		{
			context = CreateVariableAccess(ctx, source, *variable, true);

			if(ExprVariableAccess ref access = getType with<ExprVariableAccess>(context))
			{
				assert(access.variable == *variable);

				context = new ExprFunctionContextAccess(source, access.type, function, *variable);
			}
		}
		else
		{
			context = new ExprNullptrLiteral(source, function.contextType);
		}
	}
	else
	{
		context = new ExprNullptrLiteral(source, function.contextType);
	}

	return context;
}

ExprBase ref CreateFunctionAccess(ExpressionContext ref ctx, SynBase ref source, hashmap_node<int, FunctionData ref> ref function, ExprBase ref context)
{
	if(auto curr = ctx.functionMap.next(function))
	{
		vector<TypeBase ref> types;
		RefList<FunctionHandle> functions;

		types.push_back(function.value.type);
		functions.push_back(new FunctionHandle(function.value));

		while(curr)
		{
			types.push_back(curr.value.type);
			functions.push_back(new FunctionHandle(curr.value));

			curr = ctx.functionMap.next(curr);
		}

		TypeFunctionSet ref type = ctx.GetFunctionSetType(ArrayView<TypeBase ref>(types));

		return new ExprFunctionOverloadSet(source, type, functions, context);
	}

	if(!context)
		context = CreateFunctionContextAccess(ctx, ctx.MakeInternal(source), function.value);

	return new ExprFunctionAccess(source, function.value.type, function.value, context);
}

ExprBase ref CreateFunctionCoroutineStateUpdate(ExpressionContext ref ctx, SynBase ref source, FunctionData ref function, int state)
{
	if(!function.isCoroutine)
		return nullptr;

	ExprBase ref member = CreateVariableAccess(ctx, source, function.coroutineJumpOffset, true);

	return CreateAssignment(ctx, source, CreateGetAddress(ctx, source, member), new ExprIntegerLiteral(source, ctx.typeInt, state));
}

VariableData ref AddFunctionUpvalue(ExpressionContext ref ctx, SynBase ref source, FunctionData ref function, VariableData ref data)
{
	if(UpvalueData ref ref prev = function.upvalueVariableMap.find(data))
		return (*prev).target;

	TypeRef ref refType = getType with<TypeRef>(function.contextType);

	assert(refType != nullptr);

	TypeClass ref classType = getType with<TypeClass>(refType.subType);

	assert(classType != nullptr);

	ScopeData ref currScope = ctx.scope;

	ctx.scope = classType.typeScope;

	int index = 0;

	if(function.upvalueNameSet.contains(data.name.name))
		index = classType.members.size();

	// Pointer to target variable
	VariableData ref target = AllocateClassMember(ctx, source, 0, ctx.GetReferenceType(data.type), GetFunctionContextMemberName(ctx, data.name.name, InplaceStr("target"), index), true, ctx.uniqueVariableId++);

	classType.members.push_back(new MemberHandle(nullptr, target, nullptr));

	// Pointer to next upvalue
	VariableData ref nextUpvalue = AllocateClassMember(ctx, source, 0, ctx.GetReferenceType(ctx.typeVoid), GetFunctionContextMemberName(ctx, data.name.name, InplaceStr("nextUpvalue"), index), true, ctx.uniqueVariableId++);

	classType.members.push_back(new MemberHandle(nullptr, nextUpvalue, nullptr));

	// Copy of the data
	VariableData ref copy = AllocateClassMember(ctx, source, data.alignment, data.type, GetFunctionContextMemberName(ctx, data.name.name, InplaceStr("copy"), index), true, ctx.uniqueVariableId++);

	classType.members.push_back(new MemberHandle(nullptr, copy, nullptr));

	ctx.scope = currScope;

	data.usedAsExternal = true;

	UpvalueData ref upvalue = new UpvalueData(data, target, nextUpvalue, copy);

	function.upvalues.push_back(upvalue);

	function.upvalueVariableMap.insert(data, upvalue);
	function.upvalueNameSet.insert(data.name.name, true);

	return target;
}

VariableData ref AddFunctionCoroutineVariable(ExpressionContext ref ctx, SynBase ref source, FunctionData ref function, VariableData ref data)
{
	if(CoroutineStateData ref ref prev = function.coroutineStateVariableMap.find(data))
		return (*prev).storage;

	TypeRef ref refType = getType with<TypeRef>(function.contextType);

	assert(refType != nullptr);

	TypeClass ref classType = getType with<TypeClass>(refType.subType);

	assert(classType != nullptr);

	ScopeData ref currScope = ctx.scope;

	ctx.scope = classType.typeScope;

	int index = 0;

	if(function.coroutineStateNameSet.contains(data.name.name))
		index = classType.members.size();

	// Copy of the data
	VariableData ref storage = AllocateClassMember(ctx, source, data.alignment, data.type, GetFunctionContextMemberName(ctx, data.name.name, InplaceStr("storage"), index), true, ctx.uniqueVariableId++);

	classType.members.push_back(new MemberHandle(nullptr, storage, nullptr));

	ctx.scope = currScope;

	CoroutineStateData ref state = new CoroutineStateData(data, storage);

	function.coroutineState.push_back(state);

	function.coroutineStateVariableMap.insert(data, state);
	function.coroutineStateNameSet.insert(data.name.name, true);

	return storage;
}

ExprBase ref CreateVariableAccess(ExpressionContext ref ctx, SynBase ref source, VariableData ref variable, bool handleReference)
{
	if(variable.type == ctx.typeAuto)
		Stop(ctx, source, "ERROR: variable '%.*s' is being used while its type is unknown", FMT_ISTR(variable.name.name));

	if(variable.type.isGeneric)
		Stop(ctx, source, "ERROR: variable '%.*s' is being used while its type is unknown", FMT_ISTR(variable.name.name));

	if(isType with<TypeError>(variable.type))
		return new ExprError(source, ctx.GetErrorType());

	// Is this is a class member access
	if(variable.scope.ownerType)
	{
		ExprBase ref thisAccess = CreateVariableAccess(ctx, source, RefList<SynIdentifier>(), InplaceStr("this"), false);

		if(!thisAccess)
			Stop(ctx, source, "ERROR: 'this' variable is not available");

		// Member access only shifts an address, so we are left with a reference to get value from
		ExprMemberAccess ref shift = new ExprMemberAccess(source, ctx.GetReferenceType(variable.type), thisAccess, new VariableHandle(source, variable));

		return new ExprDereference(source, variable.type, shift);
	}

	ExprBase ref access = nullptr;

	FunctionData ref currentFunction = ctx.GetCurrentFunction(ctx.scope.ownerType ? ctx.scope.scope : ctx.scope);

	FunctionData ref variableFunctionOwner = ctx.GetFunctionOwner(variable.scope);

	bool externalAccess = false;
	bool coroutineAccess = false;

	if(currentFunction && variable.scope.type != ScopeType.SCOPE_TEMPORARY && ctx.scope.type != ScopeType.SCOPE_TEMPORARY)
	{
		if(variableFunctionOwner && variableFunctionOwner != currentFunction)
			externalAccess = true;
		else if(!variableFunctionOwner && !(variable.scope == ctx.globalScope || variable.scope.ownerNamespace))
			externalAccess = true;
		else if(variableFunctionOwner == currentFunction && currentFunction.isCoroutine && !IsArgumentVariable(currentFunction, variable))
			coroutineAccess = true;
	}

	if(externalAccess)
	{
		if(currentFunction.scope.ownerType)
			Stop(ctx, source, "ERROR: member function '%.*s' cannot access external variable '%.*s'", FMT_ISTR(currentFunction.name.name), FMT_ISTR(variable.name.name));

		ExprBase ref context = new ExprVariableAccess(source, currentFunction.contextArgument.type, currentFunction.contextArgument);

		VariableData ref closureMember = AddFunctionUpvalue(ctx, source, currentFunction, variable);

		ExprBase ref member = new ExprMemberAccess(source, ctx.GetReferenceType(closureMember.type), context, new VariableHandle(source, closureMember));

		member = new ExprDereference(source, closureMember.type, member);

		access = new ExprDereference(source, variable.type, member);
	}
	else if(coroutineAccess)
	{
		ExprBase ref context = new ExprVariableAccess(source, currentFunction.contextArgument.type, currentFunction.contextArgument);

		VariableData ref closureMember = AddFunctionCoroutineVariable(ctx, source, currentFunction, variable);

		ExprBase ref member = new ExprMemberAccess(source, ctx.GetReferenceType(closureMember.type), context, new VariableHandle(source, closureMember));

		access = new ExprDereference(source, variable.type, member);
	}
	else
	{
		access = new ExprVariableAccess(source, variable.type, variable);
	}

	if(variable.isReference && handleReference)
	{
		assert(isType with<TypeRef>(variable.type));

		TypeRef ref type = getType with<TypeRef>(variable.type);

		access = new ExprDereference(source, type.subType, access);
	}

	return access;
}

ExprBase ref CreateVariableAccess(ExpressionContext ref ctx, SynBase ref source, RefList<SynIdentifier> path, InplaceStr name, bool allowInternal)
{
	// Check local scope first
	{
		int hash = GetStringHash("");

		for(SynIdentifier ref part = path.head; part; part = getType with<SynIdentifier>(part.next))
		{
			hash = StringHashContinue(hash, part.name);
			hash = StringHashContinue(hash, ".");
		}

		hash = StringHashContinue(hash, name);

		if(VariableData ref ref variable = ctx.variableMap.find(hash))
		{
			// Must be non-global scope
			if(NamedOrGlobalScopeFrom((*variable).scope) != ctx.globalScope)
				return CreateVariableAccess(ctx, source, *variable, true);
		}
	}

	// Search for variable starting from current namespace to global scope
	for(ScopeData ref nsScope = NamedOrGlobalScopeFrom(ctx.scope); nsScope; nsScope = NamedOrGlobalScopeFrom(nsScope.scope))
	{
		int hash = nsScope.ownerNamespace ? StringHashContinue(nsScope.ownerNamespace.fullNameHash, ".") : GetStringHash("");

		for(SynIdentifier ref part = path.head; part; part = getType with<SynIdentifier>(part.next))
		{
			hash = StringHashContinue(hash, part.name);
			hash = StringHashContinue(hash, ".");
		}

		hash = StringHashContinue(hash, name);

		if(VariableData ref ref variable = ctx.variableMap.find(hash))
			return CreateVariableAccess(ctx, source, *variable, true);
	}

	{
		// Try a class constant or an alias
		if(TypeStruct ref structType = getType with<TypeStruct>(ctx.GetCurrentType(ctx.scope)))
		{
			for(ConstantData ref curr = structType.constants.head; curr; curr = curr.next)
			{
				if(curr.name.name == name)
					return CreateLiteralCopy(ctx, source, curr.value);
			}
		}
	}

	if(path.empty())
	{
		if(FindNextTypeFromScope(ctx.scope))
		{
			if(VariableData ref ref variable = ctx.variableMap.find(InplaceStr("this").hash()))
			{
				if(ExprBase ref member = CreateMemberAccess(ctx, source, CreateVariableAccess(ctx, source, *variable, true), new SynIdentifier(name), true))
					return member;
			}
		}
	}

	hashmap_node<int, FunctionData ref> ref function = nullptr;

	for(ScopeData ref nsScope = NamedOrGlobalScopeFrom(ctx.scope); nsScope; nsScope = NamedOrGlobalScopeFrom(nsScope.scope))
	{
		int hash = nsScope.ownerNamespace ? StringHashContinue(nsScope.ownerNamespace.fullNameHash, ".") : GetStringHash("");

		for(SynIdentifier ref part = path.head; part; part = getType with<SynIdentifier>(part.next))
		{
			hash = StringHashContinue(hash, part.name);
			hash = StringHashContinue(hash, ".");
		}

		hash = StringHashContinue(hash, name);

		function = ctx.functionMap.first(hash);

		if(function)
		{
			if(function.value.isInternal && !allowInternal)
				function = nullptr;

			break;
		}
	}

	if(function)
		return CreateFunctionAccess(ctx, source, function, nullptr);

	return nullptr;
}

ExprBase ref AnalyzeVariableAccess(ExpressionContext ref ctx, SynIdentifier ref syntax)
{
	ExprBase ref value = CreateVariableAccess(ctx, syntax, RefList<SynIdentifier>(), syntax.name, false);

	if(!value)
		return ReportExpected(ctx, syntax, ctx.GetErrorType(), "ERROR: unknown identifier '%.*s'", FMT_ISTR(syntax.name));

	return value;
}

ExprBase ref AnalyzeVariableAccess(ExpressionContext ref ctx, SynTypeSimple ref syntax)
{
	ExprBase ref value = CreateVariableAccess(ctx, syntax, syntax.path, syntax.name, false);

	if(!value)
		return ReportExpected(ctx, syntax, ctx.GetErrorType(), "ERROR: unknown identifier '%.*s'", FMT_ISTR(syntax.name));

	return value;
}

ExprBase ref AnalyzePreModify(ExpressionContext ref ctx, SynPreModify ref syntax)
{
	ExprBase ref value = AnalyzeExpression(ctx, syntax.value);

	if(isType with<TypeError>(value.type))
		return new ExprPreModify(syntax, ctx.GetErrorType(), value, syntax.isIncrement);

	ExprBase ref wrapped = value;

	if(ExprVariableAccess ref node = getType with<ExprVariableAccess>(value))
		wrapped = new ExprGetAddress(syntax, ctx.GetReferenceType(value.type), new VariableHandle(node.source, node.variable));
	else if(ExprDereference ref node = getType with<ExprDereference>(value))
		wrapped = node.value;

	if(!isType with<TypeRef>(wrapped.type))
		return ReportExpected(ctx, syntax, ctx.GetErrorType(), "ERROR: cannot change immutable value of type %.*s", FMT_ISTR(value.type.name));

	if(!ctx.IsNumericType(value.type))
		return ReportExpected(ctx, syntax, ctx.GetErrorType(), "ERROR: %s is not supported on '%.*s'", (syntax.isIncrement ? "increment" : "decrement"), FMT_ISTR(value.type.name));

	return new ExprPreModify(syntax, value.type, wrapped, syntax.isIncrement);
}

ExprBase ref AnalyzePostModify(ExpressionContext ref ctx, SynPostModify ref syntax)
{
	ExprBase ref value = AnalyzeExpression(ctx, syntax.value);

	if(isType with<TypeError>(value.type))
		return new ExprPreModify(syntax, ctx.GetErrorType(), value, syntax.isIncrement);

	ExprBase ref wrapped = value;

	if(ExprVariableAccess ref node = getType with<ExprVariableAccess>(value))
		wrapped = new ExprGetAddress(syntax, ctx.GetReferenceType(value.type), new VariableHandle(node.source, node.variable));
	else if(ExprDereference ref node = getType with<ExprDereference>(value))
		wrapped = node.value;

	AssertValueExpression(ctx, syntax, value);

	if(!isType with<TypeRef>(wrapped.type))
		return ReportExpected(ctx, syntax, ctx.GetErrorType(), "ERROR: cannot change immutable value of type %.*s", FMT_ISTR(value.type.name));

	if(!ctx.IsNumericType(value.type))
		return ReportExpected(ctx, syntax, ctx.GetErrorType(), "ERROR: %s is not supported on '%.*s'", (syntax.isIncrement ? "increment" : "decrement"), FMT_ISTR(value.type.name));

	return new ExprPostModify(syntax, value.type, wrapped, syntax.isIncrement);
}

ExprBase ref AnalyzeUnaryOp(ExpressionContext ref ctx, SynUnaryOp ref syntax)
{
	ExprBase ref value = AnalyzeExpression(ctx, syntax.value);

	if(isType with<TypeError>(value.type))
		return new ExprUnaryOp(syntax, ctx.GetErrorType(), syntax.type, value);

	if(ExprBase ref result = CreateFunctionCall1(ctx, syntax, InplaceStr(GetOpName(syntax.type)), value, true, false, true))
		return result;

	AssertValueExpression(ctx, syntax, value);

	bool binaryOp = IsBinaryOp(syntax.type);
	bool logicalOp = IsLogicalOp(syntax.type);

	// Type check
	if(ctx.IsFloatingPointType(value.type))
	{
		if(binaryOp || logicalOp)
		{
			Report(ctx, syntax, "ERROR: unary operation '%s' is not supported on '%.*s'", GetOpName(syntax.type), FMT_ISTR(value.type.name));

			return new ExprUnaryOp(syntax, ctx.GetErrorType(), syntax.type, value);
		}
	}
	else if(value.type == ctx.typeBool || value.type == (TypeBase ref)(ctx.typeAutoRef))
	{
		if(!logicalOp)
		{
			Report(ctx, syntax, "ERROR: unary operation '%s' is not supported on '%.*s'", GetOpName(syntax.type), FMT_ISTR(value.type.name));

			return new ExprUnaryOp(syntax, ctx.GetErrorType(), syntax.type, value);
		}
	}
	else if(isType with<TypeRef>(value.type))
	{
		if(!logicalOp)
		{
			Report(ctx, syntax, "ERROR: unary operation '%s' is not supported on '%.*s'", GetOpName(syntax.type), FMT_ISTR(value.type.name));

			return new ExprUnaryOp(syntax, ctx.GetErrorType(), syntax.type, value);
		}
	}
	else if(!ctx.IsNumericType(value.type))
	{
		Report(ctx, syntax, "ERROR: unary operation '%s' is not supported on '%.*s'", GetOpName(syntax.type), FMT_ISTR(value.type.name));

		return new ExprUnaryOp(syntax, ctx.GetErrorType(), syntax.type, value);
	}

	TypeBase ref resultType = nullptr;

	if(logicalOp)
		resultType = ctx.typeBool;
	else
		resultType = value.type;

	return new ExprUnaryOp(syntax, resultType, syntax.type, value);
}

ExprBase ref AnalyzeBinaryOp(ExpressionContext ref ctx, SynBinaryOp ref syntax)
{
	ExprBase ref lhs = AnalyzeExpression(ctx, syntax.lhs);

	// For && and || try to find a function that accepts a wrapped right-hand-side evaluation
	if((syntax.type == SynBinaryOpType.SYN_BINARY_OP_LOGICAL_AND || syntax.type == SynBinaryOpType.SYN_BINARY_OP_LOGICAL_OR) && isType with<TypeClass>(lhs.type))
	{
		if(ExprBase ref result = CreateFunctionCall2(ctx, syntax, InplaceStr(GetOpName(syntax.type)), lhs, CreateValueFunctionWrapper(ctx, syntax, syntax.rhs, nullptr, GetTemporaryFunctionName(ctx)), true, false, true))
			return result;
	}

	ExprBase ref rhs = AnalyzeExpression(ctx, syntax.rhs);

	if(isType with<TypeError>(lhs.type) || isType with<TypeError>(rhs.type))
		return new ExprBinaryOp(syntax, ctx.GetErrorType(), syntax.type, lhs, rhs);

	return CreateBinaryOp(ctx, syntax, syntax.type, lhs, rhs);
}

ExprBase ref CreateGetAddress(ExpressionContext ref ctx, SynBase ref source, ExprBase ref value)
{
	AssertValueExpression(ctx, source, value);

	if(ExprVariableAccess ref node = getType with<ExprVariableAccess>(value))
	{
		return new ExprGetAddress(source, ctx.GetReferenceType(value.type), new VariableHandle(node.source, node.variable));
	}
	else if(ExprDereference ref node = getType with<ExprDereference>(value))
	{
		return node.value;
	}

	Stop(ctx, source, "ERROR: cannot get address of the expression");

	return nullptr;
}

ExprBase ref AnalyzeGetAddress(ExpressionContext ref ctx, SynGetAddress ref syntax)
{
	ExprBase ref value = AnalyzeExpression(ctx, syntax.value);

	if(isType with<TypeError>(value.type))
		return new ExprError(syntax, ctx.GetErrorType(), value);

	return CreateGetAddress(ctx, syntax, value);
}

ExprBase ref AnalyzeDereference(ExpressionContext ref ctx, SynDereference ref syntax)
{
	ExprBase ref value = AnalyzeExpression(ctx, syntax.value);

	if(isType with<TypeError>(value.type))
		return new ExprDereference(syntax, ctx.GetErrorType(), value);

	if(TypeRef ref type = getType with<TypeRef>(value.type))
	{
		if(isType with<TypeVoid>(type.subType))
			Stop(ctx, syntax, "ERROR: cannot dereference type '%.*s'", FMT_ISTR(value.type.name));

		if(TypeClass ref typeClass = getType with<TypeClass>(type.subType))
		{
			if(!typeClass.completed)
				Stop(ctx, syntax, "ERROR: type '%.*s' is not fully defined", FMT_ISTR(typeClass.name));
		}

		return new ExprDereference(syntax, type.subType, value);
	}

	if(isType with<TypeAutoRef>(value.type))
	{
		return new ExprUnboxing(syntax, ctx.typeAutoRef, value);
	}

	Stop(ctx, syntax, "ERROR: cannot dereference type '%.*s' that is not a pointer", FMT_ISTR(value.type.name));

	return nullptr;
}

ExprBase ref AnalyzeConditional(ExpressionContext ref ctx, SynConditional ref syntax)
{
	ExprBase ref condition = AnalyzeExpression(ctx, syntax.condition);

	condition = CreateConditionCast(ctx, condition.source, condition);

	AssertValueExpression(ctx, syntax, condition);

	ExprBase ref trueBlock = AnalyzeStatement(ctx, syntax.trueBlock);
	ExprBase ref falseBlock = AnalyzeStatement(ctx, syntax.falseBlock);

	if(isType with<TypeError>(condition.type) || isType with<TypeError>(trueBlock.type) || isType with<TypeError>(falseBlock.type))
		return new ExprConditional(syntax, ctx.GetErrorType(), condition, trueBlock, falseBlock);

	if(!AssertValueExpression(ctx, syntax, trueBlock))
		return new ExprConditional(syntax, ctx.GetErrorType(), condition, trueBlock, falseBlock);

	if(!AssertValueExpression(ctx, syntax, falseBlock))
		return new ExprConditional(syntax, ctx.GetErrorType(), condition, trueBlock, falseBlock);

	// Handle null pointer promotion
	if(trueBlock.type != falseBlock.type)
	{
		if(trueBlock.type == ctx.typeNullPtr)
			trueBlock = CreateCast(ctx, syntax.trueBlock, trueBlock, falseBlock.type, false);

		if(falseBlock.type == ctx.typeNullPtr)
			falseBlock = CreateCast(ctx, syntax.falseBlock, falseBlock, trueBlock.type, false);
	}

	TypeBase ref resultType = nullptr;

	if(trueBlock.type == falseBlock.type)
	{
		resultType = trueBlock.type;
	}
	else if(ctx.IsNumericType(trueBlock.type) && ctx.IsNumericType(falseBlock.type))
	{
		resultType = ctx.GetBinaryOpResultType(trueBlock.type, falseBlock.type);

		trueBlock = CreateCast(ctx, syntax.trueBlock, trueBlock, resultType, false);
		falseBlock = CreateCast(ctx, syntax.falseBlock, falseBlock, resultType, false);
	}
	else
	{
		TypeArray ref trueBlockTypeArray = getType with<TypeArray>(trueBlock.type);
		TypeArray ref falseBlockTypeArray = getType with<TypeArray>(falseBlock.type);

		if(trueBlockTypeArray && falseBlockTypeArray && trueBlockTypeArray.subType == falseBlockTypeArray.subType)
		{
			resultType = ctx.GetUnsizedArrayType(trueBlockTypeArray.subType);

			trueBlock = CreateCast(ctx, syntax.trueBlock, trueBlock, resultType, false);
			falseBlock = CreateCast(ctx, syntax.falseBlock, falseBlock, resultType, false);
		}
		else
		{
			Report(ctx, syntax, "ERROR: can't find common type between '%.*s' and '%.*s'", FMT_ISTR(trueBlock.type.name), FMT_ISTR(falseBlock.type.name));

			resultType = ctx.GetErrorType();
		}
	}

	return new ExprConditional(syntax, resultType, condition, trueBlock, falseBlock);
}

ExprBase ref AnalyzeAssignment(ExpressionContext ref ctx, SynAssignment ref syntax)
{
	ExprBase ref lhs = AnalyzeExpression(ctx, syntax.lhs);
	ExprBase ref rhs = AnalyzeExpression(ctx, syntax.rhs);

	if(isType with<TypeError>(lhs.type) || isType with<TypeError>(rhs.type))
		return new ExprAssignment(syntax, ctx.GetErrorType(), lhs, rhs);

	return CreateAssignment(ctx, syntax, lhs, rhs);
}

ExprBase ref AnalyzeModifyAssignment(ExpressionContext ref ctx, SynModifyAssignment ref syntax)
{
	ExprBase ref lhs = AnalyzeExpression(ctx, syntax.lhs);
	ExprBase ref rhs = AnalyzeExpression(ctx, syntax.rhs);

	if(isType with<TypeError>(lhs.type) || isType with<TypeError>(rhs.type))
		return new ExprError(syntax, ctx.GetErrorType(), lhs, rhs);

	if(ExprBase ref result = CreateFunctionCall2(ctx, syntax, InplaceStr(GetOpName(syntax.type)), lhs, rhs, true, false, true))
		return result;

	// Unwrap modifiable pointer
	ExprBase ref wrapped = lhs;

	if(ExprVariableAccess ref node = getType with<ExprVariableAccess>(lhs))
	{
		wrapped = new ExprGetAddress(lhs.source, ctx.GetReferenceType(lhs.type), new VariableHandle(node.source, node.variable));
	}
	else if(ExprDereference ref node = getType with<ExprDereference>(lhs))
	{
		wrapped = node.value;
	}
	else if(ExprFunctionCall ref node = getType with<ExprFunctionCall>(lhs))
	{
		// Will try to transform 'get' accessor to 'set'
		if(ExprFunctionAccess ref access = getType with<ExprFunctionAccess>(node.function))
		{
			if(access.function && access.function.accessor)
			{
				ExprBase ref result = CreateBinaryOp(ctx, syntax, GetBinaryOpType(syntax.type), lhs, rhs);

				if(isType with<TypeError>(result.type))
					return new ExprError(syntax, ctx.GetErrorType(), lhs, rhs);

				vector<ArgumentData> arguments;
				arguments.push_back(ArgumentData(syntax, false, nullptr, result.type, result));

				if(auto function = ctx.functionMap.first(access.function.nameHash))
				{
					ExprBase ref overloads = CreateFunctionAccess(ctx, syntax, function, access.context);

					if(ExprBase ref call = CreateFunctionCall(ctx, syntax, overloads, ArrayView<ArgumentData>(arguments), true))
						return call;
				}

				if(FunctionData ref proto = access.function.proto)
				{
					if(auto function = ctx.functionMap.first(proto.nameHash))
					{
						ExprBase ref overloads = CreateFunctionAccess(ctx, syntax, function, access.context);

						if(ExprBase ref call = CreateFunctionCall(ctx, syntax, overloads, ArrayView<ArgumentData>(arguments), true))
							return call;
					}
				}
			}
		}
	}

	TypeRef ref typeRef = getType with<TypeRef>(wrapped.type);

	if(!typeRef)
	{
		Report(ctx, syntax, "ERROR: cannot change immutable value of type %.*s", FMT_ISTR(lhs.type.name));

		return new ExprError(syntax, ctx.GetErrorType(), lhs, rhs);
	}

	VariableData ref storage = AllocateTemporary(ctx, syntax, wrapped.type);

	ExprBase ref assignment = CreateAssignment(ctx, syntax, CreateVariableAccess(ctx, syntax, storage, false), wrapped);

	ExprBase ref definition = new ExprVariableDefinition(syntax, ctx.typeVoid, new VariableHandle(nullptr, storage), assignment);

	ExprBase ref lhsValue = new ExprDereference(syntax, lhs.type, CreateVariableAccess(ctx, syntax, storage, false));

	ExprBase ref result = CreateBinaryOp(ctx, syntax, GetBinaryOpType(syntax.type), lhsValue, rhs);

	if(isType with<TypeError>(result.type))
		return new ExprError(syntax, ctx.GetErrorType(), lhs, rhs);

	return CreateSequence(ctx, syntax, definition, CreateAssignment(ctx, syntax, new ExprDereference(syntax, lhs.type, CreateVariableAccess(ctx, syntax, storage, false)), result));
}

ExprBase ref CreateTypeidMemberAccess(ExpressionContext ref ctx, SynBase ref source, TypeBase ref type, SynIdentifier ref member)
{
	if(!member)
		return new ExprErrorTypeMemberAccess(source, ctx.GetErrorType(), type);

	if(TypeClass ref classType = getType with<TypeClass>(type))
	{
		for(MatchData ref curr = classType.aliases.head; curr; curr = curr.next)
		{
			if(curr.name.name== member.name)
				return new ExprTypeLiteral(source, ctx.typeTypeID, curr.type);
		}

		for(MatchData ref curr = classType.generics.head; curr; curr = curr.next)
		{
			if(curr.name.name == member.name)
				return new ExprTypeLiteral(source, ctx.typeTypeID, curr.type);
		}
	}

	if(TypeStruct ref structType = getType with<TypeStruct>(type))
	{
		for(MemberHandle ref curr = structType.members.head; curr; curr = curr.next)
		{
			if(curr.variable.name.name == member.name)
				return new ExprTypeLiteral(source, ctx.typeTypeID, curr.variable.type);
		}

		for(ConstantData ref curr = structType.constants.head; curr; curr = curr.next)
		{
			if(curr.name.name == member.name)
				return CreateLiteralCopy(ctx, source, curr.value);
		}

		if(member.name == InplaceStr("hasMember"))
			return new ExprTypeLiteral(source, ctx.typeTypeID, new TypeMemberSet(GetMemberSetTypeName(ctx, structType), structType));
	}

	if(TypeGenericClass ref typeGenericClass = getType with<TypeGenericClass>(type))
	{
		for(SynIdentifier ref curr = typeGenericClass.proto.definition.aliases.head; curr; curr = getType with<SynIdentifier>(curr.next))
		{
			if(curr.name == member.name)
				return new ExprTypeLiteral(source, ctx.typeTypeID, ctx.typeGeneric);
		}
	}

	if(member.name == InplaceStr("isReference"))
	{
		return new ExprBoolLiteral(source, ctx.typeBool, isType with<TypeRef>(type));
	}

	if(member.name == InplaceStr("isArray"))
	{
		return new ExprBoolLiteral(source, ctx.typeBool, isType with<TypeArray>(type) || isType with<TypeUnsizedArray>(type));
	}

	if(member.name == InplaceStr("isFunction"))
	{
		return new ExprBoolLiteral(source, ctx.typeBool, isType with<TypeFunction>(type));
	}

	if(member.name == InplaceStr("arraySize"))
	{
		if(isType with<TypeError>(type))
			return new ExprError(source, ctx.GetErrorType());

		if(TypeArray ref arrType = getType with<TypeArray>(type))
			return new ExprIntegerLiteral(source, ctx.typeInt, arrType.length);

		if(isType with<TypeUnsizedArray>(type))
			return new ExprIntegerLiteral(source, ctx.typeInt, -1);

		Report(ctx, source, "ERROR: 'arraySize' can only be applied to an array type, but we have '%.*s'", FMT_ISTR(type.name));
	}

	if(member.name == InplaceStr("size"))
	{
		if(isType with<TypeError>(type))
			return new ExprError(source, ctx.GetErrorType());

		if(TypeArgumentSet ref argumentsType = getType with<TypeArgumentSet>(type))
			return new ExprIntegerLiteral(source, ctx.typeInt, argumentsType.types.size());

		Report(ctx, source, "ERROR: 'size' can only be applied to a function type, but we have '%.*s'", FMT_ISTR(type.name));
	}

	if(member.name == InplaceStr("argument"))
	{
		if(isType with<TypeError>(type))
			return new ExprError(source, ctx.GetErrorType());

		if(TypeFunction ref functionType = getType with<TypeFunction>(type))
			return new ExprTypeLiteral(source, ctx.typeTypeID, new TypeArgumentSet(GetArgumentSetTypeName(ctx, functionType.arguments), functionType.arguments));

		Report(ctx, source, "ERROR: 'argument' can only be applied to a function type, but we have '%.*s'", FMT_ISTR(type.name));
	}

	if(member.name == InplaceStr("return"))
	{
		if(isType with<TypeError>(type))
			return new ExprError(source, ctx.GetErrorType());

		if(TypeFunction ref functionType = getType with<TypeFunction>(type))
			return new ExprTypeLiteral(source, ctx.typeTypeID, functionType.returnType);

		Report(ctx, source, "ERROR: 'return' can only be applied to a function type, but we have '%.*s'", FMT_ISTR(type.name));
	}

	if(member.name == InplaceStr("target"))
	{
		if(isType with<TypeError>(type))
			return new ExprError(source, ctx.GetErrorType());

		if(TypeRef ref refType = getType with<TypeRef>(type))
			return new ExprTypeLiteral(source, ctx.typeTypeID, refType.subType);

		if(TypeArray ref arrType = getType with<TypeArray>(type))
			return new ExprTypeLiteral(source, ctx.typeTypeID, arrType.subType);

		if(TypeUnsizedArray ref arrType = getType with<TypeUnsizedArray>(type))
			return new ExprTypeLiteral(source, ctx.typeTypeID, arrType.subType);

		Report(ctx, source, "ERROR: 'target' can only be applied to a pointer or array type, but we have '%.*s'", FMT_ISTR(type.name));
	}

	if(member.name == InplaceStr("first"))
	{
		if(isType with<TypeError>(type))
			return new ExprError(source, ctx.GetErrorType());

		if(TypeArgumentSet ref argumentsType = getType with<TypeArgumentSet>(type))
		{
			if(argumentsType.types.empty())
			{
				Report(ctx, source, "ERROR: function argument set is empty");

				return nullptr;
			}

			return new ExprTypeLiteral(source, ctx.typeTypeID, argumentsType.types.head.type);
		}

		Report(ctx, source, "ERROR: 'first' can only be applied to a function type, but we have '%.*s'", FMT_ISTR(type.name));
	}

	if(member.name == InplaceStr("last"))
	{
		if(isType with<TypeError>(type))
			return new ExprError(source, ctx.GetErrorType());

		if(TypeArgumentSet ref argumentsType = getType with<TypeArgumentSet>(type))
		{
			if(argumentsType.types.empty())
			{
				Report(ctx, source, "ERROR: function argument set is empty");

				return nullptr;
			}

			return new ExprTypeLiteral(source, ctx.typeTypeID, argumentsType.types.tail.type);
		}

		Report(ctx, source, "ERROR: 'last' can only be applied to a function type, but we have '%.*s'", FMT_ISTR(type.name));
	}

	return nullptr;
}

ExprBase ref CreateAutoRefFunctionSet(ExpressionContext ref ctx, SynBase ref source, ExprBase ref value, InplaceStr name, TypeClass ref preferredParent)
{
	vector<TypeBase ref> types;
	RefList<FunctionHandle> functions;

	// Find all member functions with the specified name
	for(int i = 0; i < ctx.functions.size(); i++)
	{
		FunctionData ref function = ctx.functions[i];

		TypeBase ref parentType = function.scope.ownerType;

		if(!parentType)
			continue;

		int hash = StringHashContinue(parentType.nameHash, "::");

		hash = StringHashContinue(hash, name);

		if(function.nameHash != hash)
			continue;

		// Can't specify generic function arguments for call through 'auto ref'
		if(!function.generics.empty())
			continue;

		// Ignore generic types if they don't have a single instance
		if(function.scope.ownerType.isGeneric)
		{
			if(TypeGenericClassProto ref proto = getType with<TypeGenericClassProto>(function.scope.ownerType))
			{
				if(proto.instances.empty())
					continue;
			}
		}

		if(preferredParent && !IsDerivedFrom(preferredParent, getType with<TypeClass>(parentType)))
			continue;

		FunctionHandle ref prev = nullptr;

		// Pointer to generic types don't stricly match because they might be resolved to different types
		if (!function.type.isGeneric)
		{
			for(FunctionHandle ref curr = functions.head; curr; curr = curr.next)
			{
				if(curr.function.type == function.type)
				{
					prev = curr;
					break;
				}
			}
		}

		if(prev)
		{
			// Select the most specialized function for extendable member function call
			if(preferredParent)
			{
				int prevDepth = GetDerivedFromDepth(preferredParent, getType with<TypeClass>(prev.function.scope.ownerType));
				int currDepth = GetDerivedFromDepth(preferredParent, getType with<TypeClass>(function.scope.ownerType));

				if (currDepth < prevDepth)
					prev.function = function;
			}

			continue;
		}

		types.push_back(function.type);
		functions.push_back(new FunctionHandle(function));
	}

	if(functions.empty())
	{
		if(value.type != (TypeBase ref)(ctx.typeAutoRef))
			return nullptr;

		Stop(ctx, source, "ERROR: function '%.*s' is undefined in any of existing classes", FMT_ISTR(name));
	}

	TypeFunctionSet ref type = ctx.GetFunctionSetType(ArrayView<TypeBase ref>(types));

	return new ExprFunctionOverloadSet(source, type, functions, value);
}

ExprBase ref CreateMemberAccess(ExpressionContext ref ctx, SynBase ref source, ExprBase ref value, SynIdentifier ref member, bool allowFailure)
{
	ExprBase ref wrapped = value;

	if(TypeRef ref refType = getType with<TypeRef>(value.type))
	{
		value = new ExprDereference(source, refType.subType, value);

		if(TypeRef ref refType = getType with<TypeRef>(value.type))
		{
			wrapped = value;

			value = new ExprDereference(source, refType.subType, value);
		}
	}
	else if(ExprVariableAccess ref node = getType with<ExprVariableAccess>(value))
	{
		wrapped = new ExprGetAddress(source, ctx.GetReferenceType(value.type), new VariableHandle(node.source, node.variable));
	}
	else if(ExprDereference ref node = getType with<ExprDereference>(value))
	{
		wrapped = node.value;
	}
	else if(!isType with<TypeRef>(wrapped.type))
	{
		if(!AssertValueExpression(ctx, source, wrapped))
			return new ExprMemberAccess(source, ctx.GetErrorType(), value, nullptr);

		SynBase ref sourceInternal = ctx.MakeInternal(source);

		VariableData ref storage = AllocateTemporary(ctx, sourceInternal, wrapped.type);

		ExprBase ref assignment = CreateAssignment(ctx, sourceInternal, CreateVariableAccess(ctx, sourceInternal, storage, false), value);

		ExprBase ref definition = new ExprVariableDefinition(sourceInternal, ctx.typeVoid, new VariableHandle(nullptr, storage), assignment);

		wrapped = CreateSequence(ctx, source, definition, CreateGetAddress(ctx, sourceInternal, CreateVariableAccess(ctx, sourceInternal, storage, false)));
	}

	if(!member)
		return new ExprMemberAccess(source, ctx.GetErrorType(), value, nullptr);

	if(TypeArray ref node = getType with<TypeArray>(value.type))
	{
		if(member.name == InplaceStr("size"))
			return new ExprIntegerLiteral(source, ctx.typeInt, node.length);
	}

	if(isType with<TypeRef>(wrapped.type))
	{
		if(ExprTypeLiteral ref node = getType with<ExprTypeLiteral>(value))
		{
			if(ExprBase ref result = CreateTypeidMemberAccess(ctx, source, node.value, member))
				return result;
		}

		if(TypeStruct ref node = getType with<TypeStruct>(value.type))
		{
			// Search for a member variable
			for(MemberHandle ref el = node.members.head; el; el = el.next)
			{
				if(el.variable.name.name == member.name)
				{
					// Member access only shifts an address, so we are left with a reference to get value from
					ExprMemberAccess ref shift = new ExprMemberAccess(source, ctx.GetReferenceType(el.variable.type), wrapped, new VariableHandle(member, el.variable));

					ExprBase ref memberValue = new ExprDereference(source, el.variable.type, shift);

					if(el.variable.isReadonly)
						return new ExprPassthrough(memberValue.source, memberValue.type, memberValue);

					return memberValue;
				}
			}

			// Search for a member constant
			for(ConstantData ref curr = node.constants.head; curr; curr = curr.next)
			{
				if(curr.name.name == member.name)
					return CreateLiteralCopy(ctx, source, curr.value);
			}
		}

		if(value.type == (TypeBase ref)(ctx.typeAutoRef))
			return CreateAutoRefFunctionSet(ctx, source, value, member.name, nullptr);

		if(TypeClass ref classType = getType with<TypeClass>(value.type))
		{
			if(classType.baseClass != nullptr || classType.isExtendable)
			{
				if(ExprBase ref overloads = CreateAutoRefFunctionSet(ctx, source, wrapped, member.name, classType))
					return overloads;
			}
		}

		// Check if a name resembles a type alias of the value class
		TypeBase ref aliasType = nullptr;

		if(TypeBase ref ref typeName = ctx.typeMap.find(member.name.hash()))
		{
			TypeBase ref type = *typeName;

			if(type == value.type && type.name != member.name)
			{
				if(TypeClass ref typeClass = getType with<TypeClass>(type))
				{
					if(typeClass.proto)
						type = typeClass.proto;
				}

				aliasType = type;
			}
		}

		// Look for a member function
		ExprBase ref mainFuncton = nullptr;

		int hash = StringHashContinue(value.type.nameHash, "::");

		hash = StringHashContinue(hash, member.name);

		if(auto function = ctx.functionMap.first(hash))
			mainFuncton = CreateFunctionAccess(ctx, source, function, wrapped);

		if(!mainFuncton && aliasType)
			mainFuncton = CreateConstructorAccess(ctx, source, value.type, false, wrapped);

		if(!mainFuncton)
		{
			if(TypeArray ref node = getType with<TypeArray>(value.type))
			{
				TypeUnsizedArray ref arrayType = ctx.GetUnsizedArrayType(node.subType);

				int hash = StringHashContinue(arrayType.nameHash, "::");

				hash = StringHashContinue(hash, member.name);

				if(auto function = ctx.functionMap.first(hash))
				{
					wrapped = CreateCast(ctx, source, wrapped, ctx.GetReferenceType(arrayType), false);

					return CreateFunctionAccess(ctx, source, function, wrapped);
				}
			}
		}

		ExprBase ref baseFunction = nullptr;

		// Look for a member function in a generic class base
		if(TypeClass ref classType = getType with<TypeClass>(value.type))
		{
			if(TypeGenericClassProto ref protoType = classType.proto)
			{
				int hash = StringHashContinue(protoType.nameHash, "::");

				hash = StringHashContinue(hash, member.name);

				if(auto function = ctx.functionMap.first(hash))
					baseFunction = CreateFunctionAccess(ctx, source, function, wrapped);

				if(!baseFunction && aliasType)
					baseFunction = CreateConstructorAccess(ctx, source, protoType, false, wrapped);
			}
		}

		// Add together instantiated and generic base functions
		if(mainFuncton && baseFunction)
		{
			vector<TypeBase ref> types;
			RefList<FunctionHandle> overloads;

			// Collect a set of available functions
			vector<FunctionValue> functions;

			GetNodeFunctions(ctx, source, mainFuncton, functions);
			GetNodeFunctions(ctx, source, baseFunction, functions);

			for(int i = 0; i < functions.size(); i++)
			{
				FunctionValue function = functions[i];

				bool instantiated = false;

				for(FunctionHandle ref curr = overloads.head; curr && !instantiated; curr = curr.next)
				{
					if(SameArguments(curr.function.type, function.function.type))
						instantiated = true;
				}

				if(instantiated)
					continue;

				types.push_back(function.function.type);
				overloads.push_back(new FunctionHandle(function.function));
			}

			TypeFunctionSet ref type = ctx.GetFunctionSetType(ArrayView<TypeBase ref>(types));

			return new ExprFunctionOverloadSet(source, type, overloads, wrapped);
		}

		if(mainFuncton)
			return mainFuncton;

		if(baseFunction)
		{
			if(ExprFunctionAccess ref node = getType with<ExprFunctionAccess>(baseFunction))
			{
				if(node.function.scope.ownerType && node.function.scope.ownerType.isGeneric && !node.function.type.isGeneric)
				{
					if(FunctionValue bestOverload = GetFunctionForType(ctx, source, baseFunction, node.function.type))
						return new ExprFunctionAccess(bestOverload.source, bestOverload.function.type, bestOverload.function, bestOverload.context);
				}
			}

			return baseFunction;
		}

		// Look for an accessor
		hash = StringHashContinue(hash, "$");

		ExprBase ref access = nullptr;

		if(auto function = ctx.functionMap.first(hash))
			access = CreateFunctionAccess(ctx, source, function, wrapped);

		if(!access)
		{
			if(TypeArray ref node = getType with<TypeArray>(value.type))
			{
				TypeUnsizedArray ref arrayType = ctx.GetUnsizedArrayType(node.subType);

				int hash = StringHashContinue(arrayType.nameHash, "::");

				hash = StringHashContinue(hash, member.name);

				if(auto function = ctx.functionMap.first(hash))
				{
					wrapped = CreateCast(ctx, source, wrapped, ctx.GetReferenceType(arrayType), false);

					access = CreateFunctionAccess(ctx, source, function, wrapped);
				}
			}
		}

		if(access)
		{
			ExprBase ref call = CreateFunctionCall(ctx, source, access, RefList<TypeHandle>(), nullptr, false);

			if(TypeRef ref refType = getType with<TypeRef>(call.type))
				return new ExprDereference(source, refType.subType, call);

			return call;
		}

		// Look for an accessor function in a generic class base
		if(TypeClass ref classType = getType with<TypeClass>(value.type))
		{
			if(TypeGenericClassProto ref protoType = classType.proto)
			{
				int hash = StringHashContinue(protoType.nameHash, "::");

				hash = StringHashContinue(hash, member.name);

				// Look for an accessor
				hash = StringHashContinue(hash, "$");

				if(auto function = ctx.functionMap.first(hash))
				{
					ExprBase ref access = CreateFunctionAccess(ctx, source, function, wrapped);

					ExprBase ref call = CreateFunctionCall(ctx, source, access, RefList<TypeHandle>(), nullptr, false);

					if(TypeRef ref refType = getType with<TypeRef>(call.type))
						return new ExprDereference(source, refType.subType, call);

					return call;
				}
			}
		}

		if(allowFailure)
			return nullptr;

		Report(ctx, source, "ERROR: member variable or function '%.*s' is not defined in class '%.*s'", FMT_ISTR(member.name), FMT_ISTR(value.type.name));

		return new ExprMemberAccess(source, ctx.GetErrorType(), value, nullptr);
	}

	Stop(ctx, source, "ERROR: can't access member '%.*s' of type '%.*s'", FMT_ISTR(member.name), FMT_ISTR(value.type.name));

	return nullptr;
}

ExprBase ref CreateArrayIndex(ExpressionContext ref ctx, SynBase ref source, ExprBase ref value, ArrayView<ArgumentData> arguments)
{
	// Handle argument[x] expresion
	if(ExprTypeLiteral ref type = getType with<ExprTypeLiteral>(value))
	{
		if(TypeArgumentSet ref argumentSet = getType with<TypeArgumentSet>(type.value))
		{
			if(arguments.size() == 1)
			{
				if(ExprIntegerLiteral ref number = getType with<ExprIntegerLiteral>(EvaluateExpression(ctx, source, arguments[0].value)))
				{
					if(number.value < 0)
						Stop(ctx, source, "ERROR: argument index can't be negative");

					if(argumentSet.types.empty())
						Stop(ctx, source, "ERROR: function argument set is empty");

					if(number.value >= argumentSet.types.size())
						Stop(ctx, source, "ERROR: function arguemnt set '%.*s' has only %d argument(s)", FMT_ISTR(argumentSet.name), argumentSet.types.size());

					return new ExprTypeLiteral(source, ctx.typeTypeID, argumentSet.types[int(number.value)].type);
				}
				else
				{
					Stop(ctx, source, "ERROR: expression didn't evaluate to a constant number");
				}
			}
		}
	}

	ExprBase ref wrapped = value;

	if(TypeRef ref refType = getType with<TypeRef>(value.type))
	{
		value = new ExprDereference(source, refType.subType, value);

		if(isType with<TypeUnsizedArray>(value.type))
			wrapped = value;
	}
	else if(isType with<TypeUnsizedArray>(value.type))
	{
		wrapped = value; // Do not modify
	}
	else if(ExprVariableAccess ref node = getType with<ExprVariableAccess>(value))
	{
		wrapped = new ExprGetAddress(source, ctx.GetReferenceType(value.type), new VariableHandle(node.source, node.variable));
	}
	else if(ExprDereference ref node = getType with<ExprDereference>(value))
	{
		wrapped = node.value;
	}
	else if(!isType with<TypeRef>(wrapped.type))
	{
		if(!AssertValueExpression(ctx, source, wrapped))
			return new ExprArrayIndex(source, ctx.GetErrorType(), value, new ExprError(source, ctx.GetErrorType()));

		SynBase ref sourceInternal = ctx.MakeInternal(source);

		VariableData ref storage = AllocateTemporary(ctx, sourceInternal, wrapped.type);

		ExprBase ref assignment = CreateAssignment(ctx, sourceInternal, CreateVariableAccess(ctx, sourceInternal, storage, false), value);

		ExprBase ref definition = new ExprVariableDefinition(sourceInternal, ctx.typeVoid, new VariableHandle(nullptr, storage), assignment);

		wrapped = CreateSequence(ctx, source, definition, CreateGetAddress(ctx, sourceInternal, CreateVariableAccess(ctx, sourceInternal, storage, false)));
	}

	if(isType with<TypeRef>(wrapped.type) || isType with<TypeUnsizedArray>(value.type))
	{
		bool findOverload = arguments.empty() || arguments.size() > 1;

		for(int i = 0; i < arguments.size(); i++)
		{
			if(arguments[i].name)
				findOverload = true;
		}

		if(ExprBase ref overloads = CreateVariableAccess(ctx, source, RefList<SynIdentifier>(), InplaceStr("[]"), false))
		{
			vector<ArgumentData> callArguments;
			callArguments.push_back(ArgumentData(wrapped.source, false, nullptr, wrapped.type, wrapped));

			for(int i = 0; i < arguments.size(); i++)
				callArguments.push_back(arguments[i]);

			if(ExprBase ref result = CreateFunctionCall(ctx, source, overloads, ArrayView<ArgumentData>(callArguments), true))
			{
				if(TypeRef ref refType = getType with<TypeRef>(result.type))
					return new ExprDereference(source, refType.subType, result);

				return result;
			}

			callArguments[0] = ArgumentData(value.source, false, nullptr, value.type, value);

			if(ExprBase ref result = CreateFunctionCall(ctx, source, overloads, ArrayView<ArgumentData>(callArguments), !findOverload))
			{
				if(TypeRef ref refType = getType with<TypeRef>(result.type))
					return new ExprDereference(source, refType.subType, result);

				return result;
			}
		}

		if(findOverload)
			Stop(ctx, source, "ERROR: overloaded '[]' operator is not available");

		ExprBase ref index = CreateCast(ctx, source, arguments[0].value, ctx.typeInt, false);

		ExprIntegerLiteral ref indexValue = getType with<ExprIntegerLiteral>(EvaluateExpression(ctx, source, index));

		if(indexValue && indexValue.value < 0)
			Stop(ctx, source, "ERROR: array index cannot be negative");

		if(TypeArray ref type = getType with<TypeArray>(value.type))
		{
			if(indexValue && indexValue.value >= type.length)
				Stop(ctx, source, "ERROR: array index out of bounds");

			// Array index only shifts an address, so we are left with a reference to get value from
			ExprArrayIndex ref shift = new ExprArrayIndex(source, ctx.GetReferenceType(type.subType), wrapped, index);

			return new ExprDereference(source, type.subType, shift);
		}

		if(TypeUnsizedArray ref type = getType with<TypeUnsizedArray>(value.type))
		{
			// Array index only shifts an address, so we are left with a reference to get value from
			ExprArrayIndex ref shift = new ExprArrayIndex(source, ctx.GetReferenceType(type.subType), wrapped, index);

			return new ExprDereference(source, type.subType, shift);
		}
	}

	Stop(ctx, source, "ERROR: type '%.*s' is not an array", FMT_ISTR(value.type.name));

	return nullptr;
}

ExprBase ref AnalyzeArrayIndex(ExpressionContext ref ctx, SynArrayIndex ref syntax)
{
	ExprBase ref value = AnalyzeExpression(ctx, syntax.value);

	vector<ArgumentData> arguments;

	for(SynCallArgument ref curr = syntax.arguments.head; curr; curr = getType with<SynCallArgument>(curr.next))
	{
		ExprBase ref index = AnalyzeExpression(ctx, curr.value);

		arguments.push_back(ArgumentData(index.source, false, curr.name, index.type, index));
	}

	if(isType with<TypeError>(value.type))
	{
		vector<ExprBase ref> values;

		values.push_back(value);

		for(int i = 0; i < arguments.size(); i++)
			values.push_back(arguments[i].value);

		return new ExprError(syntax, ctx.GetErrorType(), values);
	}

	for(int i = 0; i < arguments.size(); i++)
	{
		if(isType with<TypeError>(arguments[i].value.type))
		{
			vector<ExprBase ref> values;

			values.push_back(value);

			for(int i = 0; i < arguments.size(); i++)
				values.push_back(arguments[i].value);

			return new ExprError(syntax, ctx.GetErrorType(), values);
		}
	}

	return CreateArrayIndex(ctx, syntax, value, ViewOf(arguments));
}

ExprBase ref AnalyzeArrayIndex(ExpressionContext ref ctx, SynTypeArray ref syntax)
{
	assert(syntax.sizes.head != nullptr);

	SynArrayIndex ref value = nullptr;

	// Convert to a chain of SynArrayIndex
	for(SynBase ref el = syntax.sizes.head; el; el = el.next)
	{
		RefList<SynCallArgument> arguments;

		if(!isType with<SynNothing>(el))
			arguments.push_back(new SynCallArgument(el.begin, el.end, nullptr, el));

		value = new SynArrayIndex(el.begin, el.end, value ? value : syntax.type, arguments);
	}

	return AnalyzeArrayIndex(ctx, value);
}

InplaceStr GetTemporaryFunctionName(ExpressionContext ref ctx)
{
	char[] name = new char[16];
	sprintf(name, "$func%d", ctx.unnamedFuncCount++);

	return InplaceStr(name);
}

InplaceStr GetDefaultArgumentWrapperFunctionName(ExpressionContext ref ctx, FunctionData ref function, InplaceStr argumentName)
{
	char[] name = new char[function.name.name.length() + argumentName.length() + 16];
	sprintf(name, "%.*s_%u_%.*s$", FMT_ISTR(function.name.name), function.type.nameHash, FMT_ISTR(argumentName));

	return InplaceStr(name);
}

InplaceStr GetFunctionName(ExpressionContext ref ctx, ScopeData ref scope, TypeBase ref parentType, InplaceStr name, bool isOperator, bool isAccessor)
{
	if(name.empty())
		return GetTemporaryFunctionName(ctx);

	return GetFunctionNameInScope(ctx, scope, parentType, name, isOperator, isAccessor);
}

bool HasNamedCallArguments(ArrayView<ArgumentData> arguments)
{
	for(int i = 0, e = arguments.count; i < e; i++)
	{
		if(arguments.data[i].name)
			return true;
	}

	return false;
}

bool HasMatchingArgumentNames(ArrayView<ArgumentData> ref functionArguments, ArrayView<ArgumentData> arguments)
{
	for(int i = 0, e = arguments.count; i < e; i++)
	{
		if(!arguments.data[i].name)
			continue;

		InplaceStr argumentName = arguments.data[i].name.name;

		bool found = false;

		for(int k = 0; k < functionArguments.count; k++)
		{
			if(!functionArguments.data[k].name)
				continue;

			InplaceStr functionArgumentName = functionArguments.data[k].name.name;

			if(functionArgumentName == argumentName)
			{
				found = true;
				break;
			}
		}

		if(!found)
			return false;
	}

	return true;
}

bool PrepareArgumentsForFunctionCall(ExpressionContext ref ctx, SynBase ref source, ArrayView<ArgumentData> functionArguments, ArrayView<ArgumentData> arguments, vector<CallArgumentData> ref result, int ref extraRating, bool prepareValues)
{
	result.clear();

	if(HasNamedCallArguments(arguments))
	{
		if(!HasMatchingArgumentNames(functionArguments, arguments))
			return false;

		// Add first unnamed arguments
		for(int i = 0; i < arguments.count; i++)
		{
			ArgumentData ref argument = &arguments.data[i];

			if(!argument.name)
				result.push_back(CallArgumentData(argument.type, argument.value));
			else
				break;
		}

		int unnamedCount = result.size();

		// Reserve slots for all remaining arguments
		for(int i = unnamedCount; i < functionArguments.count; i++)
			result.push_back(CallArgumentData(nullptr, nullptr));

		// Put named arguments in appropriate slots
		for(int i = unnamedCount; i < arguments.count; i++)
		{
			ArgumentData ref argument = &arguments.data[i];

			if(!argument.name)
				continue;

			int targetPos = 0;

			for(int k = 0; k < functionArguments.count; k++)
			{
				ArgumentData ref functionArgument = &functionArguments.data[k];

				if(functionArgument.name && functionArgument.name.name == argument.name.name)
				{
					if(result[targetPos].type != nullptr)
						Stop(ctx, argument.value.source, "ERROR: argument '%.*s' is already set", FMT_ISTR(argument.name.name));

					result[targetPos] = CallArgumentData(argument.type, argument.value);
					break;
				}

				targetPos++;
			}
		}

		// Fill in any unset arguments with default values
		for(int i = 0; i < functionArguments.count; i++)
		{
			ArgumentData ref argument = &functionArguments.data[i];

			if(result[i].type == nullptr)
			{
				if(ExprBase ref value = argument.value)
					result[i] = CallArgumentData(value.type, new ExprPassthrough(value.source, value.type, value));
			}
		}

		// All arguments must be set
		for(int i = unnamedCount; i < functionArguments.count; i++)
		{
			if(result[i].type == nullptr)
				return false;
		}
	}
	else
	{
		// Add arguments
		for(int i = 0; i < arguments.count; i++)
		{
			ArgumentData ref argument = &arguments.data[i];

			result.push_back(CallArgumentData(argument.type, argument.value));
		}

		// Add any arguments with default values
		for(int i = result.count; i < functionArguments.count; i++)
		{
			ArgumentData ref argument = &functionArguments.data[i];

			if(ExprBase ref value = argument.value)
				result.push_back(CallArgumentData(value.type, new ExprPassthrough(value.source, value.type, value)));
		}

		// Create variadic pack if neccessary
		TypeBase ref varArgType = ctx.GetUnsizedArrayType(ctx.typeAutoRef);

		if(!functionArguments.empty() && functionArguments.back().type == varArgType && !functionArguments.back().isExplicit)
		{
			if(result.size() >= functionArguments.size() - 1 && !(result.size() == functionArguments.size() && result.back().type == varArgType))
			{
				if(extraRating)
					*extraRating = 10 + (result.size() - functionArguments.size() - 1) * 5;

				ExprBase ref value = nullptr;

				if(prepareValues)
				{
					if(!result.empty())
						source = result[0].value.source;

					RefList<ExprBase> values;

					for(int i = functionArguments.size() - 1; i < result.size(); i++)
					{
						ExprBase ref value = result[i].value;

						if(TypeArray ref arrType = getType with<TypeArray>(value.type))
						{
							// type[N] is converted to type[] first
							value = CreateCast(ctx, value.source, value, ctx.GetUnsizedArrayType(arrType.subType), false);
						}

						values.push_back(CreateCast(ctx, value.source, value, ctx.typeAutoRef, true));
					}

					if(values.empty())
						value = new ExprNullptrLiteral(source, ctx.typeNullPtr);
					else
						value = new ExprArray(source, ctx.GetArrayType(ctx.typeAutoRef, values.size()), values);

					value = CreateCast(ctx, source, value, varArgType, true);
				}

				result.shrink(functionArguments.size() - 1);
				result.push_back(CallArgumentData(varArgType, value));
			}
		}
	}

	if(result.size() != functionArguments.size())
		return false;

	// Convert all arguments to target type if this is a real call
	if(prepareValues)
	{
		for(int i = 0; i < result.count; i++)
		{
			CallArgumentData ref argument = &result.data[i];

			assert(argument.value != nullptr);

			TypeBase ref target = functionArguments.data[i].type;

			argument.value = CreateCast(ctx, argument.value.source, argument.value, target, true);
		}
	}

	return true;
}

int GetFunctionRating(ExpressionContext ref ctx, FunctionData ref function, TypeFunction ref instance, ArrayView<CallArgumentData> arguments)
{
	if(function.arguments.size() != arguments.size())
		return -1;	// Definitely, this isn't the function we are trying to call. Argument count does not match.

	int rating = 0;

	int i = 0;

	for(TypeHandle ref argType = instance.arguments.head; argType; { argType = argType.next; i++; })
	{
		ArgumentData ref expectedArgument = &function.arguments[i];
		TypeBase ref expectedType = argType.type;

		CallArgumentData ref actualArgument = &arguments[i];
		TypeBase ref actualType = actualArgument.type;

		if(expectedType != actualType)
		{
			if(actualType == ctx.typeNullPtr)
			{
				// nullptr is convertable to T ref, T[] and function pointers
				if(isType with<TypeRef>(expectedType) || isType with<TypeUnsizedArray>(expectedType) || isType with<TypeFunction>(expectedType))
					continue;

				// nullptr is also convertable to auto ref and auto[], but it has the same rating as type ref . auto ref and array . auto[] defined below
				if(expectedType == (TypeBase ref)(ctx.typeAutoRef) || expectedType == (TypeBase ref)(ctx.typeAutoArray))
				{
					rating += 5;
					continue;
				}
			}

			// Generic function argument
			if(expectedType.isGeneric)
				continue;

			if(expectedArgument.isExplicit)
			{
				if(TypeFunction ref target = getType with<TypeFunction>(expectedType))
				{
					if(actualArgument.value && (isType with<TypeFunction>(actualArgument.type) || isType with<TypeFunctionSet>(actualArgument.type)))
					{
						if(FunctionValue functionValue = GetFunctionForType(ctx, actualArgument.value.source, actualArgument.value, target))
							continue;
					}
				}

				return -1;
			}

			// array . class (unsized array)
			if(isType with<TypeUnsizedArray>(expectedType) && isType with<TypeArray>(actualType))
			{
				TypeUnsizedArray ref lArray = getType with<TypeUnsizedArray>(expectedType);
				TypeArray ref rArray = getType with<TypeArray>(actualType);

				if(lArray.subType == rArray.subType)
				{
					rating += 2;
					continue;
				}
			}

			// array . auto[]
			if(expectedType == (TypeBase ref)(ctx.typeAutoArray) && (isType with<TypeArray>(actualType) || isType with<TypeUnsizedArray>(actualType)))
			{
				rating += 5;
				continue;
			}

			// array[N] ref . array[] . array[] ref
			if(isType with<TypeRef>(expectedType) && isType with<TypeRef>(actualType))
			{
				TypeRef ref lRef = getType with<TypeRef>(expectedType);
				TypeRef ref rRef = getType with<TypeRef>(actualType);

				if(isType with<TypeUnsizedArray>(lRef.subType) && isType with<TypeArray>(rRef.subType))
				{
					TypeUnsizedArray ref lArray = getType with<TypeUnsizedArray>(lRef.subType);
					TypeArray ref rArray = getType with<TypeArray>(rRef.subType);

					if(lArray.subType == rArray.subType)
					{
						rating += 10;
						continue;
					}
				}
			}

			// derived ref . base ref
			// base ref . derived ref
			if(isType with<TypeRef>(expectedType) && isType with<TypeRef>(actualType))
			{
				TypeRef ref lRef = getType with<TypeRef>(expectedType);
				TypeRef ref rRef = getType with<TypeRef>(actualType);

				if(isType with<TypeClass>(lRef.subType) && isType with<TypeClass>(rRef.subType))
				{
					TypeClass ref lClass = getType with<TypeClass>(lRef.subType);
					TypeClass ref rClass = getType with<TypeClass>(rRef.subType);

					if(IsDerivedFrom(rClass, lClass))
					{
						rating += 5;
						continue;
					}

					if(IsDerivedFrom(lClass, rClass))
					{
						rating += 10;
						continue;
					}
				}
			}

			if(isType with<TypeClass>(expectedType) && isType with<TypeClass>(actualType))
			{
				TypeClass ref lClass = getType with<TypeClass>(expectedType);
				TypeClass ref rClass = getType with<TypeClass>(actualType);

				if(IsDerivedFrom(rClass, lClass))
				{
					rating += 5;
					continue;
				}
			}

			if(isType with<TypeFunction>(expectedType))
			{
				TypeFunction ref lFunction = getType with<TypeFunction>(expectedType);

				if(actualArgument.value && (isType with<TypeFunction>(actualArgument.type) || isType with<TypeFunctionSet>(actualArgument.type)))
				{
					if(FunctionValue functionValue = GetFunctionForType(ctx, actualArgument.value.source, actualArgument.value, lFunction))
						continue;
				}
				
				return -1;
			}

			// type . type ref
			if(isType with<TypeRef>(expectedType))
			{
				TypeRef ref lRef = getType with<TypeRef>(expectedType);

				if(lRef.subType == actualType)
				{
					rating += 5;
					continue;
				}
			}

			// type ref . auto ref
			if(expectedType == (TypeBase ref)(ctx.typeAutoRef) && isType with<TypeRef>(actualType))
			{
				rating += 5;
				continue;
			}

			// type . type ref . auto ref
			if(expectedType == (TypeBase ref)(ctx.typeAutoRef))
			{
				rating += 10;
				continue;
			}

			// numeric . numeric
			if(ctx.IsNumericType(expectedType) && ctx.IsNumericType(actualType))
			{
				rating += 1;
				continue;
			}

			return -1;
		}
	}

	return rating;
}

TypeBase ref MatchGenericType(ExpressionContext ref ctx, SynBase ref source, TypeBase ref matchType, TypeBase ref argType, RefList<MatchData> ref aliases, bool strict)
{
	if(!matchType.isGeneric)
	{
		if(argType.isGeneric)
		{
			RefList<MatchData> subAliases;

			if(TypeBase ref improved = MatchGenericType(ctx, source, argType, matchType, subAliases, true))
				argType = improved;
		}

		if(matchType == argType)
			return argType;

		if(strict)
			return nullptr;

		return matchType;
	}

	// 'generic' match with 'type' results with 'type'
	if(isType with<TypeGeneric>(matchType))
	{
		if(!strict)
		{
			// 'generic' match with 'type[N]' results with 'type[]'
			if(TypeArray ref rhs = getType with<TypeArray>(argType))
				argType = ctx.GetUnsizedArrayType(rhs.subType);
		}

		if(isType with<TypeError>(argType))
			return nullptr;

		return argType;
	}

	if(TypeGenericAlias ref lhs = getType with<TypeGenericAlias>(matchType))
	{
		if(!strict)
		{
			// 'generic' match with 'type[N]' results with 'type[]'
			if(TypeArray ref rhs = getType with<TypeArray>(argType))
				argType = ctx.GetUnsizedArrayType(rhs.subType);
		}

		for(MatchData ref curr = aliases.head; curr; curr = curr.next)
		{
			if(curr.name.name == lhs.baseName.name)
			{
				if(strict)
				{
					if(curr.type != argType)
						return nullptr;
				}

				return curr.type;
			}
		}

		if(isType with<TypeError>(argType))
			return nullptr;

		if(isType with<TypeArgumentSet>(argType) || isType with<TypeMemberSet>(argType) || isType with<TypeFunctionSet>(argType))
			return nullptr;

		aliases.push_back(new MatchData(lhs.baseName, argType));

		return argType;
	}

	if(TypeRef ref lhs = getType with<TypeRef>(matchType))
	{
		// 'generic ref' match with 'type ref' results with 'type ref'
		if(TypeRef ref rhs = getType with<TypeRef>(argType))
		{
			if(TypeBase ref match = MatchGenericType(ctx, source, lhs.subType, rhs.subType, aliases, true))
				return ctx.GetReferenceType(match);

			return nullptr;
		}

		if(strict)
			return nullptr;

		// 'generic ref' match with 'type' results with 'type ref'
		if(TypeBase ref match = MatchGenericType(ctx, source, lhs.subType, argType, aliases, true))
			return ctx.GetReferenceType(match);

		return nullptr;
	}

	if(TypeArray ref lhs = getType with<TypeArray>(matchType))
	{
		// Only match with arrays of the same size
		if(TypeArray ref rhs = getType with<TypeArray>(argType))
		{
			if(lhs.length == rhs.length)
			{
				if(TypeBase ref match = MatchGenericType(ctx, source, lhs.subType, rhs.subType, aliases, true))
					return ctx.GetArrayType(match, lhs.length);

				return nullptr;
			}
		}

		return nullptr;
	}

	if(TypeUnsizedArray ref lhs = getType with<TypeUnsizedArray>(matchType))
	{
		// 'generic[]' match with 'type[]' results with 'type[]'
		if(TypeUnsizedArray ref rhs = getType with<TypeUnsizedArray>(argType))
		{
			if(TypeBase ref match = MatchGenericType(ctx, source, lhs.subType, rhs.subType, aliases, true))
				return ctx.GetUnsizedArrayType(match);

			return nullptr;
		}

		if(strict)
			return nullptr;

		// 'generic[]' match with 'type[N]' results with 'type[]'
		if(TypeArray ref rhs = getType with<TypeArray>(argType))
		{
			if(TypeBase ref match = MatchGenericType(ctx, source, lhs.subType, rhs.subType, aliases, true))
				return ctx.GetUnsizedArrayType(match);
		}

		return nullptr;
	}

	if(TypeFunction ref lhs = getType with<TypeFunction>(matchType))
	{
		// Only match with other function type
		if(TypeFunction ref rhs = getType with<TypeFunction>(argType))
		{
			TypeBase ref returnType = MatchGenericType(ctx, source, lhs.returnType, rhs.returnType, aliases, true);

			if(!returnType)
				return nullptr;

			RefList<TypeHandle> arguments;

			TypeHandle ref lhsArg = lhs.arguments.head;
			TypeHandle ref rhsArg = rhs.arguments.head;

			while(lhsArg && rhsArg)
			{
				TypeBase ref argMatched = MatchGenericType(ctx, source, lhsArg.type, rhsArg.type, aliases, true);

				if(!argMatched)
					return nullptr;

				arguments.push_back(new TypeHandle(argMatched));

				lhsArg = lhsArg.next;
				rhsArg = rhsArg.next;
			}

			// Different number of arguments
			if(lhsArg || rhsArg)
				return nullptr;

			return ctx.GetFunctionType(source, returnType, arguments);
		}

		return nullptr;
	}

	if(TypeGenericClass ref lhs = getType with<TypeGenericClass>(matchType))
	{
		// Match with a generic class instance
		if(TypeClass ref rhs = getType with<TypeClass>(argType))
		{
			if(lhs.proto != rhs.proto)
				return nullptr;

			TypeHandle ref lhsArg = lhs.generics.head;
			MatchData ref rhsArg = rhs.generics.head;

			while(lhsArg && rhsArg)
			{
				TypeBase ref argMatched = MatchGenericType(ctx, source, lhsArg.type, rhsArg.type, aliases, true);

				if(!argMatched)
					return nullptr;

				lhsArg = lhsArg.next;
				rhsArg = rhsArg.next;
			}

			return argType;
		}

		return nullptr;
	}

	if(TypeGenericClassProto ref lhs = getType with<TypeGenericClassProto>(matchType))
	{
		// Match with a generic class instance
		if(TypeClass ref rhs = getType with<TypeClass>(argType))
		{
			if(lhs != rhs.proto)
				return nullptr;

			return argType;
		}

		return nullptr;
	}

	Stop(ctx, source, "ERROR: unknown generic type match");

	return nullptr;
}

TypeBase ref ResolveGenericTypeAliases(ExpressionContext ref ctx, SynBase ref source, TypeBase ref type, RefList<MatchData> aliases)
{
	if(!type.isGeneric || aliases.empty())
		return type;

	// Replace with alias type if there is a match, otherwise leave as generic
	if(isType with<TypeGeneric>(type))
		return type;

	// Replace with alias type if there is a match, otherwise leave as generic
	if(TypeGenericAlias ref lhs = getType with<TypeGenericAlias>(type))
	{
		for(MatchData ref curr = aliases.head; curr; curr = curr.next)
		{
			if(curr.name.name == lhs.baseName.name)
				return curr.type;
		}

		return ctx.typeGeneric;
	}

	if(TypeRef ref lhs = getType with<TypeRef>(type))
		return ctx.GetReferenceType(ResolveGenericTypeAliases(ctx, source, lhs.subType, aliases));

	if(TypeArray ref lhs = getType with<TypeArray>(type))
		return ctx.GetArrayType(ResolveGenericTypeAliases(ctx, source, lhs.subType, aliases), lhs.length);

	if(TypeUnsizedArray ref lhs = getType with<TypeUnsizedArray>(type))
		return ctx.GetUnsizedArrayType(ResolveGenericTypeAliases(ctx, source, lhs.subType, aliases));

	if(TypeFunction ref lhs = getType with<TypeFunction>(type))
	{
		TypeBase ref returnType = ResolveGenericTypeAliases(ctx, source, lhs.returnType, aliases);

		RefList<TypeHandle> arguments;

		for(TypeHandle ref curr = lhs.arguments.head; curr; curr = curr.next)
			arguments.push_back(new TypeHandle(ResolveGenericTypeAliases(ctx, source, curr.type, aliases)));

		return ctx.GetFunctionType(source, returnType, arguments);
	}

	if(TypeGenericClass ref lhs = getType with<TypeGenericClass>(type))
	{
		bool isGeneric = false;
		RefList<TypeHandle> types;

		for(TypeHandle ref curr = lhs.generics.head; curr; curr = curr.next)
		{
			TypeBase ref resolvedType = ResolveGenericTypeAliases(ctx, source, curr.type, aliases);

			if(resolvedType == ctx.typeAuto)
				Stop(ctx, source, "ERROR: 'auto' type cannot be used as template argument");

			isGeneric |= resolvedType.isGeneric;

			types.push_back(new TypeHandle(resolvedType));
		}

		if(isGeneric)
			return ctx.GetGenericClassType(source, lhs.proto, types);

		return CreateGenericTypeInstance(ctx, source, lhs.proto, types);
	}

	Stop(ctx, source, "ERROR: unknown generic type resolve");

	return nullptr;
}

TypeBase ref MatchArgumentType(ExpressionContext ref ctx, SynBase ref source, TypeBase ref expectedType, TypeBase ref actualType, ExprBase ref actualValue, RefList<MatchData> ref aliases)
{
	if(actualType.isGeneric)
	{
		if(TypeFunction ref target = getType with<TypeFunction>(expectedType))
		{
			if(FunctionValue bestOverload = GetFunctionForType(ctx, source, actualValue, target))
				actualType = bestOverload.function.type;
		}

		if(actualType.isGeneric)
			return nullptr;
	}

	return MatchGenericType(ctx, source, expectedType, actualType, aliases, !actualValue);
}

SynFunctionDefinition ref GetGenericFunctionDefinition(ExpressionContext ref ctx, SynBase ref source, FunctionData ref function)
{
	if(!function.definition && function.delayedDefinition)
	{
		ParseContext ref parser = new ParseContext(ctx.optimizationLevel, ArrayView<InplaceStr>());

		parser.currentLexeme = function.delayedDefinition;

		function.definition = ParseFunctionDefinition(*parser);

		if(!function.definition)
			Stop(ctx, source, "ERROR: failed to import generic function '%.*s' body", FMT_ISTR(function.name.name));
	}

	return function.definition;
}

TypeFunction ref GetGenericFunctionInstanceType(ExpressionContext ref ctx, SynBase ref source, TypeBase ref parentType, FunctionData ref function, ArrayView<CallArgumentData> arguments, RefList<MatchData> ref aliases)
{
	assert(function.arguments.size() == arguments.size());

	// Lookup previous match for this function
	RefList<TypeHandle> incomingArguments;
	RefList<MatchData> incomingAliases;

	for(int i = 0; i < arguments.size(); i++)
		incomingArguments.push_back(new TypeHandle(arguments[i].type));

	for(MatchData ref curr = aliases.head; curr; curr = curr.next)
		incomingAliases.push_back(new MatchData(curr.name, curr.type));

	GenericFunctionInstanceTypeRequest request = GenericFunctionInstanceTypeRequest(parentType, function, incomingArguments, incomingAliases);

	if(GenericFunctionInstanceTypeResponse ref response = ctx.genericFunctionInstanceTypeMap.find(request))
	{
		*aliases = response.aliases;

		return response.functionType;
	}

	// Switch to original function scope
	ScopeData ref scope = ctx.scope;

	ctx.SwitchToScopeAtPoint(function.scope, function.source);

	RefList<TypeHandle> types;

	bool prevErrorHandlerNested = ctx.errorHandlerNested;
	ctx.errorHandlerNested = true;

	//int traceDepth = NULLC::TraceGetDepth();

	auto tryResult = try(auto(){
		if(SynFunctionDefinition ref syntax = GetGenericFunctionDefinition(ctx, source, function))
		{
			bool addedParentScope = RestoreParentTypeScope(ctx, source, parentType);

			// Create temporary scope with known arguments for reference in type expression
			ctx.PushTemporaryScope();

			int pos = 0;

			for(SynFunctionArgument ref argument = syntax.arguments.head; argument; { argument = getType with<SynFunctionArgument>(argument.next); pos++; })
			{
				bool failed = false;
				TypeBase ref expectedType = AnalyzeType(ctx, argument.type, true, &failed);

				if(failed)
					break;

				CallArgumentData ref actualArgument = &arguments[pos];

				TypeBase ref type = expectedType == ctx.typeAuto ? actualArgument.type : MatchArgumentType(ctx, argument, expectedType, actualArgument.type, actualArgument.value, aliases);

				if(!type || isType with<TypeError>(type))
					break;

				ctx.AddVariable(new VariableData(argument, ctx.scope, 0, type, argument.name, 0, ctx.uniqueVariableId++), true);

				types.push_back(new TypeHandle(type));
			}

			ctx.PopScope(ScopeType.SCOPE_TEMPORARY);

			if(addedParentScope)
				ctx.PopScope(ScopeType.SCOPE_TYPE);
		}
		else
		{
			if(function.importModule)
				Stop(ctx, source, "ERROR: imported generic function call is not supported");

			for(int i = 0; i < function.arguments.size(); i++)
			{
				ArgumentData ref funtionArgument = &function.arguments[i];

				CallArgumentData ref actualArgument = &arguments[i];

				TypeBase ref type = MatchArgumentType(ctx, funtionArgument.source, funtionArgument.type, actualArgument.type, actualArgument.value, aliases);

				if(!type || isType with<TypeError>(type))
				{
					ctx.genericFunctionInstanceTypeMap.insert(request, GenericFunctionInstanceTypeResponse(nullptr, *aliases));

					// TODO: what about scope restore
					return 1;
				}

				types.push_back(new TypeHandle(type));
			}
		}

		return 0;
	});

	if(tryResult)
	{
		// TODO: what about scope restore
		if(tryResult.value == 1)
			return nullptr;

		// Restore old scope
		ctx.SwitchToScopeAtPoint(scope, nullptr);

		ctx.errorHandlerNested = prevErrorHandlerNested;
	}
	else
	{
		//NULLC::TraceLeaveTo(traceDepth);

		// Restore old scope
		ctx.SwitchToScopeAtPoint(scope, nullptr);

		ctx.errorHandlerNested = prevErrorHandlerNested;

		tryResult.rethrow();
	}

	if(types.size() != arguments.size())
	{
		ctx.genericFunctionInstanceTypeMap.insert(request, GenericFunctionInstanceTypeResponse(nullptr, *aliases));

		return nullptr;
	}

	// Check that all generics have been resolved
	for(MatchData ref curr = function.generics.head; curr; curr = curr.next)
	{
		bool matched = false;

		for(MatchData ref alias = aliases.head; alias; alias = alias.next)
		{
			if(curr.name.name == alias.name.name)
			{
				matched = true;
				break;
			}
		}

		if(!matched)
		{
			ctx.genericFunctionInstanceTypeMap.insert(request, GenericFunctionInstanceTypeResponse(nullptr, *aliases));

			return nullptr;
		}
	}

	TypeFunction ref typeFunction = ctx.GetFunctionType(source, function.type.returnType, types);

	ctx.genericFunctionInstanceTypeMap.insert(request, GenericFunctionInstanceTypeResponse(typeFunction, *aliases));

	return typeFunction;
}

void ReportOnFunctionSelectError(ExpressionContext ref ctx, SynBase ref source, string ref errorBuf, int messageStart, ArrayView<FunctionValue> functions)
{
	RefList<TypeHandle> generics;
	ArrayView<ArgumentData> arguments;
	ArrayView<int> ratings;

	ReportOnFunctionSelectError(ctx, source, errorBuf, messageStart, InplaceStr(), functions, generics, arguments, ratings, 0, false);
}

void ReportOnFunctionSelectError(ExpressionContext ref ctx, SynBase ref source, string ref errorBuf, int messageStart, InplaceStr functionName, ArrayView<FunctionValue> functions, RefList<TypeHandle> generics, ArrayView<ArgumentData> arguments, ArrayView<int> ratings, int bestRating, bool showInstanceInfo)
{
	//assert(errorBuf != nullptr);

	if(!functionName.empty())
	{
		errorBuf += " " + FMT_ISTR(functionName);

		if(!generics.empty())
		{
			errorBuf += "<";

			for(TypeHandle ref el = generics.head; el; el = el.next)
			{
				errorBuf += el != generics.head ? ", " : "";
				errorBuf += FMT_ISTR(el.type.name);
			}

			errorBuf += ">(";
		}
		else
		{
			errorBuf += "(";
		}

		for(int i = 0; i < arguments.size(); i++)
		{
			errorBuf += i != 0 ? ", " : "";
			errorBuf += FMT_ISTR(arguments[i].type.name);
		}

		errorBuf += !functions.empty() ? ")\n" : ")";
	}

	if(!functions.empty())
		errorBuf += bestRating == -1 ? " the only available are:\n" : " candidates are:\n";

	for(int i = 0; i < functions.size(); i++)
	{
		FunctionData ref function = functions[i].function;

		if(!ratings.empty() && ratings[i] != bestRating)
			continue;

		errorBuf += "  " + FMT_ISTR(function.type.returnType.name) + " " + FMT_ISTR(function.name.name);

		if(!function.generics.empty())
		{
			errorBuf += "<";

			for(int k = 0; k < function.generics.size(); k++)
			{
				MatchData ref match = &function.generics[k];

				errorBuf += k != 0 ? ", " : "";
				errorBuf += FMT_ISTR(match.type.name);
			}

			errorBuf += ">";
		}

		errorBuf += "(";

		for(int k = 0; k < function.arguments.size(); k++)
		{
			ArgumentData ref argument = &function.arguments[k];

			errorBuf += k != 0 ? ", " : "";
			errorBuf += argument.isExplicit ? "explicit " : "";
			errorBuf += FMT_ISTR(argument.type.name);
		}

		if(ctx.IsGenericFunction(function) && showInstanceInfo)
		{
			TypeBase ref parentType = nullptr;

			if(functions[i].context.type == (TypeBase ref)(ctx.typeAutoRef))
			{
				assert(function.scope.ownerType != nullptr);
				parentType = function.scope.ownerType;
			}
			else if(function.scope.ownerType)
			{
				parentType = getType with<TypeRef>(functions[i].context.type).subType;
			}

			RefList<MatchData> aliases;
			vector<CallArgumentData> result;

			// Handle named argument order, default argument values and variadic functions
			if(!PrepareArgumentsForFunctionCall(ctx, source, ViewOf(function.arguments), arguments, result, nullptr, false) || (functions[i].context.type == (TypeBase ref)(ctx.typeAutoRef) && !generics.empty()))
			{
				errorBuf += ") (wasn't instanced here)";
			}
			else if(TypeFunction ref instance = GetGenericFunctionInstanceType(ctx, source, parentType, function, ViewOf(result), aliases))
			{
				GetFunctionRating(ctx, function, instance, ViewOf(result));

				errorBuf += ") instanced to\n	" + FMT_ISTR(function.type.returnType.name) + " " + FMT_ISTR(function.name.name) + "(";

				TypeHandle ref curr = instance.arguments.head;

				for(int k = 0; k < function.arguments.size(); k++)
				{
					ArgumentData ref argument = &function.arguments[k];

					errorBuf += k != 0 ? ", " : "";
					errorBuf += argument.isExplicit ? "explicit " : "";
					errorBuf += FMT_ISTR(curr.type.name);

					curr = curr.next;
				}

				if(!aliases.empty())
				{
					errorBuf += ") with [";

					for(MatchData ref curr = aliases.head; curr; curr = curr.next)
					{
						errorBuf += curr != aliases.head ? ", " : "";
						errorBuf += FMT_ISTR(curr.name.name) + " = " + FMT_ISTR(curr.type.name);
					}

					errorBuf += "]";
				}
				else
				{
					errorBuf += ")";
				}
			}
			else
			{
				errorBuf += ") (wasn't instanced here)";
			}
		}
		else
		{
			errorBuf += ")";
		}

		errorBuf += "\n";
	}

	/*ctx.errorBufLocation += strlen(ctx.errorBufLocation);

	StringRef messageEnd = ctx.errorBufLocation;

	ctx.errorInfo.push_back(new ErrorInfo(messageStart, messageEnd, source.begin, source.end, source.begin.pos));

	AddErrorLocationInfo(FindModuleCodeWithSourceLocation(ctx, source.pos.begin), source.pos.begin, ctx.errorBufLocation, ctx.errorBufSize - int(ctx.errorBufLocation - ctx.errorBuf));*/
}

bool IsVirtualFunctionCall(ExpressionContext ref ctx, FunctionData ref function, TypeBase ref type)
{
	assert(function != nullptr);

	if(type == (TypeBase ref)(ctx.typeAutoRef))
		return true;

	if(TypeRef ref refType = getType with<TypeRef>(type))
	{
		if(TypeClass ref classType = getType with<TypeClass>(refType.subType))
		{
			if(classType.isExtendable || classType.baseClass != nullptr)
			{
				int hash = StringHashContinue(classType.nameHash, "::");

				InplaceStr constructor = GetTypeConstructorName(classType);

				hash = StringHashContinue(hash, constructor);

				if(function.nameHash == hash || function.nameHash == StringHashContinue(hash, "$"))
					return false;

				return true;
			}
		}
	}

	return false;
}

FunctionValue SelectBestFunction(ExpressionContext ref ctx, SynBase ref source, ArrayView<FunctionValue> functions, RefList<TypeHandle> generics, ArrayView<ArgumentData> arguments, vector<int> ref ratings)
{
	ratings.resize(functions.count);

	vector<TypeFunction ref> instanceTypes;

	vector<CallArgumentData> result;

	TypeClass ref preferredParent = nullptr;

	for(int i = 0; i < functions.count; i++)
	{
		FunctionValue value = functions.data[i];

		FunctionData ref function = value.function;

		instanceTypes.push_back(function.type);

		if(TypeRef ref typeRef = getType with<TypeRef>(value.context.type))
		{
			if(TypeClass ref typeClass = getType with<TypeClass>(typeRef.subType))
			{
				if(typeClass.isExtendable)
				{
					if(!preferredParent)
						preferredParent = typeClass;
					else
						assert(preferredParent == typeClass);
				}
			}
		}

		if(!generics.empty())
		{
			MatchData ref ca = function.generics.head;
			TypeHandle ref cb = generics.head;

			bool sameGenerics = true;

			for(; ca && cb; { ca = ca.next; cb = cb.next; })
			{
				if(!ca.type.isGeneric && ca.type != cb.type)
				{
					sameGenerics = false;
					break;
				}
			}

			// Fail if provided explicit type list elements can't match
			if(!sameGenerics)
			{
				ratings.data[i] = -1;
				continue;
			}

			// Fail if provided explicit type list is larger than expected explicit type list
			if(cb)
			{
				ratings.data[i] = -1;
				continue;
			}
		}

		int extraRating = 0;

		// Handle named argument order, default argument values and variadic functions
		if(!PrepareArgumentsForFunctionCall(ctx, source, ViewOf(function.arguments), arguments, result, &extraRating, false))
		{
			ratings.data[i] = -1;
			continue;
		}

		ratings.data[i] = GetFunctionRating(ctx, function, function.type, ViewOf(result));

		if(ratings.data[i] == -1)
			continue;

		ratings.data[i] += extraRating;

		if(ctx.IsGenericFunction(function))
		{
			TypeBase ref parentType = nullptr;

			if(value.context.type == (TypeBase ref)(ctx.typeAutoRef))
			{
				assert(function.scope.ownerType != nullptr);
				parentType = function.scope.ownerType;
			}
			else if(function.scope.ownerType)
			{
				parentType = getType with<TypeRef>(value.context.type).subType;
			}

			RefList<MatchData> aliases;

			{
				MatchData ref currMatch = function.generics.head;
				TypeHandle ref currGeneric = generics.head;

				for(; currMatch && currGeneric; { currMatch = currMatch.next; currGeneric = currGeneric.next; })
					aliases.push_back(new MatchData(currMatch.name, currGeneric.type));
			}

			TypeFunction ref instance = GetGenericFunctionInstanceType(ctx, source, parentType, function, ViewOf(result), aliases);

			instanceTypes.back() = instance;

			if(!instance)
			{
				ratings.data[i] = -1;
				continue;
			}
			
			ratings.data[i] = GetFunctionRating(ctx, function, instance, ViewOf(result));

			if(ratings.data[i] == -1)
				continue;
		}
	}

	// For member functions, if there are multiple functions with the same rating and arguments, hide those which parent is derived further from preferred parent
	if(preferredParent)
	{
		for(int i = 0; i < functions.count; i++)
		{
			FunctionData ref a = functions.data[i].function;

			for(int k = 0; k < functions.size(); k++)
			{
				if(i == k)
					continue;

				if(ratings.data[k] == -1)
					continue;

				FunctionData ref b = functions.data[k].function;

				if(ratings.data[i] == ratings.data[k] && instanceTypes[i].arguments.size() == instanceTypes[k].arguments.size())
				{
					bool sameArguments = true;

					for(int arg = 0; arg < instanceTypes[i].arguments.size(); arg++)
					{
						if(instanceTypes[i].arguments[arg].type != instanceTypes[k].arguments[arg].type)
							sameArguments = false;
					}

					if(sameArguments)
					{
						int aDepth = GetDerivedFromDepth(preferredParent, getType with<TypeClass>(a.scope.ownerType));
						int bDepth = GetDerivedFromDepth(preferredParent, getType with<TypeClass>(b.scope.ownerType));

						if (aDepth < bDepth)
							ratings.data[k] = -1;
					}
				}
			}
		}
	}

	// Select best generic and non-generic function
	int bestRating = -1;
	FunctionValue bestFunction;

	int bestGenericRating = -1;
	FunctionValue bestGenericFunction;

	for(int i = 0; i < functions.count; i++)
	{
		FunctionValue value = functions.data[i];

		FunctionData ref function = value.function;

		if(ratings.data[i] == -1)
			continue;

		if(ctx.IsGenericFunction(function))
		{
			if(bestGenericRating == -1 || ratings.data[i] < bestGenericRating)
			{
				bestGenericRating = ratings.data[i];
				bestGenericFunction = value;
			}
		}
		else
		{
			if(bestRating == -1 || ratings.data[i] < bestRating)
			{
				bestRating = ratings.data[i];
				bestFunction = value;
			}
		}
	}

	// Use generic function only if it is better that selected
	if(bestGenericRating != -1 && (bestRating == -1 || bestGenericRating < bestRating))
	{
		bestRating = bestGenericRating;
		bestFunction = bestGenericFunction;
	}
	else
	{
		// Hide all generic functions from selection
		for(int i = 0; i < functions.count; i++)
		{
			FunctionData ref function = functions.data[i].function;

			if(ctx.IsGenericFunction(function))
			{
				if(bestRating != -1 && ratings.data[i] == bestRating && functions.data[i].context.type == (TypeBase ref)(ctx.typeAutoRef) && !function.scope.ownerType.isGeneric)
					CreateGenericFunctionInstance(ctx, source, functions.data[i], generics, arguments, true);

				ratings.data[i] = -1;
			}
		}
	}

	return bestFunction;
}

FunctionValue CreateGenericFunctionInstance(ExpressionContext ref ctx, SynBase ref source, FunctionValue proto, RefList<TypeHandle> generics, ArrayView<ArgumentData> arguments, bool standalone)
{
	FunctionData ref function = proto.function;

	vector<CallArgumentData> result;

	if(!PrepareArgumentsForFunctionCall(ctx, source, ViewOf(function.arguments), arguments, result, nullptr, false))
		assert(false, "unexpected");

	TypeBase ref parentType = nullptr;

	if(proto.context.type == (TypeBase ref)(ctx.typeAutoRef))
	{
		assert(function.scope.ownerType && !function.scope.ownerType.isGeneric);
		parentType = function.scope.ownerType;
	}
	else if(function.scope.ownerType)
	{
		parentType = getType with<TypeRef>(proto.context.type).subType;
	}

	if(parentType && parentType.isGeneric)
		Stop(ctx, source, "ERROR: generic type arguments required for type '%.*s'", FMT_ISTR(parentType.name));

	RefList<MatchData> aliases;

	{
		MatchData ref currMatch = function.generics.head;
		TypeHandle ref currGeneric = generics.head;

		for(; currMatch && currGeneric; { currMatch = currMatch.next; currGeneric = currGeneric.next; })
		{
			if(isType with<TypeError>(currGeneric.type))
				return FunctionValue();

			aliases.push_back(new MatchData(currMatch.name, currGeneric.type));
		}
	}

	TypeFunction ref instance = GetGenericFunctionInstanceType(ctx, source, parentType, function, ViewOf(result), aliases);

	if(!instance)
	{
		Report(ctx, source, "ERROR: failed to instantiate generic function '%.*s'", FMT_ISTR(function.name.name));

		return FunctionValue();
	}

	assert(!instance.isGeneric);

	// Search for an existing function
	for(int i = 0; i < function.instances.size(); i++)
	{
		FunctionData ref data = function.instances[i];

		if(parentType != data.scope.ownerType)
			continue;

		if(!SameGenerics(data.generics, generics))
			continue;

		if(!SameArguments(data.type, instance))
			continue;

		ExprBase ref context = proto.context;

		if(!data.scope.ownerType)
		{
			assert(isType with<ExprNullptrLiteral>(context));

			context = CreateFunctionContextAccess(ctx, ctx.MakeInternal(source), data);
		}

		return FunctionValue(source, function.instances[i], context);
	}

	/*TRACE_SCOPE("analyze", "CreateGenericFunctionInstance");

	if(proto.function.name && proto.function.name.begin)
		TRACE_LABEL2(proto.function.name.begin.pos, proto.function.name.end.pos);*/

	// Switch to original function scope
	ScopeData ref scope = ctx.scope;

	ctx.SwitchToScopeAtPoint(function.scope, function.source);

	ctx.functionInstanceDepth++;

	if(ctx.functionInstanceDepth > NULLC_MAX_GENERIC_INSTANCE_DEPTH)
		Stop(ctx, source, "ERROR: reached maximum generic function instance depth (%d)", NULLC_MAX_GENERIC_INSTANCE_DEPTH);

	bool prevErrorHandlerNested = ctx.errorHandlerNested;
	ctx.errorHandlerNested = true;

	ExprBase ref expr = nullptr;
	
	//int traceDepth = NULLC::TraceGetDepth();

	auto tryResult = try(auto(){
		if(SynFunctionDefinition ref syntax = GetGenericFunctionDefinition(ctx, source, function))
			expr = AnalyzeFunctionDefinition(ctx, syntax, function, instance, parentType, aliases, false, false, false);
		else if(SynShortFunctionDefinition ref node = getType with<SynShortFunctionDefinition>(function.declaration.source))
			expr = AnalyzeShortFunctionDefinition(ctx, node, function, instance);
		else
			Stop(ctx, source, "ERROR: imported generic function call is not supported");
	});

	if(tryResult)
	{
		ctx.functionInstanceDepth--;

		// Restore old scope
		ctx.SwitchToScopeAtPoint(scope, nullptr);

		ctx.errorHandlerNested = prevErrorHandlerNested;
	}
	else
	{
		//NULLC::TraceLeaveTo(traceDepth);

		ctx.functionInstanceDepth--;

		// Restore old scope
		ctx.SwitchToScopeAtPoint(scope, nullptr);

		// Additional error info
		if(ctx.errorBuf)
		{
			/*StringRef errorCurr = ctx.errorBuf + strlen(ctx.errorBuf);

			StringRef messageStart = errorCurr;

			errorCurr += SafeSprintf(errorCurr, ctx.errorBufSize - int(errorCurr - ctx.errorBuf), "while instantiating generic function %.*s(", FMT_ISTR(function.name.name));

			for(TypeHandle ref curr = function.type.arguments.head; curr; curr = curr.next)
				errorCurr += SafeSprintf(errorCurr, ctx.errorBufSize - int(errorCurr - ctx.errorBuf), "%s%.*s", curr != function.type.arguments.head ? ", " : "", FMT_ISTR(curr.type.name));

			errorCurr += SafeSprintf(errorCurr, ctx.errorBufSize - int(errorCurr - ctx.errorBuf), ")");

			if(!arguments.empty())
			{
				errorCurr += SafeSprintf(errorCurr, ctx.errorBufSize - int(errorCurr - ctx.errorBuf), "\n  using argument(s) (");

				for(int i = 0; i < arguments.size(); i++)
					errorCurr += SafeSprintf(errorCurr, ctx.errorBufSize - int(errorCurr - ctx.errorBuf), "%s%.*s", i != 0 ? ", " : "", FMT_ISTR(arguments[i].type.name));

				errorCurr += SafeSprintf(errorCurr, ctx.errorBufSize - int(errorCurr - ctx.errorBuf), ")");
			}

			if(!aliases.empty())
			{
				errorCurr += SafeSprintf(errorCurr, ctx.errorBufSize - int(errorCurr - ctx.errorBuf), "\n  with [");

				for(MatchData ref curr = aliases.head; curr; curr = curr.next)
					errorCurr += SafeSprintf(errorCurr, ctx.errorBufSize - int(errorCurr - ctx.errorBuf), "%s%.*s = %.*s", curr != aliases.head ? ", " : "", FMT_ISTR(curr.name.name), FMT_ISTR(curr.type.name));

				errorCurr += SafeSprintf(errorCurr, ctx.errorBufSize - int(errorCurr - ctx.errorBuf), "]");
			}

			StringRef messageEnd = errorCurr;

			ctx.errorInfo.back().related.push_back(new ErrorInfo(messageStart, messageEnd, source.begin, source.end, source.pos.begin));

			AddErrorLocationInfo(FindModuleCodeWithSourceLocation(ctx, source.pos.begin), source.pos.begin, ctx.errorBuf, ctx.errorBufSize);

			ctx.errorBufLocation += strlen(ctx.errorBufLocation);*/
		}

		ctx.errorHandlerNested = prevErrorHandlerNested;

		tryResult.rethrow();
	}

	ExprFunctionDefinition ref definition = getType with<ExprFunctionDefinition>(expr);

	assert(definition != nullptr);

	if(definition.contextVariableDefinition)
	{
		if(ExprGenericFunctionPrototype ref genericProto = getType with<ExprGenericFunctionPrototype>(function.declaration))
			genericProto.contextVariables.push_back(definition.contextVariableDefinition);
		else
			ctx.setup.push_back(definition.contextVariableDefinition);
	}

	if(standalone)
		return FunctionValue();

	ExprBase ref context = proto.context;

	if(!definition.function.scope.ownerType)
	{
		assert(isType with<ExprNullptrLiteral>(context));

		context = CreateFunctionContextAccess(ctx, ctx.MakeInternal(source), definition.function);
	}

	return FunctionValue(source, definition.function, CreateSequence(ctx, source, definition, context));
}

void GetNodeFunctions(ExpressionContext ref ctx, SynBase ref source, ExprBase ref function, vector<FunctionValue> ref functions)
{
	if(ExprPassthrough ref node = getType with<ExprPassthrough>(function))
		function = node.value;

	if(ExprFunctionAccess ref node = getType with<ExprFunctionAccess>(function))
	{
		functions.push_back(FunctionValue(node.source, node.function, node.context));
	}
	else if(ExprFunctionDefinition ref node = getType with<ExprFunctionDefinition>(function))
	{
		functions.push_back(FunctionValue(node.source, node.function, CreateFunctionContextAccess(ctx, ctx.MakeInternal(source), node.function)));
	}
	else if(ExprGenericFunctionPrototype ref node = getType with<ExprGenericFunctionPrototype>(function))
	{
		functions.push_back(FunctionValue(node.source, node.function, CreateFunctionContextAccess(ctx, ctx.MakeInternal(source), node.function)));
	}
	else if(ExprFunctionOverloadSet ref node = getType with<ExprFunctionOverloadSet>(function))
	{
		for(FunctionHandle ref arg = node.functions.head; arg; arg = arg.next)
		{
			ExprBase ref context = node.context;

			if(!context)
				context = CreateFunctionContextAccess(ctx, ctx.MakeInternal(source), arg.function);

			functions.push_back(FunctionValue(node.source, arg.function, context));
		}
	}
	else if(ExprShortFunctionOverloadSet ref node = getType with<ExprShortFunctionOverloadSet>(function))
	{
		for(ShortFunctionHandle ref option = node.functions.head; option; option = option.next)
			functions.push_back(FunctionValue(node.source, option.function, option.context));
	}
}

ExprBase ref GetFunctionTable(ExpressionContext ref ctx, SynBase ref source, FunctionData ref function)
{
	assert(!isType with<TypeAuto>(function.type.returnType));

	InplaceStr vtableName = GetFunctionTableName(ctx, function);

	if(VariableData ref ref variable = ctx.vtableMap.find(vtableName))
	{
		return new ExprVariableAccess(source, (*variable).type, *variable);
	}
	
	TypeBase ref type = ctx.GetUnsizedArrayType(ctx.typeFunctionID);

	int offset = AllocateGlobalVariable(ctx, source, type.alignment, type.size);
	VariableData ref variable = new VariableData(source, ctx.globalScope, type.alignment, type, new SynIdentifier(vtableName), offset, ctx.uniqueVariableId++);

	ctx.globalScope.variables.push_back(variable);
	ctx.globalScope.allVariables.push_back(variable);

	ctx.variables.push_back(variable);

	ctx.vtables.push_back(variable);
	ctx.vtableMap.insert(vtableName, variable);

	return new ExprVariableAccess(source, variable.type, variable);
}

ExprBase ref CreateFunctionCall0(ExpressionContext ref ctx, SynBase ref source, InplaceStr name, bool allowFailure, bool allowInternal, bool allowFastLookup)
{
	vector<ArgumentData> arguments;

	return CreateFunctionCallByName(ctx, source, name, ArrayView<ArgumentData>(arguments), allowFailure, allowInternal, allowFastLookup);
}

ExprBase ref CreateFunctionCall1(ExpressionContext ref ctx, SynBase ref source, InplaceStr name, ExprBase ref arg0, bool allowFailure, bool allowInternal, bool allowFastLookup)
{
	vector<ArgumentData> arguments;

	arguments.push_back(ArgumentData(arg0.source, false, nullptr, arg0.type, arg0));

	return CreateFunctionCallByName(ctx, source, name, ArrayView<ArgumentData>(arguments), allowFailure, allowInternal, allowFastLookup);
}

ExprBase ref CreateFunctionCall2(ExpressionContext ref ctx, SynBase ref source, InplaceStr name, ExprBase ref arg0, ExprBase ref arg1, bool allowFailure, bool allowInternal, bool allowFastLookup)
{
	vector<ArgumentData> arguments;

	arguments.push_back(ArgumentData(arg0.source, false, nullptr, arg0.type, arg0));
	arguments.push_back(ArgumentData(arg1.source, false, nullptr, arg1.type, arg1));

	return CreateFunctionCallByName(ctx, source, name, ArrayView<ArgumentData>(arguments), allowFailure, allowInternal, allowFastLookup);
}

ExprBase ref CreateFunctionCall3(ExpressionContext ref ctx, SynBase ref source, InplaceStr name, ExprBase ref arg0, ExprBase ref arg1, ExprBase ref arg2, bool allowFailure, bool allowInternal, bool allowFastLookup)
{
	vector<ArgumentData> arguments;

	arguments.push_back(ArgumentData(arg0.source, false, nullptr, arg0.type, arg0));
	arguments.push_back(ArgumentData(arg1.source, false, nullptr, arg1.type, arg1));
	arguments.push_back(ArgumentData(arg2.source, false, nullptr, arg2.type, arg2));

	return CreateFunctionCallByName(ctx, source, name, ArrayView<ArgumentData>(arguments), allowFailure, allowInternal, allowFastLookup);
}

ExprBase ref CreateFunctionCall4(ExpressionContext ref ctx, SynBase ref source, InplaceStr name, ExprBase ref arg0, ExprBase ref arg1, ExprBase ref arg2, ExprBase ref arg3, bool allowFailure, bool allowInternal, bool allowFastLookup)
{
	vector<ArgumentData> arguments;

	arguments.push_back(ArgumentData(arg0.source, false, nullptr, arg0.type, arg0));
	arguments.push_back(ArgumentData(arg1.source, false, nullptr, arg1.type, arg1));
	arguments.push_back(ArgumentData(arg2.source, false, nullptr, arg2.type, arg2));
	arguments.push_back(ArgumentData(arg3.source, false, nullptr, arg3.type, arg3));

	return CreateFunctionCallByName(ctx, source, name, ArrayView<ArgumentData>(arguments), allowFailure, allowInternal, allowFastLookup);
}

ExprBase ref CreateFunctionCallByName(ExpressionContext ref ctx, SynBase ref source, InplaceStr name, ArrayView<ArgumentData> arguments, bool allowFailure, bool allowInternal, bool allowFastLookup)
{
	if(allowFastLookup)
	{
		if(TypeBase ref nextType = FindNextTypeFromScope(ctx.scope))
		{
			int hash = nextType.nameHash;

			hash = StringHashContinue(hash, "::");
			hash = StringHashContinue(hash, name);

			if(ctx.functionMap.first(hash))
			{
				if(ExprBase ref overloads = CreateVariableAccess(ctx, source, RefList<SynIdentifier>(), name, allowInternal))
				{
					if(ExprBase ref result = CreateFunctionCall(ctx, source, overloads, arguments, allowFailure))
						return result;
				}
			}
		}

		if(auto function = ctx.functionMap.first(name.hash()))
		{
			// Collect a set of available functions
			vector<FunctionValue> functions;

			while(function)
			{
				functions.push_back(FunctionValue(source, function.value, CreateFunctionContextAccess(ctx, ctx.MakeInternal(source), function.value)));

				function = ctx.functionMap.next(function);
			}

			if(ExprBase ref result = CreateFunctionCallFinal(ctx, source, nullptr, ViewOf(functions), RefList<TypeHandle>(), arguments, true))
				return result;
		}
	}
	else
	{
		if(ExprBase ref overloads = CreateVariableAccess(ctx, source, RefList<SynIdentifier>(), name, allowInternal))
		{
			if(ExprBase ref result = CreateFunctionCall(ctx, source, overloads, arguments, allowFailure))
				return result;
		}
	}

	if(!allowFailure)
	{
		RefList<TypeHandle> generics;
		ArrayView<FunctionValue> functions;
		ArrayView<int> ratings;

		if(ctx.errorBuf)
		{
			if(ctx.errorCount == 0)
			{
				ctx.errorPos = StringRef(ctx.code, source.pos.begin);
			}

			int messageStart = ctx.errorBuf.length();

			ctx.errorBuf += "ERROR: can't find function '" + FMT_ISTR(name) + "' with following arguments:\n";

			ReportOnFunctionSelectError(ctx, source, ctx.errorBuf, messageStart, name, functions, generics, arguments, ratings, -1, true);
		}

		// Temp:
		io.out << "ERROR: can't find function '" << FMT_ISTR(name) << "' with following arguments:\n" << io.endl;

		assert(ctx.errorHandlerActive);

		throw(new AnalyzerError());
	}

	return nullptr;
}

ExprBase ref CreateFunctionCall(ExpressionContext ref ctx, SynBase ref source, ExprBase ref value, ArrayView<ArgumentData> arguments, bool allowFailure)
{
	// Collect a set of available functions
	vector<FunctionValue> functions;

	GetNodeFunctions(ctx, source, value, functions);

	return CreateFunctionCallFinal(ctx, source, value, ViewOf(functions), RefList<TypeHandle>(), arguments, allowFailure);
}

ExprBase ref CreateFunctionCall(ExpressionContext ref ctx, SynBase ref source, ExprBase ref value, RefList<TypeHandle> generics, SynCallArgument ref argumentHead, bool allowFailure)
{
	// Collect a set of available functions
	vector<FunctionValue> functions;

	GetNodeFunctions(ctx, source, value, functions);

	return CreateFunctionCallOverloaded(ctx, source, value, ViewOf(functions), generics, argumentHead, allowFailure);
}

void AnalyzeFunctionArgumentsEarly(ExpressionContext ref ctx, SynCallArgument ref argumentHead, vector<ArgumentData> ref resultArguments)
{
	for(SynCallArgument ref el = argumentHead; el; el = getType with<SynCallArgument>(el.next))
	{
		if(isType with<SynShortFunctionDefinition>(el.value))
		{
			resultArguments.push_back(ArgumentData());
		}
		else
		{
			ExprBase ref argument = AnalyzeExpression(ctx, el.value);

			resultArguments.push_back(ArgumentData(el, false, el.name, argument.type, argument));
		}
	}
}

void AnalyzeFunctionArgumentsFinal(ExpressionContext ref ctx, SynBase ref source, ExprBase ref value, ArrayView<FunctionValue> functions, SynCallArgument ref argumentHead, vector<ArgumentData> ref resultArguments)
{
	int pos = 0;

	for(SynCallArgument ref el = argumentHead; el; el = getType with<SynCallArgument>(el.next))
	{
		if(functions.empty() && el.name)
			Stop(ctx, source, "ERROR: function argument names are unknown at this point");

		if(SynShortFunctionDefinition ref node = getType with<SynShortFunctionDefinition>(el.value))
		{
			vector<ExprBase ref> options;

			if(functions.empty())
			{
				if(ExprBase ref option = AnalyzeShortFunctionDefinition(ctx, node, value.type, ArrayView<ArgumentData>(resultArguments, pos), RefList<MatchData>()))
					options.push_back(option);
			}
			else
			{
				for(int i = 0; i < functions.size(); i++)
				{
					RefList<MatchData> aliases;

					FunctionData ref function = functions[i].function;

					if(function.scope.ownerType)
					{
						if(TypeRef ref contextType = getType with<TypeRef>(functions[i].context.type))
						{
							if(TypeClass ref classType = getType with<TypeClass>(contextType.subType))
							{
								for(MatchData ref el = classType.generics.head; el; el = el.next)
									aliases.push_back(new MatchData(el.name, el.type));
							}
						}
					}

					if(ExprBase ref option = AnalyzeShortFunctionDefinition(ctx, node, function.type, ArrayView<ArgumentData>(resultArguments, pos), aliases))
					{
						bool found = false;

						for(int k = 0; k < options.size(); k++)
						{
							if(options[k].type == option.type)
								found = true;
						}

						if(!found)
							options.push_back(option);
					}
				}
			}

			ExprBase ref argument = nullptr;

			if(options.empty())
			{
				Report(ctx, source, "ERROR: cannot find function which accepts a function with %d argument(s) as an argument #%d", node.arguments.size(), pos + 1);

				argument = new ExprError(source, ctx.GetErrorType());
			}
			else if(options.size() == 1)
			{
				argument = options[0];
			}
			else
			{
				vector<TypeBase ref> types;

				RefList<ShortFunctionHandle> overloads;

				for(int i = 0; i < options.size(); i++)
				{
					ExprBase ref option = options[i];

					assert(isType with<ExprFunctionDefinition>(option) || isType with<ExprGenericFunctionPrototype>(option));

					types.push_back(option.type);

					if(ExprFunctionDefinition ref node = getType with<ExprFunctionDefinition>(options[i]))
					{
						ExprBase ref context = nullptr;

						if(node.contextVariableDefinition)
						{
							vector<ExprBase ref> expressions;

							expressions.push_back(node);

							if(node.contextVariableDefinition)
								expressions.push_back(node.contextVariableDefinition);

							if(node.contextVariable)
								expressions.push_back(CreateVariableAccess(ctx, source, node.contextVariable, true));

							context = new ExprSequence(source, node.function.contextType, ViewOf(expressions));
						}
						else
						{
							context = new ExprNullptrLiteral(source, node.function.contextType);
						}

						overloads.push_back(new ShortFunctionHandle(node.function, context));
					}
					else if(ExprGenericFunctionPrototype ref node = getType with<ExprGenericFunctionPrototype>(option))
					{
						ExprBase ref context = new ExprNullptrLiteral(source, node.function.contextType);

						overloads.push_back(new ShortFunctionHandle(node.function, context));
					}
				}

				TypeFunctionSet ref type = ctx.GetFunctionSetType(ArrayView<TypeBase ref>(types));

				argument = new ExprShortFunctionOverloadSet(source, type, overloads);
			}

			resultArguments[pos++] = ArgumentData(el, false, el.name, argument.type, argument);
		}
		else
		{
			pos++;
		}
	}
}

ExprBase ref CreateFunctionCallOverloaded(ExpressionContext ref ctx, SynBase ref source, ExprBase ref value, ArrayView<FunctionValue> functions, RefList<TypeHandle> generics, SynCallArgument ref argumentHead, bool allowFailure)
{
	vector<ArgumentData> arguments;
	
	AnalyzeFunctionArgumentsEarly(ctx, argumentHead, arguments);
	AnalyzeFunctionArgumentsFinal(ctx, source, value, functions, argumentHead, arguments);

	return CreateFunctionCallFinal(ctx, source, value, functions, generics, ViewOf(arguments), allowFailure);
}

ExprBase ref CreateFunctionCallFinal(ExpressionContext ref ctx, SynBase ref source, ExprBase ref value, ArrayView<FunctionValue> functions, RefList<TypeHandle> generics, ArrayView<ArgumentData> arguments, bool allowFailure)
{
	bool isErrorCall = false;

	if(value)
	{
		if(isType with<TypeError>(value.type))
			isErrorCall = true;
	}

	for(int i = 0; i < arguments.size(); i++)
	{
		if(isType with<TypeError>(arguments[i].value.type) || !AssertResolvableTypeLiteral(ctx, source, arguments[i].value))
			isErrorCall = true;
	}

	for(TypeHandle ref curr = generics.head; curr; curr = curr.next)
	{
		if(isType with<TypeError>(curr.type) || !AssertResolvableType(ctx, source, curr.type, false))
			isErrorCall = true;
	}

	if(isErrorCall)
	{
		RefList<ExprBase> errorArguments;

		for(int i = 0; i < arguments.size(); i++)
			errorArguments.push_back(new ExprPassthrough(arguments[i].value.source, arguments[i].value.type, arguments[i].value));

		return new ExprFunctionCall(source, ctx.GetErrorType(), value, errorArguments);
	}

	TypeFunction ref type = value ? getType with<TypeFunction>(value.type) : nullptr;

	RefList<ExprBase> actualArguments;

	if(!functions.empty())
	{
		vector<int> ratings;

		FunctionValue bestOverload = SelectBestFunction(ctx, source, functions, generics, arguments, ratings);

		// Didn't find an appropriate function
		if(!bestOverload)
		{
			if(allowFailure)
				return nullptr;

			// auto ref . type cast
			if(isType with<ExprTypeLiteral>(value) && arguments.size() == 1 && arguments[0].type == (TypeBase ref)(ctx.typeAutoRef) && !arguments[0].name)
			{
				ExprBase ref result = CreateCast(ctx, source, arguments[0].value, ((ExprTypeLiteral ref)(value)).value, true);

				// If this was a member function call, store to context
				if(!isType with<ExprNullptrLiteral>(functions[0].context))
					return CreateAssignment(ctx, source, functions[0].context, result);

				return result;
			}

			if(ctx.errorBuf)
			{
				if(ctx.errorCount == 0)
				{
					ctx.errorPos = StringRef(ctx.code, source.pos.begin);
				}

				int messageStart = ctx.errorBuf.length();

				ctx.errorBuf += "ERROR: can't find function '" + FMT_ISTR(functions[0].function.name.name) + "' with following arguments:\n";

				ReportOnFunctionSelectError(ctx, source, ctx.errorBuf, messageStart, functions[0].function.name.name, functions, generics, arguments, ViewOf(ratings), -1, true);
			}

			if(ctx.errorHandlerNested)
			{
				assert(ctx.errorHandlerActive);

				throw(new AnalyzerError());
			}

			ctx.errorCount++;

			RefList<ExprBase> errorArguments;

			for(int i = 0; i < arguments.size(); i++)
				errorArguments.push_back(new ExprPassthrough(arguments[i].value.source, arguments[i].value.type, arguments[i].value));

			return new ExprFunctionCall(source, ctx.GetErrorType(), value, errorArguments);
		}

		int bestRating = -1;

		for(int i = 0; i < functions.size(); i++)
		{
			if(functions[i].function == bestOverload.function)
				bestRating = ratings[i];
		}

		// Check if multiple functions share the same rating
		for(int i = 0; i < functions.size(); i++)
		{
			if(functions[i].function != bestOverload.function && ratings[i] == bestRating)
			{
				// For a function call through 'auto ref' it is ok to have the same function signature in different types
				if(isType with<TypeAutoRef>(bestOverload.context.type) && ctx.IsGenericFunction(functions[i].function) && ctx.IsGenericFunction(bestOverload.function))
				{
					TypeFunction ref instanceA = nullptr;
					RefList<MatchData> aliasesA;
					vector<CallArgumentData> resultA;

					// Handle named argument order, default argument values and variadic functions
					if(PrepareArgumentsForFunctionCall(ctx, source, ViewOf(functions[i].function.arguments), arguments, resultA, nullptr, false))
						instanceA = GetGenericFunctionInstanceType(ctx, source, functions[i].function.scope.ownerType, functions[i].function, ViewOf(resultA), aliasesA);

					TypeFunction ref instanceB = nullptr;
					RefList<MatchData> aliasesB;
					vector<CallArgumentData> resultB;

					if(PrepareArgumentsForFunctionCall(ctx, source, ViewOf(bestOverload.function.arguments), arguments, resultB, nullptr, false))
						instanceB = GetGenericFunctionInstanceType(ctx, source, bestOverload.function.scope.ownerType, bestOverload.function, ViewOf(resultB), aliasesB);

					if(instanceA && instanceB && instanceA == instanceB)
						continue;
				}

				if(ctx.errorBuf)
				{
					if(ctx.errorCount == 0)
					{
						ctx.errorPos = StringRef(ctx.code, source.pos.begin);
					}

					int messageStart = ctx.errorBuf.length();

					ctx.errorBuf += "ERROR: ambiguity, there is more than one overloaded function available for the call:\n";

					ReportOnFunctionSelectError(ctx, source, ctx.errorBuf, messageStart, functions[0].function.name.name, functions, generics, arguments, ViewOf(ratings), bestRating, true);
				}

				if(ctx.errorHandlerNested)
				{
					assert(ctx.errorHandlerActive);

					throw(new AnalyzerError());
				}

				ctx.errorCount++;

				RefList<ExprBase> errorArguments;

				for(int i = 0; i < arguments.size(); i++)
					errorArguments.push_back(new ExprPassthrough(arguments[i].value.source, arguments[i].value.type, arguments[i].value));

				return new ExprFunctionCall(source, ctx.GetErrorType(), value, errorArguments);
			}
		}

		FunctionData ref function = bestOverload.function;

		type = getType with<TypeFunction>(function.type);

		if(isType with<TypeAutoRef>(bestOverload.context.type))
		{
			InplaceStr baseName = bestOverload.function.name.name;

			if(bestOverload.function.scope.ownerType)
			{
				/*if(StringRef pos = strstr(baseName.begin, "::"))
					baseName = InplaceStr(pos + 2);*/
			}

			// For function call through 'auto ref', we have to instantiate all matching member functions of generic types
			for(int i = 0; i < ctx.functions.size(); i++)
			{
				FunctionData ref el = ctx.functions[i];

				if(!el.scope.ownerType)
					continue;

				if(!el.scope.ownerType.isGeneric && !el.type.isGeneric)
					continue;

				int hash = StringHashContinue(el.scope.ownerType.nameHash, "::");

				hash = StringHashContinue(hash, baseName);

				if(el.nameHash != hash)
					continue;

				if(el.generics.size() != generics.size())
					continue;

				if(el.type.arguments.size() != bestOverload.function.type.arguments.size())
					continue;

				if(TypeGenericClassProto ref proto = getType with<TypeGenericClassProto>(el.scope.ownerType))
				{
					for(int k = 0; k < proto.instances.size(); k++)
					{
						ExprClassDefinition ref definition = getType with<ExprClassDefinition>(proto.instances[k]);

						ExprBase ref emptyContext = new ExprNullptrLiteral(source, ctx.GetReferenceType(definition.classType));

						if(bestOverload.function.scope.ownerType.isGeneric)
						{
							FunctionValue instance = CreateGenericFunctionInstance(ctx, source, FunctionValue(source, el, emptyContext), generics, arguments, false);

							bestOverload.function = instance.function;

							function = instance.function;

							type = getType with<TypeFunction>(function.type);
						}
						else
						{
							CreateGenericFunctionInstance(ctx, source, FunctionValue(source, el, emptyContext), generics, arguments, true);
						}
					}
				}
				else
				{
					ExprBase ref emptyContext = new ExprNullptrLiteral(source, ctx.GetReferenceType(el.scope.ownerType));

					CreateGenericFunctionInstance(ctx, source, FunctionValue(source, el, emptyContext), generics, arguments, true);
				}
			}
		}

		if(ctx.IsGenericFunction(function))
		{
			bestOverload = CreateGenericFunctionInstance(ctx, source, bestOverload, generics, arguments, false);

			if(!bestOverload)
				return new ExprFunctionCall(source, ctx.GetErrorType(), value, actualArguments);

			function = bestOverload.function;

			type = getType with<TypeFunction>(function.type);
		}

		if(type.returnType == ctx.typeAuto)
		{
			Report(ctx, source, "ERROR: function type is unresolved at this point");

			return new ExprFunctionCall(source, ctx.GetErrorType(), value, actualArguments);
		}

		if(IsVirtualFunctionCall(ctx, function, bestOverload.context.type))
		{
			ExprBase ref table = GetFunctionTable(ctx, source, bestOverload.function);

			value = CreateFunctionCall2(ctx, source, InplaceStr("__redirect"), bestOverload.context, table, false, true, true);

			value = new ExprTypeCast(source, function.type, value, ExprTypeCastCategory.EXPR_CAST_REINTERPRET);
		}
		else
		{
			value = new ExprFunctionAccess(bestOverload.source, function.type, function, bestOverload.context);
		}

		vector<CallArgumentData> result;

		PrepareArgumentsForFunctionCall(ctx, source, ViewOf(function.arguments), arguments, result, nullptr, true);

		for(int i = 0; i < result.size(); i++)
			actualArguments.push_back(result[i].value);
	}
	else if(type)
	{
		if(type.returnType == ctx.typeAuto)
		{
			Report(ctx, source, "ERROR: function type is unresolved at this point");

			return new ExprFunctionCall(source, ctx.GetErrorType(), value, actualArguments);
		}

		vector<ArgumentData> functionArguments;

		for(TypeHandle ref argType = type.arguments.head; argType; argType = argType.next)
			functionArguments.push_back(ArgumentData(nullptr, false, nullptr, argType.type, nullptr));

		vector<CallArgumentData> result;

		if(!PrepareArgumentsForFunctionCall(ctx, source, ViewOf(functionArguments), arguments, result, nullptr, true))
		{
			if(allowFailure)
				return nullptr;

			if(ctx.errorBuf)
			{
				if(ctx.errorCount == 0)
				{
					ctx.errorPos = StringRef(ctx.code, source.pos.begin);
				}

				/*StringRef messageStart = ctx.errorBufLocation;

				char ref errorBuf = ctx.errorBufLocation;
				int errorBufSize = ctx.errorBufSize - int(ctx.errorBufLocation - ctx.errorBuf);

				char ref errPos = ctx.errorBufLocation;

				if(arguments.size() != functionArguments.size())
					errPos += SafeSprintf(errPos, errorBufSize - int(errPos - errorBuf), "ERROR: function expects %d argument(s), while %d are supplied\n", functionArguments.size(), arguments.size());
				else
					errPos += SafeSprintf(errPos, errorBufSize - int(errPos - errorBuf), "ERROR: there is no conversion from specified arguments and the ones that function accepts\n");

				errPos += SafeSprintf(errPos, errorBufSize - int(errPos - errorBuf), "\tExpected: (");

				for(int i = 0; i < functionArguments.size(); i++)
					errPos += SafeSprintf(errPos, errorBufSize - int(errPos - errorBuf), "%s%.*s", i != 0 ? ", " : "", FMT_ISTR(functionArguments[i].type.name));

				errPos += SafeSprintf(errPos, errorBufSize - int(errPos - errorBuf), ")\n");
			
				errPos += SafeSprintf(errPos, errorBufSize - int(errPos - errorBuf), "\tProvided: (");

				for(int i = 0; i < arguments.size(); i++)
					errPos += SafeSprintf(errPos, errorBufSize - int(errPos - errorBuf), "%s%.*s", i != 0 ? ", " : "", FMT_ISTR(arguments[i].type.name));

				errPos += SafeSprintf(errPos, errorBufSize - int(errPos - errorBuf), ")");

				ctx.errorBufLocation += strlen(ctx.errorBufLocation);

				StringRef messageEnd = ctx.errorBufLocation;

				ctx.errorInfo.push_back(new ErrorInfo(messageStart, messageEnd, source.begin, source.end, source.pos.begin));

				AddErrorLocationInfo(FindModuleCodeWithSourceLocation(ctx, source.pos.begin), source.pos.begin, ctx.errorBufLocation, ctx.errorBufSize - int(ctx.errorBufLocation - ctx.errorBuf));

				ctx.errorBufLocation += strlen(ctx.errorBufLocation);*/
			}

			if(ctx.errorHandlerNested)
			{
				assert(ctx.errorHandlerActive);

				throw(new AnalyzerError());
			}

			ctx.errorCount++;

			RefList<ExprBase> errorArguments;

			for(int i = 0; i < arguments.size(); i++)
				errorArguments.push_back(new ExprPassthrough(arguments[i].value.source, arguments[i].value.type, arguments[i].value));

			return new ExprFunctionCall(source, ctx.GetErrorType(), value, errorArguments);
		}

		for(int i = 0; i < result.size(); i++)
			actualArguments.push_back(result[i].value);
	}
	else if(isType with<ExprTypeLiteral>(value) && arguments.size() == 1 && !arguments[0].name)
	{
		if(ExprTypeLiteral ref typeLiteral = getType with<ExprTypeLiteral>(value))
		{
			if(isType with<TypeGenericClassProto>(typeLiteral.value))
				Stop(ctx, source, "ERROR: generic type arguments in <> are not found after constructor name");
			else if(typeLiteral.value.isGeneric)
				Stop(ctx, source, "ERROR: can't cast to a generic type");
		}

		// Function-style type casts
		return CreateCast(ctx, source, arguments[0].value, ((ExprTypeLiteral ref)(value)).value, true);
	}
	else
	{
		if(ExprTypeLiteral ref typeLiteral = getType with<ExprTypeLiteral>(value))
		{
			if(isType with<TypeGenericClassProto>(typeLiteral.value))
				Stop(ctx, source, "ERROR: generic type arguments in <> are not found after constructor name");
			else if(typeLiteral.value.isGeneric)
				Stop(ctx, source, "ERROR: can't cast to a generic type");
		}

		// Call operator()
		if(ExprBase ref overloads = CreateVariableAccess(ctx, source, RefList<SynIdentifier>(), InplaceStr("()"), false))
		{
			vector<ArgumentData> callArguments;
			callArguments.push_back(ArgumentData(value.source, false, nullptr, value.type, value));

			for(int i = 0; i < arguments.size(); i++)
				callArguments.push_back(arguments[i]);

			if(ExprBase ref result = CreateFunctionCall(ctx, source, overloads, ViewOf(callArguments), false))
				return result;
		}
		else
		{
			Report(ctx, source, "ERROR: operator '()' accepting %d argument(s) is undefined for a class '%.*s'", arguments.size(), FMT_ISTR(value.type.name));

			return new ExprFunctionCall(source, ctx.GetErrorType(), value, actualArguments);
		}
	}

	assert(type != nullptr);

	if(type.isGeneric)
		Stop(ctx, source, "ERROR: generic function call is not supported");

	assert(actualArguments.size() == type.arguments.size());

	{
		ExprBase ref actual = actualArguments.head;
		TypeHandle ref expected = type.arguments.head;

		for(; actual && expected; { actual = actual.next; expected = expected.next; })
			assert(actual.type == expected.type);

		assert(actual == nullptr);
		assert(expected == nullptr);
	}

	return new ExprFunctionCall(source, type.returnType, value, actualArguments);
}

ExprBase ref CreateObjectAllocation(ExpressionContext ref ctx, SynBase ref source, TypeBase ref type)
{
	ExprBase ref size = new ExprIntegerLiteral(source, ctx.typeInt, type.size);
	ExprBase ref typeId = new ExprTypeCast(source, ctx.typeInt, new ExprTypeLiteral(source, ctx.typeTypeID, type), ExprTypeCastCategory.EXPR_CAST_REINTERPRET);

	ExprFunctionCall ref alloc = getType with<ExprFunctionCall>(CreateFunctionCall2(ctx, source, InplaceStr("__newS"), size, typeId, false, true, true));

	if(isType with<TypeError>(type))
		return alloc;

	return new ExprTypeCast(source, ctx.GetReferenceType(type), alloc, ExprTypeCastCategory.EXPR_CAST_REINTERPRET);
}

ExprBase ref CreateArrayAllocation(ExpressionContext ref ctx, SynBase ref source, TypeBase ref type, ExprBase ref count)
{
	ExprBase ref size = new ExprIntegerLiteral(source, ctx.typeInt, type.size);
	ExprBase ref typeId = new ExprTypeCast(source, ctx.typeInt, new ExprTypeLiteral(source, ctx.typeTypeID, type), ExprTypeCastCategory.EXPR_CAST_REINTERPRET);

	count = CreateCast(ctx, source, count, ctx.typeInt, true);

	ExprFunctionCall ref alloc = getType with<ExprFunctionCall>(CreateFunctionCall3(ctx, source, InplaceStr("__newA"), size, count, typeId, false, true, true));

	if(isType with<TypeError>(type))
		return alloc;

	if(type.size >= 64 * 1024)
		Stop(ctx, source, "ERROR: array element size cannot exceed 65535 bytes");

	return new ExprTypeCast(source, ctx.GetUnsizedArrayType(type), alloc, ExprTypeCastCategory.EXPR_CAST_REINTERPRET);
}

ExprBase ref AnalyzeFunctionCall(ExpressionContext ref ctx, SynFunctionCall ref syntax)
{
	ExprBase ref function = AnalyzeExpression(ctx, syntax.value);

	if(isType with<TypeError>(function.type))
	{
		RefList<ExprBase> arguments;

		for(SynCallArgument ref el = syntax.arguments.head; el; el = getType with<SynCallArgument>(el.next))
			arguments.push_back(AnalyzeExpression(ctx, el.value));

		return new ExprFunctionCall(syntax, ctx.GetErrorType(), function, arguments);
	}

	RefList<TypeHandle> generics;

	for(SynBase ref curr = syntax.aliases.head; curr; curr = curr.next)
	{
		TypeBase ref type = AnalyzeType(ctx, curr);

		if(type == ctx.typeAuto)
			Stop(ctx, syntax, "ERROR: explicit generic argument type can't be auto");

		if(type == ctx.typeVoid)
			Stop(ctx, syntax, "ERROR: explicit generic argument cannot be a void type");

		generics.push_back(new TypeHandle(type));
	}

	// Collect a set of available functions
	vector<FunctionValue> functions;
	vector<ArgumentData> arguments;

	if(ExprTypeLiteral ref type = getType with<ExprTypeLiteral>(function))
	{
		if(TypeClass ref typeClass = getType with<TypeClass>(type.value))
		{
			if(!typeClass.completed)
			{
				Report(ctx, syntax, "ERROR: type '%.*s' is not fully defined", FMT_ISTR(typeClass.name));

				RefList<ExprBase> arguments;

				for(SynCallArgument ref el = syntax.arguments.head; el; el = getType with<SynCallArgument>(el.next))
					arguments.push_back(AnalyzeExpression(ctx, el.value));

				return new ExprFunctionCall(syntax, ctx.GetErrorType(), function, arguments);
			}
		}

		if(isType with<TypeError>(type.value))
		{
			RefList<ExprBase> arguments;

			for(SynCallArgument ref el = syntax.arguments.head; el; el = getType with<SynCallArgument>(el.next))
				arguments.push_back(AnalyzeExpression(ctx, el.value));

			return new ExprFunctionCall(syntax, ctx.GetErrorType(), function, arguments);
		}

		// Handle hasMember(x) expresion
		if(TypeMemberSet ref memberSet = getType with<TypeMemberSet>(type.value))
		{
			if(generics.empty() && syntax.arguments.size() == 1 && !syntax.arguments.head.name)
			{
				if(SynTypeSimple ref name = getType with<SynTypeSimple>(syntax.arguments.head.value))
				{
					if(name.path.empty())
					{
						for(MemberHandle ref curr = memberSet.type.members.head; curr; curr = curr.next)
						{
							if(curr.variable.name.name == name.name)
								return new ExprBoolLiteral(syntax, ctx.typeBool, true);
						}

						return new ExprBoolLiteral(syntax, ctx.typeBool, false);
					}
				}
			}
		}

		ExprBase ref regular = nullptr;

		if(SynTypeSimple ref node = getType with<SynTypeSimple>(syntax.value))
		{
			regular = CreateVariableAccess(ctx, syntax.value, node.path, node.name, false);

			if(!regular && node.path.empty())
				regular = CreateVariableAccess(ctx, syntax.value, RefList<SynIdentifier>(), type.value.name, false);
		}
		else
		{
			regular = CreateVariableAccess(ctx, syntax.value, RefList<SynIdentifier>(), type.value.name, false);
		}

		if(regular)
		{
			// Collect a set of available functions
			functions.clear();
			GetNodeFunctions(ctx, syntax, regular, functions);

			// If only constructors are available, do not call as a regular function
			bool hasReturnValue = false;

			for(int i = 0; i < functions.size(); i++)
			{
				if(functions[i].function.type.returnType != ctx.typeVoid)
				{
					hasReturnValue = true;
					break;
				}
			}

			if(hasReturnValue)
			{
				if(arguments.empty())
					AnalyzeFunctionArgumentsEarly(ctx, syntax.arguments.head, arguments);

				AnalyzeFunctionArgumentsFinal(ctx, syntax, function, ViewOf(functions), syntax.arguments.head, arguments);

				ExprBase ref call = CreateFunctionCallFinal(ctx, syntax, function, ViewOf(functions), generics, ViewOf(arguments), true);

				if(call)
					return call;
			}
		}

		if(!type.value.isGeneric)
		{
			VariableData ref variable = AllocateTemporary(ctx, syntax, type.value);

			ExprBase ref pointer = CreateGetAddress(ctx, syntax, CreateVariableAccess(ctx, syntax, variable, false));

			ExprBase ref definition = new ExprVariableDefinition(syntax, ctx.typeVoid, new VariableHandle(nullptr, variable), nullptr);

			ExprBase ref constructor = CreateConstructorAccess(ctx, syntax, type.value, syntax.arguments.empty(), pointer);

			if(!constructor && syntax.arguments.empty())
			{
				vector<ExprBase ref> expressions;

				expressions.push_back(definition);
				expressions.push_back(CreateVariableAccess(ctx, syntax, variable, false));

				return new ExprSequence(syntax, type.value, ViewOf(expressions));
			}

			if(constructor)
			{
				// Collect a set of available functions
				functions.clear();
				GetNodeFunctions(ctx, syntax, constructor, functions);

				if(arguments.empty())
					AnalyzeFunctionArgumentsEarly(ctx, syntax.arguments.head, arguments);

				AnalyzeFunctionArgumentsFinal(ctx, syntax, function, ViewOf(functions), syntax.arguments.head, arguments);

				ExprBase ref call = CreateFunctionCallFinal(ctx, syntax, function, ViewOf(functions), generics, ViewOf(arguments), false);

				vector<ExprBase ref> expressions;

				expressions.push_back(definition);
				expressions.push_back(call);
				expressions.push_back(CreateVariableAccess(ctx, syntax, variable, false));

				return new ExprSequence(syntax, type.value, ViewOf(expressions));
			}
		}
	}

	// Collect a set of available functions
	functions.clear();
	GetNodeFunctions(ctx, syntax, function, functions);

	if(arguments.empty())
		AnalyzeFunctionArgumentsEarly(ctx, syntax.arguments.head, arguments);

	AnalyzeFunctionArgumentsFinal(ctx, syntax, function, ViewOf(functions), syntax.arguments.head, arguments);

	return CreateFunctionCallFinal(ctx, syntax, function, ViewOf(functions), generics, ViewOf(arguments), false);
}

ExprBase ref AnalyzeNew(ExpressionContext ref ctx, SynNew ref syntax)
{
	TypeBase ref type = AnalyzeType(ctx, syntax.type, false);

	// If there is no count and we have an array type that failed, take last extend as the size
	if(!type && !syntax.count && isType with<SynArrayIndex>(syntax.type))
	{
		SynArrayIndex ref arrayIndex = getType with<SynArrayIndex>(syntax.type);

		if(arrayIndex.arguments.size() == 1 && !arrayIndex.arguments.head.name)
		{
			syntax.count = arrayIndex.arguments.head.value;
			syntax.type = arrayIndex.value;

			type = AnalyzeType(ctx, syntax.type, false);
		}
	}

	// If there are no arguments and we have a function type that failed, take the arguments list as constructor arguments
	if(!type && syntax.arguments.empty() && isType with<SynTypeFunction>(syntax.type))
	{
		SynTypeFunction ref functionType = getType with<SynTypeFunction>(syntax.type);

		for(SynBase ref curr = functionType.arguments.head; curr; curr = curr.next)
			syntax.arguments.push_back(new SynCallArgument(curr.begin, curr.end, nullptr, curr));

		syntax.type = new SynTypeReference(functionType.begin, functionType.end, functionType.returnType);

		type = AnalyzeType(ctx, syntax.type, false);
	}

	// Report the original error
	if(!type)
	{
		AnalyzeType(ctx, syntax.type);

		type = ctx.GetErrorType();
	}

	if(type.isGeneric)
		Stop(ctx, syntax.type, "ERROR: generic type is not allowed");

	if(type == ctx.typeVoid || type == ctx.typeAuto)
		Stop(ctx, syntax.type, "ERROR: can't allocate objects of type '%.*s'", FMT_ISTR(type.name));

	if(TypeClass ref typeClass = getType with<TypeClass>(type))
	{
		if(!typeClass.completed)
			Stop(ctx, syntax.type, "ERROR: type '%.*s' is not fully defined", FMT_ISTR(type.name));
	}

	SynBase ref syntaxInternal = ctx.MakeInternal(syntax);

	if(syntax.count)
	{
		if(!syntax.arguments.empty())
			Report(ctx, syntax.arguments.head, "ERROR: can't provide constructor arguments to array allocation");

		if(!syntax.constructor.empty())
			Report(ctx, syntax.constructor.head, "ERROR: can't provide custom construction code for array allocation");

		ExprBase ref count = AnalyzeExpression(ctx, syntax.count);

		ExprBase ref alloc = CreateArrayAllocation(ctx, syntax, type, count);

		if(HasDefaultConstructor(ctx, syntax, type))
		{
			VariableData ref variable = AllocateTemporary(ctx, syntax, alloc.type);

			ExprBase ref initializer = CreateAssignment(ctx, syntax, CreateVariableAccess(ctx, syntaxInternal, variable, false), alloc);
			ExprBase ref definition = new ExprVariableDefinition(syntaxInternal, ctx.typeVoid, new VariableHandle(nullptr, variable), initializer);

			if(ExprBase ref call = CreateDefaultConstructorCall(ctx, syntax, variable.type, CreateVariableAccess(ctx, syntaxInternal, variable, true)))
				return CreateSequence(ctx, syntax, definition, call, CreateVariableAccess(ctx, syntaxInternal, variable, true));
		}

		return alloc;
	}

	ExprBase ref alloc = CreateObjectAllocation(ctx, syntax, type);

	// Call constructor
	TypeRef ref allocType = getType with<TypeRef>(alloc.type);

	TypeBase ref parentType = allocType.subType;

	vector<FunctionData ref> functions;

	if(GetTypeConstructorFunctions(ctx, parentType, syntax.arguments.empty(), functions))
	{
		VariableData ref variable = AllocateTemporary(ctx, syntax, alloc.type);

		ExprBase ref initializer = CreateAssignment(ctx, syntaxInternal, CreateVariableAccess(ctx, syntaxInternal, variable, false), alloc);
		ExprBase ref definition = new ExprVariableDefinition(syntaxInternal, ctx.typeVoid, new VariableHandle(nullptr, variable), initializer);

		ExprBase ref overloads = CreateConstructorAccess(ctx, syntax, ViewOf(functions), CreateVariableAccess(ctx, syntaxInternal, variable, false));

		if(ExprBase ref call = CreateFunctionCall(ctx, syntax, overloads, RefList<TypeHandle>(), syntax.arguments.head, syntax.arguments.empty()))
		{
			vector<ExprBase ref> expressions;

			expressions.push_back(definition);
			expressions.push_back(call);
			expressions.push_back(CreateVariableAccess(ctx, syntaxInternal, variable, false));

			alloc = new ExprSequence(syntax, allocType, ViewOf(expressions));
		}
	}
	else if(syntax.arguments.size() == 1 && !syntax.arguments.head.name)
	{
		VariableData ref variable = AllocateTemporary(ctx, syntax, alloc.type);

		ExprBase ref initializer = CreateAssignment(ctx, syntaxInternal, CreateVariableAccess(ctx, syntaxInternal, variable, false), alloc);
		ExprBase ref definition = new ExprVariableDefinition(syntaxInternal, ctx.typeVoid, new VariableHandle(nullptr, variable), initializer);

		ExprBase ref copy = CreateAssignment(ctx, syntax, new ExprDereference(syntax, parentType, CreateVariableAccess(ctx, syntaxInternal, variable, false)), AnalyzeExpression(ctx, syntax.arguments.head.value));

		vector<ExprBase ref> expressions;

		expressions.push_back(definition);
		expressions.push_back(copy);
		expressions.push_back(CreateVariableAccess(ctx, syntaxInternal, variable, false));

		alloc = new ExprSequence(syntax, allocType, ViewOf(expressions));
	}
	else if(!syntax.arguments.empty())
	{
		Stop(ctx, syntax, "ERROR: function '%.*s::%.*s' that accepts %d arguments is undefined", FMT_ISTR(parentType.name), FMT_ISTR(parentType.name), syntax.arguments.size());
	}

	// Handle custom constructor
	if(!syntax.constructor.empty())
	{
		VariableData ref variable = AllocateTemporary(ctx, syntax, alloc.type);

		ExprBase ref initializer = CreateAssignment(ctx, syntaxInternal, CreateVariableAccess(ctx, syntaxInternal, variable, false), alloc);
		ExprBase ref definition = new ExprVariableDefinition(syntaxInternal, ctx.typeVoid, new VariableHandle(nullptr, variable), initializer);

		TypedFunctionInstanceRequest request = TypedFunctionInstanceRequest(parentType, syntax);

		ExprBase ref function = nullptr;

		if(ExprBase ref ref it = ctx.newConstructorFunctions.find(request))
			function = *it;

		if(!function)
		{
			// Create a member function with the constructor body
			InplaceStr name = GetTemporaryFunctionName(ctx);

			SynIdentifier ref nameIdentifier = new SynIdentifier(name);

			function = CreateFunctionDefinition(ctx, syntax, false, false, parentType, false, ctx.typeVoid, false, nameIdentifier, RefList<SynIdentifier>(), RefList<SynFunctionArgument>(), syntax.constructor, nullptr, nullptr, RefList<MatchData>());

			ctx.newConstructorFunctions.insert(request, function);
		}

		ExprFunctionDefinition ref functionDefinition = getType with<ExprFunctionDefinition>(function);

		// Call this member function
		vector<FunctionValue> functions;
		functions.push_back(FunctionValue(syntax, functionDefinition.function, CreateVariableAccess(ctx, syntaxInternal, variable, false)));

		vector<ArgumentData> arguments;

		ExprBase ref call = CreateFunctionCallFinal(ctx, syntax, function, ViewOf(functions), RefList<TypeHandle>(), ViewOf(arguments), false);

		vector<ExprBase ref> expressions;

		expressions.push_back(definition);
		expressions.push_back(call);
		expressions.push_back(CreateVariableAccess(ctx, syntaxInternal, variable, false));

		alloc = new ExprSequence(syntax, allocType, ViewOf(expressions));
	}

	return alloc;
}

ExprBase ref CreateReturn(ExpressionContext ref ctx, SynBase ref source, ExprBase ref result)
{
	if(isType with<TypeError>(result.type))
	{
		if(FunctionData ref function = ctx.GetCurrentFunction(ctx.scope))
			function.hasExplicitReturn = true;

		return new ExprReturn(source, result.type, result, nullptr, nullptr);
	}

	if(FunctionData ref function = ctx.GetCurrentFunction(ctx.scope))
	{
		TypeBase ref returnType = function.type.returnType;

		// If return type is auto, set it to type that is being returned
		if(returnType == ctx.typeAuto)
		{
			if(!AssertValueExpression(ctx, source, result))
				return new ExprReturn(source, ctx.typeVoid, new ExprError(result.source, ctx.GetErrorType(), result, nullptr), nullptr, nullptr);

			returnType = result.type;

			function.type = ctx.GetFunctionType(source, returnType, function.type.arguments);
		}

		if(returnType == ctx.typeVoid && result.type != ctx.typeVoid)
			Report(ctx, source, "ERROR: 'void' function returning a value");
		if(returnType != ctx.typeVoid && result.type == ctx.typeVoid)
			Report(ctx, source, "ERROR: function must return a value of type '%.*s'", FMT_ISTR(returnType.name));

		result = CreateCast(ctx, source, result, function.type.returnType, false);

		function.hasExplicitReturn = true;

		// TODO: checked return value

		return new ExprReturn(source, ctx.typeVoid, result, CreateFunctionCoroutineStateUpdate(ctx, source, function, 0), CreateFunctionUpvalueClose(ctx, ctx.MakeInternal(source), function, ctx.scope));
	}

	if(isType with<TypeFunction>(result.type))
		result = CreateCast(ctx, source, result, result.type, false);

	if(!AssertValueExpression(ctx, result.source, result))
		return new ExprReturn(source, ctx.typeVoid, new ExprError(result.source, ctx.GetErrorType(), result, nullptr), nullptr, nullptr);

	if(!ctx.IsNumericType(result.type) && !isType with<TypeEnum>(result.type))
		Report(ctx, source, "ERROR: global return cannot accept '%.*s'", FMT_ISTR(result.type.name));

	return new ExprReturn(source, ctx.typeVoid, result, nullptr, nullptr);
}

ExprBase ref AnalyzeReturn(ExpressionContext ref ctx, SynReturn ref syntax)
{
	ExprBase ref result = syntax.value ? AnalyzeExpression(ctx, syntax.value) : new ExprVoid(syntax, ctx.typeVoid);

	return CreateReturn(ctx, syntax, result);
}

ExprBase ref AnalyzeYield(ExpressionContext ref ctx, SynYield ref syntax)
{
	ExprBase ref result = syntax.value ? AnalyzeExpression(ctx, syntax.value) : new ExprVoid(syntax, ctx.typeVoid);

	if(isType with<TypeError>(result.type))
	{
		if(FunctionData ref function = ctx.GetCurrentFunction(ctx.scope))
			function.hasExplicitReturn = true;

		return new ExprYield(syntax, result.type, result, nullptr, nullptr, 0);
	}

	if(FunctionData ref function = ctx.GetCurrentFunction(ctx.scope))
	{
		if(!function.isCoroutine)
			Stop(ctx, syntax, "ERROR: yield can only be used inside a coroutine");

		TypeBase ref returnType = function.type.returnType;

		// If return type is auto, set it to type that is being returned
		if(returnType == ctx.typeAuto)
		{
			if(!AssertValueExpression(ctx, syntax, result))
				return new ExprReturn(syntax, ctx.typeVoid, new ExprError(result.source, ctx.GetErrorType(), result, nullptr), nullptr, nullptr);

			returnType = result.type;

			function.type = ctx.GetFunctionType(syntax, returnType, function.type.arguments);
		}

		if(returnType == ctx.typeVoid && result.type != ctx.typeVoid)
			Report(ctx, syntax, "ERROR: 'void' function returning a value");
		if(returnType != ctx.typeVoid && result.type == ctx.typeVoid)
			Report(ctx, syntax, "ERROR: function must return a value of type '%.*s'", FMT_ISTR(returnType.name));

		result = CreateCast(ctx, syntax, result, function.type.returnType, false);

		function.hasExplicitReturn = true;

		// TODO: checked return value

		int yieldId = ++function.yieldCount;

		return new ExprYield(syntax, ctx.typeVoid, result, CreateFunctionCoroutineStateUpdate(ctx, syntax, function, yieldId), CreateArgumentUpvalueClose(ctx, syntax, function), yieldId);
	}

	Stop(ctx, syntax, "ERROR: global yield is not allowed");

	return nullptr;
}

ExprBase ref ResolveInitializerValue(ExpressionContext ref ctx, SynBase ref source, ExprBase ref initializer)
{
	if(!initializer)
	{
		Report(ctx, source, "ERROR: auto variable must be initialized in place of definition");

		return new ExprError(source, ctx.GetErrorType());
	}

	if(initializer.type == ctx.typeVoid)
	{
		Report(ctx, source, "ERROR: r-value type is 'void'");

		return new ExprError(source, ctx.GetErrorType());
	}

	if(TypeFunction ref target = getType with<TypeFunction>(initializer.type))
	{
		if(FunctionValue bestOverload = GetFunctionForType(ctx, initializer.source, initializer, target))
			initializer = new ExprFunctionAccess(bestOverload.source, bestOverload.function.type, bestOverload.function, bestOverload.context);
	}

	if(ExprFunctionOverloadSet ref node = getType with<ExprFunctionOverloadSet>(initializer))
	{
		if(node.functions.size() == 1)
		{
			FunctionData ref function = node.functions.head.function;

			if(function.type.returnType == ctx.typeAuto)
			{
				Report(ctx, source, "ERROR: function type is unresolved at this point");

				return new ExprError(source, ctx.GetErrorType());
			}

			if(IsVirtualFunctionCall(ctx, function, node.context.type))
			{
				ExprBase ref table = GetFunctionTable(ctx, source, function);

				initializer = CreateFunctionCall2(ctx, source, InplaceStr("__redirect_ptr"), node.context, table, false, true, true);

				if(!isType with<TypeError>(initializer))
					initializer = new ExprTypeCast(source, function.type, initializer, ExprTypeCastCategory.EXPR_CAST_REINTERPRET);
			}
			else
			{
				initializer = new ExprFunctionAccess(initializer.source, function.type, function, node.context);
			}
		}
		else
		{
			vector<FunctionValue> functions;

			GetNodeFunctions(ctx, initializer.source, initializer, functions);

			if(ctx.errorBuf)
			{
				if(ctx.errorCount == 0)
				{
					ctx.errorPos = StringRef(ctx.code, source.pos.begin);
				}

				int messageStart = ctx.errorBuf.length();

				ctx.errorBuf += "ERROR: ambiguity, there is more than one overloaded function available:\n";

				ReportOnFunctionSelectError(ctx, source, ctx.errorBuf, messageStart, ViewOf(functions));
			}

			assert(ctx.errorHandlerActive);

			throw(new AnalyzerError());
		}
	}

	if(isType with<ExprGenericFunctionPrototype>(initializer) || initializer.type.isGeneric)
		Stop(ctx, source, "ERROR: cannot instantiate generic function, because target type is not known");

	return initializer;
}

ExprBase ref AnalyzeVariableDefinition(ExpressionContext ref ctx, SynVariableDefinition ref syntax, int alignment, TypeBase ref type)
{
	if(!syntax.name)
		return new ExprError(syntax, ctx.GetErrorType());

	if(syntax.name.name == InplaceStr("this"))
		Stop(ctx, syntax, "ERROR: 'this' is a reserved keyword");

	if(isType with<TypeError>(type))
	{
		if(syntax.initializer && !ctx.scope.ownerType)
			return new ExprError(syntax, ctx.GetErrorType(), AnalyzeExpression(ctx, syntax.initializer));

		return new ExprError(syntax, ctx.GetErrorType());
	}

	InplaceStr fullName = GetVariableNameInScope(ctx, ctx.scope, syntax.name.name);

	bool conflict = CheckVariableConflict(ctx, syntax, fullName);

	VariableData ref variable = new VariableData(syntax, ctx.scope, 0, type, new SynIdentifier(syntax.name, fullName), 0, ctx.uniqueVariableId++);

	if (IsLookupOnlyVariable(ctx, variable))
		variable.lookupOnly = true;

	if(!conflict)
		ctx.AddVariable(variable, true);

	ExprBase ref initializer = syntax.initializer && !variable.scope.ownerType ? AnalyzeExpression(ctx, syntax.initializer) : nullptr;

	if(type == ctx.typeAuto)
	{
		if(variable.scope.ownerType)
			return ReportExpected(ctx, syntax, ctx.GetErrorType(), "ERROR: member variable type cannot be 'auto'");

		initializer = ResolveInitializerValue(ctx, syntax, initializer);

		if(isType with<TypeError>(initializer.type))
		{
			if(!conflict)
				ctx.variableMap.remove(variable.nameHash, variable);

			return new ExprError(syntax, ctx.GetErrorType(), initializer);
		}

		type = initializer.type;
	}
	else if(type.isGeneric && initializer)
	{
		if(isType with<TypeError>(initializer.type))
		{
			if(!conflict)
				ctx.variableMap.remove(variable.nameHash, variable);

			return new ExprError(syntax, ctx.GetErrorType(), initializer);
		}

		RefList<MatchData> aliases;

		TypeBase ref match = MatchGenericType(ctx, syntax, type, initializer.type, aliases, true);

		if(!match || match.isGeneric)
			Stop(ctx, syntax, "ERROR: can't resolve generic type '%.*s' instance for '%.*s'", FMT_ISTR(initializer.type.name), FMT_ISTR(type.name));

		type = match;
	}
	else if(type.isGeneric)
	{
		if(variable.scope.ownerType)
			return ReportExpected(ctx, syntax, ctx.GetErrorType(), "ERROR: member variable type cannot be '%.*s'", FMT_ISTR(type.name));

		Stop(ctx, syntax, "ERROR: initializer is required to resolve generic type '%.*s'", FMT_ISTR(type.name));
	}

	if(alignment == 0)
	{
		TypeBase ref parentType = ctx.scope.ownerType;

		if(parentType && parentType.alignment != 0 && parentType.alignment < type.alignment)
			alignment = parentType.alignment;
		else
			alignment = type.alignment;
	}

	// Fixup variable data not that the final type is known
	int offset = AllocateVariableInScope(ctx, syntax, alignment, type);
	
	variable.type = type;
	variable.alignment = alignment;
	variable.offset = offset;

	if(TypeClass ref classType = getType with<TypeClass>(variable.type))
	{
		if(classType.hasFinalizer)
			Stop(ctx, syntax, "ERROR: cannot create '%.*s' that implements 'finalize' on stack", FMT_ISTR(classType.name));
	}

	if(variable.scope.ownerType)
		return new ExprVariableDefinition(syntax, ctx.typeVoid, new VariableHandle(syntax.name, variable), nullptr);

	if(initializer)
	{
		if(isType with<TypeError>(initializer.type))
			return new ExprVariableDefinition(syntax, ctx.typeVoid, new VariableHandle(syntax.name, variable), initializer);

		ExprBase ref access = CreateVariableAccess(ctx, syntax.name, variable, true);

		TypeArray ref arrType = getType with<TypeArray>(variable.type);

		// Single-level array might be set with a single element at the point of definition
		if(arrType && !isType with<TypeArray>(initializer.type) && initializer.type != (TypeBase ref)(ctx.typeAutoArray))
		{
			initializer = CreateCast(ctx, syntax.initializer, initializer, arrType.subType, false);

			if(ExprVariableAccess ref node = getType with<ExprVariableAccess>(access))
				access = new ExprGetAddress(access.source, ctx.GetReferenceType(access.type), new VariableHandle(node.source, node.variable));
			else if(ExprDereference ref node = getType with<ExprDereference>(access))
				access = node.value;

			initializer = new ExprArraySetup(syntax.initializer, ctx.typeVoid, access, initializer);
		}
		else
		{
			initializer = CreateAssignment(ctx, syntax.initializer, access, initializer);
		}
	}
	else if(HasDefaultConstructor(ctx, syntax, variable.type))
	{
		ExprBase ref access = CreateVariableAccess(ctx, syntax.name, variable, true);

		if(ExprBase ref call = CreateDefaultConstructorCall(ctx, syntax, variable.type, CreateGetAddress(ctx, syntax, access)))
			initializer = call;
	}

	return new ExprVariableDefinition(syntax, ctx.typeVoid, new VariableHandle(syntax.name, variable), initializer);
}

ExprVariableDefinitions ref AnalyzeVariableDefinitions(ExpressionContext ref ctx, SynVariableDefinitions ref syntax)
{
	int alignment = syntax.alignment ? AnalyzeAlignment(ctx, syntax.alignment) : 0;

	TypeBase ref parentType = ctx.scope.ownerType;

	if(parentType)
	{
		// Introduce 'this' variable into a temporary scope
		ctx.PushTemporaryScope();

		ctx.AddVariable(new VariableData(syntax, ctx.scope, 0, ctx.GetReferenceType(parentType), new SynIdentifier(InplaceStr("this")), 0, ctx.uniqueVariableId++), true);
	}

	TypeBase ref type = AnalyzeType(ctx, syntax.type);

	if(parentType)
		ctx.PopScope(ScopeType.SCOPE_TEMPORARY);

	RefList<ExprBase> definitions;

	for(SynVariableDefinition ref el = syntax.definitions.head; el; el = getType with<SynVariableDefinition>(el.next))
		definitions.push_back(AnalyzeVariableDefinition(ctx, el, alignment, type));

	return new ExprVariableDefinitions(syntax, ctx.typeVoid, type, definitions);
}

TypeBase ref CreateFunctionContextType(ExpressionContext ref ctx, SynBase ref source, InplaceStr functionName)
{
	InplaceStr functionContextName = GetFunctionContextTypeName(ctx, functionName, ctx.functions.size());

	TypeClass ref contextClassType = new TypeClass(SynIdentifier(functionContextName), source, ctx.scope, nullptr, RefList<MatchData>(), false, nullptr);

	contextClassType.isInternal = true;

	ctx.AddType(contextClassType);

	ctx.PushScope(contextClassType);

	contextClassType.typeScope = ctx.scope;

	ctx.PopScope(ScopeType.SCOPE_TYPE);

	contextClassType.completed = true;

	return contextClassType;
}

ExprVariableDefinition ref CreateFunctionContextArgument(ExpressionContext ref ctx, SynBase ref source, FunctionData ref function)
{
	TypeBase ref type = function.contextType;

	assert(!type.isGeneric);

	int offset = AllocateArgumentInScope(ctx, source, 0, type);

	SynIdentifier ref nameIdentifier = new SynIdentifier(InplaceStr(function.scope.ownerType ? "this" : "$context"));

	function.contextArgument = new VariableData(source, ctx.scope, 0, type, nameIdentifier, offset, ctx.uniqueVariableId++);

	ctx.AddVariable(function.contextArgument, true);

	if(function.functionScope.dataSize >= 65536)
		Report(ctx, source, "ERROR: function argument size cannot exceed 65536");

	if(function.type.returnType.size >= 65536)
		Report(ctx, source, "ERROR: function return size cannot exceed 65536");

	return new ExprVariableDefinition(source, ctx.typeVoid, new VariableHandle(nullptr, function.contextArgument), nullptr);
}

VariableData ref CreateFunctionContextVariable(ExpressionContext ref ctx, SynBase ref source, FunctionData ref function, FunctionData ref prototype)
{
	if(function.scope.ownerType)
		return nullptr;

	TypeRef ref refType = getType with<TypeRef>(function.contextType);

	assert(refType != nullptr);

	TypeClass ref classType = getType with<TypeClass>(refType.subType);

	assert(classType != nullptr);

	FinalizeAlignment(classType);

	if(classType.members.empty())
		return nullptr;

	VariableData ref context = nullptr;

	if(prototype)
	{
		ExprFunctionDefinition ref definition = getType with<ExprFunctionDefinition>(prototype.declaration);

		context = definition.contextVariable;

		context.isAlloca = false;
		context.offset = AllocateVariableInScope(ctx, source, refType.alignment, refType);
	}
	else
	{
		// Create a variable holding a reference to a closure
		int offset = AllocateVariableInScope(ctx, source, refType.alignment, refType);

		SynIdentifier ref nameIdentifier = new SynIdentifier(GetFunctionContextVariableName(ctx, function, ctx.GetFunctionIndex(function)));

		context = new VariableData(source, ctx.scope, refType.alignment, refType, nameIdentifier, offset, ctx.uniqueVariableId++);

		ctx.AddVariable(context, true);
	}

	return context;
}

ExprVariableDefinition ref CreateFunctionContextVariableDefinition(ExpressionContext ref ctx, SynBase ref source, FunctionData ref function, FunctionData ref prototype, VariableData ref context)
{
	if(!context)
		return nullptr;

	TypeRef ref refType = getType with<TypeRef>(function.contextType);

	assert(refType != nullptr);

	TypeClass ref classType = getType with<TypeClass>(refType.subType);

	assert(classType != nullptr);

	// Allocate closure
	ExprBase ref alloc = CreateObjectAllocation(ctx, source, classType);

	// Initialize closure
	RefList<ExprBase> expressions;

	expressions.push_back(CreateAssignment(ctx, source, CreateVariableAccess(ctx, source, context, true), alloc));

	for(UpvalueData ref upvalue = function.upvalues.head; upvalue; upvalue = upvalue.next)
	{
		ExprBase ref target = new ExprMemberAccess(source, ctx.GetReferenceType(upvalue.target.type), CreateVariableAccess(ctx, source, context, true), new VariableHandle(nullptr, upvalue.target));

		target = new ExprDereference(source, upvalue.target.type, target);

		// Save variable address to current target value
		ExprBase ref value = CreateVariableAccess(ctx, source, upvalue.variable, false);

		expressions.push_back(CreateAssignment(ctx, source, target, CreateGetAddress(ctx, source, value)));

		// Link to the current head of the upvalue list
		ExprBase ref nextUpvalue = new ExprMemberAccess(source, ctx.GetReferenceType(upvalue.nextUpvalue.type), CreateVariableAccess(ctx, source, context, true), new VariableHandle(nullptr, upvalue.nextUpvalue));

		nextUpvalue = new ExprDereference(source, upvalue.nextUpvalue.type, nextUpvalue);

		expressions.push_back(CreateAssignment(ctx, source, nextUpvalue, GetFunctionUpvalue(ctx, source, upvalue.variable)));

		// Update current head of the upvalue list to our upvalue
		ExprBase ref newHead = new ExprMemberAccess(source, ctx.GetReferenceType(upvalue.target.type), CreateVariableAccess(ctx, source, context, true), new VariableHandle(nullptr, upvalue.target));

		newHead = new ExprTypeCast(source, ctx.GetReferenceType(ctx.typeVoid), newHead, ExprTypeCastCategory.EXPR_CAST_REINTERPRET);

		expressions.push_back(CreateAssignment(ctx, source, GetFunctionUpvalue(ctx, source, upvalue.variable), newHead));
	}

	ExprBase ref initializer = new ExprBlock(source, ctx.typeVoid, expressions, nullptr);

	if(prototype)
	{
		ExprFunctionDefinition ref declaration = getType with<ExprFunctionDefinition>(prototype.declaration);

		assert(declaration != nullptr);

		declaration.contextVariableDefinition.initializer = initializer;
		return nullptr;
	}

	return new ExprVariableDefinition(source, ctx.typeVoid, new VariableHandle(nullptr, context), initializer);
}

bool RestoreParentTypeScope(ExpressionContext ref ctx, SynBase ref source, TypeBase ref parentType)
{
	if(parentType && ctx.scope.ownerType != parentType)
	{
		ctx.PushScope(parentType);

		if(TypeClass ref classType = getType with<TypeClass>(parentType))
		{
			for(MatchData ref el = classType.generics.head; el; el = el.next)
				ctx.AddAlias(new AliasData(source, ctx.scope, el.type, el.name, ctx.uniqueAliasId++));

			for(MatchData ref el = classType.aliases.head; el; el = el.next)
				ctx.AddAlias(new AliasData(source, ctx.scope, el.type, el.name, ctx.uniqueAliasId++));

			for(MemberHandle ref el = classType.members.head; el; el = el.next)
				ctx.AddVariable(el.variable, true);
		}
		else if(TypeGenericClassProto ref genericProto = getType with<TypeGenericClassProto>(parentType))
		{
			SynClassDefinition ref definition = genericProto.definition;

			for(SynIdentifier ref curr = definition.aliases.head; curr; curr = getType with<SynIdentifier>(curr.next))
				ctx.AddAlias(new AliasData(source, ctx.scope, ctx.GetGenericAliasType(curr), curr, ctx.uniqueAliasId++));
		}

		return true;
	}

	return false;
}

void CreateFunctionArgumentVariables(ExpressionContext ref ctx, SynBase ref source, FunctionData ref function, ArrayView<ArgumentData> arguments, RefList<ExprVariableDefinition> ref variables)
{
	for(int i = 0; i < arguments.size(); i++)
	{
		ArgumentData ref argument = &arguments[i];

		assert(!argument.type.isGeneric);

		bool conflict = CheckVariableConflict(ctx, argument.source, argument.name.name);

		int offset = AllocateArgumentInScope(ctx, source, 4, argument.type);
		VariableData ref variable = new VariableData(argument.source, ctx.scope, 0, argument.type, argument.name, offset, ctx.uniqueVariableId++);

		if(TypeClass ref classType = getType with<TypeClass>(variable.type))
		{
			if(classType.hasFinalizer)
				Stop(ctx, argument.source, "ERROR: cannot create '%.*s' that implements 'finalize' on stack", FMT_ISTR(classType.name));
		}

		if(!conflict)
			ctx.AddVariable(variable, true);

		variables.push_back(new ExprVariableDefinition(argument.source, ctx.typeVoid, new VariableHandle(argument.name, variable), nullptr));

		function.argumentVariables.push_back(new VariableHandle(argument.source, variable));
	}
}

ExprBase ref AnalyzeFunctionDefinition(ExpressionContext ref ctx, SynFunctionDefinition ref syntax, FunctionData ref genericProto, TypeFunction ref instance, TypeBase ref instanceParent, RefList<MatchData> matches, bool createAccess, bool hideFunction, bool checkParent)
{
	TypeBase ref parentType = nullptr;

	if(instanceParent)
	{
		parentType = instanceParent;
	}
	else if(syntax.parentType)
	{
		parentType = AnalyzeType(ctx, syntax.parentType);

		if(isType with<TypeError>(parentType))
			parentType = nullptr;

		if(parentType && checkParent)
		{
			if(TypeBase ref currentType = ctx.GetCurrentType(ctx.scope))
			{
				if(parentType == currentType)
					Stop(ctx, syntax.parentType, "ERROR: class name repeated inside the definition of class");

				Stop(ctx, syntax, "ERROR: cannot define class '%.*s' function inside the scope of class '%.*s'", FMT_ISTR(parentType.name), FMT_ISTR(currentType.name));
			}
		}
	}

	if(parentType && (isType with<TypeGeneric>(parentType) || isType with<TypeGenericAlias>(parentType) || isType with<TypeAuto>(parentType) || isType with<TypeVoid>(parentType)))
	{
		if(syntax.isAccessor)
			return ReportExpected(ctx, syntax, ctx.GetErrorType(), "ERROR: cannot add accessor to type '%.*s'", FMT_ISTR(parentType.name));

		return ReportExpected(ctx, syntax, ctx.GetErrorType(), "ERROR: cannot add member function to type '%.*s'", FMT_ISTR(parentType.name));
	}

	TypeBase ref returnType = AnalyzeType(ctx, syntax.returnType);

	if(returnType.isGeneric)
		Stop(ctx, syntax, "ERROR: return type can't be generic");

	if(syntax.isAccessor && !parentType)
		return new ExprError(syntax, ctx.GetErrorType());

	ExprBase ref value = CreateFunctionDefinition(ctx, syntax, syntax.isPrototype, syntax.isCoroutine, parentType, syntax.isAccessor, returnType, syntax.isOperator, syntax.name, syntax.aliases, syntax.arguments, syntax.expressions, genericProto, instance, matches);

	if(ExprFunctionDefinition ref definition = getType with<ExprFunctionDefinition>(value))
	{
		if(definition.function.scope.ownerType)
			return value;

		if(createAccess)
			return CreateFunctionPointer(ctx, syntax, definition, hideFunction);
	}

	return value;
}

void CheckOperatorName(ExpressionContext ref ctx, SynBase ref source, InplaceStr name, ArrayView<ArgumentData> argData)
{
	if(name == InplaceStr("~") || name == InplaceStr("!"))
	{
		if(argData.size() != 1)
			Stop(ctx, source, "ERROR: operator '%.*s' definition must accept exactly one argument", FMT_ISTR(name));
	}
	else if(name == InplaceStr("+") || name == InplaceStr("-"))
	{
		if(argData.size() != 1 && argData.size() != 2)
			Stop(ctx, source, "ERROR: operator '%.*s' definition must accept one or two arguments", FMT_ISTR(name));
	}
	else if(name == InplaceStr("&&") || name == InplaceStr("||"))
	{
		// Two arguments with the second argument being special
		if(argData.size() != 2 || !isType with<TypeFunction>(argData[1].type) || getType with<TypeFunction>(argData[1].type).arguments.size() != 0)
			Stop(ctx, source, "ERROR: operator '%.*s' definition must accept a function returning desired type as the second argument", FMT_ISTR(name));
	}
	else if(name != InplaceStr("()") && name != InplaceStr("[]"))
	{
		if(argData.size() != 2)
			Stop(ctx, source, "ERROR: operator '%.*s' definition must accept exactly two arguments", FMT_ISTR(name));
	}
}

void AnalyzeFunctionDefinitionArguments(ExpressionContext ref ctx, RefList<SynFunctionArgument> arguments, TypeBase ref parentType, TypeFunction ref instance, vector<ArgumentData> ref argData)
{
	TypeHandle ref instanceArg = instance ? instance.arguments.head : nullptr;

	bool hadGenericArgument = parentType ? parentType.isGeneric : false;

	for(SynFunctionArgument ref argument = arguments.head; argument; argument = getType with<SynFunctionArgument>(argument.next))
	{
		ExprBase ref initializer = argument.initializer ? AnalyzeExpression(ctx, argument.initializer) : nullptr;

		TypeBase ref type = nullptr;

		if(instance)
		{
			type = instanceArg.type;

			instanceArg = instanceArg.next;
		}
		else
		{
			// Create temporary scope with known arguments for reference in type expression
			ctx.PushTemporaryScope();

			int pos = 0;

			for(SynFunctionArgument ref prevArg = arguments.head; prevArg && prevArg != argument; prevArg = getType with<SynFunctionArgument>(prevArg.next))
			{
				ArgumentData ref data = &argData[pos++];

				ctx.AddVariable(new VariableData(prevArg, ctx.scope, 0, data.type, data.name, 0, ctx.uniqueVariableId++), true);
			}

			bool failed = false;
			type = AnalyzeType(ctx, argument.type, true, hadGenericArgument ? &failed : nullptr);

			if(type == ctx.typeAuto)
			{
				if(!initializer)
					Stop(ctx, argument.type, "ERROR: function argument cannot be an auto type");

				initializer = ResolveInitializerValue(ctx, argument, initializer);

				type = initializer.type;
			}
			else if(initializer && !isType with<TypeError>(initializer.type))
			{
				// Just a test
				if(!type.isGeneric && !isType with<TypeError>(type))
					CreateCast(ctx, argument.type, initializer, type, true);
			}

			if(type == ctx.typeVoid)
				Stop(ctx, argument.type, "ERROR: function argument cannot be a void type");

			hadGenericArgument |= type.isGeneric;

			// Remove temporary scope
			ctx.PopScope(ScopeType.SCOPE_TEMPORARY);
		}

		argData.push_back(ArgumentData(argument, argument.isExplicit, argument.name, type, initializer));
	}
}

ExprBase ref CreateFunctionDefinition(ExpressionContext ref ctx, SynBase ref source, bool prototype, bool isCoroutine, TypeBase ref parentType, bool accessor, TypeBase ref returnType, bool isOperator, SynIdentifier ref name, RefList<SynIdentifier> aliases, RefList<SynFunctionArgument> arguments, RefList<SynBase> expressions, FunctionData ref genericProto, TypeFunction ref instance, RefList<MatchData> matches)
{
	SynBase ref errorLocation = name.begin ? name : source;

	bool addedParentScope = RestoreParentTypeScope(ctx, source, parentType);

	if(ctx.scope.ownerType && !parentType)
		parentType = ctx.scope.ownerType;

	if(parentType && isCoroutine)
		Stop(ctx, errorLocation, "ERROR: coroutine cannot be a member function");

	RefList<MatchData> generics;

	for(SynIdentifier ref curr = aliases.head; curr; curr = getType with<SynIdentifier>(curr.next))
	{
		if(ctx.typeMap.find(curr.name.hash()))
			Stop(ctx, curr, "ERROR: there is already a type with the same name");

		for(SynIdentifier ref prev = aliases.head; prev && prev != curr; prev = getType with<SynIdentifier>(prev.next))
		{
			if(prev.name == curr.name)
				Stop(ctx, curr, "ERROR: there is already an alias with the same name");
		}

		TypeBase ref target = nullptr;

		for(MatchData ref match = matches.head; match; match = match.next)
		{
			if(curr.name == match.name.name)
			{
				target = match.type;
				break;
			}
		}

		if(!target)
			target = ctx.GetGenericAliasType(curr);

		generics.push_back(new MatchData(curr, target));
	}

	vector<ArgumentData> argData;

	AnalyzeFunctionDefinitionArguments(ctx, arguments, parentType, instance, argData);

	// Check required operator properties
	if(isOperator)
		CheckOperatorName(ctx, source, name.name, ViewOf(argData));

	if(accessor && name.name == parentType.name)
		Stop(ctx, errorLocation, "ERROR: name '%.*s' is already taken for a type", FMT_ISTR(name.name));

	InplaceStr functionName = GetFunctionName(ctx, ctx.scope, parentType, name.name, isOperator, accessor);

	/*TRACE_SCOPE("analyze", "CreateFunctionDefinition");
	TRACE_LABEL2(functionName.begin, functionName.end);*/

	TypeBase ref contextRefType = nullptr;

	if(parentType)
		contextRefType = ctx.GetReferenceType(parentType);
	else if(!isCoroutine && (ctx.scope == ctx.globalScope || ctx.scope.ownerNamespace))
		contextRefType = ctx.GetReferenceType(ctx.typeVoid);
	else
		contextRefType = ctx.GetReferenceType(CreateFunctionContextType(ctx, source, functionName));

	if(isType with<TypeError>(returnType))
	{
		if(addedParentScope)
			ctx.PopScope(ScopeType.SCOPE_TYPE);

		return new ExprError(source, ctx.GetErrorType());
	}

	for(int i = 0; i < argData.size(); i++)
	{
		if(isType with<TypeError>(argData[i].type))
		{
			if(addedParentScope)
				ctx.PopScope(ScopeType.SCOPE_TYPE);

			return new ExprError(source, ctx.GetErrorType());
		}
	}

	TypeFunction ref functionType = ctx.GetFunctionType(source, returnType, ViewOf(argData));

	if(instance)
		assert(functionType == instance);

	if(VariableData ref ref variable = ctx.variableMap.find(functionName.hash()))
	{
		if((*variable).scope == ctx.scope)
			Stop(ctx, errorLocation, "ERROR: name '%.*s' is already taken for a variable in current scope", FMT_ISTR(name.name));
	}

	if(TypeClass ref classType = getType with<TypeClass>(parentType))
	{
		if(name.name == InplaceStr("finalize"))
			classType.hasFinalizer = true;
	}

	SynIdentifier ref nameIdentifier = new SynIdentifier(name, functionName);

	FunctionData ref function = new FunctionData(source, ctx.scope, isCoroutine, accessor, isOperator, functionType, contextRefType, nameIdentifier, generics, ctx.uniqueFunctionId++);

	function.aliases = matches;

	// Fill in argument data
	for(int i = 0; i < argData.size(); i++)
		function.arguments.push_back(argData[i]);

	FunctionData ref implementedPrototype = nullptr;

	// If the type is known, implement the prototype immediately
	if(functionType.returnType != ctx.typeAuto)
	{
		if(FunctionData ref functionPrototype = ImplementPrototype(ctx, function))
		{
			if(prototype)
				Stop(ctx, errorLocation, "ERROR: function is already defined");

			if(isCoroutine && !functionPrototype.isCoroutine)
				Stop(ctx, errorLocation, "ERROR: function prototype was not a coroutine");

			if(!isCoroutine && functionPrototype.isCoroutine)
				Stop(ctx, errorLocation, "ERROR: function prototype was a coroutine");

			function.contextType = functionPrototype.contextType;

			implementedPrototype = functionPrototype;
		}
	}

	CheckFunctionConflict(ctx, source, function.name.name);

	ctx.AddFunction(function);

	if(ctx.IsGenericFunction(function))
	{
		assert(!instance);

		if(prototype)
			Stop(ctx, errorLocation, "ERROR: generic function cannot be forward-declared");

		if(addedParentScope)
			ctx.PopScope(ScopeType.SCOPE_TYPE);

		assert(isType with<SynFunctionDefinition>(source));

		function.definition = getType with<SynFunctionDefinition>(source);
		function.declaration = new ExprGenericFunctionPrototype(source, function.type, function);

		return function.declaration;
	}

	// Operator overloads can't be called recursively and become available at the end of the definition
	if (isOperator)
	{
		ctx.functionMap.remove(function.nameHash, function);

		ctx.noAssignmentOperatorForTypePair.clear();
	}

	if(genericProto)
	{
		function.proto = genericProto;

		genericProto.instances.push_back(function);
	}

	ctx.PushScope(function);

	function.functionScope = ctx.scope;

	for(MatchData ref curr = function.aliases.head; curr; curr = curr.next)
		ctx.AddAlias(new AliasData(source, ctx.scope, curr.type, curr.name, ctx.uniqueAliasId++));

	RefList<ExprVariableDefinition> variables;

	CreateFunctionArgumentVariables(ctx, source, function, ViewOf(argData), variables);

	ExprVariableDefinition ref contextArgumentDefinition = CreateFunctionContextArgument(ctx, ctx.MakeInternal(source), function);

	function.argumentsSize = function.functionScope.dataSize;

	ExprBase ref coroutineStateRead = nullptr;

	RefList<ExprBase> code;

	if(prototype)
	{
		if(function.type.returnType == ctx.typeAuto)
			Stop(ctx, errorLocation, "ERROR: function prototype with unresolved return type");

		function.isPrototype = true;
	}
	else
	{
		if(function.isCoroutine)
		{
			int offset = AllocateVariableInScope(ctx, source, ctx.typeInt.alignment, ctx.typeInt);

			SynIdentifier ref nameIdentifier = new SynIdentifier(InplaceStr("$jmpOffset"));

			function.coroutineJumpOffset = new VariableData(source, ctx.scope, 0, ctx.typeInt, nameIdentifier, offset, ctx.uniqueVariableId++);

			if (IsLookupOnlyVariable(ctx, function.coroutineJumpOffset))
				function.coroutineJumpOffset.lookupOnly = true;

			ctx.AddVariable(function.coroutineJumpOffset, false);

			AddFunctionCoroutineVariable(ctx, source, function, function.coroutineJumpOffset);

			coroutineStateRead = CreateVariableAccess(ctx, source, function.coroutineJumpOffset, true);
		}

		// If this is a custom default constructor, add a prolog
		if(TypeClass ref classType = getType with<TypeClass>(function.scope.ownerType))
		{
			if(GetTypeConstructorName(classType) == name.name)
				CreateDefaultConstructorCode(ctx, source, classType, code);
		}

		for(SynBase ref expression = expressions.head; expression; expression = expression.next)
			code.push_back(AnalyzeStatement(ctx, expression));

		// If the function type is still auto it means that it hasn't returned anything
		if(function.type.returnType == ctx.typeAuto)
			function.type = ctx.GetFunctionType(source, ctx.typeVoid, function.type.arguments);

		if(function.type.returnType != ctx.typeVoid && !function.hasExplicitReturn)
			Report(ctx, errorLocation, "ERROR: function must return a value of type '%.*s'", FMT_ISTR(returnType.name));

		// User might have not returned from all control paths, for a void function we will generate a return
		if(function.type.returnType == ctx.typeVoid)
		{
			SynBase ref location = ctx.MakeInternal(source);

			code.push_back(new ExprReturn(location, ctx.typeVoid, new ExprVoid(location, ctx.typeVoid), CreateFunctionCoroutineStateUpdate(ctx, location, function, 0), CreateFunctionUpvalueClose(ctx, ctx.MakeInternal(location), function, ctx.scope)));
		}
	}

	ClosePendingUpvalues(ctx, function);

	ctx.PopScope(ScopeType.SCOPE_FUNCTION);

	if(addedParentScope)
		ctx.PopScope(ScopeType.SCOPE_TYPE);

	if(parentType)
	{
		InplaceStr parentName = parentType.name;

		if(TypeClass ref classType = getType with<TypeClass>(parentType))
		{
			if(classType.proto)
				parentName = classType.proto.name;
		}

		if(name.name == parentName && function.type.returnType != ctx.typeVoid)
			Stop(ctx, errorLocation, "ERROR: type constructor return type must be 'void'");
	}

	VariableData ref contextVariable = nullptr;
	ExprVariableDefinition ref contextVariableDefinition = nullptr;

	if(parentType)
	{
		contextVariableDefinition = nullptr;
	}
	else if(!isCoroutine && (ctx.scope == ctx.globalScope || ctx.scope.ownerNamespace))
	{
		contextVariableDefinition = nullptr;
	}
	else if(prototype)
	{
		TypeRef ref refType = getType with<TypeRef>(function.contextType);

		assert(refType != nullptr);

		SynIdentifier ref nameIdentifier = new SynIdentifier(GetFunctionContextVariableName(ctx, function, ctx.GetFunctionIndex(function)));

		contextVariable = new VariableData(source, ctx.scope, refType.alignment, refType, nameIdentifier, 0, ctx.uniqueVariableId++);

		contextVariable.isAlloca = true;
		contextVariable.offset = -1;

		//function.contextVariable = context;

		ctx.AddVariable(contextVariable, true);

		contextVariableDefinition = new ExprVariableDefinition(source, ctx.typeVoid, new VariableHandle(nullptr, contextVariable), nullptr);
	}
	else
	{
		contextVariable = CreateFunctionContextVariable(ctx, source, function, implementedPrototype);
		contextVariableDefinition = CreateFunctionContextVariableDefinition(ctx, source, function, implementedPrototype, contextVariable);
	}

	// If the type was deduced, implement prototype now that it's known
	if(ImplementPrototype(ctx, function))
	{
		TypeRef ref refType = getType with<TypeRef>(function.contextType);

		assert(refType != nullptr);

		if(refType.subType != ctx.typeVoid)
		{
			TypeClass ref classType = getType with<TypeClass>(refType.subType);

			assert(classType != nullptr);

			if(!classType.members.empty())
				Report(ctx, errorLocation, "ERROR: function '%.*s' is being defined with the same set of arguments", FMT_ISTR(function.name.name));
		}
	}

	// Time to make operator overload visible
	if(isOperator)
	{
		ctx.functionMap.insert(function.nameHash, function);

		ctx.noAssignmentOperatorForTypePair.clear();
	}

	FunctionData ref conflict = CheckUniqueness(ctx, function);

	if(conflict)
		Report(ctx, errorLocation, "ERROR: function '%.*s' is being defined with the same set of arguments", FMT_ISTR(function.name.name));

	function.declaration = new ExprFunctionDefinition(source, function.type, function, contextArgumentDefinition, variables, coroutineStateRead, code, contextVariableDefinition, contextVariable);

	ctx.definitions.push_back(function.declaration);

	return function.declaration;
}

void DeduceShortFunctionReturnValue(ExpressionContext ref ctx, SynBase ref source, FunctionData ref function, RefList<ExprBase> ref expressions)
{
	if(function.hasExplicitReturn)
		return;

	TypeBase ref expected = function.type.returnType;

	if(expected == ctx.typeVoid)
		return;

	TypeBase ref actual = expressions.tail ? expressions.tail.type : ctx.typeVoid;

	if(actual == ctx.typeVoid)
		return;

	if(isType with<TypeError>(actual))
		return;

	ExprBase ref result = expressions.tail;

	// If return type is auto, set it to type that is being returned
	if(expected == ctx.typeAuto)
	{
		if(result && !AssertValueExpression(ctx, result.source, result))
			return;

		function.type = ctx.GetFunctionType(source, actual, function.type.arguments);
	}
	else
	{
		result = CreateCast(ctx, source, expressions.tail, expected, false);
	}

	result = new ExprReturn(source, ctx.typeVoid, result, CreateFunctionCoroutineStateUpdate(ctx, source, function, 0), CreateFunctionUpvalueClose(ctx, ctx.MakeInternal(source), function, ctx.scope));

	if(expressions.head == expressions.tail)
	{
		expressions.head = expressions.tail = result;
	}
	else
	{
		ExprBase ref curr = expressions.head;

		while(curr)
		{
			if(curr.next == expressions.tail)
				curr.next = result;

			curr = curr.next;
		}
	}

	function.hasExplicitReturn = true;
}

ExprBase ref AnalyzeShortFunctionDefinition(ExpressionContext ref ctx, SynShortFunctionDefinition ref syntax, FunctionData ref genericProto, TypeFunction ref argumentType)
{
	if(syntax.arguments.size() != argumentType.arguments.size())
		return nullptr;

	TypeBase ref returnType = argumentType.returnType;

	if(returnType.isGeneric)
		returnType = ctx.typeAuto;

	RefList<MatchData> argCasts;
	vector<ArgumentData> argData;

	TypeHandle ref expected = argumentType.arguments.head;

	for(SynShortFunctionArgument ref param = syntax.arguments.head; param; param = getType with<SynShortFunctionArgument>(param.next))
	{
		TypeBase ref type = nullptr;

		if(param.type)
			type = AnalyzeType(ctx, param.type);

		if(type)
		{
			if(type == ctx.typeAuto)
				Stop(ctx, syntax, "ERROR: function argument cannot be an auto type");

			if(type == ctx.typeVoid)
				Stop(ctx, syntax, "ERROR: function argument cannot be a void type");

			if(isType with<TypeError>(type))
				return nullptr;

			char[] name = new char[param.name.name.length() + 2];

			/*sprintf(name, "%.*s$", FMT_ISTR(param.name.name));*/

			if(expected.type.isGeneric)
			{
				RefList<MatchData> aliases;

				if(TypeBase ref match = MatchGenericType(ctx, syntax, expected.type, type, aliases, false))
					argData.push_back(ArgumentData(param, false, new SynIdentifier(param.name, InplaceStr(name)), match, nullptr));
				else
					return nullptr;
			}
			else
			{
				argData.push_back(ArgumentData(param, false, new SynIdentifier(param.name, InplaceStr(name)), expected.type, nullptr));
			}

			argCasts.push_back(new MatchData(param.name, type));
		}
		else
		{
			argData.push_back(ArgumentData(param, false, param.name, expected.type, nullptr));
		}

		expected = expected.next;
	}

	TypeFunction ref instanceType = ctx.GetFunctionType(syntax, returnType, ViewOf(argData));

	ExprBase ref definition = AnalyzeShortFunctionDefinition(ctx, syntax, genericProto, instanceType, argCasts, ViewOf(argData));

	if(ExprFunctionDefinition ref node = getType with<ExprFunctionDefinition>(definition))
	{
		node.contextVariable = CreateFunctionContextVariable(ctx, syntax, node.function, nullptr);
		node.contextVariableDefinition = CreateFunctionContextVariableDefinition(ctx, syntax, node.function, nullptr, node.contextVariable);
	}

	return definition;
}

ExprBase ref AnalyzeShortFunctionDefinition(ExpressionContext ref ctx, SynShortFunctionDefinition ref syntax, FunctionData ref genericProto, TypeFunction ref instanceType, RefList<MatchData> argCasts, ArrayView<ArgumentData> argData)
{
	InplaceStr functionName = GetFunctionName(ctx, ctx.scope, nullptr, InplaceStr(), false, false);

	TypeBase ref contextClassType = CreateFunctionContextType(ctx, syntax, functionName);

	SynIdentifier ref functionNameIdentifier = new SynIdentifier(functionName);

	FunctionData ref function = new FunctionData(syntax, ctx.scope, false, false, false, instanceType, ctx.GetReferenceType(contextClassType), functionNameIdentifier, RefList<MatchData>(), ctx.uniqueFunctionId++);

	// Fill in argument data
	for(int i = 0; i < argData.size(); i++)
		function.arguments.push_back(argData[i]);

	CheckFunctionConflict(ctx, syntax, function.name.name);

	ctx.AddFunction(function);

	if(ctx.IsGenericFunction(function))
	{
		function.declaration = new ExprGenericFunctionPrototype(syntax, function.type, function);

		function.contextType = ctx.GetReferenceType(ctx.typeVoid);

		return function.declaration;
	}

	if(genericProto)
	{
		function.proto = genericProto;

		genericProto.instances.push_back(function);
	}

	ctx.PushScope(function);

	function.functionScope = ctx.scope;

	RefList<ExprVariableDefinition> arguments;

	CreateFunctionArgumentVariables(ctx, syntax, function, argData, arguments);

	ExprVariableDefinition ref contextArgumentDefinition = CreateFunctionContextArgument(ctx, ctx.MakeInternal(syntax), function);

	function.argumentsSize = function.functionScope.dataSize;

	RefList<ExprBase> expressions;

	// Create casts of arguments with a wrong type
	for(MatchData ref el = argCasts.head; el; el = el.next)
	{
		bool conflict = CheckVariableConflict(ctx, syntax, el.name.name);

		if(isType with<TypeError>(el.type))
			continue;

		int offset = AllocateVariableInScope(ctx, syntax, el.type.alignment, el.type);
		VariableData ref variable = new VariableData(syntax, ctx.scope, el.type.alignment, el.type, el.name, offset, ctx.uniqueVariableId++);

		if (IsLookupOnlyVariable(ctx, variable))
			variable.lookupOnly = true;

		if(!conflict)
			ctx.AddVariable(variable, true);

		char[] name = new char[el.name.name.length() + 2];

		/*sprintf(name, "%.*s$", FMT_ISTR(el.name.name));*/

		ExprBase ref access = CreateVariableAccess(ctx, syntax, RefList<SynIdentifier>(), InplaceStr(name), false);

		if(ctx.GetReferenceType(el.type) == access.type)
			access = new ExprDereference(syntax, el.type, access);
		else
			access = CreateCast(ctx, syntax, access, el.type, true);

		expressions.push_back(new ExprVariableDefinition(syntax, ctx.typeVoid, new VariableHandle(nullptr, variable), CreateAssignment(ctx, syntax, CreateVariableAccess(ctx, syntax, variable, false), access)));
	}

	for(SynBase ref expression = syntax.expressions.head; expression; expression = expression.next)
		expressions.push_back(AnalyzeStatement(ctx, expression));

	DeduceShortFunctionReturnValue(ctx, syntax, function, expressions);

	// If the function type is still auto it means that it hasn't returned anything
	if(function.type.returnType == ctx.typeAuto)
		function.type = ctx.GetFunctionType(syntax, ctx.typeVoid, function.type.arguments);

	if(function.type.returnType != ctx.typeVoid && !function.hasExplicitReturn)
		Report(ctx, syntax, "ERROR: function must return a value of type '%.*s'", FMT_ISTR(function.type.returnType.name));

	// User might have not returned from all control paths, for a void function we will generate a return
	if(function.type.returnType == ctx.typeVoid)
	{
		SynBase ref location = ctx.MakeInternal(syntax);

		expressions.push_back(new ExprReturn(location, ctx.typeVoid, new ExprVoid(location, ctx.typeVoid), CreateFunctionCoroutineStateUpdate(ctx, location, function, 0), CreateFunctionUpvalueClose(ctx, ctx.MakeInternal(location), function, ctx.scope)));
	}

	ClosePendingUpvalues(ctx, function);

	ctx.PopScope(ScopeType.SCOPE_FUNCTION);

	function.declaration = new ExprFunctionDefinition(syntax, function.type, function, contextArgumentDefinition, arguments, nullptr, expressions, nullptr, nullptr);

	ctx.definitions.push_back(function.declaration);

	return function.declaration;
}

ExprBase ref AnalyzeGenerator(ExpressionContext ref ctx, SynGenerator ref syntax)
{
	InplaceStr functionName = GetTemporaryFunctionName(ctx);

	vector<ArgumentData> arguments;

	TypeBase ref contextClassType = CreateFunctionContextType(ctx, syntax, functionName);

	SynIdentifier ref functionNameIdentifier = new SynIdentifier(functionName);

	FunctionData ref function = new FunctionData(syntax, ctx.scope, true, false, false, ctx.GetFunctionType(syntax, ctx.typeAuto, ViewOf(arguments)), ctx.GetReferenceType(contextClassType), functionNameIdentifier, RefList<MatchData>(), ctx.uniqueFunctionId++);

	CheckFunctionConflict(ctx, syntax, function.name.name);

	ctx.AddFunction(function);

	ctx.PushScope(function);

	function.functionScope = ctx.scope;

	ExprVariableDefinition ref contextArgumentDefinition = CreateFunctionContextArgument(ctx, ctx.MakeInternal(syntax), function);

	function.argumentsSize = function.functionScope.dataSize;

	ExprBase ref coroutineStateRead = nullptr;

	RefList<ExprBase> expressions;

	if(function.isCoroutine)
	{
		int offset = AllocateVariableInScope(ctx, syntax, ctx.typeInt.alignment, ctx.typeInt);

		SynIdentifier ref nameIdentifier = new SynIdentifier(InplaceStr("$jmpOffset"));

		function.coroutineJumpOffset = new VariableData(syntax, ctx.scope, 0, ctx.typeInt, nameIdentifier, offset, ctx.uniqueVariableId++);

		if (IsLookupOnlyVariable(ctx, function.coroutineJumpOffset))
			function.coroutineJumpOffset.lookupOnly = true;

		ctx.AddVariable(function.coroutineJumpOffset, false);

		AddFunctionCoroutineVariable(ctx, syntax, function, function.coroutineJumpOffset);

		coroutineStateRead = CreateVariableAccess(ctx, syntax, function.coroutineJumpOffset, true);
	}

	for(SynBase ref expression = syntax.expressions.head; expression; expression = expression.next)
		expressions.push_back(AnalyzeStatement(ctx, expression));

	if(!function.hasExplicitReturn)
	{
		Report(ctx, syntax, "ERROR: not a single element is generated, and an array element type is unknown");
	}
	else if(function.type.returnType == ctx.typeVoid)
	{
		Report(ctx, syntax, "ERROR: cannot generate an array of 'void' element type");
	}
	else
	{
		VariableData ref empty = AllocateTemporary(ctx, syntax, function.type.returnType);

		expressions.push_back(new ExprReturn(syntax, ctx.typeVoid, CreateVariableAccess(ctx, syntax, empty, false), CreateFunctionCoroutineStateUpdate(ctx, syntax, function, 0), CreateFunctionUpvalueClose(ctx, ctx.MakeInternal(syntax), function, ctx.scope)));
	}

	ClosePendingUpvalues(ctx, function);

	ctx.PopScope(ScopeType.SCOPE_FUNCTION);

	VariableData ref contextVariable = CreateFunctionContextVariable(ctx, syntax, function, nullptr);
	ExprVariableDefinition ref contextVariableDefinition = CreateFunctionContextVariableDefinition(ctx, syntax, function, nullptr, contextVariable);

	function.declaration = new ExprFunctionDefinition(syntax, function.type, function, contextArgumentDefinition, RefList<ExprVariableDefinition>(), coroutineStateRead, expressions, contextVariableDefinition, contextVariable);

	ctx.definitions.push_back(function.declaration);

	ExprBase ref access = new ExprFunctionAccess(syntax, function.type, function, CreateFunctionContextAccess(ctx, syntax, function));

	return CreateFunctionCall1(ctx, syntax, InplaceStr("__gen_list"), contextVariableDefinition ? CreateSequence(ctx, syntax, contextVariableDefinition, access) : access, false, true, true);
}

ExprBase ref AnalyzeShortFunctionDefinition(ExpressionContext ref ctx, SynShortFunctionDefinition ref syntax, TypeBase ref type, ArrayView<ArgumentData> currArguments, RefList<MatchData> aliases)
{
	TypeFunction ref functionType = getType with<TypeFunction>(type);

	// Only applies to function calls
	if(!functionType)
		return nullptr;

	RefList<TypeHandle> ref fuctionArgs = &functionType.arguments;

	// Function doesn't accept any more arguments
	if(currArguments.size() + 1 > fuctionArgs.size())
		return nullptr;

	// Get current argument type
	TypeBase ref target = nullptr;

	if(functionType.isGeneric)
	{
		// Collect aliases up to the current argument
		for(int i = 0; i < currArguments.size(); i++)
		{
			// Exit if the arguments before the short inline function fail to match
			if(!MatchGenericType(ctx, syntax, fuctionArgs[i].type, currArguments[i].type, aliases, false))
				return nullptr;
		}

		target = ResolveGenericTypeAliases(ctx, syntax, fuctionArgs[currArguments.size()].type, aliases);
	}
	else
	{
		target = fuctionArgs[currArguments.size()].type;
	}

	TypeFunction ref argumentType = getType with<TypeFunction>(target);

	if(!argumentType)
		return nullptr;

	return AnalyzeShortFunctionDefinition(ctx, syntax, nullptr, argumentType);
}

bool AssertResolvableType(ExpressionContext ref ctx, SynBase ref source, TypeBase ref type, bool allowGeneric)
{
	if(isType with<TypeArgumentSet>(type))
	{
		Report(ctx, source, "ERROR: expected '.first'/'.last'/'[N]'/'.size' after 'argument'");

		return false;
	}

	if(isType with<TypeMemberSet>(type))
	{
		Report(ctx, source, "ERROR: expected '(' after 'hasMember'");

		return false;
	}

	if(type.isGeneric && !allowGeneric)
	{
		Report(ctx, source, "ERROR: cannot take typeid from generic type");

		return false;
	}

	return true;
}

bool AssertResolvableTypeLiteral(ExpressionContext ref ctx, SynBase ref source, ExprBase ref expr)
{
	if(ExprTypeLiteral ref node = getType with<ExprTypeLiteral>(expr))
		return AssertResolvableType(ctx, source, node.value, false);

	return true;
}

bool AssertValueExpression(ExpressionContext ref ctx, SynBase ref source, ExprBase ref expr)
{
	if(isType with<ExprFunctionOverloadSet>(expr))
	{
		vector<FunctionValue> functions;

		GetNodeFunctions(ctx, source, expr, functions);

		if(ctx.errorBuf)
		{
			if(ctx.errorCount == 0)
			{
				ctx.errorPos = StringRef(ctx.code, source.pos.begin);
			}

			int messageStart = ctx.errorBuf.length();

			ctx.errorBuf += "ERROR: ambiguity, there is more than one overloaded function available:\n";

			ReportOnFunctionSelectError(ctx, source, ctx.errorBuf, messageStart, ViewOf(functions));
		}

		if(ctx.errorHandlerNested)
		{
			assert(ctx.errorHandlerActive);

			throw(new AnalyzerError());
		}

		ctx.errorCount++;

		return false;
	}

	if(isType with<ExprGenericFunctionPrototype>(expr))
	{
		Report(ctx, source, "ERROR: ambiguity, the expression is a generic function");

		return false;
	}

	if(ExprFunctionAccess ref node = getType with<ExprFunctionAccess>(expr))
	{
		if(ctx.IsGenericFunction(node.function))
		{
			Report(ctx, source, "ERROR: ambiguity, '%.*s' is a generic function", FMT_ISTR(node.function.name.name));

			return false;
		}

		if(node.function.type.returnType == ctx.typeAuto)
		{
			Report(ctx, source, "ERROR: function '%.*s' type is unresolved at this point", FMT_ISTR(node.function.name.name));

			return false;
		}
	}

	return AssertResolvableTypeLiteral(ctx, source, expr);
}

InplaceStr GetTypeConstructorName(InplaceStr functionName)
{
	// TODO: add type scopes and lookup owner namespace
	/*for(const char ref pos = functionName.end; pos > functionName.begin; pos--)
	{
		if(*pos == '.')
		{
			functionName = InplaceStr(pos + 1, functionName.end);
			break;
		}
	}*/

	return functionName;
}

InplaceStr GetTypeConstructorName(TypeClass ref classType)
{
	if(TypeGenericClassProto ref proto = classType.proto)
		return GetTypeConstructorName(proto.name);

	return GetTypeConstructorName(classType.name);
}

InplaceStr GetTypeConstructorName(TypeGenericClassProto ref typeGenericClassProto)
{
	return GetTypeConstructorName(typeGenericClassProto.name);
}

InplaceStr GetTypeDefaultConstructorName(ExpressionContext ref ctx, TypeClass ref classType)
{
	InplaceStr baseName = GetTypeConstructorName(classType);

	char[] name = new char[baseName.length() + 2];
	//sprintf(name, "%.*s$", FMT_ISTR(baseName));

	return InplaceStr(name);
}

bool ContainsSameOverload(ArrayView<FunctionData ref> functions, FunctionData ref value)
{
	for(int i = 0; i < functions.size(); i++)
	{
		if(SameArguments(functions[i].type, value.type))
			return true;
	}

	return false;
}

bool GetTypeConstructorFunctions(ExpressionContext ref ctx, TypeBase ref type, bool noArguments, vector<FunctionData ref> ref functions)
{
	TypeClass ref classType = getType with<TypeClass>(type);
	TypeGenericClassProto ref typeGenericClassProto = getType with<TypeGenericClassProto>(type);

	if(classType && classType.proto)
		typeGenericClassProto = classType.proto;

	int hash = StringHashContinue(type.nameHash, "::");

	if(classType)
	{
		InplaceStr functionName = GetTypeConstructorName(classType);

		hash = StringHashContinue(hash, functionName);
	}
	else
	{
		hash = StringHashContinue(hash, type.name);
	}

	for(auto node = ctx.functionMap.first(hash); node; node = ctx.functionMap.next(node))
	{
		if(noArguments && !node.value.arguments.empty() && !node.value.arguments[0].value)
			continue;

		if(node.value.scope.ownerType != type)
			continue;

		if(!ContainsSameOverload(ViewOf(functions), node.value))
			functions.push_back(node.value);
	}

	if(typeGenericClassProto)
	{
		// Look for a member function in a generic class base and instantiate them
		int hash = StringHashContinue(typeGenericClassProto.nameHash, "::");

		InplaceStr functionName = GetTypeConstructorName(typeGenericClassProto);

		hash = StringHashContinue(hash, functionName);

		for(auto node = ctx.functionMap.first(hash); node; node = ctx.functionMap.next(node))
		{
			if(noArguments && !node.value.arguments.empty() && !node.value.arguments[0].value)
				continue;

			if(!ContainsSameOverload(ViewOf(functions), node.value))
				functions.push_back(node.value);
		}
	}

	for(auto node = ctx.functionMap.first(StringHashContinue(hash, "$")); node; node = ctx.functionMap.next(node))
	{
		if(noArguments && !node.value.arguments.empty() && !node.value.arguments[0].value)
			continue;

		if(node.value.scope.ownerType != type)
			continue;

		if(!ContainsSameOverload(ViewOf(functions), node.value))
			functions.push_back(node.value);
	}

	if(typeGenericClassProto)
	{
		// Look for a member function in a generic class base and instantiate them
		int hash = StringHashContinue(typeGenericClassProto.nameHash, "::");

		InplaceStr functionName = GetTypeConstructorName(typeGenericClassProto);

		hash = StringHashContinue(hash, functionName);

		for(auto node = ctx.functionMap.first(StringHashContinue(hash, "$")); node; node = ctx.functionMap.next(node))
		{
			if(noArguments && !node.value.arguments.empty() && !node.value.arguments[0].value)
				continue;

			if(!ContainsSameOverload(ViewOf(functions), node.value))
				functions.push_back(node.value);
		}
	}

	return !functions.empty();
}

ExprBase ref CreateConstructorAccess(ExpressionContext ref ctx, SynBase ref source, ArrayView<FunctionData ref> functions, ExprBase ref context)
{
	if(functions.size() > 1)
	{
		vector<TypeBase ref> types;
		RefList<FunctionHandle> handles;

		for(int i = 0; i < functions.size(); i++)
		{
			FunctionData ref curr = functions[i];

			types.push_back(curr.type);
			handles.push_back(new FunctionHandle(curr));
		}

		TypeFunctionSet ref type = ctx.GetFunctionSetType(ArrayView<TypeBase ref>(types));

		return new ExprFunctionOverloadSet(source, type, handles, context);
	}

	return new ExprFunctionAccess(source, functions[0].type, functions[0], context);
}

ExprBase ref CreateConstructorAccess(ExpressionContext ref ctx, SynBase ref source, TypeBase ref type, bool noArguments, ExprBase ref context)
{
	vector<FunctionData ref> functions;

	if(GetTypeConstructorFunctions(ctx, type, noArguments, functions))
		return CreateConstructorAccess(ctx, source, ViewOf(functions), context);

	return nullptr;
}

bool HasDefaultConstructor(ExpressionContext ref ctx, SynBase ref source, TypeBase ref type)
{
	// Find array element type
	TypeArray ref arrType = getType with<TypeArray>(type);

	while(arrType)
	{
		type = arrType.subType;

		arrType = getType with<TypeArray>(type);
	}

	vector<FunctionData ref> functions;

	if(GetTypeConstructorFunctions(ctx, type, true, functions))
	{
		vector<FunctionValue> overloads;

		for(int i = 0; i < functions.size(); i++)
		{
			FunctionData ref curr = functions[i];

			overloads.push_back(FunctionValue(source, curr, new ExprNullptrLiteral(source, curr.contextType)));
		}

		vector<int> ratings;
		vector<ArgumentData> arguments;

		if(FunctionValue bestOverload = SelectBestFunction(ctx, source, ViewOf(overloads), RefList<TypeHandle>(), ViewOf(arguments), ratings))
			return true;

		return false;
	}

	return false;
}

ExprBase ref CreateDefaultConstructorCall(ExpressionContext ref ctx, SynBase ref source, TypeBase ref type, ExprBase ref pointer)
{
	assert(isType with<TypeRef>(pointer.type) || isType with<TypeUnsizedArray>(pointer.type));

	if(isType with<TypeArray>(type) || isType with<TypeUnsizedArray>(type))
	{
		if(TypeArray ref arrType = getType with<TypeArray>(type))
			type = arrType.subType;
		else if(TypeUnsizedArray ref arrType = getType with<TypeUnsizedArray>(type))
			type = arrType.subType;

		if(HasDefaultConstructor(ctx, source, type))
		{
			if(TypeRef ref typeRef = getType with<TypeRef>(pointer.type))
				return CreateFunctionCall1(ctx, source, InplaceStr("__init_array"), new ExprDereference(source, typeRef.subType, pointer), false, true, true);

			return CreateFunctionCall1(ctx, source, InplaceStr("__init_array"), pointer, false, true, true);
		}

		return nullptr;
	}

	if(ExprBase ref constructor = CreateConstructorAccess(ctx, source, type, true, pointer))
	{
		// Collect a set of available functions
		vector<FunctionValue> functions;

		GetNodeFunctions(ctx, source, constructor, functions);

		return CreateFunctionCallOverloaded(ctx, source, constructor, ViewOf(functions), RefList<TypeHandle>(), nullptr, false);
	}

	return nullptr;
}

void CreateDefaultConstructorCode(ExpressionContext ref ctx, SynBase ref source, TypeClass ref classType, RefList<ExprBase> ref expressions)
{
	for(MemberHandle ref el = classType.members.head; el; el = el.next)
	{
		VariableData ref variable = el.variable;

		ExprBase ref access = CreateVariableAccess(ctx, source, variable, true);

		ExprBase ref member = CreateGetAddress(ctx, source, access);

		if(variable.name.name == InplaceStr("$typeid"))
		{
			if(classType.baseClass)
			{
				assert(HasDefaultConstructor(ctx, source, classType.baseClass));

				ExprBase ref thisAccess = CreateVariableAccess(ctx, source, RefList<SynIdentifier>(), InplaceStr("this"), false);

				if(!thisAccess)
					Stop(ctx, source, "ERROR: 'this' variable is not available");

				ExprBase ref cast = CreateCast(ctx, source, thisAccess, ctx.GetReferenceType(classType.baseClass), true);

				if(ExprBase ref call = CreateDefaultConstructorCall(ctx, source, classType.baseClass, cast))
					expressions.push_back(call);
			}

			expressions.push_back(CreateAssignment(ctx, source, member, new ExprTypeLiteral(source, ctx.typeTypeID, classType)));
			continue;
		}

		if(el.initializer)
		{
			ExprBase ref initializer = ResolveInitializerValue(ctx, el.initializer, AnalyzeExpression(ctx, el.initializer));

			TypeArray ref arrType = getType with<TypeArray>(variable.type);

			// Single-level array might be set with a single element at the point of definition
			if(arrType && !isType with<TypeArray>(initializer.type) && initializer.type != (TypeBase ref)(ctx.typeAutoArray))
			{
				initializer = CreateCast(ctx, initializer.source, initializer, arrType.subType, false);

				if(ExprVariableAccess ref node = getType with<ExprVariableAccess>(access))
					access = new ExprGetAddress(access.source, ctx.GetReferenceType(access.type), new VariableHandle(node.source, node.variable));
				else if(ExprDereference ref node = getType with<ExprDereference>(access))
					access = node.value;

				expressions.push_back(new ExprArraySetup(initializer.source, ctx.typeVoid, access, initializer));
			}
			else
			{
				expressions.push_back(CreateAssignment(ctx, initializer.source, access, initializer));
			}
		}
		else if(HasDefaultConstructor(ctx, source, variable.type))
		{
			if(ExprBase ref call = CreateDefaultConstructorCall(ctx, source, variable.type, member))
				expressions.push_back(call);
		}
	}
}

void CreateDefaultClassConstructor(ExpressionContext ref ctx, SynBase ref source, ExprClassDefinition ref classDefinition)
{
	TypeClass ref classType = classDefinition.classType;

	// Check if custom default assignment operator is required
	bool customConstructor = false;

	if(classType.isExtendable)
	{
		customConstructor = true;
	}
	else
	{
		for(MemberHandle ref el = classType.members.head; el; el = el.next)
		{
			TypeBase ref base = el.variable.type;

			// Find array element type
			TypeArray ref arrType = getType with<TypeArray>(base);

			while(arrType)
			{
				base = arrType.subType;

				arrType = getType with<TypeArray>(base);
			}

			if(el.initializer)
			{
				customConstructor = true;
				break;
			}

			if(HasDefaultConstructor(ctx, source, base))
			{
				customConstructor = true;
				break;
			}
		}
	}

	if(customConstructor)
	{
		bool addedParentScope = RestoreParentTypeScope(ctx, source, classType);

		InplaceStr functionName = GetFunctionNameInScope(ctx, ctx.scope, classType, GetTypeDefaultConstructorName(ctx, classType), false, false);

		vector<ArgumentData> arguments;

		SynIdentifier ref functionNameIdentifier = new SynIdentifier(functionName);

		FunctionData ref function = new FunctionData(source, ctx.scope, false, false, false, ctx.GetFunctionType(source, ctx.typeVoid, ViewOf(arguments)), ctx.GetReferenceType(classType), functionNameIdentifier, RefList<MatchData>(), ctx.uniqueFunctionId++);

		CheckFunctionConflict(ctx, source, function.name.name);

		ctx.AddFunction(function);

		ctx.PushScope(function);

		function.functionScope = ctx.scope;

		ExprVariableDefinition ref contextArgumentDefinition = CreateFunctionContextArgument(ctx, ctx.MakeInternal(source), function);

		function.argumentsSize = function.functionScope.dataSize;

		RefList<ExprBase> expressions;

		CreateDefaultConstructorCode(ctx, source, classType, expressions);

		expressions.push_back(new ExprReturn(source, ctx.typeVoid, new ExprVoid(source, ctx.typeVoid), nullptr, CreateFunctionUpvalueClose(ctx, ctx.MakeInternal(source), function, ctx.scope)));

		ClosePendingUpvalues(ctx, function);

		ctx.PopScope(ScopeType.SCOPE_FUNCTION);

		if(addedParentScope)
			ctx.PopScope(ScopeType.SCOPE_TYPE);

		VariableData ref contextVariable = CreateFunctionContextVariable(ctx, source, function, nullptr);
		ExprVariableDefinition ref contextVariableDefinition = CreateFunctionContextVariableDefinition(ctx, source, function, nullptr, contextVariable);

		function.declaration = new ExprFunctionDefinition(source, function.type, function, contextArgumentDefinition, RefList<ExprVariableDefinition>(), nullptr, expressions, contextVariableDefinition, contextVariable);

		ctx.definitions.push_back(function.declaration);

		classDefinition.functions.push_back(function.declaration);
	}
}

void CreateDefaultClassAssignment(ExpressionContext ref ctx, SynBase ref source, ExprClassDefinition ref classDefinition)
{
	ctx.PopScope(ScopeType.SCOPE_TYPE);

	TypeClass ref classType = classDefinition.classType;

	RefList<VariableHandle> customAssignMembers;

	for(MemberHandle ref curr = classType.members.head; curr; curr = curr.next)
	{
		TypeBase ref type = curr.variable.type;

		if(isType with<TypeRef>(type) || isType with<TypeArray>(type) || isType with<TypeUnsizedArray>(type) || isType with<TypeFunction>(type) || isType with<TypeError>(type))
			continue;

		vector<ArgumentData> arguments;

		arguments.push_back(ArgumentData(source, false, nullptr, ctx.GetReferenceType(type), nullptr));
		arguments.push_back(ArgumentData(source, false, nullptr, type, nullptr));

		if(ExprBase ref overloads = CreateVariableAccess(ctx, source, RefList<SynIdentifier>(), InplaceStr("="), false))
		{
			vector<FunctionValue> functions;

			GetNodeFunctions(ctx, source, overloads, functions);

			if(!functions.empty())
			{
				vector<int> ratings;

				FunctionValue bestOverload = SelectBestFunction(ctx, source, ViewOf(functions), RefList<TypeHandle>(), ViewOf(arguments), ratings);

				if(bestOverload)
				{
					customAssignMembers.push_back(new VariableHandle(curr.source, curr.variable));
					continue;
				}
			}
		}

		if(ExprBase ref overloads = CreateVariableAccess(ctx, source, RefList<SynIdentifier>(), InplaceStr("default_assign$_"), false))
		{
			vector<FunctionValue> functions;

			GetNodeFunctions(ctx, source, overloads, functions);

			if(!functions.empty())
			{
				vector<int> ratings;

				FunctionValue bestOverload = SelectBestFunction(ctx, source, ViewOf(functions), RefList<TypeHandle>(), ViewOf(arguments), ratings);

				if(bestOverload)
				{
					customAssignMembers.push_back(new VariableHandle(curr.source, curr.variable));
					continue;
				}
			}
		}
	}

	if(!customAssignMembers.empty())
	{
		InplaceStr functionName = InplaceStr("default_assign$_");

		vector<ArgumentData> arguments;

		arguments.push_back(ArgumentData(source, false, new SynIdentifier(InplaceStr("$left")), ctx.GetReferenceType(classType), nullptr));
		arguments.push_back(ArgumentData(source, false, new SynIdentifier(InplaceStr("$right")), classType, nullptr));

		SynIdentifier ref functionNameIdentifier = new SynIdentifier(functionName);

		FunctionData ref function = new FunctionData(source, ctx.scope, false, false, false, ctx.GetFunctionType(source, ctx.typeVoid, ViewOf(arguments)), ctx.GetReferenceType(ctx.typeVoid), functionNameIdentifier, RefList<MatchData>(), ctx.uniqueFunctionId++);

		// Fill in argument data
		for(int i = 0; i < arguments.size(); i++)
			function.arguments.push_back(arguments[i]);

		CheckFunctionConflict(ctx, source, function.name.name);

		ctx.AddFunction(function);

		ctx.PushScope(function);

		function.functionScope = ctx.scope;

		RefList<ExprVariableDefinition> variables;

		CreateFunctionArgumentVariables(ctx, source, function, ViewOf(arguments), variables);

		ExprVariableDefinition ref contextArgumentDefinition = CreateFunctionContextArgument(ctx, ctx.MakeInternal(source), function);

		function.argumentsSize = function.functionScope.dataSize;

		RefList<ExprBase> expressions;

		for(VariableHandle ref curr = customAssignMembers.head; curr; curr = curr.next)
		{
			VariableData ref leftArgument = variables.head.variable.variable;

			ExprBase ref left = CreateVariableAccess(ctx, source, leftArgument, false);

			ExprBase ref leftMember = CreateMemberAccess(ctx, source, left, curr.variable.name, false);

			VariableData ref rightArgument = getType with<ExprVariableDefinition>(variables.head.next).variable.variable;

			ExprBase ref right = CreateVariableAccess(ctx, source, rightArgument, false);

			ExprBase ref rightMember = CreateMemberAccess(ctx, source, right, curr.variable.name, false);

			expressions.push_back(CreateAssignment(ctx, source, leftMember, rightMember));
		}

		expressions.push_back(new ExprReturn(source, ctx.typeVoid, new ExprVoid(source, ctx.typeVoid), nullptr, nullptr));

		ClosePendingUpvalues(ctx, function);

		ctx.PopScope(ScopeType.SCOPE_FUNCTION);

		function.declaration = new ExprFunctionDefinition(source, function.type, function, contextArgumentDefinition, variables, nullptr, expressions, nullptr, nullptr);

		ctx.definitions.push_back(function.declaration);

		classDefinition.functions.push_back(function.declaration);
	}

	RestoreParentTypeScope(ctx, source, classDefinition.classType);
}

void CreateDefaultClassMembers(ExpressionContext ref ctx, SynBase ref source, ExprClassDefinition ref classDefinition)
{
	CreateDefaultClassConstructor(ctx, ctx.MakeInternal(source), classDefinition);

	CreateDefaultClassAssignment(ctx, ctx.MakeInternal(source), classDefinition);
}

void AnalyzeClassStaticIf(ExpressionContext ref ctx, ExprClassDefinition ref classDefinition, SynClassStaticIf ref syntax, bool baseElements)
{
	ExprBase ref condition = AnalyzeExpression(ctx, syntax.condition);

	condition = CreateConditionCast(ctx, condition.source, condition);

	if(ExprBoolLiteral ref number = getType with<ExprBoolLiteral>(EvaluateExpression(ctx, syntax, CreateCast(ctx, syntax, condition, ctx.typeBool, false))))
	{
		if(number.value)
		{
			if(baseElements)
				AnalyzeClassBaseElements(ctx, classDefinition, syntax.trueBlock);
			else
				AnalyzeClassFunctionElements(ctx, classDefinition, syntax.trueBlock);
		}
		else if(syntax.falseBlock)
		{
			if(baseElements)
				AnalyzeClassBaseElements(ctx, classDefinition, syntax.falseBlock);
			else
				AnalyzeClassFunctionElements(ctx, classDefinition, syntax.falseBlock);
		}
	}
	else
	{
		Report(ctx, syntax, "ERROR: can't get condition value");
	}
}

void AnalyzeClassConstants(ExpressionContext ref ctx, SynBase ref source, TypeBase ref type, RefList<SynConstant> constants, RefList<ConstantData> ref target)
{
	for(SynConstant ref constant = constants.head; constant; constant = getType with<SynConstant>(constant.next))
	{
		ExprBase ref value = nullptr;
			
		if(constant.value)
		{
			value = AnalyzeExpression(ctx, constant.value);

			if(isType with<TypeError>(value.type))
				continue;

			if(type == ctx.typeAuto)
				type = value.type;

			if(!ctx.IsNumericType(type))
				Stop(ctx, source, "ERROR: only basic numeric types can be used as constants");

			value = EvaluateExpression(ctx, source, CreateCast(ctx, constant, value, type, false));
		}
		else if(ctx.IsIntegerType(type) && !target.empty())
		{
			value = getType with<ExprIntegerLiteral>(EvaluateExpression(ctx, source, CreateCast(ctx, constant, CreateBinaryOp(ctx, constant, SynBinaryOpType.SYN_BINARY_OP_ADD, target.tail.value, new ExprIntegerLiteral(constant, type, 1)), type, false)));
		}
		else
		{
			if(constant == constants.head)
				Report(ctx, source, "ERROR: '=' not found after constant name");
			else
				Report(ctx, source, "ERROR: only integer constant list gets automatically incremented by 1");

			value = new ExprIntegerLiteral(constant, ctx.typeInt, 0);
		}

		if(!value || (!isType with<ExprBoolLiteral>(value) && !isType with<ExprCharacterLiteral>(value) && !isType with<ExprIntegerLiteral>(value) && !isType with<ExprRationalLiteral>(value)))
		{
			Report(ctx, source, "ERROR: expression didn't evaluate to a constant number");

			value = new ExprIntegerLiteral(constant, ctx.typeInt, 0);
		}

		for(ConstantData ref curr = target.head; curr; curr = curr.next)
		{
			if(constant.name.name == curr.name.name)
				Report(ctx, source, "ERROR: name '%.*s' is already taken", FMT_ISTR(curr.name.name));
		}

		CheckVariableConflict(ctx, constant, constant.name.name);

		target.push_back(new ConstantData(constant.name, value));
	}
}

void AnalyzeClassBaseElements(ExpressionContext ref ctx, ExprClassDefinition ref classDefinition, SynClassElements ref syntax)
{
	for(SynTypedef ref typeDef = syntax.typedefs.head; typeDef; typeDef = getType with<SynTypedef>(typeDef.next))
	{
		ExprAliasDefinition ref alias = AnalyzeTypedef(ctx, typeDef);

		classDefinition.classType.aliases.push_back(new MatchData(alias.alias.name, alias.alias.type));
	}

	{
		for(SynVariableDefinitions ref member = syntax.members.head; member; member = getType with<SynVariableDefinitions>(member.next))
		{
			ExprVariableDefinitions ref node = AnalyzeVariableDefinitions(ctx, member);

			for(ExprBase ref definition = node.definitions.head; definition; definition = definition.next)
			{
				if(isType with<TypeError>(definition.type))
					continue;

				ExprVariableDefinition ref variableDefinition = getType with<ExprVariableDefinition>(definition);

				assert(variableDefinition != nullptr);

				SynVariableDefinition ref sourceDefinition = getType with<SynVariableDefinition>(definition.source);

				classDefinition.classType.members.push_back(new MemberHandle(variableDefinition.variable.source, variableDefinition.variable.variable, sourceDefinition ? sourceDefinition.initializer : nullptr));
			}
		}
	}

	for(SynConstantSet ref constantSet = syntax.constantSets.head; constantSet; constantSet = getType with<SynConstantSet>(constantSet.next))
	{
		TypeBase ref type = AnalyzeType(ctx, constantSet.type);

		AnalyzeClassConstants(ctx, constantSet, type, constantSet.constants, classDefinition.classType.constants);
	}

	// TODO: The way SynClassElements is made, it could allow member re-ordering! class should contain in-order members and static if's
	// TODO: We should be able to analyze all static if typedefs before members and constants and analyze them before functions
	for(SynClassStaticIf ref staticIf = syntax.staticIfs.head; staticIf; staticIf = getType with<SynClassStaticIf>(staticIf.next))
		AnalyzeClassStaticIf(ctx, classDefinition, staticIf, true);
}

void AnalyzeClassFunctionElements(ExpressionContext ref ctx, ExprClassDefinition ref classDefinition, SynClassElements ref syntax)
{
	for(SynFunctionDefinition ref function = syntax.functions.head; function; function = getType with<SynFunctionDefinition>(function.next))
		classDefinition.functions.push_back(AnalyzeFunctionDefinition(ctx, function, nullptr, nullptr, nullptr, RefList<MatchData>(), false, false, true));

	for(SynAccessor ref accessor = syntax.accessors.head; accessor; accessor = getType with<SynAccessor>(accessor.next))
	{
		SynBase ref parentType = new SynTypeSimple(accessor.begin, accessor.end, RefList<SynIdentifier>(), classDefinition.classType.name);

		TypeBase ref accessorType = AnalyzeType(ctx, accessor.type);

		if(accessor.getBlock && !isType with<TypeError>(accessorType))
		{
			RefList<SynIdentifier> aliases;
			RefList<SynFunctionArgument> arguments;

			RefList<SynBase> expressions;

			if(SynBlock ref block = getType with<SynBlock>(accessor.getBlock))
				expressions = block.expressions;
			else if(SynError ref error = getType with<SynError>(accessor.getBlock))
				expressions.push_back(new SynError(error.begin, error.end));

			SynFunctionDefinition ref function = new SynFunctionDefinition(accessor.begin, accessor.end, false, false, parentType, true, accessor.type, false, accessor.name, aliases, arguments, expressions);

			TypeFunction ref instance = ctx.GetFunctionType(syntax, accessorType, RefList<TypeHandle>());

			ExprBase ref definition = AnalyzeFunctionDefinition(ctx, function, nullptr, instance, nullptr, RefList<MatchData>(), false, false, false);

			if(ExprFunctionDefinition ref node = getType with<ExprFunctionDefinition>(definition))
				accessorType = node.function.type.returnType;

			classDefinition.functions.push_back(definition);
		}

		if(accessor.setBlock && !isType with<TypeError>(accessorType))
		{
			RefList<SynIdentifier> aliases;

			RefList<SynFunctionArgument> arguments;

			SynIdentifier ref setName = accessor.setName;

			if(!setName)
				setName = new SynIdentifier(InplaceStr("r"));

			arguments.push_back(new SynFunctionArgument(accessor.begin, accessor.end, false, accessor.type, setName, nullptr));

			RefList<SynBase> expressions;

			if(SynBlock ref block = getType with<SynBlock>(accessor.setBlock))
				expressions = block.expressions;
			else if(SynError ref error = getType with<SynError>(accessor.setBlock))
				expressions.push_back(new SynError(error.begin, error.end));

			SynFunctionDefinition ref function = new SynFunctionDefinition(accessor.begin, accessor.end, false, false, parentType, true, new SynTypeAuto(accessor.begin, accessor.end), false, accessor.name, aliases, arguments, expressions);

			RefList<TypeHandle> argTypes;
			argTypes.push_back(new TypeHandle(accessorType));

			TypeFunction ref instance = ctx.GetFunctionType(syntax, ctx.typeAuto, argTypes);

			classDefinition.functions.push_back(AnalyzeFunctionDefinition(ctx, function, nullptr, instance, nullptr, RefList<MatchData>(), false, false, false));
		}
	}

	for(SynClassStaticIf ref staticIf = syntax.staticIfs.head; staticIf; staticIf = getType with<SynClassStaticIf>(staticIf.next))
		AnalyzeClassStaticIf(ctx, classDefinition, staticIf, false);
}

void AnalyzeClassElements(ExpressionContext ref ctx, ExprClassDefinition ref classDefinition, SynClassElements ref syntax)
{
	AnalyzeClassBaseElements(ctx, classDefinition, syntax);

	FinalizeAlignment(classDefinition.classType);

	assert(!classDefinition.classType.completed);

	classDefinition.classType.completed = true;

	if(classDefinition.classType.size >= 64 * 1024)
		Stop(ctx, syntax, "ERROR: class size cannot exceed 65535 bytes");

	CreateDefaultClassMembers(ctx, syntax, classDefinition);

	AnalyzeClassFunctionElements(ctx, classDefinition, syntax);
}

ExprBase ref AnalyzeClassDefinition(ExpressionContext ref ctx, SynClassDefinition ref syntax, TypeGenericClassProto ref proto, RefList<TypeHandle> generics)
{
	CheckTypeConflict(ctx, syntax, syntax.name.name);

	InplaceStr typeName = GetTypeNameInScope(ctx, ctx.scope, syntax.name.name);

	/*TRACE_SCOPE("analyze", "AnalyzeClassDefinition");
	TRACE_LABEL2(typeName.begin, typeName.end);*/

	if(!proto && !syntax.aliases.empty())
	{
		for(SynIdentifier ref curr = syntax.aliases.head; curr; curr = getType with<SynIdentifier>(curr.next))
		{
			if(ctx.typeMap.find(curr.name.hash()))
				Stop(ctx, curr, "ERROR: there is already a type or an alias with the same name");

			for(SynIdentifier ref prev = syntax.aliases.head; prev && prev != curr; prev = getType with<SynIdentifier>(prev.next))
			{
				if(prev.name == curr.name)
					Stop(ctx, curr, "ERROR: there is already a type or an alias with the same name");
			}
		}

		if(TypeBase ref ref type = ctx.typeMap.find(typeName.hash()))
		{
			TypeClass ref originalDefinition = getType with<TypeClass>(*type);

			if(originalDefinition)
				Stop(ctx, syntax, "ERROR: type '%.*s' was forward declared as a non-generic type", FMT_ISTR(typeName));
			else
				Stop(ctx, syntax, "ERROR: '%.*s' is being redefined", FMT_ISTR(typeName));
		}

		TypeGenericClassProto ref genericProtoType = new TypeGenericClassProto(SynIdentifier(syntax.name, typeName), syntax, ctx.scope, syntax);

		ctx.AddType(genericProtoType);

		return new ExprGenericClassPrototype(syntax, ctx.typeVoid, genericProtoType);
	}

	assert(generics.size() == syntax.aliases.size());

	InplaceStr className = generics.empty() ? typeName : GetGenericClassTypeName(ctx, proto, generics);

	if(className.length() > NULLC_MAX_TYPE_NAME_LENGTH)
		Stop(ctx, syntax, "ERROR: generated type name exceeds maximum type length '%d'", NULLC_MAX_TYPE_NAME_LENGTH);

	TypeClass ref originalDefinition = nullptr;

	if(TypeBase ref ref type = ctx.typeMap.find(className.hash()))
	{
		originalDefinition = getType with<TypeClass>(*type);

		if(!originalDefinition)
			Stop(ctx, syntax, "ERROR: '%.*s' is being redefined", FMT_ISTR(className));

		if(originalDefinition.completed)
			Stop(ctx, syntax, "ERROR: '%.*s' is being redefined", FMT_ISTR(className));

		for(ScopeData ref scope = ctx.scope; scope; scope = scope.scope)
		{
			if(scope.ownerType == originalDefinition)
				Stop(ctx, syntax, "ERROR: '%.*s' is being redefined", FMT_ISTR(className));
		}
	}

	if(!generics.empty())
	{
		// Check if type already exists
		assert(ctx.genericTypeMap.find(className.hash()) == nullptr);
	}

	int alignment = syntax.alignment ? AnalyzeAlignment(ctx, syntax.alignment) : 0;

	RefList<MatchData> actualGenerics;

	{
		TypeHandle ref currType = generics.head;
		SynIdentifier ref currName = syntax.aliases.head;

		while(currType && currName)
		{
			assert(!isType with<TypeError>(currType.type));

			actualGenerics.push_back(new MatchData(currName, currType.type));

			currType = currType.next;
			currName = getType with<SynIdentifier>(currName.next);
		}
	}

	TypeClass ref baseClass = nullptr;

	if(syntax.baseClass)
	{
		ctx.PushTemporaryScope();

		for(MatchData ref el = actualGenerics.head; el; el = el.next)
			ctx.AddAlias(new AliasData(syntax, ctx.scope, el.type, el.name, ctx.uniqueAliasId++));

		TypeBase ref type = AnalyzeType(ctx, syntax.baseClass);

		ctx.PopScope(ScopeType.SCOPE_TEMPORARY);

		if(!isType with<TypeError>(type))
		{
			baseClass = getType with<TypeClass>(type);

			if(!baseClass || !baseClass.isExtendable)
				Stop(ctx, syntax, "ERROR: type '%.*s' is not extendable", FMT_ISTR(type.name));
		}
	}
	
	bool isExtendable = syntax.isExtendable || baseClass;

	TypeClass ref classType = nullptr;
	
	if(originalDefinition)
	{
		classType = originalDefinition;

		classType.isExtendable = isExtendable;
		classType.baseClass = baseClass;
	}
	else
	{
		classType = new TypeClass(SynIdentifier(syntax.name, className), syntax, ctx.scope, proto, actualGenerics, isExtendable, baseClass);

		ctx.AddType(classType);
	}

	if(!generics.empty())
		ctx.genericTypeMap.insert(className.hash(), classType);

	ExprClassDefinition ref classDefinition = new ExprClassDefinition(syntax, ctx.typeVoid, classType);

	ctx.PushScope(classType);

	classType.typeScope = ctx.scope;

	for(MatchData ref el = classType.generics.head; el; el = el.next)
		ctx.AddAlias(new AliasData(syntax, ctx.scope, el.type, el.name, ctx.uniqueAliasId++));

	// Base class adds a typeid member
	if(isExtendable)
	{
		int offset = AllocateVariableInScope(ctx, syntax, ctx.typeTypeID.alignment, ctx.typeTypeID);

		SynIdentifier ref nameIdentifier = new SynIdentifier(InplaceStr("$typeid"));

		VariableData ref member = new VariableData(syntax, ctx.scope, ctx.typeTypeID.alignment, ctx.typeTypeID, nameIdentifier, offset, ctx.uniqueVariableId++);

		ctx.AddVariable(member, false);

		classType.members.push_back(new MemberHandle(nullptr, member, nullptr));
	}

	if(baseClass)
	{
		// Use base class alignment at ths point to match member locations
		classType.alignment = baseClass.alignment;

		// Add members of base class
		for(MatchData ref el = baseClass.aliases.head; el; el = el.next)
		{
			ctx.AddAlias(new AliasData(syntax, ctx.scope, el.type, el.name, ctx.uniqueAliasId++));

			classType.aliases.push_back(new MatchData(el.name, el.type));
		}

		for(MemberHandle ref el = baseClass.members.head; el; el = el.next)
		{
			if(el.variable.name.name == InplaceStr("$typeid"))
				continue;

			bool conflict = CheckVariableConflict(ctx, syntax, el.variable.name.name);

			int offset = AllocateVariableInScope(ctx, syntax, el.variable.alignment, el.variable.type);

			assert(offset == el.variable.offset);

			VariableData ref member = new VariableData(syntax, ctx.scope, el.variable.alignment, el.variable.type, el.variable.name, offset, ctx.uniqueVariableId++);

			if(!conflict)
				ctx.AddVariable(member, true);

			classType.members.push_back(new MemberHandle(el.variable.source, member, nullptr));
		}

		for(ConstantData ref el = baseClass.constants.head; el; el = el.next)
			classType.constants.push_back(new ConstantData(el.name, el.value));

		FinalizeAlignment(classType);

		assert(classType.size == baseClass.size);
	}

	if(syntax.alignment)
		classType.alignment = alignment;

	AnalyzeClassElements(ctx, classDefinition, syntax.elements);

	ctx.PopScope(ScopeType.SCOPE_TYPE);

	return classDefinition;
}

ExprBase ref AnalyzeClassPrototype(ExpressionContext ref ctx, SynClassPrototype ref syntax)
{
	CheckTypeConflict(ctx, syntax, syntax.name.name);

	InplaceStr typeName = GetTypeNameInScope(ctx, ctx.scope, syntax.name.name);

	if(TypeBase ref ref type = ctx.typeMap.find(typeName.hash()))
	{
		TypeClass ref originalDefinition = getType with<TypeClass>(*type);

		if(!originalDefinition || originalDefinition.completed)
			Stop(ctx, syntax, "ERROR: '%.*s' is being redefined", FMT_ISTR(syntax.name.name));

		return new ExprClassPrototype(syntax, ctx.typeVoid, originalDefinition);
	}

	RefList<MatchData> actualGenerics;

	TypeClass ref classType = new TypeClass(SynIdentifier(syntax.name, typeName), syntax, ctx.scope, nullptr, actualGenerics, false, nullptr);

	ctx.AddType(classType);

	return new ExprClassPrototype(syntax, ctx.typeVoid, classType);
}

void AnalyzeEnumConstants(ExpressionContext ref ctx, TypeBase ref type, RefList<SynConstant> constants, RefList<ConstantData> ref target)
{
	ExprIntegerLiteral ref last = nullptr;

	for(SynConstant ref constant = constants.head; constant; constant = getType with<SynConstant>(constant.next))
	{
		ExprIntegerLiteral ref value = nullptr;
			
		if(constant.value)
		{
			ExprBase ref rhs = AnalyzeExpression(ctx, constant.value);

			if(isType with<TypeError>(rhs.type))
				continue;

			if(rhs.type == type)
				value = getType with<ExprIntegerLiteral>(EvaluateExpression(ctx, constant, rhs));
			else
				value = getType with<ExprIntegerLiteral>(EvaluateExpression(ctx, constant, CreateCast(ctx, constant, rhs, ctx.typeInt, false)));
		}
		else if(last)
		{
			value = new ExprIntegerLiteral(constant, ctx.typeInt, last.value + 1);
		}
		else
		{
			value = new ExprIntegerLiteral(constant, ctx.typeInt, 0);
		}

		if(!value)
		{
			Report(ctx, constant, "ERROR: expression didn't evaluate to a constant number");

			value = new ExprIntegerLiteral(constant, ctx.typeInt, 0);
		}

		last = value;

		for(ConstantData ref curr = target.head; curr; curr = curr.next)
		{
			if(constant.name.name == curr.name.name)
				Report(ctx, constant, "ERROR: name '%.*s' is already taken", FMT_ISTR(curr.name.name));
		}

		CheckVariableConflict(ctx, constant, constant.name.name);

		target.push_back(new ConstantData(constant.name, new ExprIntegerLiteral(constant, type, value.value)));
	}
}

ExprBase ref AnalyzeEnumDefinition(ExpressionContext ref ctx, SynEnumDefinition ref syntax)
{
	CheckTypeConflict(ctx, syntax, syntax.name.name);

	InplaceStr typeName = GetTypeNameInScope(ctx, ctx.scope, syntax.name.name);

	if(ctx.typeMap.find(typeName.hash()) != nullptr)
		Stop(ctx, syntax, "ERROR: '%.*s' is being redefined", FMT_ISTR(syntax.name.name));

	TypeEnum ref enumType = new TypeEnum(SynIdentifier(syntax.name, typeName), syntax, ctx.scope);

	ctx.AddType(enumType);

	ctx.PushScope(enumType);

	enumType.typeScope = ctx.scope;

	AnalyzeEnumConstants(ctx, enumType, syntax.values, enumType.constants);

	enumType.alignment = ctx.typeInt.alignment;

	ctx.PopScope(ScopeType.SCOPE_TYPE);
	
	ScopeData ref scope = ctx.scope;

	// Switch to global scope
	ctx.SwitchToScopeAtPoint(ctx.globalScope, nullptr);

	ExprBase ref castToInt = nullptr;
	ExprBase ref castToEnum = nullptr;

	bool prevErrorHandlerNested = ctx.errorHandlerNested;
	ctx.errorHandlerNested = true;

	//int traceDepth = NULLC::TraceGetDepth();

	auto tryResult = try(auto(){
		SynBase ref syntaxInternal = ctx.MakeInternal(syntax);

		// Create conversion operator int int(enum_type)
		{
			vector<ArgumentData> arguments;
			arguments.push_back(ArgumentData(syntaxInternal, false, new SynIdentifier(InplaceStr("$x")), enumType, nullptr));

			SynIdentifier ref functionNameIdentifier = new SynIdentifier(InplaceStr("int"));

			TypeFunction ref typeFunction = ctx.GetFunctionType(syntaxInternal, ctx.typeInt, ViewOf(arguments));

			FunctionData ref function = new FunctionData(syntaxInternal, ctx.scope, false, false, false, typeFunction, ctx.GetReferenceType(ctx.typeVoid), functionNameIdentifier, RefList<MatchData>(), ctx.uniqueFunctionId++);

			// Fill in argument data
			for(int i = 0; i < arguments.size(); i++)
				function.arguments.push_back(arguments[i]);

			CheckFunctionConflict(ctx, syntax, function.name.name);

			ctx.AddFunction(function);

			ctx.PushScope(function);

			function.functionScope = ctx.scope;

			RefList<ExprVariableDefinition> variables;

			CreateFunctionArgumentVariables(ctx, syntaxInternal, function, ViewOf(arguments), variables);

			ExprVariableDefinition ref contextArgumentDefinition = CreateFunctionContextArgument(ctx, syntaxInternal, function);

			function.argumentsSize = function.functionScope.dataSize;

			RefList<ExprBase> expressions;

			ExprBase ref access = new ExprVariableAccess(syntaxInternal, enumType, variables.tail.variable.variable);
			ExprBase ref cast = new ExprTypeCast(syntaxInternal, ctx.typeInt, access, ExprTypeCastCategory.EXPR_CAST_REINTERPRET);

			expressions.push_back(new ExprReturn(syntaxInternal, ctx.typeVoid, cast, nullptr, CreateFunctionUpvalueClose(ctx, syntaxInternal, function, ctx.scope)));

			ClosePendingUpvalues(ctx, function);

			ctx.PopScope(ScopeType.SCOPE_FUNCTION);

			function.declaration = new ExprFunctionDefinition(syntaxInternal, function.type, function, contextArgumentDefinition, variables, nullptr, expressions, nullptr, nullptr);

			ctx.definitions.push_back(function.declaration);

			castToInt = function.declaration;
		}

		// Create conversion operator enum_type enum_type(int)
		{
			vector<ArgumentData> arguments;
			arguments.push_back(ArgumentData(syntaxInternal, false, new SynIdentifier(InplaceStr("$x")), ctx.typeInt, nullptr));

			SynIdentifier ref functionNameIdentifier = new SynIdentifier(typeName);

			TypeFunction ref typeFunction = ctx.GetFunctionType(syntaxInternal, enumType, ViewOf(arguments));

			FunctionData ref function = new FunctionData(syntaxInternal, ctx.scope, false, false, false, typeFunction, ctx.GetReferenceType(ctx.typeVoid), functionNameIdentifier, RefList<MatchData>(), ctx.uniqueFunctionId++);

			// Fill in argument data
			for(int i = 0; i < arguments.size(); i++)
				function.arguments.push_back(arguments[i]);

			CheckFunctionConflict(ctx, syntax, function.name.name);

			ctx.AddFunction(function);

			ctx.PushScope(function);

			function.functionScope = ctx.scope;

			RefList<ExprVariableDefinition> variables;

			CreateFunctionArgumentVariables(ctx, syntaxInternal, function, ViewOf(arguments), variables);

			ExprVariableDefinition ref contextArgumentDefinition = CreateFunctionContextArgument(ctx, syntaxInternal, function);

			function.argumentsSize = function.functionScope.dataSize;

			RefList<ExprBase> expressions;

			ExprBase ref access = new ExprVariableAccess(syntaxInternal, ctx.typeInt, variables.tail.variable.variable);
			ExprBase ref cast = new ExprTypeCast(syntaxInternal, enumType, access, ExprTypeCastCategory.EXPR_CAST_REINTERPRET);

			expressions.push_back(new ExprReturn(syntaxInternal, ctx.typeVoid, cast, nullptr, CreateFunctionUpvalueClose(ctx, syntaxInternal, function, ctx.scope)));

			ClosePendingUpvalues(ctx, function);

			ctx.PopScope(ScopeType.SCOPE_FUNCTION);

			function.declaration = new ExprFunctionDefinition(syntaxInternal, function.type, function, contextArgumentDefinition, variables, nullptr, expressions, nullptr, nullptr);

			ctx.definitions.push_back(function.declaration);

			castToEnum = function.declaration;
		}
	});

	if(tryResult)
	{
		// Restore old scope
		ctx.SwitchToScopeAtPoint(scope, nullptr);

		ctx.errorHandlerNested = prevErrorHandlerNested;
	}
	else
	{
		//NULLC::TraceLeaveTo(traceDepth);

		// Restore old scope
		ctx.SwitchToScopeAtPoint(scope, nullptr);

		ctx.errorHandlerNested = prevErrorHandlerNested;

		tryResult.rethrow();
	}

	return new ExprEnumDefinition(syntax, ctx.typeVoid, enumType, castToInt, castToEnum);
}

ExprBlock ref AnalyzeNamespaceDefinition(ExpressionContext ref ctx, SynNamespaceDefinition ref syntax)
{
	if(ctx.scope != ctx.globalScope && ctx.scope.ownerNamespace == nullptr)
		Stop(ctx, syntax, "ERROR: a namespace definition must appear either at file scope or immediately within another namespace definition");

	for(SynIdentifier ref name = syntax.path.head; name; name = getType with<SynIdentifier>(name.next))
	{
		NamespaceData ref parent = ctx.GetCurrentNamespace(ctx.scope);

		NamespaceData ref ns = new NamespaceData(syntax, ctx.scope, parent, *name, ctx.uniqueNamespaceId++);

		CheckNamespaceConflict(ctx, syntax, ns);

		if(parent)
			parent.children.push_back(ns);
		else
			ctx.globalNamespaces.push_back(ns);

		ctx.namespaces.push_back(ns);

		ctx.PushScope(ns);
	}

	RefList<ExprBase> expressions;

	for(SynBase ref expression = syntax.expressions.head; expression; expression = expression.next)
		expressions.push_back(AnalyzeStatement(ctx, expression));

	ExprBlock ref block = new ExprBlock(syntax, ctx.typeVoid, expressions, nullptr);

	for(SynIdentifier ref name = syntax.path.head; name; name = getType with<SynIdentifier>(name.next))
		ctx.PopScope(ScopeType.SCOPE_NAMESPACE);

	return block;
}

ExprAliasDefinition ref AnalyzeTypedef(ExpressionContext ref ctx, SynTypedef ref syntax)
{
	if(ctx.typeMap.find(syntax.alias.name.hash()))
		Stop(ctx, syntax, "ERROR: there is already a type or an alias with the same name");

	TypeBase ref type = AnalyzeType(ctx, syntax.type);

	if(type == ctx.typeAuto)
		Stop(ctx, syntax, "ERROR: can't alias 'auto' type");

	AliasData ref alias = new AliasData(syntax, ctx.scope, type, syntax.alias, ctx.uniqueAliasId++);

	ctx.AddAlias(alias);

	return new ExprAliasDefinition(syntax, ctx.typeVoid, alias);
}

ExprBase ref AnalyzeIfElse(ExpressionContext ref ctx, SynIfElse ref syntax)
{
	SynVariableDefinitions ref definitions = getType with<SynVariableDefinitions>(syntax.condition);

	ExprBase ref condition = nullptr;

	if(definitions)
	{
		ctx.PushScope(ScopeType.SCOPE_EXPLICIT);

		assert(definitions.definitions.size() == 1);

		TypeBase ref type = AnalyzeType(ctx, definitions.type);

		ExprBase ref definition = AnalyzeVariableDefinition(ctx, definitions.definitions.head, 0, type);

		if(ExprVariableDefinition ref variableDefinition = getType with<ExprVariableDefinition>(definition))
			condition = CreateSequence(ctx, syntax, definition, CreateVariableAccess(ctx, syntax, variableDefinition.variable.variable, false));
		else
			condition = definition;
	}
	else
	{
		condition = AnalyzeExpression(ctx, syntax.condition);
	}

	if(syntax.staticIf)
	{
		if(isType with<TypeError>(condition.type))
			return new ExprVoid(syntax, ctx.typeVoid);

		condition = CreateConditionCast(ctx, condition.source, condition);

		if(ExprBoolLiteral ref number = getType with<ExprBoolLiteral>(EvaluateExpression(ctx, syntax, CreateCast(ctx, syntax, condition, ctx.typeBool, false))))
		{
			if(number.value)
			{
				if(SynBlock ref node = getType with<SynBlock>(syntax.trueBlock))
					return AnalyzeBlock(ctx, node, false);
				else
					return AnalyzeStatement(ctx, syntax.trueBlock);
			}
			else if(syntax.falseBlock)
			{
				if(SynBlock ref node = getType with<SynBlock>(syntax.falseBlock))
					return AnalyzeBlock(ctx, node, false);
				else
					return AnalyzeStatement(ctx, syntax.falseBlock);
			}

			return new ExprVoid(syntax, ctx.typeVoid);
		}

		Report(ctx, syntax, "ERROR: couldn't evaluate condition at compilation time");
	}

	condition = CreateConditionCast(ctx, condition.source, condition);

	ExprBase ref trueBlock = AnalyzeStatement(ctx, syntax.trueBlock);

	if(definitions)
		ctx.PopScope(ScopeType.SCOPE_EXPLICIT);

	ExprBase ref falseBlock = syntax.falseBlock ? AnalyzeStatement(ctx, syntax.falseBlock) : nullptr;

	return new ExprIfElse(syntax, ctx.typeVoid, condition, trueBlock, falseBlock);
}

ExprFor ref AnalyzeFor(ExpressionContext ref ctx, SynFor ref syntax)
{
	ctx.PushLoopScope(false, false);

	ExprBase ref initializer = nullptr;

	if(SynBlock ref block = getType with<SynBlock>(syntax.initializer))
		initializer = AnalyzeBlock(ctx, block, false);
	else if(syntax.initializer)
		initializer = AnalyzeStatement(ctx, syntax.initializer);
	else
		initializer = new ExprVoid(syntax, ctx.typeVoid);

	ExprBase ref condition = AnalyzeExpression(ctx, syntax.condition);

	condition = CreateConditionCast(ctx, condition.source, condition);

	ctx.scope.breakDepth++;
	ctx.scope.contiueDepth++;

	ExprBase ref increment = syntax.increment ? AnalyzeStatement(ctx, syntax.increment) : new ExprVoid(syntax, ctx.typeVoid);
	ExprBase ref body = syntax.body ? AnalyzeStatement(ctx, syntax.body) : new ExprVoid(syntax, ctx.typeVoid);

	RefList<ExprBase> iteration;

	if(ExprBase ref closures = CreateBlockUpvalueClose(ctx, ctx.MakeInternal(syntax), ctx.GetCurrentFunction(ctx.scope), ctx.scope))
		iteration.push_back(closures);

	iteration.push_back(increment);

	ExprBlock ref block = new ExprBlock(syntax, ctx.typeVoid, iteration, nullptr);

	ctx.PopScope(ScopeType.SCOPE_LOOP);

	return new ExprFor(syntax, ctx.typeVoid, initializer, condition, block, body);
}

ExprFor ref AnalyzeForEach(ExpressionContext ref ctx, SynForEach ref syntax)
{
	ctx.PushLoopScope(true, true);

	RefList<ExprBase> initializers;
	RefList<ExprBase> conditions;
	RefList<ExprBase> definitions;
	RefList<ExprBase> increments;

	vector<VariableData ref> iterators;

	for(SynForEachIterator ref curr = syntax.iterators.head; curr; curr = getType with<SynForEachIterator>(curr.next))
	{
		SynBase ref sourceInternal = ctx.MakeInternal(curr);

		ExprBase ref value = AnalyzeExpression(ctx, curr.value);

		if(isType with<TypeError>(value.type))
		{
			initializers.push_back(value);
			continue;
		}

		TypeBase ref type = nullptr;

		if(curr.type)
			type = AnalyzeType(ctx, curr.type);

		if(isType with<TypeError>(type))
		{
			initializers.push_back(value);
			continue;
		}

		// Special implementation of for each for built-in arrays
		if(isType with<TypeArray>(value.type) || isType with<TypeUnsizedArray>(value.type))
		{
			if(!type || type == ctx.typeAuto)
			{
				if(TypeArray ref valueType = getType with<TypeArray>(value.type))
					type = valueType.subType;
				else if(TypeUnsizedArray ref valueType = getType with<TypeUnsizedArray>(value.type))
					type = valueType.subType;
			}

			ExprBase ref wrapped = value;

			if(ExprVariableAccess ref node = getType with<ExprVariableAccess>(value))
			{
				wrapped = new ExprGetAddress(value.source, ctx.GetReferenceType(value.type), new VariableHandle(node.source, node.variable));
			}
			else if(ExprDereference ref node = getType with<ExprDereference>(value))
			{
				wrapped = node.value;
			}
			else if(!isType with<TypeRef>(wrapped.type))
			{
				VariableData ref storage = AllocateTemporary(ctx, value.source, wrapped.type);

				ExprBase ref assignment = CreateAssignment(ctx, value.source, CreateVariableAccess(ctx, value.source, storage, false), value);

				ExprBase ref definition = new ExprVariableDefinition(value.source, ctx.typeVoid, new VariableHandle(nullptr, storage), assignment);

				wrapped = CreateSequence(ctx, value.source, definition, CreateGetAddress(ctx, value.source, CreateVariableAccess(ctx, value.source, storage, false)));
			}

			// Create initializer
			VariableData ref iterator = AllocateTemporary(ctx, curr, ctx.typeInt);

			ExprBase ref iteratorAssignment = CreateAssignment(ctx, curr, CreateVariableAccess(ctx, curr, iterator, false), new ExprIntegerLiteral(curr, ctx.typeInt, 0));

			initializers.push_back(new ExprVariableDefinition(curr, ctx.typeVoid, new VariableHandle(nullptr, iterator), iteratorAssignment));

			// Create condition
			conditions.push_back(CreateBinaryOp(ctx, curr, SynBinaryOpType.SYN_BINARY_OP_LESS, CreateVariableAccess(ctx, curr, iterator, false), CreateMemberAccess(ctx, curr.value, value, new SynIdentifier(InplaceStr("size")), false)));

			// Create definition
			type = ctx.GetReferenceType(type);

			CheckVariableConflict(ctx, curr, curr.name.name);

			int variableOffset = AllocateVariableInScope(ctx, curr, type.alignment, type);
			VariableData ref variable = new VariableData(curr, ctx.scope, type.alignment, type, curr.name, variableOffset, ctx.uniqueVariableId++);

			variable.isReference = true;

			if (IsLookupOnlyVariable(ctx, variable))
				variable.lookupOnly = true;

			iterators.push_back(variable);

			vector<ArgumentData> arguments;
			arguments.push_back(ArgumentData(curr, false, nullptr, ctx.typeInt, CreateVariableAccess(ctx, curr, iterator, false)));

			ExprBase ref arrayIndex = CreateArrayIndex(ctx, curr, value, ViewOf(arguments));

			assert(isType with<ExprDereference>(arrayIndex));

			if(ExprDereference ref node = getType with<ExprDereference>(arrayIndex))
				arrayIndex = node.value;

			definitions.push_back(new ExprVariableDefinition(curr, ctx.typeVoid, new VariableHandle(nullptr, variable), CreateAssignment(ctx, curr, CreateVariableAccess(ctx, curr, variable, false), arrayIndex)));

			// Create increment
			increments.push_back(new ExprPreModify(curr, ctx.typeInt, CreateGetAddress(ctx, curr, CreateVariableAccess(ctx, curr, iterator, false)), true));
			continue;
		}

		if(!AssertValueExpression(ctx, curr, value))
		{
			initializers.push_back(value);
			continue;
		}

		TypeFunction ref functionType = getType with<TypeFunction>(value.type);
		ExprBase ref startCall = nullptr;
		
		// If we don't have a function, get an iterator
		if(!functionType)
		{
			startCall = CreateFunctionCall(ctx, sourceInternal, CreateMemberAccess(ctx, curr.value, value, new SynIdentifier(InplaceStr("start")), false), RefList<TypeHandle>(), nullptr, false);

			if(isType with<TypeError>(startCall.type))
			{
				initializers.push_back(value);
				continue;
			}

			// Check if iteartor is a coroutine
			functionType = getType with<TypeFunction>(startCall.type);

			if(functionType)
				value = startCall;
		}

		if(functionType)
		{
			// Store function pointer in a variable
			VariableData ref functPtr = AllocateTemporary(ctx, sourceInternal, value.type);

			ExprBase ref funcPtrInitializer = CreateAssignment(ctx, sourceInternal, CreateVariableAccess(ctx, sourceInternal, functPtr, false), value);

			initializers.push_back(new ExprVariableDefinition(sourceInternal, ctx.typeVoid, new VariableHandle(nullptr, functPtr), funcPtrInitializer));

			if(ExprFunctionAccess ref access = getType with<ExprFunctionAccess>(value))
			{
				if(!access.function.isCoroutine)
					Stop(ctx, curr, "ERROR: function is not a coroutine");
			}
			else
			{
				initializers.push_back(CreateFunctionCall1(ctx, sourceInternal, InplaceStr("__assertCoroutine"), CreateVariableAccess(ctx, sourceInternal, functPtr, false), false, true, true));
			}

			// Create definition
			if(!type || type == ctx.typeAuto)
			{
				if(functionType.returnType == ctx.typeAuto)
				{
					Report(ctx, curr, "ERROR: function type is unresolved at this point");

					continue;
				}

				type = functionType.returnType;
			}

			CheckVariableConflict(ctx, curr, curr.name.name);

			int variableOffset = AllocateVariableInScope(ctx, curr, type.alignment, type);
			VariableData ref variable = new VariableData(curr, ctx.scope, type.alignment, type, curr.name, variableOffset, ctx.uniqueVariableId++);

			if (IsLookupOnlyVariable(ctx, variable))
				variable.lookupOnly = true;

			iterators.push_back(variable);

			if(ExprBase ref call = CreateFunctionCall(ctx, curr, CreateVariableAccess(ctx, sourceInternal, functPtr, false), RefList<TypeHandle>(), nullptr, false))
			{
				if(ctx.GetReferenceType(type) == call.type)
					call = new ExprDereference(curr, type, call);

				initializers.push_back(new ExprVariableDefinition(curr, ctx.typeVoid, new VariableHandle(nullptr, variable), CreateAssignment(ctx, curr, CreateVariableAccess(ctx, curr, variable, false), call)));
			}

			// Create condition
			conditions.push_back(new ExprUnaryOp(curr, ctx.typeBool, SynUnaryOpType.SYN_UNARY_OP_LOGICAL_NOT, CreateFunctionCall1(ctx, curr, InplaceStr("isCoroutineReset"), CreateVariableAccess(ctx, sourceInternal, functPtr, false), false, false, true)));

			// Create increment
			if(ExprBase ref call = CreateFunctionCall(ctx, curr, CreateVariableAccess(ctx, sourceInternal, functPtr, false), RefList<TypeHandle>(), nullptr, false))
			{
				if(ctx.GetReferenceType(type) == call.type)
					call = new ExprDereference(curr, type, call);

				increments.push_back(CreateAssignment(ctx, curr, CreateVariableAccess(ctx, curr, variable, false), call));
			}
		}
		else
		{
			// Store iterator in a variable
			VariableData ref iterator = AllocateTemporary(ctx, sourceInternal, startCall.type);

			ExprBase ref iteratorInitializer = CreateAssignment(ctx, sourceInternal, CreateVariableAccess(ctx, sourceInternal, iterator, false), startCall);
			
			initializers.push_back(new ExprVariableDefinition(sourceInternal, ctx.typeVoid, new VariableHandle(nullptr, iterator), iteratorInitializer));

			// Create condition
			conditions.push_back(CreateFunctionCall(ctx, curr, CreateMemberAccess(ctx, sourceInternal, CreateVariableAccess(ctx, sourceInternal, iterator, false), new SynIdentifier(InplaceStr("hasnext")), false), RefList<TypeHandle>(), nullptr, false));

			// Create definition
			ExprBase ref call = CreateFunctionCall(ctx, curr, CreateMemberAccess(ctx, sourceInternal, CreateVariableAccess(ctx, sourceInternal, iterator, false), new SynIdentifier(InplaceStr("next")), false), RefList<TypeHandle>(), nullptr, false);

			if(!type || type == ctx.typeAuto)
				type = call.type;
			else
				type = ctx.GetReferenceType(type);

			if(isType with<TypeError>(type))
				continue;

			CheckVariableConflict(ctx, curr, curr.name.name);

			int variableOffset = AllocateVariableInScope(ctx, curr, type.alignment, type);
			VariableData ref variable = new VariableData(curr, ctx.scope, type.alignment, type, curr.name, variableOffset, ctx.uniqueVariableId++);

			variable.isReference = isType with<TypeRef>(type);

			if (IsLookupOnlyVariable(ctx, variable))
				variable.lookupOnly = true;

			iterators.push_back(variable);

			definitions.push_back(new ExprVariableDefinition(curr, ctx.typeVoid, new VariableHandle(nullptr, variable), CreateAssignment(ctx, sourceInternal, CreateVariableAccess(ctx, sourceInternal, variable, false), call)));
		}
	}

	for(int i = 0; i < iterators.size(); i++)
		ctx.AddVariable(iterators[i], true);

	ExprBase ref initializer = new ExprBlock(syntax, ctx.typeVoid, initializers, nullptr);

	ExprBase ref condition = nullptr;

	for(ExprBase ref curr = conditions.head; curr; curr = curr.next)
	{
		if(!condition)
			condition = curr;
		else
			condition = CreateBinaryOp(ctx, syntax, SynBinaryOpType.SYN_BINARY_OP_LOGICAL_AND, condition, curr);
	}

	ExprBase ref increment = new ExprBlock(syntax, ctx.typeVoid, increments, nullptr);

	if(syntax.body)
		definitions.push_back(AnalyzeStatement(ctx, syntax.body));

	ExprBase ref body = new ExprBlock(syntax, ctx.typeVoid, definitions, CreateBlockUpvalueClose(ctx, ctx.MakeInternal(syntax), ctx.GetCurrentFunction(ctx.scope), ctx.scope));

	ctx.PopScope(ScopeType.SCOPE_LOOP);

	return new ExprFor(syntax, ctx.typeVoid, initializer, condition, increment, body);
}

ExprWhile ref AnalyzeWhile(ExpressionContext ref ctx, SynWhile ref syntax)
{
	ctx.PushLoopScope(true, true);

	ExprBase ref condition = AnalyzeExpression(ctx, syntax.condition);
	ExprBase ref body = syntax.body ? AnalyzeStatement(ctx, syntax.body) : new ExprVoid(syntax, ctx.typeVoid);

	condition = CreateConditionCast(ctx, condition.source, condition);

	ctx.PopScope(ScopeType.SCOPE_LOOP);

	return new ExprWhile(syntax, ctx.typeVoid, condition, body);
}

ExprDoWhile ref AnalyzeDoWhile(ExpressionContext ref ctx, SynDoWhile ref syntax)
{
	ctx.PushLoopScope(true, true);

	RefList<ExprBase> expressions;

	for(SynBase ref expression = syntax.expressions.head; expression; expression = expression.next)
		expressions.push_back(AnalyzeStatement(ctx, expression));

	ExprBase ref condition = AnalyzeExpression(ctx, syntax.condition);

	condition = CreateConditionCast(ctx, condition.source, condition);

	ExprBase ref block = new ExprBlock(syntax, ctx.typeVoid, expressions, CreateBlockUpvalueClose(ctx, ctx.MakeInternal(syntax), ctx.GetCurrentFunction(ctx.scope), ctx.scope));

	ctx.PopScope(ScopeType.SCOPE_LOOP);

	return new ExprDoWhile(syntax, ctx.typeVoid, block, condition);
}

ExprSwitch ref AnalyzeSwitch(ExpressionContext ref ctx, SynSwitch ref syntax)
{
	ctx.PushLoopScope(true, false);

	ExprBase ref condition = AnalyzeExpression(ctx, syntax.condition);

	if(condition.type == ctx.typeVoid)
		Report(ctx, syntax.condition, "ERROR: condition type cannot be '%.*s'", FMT_ISTR(condition.type.name));

	VariableData ref conditionVariable = nullptr;
	
	if(!isType with<TypeError>(condition.type))
	{
		if(!AssertValueExpression(ctx, syntax.condition, condition))
			conditionVariable = AllocateTemporary(ctx, ctx.MakeInternal(syntax), ctx.GetErrorType());
		else
			conditionVariable = AllocateTemporary(ctx, ctx.MakeInternal(syntax), condition.type);

		ExprBase ref access = CreateVariableAccess(ctx, ctx.MakeInternal(syntax), conditionVariable, false);

		ExprBase ref initializer = CreateAssignment(ctx, syntax.condition, access, condition);

		condition = new ExprVariableDefinition(syntax.condition, ctx.typeVoid, new VariableHandle(nullptr, conditionVariable), initializer);
	}

	RefList<ExprBase> cases;
	RefList<ExprBase> blocks;
	ExprBase ref defaultBlock = nullptr;

	for(SynSwitchCase ref curr = syntax.cases.head; curr; curr = getType with<SynSwitchCase>(curr.next))
	{
		if(curr.value)
		{
			ExprBase ref caseValue = AnalyzeExpression(ctx, curr.value);

			if(isType with<TypeError>(caseValue.type) || isType with<TypeError>(condition.type))
			{
				cases.push_back(caseValue);
			}
			else
			{
				if(caseValue.type == ctx.typeVoid)
					Report(ctx, syntax.condition, "ERROR: case value type cannot be '%.*s'", FMT_ISTR(caseValue.type.name));

				ExprBase ref conditionValue = CreateBinaryOp(ctx, curr.value, SynBinaryOpType.SYN_BINARY_OP_EQUAL, caseValue, CreateVariableAccess(ctx, ctx.MakeInternal(syntax), conditionVariable, false));

				if(!ctx.IsIntegerType(conditionValue.type) || conditionValue.type == ctx.typeLong)
					Report(ctx, curr, "ERROR: '==' operator result type must be bool, char, short or int");

				cases.push_back(conditionValue);
			}
		}

		RefList<ExprBase> expressions;

		for(SynBase ref expression = curr.expressions.head; expression; expression = expression.next)
			expressions.push_back(AnalyzeStatement(ctx, expression));

		ExprBase ref block = new ExprBlock(ctx.MakeInternal(syntax), ctx.typeVoid, expressions, nullptr);

		if(curr.value)
			blocks.push_back(block);
		else
			defaultBlock = block;
	}

	ctx.PopScope(ScopeType.SCOPE_LOOP);

	return new ExprSwitch(syntax, ctx.typeVoid, condition, cases, blocks, defaultBlock);
}

ExprBreak ref AnalyzeBreak(ExpressionContext ref ctx, SynBreak ref syntax)
{
	int depth = 1;

	if(syntax.number)
	{
		ExprBase ref numberValue = AnalyzeExpression(ctx, syntax.number);

		if(ExprIntegerLiteral ref number = getType with<ExprIntegerLiteral>(EvaluateExpression(ctx, syntax.number, CreateCast(ctx, syntax.number, numberValue, ctx.typeLong, false))))
		{
			if(number.value <= 0)
				Stop(ctx, syntax.number, "ERROR: break level can't be negative or zero");

			depth = int(number.value);
		}
		else
		{
			Stop(ctx, syntax.number, "ERROR: break statement must be followed by ';' or a constant");
		}
	}

	if(ctx.scope.breakDepth < depth)
		Stop(ctx, syntax, "ERROR: break level is greater that loop depth");

	return new ExprBreak(syntax, ctx.typeVoid, depth, CreateBreakUpvalueClose(ctx, syntax, ctx.GetCurrentFunction(ctx.scope), ctx.scope, depth));
}

ExprContinue ref AnalyzeContinue(ExpressionContext ref ctx, SynContinue ref syntax)
{
	int depth = 1;

	if(syntax.number)
	{
		ExprBase ref numberValue = AnalyzeExpression(ctx, syntax.number);

		if(ExprIntegerLiteral ref number = getType with<ExprIntegerLiteral>(EvaluateExpression(ctx, syntax.number, CreateCast(ctx, syntax.number, numberValue, ctx.typeLong, false))))
		{
			if(number.value <= 0)
				Stop(ctx, syntax.number, "ERROR: continue level can't be negative or zero");

			depth = int(number.value);
		}
		else
		{
			Stop(ctx, syntax.number, "ERROR: continue statement must be followed by ';' or a constant");
		}
	}

	if(ctx.scope.contiueDepth < depth)
		Stop(ctx, syntax, "ERROR: continue level is greater that loop depth");

	return new ExprContinue(syntax, ctx.typeVoid, depth, CreateContinueUpvalueClose(ctx, syntax, ctx.GetCurrentFunction(ctx.scope), ctx.scope, depth));
}

ExprBlock ref AnalyzeBlock(ExpressionContext ref ctx, SynBlock ref syntax, bool createScope)
{
	if(createScope)
	{
		ctx.PushScope(ScopeType.SCOPE_EXPLICIT);

		RefList<ExprBase> expressions;

		for(SynBase ref expression = syntax.expressions.head; expression; expression = expression.next)
			expressions.push_back(AnalyzeStatement(ctx, expression));

		ExprBlock ref block = new ExprBlock(syntax, ctx.typeVoid, expressions, CreateBlockUpvalueClose(ctx, ctx.MakeInternal(syntax), ctx.GetCurrentFunction(ctx.scope), ctx.scope));

		ctx.PopScope(ScopeType.SCOPE_EXPLICIT);

		return block;
	}

	RefList<ExprBase> expressions;

	for(SynBase ref expression = syntax.expressions.head; expression; expression = expression.next)
		expressions.push_back(AnalyzeStatement(ctx, expression));

	return new ExprBlock(syntax, ctx.typeVoid, expressions, nullptr);
}

ExprBase ref AnalyzeBool(ExpressionContext ref ctx, SynBool ref syntax)
{
	return new ExprBoolLiteral(syntax, ctx.typeBool, syntax.value);
}

ExprBase ref AnalyzeCharacter(ExpressionContext ref ctx, SynCharacter ref syntax)
{
	char result = syntax.value[1];

	if(result == '\\')
		result = ParseEscapeSequence(ctx, syntax, syntax.value.begin_ref + 1);

	return new ExprCharacterLiteral(syntax, ctx.typeChar, result);
}

ExprBase ref AnalyzeString(ExpressionContext ref ctx, SynString ref syntax)
{
	int length = 0;

	if(syntax.rawLiteral)
	{
		if(syntax.value.length() >= 2)
			length = syntax.value.length() - 2;
	}
	else
	{
		// Find the length of the string with collapsed escape-sequences
		for(StringRef curr = syntax.value.begin_ref + 1, end = syntax.value.end_ref - 1; curr < end; { curr.advance(); length++; })
		{
			if(curr.curr() == '\\')
				curr.advance();
		}
	}

	int mem = length ? ((length + 1) + 3) & ~3 : 4;

	char[] value = new char[mem];

	for(int i = length; i < mem; i++)
		value[i] = 0;

	if(syntax.rawLiteral)
	{
		for(int i = 0; i < length; i++)
			value[i] = syntax.value[i + 1];

		value[length] = 0;
	}
	else
	{
		int i = 0;

		// Find the length of the string with collapsed escape-sequences
		for(StringRef curr = syntax.value.begin_ref + 1, end = syntax.value.end_ref - 1; curr < end;)
		{
			if(curr.curr() == '\\')
			{
				value[i++] = ParseEscapeSequence(ctx, syntax, curr);
				curr += 2;
			}
			else
			{
				value[i++] = curr.curr();
				curr += 1;
			}
		}

		value[length] = 0;
	}

	return new ExprStringLiteral(syntax, ctx.GetArrayType(ctx.typeChar, length + 1), value, length);
}

ExprBase ref AnalyzeNullptr(ExpressionContext ref ctx, SynNullptr ref syntax)
{
	return new ExprNullptrLiteral((SynNullptr ref)(syntax), ctx.typeNullPtr);
}

ExprBase ref AnalyzeTypeof(ExpressionContext ref ctx, SynTypeof ref syntax)
{
	ExprBase ref value = AnalyzeExpression(ctx, syntax.value);

	if(value.type == ctx.typeAuto)
		Stop(ctx, syntax, "ERROR: cannot take typeid from auto type");

	if(isType with<ExprTypeLiteral>(value))
		return value;

	ResolveInitializerValue(ctx, syntax, value);

	assert(!isType with<TypeArgumentSet>(value.type) && !isType with<TypeMemberSet>(value.type) && !isType with<TypeFunctionSet>(value.type));

	return new ExprTypeLiteral(syntax, ctx.typeTypeID, value.type);
}

ExprBase ref AnalyzeTypeSimple(ExpressionContext ref ctx, SynTypeSimple ref syntax)
{
	// It could be a typeid
	if(TypeBase ref type = AnalyzeType(ctx, syntax, false))
	{
		if(type == ctx.typeAuto)
			Stop(ctx, syntax, "ERROR: cannot take typeid from auto type");

		return new ExprTypeLiteral(syntax, ctx.typeTypeID, type);
	}

	return AnalyzeVariableAccess(ctx, syntax);
}

ExprBase ref AnalyzeSizeof(ExpressionContext ref ctx, SynSizeof ref syntax)
{
	if(TypeBase ref type = AnalyzeType(ctx, syntax.value, false))
	{
		if(type.isGeneric)
			Stop(ctx, syntax, "ERROR: sizeof generic type is illegal");

		if(type == ctx.typeAuto)
			Stop(ctx, syntax, "ERROR: sizeof auto type is illegal");

		if(TypeClass ref typeClass = getType with<TypeClass>(type))
		{
			if(!typeClass.completed)
				Stop(ctx, syntax, "ERROR: type '%.*s' is not fully defined", FMT_ISTR(type.name));
		}

		assert(!isType with<TypeArgumentSet>(type) && !isType with<TypeMemberSet>(type) && !isType with<TypeFunctionSet>(type));

		return new ExprIntegerLiteral(syntax, ctx.typeInt, type.size);
	}

	ExprBase ref value = AnalyzeExpression(ctx, syntax.value);

	if(value.type == ctx.typeAuto)
		Stop(ctx, syntax, "ERROR: sizeof auto type is illegal");

	if(TypeClass ref typeClass = getType with<TypeClass>(value.type))
	{
		if(!typeClass.completed)
			Stop(ctx, syntax, "ERROR: type '%.*s' is not fully defined", FMT_ISTR(value.type.name));
	}

	ResolveInitializerValue(ctx, syntax, value);

	assert(!isType with<TypeArgumentSet>(value.type) && !isType with<TypeMemberSet>(value.type) && !isType with<TypeFunctionSet>(value.type));

	return new ExprIntegerLiteral(syntax, ctx.typeInt, value.type.size);
}

ExprBase ref AnalyzeMemberAccess(ExpressionContext ref ctx, SynMemberAccess ref syntax)
{
	// It could be a typeid
	if(TypeBase ref type = AnalyzeType(ctx, syntax, false))
	{
		if(type == ctx.typeAuto)
			Stop(ctx, syntax, "ERROR: cannot take typeid from auto type");

		return new ExprTypeLiteral(syntax, ctx.typeTypeID, type);
	}

	// It could be a type property
	if(TypeBase ref type = AnalyzeType(ctx, syntax.value, false))
	{
		if(ExprBase ref result = CreateTypeidMemberAccess(ctx, syntax, type, syntax.member))
			return result;
	}

	ExprBase ref value = AnalyzeExpression(ctx, syntax.value);

	if(isType with<TypeError>(value.type))
		return new ExprMemberAccess(syntax, ctx.GetErrorType(), value, nullptr);

	return CreateMemberAccess(ctx, syntax, value, syntax.member, false);
}

ExprBase ref AnalyzeTypeArray(ExpressionContext ref ctx, SynTypeArray ref syntax)
{
	// It could be a typeid
	if(TypeBase ref type = AnalyzeType(ctx, syntax, false))
	{
		if(type == ctx.typeAuto)
			Stop(ctx, syntax, "ERROR: cannot take typeid from auto type");

		return new ExprTypeLiteral(syntax, ctx.typeTypeID, type);
	}

	return AnalyzeArrayIndex(ctx, syntax);
}

ExprBase ref AnalyzeTypeReference(ExpressionContext ref ctx, SynTypeReference ref syntax)
{
	return new ExprTypeLiteral(syntax, ctx.typeTypeID, AnalyzeType(ctx, syntax));
}

ExprBase ref AnalyzeTypeFunction(ExpressionContext ref ctx, SynTypeFunction ref syntax)
{
	if(TypeBase ref type = AnalyzeType(ctx, syntax, false))
		return new ExprTypeLiteral(syntax, ctx.typeTypeID, type);

	// Transform 'type ref(arguments)' into a 'type ref' constructor call
	SynBase ref value = new SynTypeReference(syntax.begin, syntax.end, syntax.returnType);

	RefList<SynCallArgument> arguments;

	for(SynBase ref curr = syntax.arguments.head; curr; curr = curr.next)
		arguments.push_back(new SynCallArgument(curr.begin, curr.end, nullptr, curr));

	return AnalyzeFunctionCall(ctx, new SynFunctionCall(syntax.begin, syntax.end, value, RefList<SynBase>(), arguments));
}

ExprBase ref AnalyzeTypeGenericInstance(ExpressionContext ref ctx, SynTypeGenericInstance ref syntax)
{
	return new ExprTypeLiteral(syntax, ctx.typeTypeID, AnalyzeType(ctx, syntax));
}

ExprBase ref AnalyzeError(ExpressionContext ref ctx, SynError ref syntax)
{
	return new ExprError(syntax, ctx.GetErrorType());
}

ExprBase ref AnalyzeExpression(ExpressionContext ref ctx, SynBase ref syntax)
{
	ctx.expressionDepth++;

	if(ctx.expressionDepth > NULLC_MAX_EXPRESSION_DEPTH)
		Stop(ctx, syntax, "ERROR: reached maximum generic expression depth (%d)", NULLC_MAX_EXPRESSION_DEPTH);

	ExprBase ref result = nullptr;

	switch(typeid(syntax))
	{
	case SynBool:
		result = AnalyzeBool(ctx, (SynBool ref)(syntax));
		break;
	case SynCharacter:
		result = AnalyzeCharacter(ctx, (SynCharacter ref)(syntax));
		break;
	case SynString:
		result = AnalyzeString(ctx, (SynString ref)(syntax));
		break;
	case SynNullptr:
		result = AnalyzeNullptr(ctx, (SynNullptr ref)(syntax));
		break;
	case SynNumber:
		result = AnalyzeNumber(ctx, (SynNumber ref)(syntax));
		break;
	case SynArray:
		result = AnalyzeArray(ctx, (SynArray ref)(syntax));
		break;
	case SynPreModify:
		result = AnalyzePreModify(ctx, (SynPreModify ref)(syntax));
		break;
	case SynPostModify:
		result = AnalyzePostModify(ctx, (SynPostModify ref)(syntax));
		break;
	case SynUnaryOp:
		result = AnalyzeUnaryOp(ctx, (SynUnaryOp ref)(syntax));
		break;
	case SynBinaryOp:
		result = AnalyzeBinaryOp(ctx, (SynBinaryOp ref)(syntax));
		break;
	case SynGetAddress:
		result = AnalyzeGetAddress(ctx, (SynGetAddress ref)(syntax));
		break;
	case SynDereference:
		result = AnalyzeDereference(ctx, (SynDereference ref)(syntax));
		break;
	case SynTypeof:
		result = AnalyzeTypeof(ctx, (SynTypeof ref)(syntax));
		break;
	case SynIdentifier:
		result = AnalyzeVariableAccess(ctx, (SynIdentifier ref)(syntax));
		break;
	case SynTypeSimple:
		result = AnalyzeTypeSimple(ctx, (SynTypeSimple ref)(syntax));
		break;
	case SynSizeof:
		result = AnalyzeSizeof(ctx, (SynSizeof ref)(syntax));
		break;
	case SynConditional:
		result = AnalyzeConditional(ctx, (SynConditional ref)(syntax));
		break;
	case SynAssignment:
		result = AnalyzeAssignment(ctx, (SynAssignment ref)(syntax));
		break;
	case SynModifyAssignment:
		result = AnalyzeModifyAssignment(ctx, (SynModifyAssignment ref)(syntax));
		break;
	case SynMemberAccess:
		result = AnalyzeMemberAccess(ctx, (SynMemberAccess ref)(syntax));
		break;
	case SynTypeArray:
		result = AnalyzeTypeArray(ctx, (SynTypeArray ref)(syntax));
		break;
	case SynArrayIndex:
		result = AnalyzeArrayIndex(ctx, (SynArrayIndex ref)(syntax));
		break;
	case SynFunctionCall:
		result = AnalyzeFunctionCall(ctx, (SynFunctionCall ref)(syntax));
		break;
	case SynNew:
		result = AnalyzeNew(ctx, (SynNew ref)(syntax));
		break;
	case SynFunctionDefinition:
		result = AnalyzeFunctionDefinition(ctx, (SynFunctionDefinition ref)(syntax), nullptr, nullptr, nullptr, RefList<MatchData>(), true, true, true);
		break;
	case SynGenerator:
		result = AnalyzeGenerator(ctx, (SynGenerator ref)(syntax));
		break;
	case SynTypeReference:
		result = AnalyzeTypeReference(ctx, (SynTypeReference ref)(syntax));
		break;
	case SynTypeFunction:
		result = AnalyzeTypeFunction(ctx, (SynTypeFunction ref)(syntax));
		break;
	case SynTypeGenericInstance:
		result = AnalyzeTypeGenericInstance(ctx, (SynTypeGenericInstance ref)(syntax));
		break;
	case SynShortFunctionDefinition:
		result = ReportExpected(ctx, syntax, ctx.GetErrorType(), "ERROR: cannot infer type for inline function outside of the function call");
		break;
	case SynTypeAuto:
		result = ReportExpected(ctx, syntax, ctx.GetErrorType(), "ERROR: cannot take typeid from auto type");
		break;
	case SynTypeAlias:
		result = ReportExpected(ctx, syntax, ctx.GetErrorType(), "ERROR: cannot take typeid from generic type");
		break;
	case SynTypeGeneric:
		Stop(ctx, syntax, "ERROR: cannot take typeid from generic type");

		break;
	case SynError:
		result = AnalyzeError(ctx, (SynError ref)(syntax));
		break;
	default:
		break;
	}

	ctx.expressionDepth--;

	if(!result)
		Stop(ctx, syntax, "ERROR: unknown expression type");

	return result;
}

ExprBase ref AnalyzeStatement(ExpressionContext ref ctx, SynBase ref syntax)
{
	if(SynReturn ref node = getType with<SynReturn>(syntax))
	{
		return AnalyzeReturn(ctx, node);
	}

	if(SynYield ref node = getType with<SynYield>(syntax))
	{
		return AnalyzeYield(ctx, node);
	}

	if(SynVariableDefinitions ref node = getType with<SynVariableDefinitions>(syntax))
	{
		return AnalyzeVariableDefinitions(ctx, node);
	}

	if(SynFunctionDefinition ref node = getType with<SynFunctionDefinition>(syntax))
	{
		return AnalyzeFunctionDefinition(ctx, node, nullptr, nullptr, nullptr, RefList<MatchData>(), true, false, true);
	}

	if(SynClassDefinition ref node = getType with<SynClassDefinition>(syntax))
	{
		RefList<TypeHandle> generics;

		return AnalyzeClassDefinition(ctx, node, nullptr, generics);
	}

	if(SynClassPrototype ref node = getType with<SynClassPrototype>(syntax))
	{
		return AnalyzeClassPrototype(ctx, node);
	}

	if(SynEnumDefinition ref node = getType with<SynEnumDefinition>(syntax))
	{
		return AnalyzeEnumDefinition(ctx, node);
	}

	if(SynNamespaceDefinition ref node = getType with<SynNamespaceDefinition>(syntax))
	{
		return AnalyzeNamespaceDefinition(ctx, node);
	}

	if(SynTypedef ref node = getType with<SynTypedef>(syntax))
	{
		return AnalyzeTypedef(ctx, node);
	}

	if(SynIfElse ref node = getType with<SynIfElse>(syntax))
	{
		return AnalyzeIfElse(ctx, node);
	}

	if(SynFor ref node = getType with<SynFor>(syntax))
	{
		return AnalyzeFor(ctx, node);
	}

	if(SynForEach ref node = getType with<SynForEach>(syntax))
	{
		return AnalyzeForEach(ctx, node);
	}

	if(SynWhile ref node = getType with<SynWhile>(syntax))
	{
		return AnalyzeWhile(ctx, node);
	}

	if(SynDoWhile ref node = getType with<SynDoWhile>(syntax))
	{
		return AnalyzeDoWhile(ctx, node);
	}

	if(SynSwitch ref node = getType with<SynSwitch>(syntax))
	{
		return AnalyzeSwitch(ctx, node);
	}

	if(SynBreak ref node = getType with<SynBreak>(syntax))
	{
		return AnalyzeBreak(ctx, node);
	}

	if(SynContinue ref node = getType with<SynContinue>(syntax))
	{
		return AnalyzeContinue(ctx, node);
	}

	if(SynBlock ref node = getType with<SynBlock>(syntax))
	{
		return AnalyzeBlock(ctx, node, true);
	}

	ExprBase ref expression = AnalyzeExpression(ctx, syntax);

	if(!AssertValueExpression(ctx, syntax, expression))
		return new ExprError(syntax, ctx.GetErrorType(), expression);

	return expression;
}

class ModuleContext
{
	void ModuleContext()
	{
		data = nullptr;

		dependencyDepth = 1;
	}

	vector<TypeBase ref> types;

	ModuleData ref data;

	int dependencyDepth;
}

void ImportModuleDependencies(ExpressionContext ref ctx, SynBase ref source, ModuleContext ref moduleCtx, ByteCode ref moduleBytecode)
{
	//TRACE_SCOPE("analyze", "ImportModuleDependencies");

	char[] symbols = FindSymbols(moduleBytecode);

	ExternModuleInfo[] moduleList = FindFirstModule(moduleBytecode);

	for(int i = 0; i < moduleBytecode.dependsCount; i++)
	{
		ExternModuleInfo ref moduleInfo = &moduleList[i];

		StringRef moduleFileName = StringRef(symbols, moduleInfo.nameOffset);

		ByteCode ref bytecode = BinaryCache::FindBytecode(string(moduleFileName), false);

		Lexeme[] lexStream = BinaryCache::FindLexems(string(moduleFileName), false);

		if(!bytecode)
			Stop(ctx, source, "ERROR: module dependency import is not implemented");

/*#ifdef IMPORT_VERBOSE_DEBUG_OUTPUT
		for(int k = 0; k < moduleCtx.dependencyDepth; k++)
			printf("  ");
		printf("  importing module %s as dependency #%d\n", moduleFileName, ctx.dependencies.size() + 1);
#endif*/

		ModuleData ref moduleData = new ModuleData(source, InplaceStr(moduleFileName));

		ctx.dependencies.push_back(moduleData);
		moduleData.dependencyIndex = ctx.dependencies.size();

		moduleData.bytecode = bytecode;

		if(lexStream == nullptr)
		{
			moduleData.lexer = new Lexer;

			moduleData.lexer.Lexify(FindSource(moduleData.bytecode));
			lexStream = moduleData.lexer.lexems.data;

			BinaryCache::PutLexemes(string(moduleFileName), lexStream);
		}

		moduleData.lexStream = lexStream;

		moduleCtx.dependencyDepth++;

		ImportModuleDependencies(ctx, source, moduleCtx, moduleData.bytecode);

		moduleCtx.dependencyDepth--;
	}
}

void ImportModuleNamespaces(ExpressionContext ref ctx, SynBase ref source, ModuleContext ref moduleCtx)
{
	//TRACE_SCOPE("analyze", "ImportModuleNamespaces");

	ByteCode ref bCode = moduleCtx.data.bytecode;
	char[] symbols = FindSymbols(bCode);

	// Import namespaces
	ExternNamespaceInfo[] namespaceList = FindFirstNamespace(bCode);

	for(int i = 0; i < bCode.namespaceCount; i++)
	{
		ExternNamespaceInfo ref namespaceData = &namespaceList[i];

		NamespaceData ref parent = nullptr;

		if(namespaceData.parentHash != -1)
		{
			for(int k = 0; k < ctx.namespaces.size(); k++)
			{
				if(ctx.namespaces[k].nameHash == namespaceData.parentHash)
				{
					parent = ctx.namespaces[k];
					break;
				}
			}

			if(!parent)
				Stop(ctx, source, "ERROR: namespace %s parent not found", StringRef(symbols, namespaceData.offsetToName));
		}

		NamespaceData ref ns = new NamespaceData(source, ctx.scope, parent, SynIdentifier(InplaceStr(StringRef(symbols, namespaceData.offsetToName))), ctx.uniqueNamespaceId++);

		if(parent)
			parent.children.push_back(ns);
		else
			ctx.globalNamespaces.push_back(ns);

		ctx.namespaces.push_back(ns);
	}
}

class DelayedType
{
	void DelayedType()
	{
	}

	void DelayedType(int index, int constantPos)
	{
		this.index = index;
		this.constantPos = constantPos;
	}

	int index;
	int constantPos;
}

void ImportModuleTypes(ExpressionContext ref ctx, SynBase ref source, ModuleContext ref moduleCtx)
{
	//TRACE_SCOPE("analyze", "ImportModuleTypes");

	ByteCode ref bCode = moduleCtx.data.bytecode;
	char[] symbols = FindSymbols(bCode);

	// Import types
	ExternTypeInfo[] typeList = FindFirstType(bCode);
	ExternMemberInfo[] memberList = FindFirstMember(bCode);
	ExternConstantInfo[] constantList = FindFirstConstant(bCode);
	ExternTypedefInfo[] aliasList = FindFirstTypedef(bCode);

	int prevSize = moduleCtx.types.size();

	moduleCtx.types.resize(bCode.typeCount);

	for(int i = prevSize; i < moduleCtx.types.size(); i++)
		moduleCtx.types[i] = nullptr;

	vector<DelayedType> delayedTypes;

	int currentConstant = 0;

	for(int i = 0; i < bCode.typeCount; i++)
	{
		ExternTypeInfo ref type = &typeList[i];

		ModuleData ref importModule = moduleCtx.data;

		if(type.definitionModule != 0)
			importModule = ctx.dependencies[moduleCtx.data.startingDependencyIndex + type.definitionModule - 1];

		InplaceStr typeName = InplaceStr(StringRef(symbols, type.offsetToName));

		TypeClass ref forwardDeclaration = nullptr;

		// Skip existing types
		if(TypeBase ref ref prev = ctx.typeMap.find(type.nameHash))
		{
			TypeBase ref prevType = *prev;

			TypeClass ref prevTypeClass = getType with<TypeClass>(prevType);

			if(type.definitionModule == 0 && prevType.importModule && moduleCtx.data.bytecode != prevType.importModule.bytecode)
			{
				bool duplicate = isType with<TypeGenericClassProto>(prevType);

				if(prevTypeClass && prevTypeClass.generics.empty())
					duplicate = true;

				if(duplicate)
					Stop(ctx, source, "ERROR: type '%.*s' in module '%.*s' is already defined in module '%.*s'", FMT_ISTR(typeName), FMT_ISTR(moduleCtx.data.name), FMT_ISTR(prevType.importModule.name));
			}

			moduleCtx.types[i] = prevType;

			if(prevTypeClass && !prevTypeClass.completed && (type.typeFlags & int(TypeFlags.TYPE_IS_COMPLETED)) != 0)
			{
				forwardDeclaration = prevTypeClass;
			}
			else
			{
				currentConstant += type.constantCount;
				continue;
			}
		}

		switch(type.subCat)
		{
		case SubCategory.CAT_NONE:
			if(string(symbols, type.offsetToName) == "generic")
			{
				// TODO: explicit category
				moduleCtx.types[i] = ctx.typeGeneric;

				moduleCtx.types[i].importModule = importModule;

				assert(moduleCtx.types[i].name == typeName);
			}
			else if(symbols[type.offsetToName] == '@')
			{
				// TODO: explicit category
				moduleCtx.types[i] = ctx.GetGenericAliasType(new SynIdentifier(InplaceStr(StringRef(symbols, type.offsetToName + 1))));

				moduleCtx.types[i].importModule = importModule;

				assert(moduleCtx.types[i].name == typeName);
			}
			else
			{
				Stop(ctx, source, "ERROR: new type in module %.*s named %s unsupported", FMT_ISTR(moduleCtx.data.name), StringRef(symbols, type.offsetToName));
			}
			break;
		case SubCategory.CAT_ARRAY:
			if(TypeBase ref subType = moduleCtx.types[type.subTypeOrMemberOffset])
			{
				if(type.arrSizeOrMemberCount == -1)
					moduleCtx.types[i] = ctx.GetUnsizedArrayType(subType);
				else
					moduleCtx.types[i] = ctx.GetArrayType(subType, type.arrSizeOrMemberCount);

				moduleCtx.types[i].importModule = importModule;

				assert(moduleCtx.types[i].name == typeName);
			}
			else
			{
				Stop(ctx, source, "ERROR: can't find sub type for '%s' in module %.*s", StringRef(symbols, type.offsetToName), FMT_ISTR(moduleCtx.data.name));
			}
			break;
		case SubCategory.CAT_POINTER:
			if(TypeBase ref subType = moduleCtx.types[type.subTypeOrMemberOffset])
			{
				moduleCtx.types[i] = ctx.GetReferenceType(subType);

				moduleCtx.types[i].importModule = importModule;

				assert(moduleCtx.types[i].name == typeName);
			}
			else
			{
				Stop(ctx, source, "ERROR: can't find sub type for '%s' in module %.*s", StringRef(symbols, type.offsetToName), FMT_ISTR(moduleCtx.data.name));
			}
			break;
		case SubCategory.CAT_FUNCTION:
			if(TypeBase ref returnType = moduleCtx.types[memberList[type.subTypeOrMemberOffset].type])
			{
				RefList<TypeHandle> arguments;

				for(int n = 0; n < type.arrSizeOrMemberCount; n++)
				{
					TypeBase ref argType = moduleCtx.types[memberList[type.subTypeOrMemberOffset + n + 1].type];

					if(!argType)
						Stop(ctx, source, "ERROR: can't find argument %d type for '%s' in module %.*s", n + 1, StringRef(symbols, type.offsetToName), FMT_ISTR(moduleCtx.data.name));

					arguments.push_back(new TypeHandle(argType));
				}

				moduleCtx.types[i] = ctx.GetFunctionType(source, returnType, arguments);

				moduleCtx.types[i].importModule = importModule;

				assert(moduleCtx.types[i].name == typeName);
			}
			else
			{
				Stop(ctx, source, "ERROR: can't find return type for '%s' in module %.*s", StringRef(symbols, type.offsetToName), FMT_ISTR(moduleCtx.data.name));
			}
			break;
		case SubCategory.CAT_CLASS:
			{
				TypeBase ref importedType = nullptr;

				NamespaceData ref parentNamespace = nullptr;

				for(int k = 0; k < ctx.namespaces.size(); k++)
				{
					if(ctx.namespaces[k].fullNameHash == type.namespaceHash)
					{
						parentNamespace = ctx.namespaces[k];
						break;
					}
				}

				if(parentNamespace)
					ctx.PushScope(parentNamespace);

				// Find all generics for this type
				bool isGeneric = false;

				RefList<TypeHandle> generics;
				RefList<MatchData> actualGenerics;

				for(int k = 0; k < bCode.typedefCount; k++)
				{
					ExternTypedefInfo ref alias = &aliasList[k];

					if(alias.parentType == i && generics.size() < type.genericTypeCount)
					{
						InplaceStr aliasName = InplaceStr(StringRef(symbols, alias.offsetToName));

						SynIdentifier ref aliasNameIdentifier = new SynIdentifier(aliasName);

						TypeBase ref targetType = moduleCtx.types[alias.targetType];

						if(!targetType)
							Stop(ctx, source, "ERROR: can't find type '%.*s' alias '%s' target type in module %.*s", FMT_ISTR(typeName), StringRef(symbols, alias.offsetToName), FMT_ISTR(moduleCtx.data.name));

						isGeneric |= targetType.isGeneric;

						generics.push_back(new TypeHandle(targetType));
						actualGenerics.push_back(new MatchData(aliasNameIdentifier, targetType));
					}
				}

				TypeClass ref baseType = nullptr;

				if(type.baseType)
				{
					baseType = getType with<TypeClass>(moduleCtx.types[type.baseType]);

					if(!baseType)
						Stop(ctx, source, "ERROR: can't find type '%.*s' base type in module %.*s", FMT_ISTR(typeName), FMT_ISTR(moduleCtx.data.name));
				}

				assert(type.definitionLocationStart < importModule.lexStream.size);
				assert(type.definitionLocationEnd < importModule.lexStream.size);
				
				LexemeRef locationSourceStart = LexemeRef(importModule.lexer, type.definitionLocationStart);
				LexemeRef locationSourceEnd = LexemeRef(importModule.lexer, type.definitionLocationEnd);

				SynBase ref locationSource = type.definitionLocationStart != 0 || type.definitionLocationEnd != 0 ? new SynImportLocation(locationSourceStart, locationSourceEnd) : source;

				assert(type.definitionLocationName < importModule.lexStream.size);

				LexemeRef locationName = LexemeRef(importModule.lexer, type.definitionLocationName);

				SynIdentifier identifier = type.definitionLocationName != 0 ? SynIdentifier(locationName, locationName, typeName) : SynIdentifier(typeName);

				if(type.definitionOffset != -1 && type.definitionOffset & 0x80000000)
				{
					assert(!forwardDeclaration);

					TypeBase ref proto = moduleCtx.types[type.definitionOffset & ~0x80000000];

					if(!proto)
						Stop(ctx, source, "ERROR: can't find proto type for '%s' in module %.*s", StringRef(symbols, type.offsetToName), FMT_ISTR(moduleCtx.data.name));

					TypeGenericClassProto ref protoClass = getType with<TypeGenericClassProto>(proto);

					if(!protoClass)
						Stop(ctx, source, "ERROR: can't find correct proto type for '%s' in module %.*s", StringRef(symbols, type.offsetToName), FMT_ISTR(moduleCtx.data.name));

					if(isGeneric)
					{
						importedType = ctx.GetGenericClassType(source, protoClass, generics);

						// TODO: assert that alias list is empty and that correct number of generics was exported
					}
					else
					{
						TypeClass ref classType = new TypeClass(identifier, locationSource, ctx.scope, protoClass, actualGenerics, (type.typeFlags & int(TypeFlags.TYPE_IS_EXTENDABLE)) != 0, baseType);

						if(type.typeFlags & int(TypeFlags.TYPE_IS_COMPLETED))
							classType.completed = true;

						if(type.typeFlags & int(TypeFlags.TYPE_IS_INTERNAL))
							classType.isInternal = true;

						importedType = classType;

						ctx.AddType(importedType);

						assert(type.genericTypeCount == generics.size());

						if(!generics.empty())
							ctx.genericTypeMap.insert(typeName.hash(), classType);
					}
				}
				else if(type.definitionOffsetStart != -1)
				{
					assert(!forwardDeclaration);

					assert(type.definitionOffsetStart < importModule.lexStream.size);

					LexemeRef start = LexemeRef(importModule.lexer, type.definitionOffsetStart);

					ParseContext ref parser = new ParseContext(ctx.optimizationLevel, ArrayView<InplaceStr>());

					parser.currentLexeme = start;

					SynClassDefinition ref definition = getType with<SynClassDefinition>(ParseClassDefinition(*parser));

					if(!definition)
						Stop(ctx, source, "ERROR: failed to import generic class body");

					definition.imported = true;

					importedType = new TypeGenericClassProto(identifier, locationSource, ctx.scope, definition);

					ctx.AddType(importedType);

					// TODO: check that type doesn't have generics or aliases
				}
				else if(type.type != TypeCategory.TYPE_COMPLEX)
				{
					assert(!forwardDeclaration);

					TypeEnum ref enumType = new TypeEnum(identifier, locationSource, ctx.scope);

					importedType = enumType;

					ctx.AddType(importedType);

					assert(generics.empty());
				}
				else
				{
					RefList<MatchData> actualGenerics;

					TypeClass ref classType = nullptr;

					if(forwardDeclaration)
					{
						classType = forwardDeclaration;

						classType.source = locationSource;
						classType.scope = ctx.scope;
						classType.isExtendable = (type.typeFlags & int(TypeFlags.TYPE_IS_EXTENDABLE)) != 0;
						classType.baseClass = baseType;
					}
					else
					{
						classType = new TypeClass(identifier, locationSource, ctx.scope, nullptr, actualGenerics, (type.typeFlags & int(TypeFlags.TYPE_IS_EXTENDABLE)) != 0, baseType);
					}

					if(type.typeFlags & int(TypeFlags.TYPE_IS_COMPLETED))
						classType.completed = true;

					if(type.typeFlags & int(TypeFlags.TYPE_IS_INTERNAL))
						classType.isInternal = true;

					importedType = classType;

					if(!forwardDeclaration)
						ctx.AddType(importedType);
				}

				moduleCtx.types[i] = importedType;

				moduleCtx.types[i].importModule = importModule;

				assert(moduleCtx.types[i].name == typeName);

				importedType.alignment = type.defaultAlign;
				importedType.size = type.size;

				importedType.hasPointers = type.pointerCount != 0;

				if(getType with<TypeStruct>(importedType))
				{
					delayedTypes.push_back(DelayedType(i, currentConstant));

					currentConstant += type.constantCount;
				}

				if(TypeClass ref classType = getType with<TypeClass>(importedType))
					classType.hasFinalizer = type.typeFlags & int(TypeFlags.TYPE_HAS_FINALIZER);

				if(parentNamespace)
					ctx.PopScope(ScopeType.SCOPE_NAMESPACE);
			}
			break;
		default:
			Stop(ctx, source, "ERROR: new type in module %.*s named %s unsupported", FMT_ISTR(moduleCtx.data.name), StringRef(symbols, type.offsetToName));
		}
	}

	for(int i = 0; i < delayedTypes.size(); i++)
	{
		DelayedType ref delayedType = &delayedTypes[i];
		ExternTypeInfo ref type = &typeList[delayedType.index];

		switch(type.subCat)
		{
		case SubCategory.CAT_CLASS:
			{
				InplaceStr typeName = InplaceStr(StringRef(symbols, type.offsetToName));

				TypeBase ref importedType = moduleCtx.types[delayedType.index];

				StringRef memberNames = StringRef(symbols, typeName.end + 1);

				if(TypeStruct ref structType = getType with<TypeStruct>(importedType))
				{
					ctx.PushScope(importedType);

					if(TypeStruct ref classType = getType with<TypeStruct>(structType))
						classType.typeScope = ctx.scope;

					for(int n = 0; n < type.arrSizeOrMemberCount; n++)
					{
						InplaceStr memberName = InplaceStr(memberNames);

						SynIdentifier ref memberNameIdentifier = new SynIdentifier(memberName);

						memberNames = StringRef(symbols, memberName.end + 1);

						TypeBase ref memberType = moduleCtx.types[memberList[type.subTypeOrMemberOffset + n].type];

						if(!memberType)
							Stop(ctx, source, "ERROR: can't find member %d type for '%s' in module %.*s", n + 1, StringRef(symbols, type.offsetToName), FMT_ISTR(moduleCtx.data.name));

						VariableData ref member = new VariableData(source, ctx.scope, 0, memberType, memberNameIdentifier, memberList[type.subTypeOrMemberOffset + n].offset, ctx.uniqueVariableId++);

						structType.members.push_back(new MemberHandle(source, member, nullptr));
					}

					for(int n = 0; n < type.constantCount; n++)
					{
						ExternConstantInfo ref constantInfo = &constantList[delayedType.constantPos + n];

						InplaceStr memberName = InplaceStr(memberNames);

						SynIdentifier ref memberNameIdentifier = new SynIdentifier(memberName);

						memberNames = StringRef(symbols, memberName.end + 1);

						TypeBase ref constantType = moduleCtx.types[constantInfo.type];

						if(!constantType)
							Stop(ctx, source, "ERROR: can't find constant %d type for '%s' in module %.*s", n + 1, StringRef(symbols, type.offsetToName), FMT_ISTR(moduleCtx.data.name));

						ExprBase ref value = nullptr;

						if(constantType == ctx.typeBool)
						{
							value = new ExprBoolLiteral(source, constantType, constantInfo.value != 0);
						}
						else if(ctx.IsIntegerType(constantType) || isType with<TypeEnum>(constantType))
						{
							value = new ExprIntegerLiteral(source, constantType, constantInfo.value);
						}
						else if(ctx.IsFloatingPointType(constantType))
						{
							double data = memory::as_double(constantInfo.value);
							value = new ExprRationalLiteral(source, constantType, data);
						}
							
						if(!value)
							Stop(ctx, source, "ERROR: can't import constant %d of type '%.*s'", n + 1, FMT_ISTR(constantType.name));

						structType.constants.push_back(new ConstantData(memberNameIdentifier, value));
					}

					ctx.PopScope(ScopeType.SCOPE_TYPE);
				}

				if(TypeClass ref typeClass = getType with<TypeClass>(importedType))
				{
					int genericsFound = 0;

					RefList<MatchData> aliases;

					for(int k = 0; k < bCode.typedefCount; k++)
					{
						ExternTypedefInfo ref alias = &aliasList[k];

						if(alias.parentType == delayedType.index)
						{
							InplaceStr aliasName = InplaceStr(StringRef(symbols, alias.offsetToName));

							SynIdentifier ref aliasNameIdentifier = new SynIdentifier(aliasName);

							TypeBase ref targetType = moduleCtx.types[alias.targetType];

							if(!targetType)
								Stop(ctx, source, "ERROR: can't find type '%.*s' alias '%s' target type in module %.*s", FMT_ISTR(typeName), StringRef(symbols, alias.offsetToName), FMT_ISTR(moduleCtx.data.name));

							if(genericsFound < type.genericTypeCount)
								genericsFound++;
							else
								aliases.push_back(new MatchData(aliasNameIdentifier, targetType));
						}
					}

					typeClass.aliases = aliases;
				}
			}
			break;
		default:
			break;
		}
	}
}

void ImportModuleVariables(ExpressionContext ref ctx, SynBase ref source, ModuleContext ref moduleCtx)
{
	//TRACE_SCOPE("analyze", "ImportModuleVariables");

	ByteCode ref bCode = moduleCtx.data.bytecode;
	char[] symbols = FindSymbols(bCode);

	// Import variables
	ExternVarInfo[] variableList = FindFirstVar(bCode);

	for(int i = 0; i < bCode.variableExportCount; i++)
	{
		ExternVarInfo ref variable = &variableList[i];

		InplaceStr name = InplaceStr(StringRef(symbols, variable.offsetToName));

		// Exclude temporary variables from import
		if(name.length() >= 5 && InplaceStr(name.data, name.begin, name.begin + 5) == InplaceStr("$temp"))
			continue;

		TypeBase ref type = moduleCtx.types[variable.type];

		if(!type)
			Stop(ctx, source, "ERROR: can't find variable '%s' type in module %.*s", StringRef(symbols, variable.offsetToName), FMT_ISTR(moduleCtx.data.name));

		SynIdentifier ref nameIdentifier = new SynIdentifier(name);

		VariableData ref data = new VariableData(source, ctx.scope, 0, type, nameIdentifier, variable.offset, ctx.uniqueVariableId++);

		data.importModule = moduleCtx.data;

		ctx.AddVariable(data, true);

		if(name.length() > 5 && string(name).find("$vtbl") == 0)
		{
			ctx.vtables.push_back(data);
			ctx.vtableMap.insert(name, data);
		}
	}
}

void ImportModuleTypedefs(ExpressionContext ref ctx, SynBase ref source, ModuleContext ref moduleCtx)
{
	//TRACE_SCOPE("analyze", "ImportModuleTypedefs");

	ByteCode ref bCode = moduleCtx.data.bytecode;
	char[] symbols = FindSymbols(bCode);

	// Import type aliases
	ExternTypedefInfo[] aliasList = FindFirstTypedef(bCode);

	for(int i = 0; i < bCode.typedefCount; i++)
	{
		ExternTypedefInfo ref alias = &aliasList[i];

		InplaceStr aliasName = InplaceStr(StringRef(symbols, alias.offsetToName));

		SynIdentifier ref aliasNameIdentifier = new SynIdentifier(aliasName);

		TypeBase ref targetType = moduleCtx.types[alias.targetType];

		if(!targetType)
			Stop(ctx, source, "ERROR: can't find alias '%s' target type in module %.*s", StringRef(symbols, alias.offsetToName), FMT_ISTR(moduleCtx.data.name));

		if(TypeBase ref ref prev = ctx.typeMap.find(aliasName.hash()))
		{
			TypeBase ref type = *prev;

			if(type.name == aliasName)
				Stop(ctx, source, "ERROR: type '%.*s' alias '%s' is equal to previously imported class", FMT_ISTR(targetType.name), StringRef(symbols, alias.offsetToName));

			if(type != targetType)
				Stop(ctx, source, "ERROR: type '%.*s' alias '%s' is equal to previously imported alias", FMT_ISTR(targetType.name), StringRef(symbols, alias.offsetToName));
		}
		else if(alias.parentType != -1)
		{
			// Type alises were imported during type import
		}
		else
		{
			AliasData ref alias = new AliasData(source, ctx.scope, targetType, aliasNameIdentifier, ctx.uniqueAliasId++);

			alias.importModule = moduleCtx.data;

			ctx.AddAlias(alias);
		}
	}
}

void ImportModuleFunctions(ExpressionContext ref ctx, SynBase ref source, ModuleContext ref moduleCtx)
{
	//TRACE_SCOPE("analyze", "ImportModuleFunctions");

	ByteCode ref bCode = moduleCtx.data.bytecode;
	char[] symbols = FindSymbols(bCode);

	//ExternVarInfo ref explicitTypeInfo = FindFirstVar(bCode) + bCode.variableCount;
	ExternVarInfo[] baseVariables = FindFirstVar(bCode);
	int explicitTypeInfoOffset = 0;

	moduleCtx.data.importedFunctionCount = bCode.moduleFunctionCount;

	// Import functions
	ExternFuncInfo[] functionList = FindFirstFunc(bCode);
	ExternLocalInfo[] localList = FindFirstLocal(bCode);

	int currCount = ctx.functions.size();

	for(int i = 0; i < bCode.functionCount - bCode.moduleFunctionCount; i++)
	{
		ExternFuncInfo ref function = &functionList[i];

		InplaceStr functionName = InplaceStr(StringRef(symbols, function.offsetToName));

		TypeBase ref functionType = moduleCtx.types[function.funcType];

		if(!functionType)
			Stop(ctx, source, "ERROR: can't find function '%s' type in module %.*s", StringRef(symbols, function.offsetToName), FMT_ISTR(moduleCtx.data.name));

		// Import function explicit type list
		RefList<MatchData> generics;

		bool hasGenericExplicitType = false;

		for(int k = 0; k < function.explicitTypeCount; k++)
		{
			InplaceStr name = InplaceStr(StringRef(symbols, baseVariables[explicitTypeInfoOffset + k].offsetToName));

			SynIdentifier ref nameIdentifier = new SynIdentifier(name);

			TypeBase ref type = baseVariables[explicitTypeInfoOffset + k].type == -1 ? ctx.typeGeneric : moduleCtx.types[baseVariables[explicitTypeInfoOffset + k].type];

			if(!type)
				Stop(ctx, source, "ERROR: can't find function '%s' explicit type '%d' in module %.*s", StringRef(symbols, function.offsetToName), k, FMT_ISTR(moduleCtx.data.name));

			if(type.isGeneric)
				hasGenericExplicitType = true;

			generics.push_back(new MatchData(nameIdentifier, type));
		}

		explicitTypeInfoOffset += function.explicitTypeCount;

		FunctionData ref prev = nullptr;
		FunctionData ref prototype = nullptr;

		for(auto curr = ctx.functionMap.first(function.nameHash); curr; curr = ctx.functionMap.next(curr))
		{
			if(curr.value.isPrototype)
			{
				prototype = curr.value;
				continue;
			}

			if(curr.value.type == functionType)
			{
				bool explicitTypeMatch = true;

				for(int k = 0; k < function.explicitTypeCount; k++)
				{
					TypeBase ref prevType = curr.value.generics[k].type;
					TypeBase ref type = generics[k].type;

					if(&prevType != &type)
						explicitTypeMatch = false;
				}

				if(explicitTypeMatch)
				{
					prev = curr.value;
					break;
				}
			}
		}

		if(prev)
		{
			if(prev.name.name[0] == '$' || prev.isGenericInstance)
				ctx.functions.push_back(prev);
			else
				Stop(ctx, source, "ERROR: function %.*s (type %.*s) is already defined. While importing %.*s", FMT_ISTR(prev.name.name), FMT_ISTR(prev.type.name), FMT_ISTR(moduleCtx.data.name));

			continue;
		}

		NamespaceData ref parentNamespace = nullptr;

		for(int k = 0; k < ctx.namespaces.size(); k++)
		{
			if(ctx.namespaces[k].fullNameHash == function.namespaceHash)
			{
				parentNamespace = ctx.namespaces[k];
				break;
			}
		}

		if(parentNamespace)
			ctx.PushScope(parentNamespace);

		TypeBase ref parentType = nullptr;

		if(function.parentType != -1)
		{
			parentType = moduleCtx.types[function.parentType];

			if(!parentType)
				Stop(ctx, source, "ERROR: can't find function '%s' parent type in module %.*s", StringRef(symbols, function.offsetToName), FMT_ISTR(moduleCtx.data.name));
		}

		TypeBase ref contextType = nullptr;

		if(function.contextType != -1)
		{
			contextType = moduleCtx.types[function.contextType];

			if(!contextType)
				Stop(ctx, source, "ERROR: can't find function '%s' context type in module %.*s", StringRef(symbols, function.offsetToName), FMT_ISTR(moduleCtx.data.name));
		}

		if(!contextType)
			contextType = ctx.GetReferenceType(parentType ? parentType : ctx.typeVoid);

		bool isCoroutine = function.funcCat == int(FunctionCategory.COROUTINE);
		bool accessor = functionName[functionName.length() - 1] == '$';
		bool isOperator = function.isOperator != 0;

		if(parentType)
			ctx.PushScope(parentType);

		ModuleData ref importModule = moduleCtx.data;

		if(function.definitionModule != 0)
			importModule = ctx.dependencies[moduleCtx.data.startingDependencyIndex + function.definitionModule - 1];

		assert(function.definitionLocationStart < importModule.lexStream.size);
		assert(function.definitionLocationEnd < importModule.lexStream.size);

		LexemeRef locationSourceStart = LexemeRef(importModule.lexer, function.definitionLocationStart);
		LexemeRef locationSourceEnd = LexemeRef(importModule.lexer, function.definitionLocationEnd);

		SynBase ref locationSource = function.definitionLocationStart != 0 || function.definitionLocationEnd != 0 ? new SynImportLocation(locationSourceStart, locationSourceEnd) : source;

		assert(function.definitionLocationName < importModule.lexStream.size);

		LexemeRef locationName = LexemeRef(importModule.lexer, function.definitionLocationName);

		SynIdentifier ref identifier = function.definitionLocationName != 0 ? new SynIdentifier(locationName, locationName, functionName) : new SynIdentifier(functionName);

		FunctionData ref data = new FunctionData(locationSource, ctx.scope, isCoroutine, accessor, isOperator, getType with<TypeFunction>(functionType), contextType, identifier, generics, ctx.uniqueFunctionId++);

		data.importModule = importModule;

		data.isPrototype = (function.regVmCodeSize & 0x80000000) != 0;

		assert(data.isPrototype == ((function.regVmCodeSize & 0x80000000) != 0));

		if(prototype)
			prototype.implementation = data;

		// TODO: find function proto
		data.isGenericInstance = !!function.isGenericInstance;

		if(data.name.name == InplaceStr("__newS") || data.name.name == InplaceStr("__newA") || data.name.name == InplaceStr("__closeUpvalue"))
			data.isInternal = true;

		if(function.funcCat == int(FunctionCategory.LOCAL))
			data.isHidden = true;

		ctx.AddFunction(data);

		ctx.PushScope(data);

		data.functionScope = ctx.scope;

		for(int n = 0; n < function.paramCount; n++)
		{
			ExternLocalInfo ref argument = &localList[function.offsetToFirstLocal + n];

			bool isExplicit = (argument.paramFlags & int(LocalFlags.IS_EXPLICIT)) != 0;

			TypeBase ref argType = argument.type == -1 ? ctx.typeGeneric : moduleCtx.types[argument.type];

			if(!argType)
				Stop(ctx, source, "ERROR: can't find argument %d type for '%s' in module %.*s", n + 1, StringRef(symbols, function.offsetToName), FMT_ISTR(moduleCtx.data.name));

			InplaceStr argName = InplaceStr(StringRef(symbols, argument.offsetToName));

			SynIdentifier ref argNameIdentifier = new SynIdentifier(argName);

			data.arguments.push_back(ArgumentData(source, isExplicit, argNameIdentifier, argType, nullptr));

			int offset = AllocateArgumentInScope(ctx, source, 0, argType);
			VariableData ref variable = new VariableData(source, ctx.scope, 0, argType, argNameIdentifier, offset, ctx.uniqueVariableId++);

			ctx.AddVariable(variable, true);
		}

		assert(contextType != nullptr);

		if(parentType)
		{
			TypeBase ref type = ctx.GetReferenceType(parentType);

			int offset = AllocateArgumentInScope(ctx, source, 0, type);

			SynIdentifier ref argNameIdentifier = new SynIdentifier(InplaceStr("this"));

			VariableData ref variable = new VariableData(source, ctx.scope, 0, type, argNameIdentifier, offset, ctx.uniqueVariableId++);

			ctx.AddVariable(variable, true);
		}
		else if(contextType)
		{
			int offset = AllocateArgumentInScope(ctx, source, 0, contextType);

			SynIdentifier ref argNameIdentifier = new SynIdentifier(InplaceStr("$context"));

			VariableData ref variable = new VariableData(source, ctx.scope, 0, contextType, argNameIdentifier, offset, ctx.uniqueVariableId++);

			ctx.AddVariable(variable, false);
		}

		data.argumentsSize = data.functionScope.dataSize;

		// TODO: explicit flag
		if(function.funcType == 0 || functionType.isGeneric || hasGenericExplicitType || (parentType && parentType.isGeneric))
		{
			assert(function.genericOffsetStart < data.importModule.lexStream.size);

			data.delayedDefinition = LexemeRef(data.importModule.lexer, function.genericOffsetStart);

			TypeBase ref returnType = ctx.typeAuto;

			if(function.genericReturnType != -1)
				returnType = moduleCtx.types[function.genericReturnType];

			if(!returnType)
				Stop(ctx, source, "ERROR: can't find generic function '%s' return type in module %.*s", StringRef(symbols, function.offsetToName), FMT_ISTR(moduleCtx.data.name));

			RefList<TypeHandle> argTypes;

			for(int n = 0; n < function.paramCount; n++)
			{
				ExternLocalInfo ref argument = &localList[function.offsetToFirstLocal + n];

				argTypes.push_back(new TypeHandle(argument.type == -1 ? ctx.typeGeneric : moduleCtx.types[argument.type]));
			}

			data.type = ctx.GetFunctionType(source, returnType, argTypes);
		}

		assert(data.type != nullptr);

		ctx.PopScope(ScopeType.SCOPE_FUNCTION);

		if(data.isPrototype)
			ctx.HideFunction(data);
		else if(data.isHidden)
			ctx.HideFunction(data);

		if(parentType)
			ctx.PopScope(ScopeType.SCOPE_TYPE);

		if(parentNamespace)
			ctx.PopScope(ScopeType.SCOPE_NAMESPACE);
	}

	for(int i = 0; i < bCode.functionCount - bCode.moduleFunctionCount; i++)
	{
		ExternFuncInfo ref function = &functionList[i];

		FunctionData ref data = ctx.functions[currCount + i];

		for(int n = 0; n < function.paramCount; n++)
		{
			ExternLocalInfo ref argument = &localList[function.offsetToFirstLocal + n];

			if(argument.defaultFuncId != -1)
			{
				FunctionData ref target = ctx.functions[currCount + argument.defaultFuncId - bCode.moduleFunctionCount];

				ExprBase ref access = new ExprFunctionAccess(source, target.type, target, new ExprNullptrLiteral(source, target.contextType));

				data.arguments[n].value = new ExprFunctionCall(source, target.type.returnType, access, RefList<ExprBase>());
			}
		}
	}
}

void ImportModule(ExpressionContext ref ctx, SynBase ref source, ByteCode ref bytecode, Lexeme[] lexStream, InplaceStr name)
{
	//TRACE_SCOPE("analyze", "ImportModule");

/*#ifdef IMPORT_VERBOSE_DEBUG_OUTPUT
	printf("  importing module %.*s (import #%d) as dependency #%d\n", FMT_ISTR(name), ctx.imports.size() + 1, ctx.dependencies.size() + 1);
#endif*/

	assert(bytecode != nullptr);

	/*assert(*name.end == 0);*/
	assert(string(name).find(".nc") != -1);

	ModuleData ref moduleData = new ModuleData(source, name);

	ctx.imports.push_back(moduleData);
	moduleData.importIndex = ctx.imports.size();

	ctx.dependencies.push_back(moduleData);
	moduleData.dependencyIndex = ctx.dependencies.size();

	moduleData.bytecode = bytecode;

	if(lexStream == nullptr)
	{
		moduleData.lexer = new Lexer;

		moduleData.lexer.Lexify(FindSource(bytecode));
		lexStream = moduleData.lexer.lexems.data;

		/*assert(!*name.end);*/

		BinaryCache::PutLexemes(string(name), lexStream);
	}
	else
	{
		moduleData.lexer = new Lexer;

		moduleData.lexer.code = FindSource(bytecode);
		moduleData.lexer.lexems.data = lexStream;
		moduleData.lexer.lexems.count = lexStream.size;
	}

	moduleData.lexStream = lexStream;

	moduleData.startingFunctionIndex = ctx.functions.size();

	moduleData.startingDependencyIndex = ctx.dependencies.size();

	ModuleContext moduleCtx;

	moduleCtx.data = moduleData;

	ImportModuleDependencies(ctx, source, moduleCtx, moduleData.bytecode);

	ImportModuleNamespaces(ctx, source, moduleCtx);

	ImportModuleTypes(ctx, source, moduleCtx);

	ImportModuleVariables(ctx, source, moduleCtx);

	ImportModuleTypedefs(ctx, source, moduleCtx);

	ImportModuleFunctions(ctx, source, moduleCtx);

	moduleData.moduleFunctionCount = ctx.functions.size() - moduleData.startingFunctionIndex;
}

void AnalyzeModuleImport(ExpressionContext ref ctx, SynModuleImport ref syntax)
{
	InplaceStr moduleName = GetModuleName(syntax.path);

	//TRACE_SCOPE("analyze", "AnalyzeModuleImport");
	//TRACE_LABEL2(moduleName.begin, moduleName.end);

	ByteCode ref bytecode = BinaryCache::FindBytecode(string(moduleName), false);

	Lexeme[] lexStream = BinaryCache::FindLexems(string(moduleName), false);

	if(!bytecode)
		Stop(ctx, syntax, "ERROR: module import is not implemented");

	ImportModule(ctx, syntax, bytecode, lexStream, moduleName);
}

void AnalyzeImplicitModuleImports(ExpressionContext ref ctx)
{
	// Find which transitive dependencies haven't been imported explicitly
	for(int i = 0; i < ctx.dependencies.size(); i++)
	{
		bool hasImport = false;

		for(int k = 0; k < ctx.imports.size(); k++)
		{
			if(ctx.imports[k].bytecode == ctx.dependencies[i].bytecode)
			{
				hasImport = true;
				break;
			}
		}

		if(hasImport)
			continue;

		bool hasImplicitImport = false;

		for(int k = 0; k < ctx.implicitImports.size(); k++)
		{
			if(ctx.implicitImports[k].bytecode == ctx.dependencies[i].bytecode)
			{
				hasImplicitImport = true;
				break;
			}
		}

		if(hasImplicitImport)
			continue;

		ctx.implicitImports.push_back(ctx.dependencies[i]);
	}

	// Import additional modules
	for(int i = 0; i < ctx.implicitImports.size(); i++)
	{
		ModuleData ref moduleData = ctx.implicitImports[i];

		ImportModule(ctx, moduleData.source, moduleData.bytecode, moduleData.lexStream, moduleData.name);
	}
}

void CreateDefaultArgumentFunctionWrappers(ExpressionContext ref ctx)
{
	//TRACE_SCOPE("analyze", "CreateDefaultArgumentFunctionWrappers");

	for(int i = 0; i < ctx.functions.size(); i++)
	{
		FunctionData ref function = ctx.functions[i];

		if(function.importModule)
			continue;

		// Handle only global visible functions
		if(function.scope != ctx.globalScope && !function.scope.ownerType)
			continue;

		if(function.isHidden)
			continue;

		// Go through all function arguments
		for(int k = 0; k < function.arguments.size(); k++)
		{
			ArgumentData ref argument = &function.arguments[k];

			if(argument.value)
			{
				assert(argument.valueFunction == nullptr);

				ExprBase ref value = argument.value;

				if(isType with<TypeError>(value.type))
					continue;

				if(isType with<TypeFunctionSet>(value.type))
					value = CreateCast(ctx, argument.source, argument.value, argument.type, true);

				InplaceStr functionName = GetDefaultArgumentWrapperFunctionName(ctx, function, argument.name.name);

				ExprBase ref access = CreateValueFunctionWrapper(ctx, argument.source, nullptr, value, functionName);

				if(isType with<ExprError>(access))
					continue;

				assert(isType with<ExprFunctionAccess>(access));

				if(ExprFunctionAccess ref expr = getType with<ExprFunctionAccess>(access))
					argument.valueFunction = expr.function;
			}
		}
	}
}

ExprBase ref CreateVirtualTableUpdate(ExpressionContext ref ctx, SynBase ref source, VariableData ref vtable)
{
	RefList<ExprBase> expressions;

	// Find function name
	InplaceStr name = InplaceStr(vtable.name.name.data, vtable.name.name.begin + 15, vtable.name.name.end); // 15 to skip $vtbl0123456789 from name

	// Find function type from name
	long tmpNum = 0;
	int strPos = 5;
	while(vtable.name.name[strPos] >= '0' && vtable.name.name[strPos] <= '9')
		tmpNum = tmpNum * 10 + (vtable.name.name[strPos++] - '0');

	int typeNameHash = tmpNum;

	TypeBase ref functionType = nullptr;

	for(int i = 0; i < ctx.types.size(); i++)
	{
		if(ctx.types[i].nameHash == typeNameHash)
		{
			functionType = getType with<TypeFunction>(ctx.types[i]);
			break;
		}
	}

	if(!functionType)
		Stop(ctx, source, "ERROR: Can't find function type for virtual function table '%.*s'", FMT_ISTR(vtable.name.name));

	if(vtable.importModule == nullptr)
	{
		ExprBase ref count = CreateFunctionCall0(ctx, source, InplaceStr("__typeCount"), false, true, true);

		ExprBase ref alloc = CreateArrayAllocation(ctx, source, ctx.typeFunctionID, count);

		ExprBase ref assignment = CreateAssignment(ctx, source, new ExprVariableAccess(source, vtable.type, vtable), alloc);

		expressions.push_back(new ExprVariableDefinition(source, ctx.typeVoid, new VariableHandle(nullptr, vtable), assignment));
	}

	// Find all functions with called name that are member functions and have target type
	vector<FunctionData ref> functions;

	for(int i = 0; i < ctx.functions.size(); i++)
	{
		FunctionData ref function = ctx.functions[i];

		TypeBase ref parentType = function.scope.ownerType;

		if(!parentType)
			continue;

		if(parentType.isGeneric)
			continue;

		// If both type and table are imported, then it should have been filled up inside the module for that type
		if(parentType.importModule && vtable.importModule && parentType.importModule == vtable.importModule)
			continue;

		/*StringRef pos = strstr(function.name.name.begin, "::");

		if(!pos)
			continue;

		if(InplaceStr(pos + 2) == name && function.type == functionType)
			functions.push_back(function);*/
	}

	for(int i = 0; i < ctx.types.size(); i++)
	{
		for(int k = 0; k < functions.size(); k++)
		{
			TypeBase ref type = ctx.types[i];
			FunctionData ref function = functions[k];

			while(type)
			{
				if(function.scope.ownerType == type)
				{
					ExprBase ref vtableAccess = new ExprVariableAccess(source, vtable.type, vtable);

					ExprBase ref typeId = new ExprTypeLiteral(source, ctx.typeTypeID, ctx.types[i]);

					vector<ArgumentData> arguments;
					arguments.push_back(ArgumentData(source, false, nullptr, ctx.typeInt, new ExprTypeCast(source, ctx.typeInt, typeId, ExprTypeCastCategory.EXPR_CAST_REINTERPRET)));

					ExprBase ref arraySlot = CreateArrayIndex(ctx, source, vtableAccess, ViewOf(arguments));

					ExprBase ref assignment = CreateAssignment(ctx, source, arraySlot, new ExprFunctionIndexLiteral(source, ctx.typeFunctionID, function));

					expressions.push_back(assignment);
					break;
				}

				// Stepping through the class inheritance tree will ensure that the base class function will be used if the derived class function is not available
				if(TypeClass ref classType = getType with<TypeClass>(type))
					type = classType.baseClass;
				else
					type = nullptr;
			}
		}
	}

	return new ExprBlock(source, ctx.typeVoid, expressions, nullptr);
}

ExprModule ref AnalyzeModule(ExpressionContext ref ctx, SynModule ref syntax)
{
	//TRACE_SCOPE("analyze", "AnalyzeModule");

	// Import base module
	if(ByteCode ref bytecode = BinaryCache::GetBytecode(string("$base$.nc")))
	{
		Lexeme[] lexStream = BinaryCache::GetLexems(string("$base$.nc"));

		if(bytecode)
			ImportModule(ctx, syntax, bytecode, lexStream, InplaceStr("$base$.nc"));
		else
			Stop(ctx, syntax, "ERROR: base module couldn't be imported");

		ctx.baseModuleFunctionCount = ctx.functions.size();
	}

	for(SynModuleImport ref moduleImport = syntax.imports.head; moduleImport; moduleImport = getType with<SynModuleImport>(moduleImport.next))
		AnalyzeModuleImport(ctx, moduleImport);

	AnalyzeImplicitModuleImports(ctx);

	RefList<ExprBase> expressions;

	for(SynBase ref expr = syntax.expressions.head; expr; expr = expr.next)
		expressions.push_back(AnalyzeStatement(ctx, expr));

	ClosePendingUpvalues(ctx, nullptr);

	// Don't create wrappers in ill-formed module
	if(ctx.errorCount == 0)
		CreateDefaultArgumentFunctionWrappers(ctx);

	ExprModule ref module = new ExprModule(syntax, ctx.typeVoid, ctx.globalScope, expressions);

	for(int i = 0; i < ctx.definitions.size(); i++)
		module.definitions.push_back(ctx.definitions[i]);

	for(int i = 0; i < ctx.setup.size(); i++)
		module.setup.push_back(ctx.setup[i]);

	for(int i = 0; i < ctx.vtables.size(); i++)
		module.setup.push_back(CreateVirtualTableUpdate(ctx, ctx.MakeInternal(syntax), ctx.vtables[i]));

	for(int i = 0; i < ctx.upvalues.size(); i++)
		module.setup.push_back(new ExprVariableDefinition(ctx.MakeInternal(syntax), ctx.typeVoid, new VariableHandle(nullptr, ctx.upvalues[i]), nullptr));

	return module;
}

ExprModule ref Analyze(ExpressionContext ref ctx, SynModule ref syntax, char[] code)
{
	//TRACE_SCOPE("analyze", "Analyze");

	assert(!ctx.globalScope);

	ctx.code = code;

	ctx.PushScope(ScopeType.SCOPE_EXPLICIT);
	ctx.globalScope = ctx.scope;

	ctx.AddType(ctx.typeVoid = new TypeVoid(InplaceStr("void")));

	ctx.AddType(ctx.typeBool = new TypeBool(InplaceStr("bool")));

	ctx.AddType(ctx.typeChar = new TypeChar(InplaceStr("char")));
	ctx.AddType(ctx.typeShort = new TypeShort(InplaceStr("short")));
	ctx.AddType(ctx.typeInt = new TypeInt(InplaceStr("int")));
	ctx.AddType(ctx.typeLong = new TypeLong(InplaceStr("long")));

	ctx.AddType(ctx.typeFloat = new TypeFloat(InplaceStr("float")));
	ctx.AddType(ctx.typeDouble = new TypeDouble(InplaceStr("double")));

	ctx.AddType(ctx.typeTypeID = new TypeTypeID(InplaceStr("typeid")));
	ctx.AddType(ctx.typeFunctionID = new TypeFunctionID(InplaceStr("__function")));
	ctx.AddType(ctx.typeNullPtr = new TypeNullptr(InplaceStr("__nullptr")));

	ctx.AddType(ctx.typeGeneric = new TypeGeneric(InplaceStr("generic")));

	ctx.AddType(ctx.typeAuto = new TypeAuto(InplaceStr("auto")));

	ctx.AddType(ctx.typeAutoRef = new TypeAutoRef(InplaceStr("auto ref")));
	ctx.PushScope(ctx.typeAutoRef);
	ctx.typeAutoRef.typeScope = ctx.scope;
	ctx.typeAutoRef.members.push_back(new MemberHandle(syntax, AllocateClassMember(ctx, syntax, 0, ctx.typeTypeID, InplaceStr("type"), true, ctx.uniqueVariableId++), nullptr));
	ctx.typeAutoRef.members.push_back(new MemberHandle(syntax, AllocateClassMember(ctx, syntax, 0, ctx.GetReferenceType(ctx.typeVoid), InplaceStr("ptr"), true, ctx.uniqueVariableId++), nullptr));
	FinalizeAlignment(ctx.typeAutoRef);
	ctx.PopScope(ScopeType.SCOPE_TYPE);

	ctx.AddType(ctx.typeAutoArray = new TypeAutoArray(InplaceStr("auto[]")));
	ctx.PushScope(ctx.typeAutoArray);
	ctx.typeAutoArray.typeScope = ctx.scope;
	ctx.typeAutoArray.members.push_back(new MemberHandle(syntax, AllocateClassMember(ctx, syntax, 0, ctx.typeTypeID, InplaceStr("type"), true, ctx.uniqueVariableId++), nullptr));
	ctx.typeAutoArray.members.push_back(new MemberHandle(syntax, AllocateClassMember(ctx, syntax, 0, ctx.GetReferenceType(ctx.typeVoid), InplaceStr("ptr"), true, ctx.uniqueVariableId++), nullptr));
	ctx.typeAutoArray.members.push_back(new MemberHandle(syntax, AllocateClassMember(ctx, syntax, 0, ctx.typeInt, InplaceStr("size"), true, ctx.uniqueVariableId++), nullptr));
	FinalizeAlignment(ctx.typeAutoArray);
	ctx.PopScope(ScopeType.SCOPE_TYPE);

	//int traceDepth = NULLC::TraceGetDepth();

	// Analyze module
	auto tryResult = try(auto(){
		ctx.errorHandlerActive = true;

		/*if(ctx.memoryLimit != 0)
		{
			int totalLimit = ctx.allocator.requested() + ctx.memoryLimit;

			ctx.allocator.set_limit(totalLimit, &ctx, OnMemoryLimitHit);
		}*/

		ExprModule ref module = AnalyzeModule(ctx, syntax);

		ctx.errorHandlerActive = false;

		/*if(ctx.memoryLimit != 0)
			ctx.allocator.clear_limit();*/

		ctx.PopScope(ScopeType.SCOPE_EXPLICIT);

		assert(ctx.scope == nullptr);

		return module;
	});

	if(tryResult)
	{
		return tryResult.value;
	}
	else
	{
		if(tryResult.exception.type != AnalyzerError)
			tryResult.rethrow();
	}

	//NULLC::TraceLeaveTo(traceDepth);

	/*assert(ctx.errorPos != 0);*/

	return nullptr;
}

void VisitExpressionTreeNodes(ExprBase ref expression, void ref(ExprBase ref) accept)
{
	if(!expression)
		return;

	accept(expression);

	if(ExprError ref node = getType with<ExprError>(expression))
	{
		for(int i = 0; i < node.values.size(); i++)
			VisitExpressionTreeNodes(node.values[i], accept);
	}
	else if(ExprPassthrough ref node = getType with<ExprPassthrough>(expression))
	{
		VisitExpressionTreeNodes(node.value, accept);
	}
	else if(ExprArray ref node = getType with<ExprArray>(expression))
	{
		for(ExprBase ref value = node.values.head; value; value = value.next)
			VisitExpressionTreeNodes(value, accept);
	}
	else if(ExprPreModify ref node = getType with<ExprPreModify>(expression))
	{
		VisitExpressionTreeNodes(node.value, accept);
	}
	else if(ExprPostModify ref node = getType with<ExprPostModify>(expression))
	{
		VisitExpressionTreeNodes(node.value, accept);
	}
	else if(ExprTypeCast ref node = getType with<ExprTypeCast>(expression))
	{
		VisitExpressionTreeNodes(node.value, accept);
	}
	else if(ExprUnaryOp ref node = getType with<ExprUnaryOp>(expression))
	{
		VisitExpressionTreeNodes(node.value, accept);
	}
	else if(ExprBinaryOp ref node = getType with<ExprBinaryOp>(expression))
	{
		VisitExpressionTreeNodes(node.lhs, accept);
		VisitExpressionTreeNodes(node.rhs, accept);
	}
	else if(ExprDereference ref node = getType with<ExprDereference>(expression))
	{
		VisitExpressionTreeNodes(node.value, accept);
	}
	else if(ExprUnboxing ref node = getType with<ExprUnboxing>(expression))
	{
		VisitExpressionTreeNodes(node.value, accept);
	}
	else if(ExprConditional ref node = getType with<ExprConditional>(expression))
	{
		VisitExpressionTreeNodes(node.condition, accept);
		VisitExpressionTreeNodes(node.trueBlock, accept);
		VisitExpressionTreeNodes(node.falseBlock, accept);
	}
	else if(ExprAssignment ref node = getType with<ExprAssignment>(expression))
	{
		VisitExpressionTreeNodes(node.lhs, accept);
		VisitExpressionTreeNodes(node.rhs, accept);
	}
	else if(ExprMemberAccess ref node = getType with<ExprMemberAccess>(expression))
	{
		VisitExpressionTreeNodes(node.value, accept);
	}
	else if(ExprArrayIndex ref node = getType with<ExprArrayIndex>(expression))
	{
		VisitExpressionTreeNodes(node.value, accept);
		VisitExpressionTreeNodes(node.index, accept);
	}
	else if(ExprReturn ref node = getType with<ExprReturn>(expression))
	{
		VisitExpressionTreeNodes(node.value, accept);

		VisitExpressionTreeNodes(node.coroutineStateUpdate, accept);

		VisitExpressionTreeNodes(node.closures, accept);
	}
	else if(ExprYield ref node = getType with<ExprYield>(expression))
	{
		VisitExpressionTreeNodes(node.value, accept);

		VisitExpressionTreeNodes(node.coroutineStateUpdate, accept);

		VisitExpressionTreeNodes(node.closures, accept);
	}
	else if(ExprVariableDefinition ref node = getType with<ExprVariableDefinition>(expression))
	{
		VisitExpressionTreeNodes(node.initializer, accept);
	}
	else if(ExprArraySetup ref node = getType with<ExprArraySetup>(expression))
	{
		VisitExpressionTreeNodes(node.lhs, accept);
		VisitExpressionTreeNodes(node.initializer, accept);
	}
	else if(ExprVariableDefinitions ref node = getType with<ExprVariableDefinitions>(expression))
	{
		for(ExprBase ref value = node.definitions.head; value; value = value.next)
			VisitExpressionTreeNodes(value, accept);
	}
	else if(ExprFunctionDefinition ref node = getType with<ExprFunctionDefinition>(expression))
	{
		VisitExpressionTreeNodes(node.contextArgument, accept);

		for(ExprBase ref arg = node.arguments.head; arg; arg = arg.next)
			VisitExpressionTreeNodes(arg, accept);

		VisitExpressionTreeNodes(node.coroutineStateRead, accept);

		for(ExprBase ref expr = node.expressions.head; expr; expr = expr.next)
			VisitExpressionTreeNodes(expr, accept);
	}
	else if(ExprGenericFunctionPrototype ref node = getType with<ExprGenericFunctionPrototype>(expression))
	{
		for(ExprBase ref expr = node.contextVariables.head; expr; expr = expr.next)
			VisitExpressionTreeNodes(expr, accept);
	}
	else if(ExprFunctionAccess ref node = getType with<ExprFunctionAccess>(expression))
	{
		VisitExpressionTreeNodes(node.context, accept);
	}
	else if(ExprFunctionOverloadSet ref node = getType with<ExprFunctionOverloadSet>(expression))
	{
		VisitExpressionTreeNodes(node.context, accept);
	}
	else if(ExprShortFunctionOverloadSet ref node = getType with<ExprShortFunctionOverloadSet>(expression))
	{
		for(ShortFunctionHandle ref arg = node.functions.head; arg; arg = arg.next)
			VisitExpressionTreeNodes(arg.context, accept);
	}
	else if(ExprFunctionCall ref node = getType with<ExprFunctionCall>(expression))
	{
		VisitExpressionTreeNodes(node.function, accept);

		for(ExprBase ref arg = node.arguments.head; arg; arg = arg.next)
			VisitExpressionTreeNodes(arg, accept);
	}
	else if(ExprGenericClassPrototype ref node = getType with<ExprGenericClassPrototype>(expression))
	{
		for(ExprBase ref value in node.genericProtoType.instances)
			VisitExpressionTreeNodes(value, accept);
	}
	else if(ExprClassDefinition ref node = getType with<ExprClassDefinition>(expression))
	{
		for(ExprBase ref value = node.functions.head; value; value = value.next)
			VisitExpressionTreeNodes(value, accept);

		for(ConstantData ref value = node.classType.constants.head; value; value = value.next)
			VisitExpressionTreeNodes(value.value, accept);
	}
	else if(ExprEnumDefinition ref node = getType with<ExprEnumDefinition>(expression))
	{
		for(ConstantData ref value = node.enumType.constants.head; value; value = value.next)
			VisitExpressionTreeNodes(value.value, accept);

		VisitExpressionTreeNodes(node.toInt, accept);
		VisitExpressionTreeNodes(node.toEnum, accept);
	}
	else if(ExprIfElse ref node = getType with<ExprIfElse>(expression))
	{
		VisitExpressionTreeNodes(node.condition, accept);
		VisitExpressionTreeNodes(node.trueBlock, accept);
		VisitExpressionTreeNodes(node.falseBlock, accept);
	}
	else if(ExprFor ref node = getType with<ExprFor>(expression))
	{
		VisitExpressionTreeNodes(node.initializer, accept);
		VisitExpressionTreeNodes(node.condition, accept);
		VisitExpressionTreeNodes(node.increment, accept);
		VisitExpressionTreeNodes(node.body, accept);
	}
	else if(ExprWhile ref node = getType with<ExprWhile>(expression))
	{
		VisitExpressionTreeNodes(node.condition, accept);
		VisitExpressionTreeNodes(node.body, accept);
	}
	else if(ExprDoWhile ref node = getType with<ExprDoWhile>(expression))
	{
		VisitExpressionTreeNodes(node.body, accept);
		VisitExpressionTreeNodes(node.condition, accept);
	}
	else if(ExprSwitch ref node = getType with<ExprSwitch>(expression))
	{
		VisitExpressionTreeNodes(node.condition, accept);

		for(ExprBase ref value = node.cases.head; value; value = value.next)
			VisitExpressionTreeNodes(value, accept);

		for(ExprBase ref value = node.blocks.head; value; value = value.next)
			VisitExpressionTreeNodes(value, accept);

		VisitExpressionTreeNodes(node.defaultBlock, accept);
	}
	else if(ExprBreak ref node = getType with<ExprBreak>(expression))
	{
		VisitExpressionTreeNodes(node.closures, accept);
	}
	else if(ExprContinue ref node = getType with<ExprContinue>(expression))
	{
		VisitExpressionTreeNodes(node.closures, accept);
	}
	else if(ExprBlock ref node = getType with<ExprBlock>(expression))
	{
		for(ExprBase ref value = node.expressions.head; value; value = value.next)
			VisitExpressionTreeNodes(value, accept);

		VisitExpressionTreeNodes(node.closures, accept);
	}
	else if(ExprSequence ref node = getType with<ExprSequence>(expression))
	{
		for(int i = 0; i < node.expressions.size(); i++)
			VisitExpressionTreeNodes(node.expressions[i], accept);
	}
	else if(ExprModule ref node = getType with<ExprModule>(expression))
	{
		for(ExprBase ref value = node.setup.head; value; value = value.next)
			VisitExpressionTreeNodes(value, accept);

		for(ExprBase ref value = node.expressions.head; value; value = value.next)
			VisitExpressionTreeNodes(value, accept);
	}
}

char[] GetExpressionTreeNodeName(ExprBase ref expression)
{
	switch(typeid(expression))
	{
	case ExprError:
		return "ExprError";
	case ExprErrorTypeMemberAccess:
		return "ExprErrorTypeMemberAccess";
	case ExprVoid:
		return "ExprVoid";
	case ExprBoolLiteral:
		return "ExprBoolLiteral";
	case ExprCharacterLiteral:
		return "ExprCharacterLiteral";
	case ExprStringLiteral:
		return "ExprStringLiteral";
	case ExprIntegerLiteral:
		return "ExprIntegerLiteral";
	case ExprRationalLiteral:
		return "ExprRationalLiteral";
	case ExprTypeLiteral:
		return "ExprTypeLiteral";
	case ExprNullptrLiteral:
		return "ExprNullptrLiteral";
	case ExprFunctionIndexLiteral:
		return "ExprFunctionIndexLiteral";
	case ExprPassthrough:
		return "ExprPassthrough";
	case ExprArray:
		return "ExprArray";
	case ExprPreModify:
		return "ExprPreModify";
	case ExprPostModify:
		return "ExprPostModify";
	case ExprTypeCast:
		return "ExprTypeCast";
	case ExprUnaryOp:
		return "ExprUnaryOp";
	case ExprBinaryOp:
		return "ExprBinaryOp";
	case ExprGetAddress:
		return "ExprGetAddress";
	case ExprDereference:
		return "ExprDereference";
	case ExprUnboxing:
		return "ExprUnboxing";
	case ExprConditional:
		return "ExprConditional";
	case ExprAssignment:
		return "ExprAssignment";
	case ExprMemberAccess:
		return "ExprMemberAccess";
	case ExprArrayIndex:
		return "ExprArrayIndex";
	case ExprReturn:
		return "ExprReturn";
	case ExprYield:
		return "ExprYield";
	case ExprVariableDefinition:
		return "ExprVariableDefinition";
	case ExprArraySetup:
		return "ExprArraySetup";
	case ExprVariableDefinitions:
		return "ExprVariableDefinitions";
	case ExprVariableAccess:
		return "ExprVariableAccess";
	case ExprFunctionContextAccess:
		return "ExprFunctionContextAccess";
	case ExprFunctionDefinition:
		return "ExprFunctionDefinition";
	case ExprGenericFunctionPrototype:
		return "ExprGenericFunctionPrototype";
	case ExprFunctionAccess:
		return "ExprFunctionAccess";
	case ExprFunctionOverloadSet:
		return "ExprFunctionOverloadSet";
	case ExprShortFunctionOverloadSet:
		return "ExprShortFunctionOverloadSet";
	case ExprFunctionCall:
		return "ExprFunctionCall";
	case ExprAliasDefinition:
		return "ExprAliasDefinition";
	case ExprClassPrototype:
		return "ExprClassPrototype";
	case ExprGenericClassPrototype:
		return "ExprGenericClassPrototype";
	case ExprClassDefinition:
		return "ExprClassDefinition";
	case ExprEnumDefinition:
		return "ExprEnumDefinition";
	case ExprIfElse:
		return "ExprIfElse";
	case ExprFor:
		return "ExprFor";
	case ExprWhile:
		return "ExprWhile";
	case ExprDoWhile:
		return "ExprDoWhile";
	case ExprSwitch:
		return "ExprSwitch";
	case ExprBreak:
		return "ExprBreak";
	case ExprContinue:
		return "ExprContinue";
	case ExprBlock:
		return "ExprBlock";
	case ExprSequence:
		return "ExprSequence";
	case ExprModule:
		return "ExprModule";
	default:
		break;
	}
	
	assert(false, "unknown type");
	return "unknown";
}
