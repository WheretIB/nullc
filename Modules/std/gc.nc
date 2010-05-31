// std.gc

class NamespaceGC
{
	void CollectMemory();

	int		UsedMemory();

	double	MarkTime();
	double	CollectTime();
}
NamespaceGC GC;
