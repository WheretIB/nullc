#include "stdafx.h"
#include "CodeInfo.h"
#include "Bytecode.h"

//////////////////////////////////////////////////////////////////////////
// Function returns reference type for the type
TypeInfo* CodeInfo::GetReferenceType(TypeInfo* type)
{
	// If type already has reference type, return it
	if(type->refType)
		return type->refType;

	// Create new type
	TypeInfo* newInfo = new TypeInfo(typeInfo.size(), NULL, type->refLevel + 1, 0, 1, type, TypeInfo::NULLC_PTR_TYPE);
	newInfo->alignBytes = 4;
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
			if(aType->type != TypeInfo::TYPE_COMPLEX && aType->type != TypeInfo::TYPE_VOID && aType->refLevel == 0)
			{
				arrSize = static_cast<NodeNumber*>(zOP)->GetInteger();
			}else if(aType == typeVoid){	// If number type is void, then array with explicit type must be created
				arrSize = -1;
				unFixed = true;
			}else{
				ThrowError(lastKnownStartPos, "ERROR: invalid value type for array size: '%s'", aType->GetFullTypeName());
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
	
	if(!unFixed && type->hasFinalizer)
		ThrowError(lastKnownStartPos, "ERROR: class '%s' implements 'finalize' so only an unsized array type can be created", type->GetFullTypeName());
	if(!unFixed && !type->hasFinished)
		ThrowError(lastKnownStartPos, "ERROR: type '%s' is not fully defined. You can use '%s ref' or '%s[]' at this point", type->GetFullTypeName(), type->GetFullTypeName(), type->GetFullTypeName());

	// Search type list for the type that we need
	TypeInfo *target = type->arrayType;
	while(target && target->arrSize != (unsigned int)arrSize)
		target = target->nextArrayType;
	if(target)
		return target;

	// If not found, create new type
	TypeInfo* newInfo = new TypeInfo(typeInfo.size(), NULL, 0, type->arrLevel + 1, arrSize, type, TypeInfo::TYPE_COMPLEX);
	newInfo->alignBytes = 4;

	if(unFixed)
	{
		newInfo->size = NULLC_PTR_SIZE;
		newInfo->AddMemberVariable("size", typeInt);
		type->unsizedType = newInfo;
	}else{
		newInfo->size = type->size * arrSize;
		newInfo->alignBytes = type->alignBytes;

		if((unsigned long long)type->size * arrSize > NULLC_MAX_TYPE_SIZE)
			ThrowError(lastKnownStartPos, "ERROR: type size (%lld) exceeds maximum of %d", (unsigned long long)type->size * arrSize, NULLC_MAX_TYPE_SIZE);

		unsigned maximumAlignment = newInfo->alignBytes < 4 ? 4 : newInfo->alignBytes;

		if(newInfo->size % maximumAlignment != 0)
		{
			newInfo->paddingBytes = maximumAlignment - (newInfo->size % maximumAlignment);
			newInfo->size += maximumAlignment - (newInfo->size % maximumAlignment);
		}
	}
	newInfo->nextArrayType = type->arrayType;
	type->arrayType = newInfo;

	newInfo->dependsOnGeneric = type->dependsOnGeneric;

	typeArrays.push_back(newInfo);
	typeInfo.push_back(newInfo);
	return newInfo;
}

int CodeInfo::FindFunctionByName(unsigned int hash, int startPos)
{
	for(int i = startPos; i >= 0; i--)
		if(funcInfo[i]->nameHash == hash && funcInfo[i]->visible && !((funcInfo[i]->address & 0x80000000) && !(funcInfo[i]->address == -1)))
			return i;

	return -1;
}

unsigned int CodeInfo::FindFunctionByPtr(FunctionInfo* funcPtr)
{
	return funcPtr ? funcPtr->indexInArr : ~0u;
}
