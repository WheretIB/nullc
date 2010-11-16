#pragma once
#include "ParseClass.h"
#include "InstructionSet.h"
#include "SyntaxTree.h"

#include "Compiler.h"

// Some basic predefined types
extern TypeInfo*	typeVoid;
extern TypeInfo*	typeChar;
extern TypeInfo*	typeShort;
extern TypeInfo*	typeInt;
extern TypeInfo*	typeFloat;
extern TypeInfo*	typeLong;
extern TypeInfo*	typeDouble;
extern TypeInfo*	typeObject;
extern TypeInfo*	typeTypeid;
extern TypeInfo*	typeAutoArray;

extern TypeInfo*	typeGeneric;
extern TypeInfo*	typeBool;

// Information about code, function, variables, types, node, etc
namespace CodeInfo
{
	static const unsigned int EXEC_VM = 0;
	static const unsigned int EXEC_X86 = 1;

	extern CompilerError	lastError;

	// Информация о функциях
	// Information about functions
	extern FastVector<FunctionInfo*>	funcInfo;

	// Информация о переменных
	// Information about variables
	extern FastVector<VariableInfo*>	varInfo;

	// Информация о типах
	// Information about types
	extern FastVector<TypeInfo*>	typeInfo;

	// A hash map with all class types
	extern HashMap<TypeInfo*>		classMap;
	extern FastVector<TypeInfo*>	typeArrays;
	extern FastVector<TypeInfo*>	typeFunctions;
	extern AliasInfo				*globalAliases;

	// Information about namespaces
	extern FastVector<NamespaceInfo*>	namespaceInfo;

	// Поток комманд
	// Command stream
	extern SourceInfo				cmdInfoList;
	extern FastVector<VMCmd>		cmdList;

	extern Lexeme					*lexStart, *lexFullStart;

	// Список узлов, которые определяют код функции
	extern FastVector<NodeZeroOP*>	funcDefList;

	// Список узлов дерева
	// Отдельные узлы помещаются сюда, и в дальнейшем объеденяются в более комплексные узлы,
	// создавая дерево. После правильной компиляции количество узлов в этом массиве должно равнятся 1
	// Node tree list
	// Individual nodes are placed here, and later on, combined into a more complex nodes, 
	// creating AST. After successful compilation, node count should be equal to 1
	extern FastVector<NodeZeroOP*>	nodeList;

	extern const char* lastKnownStartPos;

	extern jmp_buf	errorHandler;

	//////////////////////////////////////////////////////////////////////////
	// Function returns reference type for the type
	TypeInfo* GetReferenceType(TypeInfo* type);

	// Function returns type that reference points to
	TypeInfo* GetDereferenceType(TypeInfo* type);

	// Function returns type that is an array for passed type
	TypeInfo* GetArrayType(TypeInfo* type, unsigned int sizeInArgument = 0);

	// Function returns function type
	template<typename T>
	TypeInfo* GetFunctionType(TypeInfo* retType, T* paramTypes, unsigned int paramCount)
	{
		// Find out the function type
		TypeInfo	*bestFit = NULL;
		// Search through function types
		for(unsigned int i = 0; i < typeFunctions.size(); i++)
		{
			TypeInfo *type = typeFunctions[i];
			if(type->funcType->retType != retType)
				continue;
			if(type->funcType->paramCount != paramCount)
				continue;
			bool good = true;
			unsigned int n = 0;
			TypeInfo	**paramType = type->funcType->paramType;
			for(T *curr = paramTypes; curr; curr = curr->next, n++)
			{
				if(curr->varType != paramType[n])
				{
					good = false;
					break;
				}
			}
			if(good)
			{
				bestFit = type;
				break;
			}
		}
		// If none found, create new
		if(!bestFit)
		{
			typeInfo.push_back(new TypeInfo(typeInfo.size(), NULL, 0, 0, 1, NULL, TypeInfo::TYPE_COMPLEX));
			bestFit = typeInfo.back();
			bestFit->CreateFunctionType(retType, paramCount);

			bestFit->dependsOnGeneric = retType ? retType->dependsOnGeneric : true;

			unsigned int i = 0;
			for(T *curr = paramTypes; curr; curr = curr->next, i++)
			{
				bestFit->funcType->paramType[i] = curr->varType;
				if(curr->varType->dependsOnGeneric)
					bestFit->dependsOnGeneric = true;
				bestFit->funcType->paramSize += curr->varType->size > 4 ? curr->varType->size : 4;
			}

	#ifdef _DEBUG
			bestFit->AddMemberVariable("context", typeInt);
			bestFit->AddMemberVariable("ptr", typeInt);
	#endif
			bestFit->size = 4 + NULLC_PTR_SIZE;
			bestFit->hasPointers = true;

			// if return type is auto, then it's a generic function type and it shouldn't be saved
			if(retType)
				typeFunctions.push_back(bestFit);
			else
				typeInfo.pop_back();
		}
		
		return bestFit;
	}

	// Search for the function from specified position to the beginning of a list. -1 is returned if the function not found
	int FindFunctionByName(unsigned int hash, int startPos);

	// Search for the function index by having pointer to it
	unsigned int FindFunctionByPtr(FunctionInfo* funcInfo);
};
