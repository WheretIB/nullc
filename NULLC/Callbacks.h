#pragma once
#include "stdafx.h"
#include "ParseCommand.h"

void CallbackInitialize();

void SetTypeConst(bool isConst);
void SetCurrentAlignment(unsigned int alignment);

// Вызывается в начале блока {}, чтобы сохранить количество определённых переменных, к которому можно
// будет вернутся после окончания блока.
void blockBegin(char const* s, char const* e);
// Вызывается в конце блока {}, чтобы убрать информацию о переменных внутри блока, тем самым обеспечивая
// их выход из области видимости. Также уменьшает вершину стека переменных в байтах.
void blockEnd(char const* s, char const* e);

// Функции для добавления узлов с константными числами разных типов
void addNumberNodeChar(char const*s, char const*e);
void addNumberNodeInt(char const*s, char const*e);
void addNumberNodeFloat(char const*s, char const*e);
void addNumberNodeLong(char const*s, char const*e);
void addNumberNodeDouble(char const*s, char const*e);

void addVoidNode(char const*s, char const*e);

void addHexInt(char const*s, char const*e);
void addOctInt(char const*s, char const*e);
void addBinInt(char const*s, char const*e);

// Функция для создания узла, который кладёт массив в стек
// Используется NodeExpressionList, что не является самым быстрым и красивым вариантом
// но зато не надо писать отдельный класс с одинаковыми действиями внутри.
void addStringNode(char const*s, char const*e);

// Функция для создания узла, который уберёт значение со стека переменных
// Узел заберёт к себе последний узел в списке.
void addPopNode(char const* s, char const* e);

// Функция для создания узла, которые поменяет знак значения в стеке
// Узел заберёт к себе последний узел в списке.
void addNegNode(char const* s, char const* e);

// Функция для создания узла, которые произведёт логическое отрицания над значением в стеке
// Узел заберёт к себе последний узел в списке.
void addLogNotNode(char const* s, char const* e);
void addBitNotNode(char const* s, char const* e);

typedef void (*ParseFuncPtr)(char const* s, char const* e);
ParseFuncPtr addCmd(CmdID cmd);

void addReturnNode(char const* s, char const* e);

void addBreakNode(char const* s, char const* e);

void AddContinueNode(char const* s, char const* e);

//Finds TypeInfo in a typeInfo list by name
void SelectTypeByName(char const* pos, char const* typeName);

void AddVariable(char const* pos, const char* varName);

void AddVariableReserveNode(char const* pos);

void pushType(char const* s, char const* e);
void popType(char const* s, char const* e);

void convertTypeToRef(char const* s, char const* e);

void convertTypeToArray(char const* s, char const* e);

void GetTypeSize(char const* s, char const* e, bool sizeOfExpr);

void SetTypeOfLastNode(char const* s, char const* e);

// Функция для получения адреса переменной, имя которое передаётся в параметрах
void AddGetAddressNode(char const* pos, char const* varName);

// Функция вызывается для индексации массива
void AddArrayIndexNode(char const* s, char const* e);

// Функция вызывается для разыменования указателя
void AddDereferenceNode(char const* s, char const* e);

// Компилятор в начале предполагает, что после переменной будет слодовать знак присваивания
// Часто его нету, поэтому требуется удалить узел
void FailedSetVariable(char const* s, char const* e);

// Функция вызывается для определния переменной с одновременным присваиванием ей значения
void AddDefineVariableNode(char const* pos, const char* varName);

void AddSetVariableNode(char const* s, char const* e);

void AddGetVariableNode(char const* s, char const* e);
void AddMemberAccessNode(char const* pos, char const* varName);

void AddPreOrPostOpNode(bool isInc, bool prefixOp);

void AddModifyVariableNode(char const* s, char const* e, CmdID cmd);

void addOneExprNode(char const* s, char const* e);
void addTwoExprNode(char const* s, char const* e);

void addArrayConstructor(char const* s, char const* e, unsigned int arrElementCount);

void FunctionAdd(char const* pos, char const* funcName);
void FunctionParameter(char const* pos, char const* paramName);
void FunctionStart(char const* pos);
void FunctionEnd(char const* pos, char const* funcName);

void AddFunctionCallNode(char const* pos, char const* funcName, unsigned int callArgCount);
void AddMemberFunctionCall(char const* pos, char const* funcName, unsigned int callArgCount);

void addIfNode(char const* s, char const* e);
void addIfElseNode(char const* s, char const* e);
void addIfElseTermNode(char const* s, char const* e);

void saveVarTop(char const* s, char const* e);
void addForNode(char const* s, char const* e);
void addWhileNode(char const* s, char const* e);
void addDoWhileNode(char const* s, char const* e);

void preSwitchNode(char const* s, char const* e);
void addCaseNode(char const* s, char const* e);
void addSwitchNode(char const* s, char const* e);

void TypeBegin(char const* s, char const* e);
void TypeAddMember(char const* pos, const char* varName);
void TypeFinish(char const* s, char const* e);

void addUnfixedArraySize(char const*s, char const*e);

// Эти функции вызываются, чтобы привязать строку кода к узлу, который его компилирует
void SetStringToLastNode(char const *s, char const *e);
void SaveStringIndex(char const *s, char const *e);
void SetStringFromIndex(char const *s, char const *e);

void CallbackDeinitialize();