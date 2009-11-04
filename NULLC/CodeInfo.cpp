#include "stdafx.h"
#include "CodeInfo.h"
#include "Bytecode.h"

void ThrowError(const char* pos, const char* err, ...)
{
	char errorText[512];

	va_list args;
	va_start(args, err);

	vsnprintf(errorText, 512, err, args);
	errorText[512-1] = '\0';

	CodeInfo::lastError = CompilerError(errorText, pos);
	longjmp(CodeInfo::errorHandler, 1);
}

//////////////////////////////////////////////////////////////////////////
// Function returns reference type for the type
TypeInfo* CodeInfo::GetReferenceType(TypeInfo* type)
{
	// If type already has reference type, return it
	if(type->refType)
		return type->refType;

	// Create new type
	TypeInfo* newInfo = new TypeInfo(typeInfo.size(), NULL, type->refLevel + 1, 0, 1, type, TypeInfo::TYPE_INT);
	newInfo->size = 4;

	// Save it for future use
	type->refType = newInfo;

	typeInfo.push_back(newInfo);
	return newInfo;
}

// Function returns type that reference points to
TypeInfo* CodeInfo::GetDereferenceType(TypeInfo* type)
{
	// If it's not a reference, or a reference to an invalid type, return error
	if(!type->subType || type->refLevel == 0)
		ThrowError(lastKnownStartPos, "ERROR: Cannot dereference type '%s' - there is no result type available", type->GetFullTypeName());

	// Return type
	return type->subType;
}

// Function returns type that is an array for passed type
TypeInfo* CodeInfo::GetArrayType(TypeInfo* type, unsigned int sizeInArgument)
{
	int arrSize = -1;
	bool unFixed = false;
	// If size wasn't passed through argument, then it can be found in previous node
	if(sizeInArgument == 0)
	{
		// It must be a constant type
		if(nodeList.back()->nodeType == typeNodeNumber)
		{
			TypeInfo *aType = nodeList.back()->typeInfo;
			NodeZeroOP* zOP = nodeList.back();
			if(aType->type != TypeInfo::TYPE_COMPLEX && aType->type != TypeInfo::TYPE_VOID)
			{
				arrSize = static_cast<NodeNumber*>(zOP)->GetInteger();
			}else if(aType == typeVoid){	// If number type is void, then array with explicit type must be created
				arrSize = -1;
				unFixed = true;
			}else{
				ThrowError(lastKnownStartPos, "ERROR: Unknown type of constant number node '%s'", aType->name);
			}
			nodeList.pop_back();
		}else{
			ThrowError(lastKnownStartPos, "ERROR: Array size must be a constant expression");
		}
	}else{
		arrSize = sizeInArgument;
		if(arrSize == -1)
			unFixed = true;
	}

	if(!unFixed && arrSize < 1)
		ThrowError(lastKnownStartPos, "ERROR: Array size can't be negative or zero");

	// Search type list for the type that we need
	unsigned int targetArrLevel = type->arrLevel + 1;
	for(unsigned int i = 0; i < typeInfo.size(); i++)
	{
		if(type == typeInfo[i]->subType && targetArrLevel == typeInfo[i]->arrLevel && typeInfo[i]->arrSize == (unsigned int)arrSize)
		{
			return typeInfo[i];
		}
	}
	// If not found, create new type
	TypeInfo* newInfo = new TypeInfo(typeInfo.size(), NULL, 0, type->arrLevel + 1, arrSize, type, TypeInfo::TYPE_COMPLEX);

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

// Function return function type
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

int	CodeInfo::FindVariableByName(unsigned int hash)
{
	for(int i = CodeInfo::varInfo.size()-1; i >= 0; i--)
		if(CodeInfo::varInfo[i]->nameHash == hash)
			return i;
	return -1;
}

int CodeInfo::FindFunctionByName(unsigned int hash, int startPos)
{
	for(int i = startPos; i >= 0; i--)
		if(CodeInfo::funcInfo[i]->nameHash == hash && CodeInfo::funcInfo[i]->visible)
			return i;
	return -1;
}
