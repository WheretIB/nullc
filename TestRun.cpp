#include "NULLC/nullc.h"
#include "UnitTests.h"

int main(int argc, char** argv)
{
	return RunTests(argc == 2, 0, argc == 3);
}
