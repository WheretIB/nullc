#pragma once
#include "Bytecode.h"
#include "Linker.h"

void CommonSetLinker(Linker* linker);

void ClosureCreate(char* paramBase, unsigned int helper, unsigned int argument, ExternFuncInfo::Upvalue* upvalue);
void CloseUpvalues(char* paramBase, unsigned int argument);

unsigned int PrintStackFrame(int address, char* current, unsigned int bufSize);

namespace NULLCTypeInfo
{
	static const char *typeInfoModule =
	"class typeid{ int id; int memberCount(); typeid memberType(int member); char[] memberName(int member); }\
	char[] typename(auto ref type);\
	typeid typeid(auto ref type);\
	int isFunction(typeid type);\
	int isClass(typeid type);\
	int isSimple(typeid type);\
	int isArray(typeid type);\
	int isPointer(typeid type);\
	int isFunction(auto ref type);\
	int isClass(auto ref type);\
	int isSimple(auto ref type);\
	int isArray(auto ref type);\
	int isPointer(auto ref type);";

	int			MemberCount(int* type);
	int			MemberType(int member, int* type);
	NullCArray	MemberName(int member, int* type);

	NullCArray	Typename(NULLCRef r);
	int			Typeid(NULLCRef r);

	int			IsFunction(int id);
	int			IsClass(int id);
	int			IsSimple(int id);
	int			IsArray(int id);
	int			IsPointer(int id);

	int			IsFunctionRef(NULLCRef r);
	int			IsClassRef(NULLCRef r);
	int			IsSimpleRef(NULLCRef r);
	int			IsArrayRef(NULLCRef r);
	int			IsPointerRef(NULLCRef r);
}

// Garbage collector

void SetUnmanagableRange(char* base, unsigned int size);
void MarkUsedBlocks();
