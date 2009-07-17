#pragma once
#include "stdafx.h"

void ParseSpace(char** str);

bool ParseTypename(char** str);

bool ParseNumber(char** str);
bool ParseArrayDefinition(char** str);
bool ParseSelectType(char** str);
bool ParseIsConst(char** str);
bool ParseVariableName(char** str);

bool ParseClassDefinition(char** str);

bool ParseFunctionCall(char** str);

bool ParseFunctionVariables(char** str);
bool ParseFunctionDefinition(char** str);
bool ParseFunctionPrototype(char** str);

bool ParseAlignment(char** str);

bool ParseAddVariable(char** str);
bool ParseVariableDefineSub(char** str);
bool ParseVariableDefine(char** str);

bool ParseIfExpr(char** str);
bool ParseForExpr(char** str);
bool ParseWhileExpr(char** str);
bool ParseDoWhileExpr(char** str);
bool ParseSwitchExpr(char** str);

bool ParseReturnExpr(char** str);
bool ParseBreakExpr(char** str);
bool ParseContinueExpr(char** str);

bool ParseGroup(char** str);

bool ParseVariable(char** str);
bool ParsePostExpression(char** str);

bool ParseTerminal(char** str);
bool ParsePower(char** str);
bool ParseMultiplicative(char** str);
bool ParseAdditive(char** str);
bool ParseBinaryShift(char** str);
bool ParseComparision(char** str);
bool ParseStrongComparision(char** str);
bool ParseBinaryAnd(char** str);
bool ParseBinaryXor(char** str);
bool ParseBinaryOr(char** str);
bool ParseLogicalAnd(char** str);
bool ParseLogicalXor(char** str);
bool ParseLogicalOr(char** str);
bool ParseTernaryExpr(char** str);
bool ParseVaribleSet(char** str);

bool ParseBlock(char** str);
bool ParseExpression(char** str);
bool ParseCode(char** str);
