#include "Parser.h"
#include "CodeInfo.h"
using namespace CodeInfo;

#include "Callbacks.h"

enum chartype
{
	ct_symbol = 64,			// Any symbol > 127, a-z, A-Z, 0-9, _
	ct_start_symbol = 128	// Any symbol > 127, a-z, A-Z, _, :
};

const unsigned char chartype_table[256] =
{
	0,   0,   0,   0,   0,   0,   0,   0,      0,   0,   0,   0,   0,   0,   0,   0,   // 0-15
	0,   0,   0,   0,   0,   0,   0,   0,      0,   0,   0,   0,   0,   0,   0,   0,   // 16-31
	0,   0,   6,   0,   0,   0,   0,   0,      0,   0,   0,   0,   0,   0,   0,   0,   // 32-47
	64,  64,  64,  64,  64,  64,  64,  64,     64,  64,  0,   0,   0,   0,   0,   0,   // 48-63
	0,   192, 192, 192, 192, 192, 192, 192,    192, 192, 192, 192, 192, 192, 192, 192, // 64-79
	192, 192, 192, 192, 192, 192, 192, 192,    192, 192, 192, 0,   0,   0,   0,   192, // 80-95
	0,   192, 192, 192, 192, 192, 192, 192,    192, 192, 192, 192, 192, 192, 192, 192, // 96-111
	192, 192, 192, 192, 192, 192, 192, 192,    192, 192, 192, 0, 0, 0, 0, 0,           // 112-127

	192, 192, 192, 192, 192, 192, 192, 192,    192, 192, 192, 192, 192, 192, 192, 192, // 128+
	192, 192, 192, 192, 192, 192, 192, 192,    192, 192, 192, 192, 192, 192, 192, 192,
	192, 192, 192, 192, 192, 192, 192, 192,    192, 192, 192, 192, 192, 192, 192, 192,
	192, 192, 192, 192, 192, 192, 192, 192,    192, 192, 192, 192, 192, 192, 192, 192,
	192, 192, 192, 192, 192, 192, 192, 192,    192, 192, 192, 192, 192, 192, 192, 192,
	192, 192, 192, 192, 192, 192, 192, 192,    192, 192, 192, 192, 192, 192, 192, 192,
	192, 192, 192, 192, 192, 192, 192, 192,    192, 192, 192, 192, 192, 192, 192, 192,
	192, 192, 192, 192, 192, 192, 192, 192,    192, 192, 192, 192, 192, 192, 192, 192
};

static inline bool isDigit(char data)
{
	return (unsigned char)(data - '0') < 10;
}

void	ParseSpace(char** str)
{
	while(**str == ' ')
		(*str)++;
}

#define SKIP_SPACE(str) ParseSpace(str);
#define CALLBACK(x) x
//#define CALLBACK(x) 1

bool ParseTypename(char** str)
{
	SKIP_SPACE(str);
	char *curr = *str;
	if(!(chartype_table[*curr] & ct_start_symbol))
		return false;
	while(chartype_table[*curr] & ct_symbol)
		curr++;
	unsigned int hash = 5381;
	while(*str < curr)
		hash = ((hash << 5) + hash) + *(*str)++;
	for(unsigned int s = 0, e = CodeInfo::typeInfo.size(); s != e; s++)
		if(CodeInfo::typeInfo[s]->nameHash == hash)
			return true;
	return false;
}

bool  ParseInt(char** str)
{
	SKIP_SPACE(str);
	char *curr = *str;
	while(isDigit(*str[0]))
		(*str)++;
	if(curr == *str)
		return false;	//no characters were parsed
	return true;
}

bool  ParseNumber(char** str)
{
	//SKIP_SPACE(str);
	char* start = *str;
	if(!isDigit(*start))
		return false;
	if(start[0] == '0' && start[1] == 'x')	// hexadecimal
	{
		*str += 2;
		while(isDigit(**str) || ((unsigned char)(**str - 'a') < 6) || ((unsigned char)(**str - 'A') < 6))
			(*str)++;
		if(*str == start+2)
			ThrowError("ERROR: '0x' must be followed by number", *str);
		CALLBACK(addHexInt(start, *str));
		return true;
	}
	while(isDigit(**str))
		(*str)++;
	if(start == *str && **str != '.')
		return false;

	if(**str == 'b')
	{
		(*str)++;
		CALLBACK(addBinInt(start, *str));
		return true;
	}else if(**str == 'l'){
		(*str)++;
		CALLBACK(addNumberNodeLong(start, *str));
		return true;
	}else if(**str != '.' && start[0] == '0' && isDigit(start[1])){
		CALLBACK(addOctInt(start, *str));
		return true;
	}else if(**str != '.' && **str != 'e'){
		CALLBACK(addNumberNodeInt(start, *str));
		return true;
	}

	if(*str[0] == '.')
	{
		(*str)++;
		while(isDigit(*str[0]))
			(*str)++;
	}
	if(*str[0] == 'e')
	{
		(*str)++;
		if(*str[0] == '-')
			(*str)++;
		while(isDigit(*str[0]))
			(*str)++;
	}
	if(start[0] == '.' && (*str)-start == 1)
	{
		(*str) = start;
		return false;
	}
	if(**str == 'f')
	{
		(*str)++;
		CALLBACK(addNumberNodeFloat(start, *str));
		return true;
	}else{
		CALLBACK(addNumberNodeDouble(start, *str));
		return true;
	}
}

// arrayDef	=	('[' >> (term4_9 | epsP[addUnfixedArraySize]) >> ']' >> !arrayDef)[convertTypeToArray];
bool  ParseArrayDefinition(char** str)
{
	SKIP_SPACE(str);
	char *start = *str;
	if(**str != '[')
		return false;
	(*str)++;
	if(!ParseTernaryExpr(str))
		CALLBACK(addUnfixedArraySize(*str, *str));
	SKIP_SPACE(str);
	if(**str != ']')
		ThrowError("ERROR: Matching ']' not found", *str);
	(*str)++;
	ParseArrayDefinition(str);
	CALLBACK(convertTypeToArray(start, *str));
	return true;
}
// seltype		=	((strP("auto") | typenameP(varname))[selType] | (strP("typeof") >> chP('(') >> ((varname[GetVariableType] >> chP(')')) | (term5[SetTypeOfLastNode] >> chP(')'))))) >> *((lexemeD[strP("ref") >> (~alnumP | nothingP)])[convertTypeToRef] | arrayDef);
bool  ParseSelectType(char** str)
{
	SKIP_SPACE(str);
	char *curr = *str;
	if(memcmp(curr, "typeof", 6) == 0)
	{
		curr += 6;
		SKIP_SPACE(&curr);
		if(*curr != '(')
			ThrowError("ERROR: typeof must be followed by '('", curr);
		curr++;
		char *old = curr;
		if(ParseVaribleSet(&curr))
		{
			CALLBACK(SetTypeOfLastNode(old, curr));
			SKIP_SPACE(&curr);
			if(*curr != ')')
				ThrowError("ERROR: ')' not found after expression in typeof", curr);
			curr++;
		}else{
			ThrowError("ERROR: expression not found after typeof(", curr);
		}
	}else if(memcmp(curr, "auto", 4) == 0){
		curr += 4;
		CALLBACK(selType(curr-4, curr));
	}else{
		char *old = curr;
		if(!ParseTypename(&curr))
			return false;
		CALLBACK(selType(old, curr));
	}
	*str = curr;

	for(;;)
	{
		SKIP_SPACE(&curr);
		if(memcmp(curr, "ref", 3) == 0 && !(chartype_table[curr[3]] & ct_symbol))
		{
			curr += 3;
			CALLBACK(convertTypeToRef(curr-3, curr));
		}else{
			if(!ParseArrayDefinition(&curr))
				break;
		}
	}
	*str = curr;
	return true;
}
// isconst		=	epsP[AssignVar<bool>(currValConst, false)] >> !strP("const")[AssignVar<bool>(currValConst, true)];
bool  ParseIsConst(char** str)
{
	SKIP_SPACE(str);
	CALLBACK(SetTypeConst(false));
	if(memcmp(*str, "const", 5) == 0)
	{
		(*str) += 5;
		CALLBACK(SetTypeConst(true));
	}
	return true;
}
// varname		=	lexemeD[alphaP >> *(alnumP | '_')];
bool  ParseVariableName(char** str)
{
	SKIP_SPACE(str);
	if(!(chartype_table[**str] & ct_start_symbol))
		return false;
	while(chartype_table[**str] & ct_symbol)
		(*str)++;
	return true;
}
/* classdef	=	((strP("align") >> '(' >> intP[StrToInt(currAlign)] >> ')') | (strP("noalign") | epsP)[AssignVar<unsigned int>(currAlign, 0)]) >>
						strP("class") >> varname[TypeBegin] >> chP('{') >>
						*(
							funcdef |
							(seltype >> varname[TypeAddMember] >> *(',' >> varname[TypeAddMember]) >> chP(';'))
						)
						>> chP('}')[TypeFinish];*/
bool  ParseClassDefinition(char** str)
{
	SKIP_SPACE(str);
	char *curr = *str;
	if(!ParseAlignment(&curr))
		CALLBACK(SetCurrentAlignment(0));

	SKIP_SPACE(&curr);
	if(memcmp(curr, "class", 5) == 0)
	{
		curr += 5;
		SKIP_SPACE(&curr);
		char *old = curr;
		if(!ParseVariableName(&curr))
			ThrowError("ERROR: class name expected", curr);
		CALLBACK(TypeBegin(old, curr));
		SKIP_SPACE(&curr);
		if(*curr != '{')
			ThrowError("ERROR: '{' not found after class name", curr);
		curr++;

		for(;;)
		{
			if(!ParseFunctionDefinition(&curr))
			{
				if(!ParseSelectType(&curr))
					break;
				SKIP_SPACE(&curr);
				char *old = curr;
				if(!ParseVariableName(&curr))
					ThrowError("ERROR: class member name expected after type", curr);
				CALLBACK(TypeAddMember(old, curr));
				SKIP_SPACE(&curr);
				while(*curr == ',')
				{
					curr++;
					SKIP_SPACE(&curr);
					char *old = curr;
					if(!ParseVariableName(&curr))
						ThrowError("ERROR: member name expected after ','", curr);
					CALLBACK(TypeAddMember(old, curr));
					SKIP_SPACE(&curr);
				}
				if(*curr != ';')
					ThrowError("ERROR: ';' not found after class member list", curr);
				curr++;
			}
		}
		SKIP_SPACE(&curr);
		if(*curr != '}')
			ThrowError("ERROR: '}' not found after class definition", curr);
		curr++;
		CALLBACK(TypeFinish(curr, curr));
		*str = curr;
		return true;
	}
	return false;
}
/* funccall	=	varname[strPush] >> 
				('(' | (epsP[strPop] >> nothingP)) >>
				epsP[PushBackVal<std::vector<unsigned int>, unsigned int>(callArgCount, 0)] >> 
				!(
				term5[ArrBackInc<std::vector<unsigned int> >(callArgCount)] >>
				*(',' >> term5[ArrBackInc<std::vector<unsigned int> >(callArgCount)])
				) >>
				(')' | epsP[ThrowError("ERROR: ')' not found after function call")]);*/
bool  ParseFunctionCall(char** str, bool memberFunctionCall)
{
	char *curr = *str;
	ParseVariableName(&curr);
	CALLBACK(ParseStrPush(*str, curr));
	SKIP_SPACE(&curr);
	if(*curr != '(')
	{
		CALLBACK(ParseStrPop(NULL, NULL));
		return false;
	}
	curr++;
	unsigned int callArgCount = 0;
	if(ParseVaribleSet(&curr))
	{
		callArgCount++;
		SKIP_SPACE(&curr);
		while(*curr == ',')
		{
			curr++;
			if(!ParseVaribleSet(&curr))
				ThrowError("ERROR: expression not found after ',' in function parameter list", curr);
			callArgCount++;
			SKIP_SPACE(&curr);
		}
	}
	SKIP_SPACE(&curr);
	if(*curr != ')')
		ThrowError("ERROR: ')' not found after function parameter list", curr);

	if(memberFunctionCall)
		CALLBACK(AddMemberFunctionCall(*str, curr, callArgCount));
	else
		CALLBACK(addFuncCallNode(*str, curr, callArgCount));

	*str = curr+1;
	return true;
}
// funcvars	=	!(isconst >> seltype >> varname[strPush][FunctionParam]) >> *(',' >> isconst >> seltype >> varname[strPush][FunctionParam]);
bool  ParseFunctionVariables(char** str)
{
	SKIP_SPACE(str);
	char *curr = *str;
	ParseIsConst(&curr);
	if(!ParseSelectType(&curr))
		return true;

	SKIP_SPACE(&curr);
	char *old = curr;
	if(!ParseVariableName(&curr))
		ThrowError("ERROR: variable name not found after type in function variable list", curr);
	CALLBACK(ParseStrPush(old, curr));
	CALLBACK(FunctionParam(old, curr));

	SKIP_SPACE(&curr);
	while(*curr == ',')
	{
		curr++;
		ParseIsConst(&curr);
		if(!ParseSelectType(&curr))
			ThrowError("ERROR: type name not found after ',' in function variable list", curr);
		SKIP_SPACE(&curr);
		char *old = curr;
		if(!ParseVariableName(&curr))
			ThrowError("ERROR: variable name not found after type in function variable list", curr);
		CALLBACK(ParseStrPush(old, curr));
		CALLBACK(FunctionParam(old, curr));
		SKIP_SPACE(&curr);
	}
	*str = curr;
	return true;
}
// funcdef		=	seltype >> varname[strPush] >> (chP('(')[FunctionAdd] | (epsP[strPop] >> nothingP)) >> funcvars[FunctionStart] >> chP(')') >> chP('{') >> (code | epsP[addVoidNode])[FunctionEnd] >> chP('}');
bool  ParseFunctionDefinition(char** str)
{
	SKIP_SPACE(str);
	char *curr = *str;
	if(!ParseSelectType(&curr))
		return false;

	SKIP_SPACE(&curr);
	char *old = curr;
	if(!ParseVariableName(&curr))
		return false;
	CALLBACK(ParseStrPush(old, curr));

	SKIP_SPACE(&curr);
	if(*curr == '(')
	{
		CALLBACK(FunctionAdd(NULL, NULL));
	}else{
		CALLBACK(ParseStrPop(NULL, NULL));
		return false;
	}
	curr++;

	ParseFunctionVariables(&curr);
	CALLBACK(FunctionStart(curr, curr));

	SKIP_SPACE(&curr);
	if(*curr != ')')
		ThrowError("ERROR: ')' not found after function variable list", curr);
	curr++;
	SKIP_SPACE(&curr);
	if(*curr != '{')
		ThrowError("ERROR: '{' not found after function header", curr);
	curr++;

	if(!ParseCode(&curr))
		CALLBACK(addVoidNode(curr, curr));
	CALLBACK(FunctionEnd(*str, curr));
	
	SKIP_SPACE(&curr);
	if(*curr != '}')
		ThrowError("ERROR: '}' not found after function body", curr);
	curr++;
	*str = curr;
	return true;
}
// funcProt	=	seltype >> varname[strPush] >> (chP('(')[FunctionAdd] | (epsP[strPop] >> nothingP)) >> funcvars >> chP(')') >> chP(';');
bool  ParseFunctionPrototype(char** str)
{
	SKIP_SPACE(str);
	char *curr = *str;
	if(!ParseSelectType(&curr))
		return false;

	SKIP_SPACE(&curr);
	char *old = curr;
	if(!ParseVariableName(&curr))
		return false;
	CALLBACK(ParseStrPush(old, curr));

	SKIP_SPACE(&curr);
	if(*curr == '(')
	{
		CALLBACK(FunctionAdd(NULL, NULL));
	}else{
		CALLBACK(ParseStrPop(NULL, NULL));
		return false;
	}
	curr++;

	ParseFunctionVariables(&curr);

	SKIP_SPACE(&curr);
	if(*curr != ')')
		ThrowError("ERROR: ')' not found after function variable list", curr);
	curr++;
	SKIP_SPACE(&curr);
	if(*curr != ';')
		ThrowError("ERROR: ';' not found after function header", curr);
	curr++;
	*str = curr;
	return true;
}
/* addvarp		=
			(
				varname[strPush] >>
				!('[' >> (term4_9 | epsP[addUnfixedArraySize]) >> ']')[convertTypeToArray]
			)[pushType][addVar] >>
			(('=' >> (term5 | epsP[ThrowError("ERROR: expression not found after '='")]))[AddDefineVariableNode][addPopNode][popType] | epsP[addVarDefNode])[popType][strPop];*/
bool  ParseAddVariable(char** str)
{
	SKIP_SPACE(str);
	char *curr = *str;
	if(!ParseVariableName(&curr))
		return false;
	CALLBACK(ParseStrPush(*str, curr));

	SKIP_SPACE(&curr);
	char *old = curr;
	if(*curr == '[')
	{
		curr++;
		if(!ParseTernaryExpr(&curr))
			CALLBACK(addUnfixedArraySize(curr, curr));
		SKIP_SPACE(&curr);
		if(*curr != ']')
			ThrowError("ERROR: Matching ']' not found", curr);
		curr++;
		CALLBACK(convertTypeToArray(old, curr));
	}
	CALLBACK(pushType(*str, curr));
	CALLBACK(addVar(*str, curr));
	SKIP_SPACE(&curr);
	old = curr;
	if(*curr == '=')
	{
		curr++;
		if(!ParseVaribleSet(&curr))
			ThrowError("ERROR: expression not found after '='", curr);
		CALLBACK(AddDefineVariableNode(old, curr));
		CALLBACK(addPopNode(old, curr));
		CALLBACK(popType(old, curr));
	}else{
		CALLBACK(addVarDefNode(curr, curr));
	}
	CALLBACK(popType(old, curr));
	CALLBACK(ParseStrPop(NULL, NULL));
	*str = curr;
	return true;
}
// vardefsub	=	addvarp[SetStringToLastNode] >> *(',' >> vardefsub)[addTwoExprNode];
bool  ParseVariableDefineSub(char** str)
{
	char *old = *str;
	if(!ParseAddVariable(str))
		return false;
	CALLBACK(SetStringToLastNode(old, *str));

	SKIP_SPACE(str);
	old = *str;
	while(**str == ',')
	{
		(*str)++;
		if(!ParseAddVariable(str))
			ThrowError("ERROR: next variable definition excepted after ','", (*str));
		CALLBACK(addTwoExprNode(NULL, NULL));
		SKIP_SPACE(str);
	}
	return true;
}
// align = (strP("noalign")[AssignVar<unsigned int>(currAlign, 0)] | (strP("align") >> '(' >> intP[StrToInt(currAlign)] >> ')'))
bool ParseAlignment(char** str)
{
	char *curr = *str;
	SKIP_SPACE(&curr);
	if(memcmp(curr, "noalign", 7) == 0)
	{
		curr += 7;
		CALLBACK(SetCurrentAlignment(0));
		*str = curr;
		return true;
	}else if(memcmp(curr, "align", 5) == 0)
	{
		curr += 5;
		SKIP_SPACE(str);
		if(*curr != '(')
			ThrowError("ERROR: '(' expected after align", curr);
		curr++;
		char *start = curr;
		if(!ParseInt(&curr))
			ThrowError("ERROR: alignment value not found after align(", curr);
		CALLBACK(SetCurrentAlignment(atoi(start)));
		SKIP_SPACE(&curr);
		if(*curr != ')')
			ThrowError("ERROR: ')' expected after alignment value", curr);
		curr++;
		*str = curr;
		return true;
	}
	return false;
}
/* vardef		=
			epsP[AssignVar<unsigned int>(currAlign, 0xFFFFFFFF)] >>
			isconst >>
			!(strP("noalign")[AssignVar<unsigned int>(currAlign, 0)] | (strP("align") >> '(' >> intP[StrToInt(currAlign)] >> ')')) >>
			seltype >>
			vardefsub;*/
bool  ParseVariableDefine(char** str)
{
	char *curr = *str;
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
/* ifexpr		=
			(
				strWP("if") >>
				('(' | epsP[ThrowError("ERROR: '(' not found after 'if'")]) >>
				(term5 | epsP[ThrowError("ERROR: condition not found in 'if' statement")]) >>
				(')' | epsP[ThrowError("ERROR: closing ')' not found after 'if' condition")])
			)[SaveStringIndex] >>
			expression >>
			((strP("else") >> expression)[addIfElseNode] | epsP[addIfNode])[SetStringFromIndex];*/
bool  ParseIfExpr(char** str)
{
	SKIP_SPACE(str);
	char *curr = *str;
	if(memcmp(curr, "if", 2) == 0 && !(chartype_table[curr[2]] & ct_symbol))
	{
		curr += 2;
		SKIP_SPACE(&curr);
		if(*curr != '(')
			ThrowError("ERROR: '(' not found after 'if'", curr);
		curr++;
		if(!ParseVaribleSet(&curr))
			ThrowError("ERROR: condition not found in 'if' statement", curr);
		SKIP_SPACE(&curr);
		if(*curr != ')')
			ThrowError("ERROR: closing ')' not found after 'if' condition", curr);
		curr++;
		CALLBACK(SaveStringIndex(*str, curr));
		if(!ParseExpression(&curr))
			ThrowError("ERROR: expression not found after 'if'", curr);

		SKIP_SPACE(&curr);
		if(memcmp(curr, "else", 4) == 0 && !(chartype_table[curr[4]] & ct_symbol))
		{
			curr += 4;
			if(!ParseExpression(&curr))
				ThrowError("ERROR: expression not found after 'else'", curr);
			CALLBACK(addIfElseNode(curr, curr));
		}else{
			CALLBACK(addIfNode(curr, curr));
		}
		CALLBACK(SetStringFromIndex(curr, curr));

		*str = curr;
		return true;
	}
	return false;
}
/* forexpr		=
			(
				strWP("for")[saveVarTop] >>
				('(' | epsP[ThrowError("ERROR: '(' not found after 'for'")]) >>
				(
					(
						chP('{') >>
						(code | epsP[addVoidNode]) >>
						(chP('}') | epsP[ThrowError("ERROR: '}' not found after '{'")])
					) |
					vardef |
					term5[addPopNode] |
					epsP[addVoidNode]
				) >>
				(';' | epsP[ThrowError("ERROR: ';' not found after initializer in 'for'")]) >>
				(term5 | epsP[ThrowError("ERROR: condition not found in 'for' statement")]) >>
				(';' | epsP[ThrowError("ERROR: ';' not found after condition in 'for'")]) >>
				((chP('{') >> code >> chP('}')) | term5[addPopNode] | epsP[addVoidNode]) >>
				(')' | epsP[ThrowError("ERROR: ')' not found after 'for' statement")])
			)[SaveStringIndex] >> expression[addForNode][SetStringFromIndex];*/
bool  ParseForExpr(char** str)
{
	SKIP_SPACE(str);
	char *curr = *str;
	if(memcmp(curr, "for", 3) == 0 && !(chartype_table[curr[3]] & ct_symbol))
	{
		curr += 3;
		CALLBACK(saveVarTop(curr, curr));
		SKIP_SPACE(&curr);
		if(*curr != '(')
			ThrowError("ERROR: '(' not found after 'for'", curr);
		curr++;
		SKIP_SPACE(&curr);
		if(*curr == '{')
		{
			curr++;
			if(!ParseCode(&curr))
				CALLBACK(addVoidNode(curr, curr));
			SKIP_SPACE(&curr);
			if(*curr != '}')
				ThrowError("ERROR: '}' not found after '{'", curr);
			curr++;
		}else{
			if(!ParseVariableDefine(&curr))
			{
				if(!ParseVaribleSet(&curr))
					CALLBACK(addVoidNode(curr, curr));
				else
					CALLBACK(addPopNode(curr, curr));
			}
		}

		SKIP_SPACE(&curr);
		if(*curr != ';')
			ThrowError("ERROR: ';' not found after initializer in 'for'", curr);
		curr++;

		if(!ParseVaribleSet(&curr))
			ThrowError("ERROR: condition not found in 'for' statement", curr);

		SKIP_SPACE(&curr);
		if(*curr != ';')
			ThrowError("ERROR: ';' not found after condition in 'for'", curr);
		curr++;

		SKIP_SPACE(&curr);
		if(*curr == '{')
		{
			curr++;
			if(!ParseCode(&curr))
				CALLBACK(addVoidNode(curr, curr));
			SKIP_SPACE(&curr);
			if(*curr != '}')
				ThrowError("ERROR: '}' not found after '{'", curr);
			curr++;
		}else{
			if(!ParseVaribleSet(&curr))
				CALLBACK(addVoidNode(curr, curr));
			else
				CALLBACK(addPopNode(*str, curr));
		}

		SKIP_SPACE(&curr);
		if(*curr != ')')
			ThrowError("ERROR: ')' not found after 'for' statement", curr);
		curr++;

		CALLBACK(SaveStringIndex(*str, curr));

		if(!ParseExpression(&curr))
			ThrowError("ERROR: body not found after 'for' header", curr);
		CALLBACK(addForNode(curr, curr));
		CALLBACK(SetStringFromIndex(curr, curr));

		*str = curr;
		return true;
	}
	return false;
}
/* whileexpr	=
			strWP("while")[saveVarTop] >>
			(
				('(' | epsP[ThrowError("ERROR: '(' not found after 'while'")]) >>
				(term5 | epsP[ThrowError("ERROR: expression expected after 'while('")]) >>
				(')' | epsP[ThrowError("ERROR: closing ')' not found after expression in 'while' statement")])
			) >>
			(expression[addWhileNode] | epsP[ThrowError("ERROR: expression expected after 'while(...)'")]);*/
bool  ParseWhileExpr(char** str)
{
	SKIP_SPACE(str);
	char *curr = *str;
	if(memcmp(curr, "while", 5) == 0 && !(chartype_table[curr[5]] & ct_symbol))
	{
		curr += 5;
		CALLBACK(saveVarTop(curr, curr));
		SKIP_SPACE(&curr);
		if(*curr != '(')
			ThrowError("ERROR: '(' not found after 'while'", curr);
		curr++;
		if(!ParseVaribleSet(&curr))
			ThrowError("ERROR: expression expected after 'while('", curr);
		SKIP_SPACE(&curr);
		if(*curr != ')')
			ThrowError("ERROR: closing ')' not found after expression in 'while' statement", curr);
		curr++;
	
		if(!ParseExpression(&curr))
			ThrowError("ERROR: expression expected after 'while(...)'", curr);
		CALLBACK(addWhileNode(curr, curr));

		*str = curr;
		return true;
	}
	return false;
}
/* doexpr		=	
			strWP("do")[saveVarTop] >> 
			(expression | epsP[ThrowError("ERROR: expression expected after 'do'")]) >> 
			(strP("while") | epsP[ThrowError("ERROR: 'while' expected after 'do' statement")]) >>
			(
				('(' | epsP[ThrowError("ERROR: '(' not found after 'while'")]) >> 
				(term5 | epsP[ThrowError("ERROR: expression not found after 'while('")]) >> 
				(')' | epsP[ThrowError("ERROR: closing ')' not found after expression in 'while' statement")])
			)[addDoWhileNode] >> 
			(';' | epsP[ThrowError("ERROR: while(...) should be followed by ';'")]);*/
bool  ParseDoWhileExpr(char** str)
{
	SKIP_SPACE(str);
	char *curr = *str;
	if(memcmp(curr, "do", 2) == 0 && !(chartype_table[curr[2]] & ct_symbol))
	{
		curr += 2;
		CALLBACK(saveVarTop(curr, curr));

		if(!ParseExpression(&curr))
			ThrowError("ERROR: expression expected after 'do'", curr);

		SKIP_SPACE(&curr);
		if(memcmp(curr, "while", 5) != 0)
			ThrowError("ERROR: 'while' expected after 'do' statement", curr);
		SKIP_SPACE(&curr);
		if(*curr != '(')
			ThrowError("ERROR: '(' not found after 'while'", curr);
		curr++;
		if(!ParseVaribleSet(&curr))
			ThrowError("ERROR: expression expected after 'while('", curr);
		SKIP_SPACE(&curr);
		if(*curr != ')')
			ThrowError("ERROR: closing ')' not found after expression in 'while' statement", curr);
		curr++;

		CALLBACK(addDoWhileNode(curr, curr));

		SKIP_SPACE(&curr);
		if(*curr != ';')
			ThrowError("ERROR: while(...) should be followed by ';'", curr);
		curr++;

		*str = curr;
		return true;
	}
	return false;
}
/* switchexpr	=
			strP("switch") >>
			('(') >>
			(term5 | epsP[ThrowError("ERROR: expression not found after 'switch('")])[preSwitchNode] >>
			(')' | epsP[ThrowError("ERROR: closing ')' not found after expression in 'switch' statement")]) >>
			('{' | epsP[ThrowError("ERROR: '{' not found after 'switch(...)'")]) >>
			(strWP("case") >> term5 >> ':' >> expression >> *expression[addTwoExprNode])[addCaseNode] >>
			*(strWP("case") >> term5 >> ':' >> expression >> *expression[addTwoExprNode])[addCaseNode] >>
			('}' | epsP[ThrowError("ERROR: '}' not found after 'switch' statement")])[addSwitchNode];*/
bool  ParseSwitchExpr(char** str)
{
	SKIP_SPACE(str);
	char *curr = *str;
	if(memcmp(curr, "switch", 6) == 0 && !(chartype_table[curr[6]] & ct_symbol))
	{
		curr += 6;

		SKIP_SPACE(&curr);
		if(*curr != '(')
			ThrowError("ERROR: '(' not found after 'switch'", curr);
		curr++;

		if(!ParseVaribleSet(&curr))
			ThrowError("ERROR: expression not found after 'switch('", curr);
		CALLBACK(preSwitchNode(curr, curr));

		SKIP_SPACE(&curr);
		if(*curr != ')')
			ThrowError("ERROR: closing ')' not found after expression in 'switch' statement", curr);
		curr++;

		SKIP_SPACE(&curr);
		if(*curr != '{')
			ThrowError("ERROR: '{' not found after 'switch(...)'", curr);
		curr++;

		SKIP_SPACE(&curr);
		while(memcmp(curr, "case", 4) == 0 && !(chartype_table[curr[4]] & ct_symbol))
		{
			curr += 4;
			if(!ParseVaribleSet(&curr))
				ThrowError("ERROR: expression expected after 'case'", curr);
			SKIP_SPACE(&curr);
			if(*curr != ':')
				ThrowError("ERROR: ':' not found after 'case' expression", curr);
			curr++;

			if(!ParseExpression(&curr))
				ThrowError("ERROR: expression expected after 'case:'", curr);
			while(ParseExpression(&curr))
				CALLBACK(addTwoExprNode(curr, curr));
			CALLBACK(addCaseNode(curr, curr));
			SKIP_SPACE(&curr);
		}

		SKIP_SPACE(&curr);
		if(*curr != '}')
			ThrowError("ERROR: '}' not found after 'switch' statement", curr);
		curr++;
		CALLBACK(addSwitchNode(curr, curr));
		*str = curr;
		return true;
	}
	return false;
}
/* retexpr		=
			(
				strWP("return") >>
				(term5 | epsP[addVoidNode]) >>
				(+chP(';') | epsP[ThrowError("ERROR: return must be followed by ';'")])
			)[addReturnNode];*/
bool  ParseReturnExpr(char** str)
{
	SKIP_SPACE(str);
	char *curr = *str;
	if(memcmp(curr, "return", 6) == 0 && !(chartype_table[curr[6]] & ct_symbol))
	{
		curr += 6;
		if(!ParseVaribleSet(&curr))
			CALLBACK(addVoidNode(curr, curr));
		SKIP_SPACE(&curr);
		if(*curr != ';')
			ThrowError("ERROR: return must be followed by ';'", curr);
		curr++;
		CALLBACK(addReturnNode(*str, curr));
		*str = curr;
		return true;
	}
	return false;
}
/* breakexpr	=	(
			strWP("break") >>
			(+chP(';') | epsP[ThrowError("ERROR: break must be followed by ';'")])
			)[addBreakNode];*/
bool  ParseBreakExpr(char** str)
{
	SKIP_SPACE(str);
	char *curr = *str;
	if(memcmp(curr, "break", 5) == 0 && !(chartype_table[curr[5]] & ct_symbol))
	{
		curr += 5;
		SKIP_SPACE(&curr);
		if(*curr != ';')
			ThrowError("ERROR: break must be followed by ';'", curr);
		curr++;
		CALLBACK(addBreakNode(*str, curr));
		*str = curr;
		return true;
	}
	return false;
}
/* continueExpr	=
			(
				strWP("continue") >>
				(+chP(';') | epsP[ThrowError("ERROR: continue must be followed by ';'")])
			)[AddContinueNode];*/
bool  ParseContinueExpr(char** str)
{
	SKIP_SPACE(str);
	char *curr = *str;
	if(memcmp(curr, "continue", 8) == 0 && !(chartype_table[curr[8]] & ct_symbol))
	{
		curr += 8;
		SKIP_SPACE(&curr);
		if(*curr != ';')
			ThrowError("ERROR: continue must be followed by ';'", curr);
		curr++;
		CALLBACK(AddContinueNode(*str, curr));
		*str = curr;
		return true;
	}
	return false;
}
// group		=	'(' >> term5 >> (')' | epsP[ThrowError("ERROR: closing ')' not found after '('")]);
bool  ParseGroup(char** str)
{
	SKIP_SPACE(str);
	char *curr = *str;
	if(*curr != '(')
		return false;
	curr++;
	if(!ParseVaribleSet(&curr))
		ThrowError("ERROR: expression not found after '('", curr);
	SKIP_SPACE(&curr);
	if(*curr != ')')
		ThrowError("ERROR: closing ')' not found after '('", curr);
	curr++;
	*str = curr;
	return true;
}
// variable	= (chP('*') >> variable)[AddDereferenceNode] | (((varname - strP("case")) >> (~chP('(') | nothingP))[AddGetAddressNode] >> *postExpr);
bool  ParseVariable(char** str)
{
	SKIP_SPACE(str);
	char *curr = *str;
	if(*curr == '*')
	{
		curr++;
		if(!ParseVariable(&curr))
			ThrowError("ERROR: variable name not found after '*'", curr);
		CALLBACK(AddDereferenceNode(*str, curr));
		*str = curr;
		return true;
	}
	SKIP_SPACE(&curr);
	char *old = curr;
	if(!ParseVariableName(&curr))
		return false;
	char *old2 = curr;
	SKIP_SPACE(&curr);
	if(*curr == '(' || memcmp("case", old, 4) == 0)
		return false;
	CALLBACK(AddGetAddressNode(old, old2));
	while(ParsePostExpression(&curr));
	*str = curr;
	return true;
}
// postExpr	=	('.' >> varname[strPush] >> (~chP('(') | (epsP[strPop] >> nothingP)))[AddMemberAccessNode] | ('[' >> term5 >> ']')[AddArrayIndexNode];
bool  ParsePostExpression(char** str)
{
	SKIP_SPACE(str);
	char *curr = *str;
	if(*curr == '.')
	{
		curr++;
		SKIP_SPACE(str);
		char *old = curr;
		if(!ParseVariableName(&curr))
			ThrowError("ERROR: member variable or function expected after '.'", curr);
		CALLBACK(ParseStrPush(old, curr));

		SKIP_SPACE(&curr);
		if(*curr == '(')
		{
			CALLBACK(ParseStrPop(NULL, NULL));
			return false;
		}
		CALLBACK(AddMemberAccessNode(*str, curr));
	}else if(*curr == '['){
		curr++;
		if(!ParseVaribleSet(&curr))
			ThrowError("ERROR: expression not found after '['", curr);
		SKIP_SPACE(&curr);
		if(*curr != ']')
			ThrowError("ERROR: ']' not found after expression", curr);
		curr++;
		CALLBACK(AddArrayIndexNode(*str, curr));
	}else{
		return false;
	}
	*str = curr;
	return true;
}
/* term1		=
			(strP("--") >> variable[AddPreOrPostOp(false, true)])[popType] | 
			(strP("++") >> variable[AddPreOrPostOp(true, true)])[popType] |
			(chP('\"') >> *(strP("\\\"") | (anycharP - chP('\"'))) >> chP('\"'))[strPush][addStringNode] |
			(chP('\'') >> ((chP('\\') >> anycharP) | anycharP) >> chP('\''))[addChar] |
			(strP("sizeof") >> chP('(')[pushType] >> (seltype[pushType][GetTypeSize][popType] | term5[AssignVar<bool>(sizeOfExpr, true)][GetTypeSize]) >> chP(')')[popType]) |
			(chP('{')[PushBackVal<std::vector<unsigned int>, unsigned int>(arrElementCount, 0)] >> term4_9 >> *(chP(',') >> term4_9[ArrBackInc<std::vector<unsigned int> >(arrElementCount)]) >> chP('}'))[addArrayConstructor] |
			funcdef |
			funccall[addFuncCallNode] |
			(variable >>
				(
					strP("++")[AddPreOrPostOp(true, false)] |
					strP("--")[AddPreOrPostOp(false, false)] |
					('.' >> funccall)[AddMemberFunctionCall] |
					epsP[AddGetVariableNode]
				)[popType]
			);*/
bool ParseTerminal(char** str)
{
	SKIP_SPACE(str);
	char *curr = *str;
	if(ParseNumber(str))
		return true;
	if(*curr == '&')
	{
		curr++;
		if(!ParseVariable(&curr))
			ThrowError("ERROR: variable not found after '&'", curr);
		CALLBACK(popType(*str, curr));
		*str = curr;
		return true;
	}
	if(*curr == '!')
	{
		curr++;
		if(!ParseTerminal(&curr))
			ThrowError("ERROR: expression not found after '!'", curr);
		CALLBACK(addLogNotNode(*str, curr));
		*str = curr;
		return true;
	}
	if(*curr == '~')
	{
		curr++;
		if(!ParseTerminal(&curr))
			ThrowError("ERROR: expression not found after '~'", curr);
		CALLBACK(addBitNotNode(*str, curr));
		*str = curr;
		return true;
	}
	if(curr[0] == '-' && curr[1] == '-')
	{
		curr += 2;
		if(!ParseVariable(&curr))
			ThrowError("ERROR: variable not found after '--'", curr);
		CALLBACK(AddPreOrPostOpNode(false, true));
		CALLBACK(popType(*str, curr));
		*str = curr;
		return true;
	}
	if(curr[0] == '+' && curr[1] == '+')
	{
		curr += 2;
		if(!ParseVariable(&curr))
			ThrowError("ERROR: variable not found after '++'", curr);
		CALLBACK(AddPreOrPostOpNode(true, true));
		CALLBACK(popType(*str, curr));
		*str = curr;
		return true;
	}
	if(*curr == '+')
	{
		curr++;
		SKIP_SPACE(&curr);
		while(*curr == '+')
		{
			curr++;
			SKIP_SPACE(&curr);
		}
		if(!ParseTerminal(&curr))
			ThrowError("ERROR: expression not found after '+'", curr);
		*str = curr;
		return true;
	}
	if(*curr == '-')
	{
		int negCount = 1;
		curr++;
		SKIP_SPACE(&curr);
		while(*curr == '-')
		{
			negCount++;
			curr++;
			SKIP_SPACE(&curr);
		}
		if(!ParseTerminal(&curr))
			ThrowError("ERROR: expression not found after '-'", curr);
		if(negCount % 2 == 1)
			CALLBACK(addNegNode(*str, curr));
		*str = curr;
		return true;
	}
	if(*curr == '\"')
	{
		curr++;
		while(!(*curr == '\"' && curr[-1] != '\\'))
			curr++;
		curr++;
		CALLBACK(ParseStrPush(*str, curr));
		CALLBACK(addStringNode(*str, curr));
		*str = curr;
		return true;
	}
	if(*curr == '\'')
	{
		curr++;
		if(*curr == '\\')
			curr++;
		curr++;
		if(*curr != '\'')
			ThrowError("ERROR: ' not found after character", curr);
		curr++;
		CALLBACK(addNumberNodeChar(*str, curr));
		*str = curr;
		return true;
	}
	if(*curr == 's' && memcmp(curr, "sizeof", 6) == 0)
	{
		curr += 6;
		SKIP_SPACE(&curr);
		if(*curr != '(')
			ThrowError("ERROR: sizeof must be followed by '('", curr);
		curr++;
		CALLBACK(pushType(*str, curr));
		char *old = curr;
		if(ParseSelectType(&curr))
		{
			CALLBACK(pushType(old, curr));
			CALLBACK(GetTypeSize(old, curr, false));
			CALLBACK(popType(old, curr));
		}else{
			if(ParseVaribleSet(&curr))
				CALLBACK(GetTypeSize(old, curr, true));
			else
				ThrowError("ERROR: expression or type not found after sizeof(", curr);
		}
		SKIP_SPACE(&curr);
		if(*curr != ')')
			ThrowError("ERROR: ')' not found after expression in sizeof", curr);
		curr++;
		CALLBACK(popType(old, curr));
		*str = curr;
		return true;
	}
	if(*curr == '{')
	{
		curr++;
		unsigned int arrElementCount = 0;
		if(!ParseTernaryExpr(&curr))
			ThrowError("ERROR: value not found after '{'", curr);
		SKIP_SPACE(&curr);
		while(*curr == ',')
		{
			curr++;
			if(!ParseTernaryExpr(&curr))
				ThrowError("ERROR: value not found after ','", curr);
			arrElementCount++;
			SKIP_SPACE(&curr);
		}
		SKIP_SPACE(&curr);
		if(*curr != '}')
			ThrowError("ERROR: '}' not found after inline array", curr);
		curr++;
		CALLBACK(addArrayConstructor(*str, curr, arrElementCount));
		*str = curr;
		return true;
	}
	if(ParseGroup(str))
		return true;
	if(ParseFunctionDefinition(str))
		return true;
	if(ParseFunctionCall(str, false))
		return true;
	if(ParseVariable(str))
	{
		SKIP_SPACE(str);
		if((*str)[0] == '-' && (*str)[1] == '-')
		{
			(*str) += 2;
			CALLBACK(AddPreOrPostOpNode(false, false));
		}else if((*str)[0] == '+' && (*str)[1] == '+')
		{
			(*str) += 2;
			CALLBACK(AddPreOrPostOpNode(true, false));
		}else if((*str)[0] == '.'){
			(*str)++;
			if(!ParseFunctionCall(str, true))
				ThrowError("ERROR: function call is excepted after '.'", curr);
		}else{
			CALLBACK(AddGetVariableNode(curr, *str));
		}
		CALLBACK(popType(curr, *str));
		return true;
	}
	return false;
}
// term2		=	term1 >> *((strP("**") >> term1)[addCmd(cmdPow)]);
bool ParsePower(char** str)
{
	if(!ParseTerminal(str))
		return false;
	SKIP_SPACE(str);
	char *curr = *str;
	while(curr[0] == '*' && curr[1] == '*')
	{
		curr += 2;
		if(!ParseTerminal(&curr))
			ThrowError("ERROR: expression not found after **", curr);
		CALLBACK((addCmd(cmdPow))(*str, curr));
		SKIP_SPACE(&curr);
	}
	*str = curr;
	return true;
}
// term3		=	term2 >> *(('*' >> term2)[addCmd(cmdMul)] | ('/' >> term2)[addCmd(cmdDiv)] | ('%' >> term2)[addCmd(cmdMod)]);
bool ParseMultiplicative(char** str)
{
	if(!ParsePower(str))
		return false;
	SKIP_SPACE(str);
	char *curr = *str;
	while(curr[0] == '*' || curr[0] == '/' || curr[0] == '%')
	{
		char op = *curr;
		curr++;
		if(!ParsePower(&curr))
			ThrowError("ERROR: expression not found after multiplicative expression", curr);
		CALLBACK((addCmd((CmdID)(op == '*' ? cmdMul : (op == '/' ? cmdDiv : cmdMod))))(*str, curr));
		SKIP_SPACE(&curr);
	}
	*str = curr;
	return true;
}
// term4		=	term3 >> *(('+' >> term3)[addCmd(cmdMul)] | ('-' >> term3)[addCmd(cmdDiv)]);
bool ParseAdditive(char** str)
{
	if(!ParseMultiplicative(str))
		return false;
	SKIP_SPACE(str);
	char *curr = *str;
	while(curr[0] == '+' || curr[0] == '-')
	{
		char op = *curr;
		curr++;
		if(!ParseMultiplicative(&curr))
			ThrowError("ERROR: expression not found after additive expression", curr);
		CALLBACK((addCmd((CmdID)(op == '+' ? cmdAdd : cmdSub)))(*str, curr));
		SKIP_SPACE(&curr);
	}
	*str = curr;
	return true;
}
//term4_1		=	term4 >> *((strP("<<") >> term4)[addCmd(cmdShl)] | (strP(">>") >> term4)[addCmd(cmdShr)]);
bool ParseBinaryShift(char** str)
{
	if(!ParseAdditive(str))
		return false;
	SKIP_SPACE(str);
	char *curr = *str;
	while((curr[0] == '<' && curr[1] == '<') || (curr[0] == '>' && curr[1] == '>'))
	{
		char op = *curr;
		curr += 2;
		if(!ParseAdditive(&curr))
			ThrowError("ERROR: expression not found after shift expression", curr);
		CALLBACK((addCmd((CmdID)(op == '<' ? cmdShl : cmdShr)))(*str, curr));
		SKIP_SPACE(&curr);
	}
	*str = curr;
	return true;
}
//term4_2		=	term4_1 >> *(('<' >> term4_1)[addCmd(cmdLess)] | ('>' >> term4_1)[addCmd(cmdGreater)] | (strP("<=") >> term4_1)[addCmd(cmdLEqual)] | (strP(">=") >> term4_1)[addCmd(cmdGEqual)]);
bool ParseComparision(char** str)
{
	if(!ParseBinaryShift(str))
		return false;
	SKIP_SPACE(str);
	char *curr = *str;
	while(curr[0] == '<' || curr[0] == '>')
	{
		char op = *curr;
		curr += curr[1] == '=' ? 2 : 1;
		if(!ParseBinaryShift(&curr))
			ThrowError("ERROR: expression not found after comparison expression", curr);
		CALLBACK((addCmd((CmdID)(op == '<' ? (curr[1] == '=' ? cmdLEqual : cmdLess) : (curr[1] == '=' ? cmdGEqual : cmdGreater))))(*str, curr));
		SKIP_SPACE(&curr);
	}
	*str = curr;
	return true;
}
//term4_4		=	term4_2 >> *((strP("==") >> term4_2)[addCmd(cmdEqual)] | (strP("!=") >> term4_2)[addCmd(cmdNEqual)]);
bool  ParseStrongComparision(char** str)
{
	if(!ParseComparision(str))
		return false;
	SKIP_SPACE(str);
	char *curr = *str;
	while((curr[0] == '!' || curr[0] == '=') && curr[1] == '=')
	{
		char op = *curr;
		curr += 2;
		if(!ParseComparision(&curr))
			ThrowError("ERROR: expression not found after comparison expression", curr);
		CALLBACK((addCmd((CmdID)(op == '=' ? cmdEqual : cmdNEqual)))(*str, curr));
		SKIP_SPACE(&curr);
	}
	*str = curr;
	return true;
}
//term4_6		=	term4_4 >> *(strP("&") >> (term4_4 | epsP[ThrowError("ERROR: expression not found after &")]))[addCmd(cmdBitAnd)];
bool ParseBinaryAnd(char** str)
{
	if(!ParseStrongComparision(str))
		return false;
	SKIP_SPACE(str);
	char *curr = *str;
	while(curr[0] == '&')
	{
		curr++;
		if(!ParseStrongComparision(&curr))
			ThrowError("ERROR: expression not found after '&'", curr);
		CALLBACK((addCmd(cmdBitAnd))(*str, curr));
		SKIP_SPACE(&curr);
	}
	*str = curr;
	return true;
}
//term4_65	=	term4_6 >> *(strP("^") >> (term4_6 | epsP[ThrowError("ERROR: expression not found after ^")]))[addCmd(cmdBitXor)];
bool ParseBinaryXor(char** str)
{
	if(!ParseBinaryAnd(str))
		return false;
	SKIP_SPACE(str);
	char *curr = *str;
	while(curr[0] == '^')
	{
		curr++;
		if(!ParseBinaryAnd(&curr))
			ThrowError("ERROR: expression not found after '^'", curr);
		CALLBACK((addCmd(cmdBitXor))(*str, curr));
		SKIP_SPACE(&curr);
	}
	*str = curr;
	return true;
}
//term4_7		=	term4_65 >> *(strP("|") >> (term4_65 | epsP[ThrowError("ERROR: expression not found after |")]))[addCmd(cmdBitOr)];
bool ParseBinaryOr(char** str)
{
	if(!ParseBinaryXor(str))
		return false;
	SKIP_SPACE(str);
	char *curr = *str;
	while(curr[0] == '|')
	{
		curr++;
		if(!ParseBinaryXor(&curr))
			ThrowError("ERROR: expression not found after '|'", curr);
		CALLBACK((addCmd(cmdBitOr))(*str, curr));
		SKIP_SPACE(&curr);
	}
	*str = curr;
	return true;
}
//term4_75	=	term4_7 >> *(strP("and") >> (term4_7 | epsP[ThrowError("ERROR: expression not found after and")]))[addCmd(cmdLogAnd)];
bool ParseLogicalAnd(char** str)
{
	if(!ParseBinaryOr(str))
		return false;
	SKIP_SPACE(str);
	char *curr = *str;
	while(curr[0] == 'a' && curr[1] == 'n' && curr[2] == 'd')
	{
		curr += 3;
		if(!ParseBinaryOr(&curr))
			ThrowError("ERROR: expression not found after 'and'", curr);
		CALLBACK((addCmd(cmdLogAnd))(*str, curr));
		SKIP_SPACE(&curr);
	}
	*str = curr;
	return true;
}
//term4_8		=	term4_75 >> *(strP("xor") >> (term4_75 | epsP[ThrowError("ERROR: expression not found after xor")]))[addCmd(cmdLogXor)];
bool ParseLogicalXor(char** str)
{
	if(!ParseLogicalAnd(str))
		return false;
	SKIP_SPACE(str);
	char *curr = *str;
	while(curr[0] == 'x' && curr[1] == 'o' && curr[2] == 'r')
	{
		curr += 3;
		if(!ParseLogicalAnd(&curr))
			ThrowError("ERROR: expression not found after 'xor'", curr);
		CALLBACK((addCmd(cmdLogXor))(*str, curr));
		SKIP_SPACE(&curr);
	}
	*str = curr;
	return true;
}
//term4_85	=	term4_8 >> *(strP("or") >> (term4_8 | epsP[ThrowError("ERROR: expression not found after or")]))[addCmd(cmdLogOr)];
bool ParseLogicalOr(char** str)
{
	if(!ParseLogicalXor(str))
		return false;
	SKIP_SPACE(str);
	char *curr = *str;
	while(curr[0] == 'o' && curr[1] == 'r')
	{
		curr += 2;
		if(!ParseLogicalXor(&curr))
			ThrowError("ERROR: expression not found after 'or'", curr);
		CALLBACK((addCmd(cmdLogOr))(*str, curr));
		SKIP_SPACE(&curr);
	}
	*str = curr;
	return true;
}
//term4_9		=	term4_85 >> !('?' >> term5 >> ':' >> term5)[addIfElseTermNode];
bool ParseTernaryExpr(char** str)
{
	if(!ParseLogicalOr(str))
		return false;
	SKIP_SPACE(str);
	char *curr = *str;
	while(curr[0] == '?')
	{
		curr++;
		if(!ParseVaribleSet(&curr))
			ThrowError("ERROR: expression not found after '?'", curr);
		SKIP_SPACE(&curr);
		if(*curr != ':')
			ThrowError("ERROR: ':' not found after expression in ternary operator", curr);
		curr++;
		if(!ParseVaribleSet(&curr))
			ThrowError("ERROR: expression not found after ':'", curr);
		CALLBACK(addIfElseTermNode(*str, curr));
		SKIP_SPACE(&curr);
	}
	*str = curr;
	return true;
}
/* term5		=	(!(seltype) >>
						variable >> (
						(strP("=") >> term5)[AddSetVariableNode][popType] |
						(strP("+=") >> (term5 | epsP[ThrowError("ERROR: expression not found after '+='")]))[AddModifyVariable<cmdAdd>()][popType] |
						(strP("-=") >> (term5 | epsP[ThrowError("ERROR: expression not found after '-='")]))[AddModifyVariable<cmdSub>()][popType] |
						(strP("*=") >> (term5 | epsP[ThrowError("ERROR: expression not found after '*='")]))[AddModifyVariable<cmdMul>()][popType] |
						(strP("/=") >> (term5 | epsP[ThrowError("ERROR: expression not found after '/='")]))[AddModifyVariable<cmdDiv>()][popType] |
						(strP("**=") >> (term5 | epsP[ThrowError("ERROR: expression not found after '**='")]))[AddModifyVariable<cmdPow>()][popType] |
						(epsP[FailedSetVariable][popType] >> nothingP))
						) |
						term4_9;*/
bool ParseVaribleSet(char** str)
{
	SKIP_SPACE(str);
	char *curr = *str;
	ParseSelectType(&curr);
	if(!ParseVariable(&curr))
	{
		curr = *str;
	}else{
		SKIP_SPACE(&curr);
		char *pos = curr;
		if(curr[0] == '=')
		{
			curr++;
			if(ParseVaribleSet(&curr))
			{
				CALLBACK(AddSetVariableNode(pos, curr));
				CALLBACK(popType(pos, curr));
				*str = curr;
				return true;
			}else{
				CALLBACK(FailedSetVariable(NULL, NULL));
				CALLBACK(popType(NULL, NULL));
				curr = *str;
			}
		}else if(curr[1] == '=' && (curr[0] == '+' || curr[0] == '-' || curr[0] == '*' || curr[0] == '/')){
			char op = *curr;
			curr += 2;
			if(ParseVaribleSet(&curr))
			{
				CALLBACK(AddModifyVariableNode(pos, curr, (CmdID)(op == '+' ? cmdAdd : (op == '-' ? cmdSub : (op == '*' ? cmdMul : cmdDiv)))));
				CALLBACK(popType(pos, curr));
				*str = curr;
				return true;
			}else{
				ThrowError("ERROR: expression not found after assignment operator", *str);
			}
		}else if(curr[0] == '*' && curr[1] == '*' && curr[2] == '='){
			curr += 3;
			if(ParseVaribleSet(&curr))
			{
				CALLBACK(AddModifyVariableNode(pos, curr, cmdPow));
				CALLBACK(popType(pos, curr));
				*str = curr;
				return true;
			}else{
				ThrowError("ERROR: expression not found after '**='", *str);
			}
		}else{
			CALLBACK(FailedSetVariable(NULL, NULL));
			CALLBACK(popType(NULL, NULL));
			curr = *str;
		}
	}
	if(!ParseTernaryExpr(&curr))
		return false;
	*str = curr;
	return true;
}
// block		=	chP('{')[blockBegin] >> (code | epsP[ThrowError("ERROR: {} block cannot be empty")]) >> chP('}')[blockEnd];
bool  ParseBlock(char** str)
{
	SKIP_SPACE(str);
	char *curr = *str;
	if(*curr != '{')
		return false;
	CALLBACK(blockBegin(curr, curr));
	curr++;
	if(!ParseCode(&curr))
		ThrowError("ERROR: {} block cannot be empty", *str);
	SKIP_SPACE(&curr);
	if(*curr != '}')
		ThrowError("ERROR: closing '}' not found", *str);
	CALLBACK(blockEnd(curr, curr));
	curr++;
	*str = curr;
	return true;
}
// expression	=	*chP(';') >> (classdef | (vardef >> +chP(';')) | block | breakexpr | continueExpr | ifexpr | 
//					forexpr | whileexpr | doexpr | switchexpr | retexpr | (term5 >> (+chP(';')  | epsP[ThrowError("ERROR: ';' not found after expression")]))[addPopNode]);
bool  ParseExpression(char** str)
{
	SKIP_SPACE(str);
	char *curr = *str;
	while(*curr == ';')
	{
		curr++;
		SKIP_SPACE(&curr);
	}
	*str = curr;
	if(ParseClassDefinition(str))
		return true;
	if(ParseVariableDefine(str))
	{
		SKIP_SPACE(str);
		if(**str != ';')
			ThrowError("ERROR: ';' not found after variable definition", *str);
		(*str)++;
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
		SKIP_SPACE(str);
		if(**str != ';')
			ThrowError("ERROR: ';' not found after expression", *str);
		CALLBACK(addPopNode(curr, *str));
		(*str)++;
		return true;
	}
	return false;
}
// code		=	((funcdef | expression) >> (code[addTwoExprNode] | epsP[addOneExprNode]));
bool  ParseCode(char** str)
{
	SKIP_SPACE(str);
	char *curr = *str;
	if(!ParseFunctionDefinition(&curr))
	{
		if(!ParseExpression(&curr))
			return false;
	}
	if(ParseCode(&curr))
		CALLBACK(addTwoExprNode(curr, curr));
	else
		CALLBACK(addOneExprNode(curr, curr));
	*str = curr;
	return true;
}
