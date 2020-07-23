// Stop execution with an error message
void throw(char[] message);

// Stop execution with an exception object
void throw(auto ref exception);

// Execute a function while catching execution errors
// Return value indicates success
bool try(auto ref function, explicit char[] ref errorMessage, explicit auto ref ref errorObject, explicit auto ref ref result);

class try_result<T>
{
	char[] message;
	auto ref exception;

	@if(T != void)
	{
		T value;
	}
}

bool bool(try_result x)
{
	return x.message == nullptr;
}

bool operator!(try_result x)
{
	return x.message != nullptr;
}

bool try_result.has_error()
{
	return message != nullptr;
}

void try_result:rethrow()
{
	if(exception)
		throw(exception);

	if(message)
		throw(message);

	assert(false, "result has no error");
}

// Execute a function while catching execution errors
// Return value can contain an error message (and an optional exception object) or a value if the function return type is not 'void'
// Return value can be used as a condition
auto try(@T ref() function)
{
	try_result<T> result;

	auto ref anyResult;

	if(try(function, &result.message, &result.exception, &anyResult))
	{
		@if(T.isReference)
			result.value = *T ref(anyResult);
		else if(T != void)
			result.value = anyResult;
	}

	return result;
}
