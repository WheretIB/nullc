#pragma once

struct Allocator;

struct SynBase;

struct VmValue;
struct VmConstant;
struct VmInstruction;
struct VmBlock;
struct VmFunction;
struct VmModule;

struct VariableData;
struct ScopeData;
struct TypeBase;

bool IsGlobalScope(ScopeData *scope);
bool IsMemberScope(ScopeData *scope);
bool IsLocalScope(ScopeData *scope);

VmConstant* CreateConstantVoid(Allocator *allocator);
VmConstant* CreateConstantInt(Allocator *allocator, SynBase *source, int value);
VmConstant* CreateConstantDouble(Allocator *allocator, SynBase *source, double value);
VmConstant* CreateConstantLong(Allocator *allocator, SynBase *source, long long value);
VmConstant* CreateConstantPointer(Allocator *allocator, SynBase *source, int offset, VariableData *container, TypeBase *structType, bool trackUsers);
VmConstant* CreateConstantStruct(Allocator *allocator, SynBase *source, char *value, int size, TypeBase *structType);

bool DoesConstantIntegerMatch(VmValue* value, long long number);
bool DoesConstantMatchEither(VmValue* value, int iValue, double dValue, long long lValue);

bool IsConstantZero(VmValue* value);
bool IsConstantOne(VmValue* value);

unsigned GetAccessSize(VmInstruction *inst);

bool HasAddressTaken(VariableData *container);

const char* GetInstructionName(VmInstruction *inst);
