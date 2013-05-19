#include "SyntaxTree.h"
#include "CodeInfo.h"

#ifdef NULLC_ENABLE_C_TRANSLATION

#include <float.h>

void GetEscapedName(char* result)
{
	unsigned int finalLength = strlen(result);
	for(unsigned int k = 0; k < finalLength; k++)
	{
		if(result[k] == ':' || result[k] == '$' || result[k] == '[' || result[k] == ']' || result[k] == ' ' || result[k] == '(' || result[k] == ')' || result[k] == ',')
			result[k] = '_';
	}
}

void GetCFunctionName(char* fName, unsigned int size, FunctionInfo *funcInfo)
{
	const char *namePrefix = *funcInfo->name == '$' ? "__" : "";
	unsigned int nameShift = *funcInfo->name == '$' ? 1 : 0;
	unsigned int finalLength = 0;
	const char *printedName = funcInfo->GetOperatorName();
	unsigned funcID = ((funcInfo->address & 0x80000000) && (funcInfo->address != -1)) ? (funcInfo->address & ~0x80000000) : funcInfo->indexInArr;
	if((funcInfo->type == FunctionInfo::LOCAL || funcInfo->type == FunctionInfo::COROUTINE) || !funcInfo->visible)
		finalLength = SafeSprintf(fName, size, "%s%s_%d", namePrefix, printedName ? printedName : (funcInfo->name + nameShift), funcID);
	else if(funcInfo->type == FunctionInfo::THISCALL)
		finalLength = SafeSprintf(fName, size, "%s%s_%s", namePrefix, printedName ? printedName : (funcInfo->name + nameShift), funcInfo->funcType->GetFullTypeName());
	else
		finalLength = SafeSprintf(fName, size, "%s%s", namePrefix, printedName ? printedName : (funcInfo->name + nameShift));
	
	for(unsigned int k = 0; k < finalLength; k++)
	{
		if(fName[k] == ':' || fName[k] == '$' || fName[k] == '[' || fName[k] == ']' || fName[k] == ' ' || fName[k] == '(' || fName[k] == ')' || fName[k] == ',')
			fName[k] = '_';
	}
	TypeInfo **type = CodeInfo::classMap.find(funcInfo->nameHash);
	if(type)
		strcat(fName, "__");
	
	unsigned int length = (unsigned int)strlen(fName);
	if(fName[length-1] == '$')
		fName[length-1] = '_';
}

void OutputCFunctionName(FILE *fOut, FunctionInfo *funcInfo)
{
	char fName[NULLC_MAX_VARIABLE_NAME_LENGTH + 32];
	GetCFunctionName(fName, NULLC_MAX_VARIABLE_NAME_LENGTH + 32, funcInfo);
	fprintf(fOut, "%s", fName);
}

bool nodeDereferenceEndInComma = false;

unsigned int indentDepth = 1;
void OutputIdent(FILE *fOut)
{
	for(unsigned int i = 0; i < indentDepth; i++)
		fprintf(fOut, "\t");
}

void NodeZeroOP::TranslateToC(FILE *fOut)
{
	OutputIdent(fOut);
	fprintf(fOut, "/* node translation unknown */\r\n");
}

void NodeZeroOP::TranslateToCExtra(FILE *fOut)
{
	NodeZeroOP *curr = head;
	while(curr)
	{
		curr->TranslateToC(fOut);
		curr = curr->next;
	}
}

void NodeOneOP::TranslateToC(FILE *fOut)
{
	if(typeInfo->refLevel == 3 && first->typeInfo->refLevel && first->typeInfo->subType->funcType)
	{
		fprintf(fOut, "((int***)");
		first->TranslateToC(fOut);
		fprintf(fOut, ".context)");
	}else{
		first->TranslateToC(fOut);
	}
}

void NodeNumber::TranslateToC(FILE *fOut)
{
	if(typeInfo->refLevel)
	{
		fprintf(fOut, "(");
		typeInfo->OutputCType(fOut, "");
		fprintf(fOut, ")(%d)", num.integer);
	}else if(typeInfo == typeChar || typeInfo == typeShort || typeInfo == typeInt){
		fprintf(fOut, "%d", num.integer);
	}else if(typeInfo == typeDouble || typeInfo == typeFloat){
#ifdef __linux
		if(isnan(num.real))
			fprintf(fOut, "(0.0 / __nullcZero())");
		else if(isinf(num.real) && num.real < 0)
			fprintf(fOut, "(-1.0 / __nullcZero())");
		else if(isinf(num.real))
			fprintf(fOut, "(1.0 / __nullcZero())");
		else
			fprintf(fOut, typeInfo == typeFloat ? "%ff" : "%f", num.real);
#else
		switch(_fpclass(num.real))
		{
		case _FPCLASS_PINF:
			fprintf(fOut, "(1.0 / __nullcZero())");
			break;
		case _FPCLASS_NINF:
			fprintf(fOut, "(-1.0 / __nullcZero())");
			break;
		case _FPCLASS_SNAN:
		case _FPCLASS_QNAN:
			fprintf(fOut, "(0.0 / __nullcZero())");
			break;
		default:
			fprintf(fOut, typeInfo == typeFloat ? "%ff" : "%f", num.real);
		}
#endif
	}else if(typeInfo == typeLong)
		fprintf(fOut, "%lldLL", num.integer64);
	else
		fprintf(fOut, "%%unknown_number%%");
}

void NodePopOp::TranslateToC(FILE *fOut)
{
	OutputIdent(fOut);
	first->TranslateToC(fOut);
	fprintf(fOut, ";\r\n");
}

void NodeUnaryOp::TranslateToC(FILE *fOut)
{
	switch(vmCmd.cmd)
	{
	case cmdNeg:
		fprintf(fOut, "-");
		break;
	case cmdBitNot:
		fprintf(fOut, "~");
		break;
	case cmdLogNot:
		fprintf(fOut, "!");
		break;
	case cmdPushTypeID:
		fprintf(fOut, "__nullcTR[%d]", vmCmd.argument);
		return;
	case cmdFuncAddr:
		fprintf(fOut, "__nullcFR[%d]", vmCmd.argument);
		return;
	case cmdCheckedRet:
		fprintf(fOut, "__nullcCheckedRet(__nullcTR[%d], ", vmCmd.argument);
		break;
	default:
		fprintf(fOut, "%%unknown_unary_command%%");
	}
	fprintf(fOut, "(");
	first->TranslateToC(fOut);
	fprintf(fOut, ")");
	if(vmCmd.cmd == cmdCheckedRet)
	{
		assert(parentFunc);

		char name[NULLC_MAX_VARIABLE_NAME_LENGTH + 32];
		VariableInfo *param = parentFunc->firstParam;
		for(; param; param = param->next)
		{
			SafeSprintf(name, NULLC_MAX_VARIABLE_NAME_LENGTH + 32, "%.*s_%d", param->name.length(), param->name.begin, param->pos);
			fprintf(fOut, ", (void*)&%s", name);
		}
		if(parentFunc->type == FunctionInfo::THISCALL)
		{
			fprintf(fOut, ", (void*)&__context");
		}else if(parentFunc->type == FunctionInfo::LOCAL || parentFunc->type == FunctionInfo::COROUTINE){
			fprintf(fOut, ", (void*)&__");
			OutputCFunctionName(fOut, parentFunc);
			fprintf(fOut, "_ext_%d", parentFunc->allParamSize);
		}else{
			fprintf(fOut, ", (void*)&unused");
		}

		VariableInfo *local = parentFunc->firstLocal;
		for(; local; local = local->next)
		{
			const char *namePrefix = *local->name.begin == '$' ? "__" : "";
			unsigned int nameShift = *local->name.begin == '$' ? 1 : 0;
			SafeSprintf(name, NULLC_MAX_VARIABLE_NAME_LENGTH + 32, "%s%.*s_%d", namePrefix, local->name.length() - nameShift, local->name.begin+nameShift, local->pos);
			GetEscapedName(name);
			fprintf(fOut, ", (void*)&%s", name);
		}

		fprintf(fOut, ", 0)");
	}
}

void NodeReturnOp::TranslateToC(FILE *fOut)
{
	static unsigned int retVarID = 0;

	TranslateToCExtra(fOut);
	if(parentFunction && parentFunction->closeUpvals)
	{
		OutputIdent(fOut);
		fprintf(fOut, "{");
		typeInfo->OutputCType(fOut, "");
		fprintf(fOut, "__nullcRetVar%d = ", retVarID);
		if(typeInfo != first->typeInfo)
		{
			fprintf(fOut, "(");
			typeInfo->OutputCType(fOut, "");
			fprintf(fOut, ")(");
		}
		if(first->nodeType == typeNodeZeroOp && first->typeInfo != typeVoid)
		{
			if(typeInfo == typeLong)
			{
				fprintf(fOut, "(long long)0");
			}else{
				typeInfo->OutputCType(fOut, "");
				fprintf(fOut, "()");
			}
		}else{
			first->TranslateToC(fOut);
		}
		if(typeInfo != first->typeInfo)
			fprintf(fOut, ")");
		fprintf(fOut, ";\r\n");

		char name[NULLC_MAX_VARIABLE_NAME_LENGTH];
		// Glue together parameter list, extra parameter and local list. Every list could be empty.
		VariableInfo *curr = parentFunction->firstParam ? parentFunction->firstParam : (parentFunction->firstLocal ? parentFunction->firstLocal : parentFunction->extraParam);
		if(parentFunction->firstParam)
			parentFunction->lastParam->next = (parentFunction->firstLocal ? parentFunction->firstLocal : parentFunction->extraParam);
		if(parentFunction->firstLocal)
			parentFunction->lastLocal->next = parentFunction->extraParam;
		unsigned int hashThis = GetStringHash("this");
		for(; curr; curr = curr->next)
		{
			if(curr->usedAsExternal)
			{
				const char *namePrefix = *curr->name.begin == '$' ? "__" : "";
				unsigned int nameShift = *curr->name.begin == '$' ? 1 : 0;
				sprintf(name, "%s%.*s_%d", namePrefix, curr->name.length() - nameShift, curr->name.begin + nameShift, curr->pos);
				GetEscapedName(name);
				OutputIdent(fOut);
				if(curr->nameHash == hashThis)
					fprintf(fOut, "__nullcCloseUpvalue(__upvalue_%d___context, &__context);\r\n", CodeInfo::FindFunctionByPtr(curr->parentFunction));
				else
					fprintf(fOut, "__nullcCloseUpvalue(__upvalue_%d_%s, &%s);\r\n", CodeInfo::FindFunctionByPtr(curr->parentFunction), name, name);
			}
		}
		if(parentFunction->firstParam)
			parentFunction->lastParam->next = NULL;
		if(parentFunction->firstLocal)
			parentFunction->lastLocal->next = NULL;
		if(parentFunction && parentFunction->type == FunctionInfo::COROUTINE)
		{
			OutputIdent(fOut);
			fprintf(fOut, "*(int*)((char*)__");
			OutputCFunctionName(fOut, parentFunction);
			fprintf(fOut, "_ext_%d + %d)", parentFunction->allParamSize, NULLC_PTR_SIZE);
			fprintf(fOut, " = %d;\r\n", yieldResult ? (parentFunction->yieldCount + 1) : 0);
		}
		OutputIdent(fOut);
		fprintf(fOut, "return __nullcRetVar%d;}\r\n", retVarID++);
		if(yieldResult && parentFunction && parentFunction->type == FunctionInfo::COROUTINE)
		{
			OutputIdent(fOut);
			fprintf(fOut, "yield%d: (void)0;\r\n", parentFunction->yieldCount + 1);
			parentFunction->yieldCount++;
		}
		return;
	}
	if(parentFunction && parentFunction->type == FunctionInfo::COROUTINE)
	{
		OutputIdent(fOut);
		fprintf(fOut, "*(int*)((char*)__");
		OutputCFunctionName(fOut, parentFunction);
		fprintf(fOut, "_ext_%d + %d)", parentFunction->allParamSize, NULLC_PTR_SIZE);
		fprintf(fOut, " = %d;\r\n", yieldResult ? (parentFunction->yieldCount + 1) : 0);
	}
	OutputIdent(fOut);
	if(!localReturn)
	{
		if(typeInfo == typeChar || typeInfo == typeShort|| typeInfo == typeInt)
			fprintf(fOut, "__nullcOutputResultInt((int)");
		else if(typeInfo == typeLong)
			fprintf(fOut, "__nullcOutputResultLong((long long)");
		else if(typeInfo == typeFloat || typeInfo == typeDouble)
			fprintf(fOut, "__nullcOutputResultDouble((double)");
		first->TranslateToC(fOut);
		fprintf(fOut, ");\r\n");
		OutputIdent(fOut);
		fprintf(fOut, "return 0;\r\n");
		return;
	}
	if(typeInfo == typeVoid || first->typeInfo == typeVoid)
	{
		fprintf(fOut, "return;\r\n");
	}else{
		fprintf(fOut, "return ");
		if(typeInfo != first->typeInfo)
		{
			fprintf(fOut, "(");
			typeInfo->OutputCType(fOut, "");
			fprintf(fOut, ")(");
		}
		if(first->nodeType == typeNodeZeroOp && first->typeInfo != typeVoid)
		{
			if(typeInfo == typeLong)
			{
				fprintf(fOut, "(long long)0");
			}else{
				typeInfo->OutputCType(fOut, "");
				fprintf(fOut, "()");
			}
		}else{
			first->TranslateToC(fOut);
		}
		if(typeInfo != first->typeInfo)
			fprintf(fOut, ")");
		fprintf(fOut, ";\r\n");
	}
	if(yieldResult && parentFunction && parentFunction->type == FunctionInfo::COROUTINE)
	{
		OutputIdent(fOut);
		fprintf(fOut, "yield%d: (void)0;\r\n", parentFunction->yieldCount + 1);
		parentFunction->yieldCount++;
	}
}

void NodeBlock::TranslateToC(FILE *fOut)
{
	first->TranslateToC(fOut);
	char name[NULLC_MAX_VARIABLE_NAME_LENGTH];
	// Glue together parameter list, extra parameter and local list. Every list could be empty.
	VariableInfo *curr = parentFunction->firstParam ? parentFunction->firstParam : (parentFunction->firstLocal ? parentFunction->firstLocal : parentFunction->extraParam);
	if(parentFunction->firstParam)
		parentFunction->lastParam->next = (parentFunction->firstLocal ? parentFunction->firstLocal : parentFunction->extraParam);
	if(parentFunction->firstLocal)
		parentFunction->lastLocal->next = parentFunction->extraParam;
	unsigned int hashThis = GetStringHash("this");
	for(; curr; curr = curr->next)
	{
		if(curr->usedAsExternal)
		{
			const char *namePrefix = *curr->name.begin == '$' ? "__" : "";
			unsigned int nameShift = *curr->name.begin == '$' ? 1 : 0;
			sprintf(name, "%s%.*s_%d", namePrefix, curr->name.length() - nameShift, curr->name.begin + nameShift, curr->pos);
			GetEscapedName(name);
			OutputIdent(fOut);
			if(curr->nameHash == hashThis)
				fprintf(fOut, "__nullcCloseUpvalue(__upvalue_%d___context, &__context);\r\n", CodeInfo::FindFunctionByPtr(curr->parentFunction));
			else
				fprintf(fOut, "__nullcCloseUpvalue(__upvalue_%d_%s, &%s);\r\n", CodeInfo::FindFunctionByPtr(curr->parentFunction), name, name);
		}
	}
	if(parentFunction->firstParam)
		parentFunction->lastParam->next = NULL;
	if(parentFunction->firstLocal)
		parentFunction->lastLocal->next = NULL;
}

void NodeFuncDef::TranslateToC(FILE *fOut)
{
	unsigned int oldIndent = indentDepth;
	indentDepth = 0;
	if(!disabled)
	{
		if(strcmp(funcInfo->name, "$gen_list") == 0 || memcmp(funcInfo->name, "$genl", 5) == 0)
			fprintf(fOut, "static ");
		funcInfo->retType->OutputCType(fOut, " ");
		OutputCFunctionName(fOut, funcInfo);
		fprintf(fOut, "(");

		char name[NULLC_MAX_VARIABLE_NAME_LENGTH + 32];
		VariableInfo *param = funcInfo->firstParam;
		for(; param; param = param->next)
		{
			SafeSprintf(name, NULLC_MAX_VARIABLE_NAME_LENGTH + 32, "%.*s_%d", param->name.length(), param->name.begin, param->pos);
			param->varType->OutputCType(fOut, name);
			fprintf(fOut, ", ");
		}
		if(funcInfo->type == FunctionInfo::THISCALL)
		{
			funcInfo->parentClass->OutputCType(fOut, "* __context");
		}else if(funcInfo->type == FunctionInfo::LOCAL || funcInfo->type == FunctionInfo::COROUTINE){
			fprintf(fOut, "void* __");
			OutputCFunctionName(fOut, funcInfo);
			fprintf(fOut, "_ext_%d", funcInfo->allParamSize);
		}else{
			fprintf(fOut, "void* unused");
		}

		fprintf(fOut, ")\r\n{\r\n");
		indentDepth++;
		VariableInfo *local = funcInfo->firstLocal;
		for(; local; local = local->next)
		{
			OutputIdent(fOut);
			const char *namePrefix = *local->name.begin == '$' ? "__" : "";
			unsigned int nameShift = *local->name.begin == '$' ? 1 : 0;
			SafeSprintf(name, NULLC_MAX_VARIABLE_NAME_LENGTH + 32, "%s%.*s_%d", namePrefix, local->name.length() - nameShift, local->name.begin + nameShift, local->pos);
			GetEscapedName(name);
			local->varType->OutputCType(fOut, name);
			fprintf(fOut, ";\r\n");
		}
		if(funcInfo->type == FunctionInfo::COROUTINE)
		{
			for(unsigned i = 0; i < funcInfo->yieldCount; i++)
			{
				OutputIdent(fOut);

				fprintf(fOut, "if(*(int*)((char*)__");
				OutputCFunctionName(fOut, funcInfo);
				fprintf(fOut, "_ext_%d + %d)", funcInfo->allParamSize, NULLC_PTR_SIZE);

				fprintf(fOut, " == %d)\r\n", i + 1);
				OutputIdent(fOut);
				fprintf(fOut, "\tgoto yield%d;\r\n", i + 1);
			}
			funcInfo->yieldCount = 0;
		}
		first->TranslateToC(fOut);
		indentDepth--;
		fprintf(fOut, "}\r\n");
	}else{
		indentDepth++;
		TranslateToCExtra(fOut);
		indentDepth--;
	}
	indentDepth = oldIndent;
}

void NodeFuncCall::TranslateToC(FILE *fOut)
{
	static unsigned newSFuncHash = GetStringHash("__newS");
	static unsigned newAFuncHash = GetStringHash("__newA");
	TranslateToCExtra(fOut);
	if(funcInfo)
		OutputCFunctionName(fOut, funcInfo);
	if(!funcInfo)
	{
		fprintf(fOut, "((");
		funcType->retType->OutputCType(fOut, "");
		fprintf(fOut, "(*)(");
		NodeZeroOP	*curr = paramTail;
		TypeInfo	**paramType = funcType->paramType;
		while(curr)
		{
			(*paramType)->OutputCType(fOut, "");
			fprintf(fOut, ", ");
			curr = curr->prev;
			paramType++;
		}
		fprintf(fOut, "void*))");

		fprintf(fOut, "(*__nullcFM)[(");
		first->TranslateToC(fOut);
		fprintf(fOut, ").id])");
	}
	fprintf(fOut, "(");
	NodeZeroOP	*curr = paramTail;
	TypeInfo	**paramType = funcType->paramType;
	while(curr)
	{
		if(*paramType != curr->typeInfo)
		{
			fprintf(fOut, "(");
			(*paramType)->OutputCType(fOut, "");
			fprintf(fOut, ")(");
		}
		curr->TranslateToC(fOut);
		if(*paramType != curr->typeInfo)
			fprintf(fOut, ")");
		fprintf(fOut, ", ");
		curr = curr->prev;
		paramType++;
	}
	if(!funcInfo)
		fprintf(fOut, "(");
	if(first)
	{
		if(funcInfo && funcInfo->extraParam)
		{
			fprintf(fOut, "(");
			funcInfo->extraParam->varType->OutputCType(fOut, "");
			fprintf(fOut, ")(");
		}
		first->TranslateToC(fOut);
		if(funcInfo && funcInfo->extraParam)
			fprintf(fOut, ")");
	}else if(funcInfo){
		fprintf(fOut, "(void*)0");
	}
	if(!funcInfo)
		fprintf(fOut, ").context");
	fprintf(fOut, ")");
}

void NodeGetAddress::TranslateToC(FILE *fOut)
{
	if(head)
		fprintf(fOut, "(");
	NodeZeroOP *curr = head;
	while(curr)
	{
		assert(curr->nodeType == typeNodePopOp);
		((NodePopOp*)curr)->GetFirstNode()->TranslateToC(fOut);
		fprintf(fOut, ", ");
		curr = curr->next;
	}
	TranslateToCEx(fOut, true);
	if(head)
		fprintf(fOut, ")");
}
void NodeGetAddress::TranslateToCEx(FILE *fOut, bool takeAddress)
{
	if(takeAddress)
		fprintf(fOut, "&");
	if(varInfo && varInfo->nameHash != GetStringHash("this"))
	{
		const char *namePrefix = *varInfo->name.begin == '$' ? "__" : "";
		unsigned int nameShift = *varInfo->name.begin == '$' ? 1 : 0;
		fprintf(fOut, "%s%.*s", namePrefix, int(varInfo->name.end-varInfo->name.begin) - nameShift, varInfo->name.begin + nameShift);
		if(varInfo->blockDepth > 1)
			fprintf(fOut, "_%d", varInfo->pos);
	}else{
		fprintf(fOut, "__context");
	}
}

void NodeGetUpvalue::TranslateToC(FILE *fOut)
{
	fprintf(fOut, "(");
	typeInfo->OutputCType(fOut, "");
	fprintf(fOut, ")");
	fprintf(fOut, "((__nullcUpvalue*)((char*)__");
	OutputCFunctionName(fOut, parentFunc);
	fprintf(fOut, "_ext_%d + %d))->ptr", closurePos, closureElem);
}

void NodeConvertPtr::TranslateToC(FILE *fOut)
{
	if(typeInfo == typeTypeid)
	{
		fprintf(fOut, "%d", first->typeInfo->subType->typeIndex);
		return;
	}
	TranslateToCExtra(fOut);
	
	if(typeInfo == typeObject || typeInfo == typeTypeid)
	{
		fprintf(fOut, "__nullcMakeAutoRef((void*)");
		first->TranslateToC(fOut);
		fprintf(fOut, ", __nullcTR[%d])", first->typeInfo->subType->typeIndex);
	}else{
		fprintf(fOut, "(");
		typeInfo->OutputCType(fOut, "");
		fprintf(fOut, ")");
		fprintf(fOut, "__nullcGetAutoRef(");
		first->TranslateToC(fOut);
		fprintf(fOut, ", __nullcTR[%d])", typeInfo->subType->typeIndex);
	}
}

void NodeVariableSet::TranslateToC(FILE *fOut)
{
	TranslateToCExtra(fOut);
	if(arrSetAll)
	{
		if(typeInfo == typeChar)
			fprintf(fOut, "memset(*(");
		else
			fprintf(fOut, "__nullcSetArray(*(");
		first->TranslateToC(fOut);
		fprintf(fOut, ".ptr), ");
		second->TranslateToC(fOut);
		fprintf(fOut, ", %d)", elemCount);
	}else{
		if(second->nodeType == typeNodeExpressionList && second->typeInfo->subType == typeChar && second->typeInfo->arrSize != TypeInfo::UNSIZED_ARRAY)
		{
			fprintf(fOut, "memcpy((");
			first->TranslateToC(fOut);
			fprintf(fOut, ")->ptr, ");
			second->TranslateToC(fOut);
			fprintf(fOut, ", %d)", first->typeInfo->subType->size);
		}else{
			fprintf(fOut, "*(");
			first->TranslateToC(fOut);
			fprintf(fOut, ") = ");
			if(first->typeInfo->subType != second->typeInfo || (first->typeInfo->subType->refLevel && second->nodeType == typeNodeFuncCall))
			{
				fprintf(fOut, "(");
				first->typeInfo->subType->OutputCType(fOut, "");
				fprintf(fOut, ")");
			}
			second->TranslateToC(fOut);
		}
	}
}

void NodeVariableModify::TranslateToC(FILE *fOut)
{
	TranslateToCExtra(fOut);
	if(cmdID == cmdPow)
	{
		fprintf(fOut, "__nullcPowSet(");
		first->TranslateToC(fOut);
		fprintf(fOut, ", ");
		second->TranslateToC(fOut);
		fprintf(fOut, ")");
	}else{
		const char *operation = "???";
		switch(cmdID)
		{
		case cmdAdd:
			operation = "+=";
			break;
		case cmdSub:
			operation = "-=";
			break;
		case cmdMul:
			operation = "*=";
			break;
		case cmdDiv:
			operation = "/=";
			break;
		}
		fprintf(fOut, "*(");
		first->TranslateToC(fOut);
		fprintf(fOut, ") %s ", operation);
		second->TranslateToC(fOut);
	}
}

void NodeArrayIndex::TranslateToC(FILE *fOut)
{
	if(first->typeInfo->arrSize == TypeInfo::UNSIZED_ARRAY)
	{
		fprintf(fOut, "(");
		fprintf(fOut, "(");
		typeInfo->OutputCType(fOut, "");
		fprintf(fOut, ")(");
		first->TranslateToC(fOut);
		fprintf(fOut, ").ptr + __nullcIndex(");
		if(second->typeInfo != typeInt)
			fprintf(fOut, "(unsigned)(");
		second->TranslateToC(fOut);
		if(second->typeInfo != typeInt)
			fprintf(fOut, ")");

		fprintf(fOut, ", (");
		first->TranslateToC(fOut);
		fprintf(fOut, ").size)");

		fprintf(fOut, ")");
	}else{
		fprintf(fOut, "&(");
		first->TranslateToC(fOut);
		fprintf(fOut, ")");
		fprintf(fOut, "->ptr");
		fprintf(fOut, "[__nullcIndex(");
		if(second->typeInfo != typeInt)
			fprintf(fOut, "(unsigned)(");
		second->TranslateToC(fOut);
		if(second->typeInfo != typeInt)
			fprintf(fOut, ")");
		fprintf(fOut, ", %uu)]", first->typeInfo->subType->arrSize);
	}
}
void NodeDereference::TranslateToC(FILE *fOut)
{
	TranslateToCExtra(fOut);

	if(closureFunc)
	{
		OutputIdent(fOut);
		assert(first->nodeType == typeNodeVariableSet);

		VariableInfo *closure = ((NodeOneOP*)first)->GetFirstNode()->nodeType == typeNodeGetAddress ? ((NodeGetAddress*)((NodeOneOP*)first)->GetFirstNode())->varInfo : NULL;
		if(closure)
			assert(closure->varType == first->typeInfo);

		char closureName[NULLC_MAX_VARIABLE_NAME_LENGTH + 32];
		const char *namePrefix = NULL;
		unsigned int nameShift = 0;
		if(closure)
		{
			namePrefix = *closure->name.begin == '$' ? "__" : "";
			nameShift = *closure->name.begin == '$' ? 1 : 0;
			SafeSprintf(closureName, NULLC_MAX_VARIABLE_NAME_LENGTH + 32, closure->blockDepth > 1 ? "%s%.*s_%d" : "%s%.*s", namePrefix, closure->name.length() - nameShift, closure->name.begin + nameShift, closure->pos);
			GetEscapedName(closureName);
		}else{
			closureName[0] = '\0';
		}

		if(closure)
		{
			fprintf(fOut, "(%s = (", closureName);
		}else{
			fprintf(fOut, "(*");
			((NodeOneOP*)first)->GetFirstNode()->TranslateToC(fOut);
			fprintf(fOut, " = (");

		}
		first->typeInfo->OutputCType(fOut, "");
		fprintf(fOut, ")__newS(%d, __nullcTR[%d])),\r\n", first->typeInfo->subType->size, first->typeInfo->subType->typeIndex);

		unsigned int pos = 0;
		for(FunctionInfo::ExternalInfo *curr = closureFunc->firstExternal; curr; curr = curr->next)
		{
			OutputIdent(fOut);

			if(closure)
			{
				fprintf(fOut, "(*(int**)((char*)%s + %d) = ", closureName, pos * 4);
			}else{
				fprintf(fOut, "(*(int**)((char*)(*");
				((NodeOneOP*)first)->GetFirstNode()->TranslateToC(fOut);
				fprintf(fOut, ") + %d) = ", pos * 4);
			}
			VariableInfo *varInfo = curr->variable;
			char variableName[NULLC_MAX_VARIABLE_NAME_LENGTH+32];
			if(varInfo->nameHash == GetStringHash("this"))
			{
				strcpy(variableName, "__context");
			}else{
				namePrefix = *varInfo->name.begin == '$' ? "__" : "";
				nameShift = *varInfo->name.begin == '$' ? 1 : 0;
				SafeSprintf(variableName, NULLC_MAX_VARIABLE_NAME_LENGTH+32, "%s%.*s_%d", namePrefix, int(varInfo->name.end-varInfo->name.begin) - nameShift, varInfo->name.begin + nameShift, varInfo->pos);
				GetEscapedName(variableName);
			}

			if(curr->targetPos == ~0u)
			{
				fprintf(fOut, "(int*)((char*)%s + %d))", closureName, pos * 4 + NULLC_PTR_SIZE);
			}else{
				if(curr->targetLocal)
				{
					fprintf(fOut, "(int*)&%s", variableName);
				}else{
					assert(closureFunc->parentFunc);
					fprintf(fOut, "*(int**)((char*)__%s_%d_ext_%d + %d)", closureFunc->parentFunc->name, CodeInfo::FindFunctionByPtr(closureFunc->parentFunc), closureFunc->parentFunc->allParamSize, (curr->targetPos >> 2) * 4);
				}
				fprintf(fOut, "),\r\n");
				OutputIdent(fOut);
				fprintf(fOut, "(*(int**)((char*)");
				if(closure)
				{
					fprintf(fOut, "%s", closureName);
				}else{
					fprintf(fOut, "(*");
					((NodeOneOP*)first)->GetFirstNode()->TranslateToC(fOut);
					fprintf(fOut, ")");
				}
				fprintf(fOut, " + %d) = (int*)__upvalue_%d_%s),\r\n", pos * 4 + NULLC_PTR_SIZE, CodeInfo::FindFunctionByPtr(varInfo->parentFunction), variableName);
				OutputIdent(fOut);
				fprintf(fOut, "(*(int*)((char*)");
				if(closure)
				{
					fprintf(fOut, "%s", closureName);
				}else{
					fprintf(fOut, "(*");
					((NodeOneOP*)first)->GetFirstNode()->TranslateToC(fOut);
					fprintf(fOut, ")");
				}
				fprintf(fOut, " + %d) = %d),\r\n", pos * 4 + 2 * NULLC_PTR_SIZE, curr->variable->varType->size);
				OutputIdent(fOut);
				fprintf(fOut, "(__upvalue_%d_%s = (__nullcUpvalue*)((int*)", CodeInfo::FindFunctionByPtr(varInfo->parentFunction), variableName);
				if(closure)
				{
					fprintf(fOut, "%s", closureName);
				}else{
					fprintf(fOut, "(*");
					((NodeOneOP*)first)->GetFirstNode()->TranslateToC(fOut);
					fprintf(fOut, ")");
				}
				fprintf(fOut, " + %d))", pos);
			}
			if(curr->next)
				fprintf(fOut, ",\r\n");
#ifdef _M_X64
			pos += ((varInfo->varType->size >> 2) < 4 ? 5 : 2 + (varInfo->varType->size >> 2));
#else
			pos += ((varInfo->varType->size >> 2) < 3 ? 3 : 1 + (varInfo->varType->size >> 2));
#endif
		}
		if(nodeDereferenceEndInComma)
			fprintf(fOut, ",\r\n");
		else
			fprintf(fOut, ";\r\n");
	}else{
		if(!neutralized)
			fprintf(fOut, "*(");
		first->TranslateToC(fOut);
		if(!neutralized)
			fprintf(fOut, ")");
	}
}

void NodeShiftAddress::TranslateToC(FILE *fOut)
{
	fprintf(fOut, "&(");
	first->TranslateToC(fOut);
	fprintf(fOut, ")->%s", member->name);
}

void NodePreOrPostOp::TranslateToC(FILE *fOut)
{
	if(head)
		fprintf(fOut, "(");
	NodeZeroOP *curr = head;
	while(curr)
	{
		assert(curr->nodeType == typeNodePopOp);
		((NodePopOp*)curr)->GetFirstNode()->TranslateToC(fOut);
		fprintf(fOut, ", ");
		curr = curr->next;
	}
	if(typeInfo == typeDouble || typeInfo == typeFloat)
	{
		if(optimised)
		{
			OutputIdent(fOut);
			fprintf(fOut, "*(");
			first->TranslateToC(fOut);
			fprintf(fOut, incOp ? ") += 1.0" : ") -= 1.0");
		}else{
			if(prefixOp)
			{
				fprintf(fOut, "(*(");
				first->TranslateToC(fOut);
				fprintf(fOut, incOp ? ") += 1.0)" : ") -= 1.0)");
			}else{
				fprintf(fOut, "((*(");
				first->TranslateToC(fOut);
				fprintf(fOut, incOp ? ") += 1.0) - 1.0)" : ") -= 1.0) + 1.0)");
			}
		}
	}else{
		if(optimised)
			OutputIdent(fOut);
		if(prefixOp)
			fprintf(fOut, incOp ? "++" : "--");

		fprintf(fOut, "(*(");
		first->TranslateToC(fOut);
		fprintf(fOut, "))");

		if(!prefixOp)
			fprintf(fOut, incOp ? "++" : "--");
	}
	if(head)
		fprintf(fOut, ")");
	if(optimised)
		fprintf(fOut, ";\r\n");
}

void NodeFunctionAddress::TranslateToC(FILE *fOut)
{
	fprintf(fOut, "(");
	nodeDereferenceEndInComma = true;
	TranslateToCExtra(fOut);
	nodeDereferenceEndInComma = false;
	OutputIdent(fOut);
	fprintf(fOut, "(");
	typeInfo->OutputCType(fOut, ")");
	fprintf(fOut, "__nullcMakeFunction(__nullcFR[%d], ", CodeInfo::FindFunctionByPtr(funcInfo));
	if(funcInfo->type == FunctionInfo::NORMAL)
	{
		fprintf(fOut, "(void*)%uu", funcInfo->funcPtr ? ~0u : 0u);
	}else{
		if(first->nodeType == typeNodeDereference)
		{
			VariableInfo *closure = ((NodeOneOP*)first)->GetFirstNode()->nodeType == typeNodeGetAddress ? ((NodeGetAddress*)((NodeOneOP*)first)->GetFirstNode())->varInfo : NULL;

			if(closure && closure->nameHash == GetStringHash("this"))
			{
				fprintf(fOut, "__context");
			}else if(closure){
				char closureName[NULLC_MAX_VARIABLE_NAME_LENGTH];
				const char *namePrefix = *closure->name.begin == '$' ? "__" : "";
				unsigned int nameShift = *closure->name.begin == '$' ? 1 : 0;
				sprintf(closureName, closure->blockDepth > 1 ? "%s%.*s_%d" : "%s%.*s", namePrefix, closure->name.length() - nameShift, closure->name.begin + nameShift, closure->pos);
				GetEscapedName(closureName);
				fprintf(fOut, "%s", closureName);
			}else{
				first->TranslateToC(fOut);
			}
		}else{
			first->TranslateToC(fOut);
		}
	}
	fprintf(fOut, "))");
}

void NodeBinaryOp::TranslateToC(FILE *fOut)
{
	TypeInfo *tmpType = ChooseBinaryOpResultType(first->typeInfo, second->typeInfo);
	
	if(cmdID == cmdPow)
		fprintf(fOut, "__nullcPow(");
	if(cmdID == cmdMod && typeInfo == typeDouble)
		fprintf(fOut, "__nullcMod(");

	if(tmpType != first->typeInfo)
	{
		fprintf(fOut, "(");
		tmpType->OutputCType(fOut, "");
		fprintf(fOut, ")");
	}
	fprintf(fOut, "(");
	if(cmdID == cmdLogXor)
		fprintf(fOut, "!!");
	first->TranslateToC(fOut);
	fprintf(fOut, ")");
	fprintf(fOut, " %s ", cmdID == cmdLogXor ? "!=" : ((cmdID == cmdPow || (cmdID == cmdMod && typeInfo == typeDouble)) ? "," : binCommandToText[cmdID-cmdAdd]));
	if(tmpType != second->typeInfo)
	{
		fprintf(fOut, "(");
		tmpType->OutputCType(fOut, "");
		fprintf(fOut, ")");
	}
	fprintf(fOut, "(");
	if(cmdID == cmdLogXor)
		fprintf(fOut, "!!");
	second->TranslateToC(fOut);

	fprintf(fOut, ")");
	if(cmdID == cmdPow || (cmdID == cmdMod && typeInfo == typeDouble))
		fprintf(fOut, ")");
}

void NodeIfElseExpr::TranslateToC(FILE *fOut)
{
	if(typeInfo != typeVoid)
	{
		if(first->typeInfo == typeObject)
			fprintf(fOut, "(");
		first->TranslateToC(fOut);
		if(first->typeInfo == typeObject)
			fprintf(fOut, ").ptr");
		fprintf(fOut, " ? ");
		second->TranslateToC(fOut);
		fprintf(fOut, " : ");
		third->TranslateToC(fOut);
	}else{
		OutputIdent(fOut);
		fprintf(fOut, "if(");
		if(first->typeInfo == typeObject)
			fprintf(fOut, "(");
		first->TranslateToC(fOut);
		if(first->typeInfo == typeObject)
			fprintf(fOut, ").ptr");
		fprintf(fOut, ")\r\n");
		OutputIdent(fOut);
		fprintf(fOut, "{\r\n");
		indentDepth++;
		second->TranslateToC(fOut);
		indentDepth--;
		if(third)
		{
			OutputIdent(fOut);
			fprintf(fOut, "}else{\r\n");
			indentDepth++;
			third->TranslateToC(fOut);
			indentDepth--;
		}
		OutputIdent(fOut);
		fprintf(fOut, "}\r\n");
	}
}

unsigned int	translateLoopDepth = 0;
const unsigned int TRANSLATE_MAX_LOOP_DEPTH = 64;
unsigned int	currLoopID[TRANSLATE_MAX_LOOP_DEPTH];

void NodeForExpr::TranslateToC(FILE *fOut)
{
	translateLoopDepth++;
	first->TranslateToC(fOut);
	OutputIdent(fOut); fprintf(fOut, "while(");
	if(second->typeInfo == typeObject)
		fprintf(fOut, "(");
	second->TranslateToC(fOut);
	if(second->typeInfo == typeObject)
		fprintf(fOut, ").ptr");
	fprintf(fOut, ")\r\n");
	OutputIdent(fOut); fprintf(fOut, "{\r\n");
	indentDepth++;
	fourth->TranslateToC(fOut);
	OutputIdent(fOut); fprintf(fOut, "continue%d_%d:1;\r\n", currLoopID[translateLoopDepth-1], translateLoopDepth);
	third->TranslateToC(fOut);
	indentDepth--;
	OutputIdent(fOut); fprintf(fOut, "}\r\n");
	OutputIdent(fOut); fprintf(fOut, "break%d_%d:1;\r\n", currLoopID[translateLoopDepth-1], translateLoopDepth);
	translateLoopDepth--;
	assert(translateLoopDepth < TRANSLATE_MAX_LOOP_DEPTH);
	currLoopID[translateLoopDepth]++;
}

void NodeWhileExpr::TranslateToC(FILE *fOut)
{
	translateLoopDepth++;
	OutputIdent(fOut); fprintf(fOut, "while(");
	if(first->typeInfo == typeObject)
		fprintf(fOut, "(");
	first->TranslateToC(fOut);
	if(first->typeInfo == typeObject)
		fprintf(fOut, ").ptr");
	fprintf(fOut, ")\r\n");
	OutputIdent(fOut); fprintf(fOut, "{\r\n");
	indentDepth++;
	second->TranslateToC(fOut);
	OutputIdent(fOut); fprintf(fOut, "continue%d_%d:1;\r\n", currLoopID[translateLoopDepth-1], translateLoopDepth);
	indentDepth--;
	OutputIdent(fOut); fprintf(fOut, "}\r\n");
	OutputIdent(fOut); fprintf(fOut, "break%d_%d:1;\r\n", currLoopID[translateLoopDepth-1], translateLoopDepth);
	translateLoopDepth--;
	assert(translateLoopDepth < TRANSLATE_MAX_LOOP_DEPTH);
	currLoopID[translateLoopDepth]++;
}

void NodeDoWhileExpr::TranslateToC(FILE *fOut)
{
	translateLoopDepth++;
	OutputIdent(fOut); fprintf(fOut, "do\r\n");
	OutputIdent(fOut); fprintf(fOut, "{\r\n");
	indentDepth++;
	first->TranslateToC(fOut);
	OutputIdent(fOut); fprintf(fOut, "continue%d_%d:1;\r\n", currLoopID[translateLoopDepth-1], translateLoopDepth);
	indentDepth--;
	OutputIdent(fOut); fprintf(fOut, "} while(");
	if(second->typeInfo == typeObject)
		fprintf(fOut, "(");
	second->TranslateToC(fOut);
	if(second->typeInfo == typeObject)
		fprintf(fOut, ").ptr");
	fprintf(fOut, ");\r\n");
	OutputIdent(fOut);
	fprintf(fOut, "break%d_%d:1;\r\n", currLoopID[translateLoopDepth-1], translateLoopDepth);
	translateLoopDepth--;
	assert(translateLoopDepth < TRANSLATE_MAX_LOOP_DEPTH);
	currLoopID[translateLoopDepth]++;
}

void NodeBreakOp::TranslateToC(FILE *fOut)
{
	OutputIdent(fOut);
	if(breakDepth == 1)
		fprintf(fOut, "break;\r\n");
	else
		fprintf(fOut, "goto break%d_%d;\r\n", currLoopID[translateLoopDepth-breakDepth], translateLoopDepth - breakDepth + 1);
}

void NodeContinueOp::TranslateToC(FILE *fOut)
{
	OutputIdent(fOut);
	fprintf(fOut, "goto continue%d_%d;\r\n", currLoopID[translateLoopDepth-continueDepth], translateLoopDepth - continueDepth + 1);
}

void NodeSwitchExpr::TranslateToC(FILE *fOut)
{
	static int switchNum = 0;
	int myNum = switchNum++;
	OutputIdent(fOut);
	fprintf(fOut, "do\r\n");
	OutputIdent(fOut);
	fprintf(fOut, "{\r\n");
	indentDepth++;
	char buf[64];
	sprintf(buf, "switchVar");
	OutputIdent(fOut);
	first->typeInfo->OutputCType(fOut, buf);
	fprintf(fOut, " = ");
	first->TranslateToC(fOut);
	fprintf(fOut, ";\r\n");
	unsigned int i = 0;
	for(NodeZeroOP *curr = conditionHead; curr; curr = curr->next, i++)
	{
		OutputIdent(fOut);
		fprintf(fOut, "if(switchVar == ");
		curr->TranslateToC(fOut);
		fprintf(fOut, ")\r\n");
		OutputIdent(fOut);
		fprintf(fOut, "\tgoto case%d_%d;\r\n", myNum, i);
	}
	OutputIdent(fOut);
	fprintf(fOut, "goto defaultCase_%d;\r\n", myNum);
	i = 0;
	for(NodeZeroOP *block = blockHead; block; block = block->next, i++)
	{
		OutputIdent(fOut);
		fprintf(fOut, "case%d_%d:\r\n", myNum, i);
		block->TranslateToC(fOut);
	}
	OutputIdent(fOut);
	fprintf(fOut, "defaultCase_%d:\r\n", myNum);
	if(defaultCase)
	{
		defaultCase->TranslateToC(fOut);
	}else{
		OutputIdent(fOut);
		fprintf(fOut, "0;\r\n");
	}
	indentDepth--;
	OutputIdent(fOut);
	fprintf(fOut, "}while(0);\r\n");
}

void NodeExpressionList::TranslateToC(FILE *fOut)
{
	if(typeInfo->arrLevel && typeInfo->arrSize != TypeInfo::UNSIZED_ARRAY && typeInfo->subType == typeChar)
	{
		fprintf(fOut, "\"");
		NodeZeroOP	*curr = tail->prev;
		bool prevEscaped = false;
		do 
		{
			assert(curr->nodeType == typeNodeNumber);
			NodeNumber *dword = (NodeNumber*)curr;
			for(unsigned int i = 0; i < 4; i++)
			{
				unsigned char ch = (unsigned char)((dword->GetInteger() >> (i * 8)) & 0xff);
				if(ch >= ' ' && ch <= 128 && ch != '\"' && ch != '\\' && ch != '\'')
				{
					if(prevEscaped)
						fprintf(fOut, "\"\"");
					prevEscaped = false;
					fprintf(fOut, "%c", ch);
				}else{
					prevEscaped = true;
					fprintf(fOut, "\\x%x", ch);
				}
			}
			curr = curr->prev;
		}while(curr);
		fprintf(fOut, "\"");
		return;
	}else if(typeInfo != typeVoid){
		NodeZeroOP *end = first;
		if(first->nodeType == typeNodePopOp)// || !typeInfo->arrLevel)
		{
			fprintf(fOut, "(");
			((NodePopOp*)first)->GetFirstNode()->TranslateToC(fOut);
			fprintf(fOut, ", ");
			end = first->next;
		}else if(first->nodeType == typeNodeFuncDef){
			first->TranslateToC(fOut);
			end = first->next;
		}else if(!typeInfo->arrLevel && typeInfo != typeAutoArray){
			fprintf(fOut, "(");
		}
		if(tail->prev != end->prev && typeInfo->arrLevel && typeInfo->arrSize == TypeInfo::UNSIZED_ARRAY)
		{
			fprintf(fOut, "__makeNullcArray<");
			typeInfo->subType->OutputCType(fOut, "");
			fprintf(fOut, ">(");
		}else if(typeInfo->arrLevel && typeInfo->arrSize != TypeInfo::UNSIZED_ARRAY){
			typeInfo->OutputCType(fOut, "()");
			end = first->next;
		}else if(typeInfo == typeAutoArray){
			fprintf(fOut, "__makeAutoArray(");
		}

		NodeZeroOP	*curr = tail;
		unsigned int id = 0;
		do 
		{
			if(typeInfo->arrLevel && typeInfo->arrSize != TypeInfo::UNSIZED_ARRAY)
				fprintf(fOut, ".set(%d, ", id++);
			curr->TranslateToC(fOut);
			if(typeInfo->arrLevel && typeInfo->arrSize != TypeInfo::UNSIZED_ARRAY)
				fprintf(fOut, ")");
			else if(curr != end)
				fprintf(fOut, ", ");
			curr = curr->prev;
		}while(curr != end->prev);

		if(tail->prev != end->prev && typeInfo->arrLevel && typeInfo->arrSize == TypeInfo::UNSIZED_ARRAY)
			fprintf(fOut, ")");

		if(first->nodeType == typeNodePopOp || !typeInfo->arrLevel)// || typeInfo == typeAutoArray)
			fprintf(fOut, ")");
	}else{
		NodeZeroOP	*curr = first;
		do 
		{
			curr->TranslateToC(fOut);
			if(curr != tail && typeInfo != typeVoid)
				fprintf(fOut, ", ");
			curr = curr->next;
		}while(curr);
	}
}

void NodeFunctionProxy::TranslateToC(FILE *fOut)
{
	(void)fOut;
}

void NodePointerCast::TranslateToC(FILE *fOut)
{
	TranslateToCExtra(fOut);

	first->TranslateToCExtra(fOut);
}

void NodeGetFunctionContext::TranslateToC(FILE *fOut)
{
	TranslateToCExtra(fOut);

	fprintf(fOut, "*((int**)");
	first->TranslateToCExtra(fOut);
	fprintf(fOut, ")");
}

void NodeGetCoroutineState::TranslateToC(FILE *fOut)
{
	TranslateToCExtra(fOut);

	fprintf(fOut, "**((int**)");
	first->TranslateToCExtra(fOut);
	fprintf(fOut, ")");
}

void NodeCreateUnsizedArray::TranslateToC(FILE *fOut)
{
	TranslateToCExtra(fOut);

	fprintf(fOut, "__makeNullcArray<");
	typeInfo->subType->OutputCType(fOut, "");
	fprintf(fOut, ">(");
	first->TranslateToCExtra(fOut);
	fprintf(fOut, ", ");
	second->TranslateToCExtra(fOut);
	fprintf(fOut, ")");
}

void NodeCreateAutoArray::TranslateToC(FILE *fOut)
{
	TranslateToCExtra(fOut);

	fprintf(fOut, "__makeNullcAutoArray<");
	typeInfo->subType->OutputCType(fOut, "");
	fprintf(fOut, ">(");
	first->TranslateToCExtra(fOut);
	fprintf(fOut, ", ");
	second->TranslateToCExtra(fOut);
	fprintf(fOut, ")");
}

void ResetTranslationState()
{
	translateLoopDepth = 0;
	memset(currLoopID, 0, sizeof(unsigned int) * TRANSLATE_MAX_LOOP_DEPTH);
	nodeDereferenceEndInComma = false;
}

#else
void	NULLC_PreventTranslateWarning(){}
#endif
