#pragma once
#include "ParseClass.h"
#include "ParseCommand.h"
#include "ParseFunc.h"

#include "Compiler.h"

// Информация о коде, используемых функциях, переменных, типов.
namespace CodeInfo
{
	static const unsigned int EXEC_VM = 0;
	static const unsigned int EXEC_X86 = 1;

	extern unsigned int		activeExecutor;

	extern CompilerError	lastError;

	// Информация о функциях
	// Information about functions
	extern std::vector<FunctionInfo*>	funcInfo;

	// Информация о переменных
	// Information about variables
	extern std::vector<VariableInfo*>	varInfo;

	// Информация о типах
	// Information about types
	extern std::vector<TypeInfo*>		typeInfo;

	// Поток комманд
	// Command stream
	extern CommandList*				cmdInfoList;
	extern FastVector<VMCmd>		cmdList;

	// The size of all global variables in bytes
	extern unsigned int				globalSize;

	// Список узлов дерева
	// Отдельные узлы помещаются сюда, и в дальнейшем объеденяются в более комплексные узлы,
	// создавая дерево. После правильной компиляции количество узлов в этом массиве должно равнятся 1
	// Node tree list
	// Individual nodes are placed here, and later on, combined into a more complex nodes, 
	// creating AST. After successful compilation, node count should be equal to 1
	extern std::vector<NodeZeroOP*>	nodeList;

	extern const char* lastKnownStartPos;

	//////////////////////////////////////////////////////////////////////////
	// Функция возвращает тип - указателя на исходный
	TypeInfo* GetReferenceType(TypeInfo* type);

	// Функиця возвращает тип, получаемый при разименовании указателя
	TypeInfo* GetDereferenceType(TypeInfo* type);

	// Функция возвращает тип - массив исходных типов (кол-во элементов в varSize)
	TypeInfo* GetArrayType(TypeInfo* type, unsigned int sizeInArgument = 0);

	// Функция возвращает тип функции
	TypeInfo* GetFunctionType(FunctionInfo* info);
};
