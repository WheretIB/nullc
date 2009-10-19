#pragma once
#include "ParseClass.h"
#include "InstructionSet.h"
#include "SyntaxTree.h"

#include "Compiler.h"

void ThrowError(const char* pos, const char* err, ...);

// ������� ��������������� ������� �����
extern TypeInfo*	typeVoid;
extern TypeInfo*	typeChar;
extern TypeInfo*	typeShort;
extern TypeInfo*	typeInt;
extern TypeInfo*	typeFloat;
extern TypeInfo*	typeLong;
extern TypeInfo*	typeDouble;
extern TypeInfo*	typeFile;

// ���������� � ����, ������������ ��������, ����������, �����.
namespace CodeInfo
{
	static const unsigned int EXEC_VM = 0;
	static const unsigned int EXEC_X86 = 1;

	extern unsigned int		activeExecutor;

	extern CompilerError	lastError;

	// ���������� � ��������
	// Information about functions
	extern FastVector<FunctionInfo*>	funcInfo;

	// ���������� � ����������
	// Information about variables
	extern FastVector<VariableInfo*>	varInfo;

	// ���������� � �����
	// Information about types
	extern FastVector<TypeInfo*>		typeInfo;

	// ����� �������
	// Command stream
	extern SourceInfo				cmdInfoList;
	extern FastVector<VMCmd>		cmdList;

	// ������ �����, ������� ���������� ��� �������
	extern FastVector<NodeZeroOP*>	funcDefList;

	// ������ ����� ������
	// ��������� ���� ���������� ����, � � ���������� ������������ � ����� ����������� ����,
	// �������� ������. ����� ���������� ���������� ���������� ����� � ���� ������� ������ �������� 1
	// Node tree list
	// Individual nodes are placed here, and later on, combined into a more complex nodes, 
	// creating AST. After successful compilation, node count should be equal to 1
	extern FastVector<NodeZeroOP*>	nodeList;

	extern const char* lastKnownStartPos;

	extern jmp_buf	errorHandler;

	//////////////////////////////////////////////////////////////////////////
	// ������� ���������� ��� - ��������� �� ��������
	TypeInfo* GetReferenceType(TypeInfo* type);

	// ������� ���������� ���, ���������� ��� ������������� ���������
	TypeInfo* GetDereferenceType(TypeInfo* type);

	// ������� ���������� ��� - ������ �������� ����� (���-�� ��������� � varSize)
	TypeInfo* GetArrayType(TypeInfo* type, unsigned int sizeInArgument = 0);

	// ������� ���������� ��� �������
	TypeInfo* GetFunctionType(FunctionInfo* info);

	// Search for the variable starting from the end of a list. -1 is returned if the variable not found
	int	FindVariableByName(unsigned int hash);

	// Search for the function from specified position to the beginning of a list. -1 is returned if the function not found
	int FindFunctionByName(unsigned int hash, int startPos);
};
