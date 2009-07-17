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
// Функция возвращает тип - указателя на исходный
TypeInfo* CodeInfo::GetReferenceType(TypeInfo* type)
{
	// Поищем нужный тип в списке
	unsigned int targetRefLevel = type->refLevel+1;
	for(unsigned int i = 0; i < typeInfo.size(); i++)
	{
		if(type == typeInfo[i]->subType && type->name == typeInfo[i]->name && targetRefLevel == typeInfo[i]->refLevel)
		{
			return typeInfo[i];
		}
	}
	// Создадим новый тип
	TypeInfo* newInfo = new TypeInfo();
	newInfo->name = type->name;
	newInfo->nameHash = GetStringHash(newInfo->name.c_str());
	newInfo->size = 4;
	newInfo->type = TypeInfo::TYPE_INT;
	newInfo->refLevel = type->refLevel + 1;
	newInfo->subType = type;

	typeInfo.push_back(newInfo);
	return newInfo;
}

// Функиця возвращает тип, получаемый при разименовании указателя
TypeInfo* CodeInfo::GetDereferenceType(TypeInfo* type)
{
	if(!type->subType || type->refLevel == 0)
	{
		std::string fullError = std::string("Cannot dereference type ") + type->GetTypeName() + std::string(" there is no result type available");
		lastError = CompilerError(fullError, lastKnownStartPos);
		return NULL;
	}
	return type->subType;
}

// Функция возвращает тип - массив исходных типов (кол-во элементов в varSize)
TypeInfo* CodeInfo::GetArrayType(TypeInfo* type, unsigned int sizeInArgument)
{
	int arrSize = -1;
	bool unFixed = false;
	if(sizeInArgument == 0)
	{
		// В последнем узле должно находиться константное число
		if(nodeList.back()->GetNodeType() == typeNodeNumber)
		{
			TypeInfo *aType = nodeList.back()->GetTypeInfo();
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
				std::string fullError = std::string("ERROR: unknown type of constant number node ") + aType->name;
				lastError = CompilerError(fullError, lastKnownStartPos);
				return NULL;
			}
			delete nodeList.back();
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
	// Поищем нужный тип в списке
	unsigned int targetArrLevel = type->arrLevel+1;
	for(unsigned int i = 0; i < typeInfo.size(); i++)
	{
		if(type == typeInfo[i]->subType && type->name == typeInfo[i]->name && targetArrLevel == typeInfo[i]->arrLevel && typeInfo[i]->arrSize == (unsigned int)arrSize)
		{
			return typeInfo[i];
		}
	}
	// Создадим новый тип
	TypeInfo* newInfo = new TypeInfo();
	newInfo->name = type->name;
	newInfo->nameHash = GetStringHash(newInfo->name.c_str());

	if(unFixed)
	{
		newInfo->size = 4;
		newInfo->AddMember("size", typeInt);
	}else{
		newInfo->size = type->size * arrSize;
		if(newInfo->size % 4 != 0)
		{
			newInfo->paddingBytes = 4 - (newInfo->size % 4);
			newInfo->size += 4 - (newInfo->size % 4);
		}
	}

	newInfo->type = TypeInfo::TYPE_COMPLEX;
	newInfo->arrLevel = type->arrLevel + 1;
	newInfo->arrSize = arrSize;
	newInfo->subType = type;

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
			if(typeInfo[i]->funcType->paramType.size() != info->params.size())
				continue;
			bool good = true;
			for(unsigned int n = 0; n < info->params.size(); n++)
			{
				if(info->params[n].varType != typeInfo[i]->funcType->paramType[n])
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
		typeInfo.push_back(new TypeInfo());
		bestFit = typeInfo.back();

#ifdef _DEBUG
		bestFit->AddMember("context", typeInt);
		bestFit->AddMember("ptr", typeInt);
#endif

		bestFit->funcType = new FunctionType();
		bestFit->size = 8;

		bestFit->type = TypeInfo::TYPE_COMPLEX;

		bestFit->funcType->retType = info->retType;
		for(unsigned int n = 0; n < info->params.size(); n++)
		{
			bestFit->funcType->paramType.push_back(info->params[n].varType);
		}
	}
	return bestFit;
}