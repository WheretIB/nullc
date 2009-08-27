#pragma once
#include "stdafx.h"
#include "InstructionSet.h"

void CallbackInitialize();

void SetTypeConst(bool isConst);
void SetCurrentAlignment(unsigned int alignment);

// Вызывается в начале блока {}, чтобы сохранить количество определённых переменных, к которому можно
// будет вернутся после окончания блока.
void BeginBlock();
// Вызывается в конце блока {}, чтобы убрать информацию о переменных внутри блока, тем самым обеспечивая
// их выход из области видимости. Также уменьшает вершину стека переменных в байтах.
void EndBlock(bool hideFunctions = true);

// Функции для добавления узлов с константными числами разных типов
void AddNumberNodeChar(const char* pos);
void AddNumberNodeInt(const char* pos);
void AddNumberNodeFloat(const char* pos);
void AddNumberNodeLong(const char* pos, const char* end);
void AddNumberNodeDouble(const char* pos);

void AddVoidNode();

void AddHexInteger(const char* pos, const char* end);
void AddOctInteger(const char* pos, const char* end);
void AddBinInteger(const char* pos, const char* end);

// Функция для создания узла, который кладёт массив в стек
// Используется NodeExpressionList, что не является самым быстрым и красивым вариантом
// но зато не надо писать отдельный класс с одинаковыми действиями внутри.
void AddStringNode(const char* s, const char* e);

// Функция для создания узла, который уберёт значение со стека переменных
// Узел заберёт к себе последний узел в списке.
void AddPopNode(const char* s, const char* e);

// Функция для создания узла, которые поменяет знак значения в стеке
// Узел заберёт к себе последний узел в списке.
void AddNegateNode(const char* pos);

// Функция для создания узла, которые произведёт логическое отрицания над значением в стеке
// Узел заберёт к себе последний узел в списке.
void AddLogNotNode(const char* pos);
void AddBitNotNode(const char* pos);

void AddBinaryCommandNode(CmdID id);

void AddReturnNode(const char* pos, const char* end);

void AddBreakNode(const char* pos);

void AddContinueNode(const char* pos);

void SelectAutoType();
void SelectTypeByIndex(unsigned int index);

void AddVariable(const char* pos, InplaceStr varName);

void AddVariableReserveNode(const char* pos);

void PushType();
void PopType();

void ConvertTypeToReference(const char* pos);

void ConvertTypeToArray(const char* pos);

void GetTypeSize(const char* pos, bool sizeOfExpr);

void SetTypeOfLastNode();

// Функция для получения адреса переменной, имя которое передаётся в параметрах
void AddGetAddressNode(const char* pos, InplaceStr varName);

// Функция вызывается для индексации массива
void AddArrayIndexNode(const char* pos);

// Функция вызывается для разыменования указателя
void AddDereferenceNode(const char* pos);

// Компилятор в начале предполагает, что после переменной будет слодовать знак присваивания
// Часто его нету, поэтому требуется удалить узел
void FailedSetVariable();

// Функция вызывается для определния переменной с одновременным присваиванием ей значения
void AddDefineVariableNode(const char* pos, InplaceStr varName);

void AddSetVariableNode(const char* pos);

void AddGetVariableNode(const char* pos);
void AddMemberAccessNode(const char* pos, InplaceStr varName);

void AddPreOrPostOpNode(bool isInc, bool prefixOp);

void AddModifyVariableNode(const char* pos, CmdID cmd);

void AddOneExpressionNode();
void AddTwoExpressionNode();

void AddArrayConstructor(const char* pos, unsigned int arrElementCount);

void FunctionAdd(const char* pos, const char* funcName);
void FunctionParameter(const char* pos, InplaceStr paramName);
void FunctionStart(const char* pos);
void FunctionEnd(const char* pos, const char* funcName);

void AddFunctionCallNode(const char* pos, const char* funcName, unsigned int callArgCount);
void AddMemberFunctionCall(const char* pos, const char* funcName, unsigned int callArgCount);

void AddIfNode();
void AddIfElseNode();
void AddIfElseTermNode(const char* pos);

void SaveVariableTop();
void AddForNode();
void AddWhileNode();
void AddDoWhileNode();

void BeginSwitch();
void AddCaseNode();
void EndSwitch();

void TypeBegin(const char* pos, const char* end);
void TypeAddMember(const char* pos, const char* varName);
void TypeFinish();

void AddUnfixedArraySize();

// Эти функции вызываются, чтобы привязать строку кода к узлу, который его компилирует
void SetStringToLastNode(const char* pos, const char* end);
void SaveStringIndex(const char *s, const char *e);
void SetStringFromIndex();

void CallbackDeinitialize();