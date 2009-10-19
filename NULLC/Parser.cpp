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

	unsigned int hash = GetStringHash((*str)->pos, (*str)->pos + (*str)->length);

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

	if(!ParseTernaryExpr(str))
		CALLBACK(AddUnfixedArraySize());
	if(!ParseLexem(str, lex_cbracket))
		ThrowError((*str)->pos, "ERROR: Matching ']' not found");
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
			ThrowError((*str)->pos, "ERROR: typeof must be followed by '('");
		if(ParseVaribleSet(str))
		{
			CALLBACK(SetTypeOfLastNode());
			if(!ParseLexem(str, lex_cparen))
				ThrowError((*str)->pos, "ERROR: ')' not found after expression in typeof");
		}else{
			ThrowError((*str)->pos, "ERROR: expression not found after typeof(");
		}
	}else if((*str)->type == lex_auto){
		CALLBACK(SelectTypeByPointer(NULL));
		(*str)++;
	}else if((*str)->type == lex_string){
		unsigned int index;
		if((index = ParseTypename(str)) == 0)
			return false;
		CALLBACK(SelectTypeByIndex(index - 1));
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

		for(;;)
		{
			if(!ParseFunctionDefinition(str))
			{
				if(!ParseSelectType(str))
					break;

				if((*str)->type != lex_string)
					ThrowError((*str)->pos, "ERROR: class member name expected after type");
				if((*str)->length >= NULLC_MAX_VARIABLE_NAME_LENGTH)
					ThrowError((*str)->pos, "ERROR: member name length is limited to 2048 symbols");
				char	*memberName = (char*)stringPool.Allocate((*str)->length+1);
				memcpy(memberName, (*str)->pos, (*str)->length);
				memberName[(*str)->length] = 0;
				CALLBACK(TypeAddMember((*str)->pos, memberName));
				(*str)++;

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

	unsigned int callArgCount = 0;
	if(ParseVaribleSet(str))
	{
		callArgCount++;
		while(ParseLexem(str, lex_comma))
		{
			if(!ParseVaribleSet(str))
				ThrowError((*str)->pos, "ERROR: expression not found after ',' in function parameter list");
			callArgCount++;
		}
	}
	if(!ParseLexem(str, lex_cparen))
		ThrowError((*str)->pos, "ERROR: ')' not found after function parameter list");

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
		ThrowError((*str)->pos, "ERROR: variable name not found after type in function variable list");

	if((*str)->length >= NULLC_MAX_VARIABLE_NAME_LENGTH)
		ThrowError((*str)->pos, "ERROR: parameter name length is limited to 2048 symbols");
	CALLBACK(FunctionParameter((*str)->pos, InplaceStr((*str)->pos, (*str)->length)));
	(*str)++;

	while(ParseLexem(str, lex_comma))
	{
		ParseIsConst(str);
		ParseSelectType(str);

		if((*str)->type != lex_string)
			ThrowError((*str)->pos, "ERROR: variable name not found after type in function variable list");
		if((*str)->length >= NULLC_MAX_VARIABLE_NAME_LENGTH)
			ThrowError((*str)->pos, "ERROR: parameter name length is limited to 2048 symbols");
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

	Lexeme *name = *str;
	if((name->type != lex_string || name[1].type != lex_oparen) &&
		(name->type != lex_operator || (name[1].type < lex_add || name[1].type > lex_logxor && name[1].type != lex_obracket)) &&
		(start->type != lex_auto || name->type != lex_oparen))
	{
		*str = start;
		return false;
	}
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
	}
	char	*functionName = NULL;
	if((*str)->type == lex_string || ((*str)->type >= lex_add && (*str)->type <= lex_logxor))
	{
		if((*str)->length >= NULLC_MAX_VARIABLE_NAME_LENGTH)
			ThrowError((*str)->pos, "ERROR: function name length is limited to 2048 symbols");
		functionName = (char*)stringPool.Allocate((*str)->length+1);
		memcpy(functionName, (*str)->pos, (*str)->length);
		functionName[(*str)->length] = 0;
		(*str)++;
	}else if((*str)->type == lex_cbracket){
		functionName = (char*)stringPool.Allocate(16);
		sprintf(functionName, "[]");
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

	if(!ParseLexem(str, lex_cparen))
		ThrowError((*str)->pos, "ERROR: ')' not found after function variable list");

	if(ParseLexem(str, lex_semicolon))
	{
		CALLBACK(AddVoidNode());
		if(name[1].type >= lex_add && name[1].type <= lex_logxor || name[1].type == lex_obracket)
			CALLBACK(FunctionToOperator(start->pos));
		CALLBACK(FunctionPrototype(start->pos));
		return true;
	}

	CALLBACK(FunctionStart((*str-1)->pos));
	if(!ParseLexem(str, lex_ofigure))
		ThrowError((*str)->pos, "ERROR: '{' not found after function header");

	if(!ParseCode(str))
		CALLBACK(AddVoidNode());
	if(!ParseLexem(str, lex_cfigure))
		ThrowError((*str)->pos, "ERROR: '}' not found after function body");

	if(name[1].type >= lex_add && name[1].type <= lex_logxor || name[1].type == lex_obracket)
		CALLBACK(FunctionToOperator(start->pos));

	CALLBACK(FunctionEnd(start->pos, functionName));

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

	CALLBACK(AddVariable((*str)->pos, InplaceStr(varName->pos, varName->length)));

	if(ParseLexem(str, lex_set))
	{
		if(!ParseVaribleSet(str))
			ThrowError((*str)->pos, "ERROR: expression not found after '='");
		CALLBACK(AddDefineVariableNode((*str)->pos, InplaceStr(varName->pos, varName->length)));
		CALLBACK(AddPopNode((*str)->pos));
	}else{
		CALLBACK(AddVariableReserveNode((*str)->pos));
	}
	return true;
}

bool ParseVariableDefineSub(Lexeme** str)
{
	void* currType = GetSelectedType();
	if(!ParseAddVariable(str))
		ThrowError((*str)->pos, "ERROR: variable name not found after type name");

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
		ThrowError((*str)->pos, "ERROR: '(' not found after 'if'");
	const char *condPos = (*str)->pos;
	if(!ParseVaribleSet(str))
		ThrowError((*str)->pos, "ERROR: condition not found in 'if' statement");
	if(!ParseLexem(str, lex_cparen))
		ThrowError((*str)->pos, "ERROR: closing ')' not found after 'if' condition");
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

bool  ParseForExpr(Lexeme** str)
{
	if(!ParseLexem(str, lex_for))
		return false;
	
	CALLBACK(IncreaseCycleDepth());
	
	if(!ParseLexem(str, lex_oparen))
		ThrowError((*str)->pos, "ERROR: '(' not found after 'for'");
	
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

	const char *condPos = (*str)->pos;
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

	if(!ParseLexem(str, lex_cparen))
		ThrowError((*str)->pos, "ERROR: ')' not found after 'for' statement");

	if(!ParseExpression(str))
		ThrowError((*str)->pos, "ERROR: body not found after 'for' header");
	CALLBACK(AddForNode(condPos));
	return true;

}

bool  ParseWhileExpr(Lexeme** str)
{
	if(!ParseLexem(str, lex_while))
		return false;
	
	CALLBACK(IncreaseCycleDepth());
	if(!ParseLexem(str, lex_oparen))
		ThrowError((*str)->pos, "ERROR: '(' not found after 'while'");

	const char *condPos = (*str)->pos;
	if(!ParseVaribleSet(str))
		ThrowError((*str)->pos, "ERROR: expression expected after 'while('");
	if(!ParseLexem(str, lex_cparen))
		ThrowError((*str)->pos, "ERROR: closing ')' not found after expression in 'while' statement");

	if(!ParseExpression(str))
		ThrowError((*str)->pos, "ERROR: expression expected after 'while(...)'");
	CALLBACK(AddWhileNode(condPos));
	return true;
}

bool  ParseDoWhileExpr(Lexeme** str)
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

bool  ParseSwitchExpr(Lexeme** str)
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

bool  ParseReturnExpr(Lexeme** str)
{
	const char* start = (*str)->pos;
	if(!ParseLexem(str, lex_return))
		return false;

	if(!ParseVaribleSet(str))
		CALLBACK(AddVoidNode());

	if(!ParseLexem(str, lex_semicolon))
		ThrowError((*str)->pos, "ERROR: return must be followed by ';'");
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
		ThrowError((*str)->pos, "ERROR: break must be followed by ';'");
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
		ThrowError((*str)->pos, "ERROR: continue must be followed by ';'");
	CALLBACK(AddContinueNode(pos));
	return true;
}

bool  ParseGroup(Lexeme** str)
{
	if(!ParseLexem(str, lex_oparen))
		return false;

	if(!ParseVaribleSet(str))
		ThrowError((*str)->pos, "ERROR: expression not found after '('");
	if(!ParseLexem(str, lex_cparen))
		ThrowError((*str)->pos, "ERROR: closing ')' not found after '('");
	return true;
}

bool  ParseVariable(Lexeme** str)
{
	if(ParseLexem(str, lex_mul))
	{
		if(!ParseVariable(str))
			ThrowError((*str)->pos, "ERROR: variable name not found after '*'");
		CALLBACK(AddDereferenceNode((*str)->pos));
		return true;
	}
	
	if((*str)->type != lex_string || (*str)[1].type == lex_oparen)
		return false;

	if((*str)->length >= NULLC_MAX_VARIABLE_NAME_LENGTH)
		ThrowError((*str)->pos, "ERROR: variable name length is limited to 2048 symbols");
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
			ThrowError((*str)->pos, "ERROR: member variable expected after '.'");
		if((*str)[1].type == lex_oparen)
		{
			*str = start;
			return false;
		}
		if((*str)->length >= NULLC_MAX_VARIABLE_NAME_LENGTH)
			ThrowError((*str)->pos, "ERROR: variable name length is limited to 2048 symbols");
		CALLBACK(AddMemberAccessNode((*str)->pos, InplaceStr((*str)->pos, (*str)->length)));
		(*str)++;
	}else if(ParseLexem(str, lex_obracket)){
		if(!ParseVaribleSet(str))
			ThrowError((*str)->pos, "ERROR: expression not found after '['");
		if(!ParseLexem(str, lex_cbracket))
			ThrowError((*str)->pos, "ERROR: ']' not found after expression");
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
			ThrowError((*str)->pos, "ERROR: variable not found after '&'");
		return true;
	}
	if(ParseLexem(str, lex_lognot))
	{
		if(!ParseTerminal(str))
			ThrowError((*str)->pos, "ERROR: expression not found after '!'");
		CALLBACK(AddLogNotNode((*str)->pos));
		return true;
	}
	if(ParseLexem(str, lex_bitnot))
	{
		if(!ParseTerminal(str))
			ThrowError((*str)->pos, "ERROR: expression not found after '~'");
		CALLBACK(AddBitNotNode((*str)->pos));
		return true;
	}
	if(ParseLexem(str, lex_dec))
	{
		if(!ParseVariable(str))
			ThrowError((*str)->pos, "ERROR: variable not found after '--'");
		CALLBACK(AddPreOrPostOpNode((*str)->pos, false, true));
		return true;
	}
	if(ParseLexem(str, lex_inc))
	{
		if(!ParseVariable(str))
			ThrowError((*str)->pos, "ERROR: variable not found after '++'");
		CALLBACK(AddPreOrPostOpNode((*str)->pos, true, true));
		return true;
	}
	if(ParseLexem(str, lex_add))
	{
		while(ParseLexem(str, lex_add));
		if(!ParseTerminal(str))
			ThrowError((*str)->pos, "ERROR: expression not found after '+'");
		return true;
	}
	if(ParseLexem(str, lex_sub))
	{
		int negCount = 1;
		while(ParseLexem(str, lex_sub))
			negCount++;
		if(!ParseTerminal(str))
			ThrowError((*str)->pos, "ERROR: expression not found after '-'");
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
	if((*str)->type == lex_semiquotedchar)
	{
		if(((*str)->length > 3 && (*str)->pos[1] != '\\') || (*str)->length > 4)
			ThrowError((*str)->pos, "ERROR: only one character can be inside single quotes");
		CALLBACK(AddNumberNodeChar((*str)->pos));
		(*str)++;
		return true;
	}
	if(ParseLexem(str, lex_sizeof))
	{
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
	}
	if(ParseLexem(str, lex_new))
	{
		const char *pos = (*str)->pos;
		int index;
		if((index = ParseTypename(str)) == 0)
			ThrowError((*str)->pos, "ERROR: Type name expected after 'new'");
		CALLBACK(SelectTypeByIndex(index - 1));

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
	if(ParseLexem(str, lex_ofigure))
	{
		unsigned int arrElementCount = 0;
		if(!ParseTernaryExpr(str))
			ThrowError((*str)->pos, "ERROR: value not found after '{'");
		while(ParseLexem(str, lex_comma))
		{
			if(!ParseTernaryExpr(str))
				ThrowError((*str)->pos, "ERROR: value not found after ','");
			arrElementCount++;
		}
		if(!ParseLexem(str, lex_cfigure))
			ThrowError((*str)->pos, "ERROR: '}' not found after inline array");
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
			CALLBACK(AddPreOrPostOpNode((*str)->pos, false, false));
		}else if(ParseLexem(str, lex_inc))
		{
			CALLBACK(AddPreOrPostOpNode((*str)->pos, true, false));
		}else if(ParseLexem(str, lex_point)){
			if(!ParseFunctionCall(str, true))
				ThrowError((*str)->pos, "ERROR: function call is excepted after '.'");
		}else{
			CALLBACK(AddGetVariableNode((*str)->pos));
		}
		return true;
	}
	return false;
}

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
				return true;
			}else{
				CALLBACK(FailedSetVariable());
				*str = start;
			}
		}else if(ParseLexem(str, lex_addset) || ParseLexem(str, lex_subset) || ParseLexem(str, lex_mulset) || ParseLexem(str, lex_divset)){
			if(ParseVaribleSet(str))
			{
				CALLBACK(AddModifyVariableNode((*str)->pos, (CmdID)(op == '+' ? cmdAdd : (op == '-' ? cmdSub : (op == '*' ? cmdMul : cmdDiv)))));
				return true;
			}else{
				ThrowError((*str)->pos, "ERROR: expression not found after assignment operator");
			}
		}else if(ParseLexem(str, lex_powset)){
			if(ParseVaribleSet(str))
			{
				CALLBACK(AddModifyVariableNode((*str)->pos, cmdPow));
				return true;
			}else{
				ThrowError((*str)->pos, "ERROR: expression not found after '**='");
			}
		}else{
			CALLBACK(FailedSetVariable());
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
		CALLBACK(AddVoidNode());
	if(!ParseLexem(str, lex_cfigure))
		ThrowError((*str)->pos, "ERROR: closing '}' not found");
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
			ThrowError((*str)->pos, "ERROR: ';' not found after variable definition");
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
			ThrowError((*str)->pos, "ERROR: ';' not found after expression");
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
