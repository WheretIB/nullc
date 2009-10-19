#pragma once
#include "ParseClass.h"
#include "InstructionSet.h"
#include "SyntaxTree.h"

#include "Compiler.h"

void ThrowError(const char* pos, const char* err, ...);

// Немного предопределённых базовых типов
extern TypeInfo*	typeVoid;
extern TypeInfo*	typeChar;
extern TypeInfo*	typeShort;
extern TypeInfo*	typeInt;
extern TypeInfo*	typeFloat;
extern TypeInfo*	typeLong;
extern TypeInfo*	typeDouble;
extern TypeInfo*	typeFile;

// Информация о коде, используемых функциях, переменных, типов.
namespace CodeInfo
{
	static const unsigned int EXEC_VM = 0;
	static const unsigned int EXEC_X86 = 1;

	extern unsigned int		activeExecutor;

	extern CompilerError	lastError;

	// Информация о функциях
	// Information about functions
	extern FastVector<FunctionInfo*>	funcInfo;

	// Информация о переменных
	// Information about variables
	extern FastVector<VariableInfo*>	varInfo;

	// Информация о типах
	// Information about types
	extern FastVector<TypeInfo*>		typeInfo;

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
	// Функция возвращает тип - указателя на исходный
	TypeInfo* GetReferenceType(TypeInfo* type);

	// Функиця возвращает тип, получаемый при разименовании указателя
	TypeInfo* GetDereferenceType(TypeInfo* type);

	// Функция возвращает тип - массив исходных типов (кол-во элементов в varSize)
	TypeInfo* GetArrayType(TypeInfo* type, unsigned int sizeInArgument = 0);

	// Функция возвращает тип функции
	TypeInfo* GetFunctionType(FunctionInfo* info);

	// Search for the variable starting from the end of a list. -1 is returned if the variable not found
	int	FindVariableByName(unsigned int hash);

	// Search for the function from specified position to the beginning of a list. -1 is returned if the function not found
	int FindFunctionByName(unsigned int hash, int startPos);
};
