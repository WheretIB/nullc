#include "error.h"

#include "../../NULLC/nullc.h"
#include "../../NULLC/nullbind.h"
#include "../../NULLC/nullc_debug.h"

namespace NULLCError
{
	void throw_0(NULLCArray message)
	{
		nullcThrowError("%.*s", message.len, message.ptr);
	}

	void throw_1(NULLCRef exception)
	{
		nullcThrowErrorObject(exception);
	}

	bool try_0(NULLCRef function, NULLCArray* message, NULLCRef* exception, NULLCRef* result)
	{
		if(!function.ptr)
		{
			nullcThrowError("ERROR: null pointer access");
			return false;
		}

		ExternTypeInfo *exTypes = nullcDebugTypeInfo(NULL);

		if(exTypes[function.typeID].subCat != ExternTypeInfo::CAT_FUNCTION)
		{
			nullcThrowError("ERROR: argument is not a function");
			return false;
		}

		if(exTypes[function.typeID].memberCount > 1)
		{
			nullcThrowError("ERROR: can't call function with arguments");
			return false;
		}

		NULLCFuncPtr functionValue = *(NULLCFuncPtr*)function.ptr;

		if(!nullcCallFunction(functionValue))
		{
			if(message)
			{
				const char *rawMessage = nullcGetLastError();
				unsigned rawLength = unsigned(strlen(rawMessage));

				*message = nullcAllocateArrayTyped(NULLC_TYPE_CHAR, rawLength + 1);
				strcpy(message->ptr, rawMessage);
			}

			*exception = nullcGetLastErrorObject();

			nullcClearError();
			return false;
		}

		if(result)
			*result = nullcGetResultObject();

		return true;
	}
}

#define REGISTER_FUNC(funcPtr, name, index) if(!nullcBindModuleFunctionHelper("std.error", NULLCError::funcPtr, name, index)) return false;

bool nullcInitErrorModule()
{
	REGISTER_FUNC(throw_0, "throw", 0);
	REGISTER_FUNC(throw_1, "throw", 1);

	REGISTER_FUNC(try_0, "try", 0);

	return true;
}
