import std.string;
import std.io;

StdOut operator <<(StdOut out, string ref str)
{
	Print(str.data);
	return out;
}
