#pragma once
#include "ParseClass.h"
#include "ParseCommand.h"
#include "ParseFunc.h"

// Информация о коде, используемых функциях, переменных, типов.
namespace CodeInfo
{
	// Информация о функциях
	// Information about functions
	extern std::vector<FunctionInfo*>	funcInfo;

	// Информация о переменных
	// Information about variables
	extern std::vector<VariableInfo>	varInfo;

	// Информация о типах
	// Information about types
	extern std::vector<TypeInfo*>		typeInfo;

	// Поток комманд
	// Command stream
	extern CommandList*				cmdList;

	// Список узлов дерева
	// Отдельные узлы помещаются сюда, и в дальнейшем объеденяются в более комплексные узлы,
	// создавая дерево. После правильной компиляции количество узлов в этом массиве должно равнятся 1
	// Node tree list
	// Individual nodes are placed here, and later on, combined into a more complex nodes, 
	// creating AST. After successful compilation, node count should be equal to 1
	extern std::vector<shared_ptr<NodeZeroOP> >	nodeList;
};
