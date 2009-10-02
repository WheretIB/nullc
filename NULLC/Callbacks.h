#pragma once
#include "stdafx.h"
#include "InstructionSet.h"

void CallbackInitialize();

void SetTypeConst(bool isConst);
void SetCurrentAlignment(unsigned int alignment);

// ���������� � ������ ����� {}, ����� ��������� ���������� ����������� ����������, � �������� �����
// ����� �������� ����� ��������� �����.
void BeginBlock();
// ���������� � ����� ����� {}, ����� ������ ���������� � ���������� ������ �����, ��� ����� �����������
// �� ����� �� ������� ���������. ����� ��������� ������� ����� ���������� � ������.
void EndBlock(bool hideFunctions = true);

// ������� ��� ���������� ����� � ������������ ������� ������ �����
void AddNumberNodeChar(const char* pos);
void AddNumberNodeInt(const char* pos);
void AddNumberNodeFloat(const char* pos);
void AddNumberNodeLong(const char* pos, const char* end);
void AddNumberNodeDouble(const char* pos);

void AddVoidNode();

void AddHexInteger(const char* pos, const char* end);
void AddOctInteger(const char* pos, const char* end);
void AddBinInteger(const char* pos, const char* end);

// ������� ��� �������� ����, ������� ����� ������ � ����
// ������������ NodeExpressionList, ��� �� �������� ����� ������� � �������� ���������
// �� ���� �� ���� ������ ��������� ����� � ����������� ���������� ������.
void AddStringNode(const char* s, const char* e);

// ������� ��� �������� ����, ������� ����� �������� �� ����� ����������
// ���� ������ � ���� ��������� ���� � ������.
void AddPopNode(const char* pos);

// ������� ��� �������� ����, ������� �������� ���� �������� � �����
// ���� ������ � ���� ��������� ���� � ������.
void AddNegateNode(const char* pos);

// ������� ��� �������� ����, ������� ��������� ���������� ��������� ��� ��������� � �����
// ���� ������ � ���� ��������� ���� � ������.
void AddLogNotNode(const char* pos);
void AddBitNotNode(const char* pos);

void AddBinaryCommandNode(CmdID id);

void AddReturnNode(const char* pos);
void AddBreakNode(const char* pos);
void AddContinueNode(const char* pos);

void SelectAutoType();
void SelectTypeByIndex(unsigned int index);

void AddVariable(const char* pos, InplaceStr varName);

void AddVariableReserveNode(const char* pos);

void ConvertTypeToReference(const char* pos);

void ConvertTypeToArray(const char* pos);

void GetTypeSize(const char* pos, bool sizeOfExpr);

void SetTypeOfLastNode();

// ������� ��� ��������� ������ ����������, ��� ������� ��������� � ����������
void AddGetAddressNode(const char* pos, InplaceStr varName);

// ������� ���������� ��� ���������� �������
void AddArrayIndexNode(const char* pos);

// ������� ���������� ��� ������������� ���������
void AddDereferenceNode(const char* pos);

// ���������� � ������ ������������, ��� ����� ���������� ����� ��������� ���� ������������
// ����� ��� ����, ������� ��������� ������� ����
void FailedSetVariable();

// ������� ���������� ��� ���������� ���������� � ������������� ������������� �� ��������
void AddDefineVariableNode(const char* pos, InplaceStr varName);

void AddSetVariableNode(const char* pos);

void AddGetVariableNode(const char* pos);
void AddMemberAccessNode(const char* pos, InplaceStr varName);

void AddPreOrPostOpNode(const char* pos, bool isInc, bool prefixOp);

void AddModifyVariableNode(const char* pos, CmdID cmd);

void AddOneExpressionNode();
void AddTwoExpressionNode();

void AddArrayConstructor(const char* pos, unsigned int arrElementCount);

void AddTypeAllocation(const char* pos);

void FunctionAdd(const char* pos, const char* funcName);
void FunctionParameter(const char* pos, InplaceStr paramName);
void FunctionStart(const char* pos);
void FunctionEnd(const char* pos, const char* funcName);
void FunctionToOperator(const char* pos);

bool AddFunctionCallNode(const char* pos, const char* funcName, unsigned int callArgCount, bool silent = false);
void AddMemberFunctionCall(const char* pos, const char* funcName, unsigned int callArgCount);

void AddIfNode(const char* pos);
void AddIfElseNode(const char* pos);
void AddIfElseTermNode(const char* pos);

void IncreaseCycleDepth();

void AddForNode(const char* pos);
void AddWhileNode(const char* pos);
void AddDoWhileNode(const char* pos);

void BeginSwitch(const char* pos);
void AddCaseNode(const char* pos);
void EndSwitch();

void TypeBegin(const char* pos, const char* end);
void TypeAddMember(const char* pos, const char* varName);
void TypeFinish();

void AddUnfixedArraySize();

void CallbackDeinitialize();