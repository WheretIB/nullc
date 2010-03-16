#pragma once
#include "nullcdef.h"
#include "Linker.h"

namespace NULLC
{
	void	SetLinker(Linker *linker);

	void	Assert(int val);
	void	Assert2(int val, NullCArray message);

	int		StrEqual(NullCArray a, NullCArray b);
	int		StrNEqual(NullCArray a, NullCArray b);
	NullCArray	StrConcatenate(NullCArray a, NullCArray b);
	NullCArray	StrConcatenateAndSet(NullCArray *a, NullCArray b);

	// Basic type constructors
	int			Int(int a);
	long long	Long(long long a);
	float		Float(float a);
	double		Double(double a);

	NullCArray	IntToStr(int* r);
	
	void*		AllocObject(int size);
	NullCArray	AllocArray(int size, int count);
	NULLCRef	CopyObject(NULLCRef ptr);

	void		MarkMemory(unsigned int number);

	bool		IsBasePointer(void* ptr);
	void*		GetBasePointer(void* ptr);

	void		CollectMemory();

	void		ClearMemory();
	void		ResetMemory();
}
