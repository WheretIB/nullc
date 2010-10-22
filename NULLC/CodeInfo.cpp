#include "stdafx.h"
#include "CodeInfo.h"
#include "Bytecode.h"

void ThrowError(const char* pos, const char* err, ...)
{
	char errorText[4096];

	va_list args;
	va_start(args, err);

	vsnprintf(errorText, 4096, err, args);
	errorText[4096-1] = '\0';

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
	TypeInfo* newInfo = new TypeInfo(typeInfo.size(), NULL, type->refLevel + 1, 0, 1, type, TypeInfo::NULLC_PTR_TYPE);
	newInfo->size = NULLC_PTR_SIZE;

	// Save it for future use
	type->refType = newInfo;

	newInfo->dependsOnGeneric = type->dependsOnGeneric;

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

	if(unFixed && type->unsizedType)
		return type->unsizedType;

	// Search type list for the type that we need
	TypeInfo *target = type->arrayType;
	while(target && target->arrSize != (unsigned int)arrSize)
		target = target->nextArrayType;
	if(target)
		return target;

	// If not found, create new type
	TypeInfo* newInfo = new TypeInfo(typeInfo.size(), NULL, 0, type->arrLevel + 1, arrSize, type, TypeInfo::TYPE_COMPLEX);

	if(unFixed)
	{
		newInfo->size = NULLC_PTR_SIZE;
		newInfo->AddMemberVariable("size", typeInt);
		type->unsizedType = newInfo;
	}else{
		newInfo->size = type->size * arrSize;
		if(newInfo->size % 4 != 0)
		{
			newInfo->paddingBytes = 4 - (newInfo->size % 4);
			newInfo->size += 4 - (newInfo->size % 4);
		}
	}
	newInfo->nextArrayType = type->arrayType;
	type->arrayType = newInfo;

	newInfo->dependsOnGeneric = type->dependsOnGeneric;

	typeArrays.push_back(newInfo);
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
	return funcPtr ? funcPtr->indexInArr : ~0u;
}
