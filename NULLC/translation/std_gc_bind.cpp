#include "runtime.h"

struct NamespaceGC 
{
};
void NamespaceGC__CollectMemory_void_ref__(NamespaceGC * __context)
{
	NULLC::CollectMemory();
}

int NamespaceGC__UsedMemory_int_ref__(NamespaceGC * __context)
{
	return NULLC::UsedMemory();
}
double NamespaceGC__MarkTime_double_ref__(NamespaceGC * __context)
{
	return NULLC::MarkTime();
}
double NamespaceGC__CollectTime_double_ref__(NamespaceGC * __context)
{
	return NULLC::CollectTime();
}
