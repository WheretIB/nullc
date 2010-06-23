
void Print(char[] text);
void Print(int num);
void Print(double num);
void Print(long num);
void Print(char ch);

int Input(char[] buf);
void Input(int ref num);

void Write(char[] buf);

void SetConsoleCursorPos(int x, y);

void GetKeyboardState(char[] state);
void GetMouseState(int ref x, int ref y);

class StdOut{}
class StdEndline{}
class StdNonTerminatedTag{}
class StdIO
{
	StdOut out;
	StdEndline endl;
	StdNonTerminatedTag non_terminated_tag;
	auto non_terminated(char[] x)
	{
		return auto(StdNonTerminatedTag y){ return x; };
	}
}
StdIO io;

StdOut operator <<(StdOut out, char[] ref(StdNonTerminatedTag) wrapper)
{
	Write(wrapper(io.non_terminated_tag));
	return out;
}
StdOut operator <<(StdOut out, char[] str)
{
	Print(str);
	return out;
}
StdOut operator <<(StdOut out, const_string str)
{
	Print(str.arr);
	return out;
}
StdOut operator <<(StdOut out, StdEndline str)
{
	Print("\r\n");
	return out;
}
StdOut operator <<(StdOut out, char ch)
{
	Print(ch);
	return out;
}
StdOut operator <<(StdOut out, int num)
{
	Print(num);
	return out;
}
StdOut operator <<(StdOut out, double num)
{
	Print(num);
	return out;
}
StdOut operator <<(StdOut out, long num)
{
	Print(num);
	return out;
}
