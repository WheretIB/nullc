#include "Parser.h"
#include "CodeInfo.h"
using namespace CodeInfo;

#include "Callbacks.h"

ChunkedStackPool<4092>	stringPool;

// indexed by [lexeme - lex_add]
char opPrecedence[] = { 2, 2, 1, 1, 1, 0, 4, 4, 3, 4, 4, 3, 5, 5, 6, 8, 7, 9, 11, 10, 12 /* + - * / % ** < <= << > >= >> == != & | ^ and or xor in */ };
CmdID opHandler[] = { cmdAdd, cmdSub, cmdMul, cmdDiv, cmdMod, cmdPow, cmdLess, cmdLEqual, cmdShl, cmdGreater, cmdGEqual, cmdShr, cmdEqual, cmdNEqual, cmdBitAnd, cmdBitOr, cmdBitXor, cmdLogAnd, cmdLogOr, cmdLogXor, cmdNop };

char*	AllocateString(unsigned int size)
{
	return (char*)stringPool.Allocate(size);
}

void ClearStringList()
{
	stringPool.Clear();
}

char*	GetDefaultConstructorName(const char* name)
{
	unsigned length = (unsigned)strlen(name);
	char *tmp = AllocateString(length + 2);
	strcpy(tmp, name);
	tmp[length] = '$';
	tmp[length + 1] = 0;
	return tmp;
}

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

	TypeInfo *type = SelectTypeByName(InplaceStr((*str)->pos, (*str)->length));
	if(type)
	{
		(*str)++;
		return type->typeIndex + 1;
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
		AddHexInteger(number->pos, number->pos+number->length);
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
			AddBinInteger(number->pos, number->pos+number->length);
			return true;
		}else if((*str)->pos[0] == 'l'){
			(*str)++;
			AddNumberNodeLong(number->pos, number->pos+number->length);
			return true;
		}else if(number->pos[0] == '0' && isDigit(number->pos[1])){
			AddOctInteger(number->pos, number->pos+number->length);
			return true;
		}else{
			AddNumberNodeInt(number->pos);
			return true;
		}
	}else{
		if((*str)->pos[0] == 'f')
		{
			(*str)++;
			AddNumberNodeFloat(number->pos);
			return true;
		}else{
			AddNumberNodeDouble(number->pos);
			return true;
		}
	}
}

bool ParseArrayDefinition(Lexeme** str)
{
	if(!ParseLexem(str, lex_obracket))
		return false;

	TypeInfo *currType = GetSelectedType();

	ResetConstantFoldError();
	if((*str)->type == lex_cbracket)
	{
		(*str)++;
		AddUnfixedArraySize();
	}else{
		if(!ParseTernaryExpr(str))
			ThrowError((*str)->pos, "ERROR: unexpected expression after '['");
		if(!ParseLexem(str, lex_cbracket))
			ThrowError((*str)->pos, "ERROR: matching ']' not found");
	}
	ThrowConstantFoldError((*str)->pos);

	SelectTypeByPointer(currType);

	if((*str)->type == lex_obracket)
		ParseArrayDefinition(str);

	ConvertTypeToArray((*str)->pos);
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
			ThrowError(curr->pos, "ERROR: expected '.first'/'.last'/'[N]'/'.size' at this point");
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
			CodeInfo::nodeList.push_back(new NodeNumber(genericType ? 0 : (int)paramCount, typeInt));
			notType = true;
		}else{
			ThrowError(curr->pos, "ERROR: expected 'first'/'last'/'size' at this point");
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
		CodeInfo::nodeList.push_back(new NodeNumber(genericType ? 0 : (GetSelectedType()->refLevel ? 1 : 0), typeBool));
		notType = true;
	}else if(curr->type == lex_string && curr->length == 7 && memcmp(curr->pos, "isArray", 7) == 0){
		curr++;
		CodeInfo::nodeList.push_back(new NodeNumber(genericType ? 0 : (GetSelectedType()->arrLevel ? 1 : 0), typeBool));
		notType = true;
	}else if(curr->type == lex_string && curr->length == 10 && memcmp(curr->pos, "isFunction", 10) == 0){
		curr++;
		CodeInfo::nodeList.push_back(new NodeNumber(genericType ? 0 : (GetSelectedType()->funcType ? 1 : 0), typeBool));
		notType = true;
	}else if(curr->type == lex_string && curr->length == 9 && memcmp(curr->pos, "arraySize", 9) == 0){
		curr++;
		if(!genericType && !GetSelectedType()->arrLevel)
			ThrowError(curr->pos, "ERROR: 'arraySize' can only be applied to an array type, but we have '%s'", GetSelectedType()->GetFullTypeName());
		CodeInfo::nodeList.push_back(new NodeNumber(genericType ? 0 : ((int)GetSelectedType()->arrSize), typeInt));
		notType = true;
	}else if(curr->type == lex_string && curr->length == 9 && memcmp(curr->pos, "hasMember", 9) == 0){
		curr++;
		if(!ParseLexem(&curr, lex_oparen))
			ThrowError(curr->pos, "ERROR: expected '(' at this point");
		if(curr->type != lex_string)
			ThrowError(curr->pos, "ERROR: expected member name after '('");

		if(!genericType)
		{
			TypeInfo *classType = GetSelectedType();
			unsigned hash = GetStringHash(curr->pos, curr->pos + curr->length);

			TypeInfo::MemberVariable *currMember = classType->firstVariable;
			for(; currMember; currMember = currMember->next)
			{
				if(currMember->nameHash == hash && InplaceStr(currMember->name) == InplaceStr(curr->pos, curr->length))
					break;
			}
			CodeInfo::nodeList.push_back(new NodeNumber(currMember != NULL && currMember->defaultValue == NULL ? 1 : 0, typeBool));
			notType = true;
		}
		curr++;

		if(!ParseLexem(&curr, lex_cparen))
			ThrowError(curr->pos, "ERROR: expected ')' after member name");
	}else{
		if(curr->type == lex_string && !GetSelectedType()->refLevel && !GetSelectedType()->arrLevel && !GetSelectedType()->funcType) // if this is a class
		{
			if(!genericType)
			{
				TypeInfo *classType = GetSelectedType();
				unsigned hash = GetStringHash(curr->pos, curr->pos + curr->length);

				TypeInfo::MemberVariable *currMember = classType->firstVariable;
				for(; currMember; currMember = currMember->next)
				{
					if(currMember->nameHash == hash && InplaceStr(currMember->name) == InplaceStr(curr->pos, curr->length))
						break;
				}
				if(!currMember)
				{
					// Check local type aliases
					AliasInfo *alias = classType->childAlias;
					while(alias)
					{
						if(alias->nameHash == hash && InplaceStr(alias->name) == InplaceStr(curr->pos, curr->length))
							break;
						alias = alias->next;
					}
					if(!alias)
						ThrowError(curr->pos, "ERROR: expected extended typeof expression, class member name or class typedef at this point");
					SelectTypeByPointer(alias->type);
				}else{
					if(currMember->defaultValue)
					{
						assert(currMember->defaultValue->nodeType == typeNodeNumber);
						NodeNumber *nodeCopy = new NodeNumber(0, currMember->defaultValue->typeInfo);
						nodeCopy->num.integer64 = ((NodeNumber*)currMember->defaultValue)->num.integer64;
						CodeInfo::nodeList.push_back(nodeCopy);
						notType = true;
					}else{
						SelectTypeByPointer(currMember->type);
					}
				}
			}
			curr++;
		}else{
			ThrowError(curr->pos, "ERROR: expected extended typeof expression at this point");
		}
	}
	*str = curr;
	return true;
}

void ParseTypePostExpressions(Lexeme** str, bool allowArray, bool notType, bool allowAutoReturnType, bool allowGenericType, TypeInfo* instanceType, bool* instanceFailure)
{
	if(instanceType)
		assert(instanceFailure);

	bool run = true;
	while(run)
	{
		switch((*str)->type)
		{
		case lex_ref:
			(*str)++;
			if(notType)
				ThrowError((*str)->pos, "ERROR: typeof expression result is not a type");
			if(ParseLexem(str, lex_oparen) || (allowGenericType && ParseLexem(str, lex_generic)))
			{
				Lexeme *old = (*str) - 1;
				// Prepare function type
				TypeInfo *retType = (TypeInfo*)GetSelectedType();
				if(!retType && !allowAutoReturnType)
					ThrowError((*str)->pos, "ERROR: return type of a function type cannot be auto");
				if(instanceType && (!instanceType->funcType || instanceType->funcType->retType != retType))
				{
					*instanceFailure = true;
					return;
				}
				TypeInfo *preferredType = instanceType ? (instanceType->funcType->paramCount ? instanceType->funcType->paramType[0] : NULL) : NULL;
				TypeHandler *first = NULL, *handle = NULL;
				unsigned int count = 0;
				if(ParseSelectType(str, ALLOW_ARRAY | (allowGenericType ? ALLOW_GENERIC_TYPE : 0) | ALLOW_EXTENDED_TYPEOF, preferredType, instanceFailure))
				{
					do
					{
						if(instanceType && count >= instanceType->funcType->paramCount)
						{
							*instanceFailure = true;
							return;
						}
						if(count)
						{
							preferredType = instanceType ? instanceType->funcType->paramType[count] : NULL;
							ParseSelectType(str, ALLOW_ARRAY | (allowGenericType ? ALLOW_GENERIC_TYPE : 0) | ALLOW_EXTENDED_TYPEOF, preferredType, instanceFailure);
							handle->next = (TypeHandler*)stringPool.Allocate(sizeof(TypeHandler));
							handle = handle->next;
						}else{
							first = handle = (TypeHandler*)stringPool.Allocate(sizeof(TypeHandler));
						}
						handle->varType = (TypeInfo*)GetSelectedType();
						handle->next = NULL;
						if(!handle->varType)
							ThrowError((*str)->pos, "ERROR: parameter type of a function type cannot be auto");
						bool resolvedToGeneric = handle->varType->dependsOnGeneric;
						if(instanceType)
						{
							if(resolvedToGeneric || (instanceType->funcType->paramType[count] != handle->varType && instanceType->funcType->paramType[count] != typeGeneric))
							{
								*instanceFailure = true;
								return;
							}
						}
						count++;
					}while(ParseLexem(str, lex_comma));
				}
				if(instanceType && count != instanceType->funcType->paramCount)
				{
					*instanceFailure = true;
					return;
				}
				if(ParseLexem(str, lex_cparen))
					SelectTypeByPointer(CodeInfo::GetFunctionType(retType, first, count));
				else{
					ConvertTypeToReference((*str)->pos);
					run = false;
					*str = old;
				}
			}else{
				ConvertTypeToReference((*str)->pos);
			}
			break;
		case lex_obracket:
			if(notType)
				ThrowError((*str)->pos, "ERROR: typeof expression result is not a type");
			if(allowArray)
				ParseArrayDefinition(str);
			else
				run = false;
			break;
		default:
			run = false;
		}
	}
}

void ParseGenericEnd(Lexeme** str)
{
	// If it was '>>' which is now looks like '>', then check it back
	if((*str)->type == lex_greater && (*str)->length == 2)
	{
		(*str)->type = lex_shr;
		(*str)++;
		return;
	}
	if(!ParseLexem(str, lex_greater))
	{
		if((*str)->type == lex_shr) // if we have '>>'
			(*str)->type = lex_greater; // "parse" half of it and replace lexeme with '>' while preserving original length to restore later
		else
			ThrowError((*str)->pos, "ERROR: '>' expected after generic type alias list");
	}
}

// instanceType is used when semi-instancing a generic function to resolve "generic" types to real types
// instanceFailure is a variable that signals if failure was during instancing, so we have to silent the error
bool ParseSelectType(Lexeme** str, unsigned flag, TypeInfo* instanceType, bool* instanceFailure)
{
	// If instance type is passed, we must remove array and pointer qualifiers and strip function type of its arguments
	TypeInfo *strippedType = instanceType;
	if(instanceType)
	{
		assert(instanceFailure);
		// remove array and pointer qualifiers
		while(strippedType->refLevel || strippedType->arrLevel)
			strippedType = strippedType->subType;
	}
	bool notType = false;
	if((*str)->type == lex_typeof)
	{
		(*str)++;
		if(!ParseLexem(str, lex_oparen))
			ThrowError((*str)->pos, "ERROR: typeof must be followed by '('");

		unsigned nodeCount = CodeInfo::nodeList.size();
		Lexeme *curr = *str;
		bool isType = ParseSelectType(str, ALLOW_ARRAY | ALLOW_EXTENDED_TYPEOF | ALLOW_NUMERIC_RESULT);
		if(!isType || (*str)->type != lex_cparen)
		{
			// If ParseSelectType parser extended type expression that returned a number, remove that number
			if(isType && CodeInfo::nodeList.size() == nodeCount + 1)
				CodeInfo::nodeList.pop_back();
			*str = curr;
			jmp_buf oldHandler;
			memcpy(oldHandler, CodeInfo::errorHandler, sizeof(jmp_buf));
			if(!(flag & ALLOW_GENERIC_TYPE) || !setjmp(CodeInfo::errorHandler)) // if allowGenericType is enabled, we will set error handler
			{
				if(!ParseVaribleSet(str))
					ThrowError((*str)->pos, "ERROR: expression not found after typeof(");
				SetTypeOfLastNode();
			}else{
				// Node count shouldn't change while we did this
				if(!FunctionGeneric(false) || nodeCount != CodeInfo::nodeList.size())
				{
					memcpy(CodeInfo::errorHandler, oldHandler, sizeof(jmp_buf));
					longjmp(CodeInfo::errorHandler, 1);
				}
				SelectTypeByPointer(typeGeneric);
			}
			if(flag & ALLOW_GENERIC_TYPE)
				memcpy(CodeInfo::errorHandler, oldHandler, sizeof(jmp_buf));
		}else if(!GetSelectedType()){
			ThrowError((*str)->pos, "ERROR: cannot take typeid from auto type");
		}else{
			// If there was a node pushed during type selection because of extended typeof expressions, get its type
			if(CodeInfo::nodeList.size() == nodeCount + 1)
				SetTypeOfLastNode();
		}
		if(!ParseLexem(str, lex_cparen))
			ThrowError((*str)->pos, "ERROR: ')' not found after expression in typeof");
		nodeCount = CodeInfo::nodeList.size();
		while(ParseTypeofExtended(str, notType));
		if(nodeCount != CodeInfo::nodeList.size() && !(flag & ALLOW_NUMERIC_RESULT))
			ThrowError((*str)->pos, "ERROR: expression result is not a type");
	}else if((*str)->type == lex_auto){
		SelectTypeByPointer(NULL);
		(*str)++;
	}else if((*str)->type == lex_string){
		if((flag & ALLOW_ARRAY) && (*str+1)->type == lex_oparen)
			return false;

		Lexeme *curr = *str;
		NamespaceInfo* lastNS = GetCurrentNamespace();
		SetCurrentNamespace(NULL);
		NamespaceInfo* ns = NULL;
		while((*str)->type == lex_string && (*str + 1)->type == lex_point && (ns = IsNamespace(InplaceStr((*str)->pos, (*str)->length))) != NULL)
		{
			(*str) += 2;
			SetCurrentNamespace(ns);
		}
		unsigned int index;
		if((index = ParseTypename(str)) == 0)
		{
			SetCurrentNamespace(lastNS);
			*str = curr;
			return false;
		}
		SetCurrentNamespace(lastNS);

		SelectTypeByIndex(index - 1);
		if((*str)->type != lex_less && GetSelectedType()->genericInfo)
		{
			if(flag & ALLOW_GENERIC_BASE)
				return true;
			ThrowError((*str)->pos, "ERROR: generic class instance requires list of types inside '<' '>'");
		}
		if(ParseLexem(str, lex_less))
		{
			if(!GetSelectedType()->genericInfo)
				ThrowError((*str)->pos, "ERROR: cannot specify argument list for a class that is not generic");
			// For type instancing, this is a instance type argument list in correct order
			AliasInfo *forwList = NULL;
			TypeInfo *lastStrippedType = strippedType;
			if(instanceType)
			{
				while(strippedType->funcType)
					strippedType = strippedType->funcType->retType;
				if(strippedType->genericBase != GetSelectedType())
				{
					*instanceFailure = true;
					return false;
				}
				// Reverse a list of type aliases
				AliasInfo *revList = strippedType->childAlias;
				while(revList)
				{
					AliasInfo *info = TypeInfo::CreateAlias(revList->name, revList->type);
					info->next = forwList;
					forwList = info;
					revList = revList->next;
				}
			}
			TypeInfo *genericType = GetSelectedType();
			unsigned count = 0;
			bool resolvedToGeneric = false;
			do
			{
				if(!ParseSelectType(str, flag, instanceType ? forwList->type : NULL, instanceFailure))
				{
					if(instanceFailure)
					{
						// Remove pushed type IDs
						for(unsigned i = 0; i < count; i++)
							CodeInfo::nodeList.pop_back();
						return false;
					}
					ThrowError((*str)->pos, count ? "ERROR: typename required after ','" : "ERROR: typename required after '<'");
				}
				if(!GetSelectedType())
					ThrowError((*str)->pos, "ERROR: auto type cannot be used as template parameter");
				resolvedToGeneric |= GetSelectedType() == typeGeneric || GetSelectedType()->dependsOnGeneric;
				CodeInfo::nodeList.push_back(new NodeZeroOP(GetSelectedType()));
				count++;
				if(instanceType)
					forwList = forwList->next;
			}while(ParseLexem(str, lex_comma));
			// If type depends on generic
			if(resolvedToGeneric)
			{
				if(!(flag & ALLOW_GENERIC_TYPE)) // Fail if not allowed
					ThrowError((*str)->pos, "ERROR: type depends on 'generic' in a context where it is not allowed");
				// Instance type that has generic arguments
				TypeInstanceGeneric((*str)->pos, genericType, count, true);
			}else{
				TypeInstanceGeneric((*str)->pos, genericType, count);
			}
			ParseGenericEnd(str);
			strippedType = lastStrippedType;
		}
		if(flag & ALLOW_EXTENDED_TYPEOF)
		{
			unsigned nodeCount = CodeInfo::nodeList.size();
			while(ParseTypeofExtended(str, notType));
			if(nodeCount != CodeInfo::nodeList.size() && !(flag & ALLOW_NUMERIC_RESULT))
				ThrowError((*str)->pos, "ERROR: type is expected at this point");
		}
	}else if((flag & ALLOW_GENERIC_TYPE) && (ParseLexem(str, lex_generic) || ParseLexem(str, lex_at))){
		bool isAlias = (*str - 1)->type == lex_at;
		Lexeme *aliasName = *str;
		if(isAlias)
		{
			(*str)++;
			if(aliasName->type != lex_string)
				ThrowError(aliasName->pos, "ERROR: type alias required after '@'");
		}
		if(instanceType && (*str)->type == lex_ref && (*str + 1)->type == lex_oparen)
		{
			if(!strippedType->funcType)
			{
				*instanceFailure = true;
				return false;
			}
			SelectTypeByPointer(strippedType->funcType->retType);
			if(isAlias)
				AddAliasType(InplaceStr(aliasName->pos, aliasName->length));
		}else{
			bool takeFullType = (*str)->type != lex_ref && (*str)->type != lex_obracket;

			// Check if this alias type is specified through explicit type list on function instantiation
			bool typeExplicit = false;
			FunctionInfo *fInfo = GetCurrentFunction();
			if(fInfo)
			{
				AliasInfo *alias = fInfo->generic ? GetExplicitTypes() :  fInfo->explicitTypes;
				while(alias)
				{
					if(alias->name == InplaceStr(aliasName->pos, aliasName->length))
					{
						typeExplicit = true;
						SelectTypeByPointer(alias->type);
						break;
					}
					alias = alias->next;
				}
			}

			if(!typeExplicit)
				SelectTypeByPointer(instanceType ? (takeFullType ? instanceType : strippedType) : typeGeneric);

			if(instanceType && isAlias)
				AddAliasType(InplaceStr(aliasName->pos, aliasName->length));
			if(takeFullType)
				return true;
		}
	}else{
		return false;
	}

	ParseTypePostExpressions(str, !!(flag & ALLOW_ARRAY), notType, !!(flag & ALLOW_AUTO_RETURN_TYPE), !!(flag & ALLOW_GENERIC_TYPE), strippedType, instanceFailure);
	return true;
}

void ParseClassBody(Lexeme** str)
{
	if(ParseLexem(str, lex_extendable))
		TypeExtendable((*str)->pos);

	if(ParseLexem(str, lex_colon))
	{
		if(!ParseSelectType(str, ALLOW_ARRAY | ALLOW_EXTENDED_TYPEOF))
			ThrowError((*str)->pos, "ERROR: base type name is expected at this point");
		TypeDeriveFrom((*str)->pos, GetSelectedType());
	}
	if(!ParseLexem(str, lex_ofigure))
		ThrowError((*str)->pos, "ERROR: '{' not found after class name");
	while((*str)->type != lex_cfigure)
	{
		if(ParseTypedefExpr(str))
			continue;
		if(ParseLexem(str, lex_const))
		{
			if(!ParseSelectType(str))
				ThrowError((*str)->pos, "ERROR: type name expected after const");
			if(GetSelectedType() && (GetSelectedType()->type == TypeInfo::TYPE_COMPLEX || GetSelectedType()->refLevel || GetSelectedType()->type == TypeInfo::TYPE_VOID))
				ThrowError((*str)->pos, "ERROR: only basic numeric types can be used as constants");
			TypeInfo *constType = GetSelectedType();
			TypeInfo::MemberVariable *prevConst = NULL;
			do
			{
				if((*str)->type != lex_string)
					ThrowError((*str)->pos, "ERROR: constant name expected after %s", prevConst ? "','" : "type");
				if((*str)->length >= NULLC_MAX_VARIABLE_NAME_LENGTH)
					ThrowError((*str)->pos, "ERROR: member name length is limited to 2048 symbols");
				unsigned int memberNameLength = (*str)->length;
				char	*memberName = (char*)stringPool.Allocate(memberNameLength + 2);
				memcpy(memberName, (*str)->pos, memberNameLength);
				memberName[memberNameLength] = 0;
				(*str)++;
				if(!ParseLexem(str, lex_set))
				{
					if(!prevConst)
						ThrowError((*str)->pos, "ERROR: '=' not found after constant name");
					if(prevConst->type != typeChar && prevConst->type != typeShort && prevConst->type != typeInt && prevConst->type != typeLong)
						ThrowError((*str)->pos, "ERROR: only integer constant list gets automatically incremented by 1");
					
					CodeInfo::nodeList.push_back(prevConst->defaultValue);
					CodeInfo::nodeList.push_back(new NodeNumber(1, typeInt));
					AddBinaryCommandNode((*str)->pos, cmdAdd);
				}else{
					if(!ParseTernaryExpr(str))
						ThrowError((*str)->pos, "ERROR: expression not found after '='");
				}
				SelectTypeByPointer(constType);
				TypeAddConstant((*str)->pos, memberName);
				prevConst = GetDefinedType()->lastVariable;
			}while(ParseLexem(str, lex_comma));
			if(!ParseLexem(str, lex_semicolon))
				ThrowError((*str)->pos, "ERROR: ';' not found after constants");
			continue;
		}
		if(!ParseSelectType(str))
		{
			if((*str)->type == lex_string)
				ThrowError((*str)->pos, "ERROR: '%.*s' is not a known type name", (*str)->length, (*str)->pos);
			break;
		}
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

				FunctionAdd((*str)->pos, memberName);
				(*str)++;
				FunctionStart((*str-1)->pos);
				if(!ParseBlock(str))
					ThrowError((*str)->pos, "ERROR: function body expected after 'get'");
				FunctionEnd((*str-1)->pos);
				// Get function return type
				TypeInfo *propType = GetSelectedType();
				if((*str)->type == lex_string || (*str)->length == 3 || memcmp((*str)->pos, "set", 3) == 0)
				{
					// Set setter return type to auto
					SelectTypeByPointer(NULL);
					FunctionAdd((*str)->pos, memberName);
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
						FunctionParameter((*str)->pos, InplaceStr((*str)->pos, (*str)->length));
						(*str)++;
						if(!ParseLexem(str, lex_cparen))
							ThrowError((*str)->pos, "ERROR: ')' not found after r-value");
					}else{
						FunctionParameter((*str)->pos, InplaceStr("r"));
					}
					FunctionStart((*str-1)->pos);
					if(!ParseBlock(str))
						ThrowError((*str)->pos, "ERROR: function body expected after 'set'");
					FunctionEnd((*str-1)->pos);
				}
				if(!ParseLexem(str, lex_cfigure))
					ThrowError((*str)->pos, "ERROR: '}' is expected after property");
			}else{
				TypeAddMember((*str-1)->pos, memberName);

				while(ParseLexem(str, lex_comma))
				{
					if((*str)->type != lex_string)
						ThrowError((*str)->pos, "ERROR: member name expected after ','");
					if((*str)->length >= NULLC_MAX_VARIABLE_NAME_LENGTH)
						ThrowError((*str)->pos, "ERROR: member name length is limited to 2048 symbols");
					char	*memberName = (char*)stringPool.Allocate((*str)->length+1);
					memcpy(memberName, (*str)->pos, (*str)->length);
					memberName[(*str)->length] = 0;
					TypeAddMember((*str)->pos, memberName);
					(*str)++;
				}
			}
			if(!ParseLexem(str, lex_semicolon))
				ThrowError((*str)->pos, "ERROR: ';' not found after class member list");
		}
	}
	if(!ParseLexem(str, lex_cfigure))
		ThrowError((*str)->pos, "ERROR: '}' not found after class definition");
}

bool ParseClassDefinition(Lexeme** str)
{
	if(!ParseAlignment(str))
		SetCurrentAlignment(0);

	if(ParseLexem(str, lex_class))
	{
		if((*str)->type != lex_string)
			ThrowError((*str)->pos, "ERROR: class name expected");
		TypeInfo *proto = TypeBegin((*str)->pos, (*str)->pos+(*str)->length);
		(*str)++;

		if(ParseLexem(str, lex_less))
		{
			if(proto)
				ThrowError((*str)->pos, "ERROR: type was forward declared as a non-generic type");
			CodeInfo::typeInfo.back()->dependsOnGeneric = true;
			TypeGeneric(unsigned(*str - CodeInfo::lexStart));
			TypeInfo *newType = GetSelectedType();
			AliasInfo *aliasList = NULL;
			unsigned count = 0;
			do
			{
				if((*str)->type != lex_string)
					ThrowError((*str)->pos, count ? "ERROR: generic type alias required after ','" : "ERROR: generic type alias required after '<'");
				if((*str)->length >= NULLC_MAX_VARIABLE_NAME_LENGTH)
					ThrowError((*str)->pos, "ERROR: alias name length is limited to 2048 symbols");
				if(ParseSelectType(str))
					ThrowError((*str)->pos, "ERROR: there is already a type or an alias with the same name");

				AliasInfo *info = TypeInfo::CreateAlias(InplaceStr((*str)->pos, (*str)->length), typeGeneric);
				CodeInfo::classMap.insert(info->nameHash, info->type);
				info->next = aliasList;
				aliasList = info;

				(*str)++;
				count++;
			}while(ParseLexem(str, lex_comma));
			newType->childAlias = aliasList;
			newType->genericInfo->aliasCount = count;
			if(!ParseLexem(str, lex_greater))
				ThrowError((*str)->pos, "ERROR: '>' expected after generic type alias list");
			unsigned braces = 1;
			if(!ParseLexem(str, lex_ofigure))
			{
				if(!ParseLexem(str, lex_extendable) && !ParseLexem(str, lex_colon))
					ThrowError((*str)->pos, "ERROR: '{', ':' or 'extendable' not found after class name");
				braces = 0;
			}
			// Skip class body
			for(;;)
			{
				if((*str)->type == lex_none)
					ThrowError((*str)->pos, "ERROR: unknown lexeme in class body");
				if(ParseLexem(str, lex_ofigure))
				{
					braces++;
				}else if(ParseLexem(str, lex_cfigure)){
					braces--;
					if(!braces)
						break;
				}else{
					(*str)++;
				}
			}
			TypeFinish();
			return true;
		}
		if(ParseLexem(str, lex_semicolon))
		{
			TypePrototypeFinish();
		}else{
			ParseClassBody(str);
			TypeFinish();
		}
		return true;
	}
	return false;
}

bool ParseEnum(Lexeme** str)
{
	if(!ParseLexem(str, lex_enum))
		return false;

	SetCurrentAlignment(0);
	if((*str)->type != lex_string)
		ThrowError((*str)->pos, "ERROR: enum name expected");
	TypeBegin((*str)->pos, (*str)->pos+(*str)->length);
	(*str)++;

	TypeInfo *enumType = GetDefinedType();
	enumType->type = TypeInfo::TYPE_INT;
	enumType->stackType = podTypeToStackType[TypeInfo::TYPE_INT];
	enumType->dataType = podTypeToDataType[TypeInfo::TYPE_INT];

	if(!ParseLexem(str, lex_ofigure))
		ThrowError((*str)->pos, "ERROR: '{' not found after enum name");
	TypeInfo::MemberVariable *prevConst = NULL;
	do
	{
		if((*str)->type != lex_string)
			ThrowError((*str)->pos, "ERROR: enumeration name expected after %s", prevConst ? "','" : "{");
		if((*str)->length >= NULLC_MAX_VARIABLE_NAME_LENGTH)
			ThrowError((*str)->pos, "ERROR: enumeration name length is limited to 2048 symbols");
		unsigned int memberNameLength = (*str)->length;
		char	*memberName = (char*)stringPool.Allocate(memberNameLength + 2);
		memcpy(memberName, (*str)->pos, memberNameLength);
		memberName[memberNameLength] = 0;
		(*str)++;
		if(!ParseLexem(str, lex_set))
		{
			if(!prevConst)
			{
				CodeInfo::nodeList.push_back(new NodeNumber(0, enumType));
			}else{
				CodeInfo::nodeList.push_back(prevConst->defaultValue);
				CodeInfo::nodeList.push_back(new NodeNumber(1, enumType));
				AddBinaryCommandNode((*str)->pos, cmdAdd);
			}
		}else{
			if(!ParseTernaryExpr(str))
				ThrowError((*str)->pos, "ERROR: expression not found after '='");
		}
		SelectTypeByPointer(typeInt);
		TypeAddConstant((*str)->pos, memberName);
		enumType->lastVariable->defaultValue->typeInfo = enumType;
		prevConst = enumType->lastVariable;
	}while(ParseLexem(str, lex_comma));
	if(!ParseLexem(str, lex_cfigure))
		ThrowError((*str)->pos, "ERROR: '}' not found after enum definition");

	TypeFinish();
	enumType->size = 4;

	// Add conversion operator int int(enum_type) and enum_type enum_type(int)
	TypeInfo *target[] = { typeInt, enumType };
	TypeInfo *source[] = { enumType, typeInt };

	// First conversion operator should be defined at global scope
	unsigned prevBackupSize = 0, prevStackSize = 0;
	NamespaceInfo *lastNS = NULL;
	RestoreNamespaces(false, NULL, prevBackupSize, prevStackSize, lastNS);

	for(int i = 0; i < 2; i++)
	{
		// Second conversion operator should be defined at namespace scope
		if(i == 1)
			RestoreNamespaces(true, NULL, prevBackupSize, prevStackSize, lastNS);
		SelectTypeByPointer(target[i]);
		const char *name = target[i]->name;
		if(const char* pos = strrchr(name, '.'))
			name = pos + 1;
		FunctionAdd((*str)->pos, name);
		SelectTypeByPointer(source[i]);
		FunctionParameter((*str)->pos, InplaceStr("x"));
		FunctionStart((*str)->pos);
		AddGetAddressNode((*str)->pos, InplaceStr("x"));
		AddGetVariableNode((*str)->pos);
		CodeInfo::nodeList.back()->typeInfo = target[i];
		AddReturnNode((*str)->pos);
		FunctionEnd((*str)->pos);
		AddTwoExpressionNode();
	}

	return true;
}

unsigned int ParseFunctionArguments(Lexeme** str)
{
	unsigned callArgCount = 0;
	unsigned lastArgument = SetCurrentArgument(callArgCount);

	bool namedArgFunction = false;
	Lexeme *argName = NULL;
	if((*str)->type == lex_string && (*str)[1].type == lex_colon)
	{
		argName = (*str);
		(*str) += 2;
		namedArgFunction = true;
	}
	if(ParseVaribleSet(str))
	{
		CodeInfo::nodeList.back()->argName = argName;
		argName = NULL;
		callArgCount++;
		while(ParseLexem(str, lex_comma))
		{
			SetCurrentArgument(callArgCount);

			if((*str)->type == lex_string && (*str)[1].type == lex_colon)
			{
				argName = (*str);
				(*str) += 2;
				namedArgFunction = true;
			}else if(namedArgFunction){
				ThrowError((*str)->pos, "ERROR: function parameter name expected after ','");
			}
			if(!ParseVaribleSet(str))
			{
				if(argName)
					ThrowError((*str)->pos, "ERROR: expression not found after ':' in function parameter list");
				else
					ThrowError((*str)->pos, "ERROR: expression not found after ',' in function parameter list");
			}
			CodeInfo::nodeList.back()->argName = argName;
			argName = NULL;
			callArgCount++;
		}
	}else if(argName){
		ThrowError((*str)->pos, "ERROR: expression not found after ':' in function parameter list");
	}
	SetCurrentArgument(lastArgument);
	return callArgCount;
}

bool ParseFunctionCall(Lexeme** str, bool memberFunctionCall)
{
	if((*str)->type != lex_string || ((*str)[1].type != lex_oparen && (*str)[1].type != lex_with))
		return false;

	if((*str)->length >= NULLC_MAX_VARIABLE_NAME_LENGTH)
		ThrowError((*str)->pos, "ERROR: function name length is limited to 2048 symbols");
	char	*functionName = (char*)stringPool.Allocate((*str)->length+1);
	memcpy(functionName, (*str)->pos, (*str)->length);
	functionName[(*str)->length] = 0;

	(*str)++;

	// Parse explicit generic function type list
	if(ParseLexem(str, lex_with))
	{
		AliasInfo *aliasBegin = NULL, *aliasEnd = NULL;

		if(!ParseLexem(str, lex_less))
			ThrowError((*str)->pos, "ERROR: '<' not found before explicit generic type alias list");

		unsigned count = 0;
		do
		{
			if(!ParseSelectType(str))
				ThrowError((*str)->pos, count ? "ERROR: type name is expected after ','" : "ERROR: type name is expected after 'with'");

			AliasInfo *info = TypeInfo::CreateAlias(InplaceStr(""), GetSelectedType());

			if(!aliasBegin)
				aliasBegin = aliasEnd = info;
			else
				aliasEnd->next = info;
			aliasEnd = info;

			count++;
		}while(ParseLexem(str, lex_comma));

		PushExplicitTypes(aliasBegin);

		if(!ParseLexem(str, lex_greater))
			ThrowError((*str)->pos, "ERROR: '>' not found after explicit generic type alias list");

		if(!ParseLexem(str, lex_oparen))
			ThrowError((*str)->pos, "ERROR: '(' is expected at this point");
	}else{
		PushExplicitTypes(NULL);

		(*str)++;
	}

	// PrepareMemberCall may signal that this is not actually a member function call
	bool wasMemberCall = memberFunctionCall;
	// Prepare member function call
	if(memberFunctionCall)
		memberFunctionCall = PrepareMemberCall((*str)->pos, functionName);

	TypeInfo *lValue = memberFunctionCall ? CodeInfo::nodeList.back()->typeInfo->subType : NULL;

	// If it was a member function call, but isn't now, then we should take function pointer from the top of node list
	NodeZeroOP *fAddress = NULL;
	if(!memberFunctionCall && wasMemberCall)
	{
		fAddress = CodeInfo::nodeList.back();
		CodeInfo::nodeList.pop_back();
	}
	const char *last = SetCurrentFunction(memberFunctionCall ? GetClassFunctionName(lValue, InplaceStr(functionName)) : functionName);
	// Parse function arguments
	NamespaceInfo *lastNS = GetCurrentNamespace();
	unsigned int callArgCount = ParseFunctionArguments(str);
	SetCurrentNamespace(lastNS);
	if(!ParseLexem(str, lex_cparen))
		ThrowError((*str)->pos, "ERROR: ')' not found after function parameter list");
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

	PopExplicitTypes();

	return true;
}

bool ParseFunctionVariables(Lexeme** str, unsigned nodeOffset)
{
	bool genericArg = false;
	Lexeme *currPos = *str;
	if(!ParseSelectType(str, ALLOW_ARRAY | ALLOW_GENERIC_TYPE | ALLOW_EXTENDED_TYPEOF))
		return true;

	genericArg = GetSelectedType() ? GetSelectedType()->dependsOnGeneric : false;
	if(genericArg)
		FunctionGeneric(true);

	unsigned argID = 0;
	if(genericArg && nodeOffset)
	{
		TypeInfo *curr = GetSelectedType();
		SelectTypeForGeneric(currPos, nodeOffset - 1 + argID);
		if(curr->refLevel && !GetSelectedType()->refLevel)
			SelectTypeByPointer(CodeInfo::GetReferenceType(GetSelectedType()));
	}

	if((*str)->type != lex_string)
		ThrowError((*str)->pos, "ERROR: variable name not found after type in function variable list");

	if((*str)->length >= NULLC_MAX_VARIABLE_NAME_LENGTH)
		ThrowError((*str)->pos, "ERROR: parameter name length is limited to 2048 symbols");
	FunctionParameter((*str)->pos, InplaceStr((*str)->pos, (*str)->length));
	(*str)++;

	if(ParseLexem(str, lex_set))
	{
		FunctionPrepareDefault();
		if(!ParseTernaryExpr(str))
			ThrowError((*str)->pos, "ERROR: default parameter value not found after '='");
		FunctionParameterDefault((*str)->pos);
	}

	while(ParseLexem(str, lex_comma))
	{
		argID++;
		bool lastGeneric = genericArg;
		genericArg = false;
		Lexeme *currPosPrev = currPos;
		currPos = *str;
		if(!ParseSelectType(str, ALLOW_ARRAY | ALLOW_GENERIC_TYPE | ALLOW_EXTENDED_TYPEOF))
		{
			genericArg = lastGeneric; // if there is no type and no generic, then this parameter is as generic as the last one
			currPos = currPosPrev;
		}
		genericArg |= GetSelectedType() ? GetSelectedType()->dependsOnGeneric : false;
		if(genericArg)
			FunctionGeneric(true);
		if(genericArg && nodeOffset)
		{
			TypeInfo *curr = GetSelectedType();
			SelectTypeForGeneric(currPos, nodeOffset - 1 + argID);
			if(curr->refLevel && !GetSelectedType()->refLevel)
				SelectTypeByPointer(CodeInfo::GetReferenceType(GetSelectedType()));
		}

		if((*str)->type != lex_string)
			ThrowError((*str)->pos, "ERROR: variable name not found after type in function variable list");
		if((*str)->length >= NULLC_MAX_VARIABLE_NAME_LENGTH)
			ThrowError((*str)->pos, "ERROR: parameter name length is limited to 2048 symbols");
		FunctionParameter((*str)->pos, InplaceStr((*str)->pos, (*str)->length));
		(*str)++;
		
		if(ParseLexem(str, lex_set))
		{
			FunctionPrepareDefault();
			if(!ParseTernaryExpr(str))
				ThrowError((*str)->pos, "ERROR: default parameter value not found after '='");
			FunctionParameterDefault((*str)->pos);
		}
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
	}else if((*str)->type == lex_string && ((*str + 1)->type == lex_colon || (*str + 1)->type == lex_point || ((*str + 1)->type == lex_less && (*str + 2)->type != lex_at))){
		TypeInfo *retType = (TypeInfo*)GetSelectedType();
		if(!ParseSelectType(str, ALLOW_ARRAY | ALLOW_GENERIC_BASE))
			ThrowError((*str)->pos, "ERROR: class name expected before ':' or '.'");
		if((*str)->type == lex_point)
			funcProperty = true;
		(*str)++;
		if((*str)->type != lex_string)
			ThrowError((*str)->pos, "ERROR: function name expected after ':' or '.'");
		TypeContinue((*str)->pos);
		SelectTypeByPointer(retType);
		typeMethod = true;
	}
	char	*functionName = NULL;
	if((*str)->type == lex_string || ((*str)->type >= lex_add && (*str)->type <= lex_in) || ((*str)->type >= lex_set && (*str)->type <= lex_xorset) || (*str)->type == lex_bitnot || (*str)->type == lex_lognot)
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

	if((*str)->type != lex_oparen && (*str)->type != lex_less)
	{
		*str = name;
		return false;
	}

	bool isOperator = name[0].type == lex_operator && ((name[1].type >= lex_add && name[1].type <= lex_xorset) || name[1].type == lex_obracket || name[1].type == lex_oparen || (name[1].type >= lex_set && name[1].type <= lex_powset) || name[1].type == lex_bitnot || name[1].type == lex_lognot);

	FunctionAdd((*str)->pos, functionName, isOperator);

	AliasInfo *aliasBegin = NULL, *aliasEnd = NULL;
	unsigned aliasCount = 0;

	if((*str)->type == lex_oparen)
	{
		(*str)++;
	}else if((*str)->type == lex_less){
		// Explicit generic type list
		FunctionGeneric(true);
		(*str)++;

		do
		{
			if(!ParseLexem(str, lex_at))
				ThrowError((*str)->pos, aliasCount ? "ERROR: '@' is expected after ',' in explicit generic type alias list" : "ERROR: '@' is expected before explicit generic type alias");
			if((*str)->type != lex_string)
				ThrowError((*str)->pos, "ERROR: explicit generic type alias is expected after '@'");
			if((*str)->length >= NULLC_MAX_VARIABLE_NAME_LENGTH)
				ThrowError((*str)->pos, "ERROR: alias name length is limited to 2048 symbols");

			AliasInfo *curr = aliasBegin;
			while(curr)
			{
				if(curr->name == InplaceStr((*str)->pos, (*str)->length))
					ThrowError((*str)->pos, "ERROR: there is already a type or an alias with the same name");
				curr = curr->next;
			}

			AliasInfo *info = TypeInfo::CreateAlias(InplaceStr((*str)->pos, (*str)->length), typeGeneric);

			if(!aliasBegin)
				aliasBegin = aliasEnd = info;
			else
				aliasEnd->next = info;
			aliasEnd = info;

			(*str)++;
			aliasCount++;
		}while(ParseLexem(str, lex_comma));

		if(!ParseLexem(str, lex_greater))
			ThrowError((*str)->pos, "ERROR: '>' not found after explicit generic type alias list");

		if(!ParseLexem(str, lex_oparen))
			ThrowError((*str)->pos, "ERROR: '(' is expected at this point");
	}

	Lexeme *vars = *str;
	ParseFunctionVariables(str);

	if(!ParseLexem(str, lex_cparen))
		ThrowError((*str)->pos, "ERROR: ')' not found after function variable list");

	if(ParseLexem(str, lex_semicolon))
	{
		if(FunctionGeneric(false))
			ThrowError((*str)->pos, "ERROR: generic function cannot be forward-declared");
		if(isOperator)
			FunctionToOperator(start->pos);
		FunctionPrototype(start->pos);
		if(typeMethod)
			TypeStop();
		return true;
	}

	if(FunctionGeneric(false))
	{
		if(!ParseLexem(str, lex_ofigure))
			ThrowError((*str)->pos, "ERROR: '{' not found after function header");
		FunctionGeneric(true, unsigned(vars - CodeInfo::lexStart));

		// Setup function explicit type list
		FunctionInfo *currFunction = GetCurrentFunction();
		currFunction->explicitTypes = aliasBegin;

		if(isOperator)
			FunctionToOperator(start->pos);
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
		FunctionStart((*str-1)->pos);
		if(!ParseLexem(str, lex_ofigure))
			ThrowError((*str)->pos, "ERROR: '{' not found after function header");
		const char *lastFunc = SetCurrentFunction(NULL);

		if(!ParseCode(str))
			AddVoidNode();
		if(!ParseLexem(str, lex_cfigure))
			ThrowError((*str)->pos, "ERROR: '}' not found after function body");
		SetCurrentFunction(lastFunc);

		if(isOperator)
			FunctionToOperator(start->pos);

		FunctionEnd(start->pos);
	}

	if(typeMethod)
		TypeStop();
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

	NamespaceInfo *lastNS = GetCurrentNamespace();
	SetCurrentNamespace(NULL);

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
		if(imaginary && type->funcType->paramType[currArg] == typeGeneric)
		{
			SelectTypeByPointer(selType);
			imaginary = false;
		}
		if(!imaginary || type->funcType->paramType[currArg] == selType)
		{
			if(GetSelectedType() == typeGeneric)
				ThrowError((*str)->pos, "ERROR: function allows any type for this argument so it must be specified explicitly");
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
		if(ParseSelectType(str) && type->funcType->paramType[currArg] != GetSelectedType() && type->funcType->paramType[currArg] != typeGeneric)
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
			if(CodeInfo::nodeList.back()->typeInfo->refLevel && CodeInfo::nodeList.back()->typeInfo->subType == varInfo->varType)
				CodeInfo::nodeList.push_back(new NodeDereference());
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

	SetCurrentNamespace(lastNS);

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
		Lexeme *curr = *str;
		if(!ParseVaribleSet(str))
			ThrowError((*str)->pos, "ERROR: expression not found after '='");
		AddDefineVariableNode(curr->pos, varInfo);
		AddPopNode((*str)->pos);
	}else{
		// Try to call constructor with no arguments
		TypeInfo *info = GetSelectedType();
		// Handle array types
		TypeInfo *base = info;
		while(base && base->arrLevel && base->arrSize != TypeInfo::UNSIZED_ARRAY) // Unsized arrays are not initialized
			base = base->subType;
		bool callDefault = false;
		bool hasConstructor = base ? HasConstructor(base, 0, &callDefault) : false;
		if(hasConstructor)
		{
			const char *name = base->genericBase ? base->genericBase->name : base->name;
			if(callDefault)
				name = GetDefaultConstructorName(name);
			AddGetAddressNode((*str)->pos, varInfo->name);
			AddDefaultConstructorCall((*str)->pos, name);
		}else{
			AddVariableReserveNode((*str)->pos);
		}
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
		AddTwoExpressionNode();
	}
	return true;
}

bool ParseAlignment(Lexeme** str)
{
	if(ParseLexem(str, lex_noalign))
	{
		SetCurrentAlignment(0);
		return true;
	}else if(ParseLexem(str, lex_align))
	{
		if(!ParseLexem(str, lex_oparen))
			ThrowError((*str)->pos, "ERROR: '(' expected after align");
		
		const char *start = (*str)->pos;
		if(!ParseLexem(str, lex_number))
			ThrowError((*str)->pos, "ERROR: alignment value not found after align(");
		SetCurrentAlignment(atoi(start));
		if(!ParseLexem(str, lex_cparen))
			ThrowError((*str)->pos, "ERROR: ')' expected after alignment value");
		return true;
	}
	return false;
}

bool ParseVariableDefine(Lexeme** str)
{
	Lexeme *curr = *str;
	SetCurrentAlignment(0xFFFFFFFF);
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
		AddIfElseNode(condPos);
	}else{
		AddIfNode(condPos);
	}
	return true;
}

bool ParseForExpr(Lexeme** str)
{
	if(!ParseLexem(str, lex_for))
		return false;
	
	IncreaseCycleDepth();
	
	if(!ParseLexem(str, lex_oparen))
		ThrowError((*str)->pos, "ERROR: '(' not found after 'for'");
	
	BeginBlock();

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
				AddVoidNode();
			if(!ParseLexem(str, lex_cfigure))
				ThrowError((*str)->pos, "ERROR: '}' not found after '{'");
		}else{
			if(!ParseVariableDefine(str))
			{
				if(!ParseVaribleSet(str))
					AddVoidNode();
				else
					AddPopNode((*str)->pos);
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
				AddVoidNode();
			if(!ParseLexem(str, lex_cfigure))
				ThrowError((*str)->pos, "ERROR: '}' not found after '{'");
		}else{
			if(!ParseVaribleSet(str))
				AddVoidNode();
			else
				AddPopNode((*str)->pos);
		}
	}

	if(!ParseLexem(str, lex_cparen))
		ThrowError((*str)->pos, "ERROR: ')' not found after 'for' statement");

	if(ParseLexem(str, lex_ofigure))
	{
		if(!ParseCode(str))
			AddVoidNode();
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

	IncreaseCycleDepth();

	// Begin block outside the expression so that the variables inside can be used inside 'while' expression
	BeginBlock();

	if(!ParseLexem(str, lex_ofigure))
	{
		if(!ParseExpression(str))
			ThrowError((*str)->pos, "ERROR: expression expected after 'do'");
	}else{
		if(!ParseCode(str))
			AddVoidNode();
		if(!ParseLexem(str, lex_cfigure))
			ThrowError((*str)->pos, "ERROR: closing '}' not found");
	}

	if(!ParseLexem(str, lex_while))
		ThrowError((*str)->pos, "ERROR: 'while' expected after 'do' statement");
	if(!ParseLexem(str, lex_oparen))
		ThrowError((*str)->pos, "ERROR: '(' not found after 'while'");

	const char *condPos = (*str)->pos;
	if(!ParseVaribleSet(str))
		ThrowError((*str)->pos, "ERROR: expression expected after 'while('");
	if(!ParseLexem(str, lex_cparen))
		ThrowError((*str)->pos, "ERROR: closing ')' not found after expression in 'while' statement");

	EndBlock();

	AddDoWhileNode(condPos);

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
	BeginSwitch(condPos);

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
			AddVoidNode();
		while(ParseExpression(str))
			AddTwoExpressionNode();
		if(condPos[-1].type == lex_default)
			AddDefaultNode();
		else
			AddCaseNode(condPos->pos);
	}

	if(!ParseLexem(str, lex_cfigure))
		ThrowError((*str)->pos, "ERROR: '}' not found after 'switch' statement");
	EndSwitch();
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
	AddBreakNode(pos);
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
	AddContinueNode(pos);
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

	if((*str)->length == 1 || (*str)->pos[(*str)->length - 1] != '\"')
		ThrowError((*str)->pos, "ERROR: unclosed string constant");

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
		ParseCode(str);
		AddGeneratorReturnData((*str)->pos);
		AddReturnNode((*str)->pos);
		AddTwoExpressionNode();

		FunctionEnd((*str)->pos);

		if(!AddFunctionCallNode((*str)->pos, "__gen_list", 1, true))
			ThrowError((*str)->pos, "ERROR: internal compiler error while calling list generator");
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
		bool isFuncCall = false;
		if(!ParseVariable(str, &isFuncCall))
		{
			if(!ParseTerminal(str))
				ThrowError((*str)->pos, "ERROR: variable name not found after '*'");
			else if(lastIsFunctionCall)
				*lastIsFunctionCall = true;
		}
		if(!isFuncCall)
			AddGetVariableNode((*str)->pos, true);
		return true;
	}
	
	if((*str)->type != lex_string || (*str)[1].type == lex_oparen)
		return false;

	NamespaceInfo* lastNS = GetCurrentNamespace();
	SetCurrentNamespace(NULL);
	NamespaceInfo* ns = NULL;
	while((*str)->type == lex_string && (*str + 1)->type == lex_point && (ns = IsNamespace(InplaceStr((*str)->pos, (*str)->length))) != NULL)
	{
		(*str) += 2;
		SetCurrentNamespace(ns);
	}
	
	if((*str)->length >= NULLC_MAX_VARIABLE_NAME_LENGTH)
		ThrowError((*str)->pos, "ERROR: variable name length is limited to 2048 symbols");
	AddGetAddressNode((*str)->pos, InplaceStr((*str)->pos, (*str)->length));
	(*str)++;

	SetCurrentNamespace(NULL);
	bool isFuncCall = false;
	while(ParsePostExpression(str, &isFuncCall));
	if(lastIsFunctionCall)
		*lastIsFunctionCall = isFuncCall;
	SetCurrentNamespace(lastNS);

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
		if((*str)[1].type == lex_oparen || (*str)[1].type == lex_with)
		{
			if(!ParseFunctionCall(str, true))
				ThrowError((*str)->pos, "ERROR: function call is excepted after '.'");
		}else{
			if((*str)->length >= NULLC_MAX_VARIABLE_NAME_LENGTH)
				ThrowError((*str)->pos, "ERROR: variable name length is limited to 2048 symbols");
			AddMemberAccessNode((*str)->pos, InplaceStr((*str)->pos, (*str)->length));
			(*str)++;
		}
	}else if(ParseLexem(str, lex_obracket)){
		if(isFunctionCall)
			*isFunctionCall = false;

		const char *last = SetCurrentFunction(NULL);
		unsigned int callArgCount = ParseFunctionArguments(str);
		if(!ParseLexem(str, lex_cbracket))
			ThrowError((*str)->pos, "ERROR: ']' not found after expression");
		SetCurrentFunction(last);

		AddArrayIndexNode((*str)->pos, callArgCount);
	}else if(ParseLexem(str, lex_oparen)){
		if(isFunctionCall)
			*isFunctionCall = true;

		NodeZeroOP *fAddress = CodeInfo::nodeList.back();
		CodeInfo::nodeList.pop_back();

		const char *last = SetCurrentFunction(NULL);
		unsigned int callArgCount = ParseFunctionArguments(str);
		if(!ParseLexem(str, lex_cparen))
			ThrowError((*str)->pos, "ERROR: ')' not found after function parameter list");
		SetCurrentFunction(last);

		CodeInfo::nodeList.push_back(fAddress);
		AddFunctionCallNode((*str)->pos, NULL, callArgCount);
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
	if(hadPost && !lastIsFunctionCall && !((*str)->type >= lex_set && (*str)->type <= lex_xorset))
		AddGetVariableNode((*str)->pos);
}

void ParseCustomConstructor(Lexeme** str, TypeInfo* resultType, NodeZeroOP* getPointer)
{
	if(!ParseLexem(str, lex_ofigure))
		return;

	static int constrNum = 0;
	char	*functionName = AllocateString(16);
	sprintf(functionName, "$funcc%d", constrNum++);

	SelectTypeByPointer(resultType->subType);

	TypeInfo *currDefinedType = GetDefinedType();
	unsigned int currentDefinedTypeMethodCount = ResetDefinedTypeState();
	TypeContinue((*str)->pos);

	SelectTypeByPointer(typeVoid);
	FunctionAdd((*str)->pos, functionName);
	FunctionStart((*str)->pos);
	const char *lastFunc = SetCurrentFunction(NULL);
	if(!ParseCode(str))
		AddVoidNode();
	if(!ParseLexem(str, lex_cfigure))
		ThrowError((*str)->pos, "ERROR: '}' not found after custom constructor body");
	SetCurrentFunction(lastFunc);
	FunctionEnd((*str)->pos);
	CodeInfo::nodeList.pop_back();

	TypeStop();
	RestoreDefinedTypeState(currDefinedType, currentDefinedTypeMethodCount);

	NodeOneOP* wrap = new NodeOneOP();
	wrap->SetFirstNode(getPointer);
	CodeInfo::nodeList.push_back(wrap);
	PrepareMemberCall((*str)->pos);
	PushExplicitTypes(NULL);
	AddMemberFunctionCall((*str)->pos, functionName, 0);
	PopExplicitTypes();
	AddTwoExpressionNode(resultType);
}

bool ParseTerminal(Lexeme** str)
{
	switch((*str)->type)
	{
	case lex_true:
	case lex_false:
		CodeInfo::nodeList.push_back(new NodeNumber((*str)->type == lex_true ? 1 : 0, typeBool));
		(*str)++;
		return true;
		break;
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
		AddLogNotNode((*str)->pos);
		return true;
		break;
	case lex_bitnot:
		(*str)++;
		if(!ParseTerminal(str))
			ThrowError((*str)->pos, "ERROR: expression not found after '~'");
		AddBitNotNode((*str)->pos);
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
			AddNegateNode((*str)->pos);
		return true;
	}
		break;
	case lex_semiquotedchar:
		if((*str)->length == 1 || (*str)->pos[(*str)->length - 1] != '\'')
			ThrowError((*str)->pos, "ERROR: unclosed character constant");
		else if(((*str)->length > 3 && (*str)->pos[1] != '\\') || (*str)->length > 4)
			ThrowError((*str)->pos, "ERROR: only one character can be inside single quotes");
		else if((*str)->length < 3)
			ThrowError((*str)->pos, "ERROR: empty character constant");
		AddNumberNodeChar((*str)->pos);
		(*str)++;
		return true;
		break;
	case lex_sizeof:
		(*str)++;
		if(!ParseLexem(str, lex_oparen))
			ThrowError((*str)->pos, "ERROR: sizeof must be followed by '('");
		if(ParseSelectType(str))
		{
			GetTypeSize((*str)->pos, false);
		}else{
			if(ParseVaribleSet(str))
				GetTypeSize((*str)->pos, true);
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

		const char *pos = (*str)->pos;
		if(!ParseSelectType(str, false))
			ThrowError((*str)->pos, "ERROR: type name expected after 'new'");
		
		TypeInfo *info = GetSelectedType();
		GetTypeSize((*str)->pos, false);
		const char *name = FindConstructorName(info);
		bool callDefault = false;
		bool hasEmptyConstructor = HasConstructor(info, 0, &callDefault);

		if((*str)->type == lex_oparen)
		{
			ParseLexem(str, lex_oparen);
			AddTypeAllocation((*str)->pos);
			NodeZeroOP *getPointer = PrepareConstructorCall((*str)->pos);

			NodeOneOP *wrap = new NodeOneOP();
			wrap->SetFirstNode(getPointer);
			CodeInfo::nodeList.push_back(wrap);

			PrepareMemberCall(pos);

			const char *last = SetCurrentFunction(name);
			unsigned int callArgCount = ParseFunctionArguments(str);
			if(!ParseLexem(str, lex_cparen))
				ThrowError((*str)->pos, "ERROR: ')' not found after function parameter list");
			SetCurrentFunction(last);
			if(callArgCount == 0 && callDefault)
				name = GetDefaultConstructorName(name);
			PushExplicitTypes(NULL);
			if(!AddMemberFunctionCall((*str)->pos, name, callArgCount, callArgCount == 0)) // silence the error if default constructor is called
				AddPopNode(pos);
			PopExplicitTypes();

			FinishConstructorCall((*str)->pos);
			ParseCustomConstructor(str, getPointer->typeInfo->subType, getPointer);

			return true;
		}

		bool arrayAlloc = false;
		if(ParseLexem(str, lex_obracket))
		{
			AddUnfixedArraySize();
			ConvertTypeToArray((*str)->pos);
			info = GetSelectedType();
			arrayAlloc = true;

			if(!ParseTernaryExpr(str))
				ThrowError((*str)->pos, "ERROR: expression not found after '['");
			if(!ParseLexem(str, lex_cbracket))
				ThrowError((*str)->pos, "ERROR: ']' not found after expression");
		}
		SelectTypeByPointer(info);
		AddTypeAllocation(pos, arrayAlloc);
		// Constructor with no arguments is called even if () are not written
		if(hasEmptyConstructor)
		{
			NodeZeroOP *getPointer = PrepareConstructorCall((*str)->pos);

			NodeOneOP *wrap = new NodeOneOP();
			wrap->SetFirstNode(getPointer);
			CodeInfo::nodeList.push_back(wrap);

			PrepareMemberCall(pos);

			if(callDefault)
				name = GetDefaultConstructorName(name);
			AddDefaultConstructorCall((*str)->pos, name);
			FinishConstructorCall((*str)->pos);
			if(!arrayAlloc)
				ParseCustomConstructor(str, getPointer->typeInfo->subType, getPointer);
		}else if((*str)->type == lex_ofigure && !arrayAlloc){
			// Custom construction
			NodeZeroOP *getPointer = PrepareConstructorCall((*str)->pos);
			ParseCustomConstructor(str, getPointer->typeInfo->subType, getPointer);
			AddTwoExpressionNode(getPointer->typeInfo->subType);
		}

		return true;
	}
		break;
	case lex_at:
		if((*str)[1].type != lex_quotedstring)
		{
			(*str)++;
			bool isOperator = ((*str)->type >= lex_add && (*str)->type <= lex_in) || ((*str)->type >= lex_set && (*str)->type <= lex_xorset) || (*str)->type == lex_bitnot || (*str)->type == lex_lognot;
			if(!isOperator)
				ThrowError((*str)->pos, "ERROR: string expected after '@'");
			AddGetAddressNode((*str)->pos, InplaceStr((*str)->pos, (*str)->pos + (*str)->length));
			(*str)++;
			return true;
		}
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
		{
			Lexeme *curr = *str;
			NamespaceInfo* lastNS = GetCurrentNamespace();
			SetCurrentNamespace(NULL);
			NamespaceInfo* ns = NULL;
			while((*str)->type == lex_string && (*str + 1)->type == lex_point && (ns = IsNamespace(InplaceStr((*str)->pos, (*str)->length))) != NULL)
			{
				(*str) += 2;
				SetCurrentNamespace(ns);
			}
			if((*str + 1)->type == lex_oparen || (*str + 1)->type == lex_with)
			{
				ParseFunctionCall(str, false);
				ParsePostExpressions(str);
				SetCurrentNamespace(lastNS);
				return true;
			}
			SetCurrentNamespace(lastNS);
			*str = curr;
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
		unsigned nodeCount = CodeInfo::nodeList.size();
		if(ParseSelectType(str, ALLOW_ARRAY | ALLOW_EXTENDED_TYPEOF | ALLOW_NUMERIC_RESULT))
		{
			if(ParseFunctionDefinition(str, isCoroutine))
				return true;
			if(ParseLexem(str, lex_oparen))
			{
				TypeInfo *currType = GetSelectedType();
				const char *last = SetCurrentFunction(GetSelectedTypeName());
				unsigned int callArgCount = ParseFunctionArguments(str);
				if(!ParseLexem(str, lex_cparen))
					ThrowError((*str)->pos, "ERROR: ')' not found after function parameter list");
				SelectTypeByPointer(currType);
				SetCurrentFunction(last);
				if(callArgCount && !(callArgCount == 1 && CodeInfo::nodeList.back()->typeInfo == typeObject) && !HasConstructor(currType, callArgCount))
					ThrowError((*str)->pos, "ERROR: type '%s' doesn't have a constructor accepting %d argument(s)", currType->GetFullTypeName(), callArgCount);
				AddFunctionCallNode((*str)->pos, GetSelectedTypeName(), callArgCount);
				ParsePostExpressions(str);
				return true;
			}
			if(nodeCount == CodeInfo::nodeList.size())
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
			if(!lastIsFunctionCall && !((*str)->type >= lex_set && (*str)->type <= lex_xorset))
				AddGetVariableNode((*str)->pos);
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
	while((*str)->type >= lex_add && (*str)->type <= lex_in)
	{
		LexemeType lType = (*str)->type;
		while(opCount > 0 && opPrecedence[opStack.back() - lex_add] <= opPrecedence[lType - lex_add])
		{
			NamespaceInfo *lastNS = GetCurrentNamespace();
			SetCurrentNamespace(NULL);
			AddBinaryCommandNode((*str)->pos, opHandler[opStack.back() - lex_add]);
			SetCurrentNamespace(lastNS);
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
		NamespaceInfo *lastNS = GetCurrentNamespace();
		SetCurrentNamespace(NULL);
		AddBinaryCommandNode((*str)->pos, opHandler[opStack.back() - lex_add]);
		SetCurrentNamespace(lastNS);
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
		AddIfElseTermNode(condPos);
	}
	return true;
}

bool ParseVaribleSet(Lexeme** str)
{
	if(!ParseTernaryExpr(str))
		return false;

	Lexeme *curr = *str;
	if(ParseLexem(str, lex_set))
	{
		if(ParseVaribleSet(str))
			AddSetVariableNode((*str)->pos);
		else
			ThrowError((*str)->pos, "ERROR: expression not found after '='");
	}else if((*str)->type >= lex_addset && (*str)->type <= lex_xorset){
		(*str)++;
		CmdID cmdID[] = { cmdAdd, cmdSub, cmdMul, cmdDiv, cmdPow, cmdMod, cmdShl, cmdShr, cmdBitAnd, cmdBitOr, cmdBitXor };
		const char *cmdName[] = { "+=", "-=", "*=", "/=", "**=", "%=", "<<=", ">>=", "&=", "|=", "^=" };
		assert((unsigned)(curr->type - lex_addset) <= 10);
		if(ParseVaribleSet(str))
			AddModifyVariableNode((*str)->pos, cmdID[curr->type - lex_addset], cmdName[curr->type - lex_addset]);
		else
			ThrowError((*str)->pos, "ERROR: expression not found after '%s' operator", cmdName[curr->type - lex_addset]);
	}

	return true;
}

bool ParseBlock(Lexeme** str)
{
	if(!ParseLexem(str, lex_ofigure))
		return false;
	BeginBlock();
	if(!ParseCode(str))
		AddVoidNode();
	if(!ParseLexem(str, lex_cfigure))
		ThrowError((*str)->pos, "ERROR: closing '}' not found");
	EndBlock();
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
	AddAliasType(InplaceStr((*str)->pos, (*str)->length));
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
		if(!ParseClassDefinition(str))
			ThrowError((*str)->pos, "ERROR: variable or class definition is expected after alignment specifier");
		break;
	case lex_enum:
		ParseEnum(str);
		break;
	case lex_namespace:
		{
			(*str)++;
			unsigned count = 0;
			do
			{
				count++;
				if((*str)->type != lex_string)
					ThrowError((*str)->pos, "ERROR: namespace name required");
				PushNamespace(InplaceStr((*str)->pos, (*str)->length));
				(*str)++;
			}while(ParseLexem(str, lex_point));
			if(!ParseLexem(str, lex_ofigure))
				ThrowError((*str)->pos, "ERROR: '{' not found after namespace name");
			if(!ParseCode(str))
				AddVoidNode();
			if(!ParseLexem(str, lex_cfigure))
				ThrowError((*str)->pos, "ERROR: '}' not found after namespace body");
			while(count--)
				PopNamespace();
		}
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
		AddVoidNode();
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
			if(!isVarDef && ParseFunctionDefinition(str))
				return true;

			SetCurrentAlignment(0xFFFFFFFF);
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
			AddPopNode(pos);
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
