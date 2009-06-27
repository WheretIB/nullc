#pragma once
#include "ParseClass.h"
#include "ParseCommand.h"
#include "ParseFunc.h"

#include "Compiler.h"

unsigned int GetStringHash(const char *str);

// ���������� � ����, ������������ ��������, ����������, �����.
namespace CodeInfo
{
	static const unsigned int EXEC_VM = 0;
	static const unsigned int EXEC_X86 = 1;

	extern unsigned int		activeExecutor;

	// ���������� � ��������
	// Information about functions
	extern std::vector<FunctionInfo*>	funcInfo;

	// ���������� � ����������
	// Information about variables
	extern std::vector<VariableInfo*>	varInfo;

	// ���������� � �����
	// Information about types
	extern std::vector<TypeInfo*>		typeInfo;

	// ����� �������
	// Command stream
	extern CommandList*				cmdList;

	// The size of all global variables in bytes
	extern unsigned int				globalSize;

	// ������ ����� ������
	// ��������� ���� ���������� ����, � � ���������� ������������ � ����� ����������� ����,
	// �������� ������. ����� ���������� ���������� ���������� ����� � ���� ������� ������ �������� 1
	// Node tree list
	// Individual nodes are placed here, and later on, combined into a more complex nodes, 
	// creating AST. After successful compilation, node count should be equal to 1
	extern std::vector<shared_ptr<NodeZeroOP> >	nodeList;

	static const char* lastKnownStartPos = NULL;

	// Log stream
	extern ostringstream compileLog;

	//////////////////////////////////////////////////////////////////////////
	// ������� ���������� ��� - ��������� �� ��������
	TypeInfo* GetReferenceType(TypeInfo* type);

	// ������� ���������� ���, ���������� ��� ������������� ���������
	TypeInfo* GetDereferenceType(TypeInfo* type);

	// ������� ���������� ��� - ������ �������� ����� (���-�� ��������� � varSize)
	TypeInfo* GetArrayType(TypeInfo* type, UINT sizeInArgument = 0);

	// ������� ���������� ��� �������� �������
	TypeInfo* GetArrayElementType(TypeInfo* type);

	// ������� ���������� ��� �������
	TypeInfo* GetFunctionType(FunctionInfo* info);
};
