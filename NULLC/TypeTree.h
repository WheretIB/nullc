#pragma once

#include "stdafx.h"
#include "IntrusiveList.h"
#include "StrAlgo.h"

#include "ParseTree.h"

struct ExprBase;

struct TypeBase;
struct TypeStruct;
struct TypeRef;
struct TypeArray;
struct TypeUnsizedArray;
struct TypeFunction;
struct TypeClass;

struct ScopeData;
struct NamespaceData;
struct VariableData;
struct FunctionData;
struct AliasData;

struct VariableHandle
{
	VariableHandle(VariableData *variable): variable(variable), next(0)
	{
	}

	VariableData *variable;

	VariableHandle *next;
};

struct TypeHandle
{
	TypeHandle(TypeBase *type): type(type), next(0)
	{
	}

	TypeBase *type;

	TypeHandle *next;
};

struct AliasHandle
{
	AliasHandle(AliasData *alias): alias(alias), next(0)
	{
	}

	AliasData *alias;

	AliasHandle *next;
};

struct NamespaceData
{
	NamespaceData(ScopeData *scope, NamespaceData *parent, InplaceStr name, unsigned uniqueId): scope(scope), parent(parent), name(name), uniqueId(uniqueId)
	{
		nameHash = GetStringHash(name.begin, name.end);

		if(parent)
			fullNameHash = StringHashContinue(StringHashContinue(parent->fullNameHash, "."), name.begin, name.end);
		else
			fullNameHash = nameHash;
	}

	ScopeData *scope;

	NamespaceData *parent;

	InplaceStr name;
	unsigned nameHash;

	unsigned fullNameHash;

	unsigned uniqueId;
};

struct VariableData
{
	VariableData(ScopeData *scope, unsigned alignment, TypeBase *type, InplaceStr name, unsigned offset, unsigned uniqueId): scope(scope), alignment(alignment), type(type), name(name), offset(offset), uniqueId(uniqueId)
	{
		nameHash = GetStringHash(name.begin, name.end);

		if(alignment != 0)
			assert(offset % alignment == 0);
	}

	ScopeData *scope;

	unsigned alignment;

	TypeBase *type;

	InplaceStr name;
	unsigned nameHash;

	unsigned offset;

	unsigned uniqueId;
};

struct FunctionData
{
	FunctionData(ScopeData *scope, bool coroutine, TypeFunction *type, InplaceStr name, unsigned uniqueId, SynFunctionDefinition *definition): scope(scope), coroutine(coroutine), type(type), name(name), uniqueId(uniqueId), definition(definition)
	{
		nameHash = GetStringHash(name.begin, name.end);

		stackSize = 0;

		hasExplicitReturn = false;
	}

	ScopeData *scope;

	bool coroutine;

	TypeFunction *type;

	InplaceStr name;
	unsigned nameHash;

	unsigned uniqueId;

	SynFunctionDefinition *definition;

	long long stackSize;

	bool hasExplicitReturn;
};

struct AliasData
{
	AliasData(ScopeData *scope, TypeBase *type, InplaceStr name, unsigned uniqueId): scope(scope), type(type), name(name), uniqueId(uniqueId)
	{
		nameHash = GetStringHash(name.begin, name.end);
	}

	ScopeData *scope;

	TypeBase *type;

	InplaceStr name;
	unsigned nameHash;

	unsigned uniqueId;
};

struct ScopeData
{
	ScopeData(unsigned depth, ScopeData *scope): depth(depth), scope(scope), globalSize(0), ownerNamespace(0), ownerFunction(0), ownerType(0)
	{
	}

	ScopeData(unsigned depth, ScopeData *scope, NamespaceData *ownerNamespace): depth(depth), scope(scope), globalSize(0), ownerNamespace(ownerNamespace), ownerFunction(0), ownerType(0)
	{
	}

	ScopeData(unsigned depth, ScopeData *scope, FunctionData *ownerFunction): depth(depth), scope(scope), globalSize(0), ownerNamespace(0), ownerFunction(ownerFunction), ownerType(0)
	{
	}

	ScopeData(unsigned depth, ScopeData *scope, TypeBase *ownerType): depth(depth), scope(scope), globalSize(0), ownerNamespace(0), ownerFunction(0), ownerType(ownerType)
	{
	}

	unsigned depth;

	ScopeData *scope;

	long long globalSize;

	NamespaceData *ownerNamespace;
	FunctionData *ownerFunction;
	TypeBase *ownerType;

	FastVector<TypeBase*> types;
	FastVector<FunctionData*> functions;
	FastVector<VariableData*> variables;
	FastVector<AliasData*> aliases;
};

template<typename T>
unsigned GetTypeAlignment()
{
	struct Helper
	{
		char x;
		T y;
	};

	return sizeof(Helper) - sizeof(T);
}

struct TypeBase
{
	TypeBase(unsigned typeID, InplaceStr name): typeID(typeID), name(name)
	{
		nameHash = GetStringHash(name.begin, name.end);

		size = 0;
		alignment = 0;
		padding = 0;

		isGeneric = false;

		refType = 0;
		unsizedArrayType = 0;
	}

	virtual ~TypeBase()
	{
	}

	unsigned typeID;

	InplaceStr name;
	unsigned nameHash;
	
	long long size;
	unsigned alignment;
	unsigned padding;

	bool isGeneric;

	TypeRef *refType; // Reference type to this type
	FastVector<TypeArray*> arrayTypes; // Array types derived from this type
	TypeUnsizedArray *unsizedArrayType; // An unsized array type derived from this type
};

struct TypeVoid: TypeBase
{
	TypeVoid(InplaceStr name): TypeBase(myTypeID, name)
	{
		size = 0;
		alignment = 0;
	}

	static const unsigned myTypeID = __LINE__;
};

struct TypeBool: TypeBase
{
	TypeBool(InplaceStr name): TypeBase(myTypeID, name)
	{
		size = 1;
	}

	static const unsigned myTypeID = __LINE__;
};

struct TypeChar: TypeBase
{
	TypeChar(InplaceStr name): TypeBase(myTypeID, name)
	{
		size = 1;
	}

	static const unsigned myTypeID = __LINE__;
};

struct TypeShort: TypeBase
{
	TypeShort(InplaceStr name): TypeBase(myTypeID, name)
	{
		size = 2;
		alignment = GetTypeAlignment<short>();
	}

	static const unsigned myTypeID = __LINE__;
};

struct TypeInt: TypeBase
{
	TypeInt(InplaceStr name): TypeBase(myTypeID, name)
	{
		size = 4;
		alignment = GetTypeAlignment<int>();
	}

	static const unsigned myTypeID = __LINE__;
};

struct TypeLong: TypeBase
{
	TypeLong(InplaceStr name): TypeBase(myTypeID, name)
	{
		size = 8;
		alignment = GetTypeAlignment<long>();
	}

	static const unsigned myTypeID = __LINE__;
};

struct TypeFloat: TypeBase
{
	TypeFloat(InplaceStr name): TypeBase(myTypeID, name)
	{
		size = 4;
		alignment = GetTypeAlignment<float>();
	}

	static const unsigned myTypeID = __LINE__;
};

struct TypeDouble: TypeBase
{
	TypeDouble(InplaceStr name): TypeBase(myTypeID, name)
	{
		size = 8;
		alignment = GetTypeAlignment<double>();
	}

	static const unsigned myTypeID = __LINE__;
};

struct TypeTypeID: TypeBase
{
	TypeTypeID(InplaceStr name): TypeBase(myTypeID, name)
	{
		size = 4;
		alignment = GetTypeAlignment<int>();
	}

	static const unsigned myTypeID = __LINE__;
};

struct TypeFunctionID: TypeBase
{
	TypeFunctionID(InplaceStr name): TypeBase(myTypeID, name)
	{
		size = 4;
		alignment = GetTypeAlignment<int>();
	}

	static const unsigned myTypeID = __LINE__;
};

struct TypeGeneric: TypeBase
{
	TypeGeneric(InplaceStr name): TypeBase(myTypeID, name)
	{
		isGeneric = true;
	}

	static const unsigned myTypeID = __LINE__;
};

struct TypeAuto: TypeBase
{
	TypeAuto(InplaceStr name): TypeBase(myTypeID, name)
	{
	}

	static const unsigned myTypeID = __LINE__;
};

struct TypeStruct: TypeBase
{
	TypeStruct(unsigned myTypeID, InplaceStr name): TypeBase(myTypeID, name)
	{
	}

	IntrusiveList<VariableHandle> members;
};

struct TypeAutoRef: TypeStruct
{
	TypeAutoRef(InplaceStr name): TypeStruct(myTypeID, name)
	{
	}

	static const unsigned myTypeID = __LINE__;
};

struct TypeAutoArray: TypeStruct
{
	TypeAutoArray(InplaceStr name): TypeStruct(myTypeID, name)
	{
	}

	static const unsigned myTypeID = __LINE__;
};

struct TypeRef: TypeBase
{
	TypeRef(InplaceStr name, TypeBase *subType): TypeBase(myTypeID, name), subType(subType)
	{
		size = NULLC_PTR_SIZE;
		alignment = 4;

		isGeneric = subType->isGeneric;
	}

	TypeBase *subType;

	static const unsigned myTypeID = __LINE__;
};

struct TypeArray: TypeBase
{
	TypeArray(InplaceStr name, TypeBase *subType, long long length): TypeBase(myTypeID, name), subType(subType), length(length)
	{
		size = subType->size * length;

		isGeneric = subType->isGeneric;
	}

	TypeBase *subType;
	long long length;

	static const unsigned myTypeID = __LINE__;
};

struct TypeUnsizedArray: TypeStruct
{
	TypeUnsizedArray(InplaceStr name, TypeBase *subType): TypeStruct(myTypeID, name), subType(subType)
	{
		isGeneric = subType->isGeneric;
	}

	TypeBase *subType;

	static const unsigned myTypeID = __LINE__;
};

struct TypeFunction: TypeBase
{
	TypeFunction(InplaceStr name, TypeBase *returnType, IntrusiveList<TypeHandle> arguments): TypeBase(myTypeID, name), returnType(returnType), arguments(arguments)
	{
		size = 4 + NULLC_PTR_SIZE;

		isGeneric = returnType->isGeneric;

		for(TypeHandle *el = arguments.head; el; el = el->next)
			isGeneric |= el->type->isGeneric;
	}

	TypeBase *returnType;
	IntrusiveList<TypeHandle> arguments;

	static const unsigned myTypeID = __LINE__;
};

struct TypeClass: TypeStruct
{
	TypeClass(ScopeData *scope, InplaceStr name, IntrusiveList<SynIdentifier> genericNames, IntrusiveList<TypeHandle> genericTypes, bool extenable, TypeBase *baseClass): TypeStruct(myTypeID, name), scope(scope), genericNames(genericNames), genericTypes(genericTypes), extenable(extenable), baseClass(baseClass)
	{
	}

	ScopeData *scope;

	IntrusiveList<SynIdentifier> genericNames;
	IntrusiveList<TypeHandle> genericTypes;

	bool extenable;

	TypeBase *baseClass;

	static const unsigned myTypeID = __LINE__;
};

struct TypeGenericClassProto: TypeBase
{
	TypeGenericClassProto(InplaceStr name, SynClassDefinition *definition): TypeBase(myTypeID, name), definition(definition)
	{
		isGeneric = true;
	}

	SynClassDefinition *definition;

	IntrusiveList<ExprBase> instances;

	static const unsigned myTypeID = __LINE__;
};

struct TypeGenericClass: TypeBase
{
	TypeGenericClass(InplaceStr name, TypeGenericClassProto *proto, IntrusiveList<TypeHandle> generics): TypeBase(myTypeID, name), proto(proto), generics(generics)
	{
		isGeneric = true;
	}

	TypeGenericClassProto *proto;

	IntrusiveList<TypeHandle> generics;

	static const unsigned myTypeID = __LINE__;
};

template<typename T>
bool isType(TypeBase *node)
{
	return node->typeID == typename T::myTypeID;
}

template<typename T>
T* getType(TypeBase *node)
{
	if(node && isType<T>(node))
		return static_cast<T*>(node);

	return 0;
}

template<>
inline TypeStruct* getType(TypeBase *node)
{
	if(node && (isType<TypeAutoRef>(node) || isType<TypeAutoArray>(node) || isType<TypeUnsizedArray>(node) || isType<TypeClass>(node)))
		return static_cast<TypeStruct*>(node);

	return 0;
}

InplaceStr GetReferenceTypeName(TypeBase* type);
InplaceStr GetArrayTypeName(TypeBase* type, long long length);
InplaceStr GetUnsizedArrayTypeName(TypeBase* type);
InplaceStr GetFunctionTypeName(TypeBase* returnType, IntrusiveList<TypeHandle> arguments);
InplaceStr GetGenericClassName(TypeBase* proto, IntrusiveList<TypeHandle> generics);

InplaceStr GetTypeNameInScope(ScopeData *scope, InplaceStr str);
InplaceStr GetVariableNameInScope(ScopeData *scope, InplaceStr str);
InplaceStr GetFunctionNameInScope(ScopeData *scope, InplaceStr str);
