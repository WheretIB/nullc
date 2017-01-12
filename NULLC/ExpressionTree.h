#pragma once

#include "ParseTree.h"
#include "IntrusiveList.h"
#include "Array.h"

class TypeInfo;
class FunctionInfo;

struct ExprBase;

struct ExpressionContext
{
	ExpressionContext();

	// State info
	FastVector<TypeInfo*> typeInfo;

	// Context info
	FastVector<FunctionInfo*> functionStack;

	// Error info
	const char *errorPos;
	InplaceStr errorMsg;

	// Basic types
	TypeInfo* typeVoid;
	TypeInfo* typeChar;
	TypeInfo* typeShort;
	TypeInfo* typeInt;
	TypeInfo* typeFloat;
	TypeInfo* typeLong;
	TypeInfo* typeDouble;
	TypeInfo* typeObject;
	TypeInfo* typeTypeid;
	TypeInfo* typeAutoArray;
	TypeInfo* typeFunction;
	TypeInfo* typeGeneric;
	TypeInfo* typeBool;
};

struct ExprBase
{
	ExprBase(unsigned typeID, TypeInfo *type): typeID(typeID), type(type), next(0)
	{
	}

	virtual ~ExprBase()
	{
	}

	unsigned typeID;

	TypeInfo *type;
	ExprBase *next;
};

struct ExprNumber: ExprBase
{
	ExprNumber(TypeInfo *type, int integer): ExprBase(myTypeID, type)
	{
		value.integer = integer;
	}

	ExprNumber(TypeInfo *type, long long integer64): ExprBase(myTypeID, type)
	{
		value.integer64 = integer64;
	}

	ExprNumber(TypeInfo *type, double real): ExprBase(myTypeID, type)
	{
		value.real = real;
	}

	union Numbers
	{
		int integer;
		long long integer64;
		double real;
		struct QuadWord
		{
			int low, high;
		} quad;
	} value;

	static const unsigned myTypeID = __LINE__;
};

struct ExprReturn: ExprBase
{
	ExprReturn(TypeInfo *type, ExprBase* value): ExprBase(myTypeID, type), value(value)
	{
	}

	ExprBase* value;

	static const unsigned myTypeID = __LINE__;
};

struct ExprModule: ExprBase
{
	ExprModule(TypeInfo *type, IntrusiveList<ExprBase> expressions): ExprBase(myTypeID, type), expressions(expressions)
	{
	}

	IntrusiveList<ExprBase> expressions;

	static const unsigned myTypeID = __LINE__;
};

template<typename T>
bool isType(ExprBase *node)
{
	return node->typeID == typename T::myTypeID;
}

ExprBase* Analyze(ExpressionContext &context, SynBase *syntax);
