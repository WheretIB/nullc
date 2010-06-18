#pragma once
#include "stdafx.h"
#include "Lexer.h"

char*	AllocateString(unsigned int size);
void	ClearStringList();

unsigned int ParseTypename(Lexeme** str);

bool ParseNumber(Lexeme** str);
bool ParseArrayDefinition(Lexeme** str);
bool ParseSelectType(Lexeme** str, bool arrayType);

bool ParseClassDefinition(Lexeme** str);

unsigned int ParseFunctionArguments(Lexeme** str);
bool ParseFunctionCall(Lexeme** str);

bool ParseFunctionVariables(Lexeme** str);
bool ParseFunctionDefinition(Lexeme** str, bool coroutine = false);

bool ParseAlignment(Lexeme** str);

bool ParseAddVariable(Lexeme** str);
bool ParseVariableDefineSub(Lexeme** str);
bool ParseVariableDefine(Lexeme** str);

bool ParseIfExpr(Lexeme** str);
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
bool ParseExpression(Lexeme** str);
bool ParseCode(Lexeme** str);

void ParseReset();
