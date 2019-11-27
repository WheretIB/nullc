#pragma once

struct Allocator;

struct SynBase;

struct VmType;
struct VmValue;
struct VmConstant;
struct VmInstruction;
struct VmBlock;
struct VmFunction;
struct VmModule;

struct VariableData;
struct ScopeData;
struct TypeBase;

struct ExpressionContext;

bool IsGlobalScope(ScopeData *scope);
bool IsMemberScope(ScopeData *scope);
bool IsLocalScope(ScopeData *scope);

VmConstant* CreateConstantVoid(Allocator *allocator);
VmConstant* CreateConstantInt(Allocator *allocator, SynBase *source, int value);
VmConstant* CreateConstantDouble(Allocator *allocator, SynBase *source, double value);
VmConstant* CreateConstantLong(Allocator *allocator, SynBase *source, long long value);
VmConstant* CreateConstantPointer(Allocator *allocator, SynBase *source, int offset, VariableData *container, TypeBase *structType, bool trackUsers);
VmConstant* CreateConstantStruct(Allocator *allocator, SynBase *source, char *value, int size, TypeBase *structType);
VmConstant* CreateConstantBlock(Allocator *allocator, SynBase *source, VmBlock *block);
VmConstant* CreateConstantFunction(Allocator *allocator, SynBase *source, VmFunction *function);
VmConstant* CreateConstantZero(Allocator *allocator, SynBase *source, VmType type);

bool DoesConstantIntegerMatch(VmValue* value, long long number);
bool DoesConstantMatchEither(VmValue* value, int iValue, double dValue, long long lValue);

bool IsConstantZero(VmValue* value);
bool IsConstantOne(VmValue* value);

unsigned GetAccessSize(VmInstruction *inst);

bool HasAddressTaken(VariableData *container);

const char* GetInstructionName(VmInstruction *inst);

VariableData* FindGlobalAt(ExpressionContext &exprCtx, unsigned offset);

TypeBase* GetBaseType(ExpressionContext &ctx, VmType type);
