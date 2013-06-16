#pragma once
#include "stdafx.h"
#include "Lexer.h"

class TypeInfo;

char*	AllocateString(unsigned int size);
void	ClearStringList();

char*	GetDefaultConstructorName(const char* name);

unsigned int ParseTypename(Lexeme** str);

bool ParseNumber(Lexeme** str);
bool ParseArrayDefinition(Lexeme** str);

enum TypeParseFlag
{
	ALLOW_ARRAY = 1 << 0, // allows parsing of array types. It is disabled when parsing a type for "new" expression
	ALLOW_GENERIC_TYPE = 1 << 1, // used in parsing of generic function declaration, it allows parts of types to be "generic"
	ALLOW_GENERIC_BASE = 1 << 2, // used for parsing of external generic type member function definitions so that a function can be applied to all generic type instances
	ALLOW_EXTENDED_TYPEOF = 1 << 3, // used to allow extended typeof expressions immediately after class name
	ALLOW_NUMERIC_RESULT = 1 << 4, // used to allow extended typeof expressions that result in a numeric result instead of type
	ALLOW_AUTO_RETURN_TYPE = 1 << 5, // used to allow extended typeof expressions that result in a numeric result instead of type

	DISALLOW_EXPLICIT = 1 << 6, // explicit keyword should not be skipped
};
bool ParseSelectType(Lexeme** str, unsigned flag = ALLOW_ARRAY | ALLOW_EXTENDED_TYPEOF, TypeInfo* instanceType = NULL, bool* instanceFailure = NULL);
void ParseTypePostExpressions(Lexeme** str, bool allowArray, bool notType, bool allowAutoReturnType = false, bool allowGenericType = false, TypeInfo* instanceType = NULL, bool* instanceFailure = NULL);

bool ParseClassBodyElement(Lexeme** str);
bool ParseClassStaticIf(Lexeme** str);
void ParseClassBody(Lexeme** str);
bool ParseClassDefinition(Lexeme** str);

unsigned int ParseFunctionArguments(Lexeme** str);

bool ParseFunctionVariables(Lexeme** str, unsigned nodeOffset = 0);
bool ParseFunctionDefinition(Lexeme** str, bool coroutine = false);

bool ParseAlignment(Lexeme** str);

bool ParseAddVariable(Lexeme** str);
bool ParseVariableDefineSub(Lexeme** str);
bool ParseVariableDefine(Lexeme** str);

bool ParseIfExpr(Lexeme** str, bool isStatic = false, bool (*ExpressionParser)(Lexeme**) = NULL);
bool ParseForExpr(Lexeme** str);
bool ParseWhileExpr(Lexeme** str);
bool ParseDoWhileExpr(Lexeme** str);
bool ParseSwitchExpr(Lexeme** str);

bool ParseTypedefExpr(Lexeme** str);

bool ParseReturnExpr(Lexeme** str, bool yield = false);
bool ParseBreakExpr(Lexeme** str);
bool ParseContinueExpr(Lexeme** str);

bool ParseGroup(Lexeme** str);

bool ParseVariable(Lexeme** str, bool *lastIsFunctionCall);
bool ParsePostExpression(Lexeme** str, bool *isFunctionCall);

void ParsePostExpressions(Lexeme** str);
bool ParseTerminal(Lexeme** str);
bool ParsePower(Lexeme** str);
bool ParseMultiplicative(Lexeme** str);
bool ParseAdditive(Lexeme** str);
bool ParseBinaryShift(Lexeme** str);
bool ParseComparision(Lexeme** str);
bool ParseStrongComparision(Lexeme** str);
bool ParseBinaryAnd(Lexeme** str);
bool ParseBinaryXor(Lexeme** str);
bool ParseBinaryOr(Lexeme** str);
bool ParseLogicalAnd(Lexeme** str);
bool ParseLogicalXor(Lexeme** str);
bool ParseLogicalOr(Lexeme** str);
bool ParseTernaryExpr(Lexeme** str);
bool ParseVaribleSet(Lexeme** str);

bool ParseBlock(Lexeme** str);
bool ParseExpressionStaticIf(Lexeme** str);
bool ParseExpression(Lexeme** str);
bool ParseCode(Lexeme** str);

void ParseReset();
