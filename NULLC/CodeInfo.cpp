#include "stdafx.h"
#include "CodeInfo.h"
#include "Bytecode.h"

void ThrowError(const char* err, const char* pos)
{
	CodeInfo::lastError = CompilerError(err, pos);
	longjmp(CodeInfo::errorHandler, 1);
}
void ThrowLastError()
{
	longjmp(CodeInfo::errorHandler, 1);
}

//////////////////////////////////////////////////////////////////////////
// ������� ���������� ��� - ��������� �� ��������
TypeInfo* CodeInfo::GetReferenceType(TypeInfo* type)
{
	if(type->refType)
		return type->refType;

	// �������� ����� ���
	TypeInfo* newInfo = new TypeInfo(typeInfo.size(), type->name, type->refLevel + 1, 0, 1, type, TypeInfo::TYPE_INT);
	newInfo->size = 4;

	type->refType = newInfo;

	typeInfo.push_back(newInfo);
	return newInfo;
}

// ������� ���������� ���, ���������� ��� ������������� ���������
TypeInfo* CodeInfo::GetDereferenceType(TypeInfo* type)
{
	if(!type->subType || type->refLevel == 0)
	{
		char	errBuf[128];
		_snprintf(errBuf, 128, "ERROR: Cannot dereference type '%s' - there is no result type available", type->GetFullTypeName());
		lastError = CompilerError(errBuf, lastKnownStartPos);
		return NULL;
	}
	return type->subType;
}

// ������� ���������� ��� - ������ �������� ����� (���-�� ��������� � varSize)
TypeInfo* CodeInfo::GetArrayType(TypeInfo* type, unsigned int sizeInArgument)
{
	int arrSize = -1;
	bool unFixed = false;
	if(sizeInArgument == 0)
	{
		// � ��������� ���� ������ ���������� ����������� �����
		if(nodeList.back()->nodeType == typeNodeNumber)
		{
			TypeInfo *aType = nodeList.back()->typeInfo;
			NodeZeroOP* zOP = nodeList.back();
			if(aType == typeDouble)
			{
				arrSize = (int)static_cast<NodeNumber<double>* >(zOP)->GetVal();
			}else if(aType == typeFloat){
				arrSize = (int)static_cast<NodeNumber<float>* >(zOP)->GetVal();
			}else if(aType == typeLong){
				arrSize = (int)static_cast<NodeNumber<long long>* >(zOP)->GetVal();
			}else if(aType == typeInt){
				arrSize = static_cast<NodeNumber<int>* >(zOP)->GetVal();
			}else if(aType == typeVoid){
				arrSize = -1;
				unFixed = true;
			}else{
				char	errBuf[128];
				_snprintf(errBuf, 128, "ERROR: Unknown type of constant number node '%s'", aType->name);
				lastError = CompilerError(errBuf, lastKnownStartPos);
				return NULL;
			}
			nodeList.pop_back();
		}else{
			lastError = CompilerError("ERROR: Array size must be a constant expression", lastKnownStartPos);
			return NULL;
		}
	}else{
		arrSize = sizeInArgument;
		if(arrSize == -1)
			unFixed = true;
	}

	if(!unFixed && arrSize < 1)
	{
		lastError = CompilerError("ERROR: Array size can't be negative or zero", lastKnownStartPos);
		return NULL;
	}
	// ������ ������ ��� � ������
	unsigned int targetArrLevel = type->arrLevel+1;
	for(unsigned int i = 0; i < typeInfo.size(); i++)
	{
		if(type == typeInfo[i]->subType && type->nameHash == typeInfo[i]->nameHash && targetArrLevel == typeInfo[i]->arrLevel && typeInfo[i]->arrSize == (unsigned int)arrSize)
		{
			return typeInfo[i];
		}
	}
	// �������� ����� ���
	TypeInfo* newInfo = new TypeInfo(typeInfo.size(), type->name, 0, type->arrLevel + 1, arrSize, type, TypeInfo::TYPE_COMPLEX);

	if(unFixed)
	{
		newInfo->size = 4;
		newInfo->AddMemberVariable("size", typeInt);
	}else{
		newInfo->size = type->size * arrSize;
		if(newInfo->size % 4 != 0)
		{
			newInfo->paddingBytes = 4 - (newInfo->size % 4);
			newInfo->size += 4 - (newInfo->size % 4);
		}
	}

	typeInfo.push_back(newInfo);
	return newInfo;
}

TypeInfo* CodeInfo::GetFunctionType(FunctionInfo* info)
{
	// Find out the function type
	TypeInfo	*bestFit = NULL;
	// Search through active types
	for(unsigned int i = 0; i < typeInfo.size(); i++)
	{
		if(typeInfo[i]->funcType)
		{
			if(typeInfo[i]->funcType->retType != info->retType)
				continue;
			if(typeInfo[i]->funcType->paramCount != info->paramCount)
				continue;
			bool good = true;
			unsigned int n = 0;
			TypeInfo	**paramType = typeInfo[i]->funcType->paramType;
			for(VariableInfo *curr = info->firstParam; curr; curr = curr->next, n++)
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
		bestFit->CreateFunctionType(info->retType, info->paramCount);

		unsigned int i = 0;
		for(VariableInfo *curr = info->firstParam; curr; curr = curr->next, i++)
			bestFit->funcType->paramType[i] = curr->varType;

#ifdef _DEBUG
		bestFit->AddMemberVariable("context", typeInt);
		bestFit->AddMemberVariable("ptr", typeInt);
#endif
		bestFit->size = 8;
	}
	return bestFit;
}

int	CodeInfo::FindVariableByName(const InplaceStr& name)
{
	unsigned int hash = GetStringHash(name.begin, name.end);
	for(int i = CodeInfo::varInfo.size()-1; i >= 0; i--)
		if(CodeInfo::varInfo[i]->nameHash == hash)
			return i;
	return -1;
}

int CodeInfo::FindFunctionByName(const InplaceStr& name, int startPos)
{
	unsigned int hash = GetStringHash(name.begin, name.end);
	for(int i = startPos; i < (int)CodeInfo::funcInfo.size(); i++)
		if(CodeInfo::funcInfo[i]->nameHash == hash)
			return i;
	return -1;
}