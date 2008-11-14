#include "stdafx.h"
#include "CodeInfo.h"

ostringstream CodeInfo::compileLog;

//////////////////////////////////////////////////////////////////////////
// Функция возвращает тип - указателя на исходный
TypeInfo* CodeInfo::GetReferenceType(TypeInfo* type)
{
	compileLog << "GetReferenceType(" << type->GetTypeName() << ")\r\n";
	// Поищем нужный тип в списке
	UINT targetRefLevel = type->refLevel+1;
	for(UINT i = 0; i < typeInfo.size(); i++)
	{
		if(type == typeInfo[i]->subType && type->name == typeInfo[i]->name && targetRefLevel == typeInfo[i]->refLevel)
		{
			compileLog << "  returns " << typeInfo[i]->GetTypeName() << "\r\n";
			return typeInfo[i];
		}
	}
	// Создадим новый тип
	TypeInfo* newInfo = new TypeInfo();
	newInfo->name = type->name;
	newInfo->size = 4;
	newInfo->type = TypeInfo::TYPE_INT;
	newInfo->refLevel = type->refLevel + 1;
	newInfo->subType = type;

	typeInfo.push_back(newInfo);
	compileLog << "  returns " << newInfo->GetTypeName() << "\r\n";
	return newInfo;
}

// Функиця возвращает тип, получаемый при разименовании указателя
TypeInfo* CodeInfo::GetDereferenceType(TypeInfo* type)
{
	compileLog << "GetDereferenceType(" << type->GetTypeName() << ")\r\n";
	if(!type->subType || type->refLevel == 0)
		throw CompilerError(std::string("Cannot dereference type ") + type->GetTypeName() + std::string(" there is no result type available"), lastKnownStartPos);
	compileLog << "  returns " << type->subType->GetTypeName() << "\r\n";
	return type->subType;
}

// Функция возвращает тип - массив исходных типов (кол-во элементов в varSize)
TypeInfo* CodeInfo::GetArrayType(TypeInfo* type, UINT sizeInArgument)
{
	int arrSize = -1;
	bool unFixed = false;
	if(sizeInArgument == 0)
	{
		// В последнем узле должно находиться константное число
		if((*(nodeList.end()-1))->GetNodeType() == typeNodeNumber)
		{
			TypeInfo *aType = (*(nodeList.end()-1))->GetTypeInfo();
			NodeZeroOP* zOP = (nodeList.end()-1)->get();
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
				throw CompilerError(std::string("ERROR: unknown type of constant number node ") + aType->name, lastKnownStartPos);
			}
			nodeList.pop_back();
		}else{
			throw CompilerError("ERROR: Array size must be a constant expression", lastKnownStartPos);
		}
	}else{
		arrSize = sizeInArgument;
		if(arrSize == -1)
			unFixed = true;
	}

	if(!unFixed && arrSize < 1)
		throw CompilerError("ERROR: Array size can't be negative or zero", lastKnownStartPos);
	compileLog << "GetArrayType(" << type->GetTypeName() << ", " << arrSize << ")\r\n";
	// Поищем нужный тип в списке
	UINT targetArrLevel = type->arrLevel+1;
	for(UINT i = 0; i < typeInfo.size(); i++)
	{
		if(type == typeInfo[i]->subType && type->name == typeInfo[i]->name && targetArrLevel == typeInfo[i]->arrLevel && typeInfo[i]->arrSize == arrSize)
		{
			compileLog << "  returns " << typeInfo[i]->GetTypeName() << " Address: " << typeInfo[i] << "\r\n";
			return typeInfo[i];
		}
	}
	// Создадим новый тип
	TypeInfo* newInfo = new TypeInfo();
	newInfo->name = type->name;

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
	compileLog << "  returns " << newInfo->GetTypeName() << " Address: " << newInfo << "\r\n";
	return newInfo;
}

// Функция возвращает тип элемента массива
TypeInfo* CodeInfo::GetArrayElementType(TypeInfo* type)
{
	compileLog << "GetArrayElementType(" << type->GetTypeName() << ")\r\n";
	if(!type->subType || type->arrLevel == 0)
		throw CompilerError(std::string("Cannot return array element type, ") + type->GetTypeName() + std::string(" is not an array"), lastKnownStartPos);
	compileLog << "  returns " << type->subType->GetTypeName() << "\r\n";
	return type->subType;
}