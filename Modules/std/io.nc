
void Print(char[] text);
void Print(int num, int base = 10);
void Print(double num);
void Print(long num, int base = 10);
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
class StdBase{ int base; }
class StdIO
{
	StdOut out;
	StdEndline endl;
	StdNonTerminatedTag non_terminated_tag;
	StdBase bin, oct, dec, hex;
	StdBase currBase;
	auto non_terminated(char[] x)
	{
		return auto(StdNonTerminatedTag y){ return x; };
	}
	auto base(int base)
	{
		assert(base > 1 && base <= 16);
		StdBase n;
		n.base = base;
		return n;
	}
}
StdIO io;
io.bin.base = 2;
io.oct.base = 8;
io.dec.base = 10;
io.hex.base = 16;
io.currBase = io.dec;

StdOut operator <<(StdOut out, StdBase base){ io.currBase = base; return out; }
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
	Print(num, io.currBase.base);
	return out;
}
StdOut operator <<(StdOut out, double num)
{
	Print(num);
	return out;
}
StdOut operator <<(StdOut out, long num)
{
	Print(num, io.currBase.base);
	return out;
}
