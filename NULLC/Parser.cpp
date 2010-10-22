#include "Parser.h"
#include "CodeInfo.h"
using namespace CodeInfo;

#include "Callbacks.h"

ChunkedStackPool<4092>	stringPool;

// indexed by [lexeme - lex_add]
char opPrecedence[] = { 2, 2, 1, 1, 1, 0, 4, 4, 3, 4, 4, 3, 5, 5, 6, 8, 7, 9, 11, 10 /* + - * / % ** < <= << > >= >> == != & | ^ and or xor */ };
CmdID opHandler[] = { cmdAdd, cmdSub, cmdMul, cmdDiv, cmdMod, cmdPow, cmdLess, cmdLEqual, cmdShl, cmdGreater, cmdGEqual, cmdShr, cmdEqual, cmdNEqual, cmdBitAnd, cmdBitOr, cmdBitXor, cmdLogAnd, cmdLogOr, cmdLogXor };

char*	AllocateString(unsigned int size)
{
	return (char*)stringPool.Allocate(size);
}

void ClearStringList()
{
	stringPool.Clear();
}

#define CALLBACK(x) x

inline bool ParseLexem(Lexeme** str, LexemeType type)
{
	if((*str)->type != type)
		return false;
	(*str)++;
	return true;
}

unsigned int ParseTypename(Lexeme** str)
{
	if((*str)->type != lex_string)
		return false;

	unsigned int hash = GetStringHash((*str)->pos, (*str)->pos + (*str)->length);
	TypeInfo **type = CodeInfo::classMap.find(hash);
	if(type)
	{
		(*str)++;
		return (*type)->typeIndex + 1;
	}
	return 0;
}

bool ParseNumber(Lexeme** str)
{
	Lexeme *number = *str;

	if(!ParseLexem(str, lex_number))
		return false;

	const char *start = number->pos;

	if(/*start[0] == '0' && */start[1] == 'x')	// hexadecimal
	{
		if(number->length == 2)
			ThrowError(start+2, "ERROR: '0x' must be followed by number");
		CALLBACK(AddHexInteger(number->pos, number->pos+number->length));
		return true;
	}

	bool isFP = false;
	for(unsigned int i = 0; i < number->length; i++)
		if(number->pos[i] == '.' || number->pos[i] == 'e')
			isFP = true;

	if(!isFP)
	{
		if((*str)->pos[0] == 'b')
		{
			(*str)++;
			CALLBACK(AddBinInteger(number->pos, number->pos+number->length));
			return true;
		}else if((*str)->pos[0] == 'l'){
			(*str)++;
			CALLBACK(AddNumberNodeLong(number->pos, number->pos+number->length));
			return true;
		}else if(number->pos[0] == '0' && isDigit(number->pos[1])){
			CALLBACK(AddOctInteger(number->pos, number->pos+number->length));
			return true;
		}else{
			CALLBACK(AddNumberNodeInt(number->pos));
			return true;
		}
	}else{
		if((*str)->pos[0] == 'f')
		{
			(*str)++;
			CALLBACK(AddNumberNodeFloat(number->pos));
			return true;
		}else{
			CALLBACK(AddNumberNodeDouble(number->pos));
			return true;
		}
	}
}

bool ParseArrayDefinition(Lexeme** str)
{
	if(!ParseLexem(str, lex_obracket))
		return false;

	ResetConstantFoldError();
	if((*str)->type == lex_cbracket)
	{
		(*str)++;
		CALLBACK(AddUnfixedArraySize());
	}else{
		if(!ParseTernaryExpr(str))
			ThrowError((*str)->pos, "ERROR: unexpected expression after '['");
		if(!ParseLexem(str, lex_cbracket))
			ThrowError((*str)->pos, "ERROR: matching ']' not found");
	}
	ThrowConstantFoldError((*str)->pos);
	if((*str)->type == lex_obracket)
		ParseArrayDefinition(str);
	CALLBACK(ConvertTypeToArray((*str)->pos));
	return true;
}

struct TypeHandler
{
	TypeInfo	*varType;
	TypeHandler	*next;
};

bool ParseTypeofExtended(Lexeme** str, bool& notType)
{
	if(!ParseLexem(str, lex_point))
		return false;

	if(notType)
		ThrowError((*str)->pos, "ERROR: typeof expression result is not a type");

	// work around bug in msvs2008
	Lexeme *curr = *str;
	bool genericType = GetSelectedType() == typeGeneric;
	// .argument .return .target
	if(curr->type == lex_string && curr->length == 8 && memcmp(curr->pos, "argument", 8) == 0)
	{
		curr++;
		if(!GetSelectedType()->funcType && !genericType)
			ThrowError(curr->pos, "ERROR: 'argument' can only be applied to a function type, but we have '%s'", GetSelectedType()->GetFullTypeName());
		if(!ParseLexem(&curr, lex_point) && curr->type != lex_obracket)
			ThrowError(curr->pos, "ERROR: expected '.first'/'.last'/'[N]' at this point");
		unsigned paramCount = !genericType ? GetSelectedType()->funcType->paramCount : 0;
		if(curr->type == lex_string && curr->length == 5 && memcmp(curr->pos, "first", 5) == 0)
		{
			curr++;
			if(!genericType)
			{
				if(!paramCount)
					ThrowError(curr->pos, "ERROR: this function type '%s' doesn't have arguments", GetSelectedType()->GetFullTypeName());
				SelectTypeByPointer(GetSelectedType()->funcType->paramType[0]);
			}
		}else if(curr->type == lex_string && curr->length == 4 && memcmp(curr->pos, "last", 4) == 0){
			curr++;
			if(!genericType)
			{
				if(!paramCount)
					ThrowError(curr->pos, "ERROR: this function type '%s' doesn't have arguments", GetSelectedType()->GetFullTypeName());
				SelectTypeByPointer(GetSelectedType()->funcType->paramType[paramCount-1]);
			}
		}else if(ParseLexem(&curr, lex_obracket)){
			if(curr->type != lex_number)
				ThrowError(curr->pos, "ERROR: argument number expected after '['");
			unsigned request = atoi(curr->pos);
			if(request >= paramCount && !genericType)
				ThrowError(curr->pos, "ERROR: this function type '%s' has only %d argument(s)", GetSelectedType()->GetFullTypeName(), paramCount);
			curr++;
			if(!ParseLexem(&curr, lex_cbracket))
				ThrowError(curr->pos, "ERROR: expected ']'");
			if(!genericType)
				SelectTypeByPointer(GetSelectedType()->funcType->paramType[request]);
		}else if(curr->type == lex_string && curr->length == 4 && memcmp(curr->pos, "size", 4) == 0){
			curr++;
			CodeInfo::nodeList.push_back(new NodeNumber(genericType ? 0 : (int)paramCount, typeVoid));
			notType = true;
		}else{
			ThrowError(curr->pos, "ERROR: expected 'first'/'last' at this point");
		}
	}else if(ParseLexem(&curr, lex_return)){
		if(!genericType)
		{
			if(!GetSelectedType()->funcType)
				ThrowError(curr->pos, "ERROR: 'return' can only be applied to a function type, but we have '%s'", GetSelectedType()->GetFullTypeName());
			SelectTypeByPointer(GetSelectedType()->funcType->retType);
		}
	}else if(curr->type == lex_string && curr->length == 6 && memcmp(curr->pos, "target", 6) == 0){
		curr++;
		if(!genericType)
		{
			if(!GetSelectedType()->refLevel && !GetSelectedType()->arrLevel)
				ThrowError(curr->pos, "ERROR: 'target' can only be applied to a pointer or array type, but we have '%s'", GetSelectedType()->GetFullTypeName());
			SelectTypeByPointer(GetSelectedType()->subType);
		}
	}else if(curr->type == lex_string && curr->length == 11 && memcmp(curr->pos, "isReference", 11) == 0){
		curr++;
		CodeInfo::nodeList.push_back(new NodeNumber(genericType ? 0 : (GetSelectedType()->refLevel ? 1 : 0), typeVoid));
		notType = true;
	}else if(curr->type == lex_string && curr->length == 7 && memcmp(curr->pos, "isArray", 7) == 0){
		curr++;
		CodeInfo::nodeList.push_back(new NodeNumber(genericType ? 0 : (GetSelectedType()->arrLevel ? 1 : 0), typeVoid));
		notType = true;
	}else if(curr->type == lex_string && curr->length == 10 && memcmp(curr->pos, "isFunction", 10) == 0){
		curr++;
		CodeInfo::nodeList.push_back(new NodeNumber(genericType ? 0 : (GetSelectedType()->funcType ? 1 : 0), typeVoid));
		notType = true;
	}else if(curr->type == lex_string && curr->length == 9 && memcmp(curr->pos, "arraySize", 9) == 0){
		curr++;
		if(!genericType && !GetSelectedType()->arrLevel)
			ThrowError(curr->pos, "ERROR: 'arraySize' can only be applied to an array type, but we have '%s'", GetSelectedType()->GetFullTypeName());
		CodeInfo::nodeList.push_back(new NodeNumber(genericType ? 0 : ((int)GetSelectedType()->arrSize), typeVoid));
		notType = true;
	}else{
		ThrowError(curr->pos, "ERROR: expected 'argument'/'return'/'target' at this point");
	}
	*str = curr;
	return true;
}

bool ParseSelectType(Lexeme** str, bool arrayType, bool genericOnFail)
{
	bool notType = false;
	if((*str)->type == lex_typeof)
	{
		(*str)++;
		if(!ParseLexem(str, lex_oparen))
			ThrowError((*str)->pos, "ERROR: typeof must be followed by '('");

		jmp_buf oldHandler;
		memcpy(oldHandler, CodeInfo::errorHandler, sizeof(jmp_buf));
		if(!genericOnFail || !setjmp(CodeInfo::errorHandler)) // if genericOnFail is enabled, we will set error handler
		{
			if(!ParseVaribleSet(str))
				ThrowError((*str)->pos, "ERROR: expression not found after typeof(");
			SetTypeOfLastNode();
		}else{
			if(!FunctionGeneric(false))
			{
				memcpy(CodeInfo::errorHandler, oldHandler, sizeof(jmp_buf));
				longjmp(CodeInfo::errorHandler, 1);
			}
			SelectTypeByPointer(typeGeneric);
		}
		if(genericOnFail)
			memcpy(CodeInfo::errorHandler, oldHandler, sizeof(jmp_buf));
		if(!ParseLexem(str, lex_cparen))
			ThrowError((*str)->pos, "ERROR: ')' not found after expression in typeof");
		while(ParseTypeofExtended(str, notType));
	}else if((*str)->type == lex_auto){
		CALLBACK(SelectTypeByPointer(NULL));
		(*str)++;
	}else if((*str)->type == lex_string && (*str+1)->type != lex_oparen){
		unsigned int index;
		if((index = ParseTypename(str)) == 0)
			return false;
		CALLBACK(SelectTypeByIndex(index - 1));
	}else{
		return false;
	}

	bool run = true;
	while(run)
	{
		switch((*str)->type)
		{
		case lex_ref:
			(*str)++;
			if(notType)
				ThrowError((*str)->pos, "ERROR: typeof expression result is not a type");
			if(ParseLexem(str, lex_oparen))
			{
				Lexeme *old = (*str) - 1;
				// Prepare function type
				TypeInfo *retType = (TypeInfo*)CALLBACK(GetSelectedType());
				if(!retType)
					ThrowError((*str)->pos, "ERROR: return type of a function type cannot be auto");
				TypeHandler *first = NULL, *handle = NULL;
				unsigned int count = 0;
				if(ParseSelectType(str))
				{
					count++;
					first = handle = (TypeHandler*)stringPool.Allocate(sizeof(TypeHandler));
					handle->varType = (TypeInfo*)CALLBACK(GetSelectedType());
					handle->next = NULL;
					if(!handle->varType)
						ThrowError((*str)->pos, "ERROR: parameter type of a function type cannot be auto");
					while(ParseLexem(str, lex_comma))
					{
						count++;
						ParseSelectType(str);
						handle->next = (TypeHandler*)stringPool.Allocate(sizeof(TypeHandler));
						handle = handle->next;
						handle->varType = (TypeInfo*)CALLBACK(GetSelectedType());
						handle->next = NULL;
						if(!handle->varType)
							ThrowError((*str)->pos, "ERROR: parameter type of a function type cannot be auto");
					}
				}
				if(ParseLexem(str, lex_cparen))
					CALLBACK(SelectTypeByPointer(CodeInfo::GetFunctionType(retType, first, count)));
				else{
					ConvertTypeToReference((*str)->pos);
					run = false;
					*str = old;
				}
			}else{
				CALLBACK(ConvertTypeToReference((*str)->pos));
			}
			break;
		case lex_obracket:
			if(notType)
				ThrowError((*str)->pos, "ERROR: typeof expression result is not a type");
			if(arrayType)
				ParseArrayDefinition(str);
			else
				run = false;
			break;
		default:
			run = false;
		}
	}
	return true;
}

bool ParseClassDefinition(Lexeme** str)
{
	Lexeme *curr = *str;
	if(!ParseAlignment(str))
		CALLBACK(SetCurrentAlignment(0));

	if(ParseLexem(str, lex_class))
	{
		if((*str)->type != lex_string)
			ThrowError((*str)->pos, "ERROR: class name expected");
		CALLBACK(TypeBegin((*str)->pos, (*str)->pos+(*str)->length));
		(*str)++;
		if(!ParseLexem(str, lex_ofigure))
			ThrowError((*str)->pos, "ERROR: '{' not found after class name");

		while((*str)->type != lex_cfigure)
		{
			if(ParseTypedefExpr(str))
				continue;
			if(!ParseSelectType(str))
				break;
			bool isVarDef = ((*str)->type == lex_string) && ((*str + 1)->type == lex_comma || (*str + 1)->type == lex_semicolon || (*str + 1)->type == lex_set);
			if(isVarDef || !ParseFunctionDefinition(str))
			{
				if((*str)->type != lex_string)
					ThrowError((*str)->pos, "ERROR: class member name expected after type");
				if((*str)->length >= NULLC_MAX_VARIABLE_NAME_LENGTH)
					ThrowError((*str)->pos, "ERROR: member name length is limited to 2048 symbols");
				unsigned int memberNameLength = (*str)->length;
				char	*memberName = (char*)stringPool.Allocate(memberNameLength + 2);
				memcpy(memberName, (*str)->pos, memberNameLength);
				memberName[memberNameLength] = 0;
				(*str)++;

				if(ParseLexem(str, lex_ofigure))
				{
					// Parse property
					if((*str)->type != lex_string || (*str)->length != 3 || memcmp((*str)->pos, "get", 3) != 0)
						ThrowError((*str)->pos, "ERROR: 'get' is expected after '{'");
					
					memberName[memberNameLength] = '$';
					memberName[memberNameLength + 1] = 0;
					
					CALLBACK(FunctionAdd((*str)->pos, memberName));
					(*str)++;
					CALLBACK(FunctionStart((*str-1)->pos));
					if(!ParseBlock(str))
						ThrowError((*str)->pos, "ERROR: function body expected after 'get'");
					CALLBACK(FunctionEnd((*str-1)->pos));
					// Get function return type
					TypeInfo *propType = GetSelectedType();
					if((*str)->type == lex_string || (*str)->length == 3 || memcmp((*str)->pos, "set", 3) == 0)
					{
						// Set setter return type to auto
						SelectTypeByPointer(NULL);
						CALLBACK(FunctionAdd((*str)->pos, memberName));
						(*str)++;
						// Set setter parameter type to getter return type
						SelectTypeByPointer(propType);
						// Parse optional parameter name
						if(ParseLexem(str, lex_oparen))
						{
							if((*str)->type != lex_string)
								ThrowError((*str)->pos, "ERROR: r-value name not found after '('");
							if((*str)->length >= NULLC_MAX_VARIABLE_NAME_LENGTH)
								ThrowError((*str)->pos, "ERROR: r-value name length is limited to 2048 symbols");
							CALLBACK(FunctionParameter((*str)->pos, InplaceStr((*str)->pos, (*str)->length)));
							(*str)++;
							if(!ParseLexem(str, lex_cparen))
								ThrowError((*str)->pos, "ERROR: ')' not found after r-value");
						}else{
							CALLBACK(FunctionParameter((*str)->pos, InplaceStr("r")));
						}
						CALLBACK(FunctionStart((*str-1)->pos));
						if(!ParseBlock(str))
							ThrowError((*str)->pos, "ERROR: function body expected after 'set'");
						CALLBACK(FunctionEnd((*str-1)->pos));
					}
					if(!ParseLexem(str, lex_cfigure))
						ThrowError((*str)->pos, "ERROR: '}' is expected after property");
				}else{
					CALLBACK(TypeAddMember((*str-1)->pos, memberName));

					while(ParseLexem(str, lex_comma))
					{
						if((*str)->type != lex_string)
							ThrowError((*str)->pos, "ERROR: member name expected after ','");
						if((*str)->length >= NULLC_MAX_VARIABLE_NAME_LENGTH)
							ThrowError((*str)->pos, "ERROR: member name length is limited to 2048 symbols");
						char	*memberName = (char*)stringPool.Allocate((*str)->length+1);
						memcpy(memberName, (*str)->pos, (*str)->length);
						memberName[(*str)->length] = 0;
						CALLBACK(TypeAddMember((*str)->pos, memberName));
						(*str)++;
					}
				}
				if(!ParseLexem(str, lex_semicolon))
					ThrowError((*str)->pos, "ERROR: ';' not found after class member list");
			}
		}
		if(!ParseLexem(str, lex_cfigure))
			ThrowError((*str)->pos, "ERROR: '}' not found after class definition");
		CALLBACK(TypeFinish());
		return true;
	}
	*str = curr;
	return false;
}

unsigned int ParseFunctionArguments(Lexeme** str)
{
	unsigned callArgCount = 0;
	unsigned lastArgument = SetCurrentArgument(callArgCount);
	if(ParseVaribleSet(str))
	{
		callArgCount++;
		while(ParseLexem(str, lex_comma))
		{
			SetCurrentArgument(callArgCount);
			if(!ParseVaribleSet(str))
				ThrowError((*str)->pos, "ERROR: expression not found after ',' in function parameter list");
			callArgCount++;
		}
	}
	SetCurrentArgument(lastArgument);
	if(!ParseLexem(str, lex_cparen))
		ThrowError((*str)->pos, "ERROR: ')' not found after function parameter list");
	return callArgCount;
}

bool ParseFunctionCall(Lexeme** str, bool memberFunctionCall)
{
	if((*str)->type != lex_string || (*str)[1].type != lex_oparen)
		return false;

	if((*str)->length >= NULLC_MAX_VARIABLE_NAME_LENGTH)
		ThrowError((*str)->pos, "ERROR: function name length is limited to 2048 symbols");
	char	*functionName = (char*)stringPool.Allocate((*str)->length+1);
	memcpy(functionName, (*str)->pos, (*str)->length);
	functionName[(*str)->length] = 0;
	(*str) += 2;

	// PrepareMemberCall may signal that this is not actually a member function call
	bool wasMemberCall = memberFunctionCall;
	// Prepare member function call
	if(memberFunctionCall)
		memberFunctionCall = PrepareMemberCall((*str)->pos, functionName);

	// If it was a member function call, but isn't now, then we should take function pointer from the top of node list
	NodeZeroOP *fAddress = NULL;
	if(!memberFunctionCall && wasMemberCall)
	{
		fAddress = CodeInfo::nodeList.back();
		CodeInfo::nodeList.pop_back();
	}
	const char *last = SetCurrentFunction(functionName);
	// Parse function arguments
	unsigned int callArgCount = ParseFunctionArguments(str);
	SetCurrentFunction(last);

	if(memberFunctionCall)
	{
		AddMemberFunctionCall((*str)->pos, functionName, callArgCount);
	}else{
		// If it was a member function call, but isn't now, push node that calculates function pointer
		if(wasMemberCall)
			CodeInfo::nodeList.push_back(fAddress);
		AddFunctionCallNode((*str)->pos, wasMemberCall ? NULL : functionName, callArgCount);
	}

	return true;
}

bool ParseGenericType(Lexeme** str)
{
	if(!ParseLexem(str, lex_generic))
		return false;
	SelectTypeByPointer(typeGeneric);
	FunctionGeneric(true);
	if(ParseLexem(str, lex_ref))
		SelectTypeByPointer(CodeInfo::GetReferenceType(typeGeneric));
	return true;
}

bool ParseFunctionVariables(Lexeme** str, unsigned nodeOffset)
{
	bool genericArg = false;
	if(!ParseSelectType(str, true, true) && false == (genericArg = ParseGenericType(str)))
		return true;

	unsigned argID = 0;
	if(genericArg && nodeOffset)
	{
		TypeInfo *curr = GetSelectedType();
		SelectTypeForGeneric((*str)->pos, nodeOffset - 1 + argID);
		if(curr->refLevel && !GetSelectedType()->refLevel)
			SelectTypeByPointer(CodeInfo::GetReferenceType(GetSelectedType()));
	}

	if((*str)->type != lex_string)
		ThrowError((*str)->pos, "ERROR: variable name not found after type in function variable list");

	if((*str)->length >= NULLC_MAX_VARIABLE_NAME_LENGTH)
		ThrowError((*str)->pos, "ERROR: parameter name length is limited to 2048 symbols");
	CALLBACK(FunctionParameter((*str)->pos, InplaceStr((*str)->pos, (*str)->length)));
	(*str)++;

	if(ParseLexem(str, lex_set))
	{
		if(FunctionGeneric(false))
			ThrowError((*str)->pos, "ERROR: default argument values are unsupported in generic functions");
		FunctionPrepareDefault();
		if(!ParseTernaryExpr(str))
			ThrowError((*str)->pos, "ERROR: default parameter value not found after '='");
		CALLBACK(FunctionParameterDefault((*str)->pos));
	}

	while(ParseLexem(str, lex_comma))
	{
		argID++;
		bool lastGeneric = genericArg;
		genericArg = false;
		if(!ParseSelectType(str, true, true))
		{
			ParseGenericType(str);
			genericArg = lastGeneric; // if there is no type and no generic, then this parameter is as generic as the last one
		}
		if(genericArg && nodeOffset)
		{
			TypeInfo *curr = GetSelectedType();
			SelectTypeForGeneric((*str)->pos, nodeOffset - 1 + argID);
			if(curr->refLevel && !GetSelectedType()->refLevel)
				SelectTypeByPointer(CodeInfo::GetReferenceType(GetSelectedType()));
		}

		if((*str)->type != lex_string)
			ThrowError((*str)->pos, "ERROR: variable name not found after type in function variable list");
		if((*str)->length >= NULLC_MAX_VARIABLE_NAME_LENGTH)
			ThrowError((*str)->pos, "ERROR: parameter name length is limited to 2048 symbols");
		CALLBACK(FunctionParameter((*str)->pos, InplaceStr((*str)->pos, (*str)->length)));
		(*str)++;
		
		if(ParseLexem(str, lex_set))
		{
			if(FunctionGeneric(false))
				ThrowError((*str)->pos, "ERROR: default argument values are unsupported in generic functions");
			FunctionPrepareDefault();
			if(!ParseTernaryExpr(str))
				ThrowError((*str)->pos, "ERROR: default parameter value not found after '='");
			CALLBACK(FunctionParameterDefault((*str)->pos));
		}
	}
	return true;
}

bool ParseFunctionConstraints(Lexeme** str, bool instanceTime)
{
	Lexeme *start = *str;
	if((*str)->type == lex_string && (*str)->length == 5 && memcmp((*str)->pos, "where", 5) == 0)
	{
		(*str)++;
		if(!ParseTernaryExpr(str))
			ThrowError((*str)->pos, "ERROR: expression expected after 'where'");
		if(instanceTime || !FunctionGeneric(false))
		{
			if(CodeInfo::nodeList.back()->nodeType != typeNodeNumber)
				ThrowError(start->pos, "ERROR: couldn't evaluate condition at compilation time");
			int result = ((NodeNumber*)CodeInfo::nodeList.back())->GetInteger();
			if(!result)
				ThrowError(start->pos, "ERROR: function constraints are not satisfied");
		}
		CodeInfo::nodeList.pop_back();
	}
	return true;
}

bool ParseFunctionDefinition(Lexeme** str, bool coroutine)
{
	Lexeme *start = *str - 1;
	Lexeme *name = *str;

	if((*str)->type != lex_string && (*str)->type != lex_operator && !((*str)->type == lex_oparen && (*str - 1)->type == lex_auto))
	{
		if(coroutine)
			ThrowError((*str)->pos, "ERROR: function name not found after return type");
		else
			return false;
	}
	bool typeMethod = false;
	bool funcProperty = false;
	if((*str)->type == lex_operator)
	{
		(*str)++;
		if((*str)->type == lex_obracket)
		{
			if((*str)[1].type != lex_cbracket)
				ThrowError((*str)->pos, "ERROR: ']' not found after '[' in operator definition");
			else
				(*str)++;
		}
		if((*str)->type == lex_oparen)
		{
			if((*str)[1].type != lex_cparen)
				ThrowError((*str)->pos, "ERROR: ')' not found after '(' in operator definition");
			else
				(*str)++;
		}
	}else if((*str)->type == lex_string && ((*str + 1)->type == lex_colon || (*str + 1)->type == lex_point)){
		TypeInfo *retType = (TypeInfo*)CALLBACK(GetSelectedType());
		if(!ParseSelectType(str))
			ThrowError((*str)->pos, "ERROR: class name expected before ':' or '.'");
		if((*str)->type == lex_point)
			funcProperty = true;
		(*str)++;
		if((*str)->type != lex_string)
			ThrowError((*str)->pos, "ERROR: function name expected after ':' or '.'");
		CALLBACK(TypeContinue((*str)->pos));
		CALLBACK(SelectTypeByPointer(retType));
		typeMethod = true;
	}
	char	*functionName = NULL;
	if((*str)->type == lex_string || ((*str)->type >= lex_add && (*str)->type <= lex_logxor) || ((*str)->type >= lex_set && (*str)->type <= lex_powset) || (*str)->type == lex_bitnot || (*str)->type == lex_lognot)
	{
		if((*str)->length >= NULLC_MAX_VARIABLE_NAME_LENGTH)
			ThrowError((*str)->pos, "ERROR: function name length is limited to 2048 symbols");
		functionName = (char*)stringPool.Allocate((*str)->length + 2);
		memcpy(functionName, (*str)->pos, (*str)->length);
		if(!funcProperty)
		{
			functionName[(*str)->length] = 0;
		}else{
			functionName[(*str)->length] = '$';
			functionName[(*str)->length + 1] = 0;
		}
		(*str)++;
	}else if((*str)->type == lex_cbracket){
		functionName = (char*)stringPool.Allocate(16);
		sprintf(functionName, "[]");
		(*str)++;
	}else if((*str)->type == lex_cparen){
		functionName = (char*)stringPool.Allocate(16);
		sprintf(functionName, "()");
		(*str)++;
	}else{
		static int unnamedFuncCount = 0;
		functionName = (char*)stringPool.Allocate(16);
		sprintf(functionName, "$func%d", unnamedFuncCount);
		unnamedFuncCount++;
	}

	if((*str)->type != lex_oparen)
	{
		*str = name;
		return false;
	}
	(*str)++;

	CALLBACK(FunctionAdd((*str)->pos, functionName));

	Lexeme *vars = *str;
	ParseFunctionVariables(str);

	if(!ParseLexem(str, lex_cparen))
		ThrowError((*str)->pos, "ERROR: ')' not found after function variable list");

	ParseFunctionConstraints(str, false);

	if(ParseLexem(str, lex_semicolon))
	{
		if(FunctionGeneric(false))
			ThrowError((*str)->pos, "ERROR: generic function cannot be forward-declared");
		if((name[1].type >= lex_add && name[1].type <= lex_logxor) || name[1].type == lex_obracket || (name[1].type >= lex_set && name[1].type <= lex_powset) || name[1].type == lex_bitnot || name[1].type == lex_lognot)
			CALLBACK(FunctionToOperator(start->pos));
		CALLBACK(FunctionPrototype(start->pos));
		if(typeMethod)
			CALLBACK(TypeStop());
		return true;
	}

	if(FunctionGeneric(false))
	{
		if(!ParseLexem(str, lex_ofigure))
			ThrowError((*str)->pos, "ERROR: '{' not found after function header");
		FunctionGeneric(true, unsigned(vars - CodeInfo::lexStart));
		FunctionPrototype(start->pos);
		unsigned braces = 1;
		while(braces)
		{
			if((*str)->type == lex_none)
				ThrowError((*str)->pos, "ERROR: unknown lexeme in function body");
			if(ParseLexem(str, lex_ofigure))
				braces++;
			else if(ParseLexem(str, lex_cfigure))
				braces--;
			else
				(*str)++;
		}
	}else{
		CALLBACK(FunctionStart((*str-1)->pos));
		if(!ParseLexem(str, lex_ofigure))
			ThrowError((*str)->pos, "ERROR: '{' not found after function header");
		const char *lastFunc = SetCurrentFunction(NULL);

		if(!ParseCode(str))
			CALLBACK(AddVoidNode());
		if(!ParseLexem(str, lex_cfigure))
			ThrowError((*str)->pos, "ERROR: '}' not found after function body");
		SetCurrentFunction(lastFunc);

		if((name[1].type >= lex_add && name[1].type <= lex_logxor) || name[1].type == lex_obracket || (name[1].type >= lex_set && name[1].type <= lex_powset) || name[1].type == lex_bitnot || name[1].type == lex_lognot)
			CALLBACK(FunctionToOperator(start->pos));

		CALLBACK(FunctionEnd(start->pos));
	}

	if(typeMethod)
		CALLBACK(TypeStop());
	return true;
}

bool ParseShortFunctionDefinition(Lexeme** str)
{
	if(!ParseLexem(str, lex_less))
		return true;

	// Save argument starting position
	Lexeme *start = *str;
	unsigned arguments = 0;
	// parse argument count
	if((*str)->type != lex_greater)
	{
		ParseSelectType(str);
		if(!ParseLexem(str, lex_string))
			ThrowError((*str)->pos, "ERROR: function argument name not found after '<'");
		arguments++;
		while(ParseLexem(str, lex_comma))
		{
			ParseSelectType(str);
			if(!ParseLexem(str, lex_string))
				ThrowError((*str)->pos, "ERROR: function argument name not found after ','");
			arguments++;
		}
	}
	// Restore argument starting position
	*str = start;
	// Get function type
	TypeInfo *type = (TypeInfo*)GetCurrentArgumentType((*str)->pos, arguments);

	static int unnamedFuncCount = 0;
	char *functionName = (char*)stringPool.Allocate(16);
	sprintf(functionName, "$funcs%d", unnamedFuncCount);
	unnamedFuncCount++;

	SelectTypeByPointer(type->funcType->retType);
	FunctionAdd((*str)->pos, functionName);

	for(unsigned currArg = 0; currArg < arguments; currArg++)
	{
		if(currArg != 0)
			ParseLexem(str, lex_comma);
		bool imaginary = ParseSelectType(str);
		TypeInfo *selType = (TypeInfo*)GetSelectedType();
		SelectTypeByPointer(type->funcType->paramType[currArg]);
		if((*str)->length >= NULLC_MAX_VARIABLE_NAME_LENGTH)
			ThrowError((*str)->pos, "ERROR: parameter name length is limited to 2048 symbols");
		if(!imaginary || type->funcType->paramType[currArg] == selType)
		{
			FunctionParameter((*str)->pos, InplaceStr((*str)->pos, (*str)->length));
		}else{
			char *paramName = (char*)stringPool.Allocate((*str)->length + 2);
			paramName[0] = '$';
			memcpy(paramName + 1, (*str)->pos, (*str)->length);
			paramName[(*str)->length + 1] = 0;
			FunctionParameter((*str)->pos, InplaceStr(paramName, (*str)->length + 1));
		}
		(*str)++;
	}
	
	if(!ParseLexem(str, lex_greater))
		ThrowError((*str)->pos, "ERROR: '>' expected after short inline function argument list");
	
	FunctionStart((*str)->pos);

	Lexeme *curr = *str;
	*str = start;
	unsigned wraps = 0;
	for(unsigned currArg = 0; currArg < arguments; currArg++)
	{
		if(currArg != 0)
			ParseLexem(str, lex_comma);
		if(ParseSelectType(str) && type->funcType->paramType[currArg] != GetSelectedType())
		{
			Lexeme *varName = *str;
			VariableInfo *varInfo = AddVariable((*str)->pos, InplaceStr(varName->pos, varName->length));
			(*str)++;
			
			char *paramName = (char*)stringPool.Allocate(varName->length + 2);
			paramName[0] = '$';
			memcpy(paramName + 1, varName->pos, varName->length);
			paramName[varName->length + 1] = 0;
			AddGetAddressNode((*str)->pos, InplaceStr(paramName, varName->length + 1));
			AddGetVariableNode((*str)->pos);
			AddDefineVariableNode((*str)->pos, varInfo);
			AddPopNode((*str)->pos);
			wraps++;
		}
		ParseLexem(str, lex_string);
	}
	*str = curr;

	if(!ParseLexem(str, lex_ofigure))
		ThrowError((*str)->pos, "ERROR: '{' not found after function header");
	const char *lastFunc = SetCurrentFunction(NULL);

	if(!ParseCode(str))
		AddVoidNode();
	while(wraps--)
		AddTwoExpressionNode();
	if(!ParseLexem(str, lex_cfigure))
		ThrowError((*str)->pos, "ERROR: '}' not found after function body");
	SetCurrentFunction(lastFunc);

	InlineFunctionImplicitReturn((*str)->pos);
	FunctionEnd(start->pos);

	return true;
}

bool ParseAddVariable(Lexeme** str)
{
	if((*str)->type != lex_string)
		return false;

	if((*str)->length >= NULLC_MAX_VARIABLE_NAME_LENGTH)
		ThrowError((*str)->pos, "ERROR: variable name length is limited to 2048 symbols");

	Lexeme *varName = *str;
	(*str)++;

	if(ParseLexem(str, lex_obracket))
		ThrowError((*str)->pos, "ERROR: array size must be specified after typename");

	VariableInfo *varInfo = AddVariable((*str)->pos, InplaceStr(varName->pos, varName->length));

	if(ParseLexem(str, lex_set))
	{
		if(!ParseVaribleSet(str))
			ThrowError((*str)->pos, "ERROR: expression not found after '='");
		AddDefineVariableNode((*str)->pos, varInfo);
		AddPopNode((*str)->pos);
	}else{
		AddVariableReserveNode((*str)->pos);
	}
	return true;
}

bool ParseVariableDefineSub(Lexeme** str)
{
	TypeInfo* currType = GetSelectedType();
	if(!ParseAddVariable(str))
		ThrowError((*str)->pos, "ERROR: unexpected symbol '%.*s' after type name. Variable name is expected at this point", (*str)->length, (*str)->pos);

	while(ParseLexem(str, lex_comma))
	{
		SelectTypeByPointer(currType);
		if(!ParseAddVariable(str))
			ThrowError((*str)->pos, "ERROR: next variable definition excepted after ','");
		CALLBACK(AddTwoExpressionNode());
	}
	return true;
}

bool ParseAlignment(Lexeme** str)
{
	if(ParseLexem(str, lex_noalign))
	{
		CALLBACK(SetCurrentAlignment(0));
		return true;
	}else if(ParseLexem(str, lex_align))
	{
		if(!ParseLexem(str, lex_oparen))
			ThrowError((*str)->pos, "ERROR: '(' expected after align");
		
		const char *start = (*str)->pos;
		if(!ParseLexem(str, lex_number))
			ThrowError((*str)->pos, "ERROR: alignment value not found after align(");
		CALLBACK(SetCurrentAlignment(atoi(start)));
		if(!ParseLexem(str, lex_cparen))
			ThrowError((*str)->pos, "ERROR: ')' expected after alignment value");
		return true;
	}
	return false;
}

bool ParseVariableDefine(Lexeme** str)
{
	Lexeme *curr = *str;
	CALLBACK(SetCurrentAlignment(0xFFFFFFFF));
	ParseAlignment(&curr);
	if(!ParseSelectType(&curr))
		return false;
	if(!ParseVariableDefineSub(&curr))
		return false;
	*str = curr;
	return true;
}

bool ParseIfExpr(Lexeme** str, bool isStatic)
{
	if(!ParseLexem(str, lex_if))
		return false;

	if(!ParseLexem(str, lex_oparen))
		ThrowError((*str)->pos, "ERROR: '(' not found after 'if'");
	const char *condPos = (*str)->pos;
	if(!ParseVaribleSet(str))
		ThrowError((*str)->pos, "ERROR: condition not found in 'if' statement");
	if(!ParseLexem(str, lex_cparen))
		ThrowError((*str)->pos, "ERROR: closing ')' not found after 'if' condition");
	if(CodeInfo::nodeList.back()->nodeType == typeNodeNumber && isStatic)
	{
		int result = ((NodeNumber*)CodeInfo::nodeList.back())->GetInteger();
		CodeInfo::nodeList.pop_back();
		if(!result)
		{
			unsigned startBraces = ParseLexem(str, lex_ofigure), braces = startBraces;
			for(;;)
			{
				if((*str)->type == lex_none)
					ThrowError((*str)->pos, "ERROR: unknown lexeme in 'if' body");
				if(ParseLexem(str, lex_ofigure))
				{
					braces++;
				}else if(ParseLexem(str, lex_cfigure)){
					if(!--braces && startBraces)
						break;
				}else if(ParseLexem(str, lex_semicolon)){
					if(!braces)
						break;
				}else{
					(*str)++;
				}
			}
			if(ParseLexem(str, lex_else))
			{
				if(!((*str)->type == lex_if ? ParseIfExpr(str, true) : ParseExpression(str)))
					ThrowError((*str)->pos, "ERROR: expression not found after 'else'");
			}else{
				AddVoidNode();
			}
		}else{
			if(!ParseExpression(str))
				ThrowError((*str)->pos, "ERROR: expression not found after 'if'");
			if(ParseLexem(str, lex_else))
			{
				unsigned startBraces = ParseLexem(str, lex_ofigure), braces = startBraces;
				for(;;)
				{
					if((*str)->type == lex_none)
						ThrowError((*str)->pos, "ERROR: unknown lexeme in 'else' body");
					if(ParseLexem(str, lex_ofigure))
					{
						braces++;
					}else if(ParseLexem(str, lex_cfigure)){
						if(!--braces && startBraces)
							break;
					}else if(ParseLexem(str, lex_semicolon)){
						if(!braces)
							break;
					}else{
						(*str)++;
					}
				}
			}
		}
		return true;
	}
	if(CodeInfo::nodeList.back()->nodeType != typeNodeNumber && isStatic)
		ThrowError((*str)->pos, "ERROR: couldn't evaluate condition at compilation time");
	if(!ParseExpression(str))
		ThrowError((*str)->pos, "ERROR: expression not found after 'if'");

	if(ParseLexem(str, lex_else))
	{
		if(!ParseExpression(str))
			ThrowError((*str)->pos, "ERROR: expression not found after 'else'");
		CALLBACK(AddIfElseNode(condPos));
	}else{
		CALLBACK(AddIfNode(condPos));
	}
	return true;
}

bool ParseForExpr(Lexeme** str)
{
	if(!ParseLexem(str, lex_for))
		return false;
	
	CALLBACK(IncreaseCycleDepth());
	
	if(!ParseLexem(str, lex_oparen))
		ThrowError((*str)->pos, "ERROR: '(' not found after 'for'");
	
	CALLBACK(BeginBlock());

	bool	isForEach = false;
	const char *condPos = NULL;
	Lexeme *curr = *str;
	if((curr + 1)->type == lex_in || (ParseSelectType(&curr) && (curr + 1)->type == lex_in))
	{
		isForEach = true;

		TypeInfo *type = (*str + 1)->type == lex_in ? NULL : GetSelectedType();
		*str = curr;
		if((*str)->type != lex_string)
			ThrowError((*str)->pos, "ERROR: variable name expected before 'in'");
		if((*str)->length >= NULLC_MAX_VARIABLE_NAME_LENGTH)
			ThrowError((*str)->pos, "ERROR: variable name length is limited to 2048 symbols");
		Lexeme	*varName = *str;
		condPos = (*str)->pos;
		(*str) += 2;	// Skip name and 'in'

		// Parse expression
		if(!ParseTernaryExpr(str))
			ThrowError((*str)->pos, "ERROR: expression expected after 'in'");
		AddArrayIterator(varName->pos, InplaceStr(varName->pos, varName->length), type);

		while(ParseLexem(str, lex_comma))
		{
			TypeInfo *type = NULL;
			if(ParseSelectType(str))
				type = GetSelectedType();

			if((*str)->type != lex_string)
				ThrowError((*str)->pos, "ERROR: variable name expected before 'in'");
			if((*str)->length >= NULLC_MAX_VARIABLE_NAME_LENGTH)
				ThrowError((*str)->pos, "ERROR: variable name length is limited to 2048 symbols");
			Lexeme	*varName = *str;
			condPos = (*str)->pos;
			(*str)++;
			if(!ParseLexem(str, lex_in))
				ThrowError((*str)->pos, "ERROR: 'in' expected after variable name");

			// Parse expression
			if(!ParseTernaryExpr(str))
				ThrowError((*str)->pos, "ERROR: expression expected after 'in'");
			AddArrayIterator(varName->pos, InplaceStr(varName->pos, varName->length), type);
			MergeArrayIterators();
		}
	}else{
		if(ParseLexem(str, lex_ofigure))
		{
			if(!ParseCode(str))
				CALLBACK(AddVoidNode());
			if(!ParseLexem(str, lex_cfigure))
				ThrowError((*str)->pos, "ERROR: '}' not found after '{'");
		}else{
			if(!ParseVariableDefine(str))
			{
				if(!ParseVaribleSet(str))
					CALLBACK(AddVoidNode());
				else
					CALLBACK(AddPopNode((*str)->pos));
			}
		}

		if(!ParseLexem(str, lex_semicolon))
			ThrowError((*str)->pos, "ERROR: ';' not found after initializer in 'for'");

		condPos = (*str)->pos;
		if(!ParseVaribleSet(str))
			ThrowError((*str)->pos, "ERROR: condition not found in 'for' statement");

		if(!ParseLexem(str, lex_semicolon))
			ThrowError((*str)->pos, "ERROR: ';' not found after condition in 'for'");

		if(ParseLexem(str, lex_ofigure))
		{
			if(!ParseCode(str))
				CALLBACK(AddVoidNode());
			if(!ParseLexem(str, lex_cfigure))
				ThrowError((*str)->pos, "ERROR: '}' not found after '{'");
		}else{
			if(!ParseVaribleSet(str))
				CALLBACK(AddVoidNode());
			else
				CALLBACK(AddPopNode((*str)->pos));
		}
	}

	if(!ParseLexem(str, lex_cparen))
		ThrowError((*str)->pos, "ERROR: ')' not found after 'for' statement");

	if(ParseLexem(str, lex_ofigure))
	{
		if(!ParseCode(str))
			CALLBACK(AddVoidNode());
		if(!ParseLexem(str, lex_cfigure))
			ThrowError((*str)->pos, "ERROR: closing '}' not found");
	}else if(!ParseExpression(str)){
		ThrowError((*str)->pos, "ERROR: body not found after 'for' header");
	}

	EndBlock();
	if(isForEach)
		AddForEachNode(condPos);
	else
		AddForNode(condPos);
	return true;
}

bool ParseWhileExpr(Lexeme** str)
{
	if(!ParseLexem(str, lex_while))
		return false;
	
	IncreaseCycleDepth();
	if(!ParseLexem(str, lex_oparen))
		ThrowError((*str)->pos, "ERROR: '(' not found after 'while'");

	const char *condPos = (*str)->pos;
	if(!ParseVaribleSet(str))
		ThrowError((*str)->pos, "ERROR: expression expected after 'while('");
	if(!ParseLexem(str, lex_cparen))
		ThrowError((*str)->pos, "ERROR: closing ')' not found after expression in 'while' statement");

	if(!ParseExpression(str))
	{
		if(!ParseLexem(str, lex_semicolon))
			ThrowError((*str)->pos, "ERROR: expression or ';' expected after 'while(...)'");
		AddVoidNode();
	}
	AddWhileNode(condPos);
	return true;
}

bool ParseDoWhileExpr(Lexeme** str)
{
	if(!ParseLexem(str, lex_do))
		return false;

	CALLBACK(IncreaseCycleDepth());

	if(!ParseExpression(str))
		ThrowError((*str)->pos, "ERROR: expression expected after 'do'");

	if(!ParseLexem(str, lex_while))
		ThrowError((*str)->pos, "ERROR: 'while' expected after 'do' statement");
	if(!ParseLexem(str, lex_oparen))
		ThrowError((*str)->pos, "ERROR: '(' not found after 'while'");

	const char *condPos = (*str)->pos;
	if(!ParseVaribleSet(str))
		ThrowError((*str)->pos, "ERROR: expression expected after 'while('");
	if(!ParseLexem(str, lex_cparen))
		ThrowError((*str)->pos, "ERROR: closing ')' not found after expression in 'while' statement");

	CALLBACK(AddDoWhileNode(condPos));

	if(!ParseLexem(str, lex_semicolon))
		ThrowError((*str)->pos, "ERROR: while(...) should be followed by ';'");
	return true;
}

bool ParseSwitchExpr(Lexeme** str)
{
	if(!ParseLexem(str, lex_switch))
		return false;

	if(!ParseLexem(str, lex_oparen))
		ThrowError((*str)->pos, "ERROR: '(' not found after 'switch'");

	const char *condPos = (*str)->pos;
	if(!ParseVaribleSet(str))
		ThrowError((*str)->pos, "ERROR: expression not found after 'switch('");
	CALLBACK(BeginSwitch(condPos));

	if(!ParseLexem(str, lex_cparen))
		ThrowError((*str)->pos, "ERROR: closing ')' not found after expression in 'switch' statement");

	if(!ParseLexem(str, lex_ofigure))
		ThrowError((*str)->pos, "ERROR: '{' not found after 'switch(...)'");

	while(ParseLexem(str, lex_case) || ParseLexem(str, lex_default))
	{
		Lexeme *condPos = *str;
		if(condPos[-1].type == lex_case && !ParseVaribleSet(str))
			ThrowError((*str)->pos, "ERROR: expression expected after 'case' of 'default'");
		if(!ParseLexem(str, lex_colon))
			ThrowError((*str)->pos, "ERROR: ':' expected");

		if(!ParseExpression(str))
			CALLBACK(AddVoidNode());
		while(ParseExpression(str))
			CALLBACK(AddTwoExpressionNode());
		if(condPos[-1].type == lex_default)
			CALLBACK(AddDefaultNode());
		else
			CALLBACK(AddCaseNode(condPos->pos));
	}

	if(!ParseLexem(str, lex_cfigure))
		ThrowError((*str)->pos, "ERROR: '}' not found after 'switch' statement");
	CALLBACK(EndSwitch());
	return true;
}

bool ParseReturnExpr(Lexeme** str, bool yield)
{
	const char* start = (*str)->pos;
	if(!(yield ? ParseLexem(str, lex_yield) : ParseLexem(str, lex_return)))
		return false;

	if(!ParseVaribleSet(str))
		AddVoidNode();

	if(!ParseLexem(str, lex_semicolon))
		ThrowError((*str)->pos, "ERROR: return statement must be followed by ';'");
	AddReturnNode(start, yield);
	return true;
}

bool ParseBreakExpr(Lexeme** str)
{
	const char *pos = (*str)->pos;
	if(!ParseLexem(str, lex_break))
		return false;

	if(!ParseTerminal(str))
		AddVoidNode();

	if(!ParseLexem(str, lex_semicolon))
		ThrowError((*str)->pos, "ERROR: break statement must be followed by ';'");
	CALLBACK(AddBreakNode(pos));
	return true;
}

bool ParseContinueExpr(Lexeme** str)
{
	const char *pos = (*str)->pos;
	if(!ParseLexem(str, lex_continue))
		return false;

	if(!ParseTerminal(str))
		AddVoidNode();

	if(!ParseLexem(str, lex_semicolon))
		ThrowError((*str)->pos, "ERROR: continue statement must be followed by ';'");
	CALLBACK(AddContinueNode(pos));
	return true;
}

bool ParseGroup(Lexeme** str)
{
	if(!ParseLexem(str, lex_oparen))
		return false;

	if(!ParseVaribleSet(str))
		ThrowError((*str)->pos, "ERROR: expression not found after '('");
	if(!ParseLexem(str, lex_cparen))
		ThrowError((*str)->pos, "ERROR: closing ')' not found after '('");

	if(ParseLexem(str, lex_dec))
	{
		UndoDereferceNode((*str)->pos);
		AddUnaryModifyOpNode((*str)->pos, OP_DECREMENT, OP_POSTFIX);
	}else if(ParseLexem(str, lex_inc)){
		UndoDereferceNode((*str)->pos);
		AddUnaryModifyOpNode((*str)->pos, OP_INCREMENT, OP_POSTFIX);
	}

	return true;
}

bool ParseString(Lexeme** str)
{
	bool	unescaped = false;
	if(ParseLexem(str, lex_at))
		unescaped = true;
	if((*str)->type != lex_quotedstring)
		return false;

	AddStringNode((*str)->pos, (*str)->pos+(*str)->length, unescaped);
	(*str)++;
	return true;
}

bool ParseArray(Lexeme** str)
{
	if(!ParseLexem(str, lex_ofigure))
		return false;

	if((*str)->type == lex_for)
	{
		// coroutine auto(){
		BeginCoroutine();
		SelectTypeByPointer(NULL);
		
		static int generatorFuncCount = 0;
		char *functionName = (char*)stringPool.Allocate(16);
		sprintf(functionName, "$genl%d", generatorFuncCount);
		generatorFuncCount++;
		FunctionAdd((*str)->pos, functionName);
		FunctionStart((*str)->pos);
		if(!ParseCode(str))
			AddVoidNode();

		AddGeneratorReturnData((*str)->pos);
		TypeInfo *retType = GetSelectedType();
		AddReturnNode((*str)->pos);
		AddTwoExpressionNode();

		FunctionEnd((*str)->pos);

		if(!AddFunctionCallNode((*str)->pos, "$gen_list", 1, true))
		{
			// cannot find generator, create new
			NodeZeroOP *last = CodeInfo::nodeList.back();
			CodeInfo::nodeList.pop_back();
			AddListGenerator((*str)->pos, retType);
			CodeInfo::nodeList.push_back(last);
			AddFunctionCallNode((*str)->pos, "$gen_list", 1);
			AddTwoExpressionNode(CodeInfo::GetArrayType((TypeInfo*)retType, TypeInfo::UNSIZED_ARRAY));
		}
		
	}else{
		unsigned int arrElementCount = 0;
		if(!ParseTernaryExpr(str))
			ThrowError((*str)->pos, "ERROR: value not found after '{'");
		while(ParseLexem(str, lex_comma))
		{
			if(!ParseTernaryExpr(str))
				ThrowError((*str)->pos, "ERROR: value not found after ','");
			arrElementCount++;
		}
		AddArrayConstructor((*str)->pos, arrElementCount);
	}
	if(!ParseLexem(str, lex_cfigure))
		ThrowError((*str)->pos, "ERROR: '}' not found after inline array");

	return true;
}

bool ParseVariable(Lexeme** str, bool *lastIsFunctionCall = NULL)
{
	if(ParseLexem(str, lex_mul))
	{
		if(!ParseTerminal(str))
			ThrowError((*str)->pos, "ERROR: variable name not found after '*'");
		AddGetVariableNode((*str)->pos, true);
		return true;
	}
	
	if((*str)->type != lex_string || (*str)[1].type == lex_oparen)
		return false;

	if((*str)->length >= NULLC_MAX_VARIABLE_NAME_LENGTH)
		ThrowError((*str)->pos, "ERROR: variable name length is limited to 2048 symbols");
	CALLBACK(AddGetAddressNode((*str)->pos, InplaceStr((*str)->pos, (*str)->length)));
	(*str)++;

	bool isFuncCall = false;
	while(ParsePostExpression(str, &isFuncCall));
	if(lastIsFunctionCall)
		*lastIsFunctionCall = isFuncCall;
	return true;
}

bool ParsePostExpression(Lexeme** str, bool *isFunctionCall = NULL)
{
	if(ParseLexem(str, lex_point))
	{
		if((*str)->type != lex_string)
			ThrowError((*str)->pos, "ERROR: member variable expected after '.'");
		if(isFunctionCall)
			*isFunctionCall = (*str)[1].type == lex_oparen;
		if((*str)[1].type == lex_oparen)
		{
			if(!ParseFunctionCall(str, true))
				ThrowError((*str)->pos, "ERROR: function call is excepted after '.'");
		}else{
			if((*str)->length >= NULLC_MAX_VARIABLE_NAME_LENGTH)
				ThrowError((*str)->pos, "ERROR: variable name length is limited to 2048 symbols");
			CALLBACK(AddMemberAccessNode((*str)->pos, InplaceStr((*str)->pos, (*str)->length)));
			(*str)++;
		}
	}else if(ParseLexem(str, lex_obracket)){
		if(isFunctionCall)
			*isFunctionCall = false;
		if(!ParseVaribleSet(str))
			ThrowError((*str)->pos, "ERROR: expression not found after '['");
		if(!ParseLexem(str, lex_cbracket))
			ThrowError((*str)->pos, "ERROR: ']' not found after expression");
		CALLBACK(AddArrayIndexNode((*str)->pos));
	}else if(ParseLexem(str, lex_oparen)){
		if(isFunctionCall)
			*isFunctionCall = true;

		NodeZeroOP *fAddress = CodeInfo::nodeList.back();
		CodeInfo::nodeList.pop_back();

		const char *last = SetCurrentFunction(NULL);
		unsigned int callArgCount = ParseFunctionArguments(str);
		SetCurrentFunction(last);

		CodeInfo::nodeList.push_back(fAddress);
		CALLBACK(AddFunctionCallNode((*str)->pos, NULL, callArgCount));
	}else{
		return false;
	}
	return true;
}

void ParsePostExpressions(Lexeme** str)
{
	bool lastIsFunctionCall = false;
	bool hadPost = false;
	while(ParsePostExpression(str, &lastIsFunctionCall))
		hadPost = true;
	if(hadPost && !lastIsFunctionCall && (*str)->type != lex_set && (*str)->type != lex_addset && (*str)->type != lex_subset && (*str)->type != lex_mulset && (*str)->type != lex_divset && (*str)->type != lex_powset)
		CALLBACK(AddGetVariableNode((*str)->pos));
}

bool ParseTerminal(Lexeme** str)
{
	switch((*str)->type)
	{
	case lex_number:
		return ParseNumber(str);
		break;
	case lex_nullptr:
		(*str)++;
		AddNullPointer();
		return true;
		break;
	case lex_bitand:
		(*str)++;
		if(!ParseVariable(str))
			ThrowError((*str)->pos, "ERROR: variable not found after '&'");
		return true;
		break;
	case lex_lognot:
		(*str)++;
		if(!ParseTerminal(str))
			ThrowError((*str)->pos, "ERROR: expression not found after '!'");
		CALLBACK(AddLogNotNode((*str)->pos));
		return true;
		break;
	case lex_bitnot:
		(*str)++;
		if(!ParseTerminal(str))
			ThrowError((*str)->pos, "ERROR: expression not found after '~'");
		CALLBACK(AddBitNotNode((*str)->pos));
		return true;
		break;
	case lex_dec:
		(*str)++;
		if(!ParseVariable(str))
		{
			if(!ParseGroup(str))
				ThrowError((*str)->pos, "ERROR: variable not found after '--'");
			else
				UndoDereferceNode((*str)->pos);
		}
		AddUnaryModifyOpNode((*str)->pos, OP_DECREMENT, OP_PREFIX);
		return true;
		break;
	case lex_inc:
		(*str)++;
		if(!ParseVariable(str))
		{
			if(!ParseGroup(str))
				ThrowError((*str)->pos, "ERROR: variable not found after '++'");
			else
				UndoDereferceNode((*str)->pos);
		}
		AddUnaryModifyOpNode((*str)->pos, OP_INCREMENT, OP_PREFIX);
		return true;
		break;
	case lex_add:
		while(ParseLexem(str, lex_add));
		if(!ParseTerminal(str))
			ThrowError((*str)->pos, "ERROR: expression not found after '+'");
		AddPositiveNode((*str)->pos);
		return true;
		break;
	case lex_sub:
	{
		int negCount = 0;
		while(ParseLexem(str, lex_sub))
			negCount++;
		if(!ParseTerminal(str))
			ThrowError((*str)->pos, "ERROR: expression not found after '-'");
		if(negCount % 2 == 1)
			CALLBACK(AddNegateNode((*str)->pos));
		return true;
	}
		break;
	case lex_semiquotedchar:
		if(((*str)->length > 3 && (*str)->pos[1] != '\\') || (*str)->length > 4)
			ThrowError((*str)->pos, "ERROR: only one character can be inside single quotes");
		CALLBACK(AddNumberNodeChar((*str)->pos));
		(*str)++;
		return true;
		break;
	case lex_sizeof:
		(*str)++;
		if(!ParseLexem(str, lex_oparen))
			ThrowError((*str)->pos, "ERROR: sizeof must be followed by '('");
		if(ParseSelectType(str))
		{
			CALLBACK(GetTypeSize((*str)->pos, false));
		}else{
			if(ParseVaribleSet(str))
				CALLBACK(GetTypeSize((*str)->pos, true));
			else
				ThrowError((*str)->pos, "ERROR: expression or type not found after sizeof(");
		}
		if(!ParseLexem(str, lex_cparen))
			ThrowError((*str)->pos, "ERROR: ')' not found after expression in sizeof");
		return true;
		break;
	case lex_new:
	{
		(*str)++;

		if((*str)->type == lex_string && (*str + 1)->type == lex_oparen)
		{
			unsigned int index;
			if((index = ParseTypename(str)) == 0)
				ThrowError((*str)->pos, "ERROR: type name expected after 'new'");
			SelectTypeByIndex(index - 1);
			const char *name = GetSelectedTypeName();
			GetTypeSize((*str)->pos, false);
			AddTypeAllocation((*str)->pos);
			PrepareConstructorCall((*str)->pos);
			ParseLexem(str, lex_oparen);
			const char *last = SetCurrentFunction(name);
			unsigned int callArgCount = ParseFunctionArguments(str);
			SetCurrentFunction(last);
			AddMemberFunctionCall((*str)->pos, name, callArgCount);
			FinishConstructorCall((*str)->pos);
			return true;
		}

		const char *pos = (*str)->pos;
		if(!ParseSelectType(str, false))
			ThrowError((*str)->pos, "ERROR: type name expected after 'new'");

		CALLBACK(GetTypeSize((*str)->pos, false));

		if(ParseLexem(str, lex_obracket))
		{
			CALLBACK(AddUnfixedArraySize());
			CALLBACK(ConvertTypeToArray((*str)->pos));

			if(!ParseTernaryExpr(str))
				ThrowError((*str)->pos, "ERROR: expression not found after '['");
			if(!ParseLexem(str, lex_cbracket))
				ThrowError((*str)->pos, "ERROR: ']' not found after expression");
		}
		CALLBACK(AddTypeAllocation(pos));
		return true;
	}
		break;
	case lex_at:
		if((*str)[1].type != lex_quotedstring)
			ThrowError((*str)->pos, "ERROR: string expected after '@'");
	case lex_quotedstring:
		ParseString(str);
		ParsePostExpressions(str);
		return true;
		break;
	case lex_ofigure:
		ParseArray(str);
		ParsePostExpressions(str);
		return true;
		break;
	case lex_oparen:
		ParseGroup(str);
		ParsePostExpressions(str);
		return true;
		break;
	case lex_less:
		ParseShortFunctionDefinition(str);
		return true;
		break;
	case lex_string:
		if((*str + 1)->type == lex_oparen)
		{
			ParseFunctionCall(str, false);
			ParsePostExpressions(str);
			return true;
		}
	case lex_typeof:
	case lex_coroutine:
	case lex_auto:
	{
		bool isCoroutine = (*str)->type == lex_coroutine;
		if(isCoroutine)
		{
			BeginCoroutine();
			(*str)++;
		}
		bool isFunctionCall = !isCoroutine && (*str)->type != lex_typeof && (*str)->type != lex_auto && (*str + 1)->type == lex_oparen;
		if(!isFunctionCall && ParseSelectType(str))
		{
			if(ParseFunctionDefinition(str, isCoroutine))
				return true;
			if(ParseLexem(str, lex_oparen))
			{
				const char *last = SetCurrentFunction(GetSelectedTypeName());
				unsigned int callArgCount = ParseFunctionArguments(str);
				SetCurrentFunction(last);
				AddFunctionCallNode((*str)->pos, GetSelectedTypeName(), callArgCount);
				ParsePostExpressions(str);
				return true;
			}
			GetTypeId((*str)->pos);
			return true;
		}
	}
		break;
	}
	bool lastIsFunctionCall = false;
	if(((*str)->type == lex_string || (*str)->type == lex_mul) && ParseVariable(str, &lastIsFunctionCall))
	{
		if(ParseLexem(str, lex_dec))
		{
			AddUnaryModifyOpNode((*str)->pos, OP_DECREMENT, OP_POSTFIX);
		}else if(ParseLexem(str, lex_inc)){
			AddUnaryModifyOpNode((*str)->pos, OP_INCREMENT, OP_POSTFIX);
		}else{
			if(!lastIsFunctionCall && (*str)->type != lex_set && (*str)->type != lex_addset && (*str)->type != lex_subset && (*str)->type != lex_mulset && (*str)->type != lex_divset && (*str)->type != lex_powset)
				CALLBACK(AddGetVariableNode((*str)->pos));
		}
	}else{
		return false;
	}
	return true;
}

// operator stack
FastVector<LexemeType>	opStack;

bool ParseArithmetic(Lexeme** str)
{
	if(!ParseTerminal(str))
		return false;
	unsigned int opCount = 0;
	while((*str)->type >= lex_add && (*str)->type <= lex_logxor)
	{
		LexemeType lType = (*str)->type;
		while(opCount > 0 && opPrecedence[opStack.back() - lex_add] <= opPrecedence[lType - lex_add])
		{
			CALLBACK(AddBinaryCommandNode((*str)->pos, opHandler[opStack.back() - lex_add]));
			opStack.pop_back();
			opCount--;
		}
		opStack.push_back(lType);
		opCount++;	// opStack is global, but we are tracing its local size
		(*str)++;
		if(!ParseTerminal(str))
			ThrowError((*str)->pos, "ERROR: terminal expression not found after binary operation");
	}
	while(opCount > 0)
	{
		CALLBACK(AddBinaryCommandNode((*str)->pos, opHandler[opStack.back() - lex_add]));
		opStack.pop_back();
		opCount--;
	}
	return true;
}

bool ParseTernaryExpr(Lexeme** str)
{
	const char *condPos = (*str)->pos;
	if(!ParseArithmetic(str))
		return false;
	while(ParseLexem(str, lex_questionmark))
	{
		if(!ParseVaribleSet(str))
			ThrowError((*str)->pos, "ERROR: expression not found after '?'");
		if(!ParseLexem(str, lex_colon))
			ThrowError((*str)->pos, "ERROR: ':' not found after expression in ternary operator");
		if(!ParseVaribleSet(str))
			ThrowError((*str)->pos, "ERROR: expression not found after ':'");
		CALLBACK(AddIfElseTermNode(condPos));
	}
	return true;
}

bool ParseVaribleSet(Lexeme** str)
{
	if(!ParseTernaryExpr(str))
		return false;

	if(ParseLexem(str, lex_set))
	{
		if(ParseVaribleSet(str))
			CALLBACK(AddSetVariableNode((*str)->pos));
		else
			ThrowError((*str)->pos, "ERROR: expression not found after '='");
	}else if(ParseLexem(str, lex_addset) || ParseLexem(str, lex_subset) || ParseLexem(str, lex_mulset) || ParseLexem(str, lex_divset)){
		char op = (*str-1)->pos[0];
		if(ParseVaribleSet(str))
			CALLBACK(AddModifyVariableNode((*str)->pos, (CmdID)(op == '+' ? cmdAdd : (op == '-' ? cmdSub : (op == '*' ? cmdMul : cmdDiv)))));
		else
			ThrowError((*str)->pos, "ERROR: expression not found after assignment operator");
	}else if(ParseLexem(str, lex_powset)){
		if(ParseVaribleSet(str))
			CALLBACK(AddModifyVariableNode((*str)->pos, cmdPow));
		else
			ThrowError((*str)->pos, "ERROR: expression not found after '**='");
	}

	return true;
}

bool ParseBlock(Lexeme** str)
{
	if(!ParseLexem(str, lex_ofigure))
		return false;
	CALLBACK(BeginBlock());
	if(!ParseCode(str))
		CALLBACK(AddVoidNode());
	if(!ParseLexem(str, lex_cfigure))
		ThrowError((*str)->pos, "ERROR: closing '}' not found");
	CALLBACK(EndBlock());
	return true;
}

bool ParseTypedefExpr(Lexeme** str)
{
	if(!ParseLexem(str, lex_typedef))
		return false;
	if(!ParseSelectType(str))
		ThrowError((*str)->pos, "ERROR: typename expected after typedef");

	if(ParseSelectType(str))
		ThrowError((*str - 1)->pos, "ERROR: there is already a type or an alias with the same name");
	if((*str)->type != lex_string)
		ThrowError((*str)->pos, "ERROR: alias name expected after typename in typedef expression");

	if((*str)->length >= NULLC_MAX_VARIABLE_NAME_LENGTH)
		ThrowError((*str)->pos, "ERROR: alias name length is limited to 2048 symbols");
	CALLBACK(AddAliasType(InplaceStr((*str)->pos, (*str)->length)));
	(*str)++;
	if(!ParseLexem(str, lex_semicolon))
		ThrowError((*str)->pos, "ERROR: ';' not found after typedef");
	return true;
}

bool ParseExpression(Lexeme** str)
{
	switch((*str)->type)
	{
	case lex_align:
	case lex_noalign:
		if(ParseVariableDefine(str))
		{
			if(!ParseLexem(str, lex_semicolon))
				ThrowError((*str)->pos, "ERROR: ';' not found after variable definition");
			return true;
		}
	case lex_class:
		ParseClassDefinition(str);
		break;
	case lex_ofigure:
		ParseBlock(str);
		break;
	case lex_return:
		ParseReturnExpr(str);
		break;
	case lex_yield:
		ParseReturnExpr(str, true);
		break;
	case lex_break:
		ParseBreakExpr(str);
		break;
	case lex_continue:
		ParseContinueExpr(str);
		break;
	case lex_if:
		ParseIfExpr(str);
		break;
	case lex_for:
		ParseForExpr(str);
		break;
	case lex_while:
		ParseWhileExpr(str);
		break;
	case lex_do:
		ParseDoWhileExpr(str);
		break;
	case lex_switch:
		ParseSwitchExpr(str);
		break;
	case lex_typedef:
		ParseTypedefExpr(str);
		CALLBACK(AddVoidNode());
		break;
	case lex_coroutine:
		(*str)++;
		BeginCoroutine();
		if(!ParseSelectType(str))
			ThrowError((*str)->pos, "ERROR: function return type not found after 'coroutine'");
		if(!ParseFunctionDefinition(str, true))
			ThrowError((*str)->pos, "ERROR: '(' expected after function name");
		break;
	case lex_at:
		if((*str)[1].type == lex_if)
		{
			(*str)++;
			ParseIfExpr(str, true);
			return true;
		}
	default:
		if(ParseSelectType(str))
		{
			bool isVarDef = ((*str)->type == lex_string) && ((*str + 1)->type == lex_comma || (*str + 1)->type == lex_semicolon || (*str + 1)->type == lex_set);
			if(!isVarDef)
				if(ParseFunctionDefinition(str))
					return true;

			CALLBACK(SetCurrentAlignment(0xFFFFFFFF));
			ParseVariableDefineSub(str);
			if(!ParseLexem(str, lex_semicolon))
				ThrowError((*str)->pos, "ERROR: ';' not found after variable definition");
			return true;
		}
		if(ParseVaribleSet(str))
		{
			const char *pos = (*str)->pos;
			if(!ParseLexem(str, lex_semicolon))
				ThrowError((*str)->pos, "ERROR: ';' not found after expression");
			CALLBACK(AddPopNode(pos));
			return true;
		}
		return false;
	}
	return true;
}

bool ParseCode(Lexeme** str)
{
	if(!ParseExpression(str))
		return false;

	unsigned count = 0;
	while(ParseExpression(str))
		count++;

	AddOneExpressionNode();
	while(count--)
		AddTwoExpressionNode();

	return true;
}

void ParseReset()
{
	opStack.reset();
	stringPool.~ChunkedStackPool();
}
