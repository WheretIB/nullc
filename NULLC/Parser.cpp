#include "Parser.h"
#include "CodeInfo.h"
using namespace CodeInfo;

#include "Callbacks.h"

ChunkedStackPool<4092>	stringPool;

char*	AllocateString(unsigned int size)
{
	return (char*)stringPool.Allocate(size);
}

void ClearStringList()
{
	stringPool.Clear();
}

#define CALLBACK(x) x
//#define CALLBACK(x) 1

inline bool ParseLexem(Lexeme** str, LexemeType type)
{
	if((*str)->type != type)
		return false;
	(*str)++;
	return true;
}

bool ParseTypename(Lexeme** str)
{
	if((*str)->type != lex_string)
		return false;

	unsigned int hash = 5381;
	for(unsigned int i = 0; i < (*str)->length; i++)
		hash = ((hash << 5) + hash) + (*str)->pos[i];

	for(unsigned int s = 0, e = CodeInfo::typeInfo.size(); s != e; s++)
	{
		if(CodeInfo::typeInfo[s]->nameHash == hash)
		{
			(*str)++;
			return true;
		}
	}
	return false;
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
			ThrowError("ERROR: '0x' must be followed by number", start+2);
		CALLBACK(addHexInt(number->pos, number->pos+number->length));
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
			CALLBACK(addBinInt(number->pos, number->pos+number->length));
			return true;
		}else if((*str)->pos[0] == 'l'){
			(*str)++;
			CALLBACK(addNumberNodeLong(number->pos, number->pos+number->length));
			return true;
		}else if(number->pos[0] == '0' && isDigit(number->pos[1])){
			CALLBACK(addOctInt(number->pos, number->pos+number->length));
			return true;
		}else{
			CALLBACK(addNumberNodeInt(number->pos, number->pos+number->length));
			return true;
		}
	}else{
		if((*str)->pos[0] == 'f')
		{
			(*str)++;
			CALLBACK(addNumberNodeFloat(number->pos, number->pos+number->length));
			return true;
		}else{
			CALLBACK(addNumberNodeDouble(number->pos, number->pos+number->length));
			return true;
		}
	}
}

bool ParseArrayDefinition(Lexeme** str)
{
	if(!ParseLexem(str, lex_obracket))
		return false;

	if(!ParseTernaryExpr(str))
		CALLBACK(addUnfixedArraySize((*str)->pos, (*str)->pos));
	if(!ParseLexem(str, lex_cbracket))
		ThrowError("ERROR: Matching ']' not found", (*str)->pos);
	ParseArrayDefinition(str);
	CALLBACK(convertTypeToArray((*str)->pos, (*str)->pos));
	return true;
}

bool ParseSelectType(Lexeme** str)
{
	if((*str)->type == lex_typeof)
	{
		(*str)++;
		if(!ParseLexem(str, lex_oparen))
			ThrowError("ERROR: typeof must be followed by '('", (*str)->pos);
		if(ParseVaribleSet(str))
		{
			CALLBACK(SetTypeOfLastNode(NULL, NULL));
			if(!ParseLexem(str, lex_cparen))
				ThrowError("ERROR: ')' not found after expression in typeof", (*str)->pos);
		}else{
			ThrowError("ERROR: expression not found after typeof(", (*str)->pos);
		}
	}else if((*str)->type == lex_auto){
		CALLBACK(SelectTypeByName((*str)->pos, "auto"));
		(*str)++;
	}else if((*str)->type == lex_string){
		if(!ParseTypename(str))
			return false;
		(*str)--;
		char	*typeName = (char*)stringPool.Allocate((*str)->length+1);
		if((*str)->length >= NULLC_MAX_VARIABLE_NAME_LENGTH)
			ThrowError("ERROR: type name length is limited to 2048 symbols", (*str)->pos);
		memcpy(typeName, (*str)->pos, (*str)->length);
		typeName[(*str)->length] = 0;
		CALLBACK(SelectTypeByName((*str)->pos, typeName));
		(*str)++;
	}else{
		return false;
	}

	for(;;)
	{
		if(ParseLexem(str, lex_ref))
		{
			CALLBACK(convertTypeToRef(NULL, NULL));
		}else{
			if(!ParseArrayDefinition(str))
				break;
		}
	}
	return true;
}

bool ParseIsConst(Lexeme** str)
{
	CALLBACK(SetTypeConst(false));
	if(ParseLexem(str, lex_const))
		CALLBACK(SetTypeConst(true));
	return true;
}

bool ParseClassDefinition(Lexeme** str)
{
	if(!ParseAlignment(str))
		CALLBACK(SetCurrentAlignment(0));

	if(ParseLexem(str, lex_class))
	{
		if((*str)->type != lex_string)
			ThrowError("ERROR: class name expected", (*str)->pos);
		CALLBACK(TypeBegin((*str)->pos, (*str)->pos+(*str)->length));
		(*str)++;
		if(!ParseLexem(str, lex_ofigure))
			ThrowError("ERROR: '{' not found after class name", (*str)->pos);

		for(;;)
		{
			if(!ParseFunctionDefinition(str))
			{
				if(!ParseSelectType(str))
					break;

				if((*str)->type != lex_string)
					ThrowError("ERROR: class member name expected after type", (*str)->pos);
				if((*str)->length >= NULLC_MAX_VARIABLE_NAME_LENGTH)
					ThrowError("ERROR: member name length is limited to 2048 symbols", (*str)->pos);
				char	*memberName = (char*)stringPool.Allocate((*str)->length+1);
				memcpy(memberName, (*str)->pos, (*str)->length);
				memberName[(*str)->length] = 0;
				CALLBACK(TypeAddMember((*str)->pos, memberName));
				(*str)++;

				while(ParseLexem(str, lex_comma))
				{
					if((*str)->type != lex_string)
						ThrowError("ERROR: member name expected after ','", (*str)->pos);
					if((*str)->length >= NULLC_MAX_VARIABLE_NAME_LENGTH)
						ThrowError("ERROR: member name length is limited to 2048 symbols", (*str)->pos);
					char	*memberName = (char*)stringPool.Allocate((*str)->length+1);
					memcpy(memberName, (*str)->pos, (*str)->length);
					memberName[(*str)->length] = 0;
					CALLBACK(TypeAddMember((*str)->pos, memberName));
					(*str)++;
				}
				if(!ParseLexem(str, lex_semicolon))
					ThrowError("ERROR: ';' not found after class member list", (*str)->pos);
			}
		}
		if(!ParseLexem(str, lex_cfigure))
			ThrowError("ERROR: '}' not found after class definition", (*str)->pos);
		CALLBACK(TypeFinish(NULL, NULL));
		return true;
	}
	return false;
}

bool ParseFunctionCall(Lexeme** str, bool memberFunctionCall)
{
	if((*str)->type != lex_string || (*str)[1].type != lex_oparen)
		return false;

	if((*str)->length >= NULLC_MAX_VARIABLE_NAME_LENGTH)
		ThrowError("ERROR: function name length is limited to 2048 symbols", (*str)->pos);
	char	*functionName = (char*)stringPool.Allocate((*str)->length+1);
	memcpy(functionName, (*str)->pos, (*str)->length);
	functionName[(*str)->length] = 0;
	(*str) += 2;

	unsigned int callArgCount = 0;
	if(ParseVaribleSet(str))
	{
		callArgCount++;
		while(ParseLexem(str, lex_comma))
		{
			if(!ParseVaribleSet(str))
				ThrowError("ERROR: expression not found after ',' in function parameter list", (*str)->pos);
			callArgCount++;
		}
	}
	if(!ParseLexem(str, lex_cparen))
		ThrowError("ERROR: ')' not found after function parameter list", (*str)->pos);

	if(memberFunctionCall)
		CALLBACK(AddMemberFunctionCall((*str)->pos, functionName, callArgCount));
	else
		CALLBACK(AddFunctionCallNode((*str)->pos, functionName, callArgCount));

	return true;
}

bool ParseFunctionVariables(Lexeme** str)
{
	ParseIsConst(str);
	if(!ParseSelectType(str))
		return true;

	if((*str)->type != lex_string)
		ThrowError("ERROR: variable name not found after type in function variable list", (*str)->pos);

	if((*str)->length >= NULLC_MAX_VARIABLE_NAME_LENGTH)
		ThrowError("ERROR: parameter name length is limited to 2048 symbols", (*str)->pos);
	char	*paramName = (char*)stringPool.Allocate((*str)->length+1);
	memcpy(paramName, (*str)->pos, (*str)->length);
	paramName[(*str)->length] = 0;
	CALLBACK(FunctionParameter((*str)->pos, paramName));
	(*str)++;

	while(ParseLexem(str, lex_comma))
	{
		ParseIsConst(str);
		if(!ParseSelectType(str))
			ThrowError("ERROR: type name not found after ',' in function variable list", (*str)->pos);

		if((*str)->type != lex_string)
			ThrowError("ERROR: variable name not found after type in function variable list", (*str)->pos);
		if((*str)->length >= NULLC_MAX_VARIABLE_NAME_LENGTH)
			ThrowError("ERROR: parameter name length is limited to 2048 symbols", (*str)->pos);
		char	*paramName = (char*)stringPool.Allocate((*str)->length+1);
		memcpy(paramName, (*str)->pos, (*str)->length);
		paramName[(*str)->length] = 0;
		CALLBACK(FunctionParameter((*str)->pos, paramName));
		(*str)++;
	}
	return true;
}

bool ParseFunctionDefinition(Lexeme** str)
{
	Lexeme *start = *str;
	if(!ParseSelectType(str))
		return false;

	if((*str)->type != lex_string || (*str)[1].type != lex_oparen)
	{
		*str = start;
		return false;
	}
	if((*str)->length >= NULLC_MAX_VARIABLE_NAME_LENGTH)
		ThrowError("ERROR: function name length is limited to 2048 symbols", (*str)->pos);
	char	*functionName = (char*)stringPool.Allocate((*str)->length+1);
	memcpy(functionName, (*str)->pos, (*str)->length);
	functionName[(*str)->length] = 0;
	(*str) += 2;

	CALLBACK(FunctionAdd((*str)->pos, functionName));

	ParseFunctionVariables(str);
	CALLBACK(FunctionStart((*str)->pos));

	if(!ParseLexem(str, lex_cparen))
		ThrowError("ERROR: ')' not found after function variable list", (*str)->pos);
	if(!ParseLexem(str, lex_ofigure))
		ThrowError("ERROR: '{' not found after function header", (*str)->pos);

	if(!ParseCode(str))
		CALLBACK(addVoidNode(NULL, NULL));
	CALLBACK(FunctionEnd((*str)->pos, functionName));
	
	if(!ParseLexem(str, lex_cfigure))
		ThrowError("ERROR: '}' not found after function body", (*str)->pos);
	return true;
}

bool ParseFunctionPrototype(Lexeme** str)
{
	if(!ParseSelectType(str))
		ThrowError("ERROR: function prototype must begin with type name", (*str)->pos);

	if((*str)->type != lex_string)
		ThrowError("ERROR: function not found after type", (*str)->pos);
	if((*str)[1].type != lex_oparen)
		ThrowError("ERROR: '(' not found after function name", (*str)->pos);

	if((*str)->length >= NULLC_MAX_VARIABLE_NAME_LENGTH)
		ThrowError("ERROR: function name length is limited to 2048 symbols", (*str)->pos);
	char	*functionName = (char*)stringPool.Allocate((*str)->length+1);
	memcpy(functionName, (*str)->pos, (*str)->length);
	functionName[(*str)->length] = 0;
	(*str) += 2;

	CALLBACK(FunctionAdd((*str)->pos, functionName));

	ParseFunctionVariables(str);

	if(!ParseLexem(str, lex_cparen))
		ThrowError("ERROR: ')' not found after function variable list", (*str)->pos);
	if(!ParseLexem(str, lex_semicolon))
		ThrowError("ERROR: ';' not found after function header", (*str)->pos);
	return true;
}

bool ParseAddVariable(Lexeme** str)
{
	if((*str)->type != lex_string)
		return false;

	if((*str)->length >= NULLC_MAX_VARIABLE_NAME_LENGTH)
		ThrowError("ERROR: variable name length is limited to 2048 symbols", (*str)->pos);

	char	*varName = (char*)stringPool.Allocate((*str)->length+1);
	memcpy(varName, (*str)->pos, (*str)->length);
	varName[(*str)->length] = 0;

	(*str)++;

	if(ParseLexem(str, lex_obracket))
	{
		if(!ParseTernaryExpr(str))
			CALLBACK(addUnfixedArraySize(NULL, NULL));
		if(!ParseLexem(str, lex_cbracket))
			ThrowError("ERROR: Matching ']' not found", (*str)->pos);
		CALLBACK(convertTypeToArray(NULL, NULL));
	}
	CALLBACK(pushType(NULL, NULL));
	CALLBACK(AddVariable((*str)->pos, varName));

	if(ParseLexem(str, lex_set))
	{
		if(!ParseVaribleSet(str))
			ThrowError("ERROR: expression not found after '='", (*str)->pos);
		CALLBACK(AddDefineVariableNode((*str)->pos, varName));
		CALLBACK(addPopNode(NULL, NULL));
		CALLBACK(popType(NULL, NULL));
	}else{
		CALLBACK(AddVariableReserveNode((*str)->pos));
	}
	CALLBACK(popType(NULL, NULL));
	return true;
}

bool ParseVariableDefineSub(Lexeme** str)
{
	const char *old = (*str)->pos;
	if(!ParseAddVariable(str))
		return false;
	CALLBACK(SetStringToLastNode(old, (*str)->pos));

	while(ParseLexem(str, lex_comma))
	{
		if(!ParseAddVariable(str))
			ThrowError("ERROR: next variable definition excepted after ','", (*str)->pos);
		CALLBACK(addTwoExprNode(NULL, NULL));
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
			ThrowError("ERROR: '(' expected after align", (*str)->pos);
		
		const char *start = (*str)->pos;
		if(!ParseLexem(str, lex_number))
			ThrowError("ERROR: alignment value not found after align(", (*str)->pos);
		CALLBACK(SetCurrentAlignment(atoi(start)));
		if(!ParseLexem(str, lex_cparen))
			ThrowError("ERROR: ')' expected after alignment value", (*str)->pos);
		return true;
	}
	return false;
}

bool ParseVariableDefine(Lexeme** str)
{
	Lexeme *curr = *str;
	CALLBACK(SetCurrentAlignment(0xFFFFFFFF));
	if(!ParseIsConst(&curr))
		return false;
	ParseAlignment(&curr);
	if(!ParseSelectType(&curr))
		return false;
	if(!ParseVariableDefineSub(&curr))
		return false;
	*str = curr;
	return true;
}

bool ParseIfExpr(Lexeme** str)
{
	const char *pos = (*str)->pos;
	if(!ParseLexem(str, lex_if))
		return false;

	if(!ParseLexem(str, lex_oparen))
		ThrowError("ERROR: '(' not found after 'if'", (*str)->pos);
	if(!ParseVaribleSet(str))
		ThrowError("ERROR: condition not found in 'if' statement", (*str)->pos);
	if(!ParseLexem(str, lex_cparen))
		ThrowError("ERROR: closing ')' not found after 'if' condition", (*str)->pos);
	CALLBACK(SaveStringIndex(pos, (*str)->pos));
	if(!ParseExpression(str))
		ThrowError("ERROR: expression not found after 'if'", (*str)->pos);

	if(ParseLexem(str, lex_else))
	{
		if(!ParseExpression(str))
			ThrowError("ERROR: expression not found after 'else'", (*str)->pos);
		CALLBACK(addIfElseNode(NULL, NULL));
	}else{
		CALLBACK(addIfNode(NULL, NULL));
	}
	CALLBACK(SetStringFromIndex(NULL, NULL));
	return true;
}

bool  ParseForExpr(Lexeme** str)
{
	const char *pos = (*str)->pos;
	if(!ParseLexem(str, lex_for))
		return false;
	
	CALLBACK(saveVarTop(NULL, NULL));
	
	if(!ParseLexem(str, lex_oparen))
		ThrowError("ERROR: '(' not found after 'for'", (*str)->pos);
	
	if(ParseLexem(str, lex_ofigure))
	{
		if(!ParseCode(str))
			CALLBACK(addVoidNode(NULL, NULL));
		if(!ParseLexem(str, lex_cfigure))
			ThrowError("ERROR: '}' not found after '{'", (*str)->pos);
	}else{
		if(!ParseVariableDefine(str))
		{
			if(!ParseVaribleSet(str))
				CALLBACK(addVoidNode(NULL, NULL));
			else
				CALLBACK(addPopNode(NULL, NULL));
		}
	}

	if(!ParseLexem(str, lex_semicolon))
		ThrowError("ERROR: ';' not found after initializer in 'for'", (*str)->pos);

	if(!ParseVaribleSet(str))
		ThrowError("ERROR: condition not found in 'for' statement", (*str)->pos);

	if(!ParseLexem(str, lex_semicolon))
		ThrowError("ERROR: ';' not found after condition in 'for'", (*str)->pos);

	if(ParseLexem(str, lex_ofigure))
	{
		if(!ParseCode(str))
			CALLBACK(addVoidNode(NULL, NULL));
		if(!ParseLexem(str, lex_cfigure))
			ThrowError("ERROR: '}' not found after '{'", (*str)->pos);
	}else{
		if(!ParseVaribleSet(str))
			CALLBACK(addVoidNode(NULL, NULL));
		else
			CALLBACK(addPopNode(NULL, NULL));
	}

	if(!ParseLexem(str, lex_cparen))
		ThrowError("ERROR: ')' not found after 'for' statement", (*str)->pos);

	CALLBACK(SaveStringIndex(pos, (*str)->pos));

	if(!ParseExpression(str))
		ThrowError("ERROR: body not found after 'for' header", (*str)->pos);
	CALLBACK(addForNode(NULL, NULL));
	CALLBACK(SetStringFromIndex(NULL, NULL));
	return true;

}

bool  ParseWhileExpr(Lexeme** str)
{
	if(!ParseLexem(str, lex_while))
		return false;
	
	CALLBACK(saveVarTop(NULL, NULL));
	if(!ParseLexem(str, lex_oparen))
		ThrowError("ERROR: '(' not found after 'while'", (*str)->pos);
	if(!ParseVaribleSet(str))
		ThrowError("ERROR: expression expected after 'while('", (*str)->pos);
	if(!ParseLexem(str, lex_cparen))
		ThrowError("ERROR: closing ')' not found after expression in 'while' statement", (*str)->pos);

	if(!ParseExpression(str))
		ThrowError("ERROR: expression expected after 'while(...)'", (*str)->pos);
	CALLBACK(addWhileNode(NULL, NULL));
	return true;
}

bool  ParseDoWhileExpr(Lexeme** str)
{
	if(!ParseLexem(str, lex_do))
		return false;

	CALLBACK(saveVarTop(NULL, NULL));

	if(!ParseExpression(str))
		ThrowError("ERROR: expression expected after 'do'", (*str)->pos);

	if(!ParseLexem(str, lex_while))
		ThrowError("ERROR: 'while' expected after 'do' statement", (*str)->pos);
	if(!ParseLexem(str, lex_oparen))
		ThrowError("ERROR: '(' not found after 'while'", (*str)->pos);
	if(!ParseVaribleSet(str))
		ThrowError("ERROR: expression expected after 'while('", (*str)->pos);
	if(!ParseLexem(str, lex_cparen))
		ThrowError("ERROR: closing ')' not found after expression in 'while' statement", (*str)->pos);

	CALLBACK(addDoWhileNode(NULL, NULL));

	if(!ParseLexem(str, lex_semicolon))
		ThrowError("ERROR: while(...) should be followed by ';'", (*str)->pos);
	return true;
}

bool  ParseSwitchExpr(Lexeme** str)
{
	if(!ParseLexem(str, lex_switch))
		return false;

	if(!ParseLexem(str, lex_oparen))
		ThrowError("ERROR: '(' not found after 'switch'", (*str)->pos);

	if(!ParseVaribleSet(str))
		ThrowError("ERROR: expression not found after 'switch('", (*str)->pos);
	CALLBACK(preSwitchNode(NULL, NULL));

	if(!ParseLexem(str, lex_cparen))
		ThrowError("ERROR: closing ')' not found after expression in 'switch' statement", (*str)->pos);

	if(!ParseLexem(str, lex_ofigure))
		ThrowError("ERROR: '{' not found after 'switch(...)'", (*str)->pos);

	while(ParseLexem(str, lex_case))
	{
		if(!ParseVaribleSet(str))
			ThrowError("ERROR: expression expected after 'case'", (*str)->pos);
		if(!ParseLexem(str, lex_colon))
			ThrowError("ERROR: ':' not found after 'case' expression", (*str)->pos);

		if(!ParseExpression(str))
			ThrowError("ERROR: expression expected after 'case:'", (*str)->pos);
		while(ParseExpression(str))
			CALLBACK(addTwoExprNode(NULL, NULL));
		CALLBACK(addCaseNode(NULL, NULL));
	}

	if(!ParseLexem(str, lex_cfigure))
		ThrowError("ERROR: '}' not found after 'switch' statement", (*str)->pos);
	CALLBACK(addSwitchNode(NULL, NULL));
	return true;
}

bool  ParseReturnExpr(Lexeme** str)
{
	if(!ParseLexem(str, lex_return))
		return false;

	if(!ParseVaribleSet(str))
		CALLBACK(addVoidNode(NULL, NULL));

	if(!ParseLexem(str, lex_semicolon))
		ThrowError("ERROR: return must be followed by ';'", (*str)->pos);
	CALLBACK(addReturnNode(NULL, NULL));
	return true;
}

bool  ParseBreakExpr(Lexeme** str)
{
	if(!ParseLexem(str, lex_break))
		return false;

	if(!ParseLexem(str, lex_semicolon))
		ThrowError("ERROR: break must be followed by ';'", (*str)->pos);
	CALLBACK(addBreakNode(NULL, NULL));
	return true;
}

bool  ParseContinueExpr(Lexeme** str)
{
	if(!ParseLexem(str, lex_continue))
		return false;

	if(!ParseLexem(str, lex_semicolon))
		ThrowError("ERROR: continue must be followed by ';'", (*str)->pos);
	CALLBACK(AddContinueNode(NULL, NULL));
	return true;
}

bool  ParseGroup(Lexeme** str)
{
	if(!ParseLexem(str, lex_oparen))
		return false;

	if(!ParseVaribleSet(str))
		ThrowError("ERROR: expression not found after '('", (*str)->pos);
	if(!ParseLexem(str, lex_cparen))
		ThrowError("ERROR: closing ')' not found after '('", (*str)->pos);
	return true;
}

bool  ParseVariable(Lexeme** str)
{
	if(ParseLexem(str, lex_mul))
	{
		if(!ParseVariable(str))
			ThrowError("ERROR: variable name not found after '*'", (*str)->pos);
		CALLBACK(AddDereferenceNode(NULL, NULL));
		return true;
	}
	
	if((*str)->type != lex_string || (*str)[1].type == lex_oparen)
		return false;

	if((*str)->length >= NULLC_MAX_VARIABLE_NAME_LENGTH)
		ThrowError("ERROR: variable name length is limited to 2048 symbols", (*str)->pos);

	char	*varName = (char*)stringPool.Allocate((*str)->length+1);
	memcpy(varName, (*str)->pos, (*str)->length);
	varName[(*str)->length] = 0;

	CALLBACK(AddGetAddressNode((*str)->pos, varName));
	(*str)++;

	while(ParsePostExpression(str));
	return true;
}

bool  ParsePostExpression(Lexeme** str)
{
	Lexeme *start = *str;
	if(ParseLexem(str, lex_point))
	{
		if((*str)->type != lex_string)
			ThrowError("ERROR: member variable expected after '.'", (*str)->pos);
		if((*str)[1].type == lex_oparen)
		{
			*str = start;
			return false;
		}
		if((*str)->length >= NULLC_MAX_VARIABLE_NAME_LENGTH)
			ThrowError("ERROR: variable name length is limited to 2048 symbols", (*str)->pos);

		char	*varName = (char*)stringPool.Allocate((*str)->length+1);
		memcpy(varName, (*str)->pos, (*str)->length);
		varName[(*str)->length] = 0;
		(*str)++;

		CALLBACK(AddMemberAccessNode((*str)->pos, varName));
	}else if(ParseLexem(str, lex_obracket)){
		if(!ParseVaribleSet(str))
			ThrowError("ERROR: expression not found after '['", (*str)->pos);
		if(!ParseLexem(str, lex_cbracket))
			ThrowError("ERROR: ']' not found after expression", (*str)->pos);
		CALLBACK(AddArrayIndexNode(NULL, NULL));
	}else{
		return false;
	}
	return true;
}

bool ParseTerminal(Lexeme** str)
{
	if((*str)->type == lex_number)
		return ParseNumber(str);
	if(ParseLexem(str, lex_bitand))
	{
		if(!ParseVariable(str))
			ThrowError("ERROR: variable not found after '&'", (*str)->pos);
		CALLBACK(popType(NULL, NULL));
		return true;
	}
	if(ParseLexem(str, lex_lognot))
	{
		if(!ParseTerminal(str))
			ThrowError("ERROR: expression not found after '!'", (*str)->pos);
		CALLBACK(addLogNotNode(NULL, NULL));
		return true;
	}
	if(ParseLexem(str, lex_bitnot))
	{
		if(!ParseTerminal(str))
			ThrowError("ERROR: expression not found after '~'", (*str)->pos);
		CALLBACK(addBitNotNode(NULL, NULL));
		return true;
	}
	if(ParseLexem(str, lex_dec))
	{
		if(!ParseVariable(str))
			ThrowError("ERROR: variable not found after '--'", (*str)->pos);
		CALLBACK(AddPreOrPostOpNode(false, true));
		CALLBACK(popType(NULL, NULL));
		return true;
	}
	if(ParseLexem(str, lex_inc))
	{
		if(!ParseVariable(str))
			ThrowError("ERROR: variable not found after '++'", (*str)->pos);
		CALLBACK(AddPreOrPostOpNode(true, true));
		CALLBACK(popType(NULL, NULL));
		return true;
	}
	if(ParseLexem(str, lex_add))
	{
		while(ParseLexem(str, lex_add));
		if(!ParseTerminal(str))
			ThrowError("ERROR: expression not found after '+'", (*str)->pos);
		return true;
	}
	if(ParseLexem(str, lex_sub))
	{
		int negCount = 1;
		while(ParseLexem(str, lex_sub))
			negCount++;
		if(!ParseTerminal(str))
			ThrowError("ERROR: expression not found after '-'", (*str)->pos);
		if(negCount % 2 == 1)
			CALLBACK(addNegNode(NULL, NULL));
		return true;
	}
	if((*str)->type == lex_quotedstring)
	{
		CALLBACK(addStringNode((*str)->pos, (*str)->pos+(*str)->length));
		(*str)++;
		return true;
	}
	if((*str)->type == lex_semiquote)
	{
		const char *start = (*str)->pos;
		(*str)++;
		ParseLexem(str, lex_escape);
		if(!ParseLexem(str, lex_string) && !ParseLexem(str, lex_number))
			ThrowError("ERROR: character not found after '", (*str)->pos);
		if(!ParseLexem(str, lex_semiquote))
			ThrowError("ERROR: ' not found after character", (*str)->pos);
		CALLBACK(addNumberNodeChar(start, (*str)->pos));
		return true;
	}
	if(ParseLexem(str, lex_sizeof))
	{
		if(!ParseLexem(str, lex_oparen))
			ThrowError("ERROR: sizeof must be followed by '('", (*str)->pos);
		CALLBACK(pushType(NULL, NULL));
		if(ParseSelectType(str))
		{
			CALLBACK(pushType(NULL, NULL));
			CALLBACK(GetTypeSize(NULL, NULL, false));
			CALLBACK(popType(NULL, NULL));
		}else{
			if(ParseVaribleSet(str))
				CALLBACK(GetTypeSize(NULL, NULL, true));
			else
				ThrowError("ERROR: expression or type not found after sizeof(", (*str)->pos);
		}
		if(!ParseLexem(str, lex_cparen))
			ThrowError("ERROR: ')' not found after expression in sizeof", (*str)->pos);
		CALLBACK(popType(NULL, NULL));
		return true;
	}
	if(ParseLexem(str, lex_ofigure))
	{
		unsigned int arrElementCount = 0;
		if(!ParseTernaryExpr(str))
			ThrowError("ERROR: value not found after '{'", (*str)->pos);
		while(ParseLexem(str, lex_comma))
		{
			if(!ParseTernaryExpr(str))
				ThrowError("ERROR: value not found after ','", (*str)->pos);
			arrElementCount++;
		}
		if(!ParseLexem(str, lex_cfigure))
			ThrowError("ERROR: '}' not found after inline array", (*str)->pos);
		CALLBACK(addArrayConstructor(NULL, NULL, arrElementCount));
		return true;
	}
	if(ParseGroup(str))
		return true;
	if(((*str)->type == lex_typeof && (*str)[1].type == lex_oparen) || ((*str)->type == lex_string) && ((*str)[1].type == lex_string || (*str)[1].type == lex_ref || (*str)[1].type == lex_obracket))
		if(ParseFunctionDefinition(str))
			return true;
	if(ParseFunctionCall(str, false))
		return true;
	if(ParseVariable(str))
	{
		if(ParseLexem(str, lex_dec))
		{
			CALLBACK(AddPreOrPostOpNode(false, false));
		}else if(ParseLexem(str, lex_inc))
		{
			CALLBACK(AddPreOrPostOpNode(true, false));
		}else if(ParseLexem(str, lex_point)){
			if(!ParseFunctionCall(str, true))
				ThrowError("ERROR: function call is excepted after '.'", (*str)->pos);
		}else{
			CALLBACK(AddGetVariableNode(NULL, NULL));
		}
		CALLBACK(popType(NULL, NULL));
		return true;
	}
	return false;
}

bool ParsePower(Lexeme** str)
{
	if(!ParseTerminal(str))
		return false;
	while(ParseLexem(str, lex_pow))
	{
		if(!ParseTerminal(str))
			ThrowError("ERROR: expression not found after **", (*str)->pos);
		CALLBACK((addCmd(cmdPow))(NULL, NULL));
	}
	return true;
}

bool ParseMultiplicative(Lexeme** str)
{
	if(!ParsePower(str))
		return false;
	while(ParseLexem(str, lex_mul) || ParseLexem(str, lex_div) || ParseLexem(str, lex_mod))
	{
		char op = (*str)[-1].pos[0];
		if(!ParsePower(str))
			ThrowError("ERROR: expression not found after multiplicative expression", (*str)->pos);
		CALLBACK((addCmd((CmdID)(op == '*' ? cmdMul : (op == '/' ? cmdDiv : cmdMod))))(NULL, NULL));
	}
	return true;
}

bool ParseAdditive(Lexeme** str)
{
	if(!ParseMultiplicative(str))
		return false;
	while(ParseLexem(str, lex_add) || ParseLexem(str, lex_sub))
	{
		char op = (*str)[-1].pos[0];
		if(!ParseMultiplicative(str))
			ThrowError("ERROR: expression not found after additive expression", (*str)->pos);
		CALLBACK((addCmd((CmdID)(op == '+' ? cmdAdd : cmdSub)))(NULL, NULL));
	}
	return true;
}

bool ParseBinaryShift(Lexeme** str)
{
	if(!ParseAdditive(str))
		return false;
	while(ParseLexem(str, lex_shl) || ParseLexem(str, lex_shr))
	{
		char op = (*str)[-1].pos[0];
		if(!ParseAdditive(str))
			ThrowError("ERROR: expression not found after shift expression", (*str)->pos);
		CALLBACK((addCmd((CmdID)(op == '<' ? cmdShl : cmdShr)))(NULL, NULL));
	}
	return true;
}

bool ParseComparision(Lexeme** str)
{
	if(!ParseBinaryShift(str))
		return false;
	while(ParseLexem(str, lex_less) || ParseLexem(str, lex_lequal) || ParseLexem(str, lex_greater) || ParseLexem(str, lex_gequal))
	{
		char op = (*str)[-1].pos[0];
		char op2 = (*str)[-1].pos[1];
		if(!ParseBinaryShift(str))
			ThrowError("ERROR: expression not found after comparison expression", (*str)->pos);
		CALLBACK((addCmd((CmdID)(op == '<' ? (op2 == '=' ? cmdLEqual : cmdLess) : (op2 == '=' ? cmdGEqual : cmdGreater))))(NULL, NULL));
	}
	return true;
}

bool  ParseStrongComparision(Lexeme** str)
{
	if(!ParseComparision(str))
		return false;
	while(ParseLexem(str, lex_equal) || ParseLexem(str, lex_nequal))
	{
		char op = (*str)[-1].pos[0];
		if(!ParseComparision(str))
			ThrowError("ERROR: expression not found after comparison expression", (*str)->pos);
		CALLBACK((addCmd((CmdID)(op == '=' ? cmdEqual : cmdNEqual)))(NULL, NULL));
	}
	return true;
}

bool ParseBinaryAnd(Lexeme** str)
{
	if(!ParseStrongComparision(str))
		return false;
	while(ParseLexem(str, lex_bitand))
	{
		if(!ParseStrongComparision(str))
			ThrowError("ERROR: expression not found after '&'", (*str)->pos);
		CALLBACK((addCmd(cmdBitAnd))(NULL, NULL));
	}
	return true;
}

bool ParseBinaryXor(Lexeme** str)
{
	if(!ParseBinaryAnd(str))
		return false;
	while(ParseLexem(str, lex_bitxor))
	{
		if(!ParseBinaryAnd(str))
			ThrowError("ERROR: expression not found after '^'", (*str)->pos);
		CALLBACK((addCmd(cmdBitXor))(NULL, NULL));
	}
	return true;
}

bool ParseBinaryOr(Lexeme** str)
{
	if(!ParseBinaryXor(str))
		return false;
	while(ParseLexem(str, lex_bitor))
	{
		if(!ParseBinaryXor(str))
			ThrowError("ERROR: expression not found after '|'", (*str)->pos);
		CALLBACK((addCmd(cmdBitOr))(NULL, NULL));
	}
	return true;
}

bool ParseLogicalAnd(Lexeme** str)
{
	if(!ParseBinaryOr(str))
		return false;
	while(ParseLexem(str, lex_logand))
	{
		if(!ParseBinaryOr(str))
			ThrowError("ERROR: expression not found after 'and'", (*str)->pos);
		CALLBACK((addCmd(cmdLogAnd))(NULL, NULL));
	}
	return true;
}

bool ParseLogicalXor(Lexeme** str)
{
	if(!ParseLogicalAnd(str))
		return false;
	while(ParseLexem(str, lex_logxor))
	{
		if(!ParseLogicalAnd(str))
			ThrowError("ERROR: expression not found after 'xor'", (*str)->pos);
		CALLBACK((addCmd(cmdLogXor))(NULL, NULL));
	}
	return true;
}

bool ParseLogicalOr(Lexeme** str)
{
	if(!ParseLogicalXor(str))
		return false;
	while(ParseLexem(str, lex_logor))
	{
		if(!ParseLogicalXor(str))
			ThrowError("ERROR: expression not found after 'or'", (*str)->pos);
		CALLBACK((addCmd(cmdLogOr))(NULL, NULL));
	}
	return true;
}

bool ParseTernaryExpr(Lexeme** str)
{
	if(!ParseLogicalOr(str))
		return false;
	while(ParseLexem(str, lex_questionmark))
	{
		if(!ParseVaribleSet(str))
			ThrowError("ERROR: expression not found after '?'", (*str)->pos);
		if(!ParseLexem(str, lex_colon))
			ThrowError("ERROR: ':' not found after expression in ternary operator", (*str)->pos);
		if(!ParseVaribleSet(str))
			ThrowError("ERROR: expression not found after ':'", (*str)->pos);
		CALLBACK(addIfElseTermNode(NULL, NULL));
	}
	return true;
}

bool ParseVaribleSet(Lexeme** str)
{
	if(((*str)->type == lex_typeof && (*str)[1].type == lex_oparen) || ((*str)->type == lex_string) && ((*str)[1].type == lex_string || (*str)[1].type == lex_ref || (*str)[1].type == lex_obracket))
		if(ParseFunctionDefinition(str))
			return true;
	
	Lexeme *start = *str;
	if(ParseVariable(str))
	{
		char op = (*str)->pos[0];
		if(ParseLexem(str, lex_set))
		{
			if(ParseVaribleSet(str))
			{
				CALLBACK(AddSetVariableNode(NULL, NULL));
				CALLBACK(popType(NULL, NULL));
				return true;
			}else{
				CALLBACK(FailedSetVariable(NULL, NULL));
				CALLBACK(popType(NULL, NULL));
				*str = start;
			}
		}else if(ParseLexem(str, lex_addset) || ParseLexem(str, lex_subset) || ParseLexem(str, lex_mulset) || ParseLexem(str, lex_divset)){
			if(ParseVaribleSet(str))
			{
				CALLBACK(AddModifyVariableNode(NULL, NULL, (CmdID)(op == '+' ? cmdAdd : (op == '-' ? cmdSub : (op == '*' ? cmdMul : cmdDiv)))));
				CALLBACK(popType(NULL, NULL));
				return true;
			}else{
				ThrowError("ERROR: expression not found after assignment operator", (*str)->pos);
			}
		}else if(ParseLexem(str, lex_powset)){
			if(ParseVaribleSet(str))
			{
				CALLBACK(AddModifyVariableNode(NULL, NULL, cmdPow));
				CALLBACK(popType(NULL, NULL));
				return true;
			}else{
				ThrowError("ERROR: expression not found after '**='", (*str)->pos);
			}
		}else{
			CALLBACK(FailedSetVariable(NULL, NULL));
			CALLBACK(popType(NULL, NULL));
			*str = start;
		}
	}
	if(!ParseTernaryExpr(str))
		return false;
	return true;
}

bool ParseBlock(Lexeme** str)
{
	if(!ParseLexem(str, lex_ofigure))
		return false;
	CALLBACK(blockBegin(NULL, NULL));
	if(!ParseCode(str))
		ThrowError("ERROR: {} block cannot be empty", (*str)->pos);
	if(!ParseLexem(str, lex_cfigure))
		ThrowError("ERROR: closing '}' not found", (*str)->pos);
	CALLBACK(blockEnd(NULL, NULL));
	return true;
}

bool ParseExpression(Lexeme** str)
{
	while(ParseLexem(str, lex_semicolon));

	if(ParseClassDefinition(str))
		return true;
	if(ParseVariableDefine(str))
	{
		if(!ParseLexem(str, lex_semicolon))
			ThrowError("ERROR: ';' not found after variable definition", (*str)->pos);
		return true;
	}
	if(ParseBlock(str))
		return true;
	if(ParseBreakExpr(str))
		return true;
	if(ParseContinueExpr(str))
		return true;
	if(ParseReturnExpr(str))
		return true;
	if(ParseIfExpr(str))
		return true;
	if(ParseForExpr(str))
		return true;
	if(ParseWhileExpr(str))
		return true;
	if(ParseDoWhileExpr(str))
		return true;
	if(ParseSwitchExpr(str))
		return true;
	if(ParseVaribleSet(str))
	{
		if(!ParseLexem(str, lex_semicolon))
			ThrowError("ERROR: ';' not found after expression", (*str)->pos);
		CALLBACK(addPopNode(NULL, NULL));
		return true;
	}
	return false;
}

bool  ParseCode(Lexeme** str)
{
	if(!ParseFunctionDefinition(str))
	{
		if(!ParseExpression(str))
			return false;
	}
	if(ParseCode(str))
		CALLBACK(addTwoExprNode(NULL, NULL));
	else
		CALLBACK(addOneExprNode(NULL, NULL));
	return true;
}
