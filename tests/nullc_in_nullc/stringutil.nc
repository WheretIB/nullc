int memcmp(char[] a, int offset, char[] b, int len)
{
	for(int i = 0; i < len; i++)
	{
		int d = a[i + offset] - b[i];
		
		if(d != 0)
			return d;
	}
	
	return 0;
}

int GetStringHash(char[] data)
{
	int hash = 5381;

	for(i in data)
	{
		if(!i)
			break;

		hash = ((hash << 5) + hash) + i;
	}

	return hash;
}

int GetStringHash(char[] data, int begin, int end)
{
	int hash = 5381;
	for(int i = begin; i < end; i++)
		hash = ((hash << 5) + hash) + data[i];
	return hash;
}

int StringHashContinue(int hash, char[] str)
{
	for(i in str)
	{
		if(!i)
			break;
			
		hash = ((hash << 5) + hash) + i;
	}

	return hash;
}

class StringRef
{
	void StringRef(char[] string, int pos)
	{
		this.string = string;
		this.pos = pos;
	}

	char curr()
	{
		return string[pos];
	}

	void advance()
	{
		pos++;
	}

	void rewind()
	{
		assert(pos > 0);
		pos--;
	}

	void advance(char ch)
	{
		string[pos] = ch;
		pos++;
	}

	int length()
	{
		assert(string != nullptr);

		return (string.size - 1) - pos;
	}

	char[] string;
	int pos;
}

char ref operator[](StringRef ptr, int index)
{
	return &ptr.string[ptr.pos + index];
}

bool operator<(StringRef lhs, StringRef rhs)
{
	assert(lhs.string == rhs.string);

	return lhs.pos < rhs.pos;
}

bool operator==(StringRef lhs, StringRef rhs)
{
	assert(lhs.string == rhs.string);

	return lhs.pos == rhs.pos;
}

bool operator!=(StringRef lhs, StringRef rhs)
{
	assert(lhs.string == rhs.string);

	return lhs.pos != rhs.pos;
}

StringRef operator+(StringRef lhs, int offset)
{
	assert(lhs.string.size > lhs.pos + offset, "string reference overflow");

	StringRef result = lhs;
	result.pos += offset;
	return result;
}

StringRef operator-(StringRef lhs, int offset)
{
	assert(lhs.pos >= offset, "string reference underflow");

	StringRef result = lhs;
	result.pos -= offset;
	return result;
}

class InplaceStr
{
	void InplaceStr(char[] data)
	{
		this.data = data;
		this.begin = 0;
		this.end = 0;

		for(i in data)
		{
			if(i == 0)
				break;

			this.end++;
		}
	}
	
	void InplaceStr(char[] data, int begin, int end)
	{
		assert(end >= begin);
		
		this.data = data;
		this.begin = begin;
		this.end = end;
	}

	void InplaceStr(StringRef string, int length)
	{
		this.data = string.string;
		this.begin = string.pos;
		this.end = string.pos + length;
	}
	
	int hash()
	{
		return GetStringHash(data, begin, end);
	}
	
	int length()
	{
		return end - begin;
	}
	
	bool empty()
	{
		return begin == end;
	}
	
	char[] data;
	int begin, end;
}

char operator[](InplaceStr str, int index)
{
	return str.data[str.begin + index];
}

bool bool(InplaceStr str)
{
	return str.data != nullptr;
}

int hash_value(InplaceStr key)
{
	return key.hash();
}

int StringHashContinue(int hash, InplaceStr str)
{
	for(int i = str.begin; i < str.end; i++)
		hash = ((hash << 5) + hash) + str.data[i];

	return hash;
}

bool operator==(InplaceStr a, InplaceStr b)
{
	int aLen = a.end - a.begin;
	int bLen = b.end - b.begin;
	
	if(aLen != bLen)
		return false;
		
	for(int i = 0; i < aLen; i++)
	{
		if(a.data[a.begin + i] != b.data[b.begin + i])
			return false;
	}
	
	return true;
}

bool operator!=(InplaceStr a, InplaceStr b)
{
	return !(a== b);
}

void StringRef:StringRef(InplaceStr str)
{
	this.string = str.data;
	this.pos = str.begin;
}

StringRef InplaceStr.begin_ref()
{
	return StringRef(data, begin);
}

StringRef InplaceStr.end_ref()
{
	return StringRef(data, end);
}

InplaceStr FMT_ISTR(InplaceStr str)
{
	return str;
}
