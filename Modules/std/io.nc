
void Print(char[] text);
void Print(int num);
void Print(double num);
void Print(long num);
void Print(char ch);

int Input(char[] buf);
void Input(int ref num);

void SetConsoleCursorPos(int x, y);

void GetKeyboardState(char[] state);
void GetMouseState(int ref x, int ref y);

class StdOut{}
class StdEndline{}
class StdIO
{
	StdOut out;
	StdEndline endl;
}
StdIO io;

StdOut operator <<(StdOut out, char[] str)
{
	Print(str);
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
