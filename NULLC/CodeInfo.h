#pragma once
#include "ParseClass.h"
#include "InstructionSet.h"
#include "SyntaxTree.h"

#include "Compiler.h"

void ThrowError(const char* pos, const char* err, ...);

// Some basic predefined types
extern TypeInfo*	typeVoid;
extern TypeInfo*	typeChar;
extern TypeInfo*	typeShort;
extern TypeInfo*	typeInt;
extern TypeInfo*	typeFloat;
extern TypeInfo*	typeLong;
extern TypeInfo*	typeDouble;
extern TypeInfo*	typeObject;

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
	extern FastVector<AliasInfo>	aliasInfo;
	extern unsigned int				classCount;

	// Поток комманд
	// Command stream
	extern SourceInfo				cmdInfoList;
	extern FastVector<VMCmd>		cmdList;

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
		// Search through active types
		for(unsigned int i = 0; i < typeInfo.size(); i++)
		{
			if(typeInfo[i]->funcType)
			{
				if(typeInfo[i]->funcType->retType != retType)
					continue;
				if(typeInfo[i]->funcType->paramCount != paramCount)
					continue;
				bool good = true;
				unsigned int n = 0;
				TypeInfo	**paramType = typeInfo[i]->funcType->paramType;
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
					bestFit = typeInfo[i];
					break;
				}
			}
		}
		// If none found, create new
		if(!bestFit)
		{
			typeInfo.push_back(new TypeInfo(typeInfo.size(), NULL, 0, 0, 1, NULL, TypeInfo::TYPE_COMPLEX));
			bestFit = typeInfo.back();
			bestFit->CreateFunctionType(retType, paramCount);

			unsigned int i = 0;
			for(T *curr = paramTypes; curr; curr = curr->next, i++)
			{
				bestFit->funcType->paramType[i] = curr->varType;
				bestFit->funcType->paramSize += curr->varType->size > 4 ? curr->varType->size : (curr->varType->size ? 4 : 0);
			}

	#ifdef _DEBUG
			bestFit->AddMemberVariable("context", typeInt);
			bestFit->AddMemberVariable("ptr", typeInt);
	#endif
			bestFit->size = 8;
		}
		return bestFit;
	}

	// Search for the variable starting from the end of a list. -1 is returned if the variable not found
	int	FindVariableByName(unsigned int hash);

	// Search for the function from specified position to the beginning of a list. -1 is returned if the function not found
	int FindFunctionByName(unsigned int hash, int startPos);

	// Search for the function index by having pointer to it
	unsigned int FindFunctionByPtr(FunctionInfo* funcInfo);
};
