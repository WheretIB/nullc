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
		ThrowError(lastKnownStartPos, "ERROR: cannot dereference type '%s' - there is no result type available", type->GetFullTypeName());

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
				ThrowError(lastKnownStartPos, "ERROR: unknown type of constant number node '%s'", aType->name);
			}
			nodeList.pop_back();
		}else{
			ThrowError(lastKnownStartPos, "ERROR: array size must be a constant expression");
		}
	}else{
		arrSize = sizeInArgument;
		if(arrSize == -1)
			unFixed = true;
	}

	if(!unFixed && arrSize < 1)
		ThrowError(lastKnownStartPos, "ERROR: array size can't be negative or zero");

	// Search type list for the type that we need
	unsigned int targetArrLevel = type->arrLevel + 1;
	for(unsigned int i = classCount; i < typeInfo.size(); i++)
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

int	CodeInfo::FindVariableByName(unsigned int hash)
{
	for(int i = varInfo.size()-1; i >= 0; i--)
		if(varInfo[i]->nameHash == hash)
			return i;

	return -1;
}

int CodeInfo::FindFunctionByName(unsigned int hash, int startPos)
{
	for(int i = startPos; i >= 0; i--)
		if(funcInfo[i]->nameHash == hash && funcInfo[i]->visible)
			return i;

	return -1;
}

unsigned int CodeInfo::FindFunctionByPtr(FunctionInfo* funcPtr)
{
	for(unsigned int i = 0; i < funcInfo.size(); i++)
		if(funcInfo[i] == funcPtr)
			return i;

	return ~0u;
}
