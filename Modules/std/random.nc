// std.random

void srand(int i);
int rand();
int rand(int max)
{
	return rand() % (max + 1);
}
