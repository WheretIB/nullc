// Module std.io

// Function prints the contents of an array, stopping at the first zero-termination character or at the end of the array
// nullptr arrays are ignored
void Print(char[] text);

// Function prints integer number in the optionally specified base (10 is the default)
void Print(int num, int base = 10);

// Function prints floating-point number with the optionally specified precision (12 is the default)
void Print(double num, int precision = 12);

// Function prints long integer number in the optionally specified base (10 is the default)
void Print(long num, int base = 10);

// Function prints a single character
void Print(char ch);

// Function receives user input and places it in the array with a zero-termination character at the end
// buf must not be a nullptr array
// return value is the number of actual characters placed in the array excluding the zero-termination character
int Input(char[] buf);

// Function receives an integer from the user input
// num must not be a nullptr reference
void Input(int ref num);

// Function writes all the characters in the array to the output
// buf must not be a nullptr array
void Write(char[] buf);

// Function will set the curson position in the console (Windows-only)
// x should not be negative
// y should not be negative
void SetConsoleCursorPos(int x, y);

// Virtual key codes
enum VK
{
	ZERO,
	LBUTTON, RBUTTON, CANCEL, MBUTTON, XBUTTON1, XBUTTON2, BACK = 0x8, TAB, CLEAR = 0xc, RETURN, SHIFT = 0x10,
	CONTROL, MENU, PAUSE, CAPITAL, KANA, HANGUEL,HANGUL,JUNJA = 0x17,FINAL,HANJA,KANJI,ESCAPE = 0x1b,CONVERT,
	NONCONVERT,ACCEPT,MODECHANGE,SPACE,PRIOR,NEXT,END,HOME,LEFT,UP,RIGHT,DOWN,SELECT,PRINT,EXECUTE,SNAPSHOT,
	INSERT,DELETE,HELP,LWIN = 0x5b,RWIN,APPS ,SLEEP = 0x5f,NUMPAD0,NUMPAD1,NUMPAD2,NUMPAD3,NUMPAD4,NUMPAD5,
	NUMPAD6,NUMPAD7,NUMPAD8,NUMPAD9,MULTIPLY,ADD,SEPARATOR,SUBTRACT,DECIMAL,DIVIDE,
	F1,F2,F3,F4,F5,F6,F7,F8,F9,F10,F11,F12,F13,F14,F15,F16,F17,F18,F19,F20,F21,F22,F23,F24,NUMLOCK = 0x90,
	SCROLL,LSHIFT = 0xa0,RSHIFT,LCONTROL,RCONTROL,LMENU,RMENU,BROWSER_BACK,BROWSER_FORWARD,BROWSER_REFRESH,
	BROWSER_STOP,BROWSER_SEARCH,BROWSER_FAVORITES,BROWSER_HOME,VOLUME_MUTE,VOLUME_DOWN,VOLUME_UP,MEDIA_NEXT_TRACK,
	MEDIA_PREV_TRACK,MEDIA_STOP,MEDIA_PLAY_PAUSE,LAUNCH_MAIL,LAUNCH_MEDIA_SELECT,LAUNCH_APP1,LAUNCH_APP2,OEM_1 = 0xc0,
	OEM_PLUS,OEM_COMMA,OEM_MINUS,OEM_PERIOD,OEM_2,OEM_3,OEM_4 = 0xdb,OEM_5,OEM_6,OEM_7,OEM_8,OEM_102 = 0xe2,
	PROCESSKEY = 0xe5,PACKET = 0xe7,ATTN = 0xf6,CRSEL,EXSEL,EREOF,PLAY,ZOOM,NONAME,PA1,OEM_CLEAR
}
// Get the current keyboard state (Windows-only)
// state size should be at least 256
void GetKeyboardState(char[] state);

// Get the current mouse location (Windows-only)
// x must not be a mullptr reference
// y must not be a mullptr reference
void GetMouseState(int ref x, int ref y);

// Check if the key is pressed (Windows-only)
bool IsPressed(VK key);

// Check if the character corresponding to the key is pressed (Windows-only)
bool IsPressed(char key);

// Check if the key is toggled (Windows-only)
bool IsToggled(VK key);

// Check if the character corresponding to the key is toggled (Windows-only)
bool IsToggled(char key);

// io object class for formatted stream output
// usage examples:
//  io.out << 15 << io.endl; // print an integer number with a newline after it
//  io.out << io.hex << 15 << io.endl; // print an integer number in a hexidecimal format with a newline after it
//  io.out << io.precision(3) << 3.1415 << io.endl; // print a floating-point number with a pricision of 3 number after a decimal point
//  io.out << io.endl; // prints a newline

class StdOut{}
class StdEndline{}
class StdNonTerminatedTag{}
class StdBase{ int base; }
class StdPrecision{ int precision; }
class StdIO
{
	StdOut out;
	StdEndline endl;
	StdNonTerminatedTag non_terminated_tag;
	StdBase bin, oct, dec, hex;
	StdBase currBase;
	StdPrecision currPrec;
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
	auto precision(int p)
	{
		assert(p >= 0);
		StdPrecision n;
		n.precision = p;
		return n;
	}
}
StdIO io;
io.bin.base = 2;
io.oct.base = 8;
io.dec.base = 10;
io.hex.base = 16;
io.currBase = io.dec;
io.currPrec.precision = 12;

StdOut operator <<(StdOut out, StdBase base)
{
	io.currBase = base;
	return out;
}
StdOut operator <<(StdOut out, StdPrecision precision)
{
	io.currPrec = precision;
	return out;
}
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

StdOut operator <<(StdOut out, StdEndline str)
{
	Print("\n");
	return out;
}

StdOut operator <<(StdOut out, bool num)
{
	if(num)
		Print("true");
	else
		Print("false");
	return out;
}

StdOut operator <<(StdOut out, char ch)
{
	Print(ch);
	return out;
}

StdOut operator <<(StdOut out, short num)
{
	Print(int(num));
	return out;
}

StdOut operator <<(StdOut out, int num)
{
	Print(num, io.currBase.base);
	return out;
}

StdOut operator <<(StdOut out, float num)
{
	Print(double(num));
	return out;
}

StdOut operator <<(StdOut out, double num)
{
	Print(num, io.currPrec.precision);
	return out;
}

StdOut operator <<(StdOut out, long num)
{
	Print(num, io.currBase.base);
	return out;
}
