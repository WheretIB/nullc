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

unsigned int ParseTypename(Lexeme** str)
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
			return s+1;
		}
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
			ThrowError("ERROR: '0x' must be followed by number", start+2);
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

	if(!ParseTernaryExpr(str))
		CALLBACK(AddUnfixedArraySize());
	if(!ParseLexem(str, lex_cbracket))
		ThrowError("ERROR: Matching ']' not found", (*str)->pos);
	ParseArrayDefinition(str);
	CALLBACK(ConvertTypeToArray((*str)->pos));
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
			CALLBACK(SetTypeOfLastNode());
			if(!ParseLexem(str, lex_cparen))
				ThrowError("ERROR: ')' not found after expression in typeof", (*str)->pos);
		}else{
			ThrowError("ERROR: expression not found after typeof(", (*str)->pos);
		}
	}else if((*str)->type == lex_auto){
		CALLBACK(SelectAutoType());
		(*str)++;
	}else if((*str)->type == lex_string){
		unsigned int index;
		if((index = ParseTypename(str)) == 0)
			return false;
		CALLBACK(SelectTypeByIndex(index-1));
	}else{
		return false;
	}

	for(;;)
	{
		if(ParseLexem(str, lex_ref))
		{
			CALLBACK(ConvertTypeToReference((*str)->pos));
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
		CALLBACK(TypeFinish());
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
	CALLBACK(FunctionParameter((*str)->pos, InplaceStr((*str)->pos, (*str)->length)));
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
		CALLBACK(FunctionParameter((*str)->pos, InplaceStr((*str)->pos, (*str)->length)));
		(*str)++;
	}
	return true;
}

bool ParseFunctionDefinition(Lexeme** str)
{
	Lexeme *start = *str;
	if(!ParseSelectType(str))
		return false;

	if(((*str)->type != lex_string || (*str)[1].type != lex_oparen) && (start->type != lex_auto || (*str)->type != lex_oparen))
	{
		*str = start;
		return false;
	}
	char	*functionName = NULL;
	if((*str)->type == lex_string)
	{
		if((*str)->length >= NULLC_MAX_VARIABLE_NAME_LENGTH)
			ThrowError("ERROR: function name length is limited to 2048 symbols", (*str)->pos);
		functionName = (char*)stringPool.Allocate((*str)->length+1);
		memcpy(functionName, (*str)->pos, (*str)->length);
		functionName[(*str)->length] = 0;
		(*str)++;
	}else{
		static int unnamedFuncCount = 0;
		functionName = (char*)stringPool.Allocate(16);
		sprintf(functionName, "$func%d", unnamedFuncCount);
		unnamedFuncCount++;
	}
	(*str)++;

	CALLBACK(FunctionAdd((*str)->pos, functionName));

	ParseFunctionVariables(str);
	CALLBACK(FunctionStart((*str)->pos));

	if(!ParseLexem(str, lex_cparen))
		ThrowError("ERROR: ')' not found after function variable list", (*str)->pos);
	if(!ParseLexem(str, lex_ofigure))
		ThrowError("ERROR: '{' not found after function header", (*str)->pos);

	if(!ParseCode(str))
		CALLBACK(AddVoidNode());
	CALLBACK(FunctionEnd(start->pos, functionName));
	
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

	Lexeme *varName = *str;
	(*str)++;

	if(ParseLexem(str, lex_obracket))
	{
		if(!ParseTernaryExpr(str))
			CALLBACK(AddUnfixedArraySize());
		if(!ParseLexem(str, lex_cbracket))
			ThrowError("ERROR: Matching ']' not found", (*str)->pos);
		CALLBACK(ConvertTypeToArray((*str)->pos));
	}
	CALLBACK(PushType());
	CALLBACK(AddVariable((*str)->pos, InplaceStr(varName->pos, varName->length)));

	if(ParseLexem(str, lex_set))
	{
		if(!ParseVaribleSet(str))
			ThrowError("ERROR: expression not found after '='", (*str)->pos);
		CALLBACK(AddDefineVariableNode((*str)->pos, InplaceStr(varName->pos, varName->length)));
		CALLBACK(AddPopNode((*str)->pos));
		CALLBACK(PopType());
	}else{
		CALLBACK(AddVariableReserveNode((*str)->pos));
	}
	CALLBACK(PopType());
	return true;
}

bool ParseVariableDefineSub(Lexeme** str)
{
	if(!ParseAddVariable(str))
		return false;

	while(ParseLexem(str, lex_comma))
	{
		if(!ParseAddVariable(str))
			ThrowError("ERROR: next variable definition excepted after ','", (*str)->pos);
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
	if(!ParseLexem(str, lex_if))
		return false;

	if(!ParseLexem(str, lex_oparen))
		ThrowError("ERROR: '(' not found after 'if'", (*str)->pos);
	const char *condPos = (*str)->pos;
	if(!ParseVaribleSet(str))
		ThrowError("ERROR: condition not found in 'if' statement", (*str)->pos);
	if(!ParseLexem(str, lex_cparen))
		ThrowError("ERROR: closing ')' not found after 'if' condition", (*str)->pos);
	if(!ParseExpression(str))
		ThrowError("ERROR: expression not found after 'if'", (*str)->pos);

	if(ParseLexem(str, lex_else))
	{
		if(!ParseExpression(str))
			ThrowError("ERROR: expression not found after 'else'", (*str)->pos);
		CALLBACK(AddIfElseNode(condPos));
	}else{
		CALLBACK(AddIfNode(condPos));
	}
	return true;
}

bool  ParseForExpr(Lexeme** str)
{
	if(!ParseLexem(str, lex_for))
		return false;
	
	CALLBACK(IncreaseCycleDepth());
	
	if(!ParseLexem(str, lex_oparen))
		ThrowError("ERROR: '(' not found after 'for'", (*str)->pos);
	
	if(ParseLexem(str, lex_ofigure))
	{
		if(!ParseCode(str))
			CALLBACK(AddVoidNode());
		if(!ParseLexem(str, lex_cfigure))
			ThrowError("ERROR: '}' not found after '{'", (*str)->pos);
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
		ThrowError("ERROR: ';' not found after initializer in 'for'", (*str)->pos);

	const char *condPos = (*str)->pos;
	if(!ParseVaribleSet(str))
		ThrowError("ERROR: condition not found in 'for' statement", (*str)->pos);

	if(!ParseLexem(str, lex_semicolon))
		ThrowError("ERROR: ';' not found after condition in 'for'", (*str)->pos);

	if(ParseLexem(str, lex_ofigure))
	{
		if(!ParseCode(str))
			CALLBACK(AddVoidNode());
		if(!ParseLexem(str, lex_cfigure))
			ThrowError("ERROR: '}' not found after '{'", (*str)->pos);
	}else{
		if(!ParseVaribleSet(str))
			CALLBACK(AddVoidNode());
		else
			CALLBACK(AddPopNode((*str)->pos));
	}

	if(!ParseLexem(str, lex_cparen))
		ThrowError("ERROR: ')' not found after 'for' statement", (*str)->pos);

	if(!ParseExpression(str))
		ThrowError("ERROR: body not found after 'for' header", (*str)->pos);
	CALLBACK(AddForNode(condPos));
	return true;

}

bool  ParseWhileExpr(Lexeme** str)
{
	if(!ParseLexem(str, lex_while))
		return false;
	
	CALLBACK(IncreaseCycleDepth());
	if(!ParseLexem(str, lex_oparen))
		ThrowError("ERROR: '(' not found after 'while'", (*str)->pos);

	const char *condPos = (*str)->pos;
	if(!ParseVaribleSet(str))
		ThrowError("ERROR: expression expected after 'while('", (*str)->pos);
	if(!ParseLexem(str, lex_cparen))
		ThrowError("ERROR: closing ')' not found after expression in 'while' statement", (*str)->pos);

	if(!ParseExpression(str))
		ThrowError("ERROR: expression expected after 'while(...)'", (*str)->pos);
	CALLBACK(AddWhileNode(condPos));
	return true;
}

bool  ParseDoWhileExpr(Lexeme** str)
{
	if(!ParseLexem(str, lex_do))
		return false;

	CALLBACK(IncreaseCycleDepth());

	if(!ParseExpression(str))
		ThrowError("ERROR: expression expected after 'do'", (*str)->pos);

	if(!ParseLexem(str, lex_while))
		ThrowError("ERROR: 'while' expected after 'do' statement", (*str)->pos);
	if(!ParseLexem(str, lex_oparen))
		ThrowError("ERROR: '(' not found after 'while'", (*str)->pos);

	const char *condPos = (*str)->pos;
	if(!ParseVaribleSet(str))
		ThrowError("ERROR: expression expected after 'while('", (*str)->pos);
	if(!ParseLexem(str, lex_cparen))
		ThrowError("ERROR: closing ')' not found after expression in 'while' statement", (*str)->pos);

	CALLBACK(AddDoWhileNode(condPos));

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

	const char *condPos = (*str)->pos;
	if(!ParseVaribleSet(str))
		ThrowError("ERROR: expression not found after 'switch('", (*str)->pos);
	CALLBACK(BeginSwitch(condPos));

	if(!ParseLexem(str, lex_cparen))
		ThrowError("ERROR: closing ')' not found after expression in 'switch' statement", (*str)->pos);

	if(!ParseLexem(str, lex_ofigure))
		ThrowError("ERROR: '{' not found after 'switch(...)'", (*str)->pos);

	while(ParseLexem(str, lex_case))
	{
		const char *condPos = (*str)->pos;
		if(!ParseVaribleSet(str))
			ThrowError("ERROR: expression expected after 'case'", (*str)->pos);
		if(!ParseLexem(str, lex_colon))
			ThrowError("ERROR: ':' not found after 'case' expression", (*str)->pos);

		if(!ParseExpression(str))
			ThrowError("ERROR: expression expected after 'case:'", (*str)->pos);
		while(ParseExpression(str))
			CALLBACK(AddTwoExpressionNode());
		CALLBACK(AddCaseNode(condPos));
	}

	if(!ParseLexem(str, lex_cfigure))
		ThrowError("ERROR: '}' not found after 'switch' statement", (*str)->pos);
	CALLBACK(EndSwitch());
	return true;
}

bool  ParseReturnExpr(Lexeme** str)
{
	const char* start = (*str)->pos;
	if(!ParseLexem(str, lex_return))
		return false;

	if(!ParseVaribleSet(str))
		CALLBACK(AddVoidNode());

	if(!ParseLexem(str, lex_semicolon))
		ThrowError("ERROR: return must be followed by ';'", (*str)->pos);
	CALLBACK(AddReturnNode(start));
	return true;
}

bool  ParseBreakExpr(Lexeme** str)
{
	const char *pos = (*str)->pos;
	if(!ParseLexem(str, lex_break))
		return false;

	if(!ParseTerminal(str))
		AddVoidNode();

	if(!ParseLexem(str, lex_semicolon))
		ThrowError("ERROR: break must be followed by ';'", (*str)->pos);
	CALLBACK(AddBreakNode(pos));
	return true;
}

bool  ParseContinueExpr(Lexeme** str)
{
	const char *pos = (*str)->pos;
	if(!ParseLexem(str, lex_continue))
		return false;

	if(!ParseTerminal(str))
		AddVoidNode();

	if(!ParseLexem(str, lex_semicolon))
		ThrowError("ERROR: continue must be followed by ';'", (*str)->pos);
	CALLBACK(AddContinueNode(pos));
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
		CALLBACK(AddDereferenceNode((*str)->pos));
		return true;
	}
	
	if((*str)->type != lex_string || (*str)[1].type == lex_oparen)
		return false;

	if((*str)->length >= NULLC_MAX_VARIABLE_NAME_LENGTH)
		ThrowError("ERROR: variable name length is limited to 2048 symbols", (*str)->pos);
	CALLBACK(AddGetAddressNode((*str)->pos, InplaceStr((*str)->pos, (*str)->length)));
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
		CALLBACK(AddMemberAccessNode((*str)->pos, InplaceStr((*str)->pos, (*str)->length)));
		(*str)++;
	}else if(ParseLexem(str, lex_obracket)){
		if(!ParseVaribleSet(str))
			ThrowError("ERROR: expression not found after '['", (*str)->pos);
		if(!ParseLexem(str, lex_cbracket))
			ThrowError("ERROR: ']' not found after expression", (*str)->pos);
		CALLBACK(AddArrayIndexNode((*str)->pos));
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
		CALLBACK(PopType());
		return true;
	}
	if(ParseLexem(str, lex_lognot))
	{
		if(!ParseTerminal(str))
			ThrowError("ERROR: expression not found after '!'", (*str)->pos);
		CALLBACK(AddLogNotNode((*str)->pos));
		return true;
	}
	if(ParseLexem(str, lex_bitnot))
	{
		if(!ParseTerminal(str))
			ThrowError("ERROR: expression not found after '~'", (*str)->pos);
		CALLBACK(AddBitNotNode((*str)->pos));
		return true;
	}
	if(ParseLexem(str, lex_dec))
	{
		if(!ParseVariable(str))
			ThrowError("ERROR: variable not found after '--'", (*str)->pos);
		CALLBACK(AddPreOrPostOpNode(false, true));
		CALLBACK(PopType());
		return true;
	}
	if(ParseLexem(str, lex_inc))
	{
		if(!ParseVariable(str))
			ThrowError("ERROR: variable not found after '++'", (*str)->pos);
		CALLBACK(AddPreOrPostOpNode(true, true));
		CALLBACK(PopType());
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
			CALLBACK(AddNegateNode((*str)->pos));
		return true;
	}
	if((*str)->type == lex_quotedstring)
	{
		CALLBACK(AddStringNode((*str)->pos, (*str)->pos+(*str)->length));
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
		CALLBACK(AddNumberNodeChar(start));
		return true;
	}
	if(ParseLexem(str, lex_sizeof))
	{
		if(!ParseLexem(str, lex_oparen))
			ThrowError("ERROR: sizeof must be followed by '('", (*str)->pos);
		CALLBACK(PushType());
		if(ParseSelectType(str))
		{
			CALLBACK(PushType());
			CALLBACK(GetTypeSize((*str)->pos, false));
			CALLBACK(PopType());
		}else{
			if(ParseVaribleSet(str))
				CALLBACK(GetTypeSize((*str)->pos, true));
			else
				ThrowError("ERROR: expression or type not found after sizeof(", (*str)->pos);
		}
		if(!ParseLexem(str, lex_cparen))
			ThrowError("ERROR: ')' not found after expression in sizeof", (*str)->pos);
		CALLBACK(PopType());
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
		CALLBACK(AddArrayConstructor((*str)->pos, arrElementCount));
		return true;
	}
	if(ParseGroup(str))
		return true;
	if(((*str)->type == lex_auto) ||
		((*str)->type == lex_typeof && (*str)[1].type == lex_oparen) ||
		((*str)->type == lex_string) && ((*str)[1].type == lex_string || (*str)[1].type == lex_ref || (*str)[1].type == lex_obracket))
	{
		if(ParseFunctionDefinition(str))
			return true;
	}
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
			CALLBACK(AddGetVariableNode((*str)->pos));
		}
		CALLBACK(PopType());
		return true;
	}
	return false;
}

// indexed by [lexeme - lex_add]
char opPrecedence[] = { 2, 2, 1, 1, 1, 0, 4, 4, 3, 4, 4, 3, 5, 5, 6, 8, 7, 9, 11, 10 /* + - * / % ** < <= << > >= >> == != & | ^ and or xor */ };
CmdID opHandler[] = { cmdAdd, cmdSub, cmdMul, cmdDiv, cmdMod, cmdPow, cmdLess, cmdLEqual, cmdShl, cmdGreater, cmdGEqual, cmdShr, cmdEqual, cmdNEqual, cmdBitAnd, cmdBitOr, cmdBitXor, cmdLogAnd, cmdLogOr, cmdLogXor };
// operator stack
FastVector<LexemeType>	opStack(64);

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
			CALLBACK(AddBinaryCommandNode(opHandler[opStack.back() - lex_add]));
			opStack.pop_back();
			opCount--;
		}
		opStack.push_back(lType);
		opCount++;	// opStack is global, but we are tracing its local size
		(*str)++;
		if(!ParseTerminal(str))
			ThrowError("ERROR: terminal expression not found after binary operation", (*str)->pos);
	}
	while(opCount > 0)
	{
		CALLBACK(AddBinaryCommandNode(opHandler[opStack.back() - lex_add]));
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
			ThrowError("ERROR: expression not found after '?'", (*str)->pos);
		if(!ParseLexem(str, lex_colon))
			ThrowError("ERROR: ':' not found after expression in ternary operator", (*str)->pos);
		if(!ParseVaribleSet(str))
			ThrowError("ERROR: expression not found after ':'", (*str)->pos);
		CALLBACK(AddIfElseTermNode(condPos));
	}
	return true;
}

bool ParseVaribleSet(Lexeme** str)
{
	if(((*str)->type == lex_auto) ||
		((*str)->type == lex_typeof && (*str)[1].type == lex_oparen) ||
		((*str)->type == lex_string) && ((*str)[1].type == lex_string || (*str)[1].type == lex_ref || (*str)[1].type == lex_obracket))
	{
		if(ParseFunctionDefinition(str))
			return true;
	}
	
	Lexeme *start = *str;
	if(ParseVariable(str))
	{
		char op = (*str)->pos[0];
		if(ParseLexem(str, lex_set))
		{
			if(ParseVaribleSet(str))
			{
				CALLBACK(AddSetVariableNode((*str)->pos));
				CALLBACK(PopType());
				return true;
			}else{
				CALLBACK(FailedSetVariable());
				CALLBACK(PopType());
				*str = start;
			}
		}else if(ParseLexem(str, lex_addset) || ParseLexem(str, lex_subset) || ParseLexem(str, lex_mulset) || ParseLexem(str, lex_divset)){
			if(ParseVaribleSet(str))
			{
				CALLBACK(AddModifyVariableNode((*str)->pos, (CmdID)(op == '+' ? cmdAdd : (op == '-' ? cmdSub : (op == '*' ? cmdMul : cmdDiv)))));
				CALLBACK(PopType());
				return true;
			}else{
				ThrowError("ERROR: expression not found after assignment operator", (*str)->pos);
			}
		}else if(ParseLexem(str, lex_powset)){
			if(ParseVaribleSet(str))
			{
				CALLBACK(AddModifyVariableNode((*str)->pos, cmdPow));
				CALLBACK(PopType());
				return true;
			}else{
				ThrowError("ERROR: expression not found after '**='", (*str)->pos);
			}
		}else{
			CALLBACK(FailedSetVariable());
			CALLBACK(PopType());
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
	CALLBACK(BeginBlock());
	if(!ParseCode(str))
		ThrowError("ERROR: {} block cannot be empty", (*str)->pos);
	if(!ParseLexem(str, lex_cfigure))
		ThrowError("ERROR: closing '}' not found", (*str)->pos);
	CALLBACK(EndBlock());
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
		const char *pos = (*str)->pos;
		if(!ParseLexem(str, lex_semicolon))
			ThrowError("ERROR: ';' not found after expression", (*str)->pos);
		CALLBACK(AddPopNode(pos));
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
		CALLBACK(AddTwoExpressionNode());
	else
		CALLBACK(AddOneExpressionNode());
	return true;
}
