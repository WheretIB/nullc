#pragma once
#include "ParseClass.h"
#include "ParseCommand.h"
#include "ParseFunc.h"

// ���������� � ����, ������������ ��������, ����������, �����.
namespace CodeInfo
{
	// ���������� � ��������
	// Information about functions
	extern std::vector<FunctionInfo*>	funcInfo;

	// ���������� � ����������
	// Information about variables
	extern std::vector<VariableInfo>	varInfo;

	// ���������� � �����
	// Information about types
	extern std::vector<TypeInfo*>		typeInfo;

	// ����� �������
	// Command stream
	extern CommandList*				cmdList;

	// ������ ����� ������
	// ��������� ���� ���������� ����, � � ���������� ������������ � ����� ����������� ����,
	// �������� ������. ����� ���������� ���������� ���������� ����� � ���� ������� ������ �������� 1
	// Node tree list
	// Individual nodes are placed here, and later on, combined into a more complex nodes, 
	// creating AST. After successful compilation, node count should be equal to 1
	extern std::vector<shared_ptr<NodeZeroOP> >	nodeList;
};
