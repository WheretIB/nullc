// std.string

// C-compatible set of functions to use on null-terminated character arrays

// Find length of a null-terminated character array 'string'
// Error is generated if array is empty of not null-terminated
int strlen(char[] string);

// Find occurance of a null-terminated character array 'substring' inside null-terminated character array 'string'
// Result is either the position in the array or -1
// Error is generated if array is empty of not null-terminated
int strstr(char[] string, substring);

// Find occurance of character 'ch' inside null-terminated character array 'string'
// If 'ch' is '\0', result is the position of the null-terminating character in the array
// Result is either the position in the array or -1
// Error is generated if array is empty of not null-terminated
int strchr(char[] string, char ch);

// Compare and find relation between null-terminated character arrays 'a' and 'b'
// Return value is the lexographical ordering of the null-terminated character arrays
// -1 if 'a' comes before 'b'
// 0 if 'a' is equal o 'b'
// 1 if 'a' comes after 'b'
// Error is generated if either array is empty of not null-terminated
int strcmp(char[] a, b);

// Copy null-terminated character array 'src' into character array 'dst'
// Return value is the length of a null-terminated character array 'dst'
// Error is generated if either array is empty of not null-terminated
int strcpy(char[] dst, src);

int luamatch(char[] str, pattern, int offset = 0, int max_size=0);

// String class to use instead of error-prone raw character arrays
class string
{
	char[] data;
}

// Empty constructor
void string::string()
{
}

// Basic constructor
void string::string(char[] right)
{
	if(right.size == 0)
		return;

	// null-terminated source
	if(right[right.size - 1] == 0)
	{
		int len = strlen(right);

		if(len == 0)
			return;

		data = new char[len + 1];
		strcpy(data, right);
	}
	else
	{
		data = new char[right.size + 1];

		for(i in data, j in right)
			i = j;
	}
}

// Fill constructor
void string::string(int count, char ch)
{
	assert(count >= 0);

	if(count > 0)
	{
		data = new char[count + 1];
		for(int i = 0; i < count; i++)
			data[i] = ch;
	}
}

// Copy constructor
void string::string(string ref right)
{
	if(right.data)
		this.data = duplicate(right.data);
}

// Assignment operator
string ref operator=(string ref left, string ref right)
{
	if(right.data)
		left.data = duplicate(right.data);
	else
		left.data = nullptr;
	return left;
}

string ref operator=(string ref left, char[] right)
{
	if(right.size == 0)
	{
		left.data = nullptr;
		return left;
	}

	// null-terminated source
	if(right[right.size - 1] == 0)
	{
		int len = strlen(right);

		if(len == 0)
		{
			left.data = nullptr;
			return left;
		}

		left.data = new char[len + 1];
		strcpy(left.data, right);
	}
	else
	{
		left.data = new char[right.size + 1];

		for(i in left.data, j in right)
			i = j;
	}

	return left;
}

string ref operator=(string ref left, char right)
{
	left.data = new char[2];
	left.data[0] = right;
	return left;
}

// Length of the string excluding the terminating null-character
int string.size()
{
	return data.size ? data.size - 1 : 0;
}

// Length of the string excluding the terminating null-character
int string::length()
{
	return data.size ? data.size - 1 : 0;
}

// Clears the string
void string::clear()
{
	data = nullptr;
}

// Checks that the string is empty
bool string::empty()
{
	return data.size <= 1;
}

// return a single character at the specified index
char ref operator[](string ref left, int index)
{
	return &left.data[index];
}

// Returns a substring of [start, end] array elements
string operator[](string ref left, int start, int end)
{
	assert(start == 0 || (start > 0 && start < left.data.size - 1));
	assert(end == 0 || (end > 0 && end < left.data.size - 1));
	assert(start <= end);
	
	char[] buf = new char[end - start + 2];
	
	for(int i = start; i <= end; i++)
		buf[i - start] = left.data[i];
	
	string res;
	res.data = buf;
	return res;
}

// First character
char ref string::front()
{
	return &data[0];
}

// Last character not counting the null-character
char ref string::back()
{
	return &data[data.size - 2];
}

// Appends the string at the end
string ref operator+=(string ref left, string ref right)
{
	left.data = left.data + right.data;
	return left;
}

string ref operator+=(string ref left, char[] right)
{
	left.data = left.data + right;
	return left;
}

// Inserts a string before the specified character
string ref string::insert(int offset, string ref str)
{
	// No change on attempt to insert empty string
	if(str.size == 0)
		return this;

	assert(offset == 0 || (offset > 0 && offset < data.size));
	
	// Replace the whole string if it's empty
	if(size == 0)
	{
		data = duplicate(str.data);
		return this;
	}
	
	auto buf = new char[data.size + str.data.size - 1];
	
	for(int i = 0; i < offset; i++)
		buf[i] = data[i];
	for(int i = 0; i < str.data.size - 1; i++)
		buf[i + offset] = str.data[i];
	for(int i = offset; i < data.size - 1; i++)
		buf[i + str.data.size - 1] = data[i];
		
	data = buf;
	return this;
}

string ref string::insert(int offset, char[] str)
{
	return this.insert(offset, string(str));
}

string ref string::insert(int offset, int count, char ch)
{
	return this.insert(offset, string(count, ch));
}

// If the length is specified, erases the selected number of characters starting from the specified character
// If the length is not specified, erases all the characters starting from the specified character
string ref string::erase(int offset, int length = -1)
{
	if(length < 0 || length > data.size - offset)
		length = data.size - offset - 1;
	if(length == 0)
		return this;

	assert(offset == 0 || (offset > 0 && offset < data.size));
	
	// Do not do anything while erasing from empty string
	if(size == 0)
		return this;
	
	auto buf = new char[data.size - length];
	
	for(int i = 0; i < offset; i++)
		buf[i] = data[i];
	for(int i = offset + length; i < data.size; i++)
		buf[i - length] = data[i];
	
	data = buf;
	return this;
}

// Erases the selected number of characters starting from the specified character and replaces them with a specified string 
string ref string::replace(int offset, int length, string ref str)
{
	assert(offset == 0 || (offset > 0 && offset < data.size));

	if(length < 0 || length >= data.size - offset)
		length = data.size - offset - 1;

	auto buf = new char[data.size - length + str.data.size - 1];
	
	for(int i = 0; i < offset; i++)
		buf[i] = data[i];

	for(int i = 0; i < str.data.size - 1; i++)
		buf[i + offset] = str.data[i];

	for(int i = offset + length; i < data.size - 1; i++)
		buf[i - length + str.data.size - 1] = data[i];

	data = buf;
	return this;
}

string ref string::replace(int offset, int length, char[] str)
{
	return replace(offset, length, string(str));
}

string ref string::replace(int offset, int length, int count, char ch)
{
	return this.replace(offset, length, string(count, ch));
}

// Swap string data with another string
void string::swap(string ref right)
{
	char[] tmp = data;
	data = right.data;
	right.data = tmp;
}

void string::swap(char[] right)
{
	data = right;
}

// Returns the position of the first occurance of the specified string or -1 if it is not found
// Optional offset specifies how many characters to skip from the start of the string during the search
int string::find(string ref str, int offset = 0)
{
	int last = data.size - str.data.size + 1;

	for(int i = offset; i < last; i++)
	{
		bool wrong = false;
		for(int k = 0; k < str.data.size - 1 && !wrong; k++)
		{
			if(data[i + k] != str.data[k])
				wrong = true;
		}
		if(!wrong)
			return i;
	}

	return -1;
}

int string::find(char[] str, int offset = 0)
{
	return find(string(str), offset);
}

int string::find(char ch, int offset = 0)
{
	for(int i = offset; i < data.size - 1; i++)
	{
		if(data[i] == ch)
			return i;
	}

	return -1;
}

// Returns the position of the last occurance of the specified string or -1 if it is not found
// Optional offset specifies how many characters to consider in the search counting from the beginning of the string including the character at the specified offset
int string::rfind(string ref str, int offset = -1)
{
	if(str.data.size == 0)
		return offset >= 0 && offset < data.size ? offset : size;

	if(offset < 0 || offset >= data.size - 1)
		offset = data.size - 2;

	for(int i = offset; i >= 0; i--)
	{
		bool wrong = false;
		for(int k = 0; k < str.data.size - 1 && !wrong; k++)
		{
			if(data[i + k] != str.data[k])
				wrong = true;
		}
		if(!wrong)
			return i;
	}

	return -1;
}

int string::rfind(char[] str, int offset = -1)
{
	return rfind(string(str), offset);
}

int string::rfind(char ch, int offset = -1)
{
	if(offset < 0 || offset >= data.size - 1)
		offset = data.size - 2;
		
	for(int i = offset; i >= 0; i--)
	{
		if(data[i] == ch)
			return i;
	}

	return -1;
}

// Returns the position of the first occurance of any of the specified characters in a string or -1 if not a single match is found
// Optional offset specifies how many characters to skip from the start of the string during the search
int string::find_first_of(string ref str, int offset = 0)
{
	for(int i = offset; i < data.size - 1; i++)
	{
		for(int k = 0; k < str.data.size - 1; k++)
		{
			if(data[i] == str.data[k])
				return i;
		}
	}

	return -1;
}

int string::find_first_of(char[] str, int offset = 0)
{
	return find_first_of(string(str), offset);
}

int string::find_first_of(char ch, int offset = 0)
{
	return find(ch, offset);
}

// Returns the position of the last occurance of any of the specified characters in a string or -1 if not a single match is found
// Optional offset specifies how many characters to consider in the search counting from the beginning of the string including the character at the specified offset
int string::find_last_of(string ref str, int offset = -1)
{
	if(offset < 0 || offset >= data.size - 1)
		offset = data.size - 2;

	for(int i = offset; i >= 0; i--)
	{
		for(int k = 0; k < str.data.size - 1; k++)
		{
			if(data[i] == str.data[k])
				return i;
		}
	}

	return -1;
}

int string::find_last_of(char[] str, int offset = -1)
{
	return find_last_of(string(str), offset);
}

int string::find_last_of(char ch, int offset = -1)
{
	return rfind(ch, offset);
}

// Returns the position of the first occurance of a character that doesn't match any of the specified characters in a string or -1 if a match is found
// Optional offset specifies how many characters to skip from the start of the string during the search
int string::find_first_not_of(string ref str, int offset = 0)
{
	for(int i = offset; i < data.size - 1; i++)
	{
		bool found = false;
		for(int k = 0; k < str.data.size - 1 && !found; k++)
		{
			if(data[i] == str.data[k])
				found = true;
		}
		if(!found)
			return i;
	}

	return -1;
}

int string::find_first_not_of(char[] str, int offset = 0)
{
	return find_first_not_of(string(str), offset);
}

int string::find_first_not_of(char ch, int offset = 0)
{
	for(int i = offset; i < data.size - 1; i++)
	{
		if(data[i] != ch)
			return i;
	}

	return -1;
}

// Returns the position of the last occurance of a character that doesn't match any of the specified characters in a string or -1 if a match is found
// Optional offset specifies how many characters to consider in the search counting from the beginning of the string including the character at the specified offset
int string::find_last_not_of(string ref str, int offset = -1)
{
	if(offset < 0 || offset >= data.size - 1)
		offset = data.size - 2;

	for(int i = offset; i >= 0; i--)
	{
		bool found = false;
		for(int k = 0; k < str.data.size - 1 && !found; k++)
		{
			if(data[i] == str.data[k])
				found = true;
		}
		if(!found)
			return i;
	}

	return -1;
}

int string::find_last_not_of(char[] str, int offset = -1)
{
	return find_last_not_of(string(str), offset);
}

int string::find_last_not_of(char ch, int offset = -1)
{
	if(offset < 0 || offset >= data.size - 1)
		offset = data.size - 2;
		
	for(int i = offset; i >= 0; i--)
	{
		if(data[i] != ch)
			return i;
	}

	return -1;
}

// Checks if the string contains the specified character
bool operator in(char ch, string ref str)
{
	return str.find(ch) != -1;
}

// Returns a substring of [start, start + length) array elements
string string::substr(int start, int length = -1)
{
	if(length < 0 || start + length >= data.size)
		length = data.size - start - 1;
	if(length <= 0)
		return string();
	
	return this[start, start + length - 1];
}

// Concatenate two strings together
string operator+(string ref left, string ref right)
{
	if(!left.data.size)
		return string(right);
	if(!right.data.size)
		return string(left);

	return string(left.data + right.data);
}

string operator+(char[] left, string ref right)
{
	return string(left) + right;
}

string operator+(string ref left, char[] right)
{
	return left + string(right);
}

// Compare strings for equality
bool operator==(string ref left, string ref right)
{
	if(left.size != right.size)
		return false;

	return left.data == right.data;
}

bool operator==(string ref left, char[] right)
{
	if(right.size > 1)
		return strcmp(left.data, right) == 0;

	return left.size == 0;
}

bool operator==(char[] left, string ref right)
{
	if(left.size > 1)
		return strcmp(left, right.data) == 0;

	return right.size == 0;
}

// Compare strings for inequality
bool operator!=(string ref left, string ref right)
{
	if(left.size != right.size)
		return true;
	
	return left.data != right.data;
}

bool operator!=(string ref left, char[] right)
{
	if(right.size > 1)
		return strcmp(left.data, right) != 0;
	
	return left.size != 0;
}

bool operator!=(char[] left, string ref right)
{
	if(left.size > 1)
		return strcmp(left, right.data) != 0;
	
	return right.size != 0;
}

// String ordering
bool operator<(string ref left, string ref right)
{
	return strcmp(left.data, right.data) < 0;
}

bool operator<(string ref left, char[] right)
{
	return strcmp(left.data, right) < 0;
}

bool operator<(char[] left, string ref right)
{
	return strcmp(left, right.data) < 0;
}

bool operator<=(string ref left, string ref right)
{
	return strcmp(left.data, right.data) <= 0;
}

bool operator<=(string ref left, char[] right)
{
	return strcmp(left.data, right) <= 0;
}

bool operator<=(char[] left, string ref right)
{
	return strcmp(left, right.data) <= 0;
}

bool operator>(string ref left, string ref right)
{
	return strcmp(left.data, right.data) > 0;
}

bool operator>(string ref left, char[] right)
{
	return strcmp(left.data, right) > 0;
}

bool operator>(char[] left, string ref right)
{
	return strcmp(left, right.data) > 0;
}

bool operator>=(string ref left, string ref right)
{
	return strcmp(left.data, right.data) >= 0;
}

bool operator>=(string ref left, char[] right)
{
	return strcmp(left.data, right) >= 0;
}

bool operator>=(char[] left, string ref right)
{
	return strcmp(left, right.data) >= 0;
}
