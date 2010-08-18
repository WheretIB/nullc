#pragma once
#include "nullcdef.h"
#include "Linker.h"

namespace NULLC
{
	void	SetLinker(Linker *linker);

	void	Assert(int val);
	void	Assert2(int val, NULLCArray message);

	int		StrEqual(NULLCArray a, NULLCArray b);
	int		StrNEqual(NULLCArray a, NULLCArray b);
	NULLCArray	StrConcatenate(NULLCArray a, NULLCArray b);
	NULLCArray	StrConcatenateAndSet(NULLCArray *a, NULLCArray b);

	// Basic type constructors
	int			Int(int a);
	long long	Long(long long a);
	float		Float(float a);
	double		Double(double a);

	NULLCArray	IntToStr(int* r);
	NULLCArray	DoubleToStr(int precision, double* r);
	
	void*		AllocObject(int size);
	NULLCArray	AllocArray(int size, int count);
	NULLCRef	CopyObject(NULLCRef ptr);
	NULLCRef	ReplaceObject(NULLCRef l, NULLCRef r);

	void		MarkMemory(unsigned int number);

	bool		IsBasePointer(void* ptr);
	void*		GetBasePointer(void* ptr);

	void		CollectMemory();
	unsigned int	UsedMemory();
	double		MarkTime();
	double		CollectTime();

	void		ClearMemory();
	void		ResetMemory();

	void		SetGlobalLimit(unsigned int limit);

	NULLCFuncPtr	FunctionRedirect(NULLCRef r, NULLCArray* arr);

	struct TypeIDHelper
	{
		int id;
	};
	TypeIDHelper Typeid(NULLCRef r);
	int TypeSize(int* a);
	int TypesEqual(int a, int b);
	int TypesNEqual(int a, int b);

	int FuncCompare(NULLCFuncPtr a, NULLCFuncPtr b);
	int FuncNCompare(NULLCFuncPtr a, NULLCFuncPtr b);

	int TypeCount();

	NULLCAutoArray* AutoArrayAssign(NULLCAutoArray* left, NULLCRef right);
	NULLCRef AutoArrayAssignRev(NULLCRef left, NULLCAutoArray *right);
	NULLCAutoArray* AutoArrayAssignSelf(NULLCAutoArray* left, NULLCAutoArray* right);
	NULLCRef AutoArrayIndex(NULLCAutoArray* left, unsigned int index);

	NULLCAutoArray	AutoArray(int type, int count);
	void			AutoArraySet(NULLCRef x, unsigned pos, NULLCAutoArray* arr);

	int	IsCoroutineReset(NULLCRef f);
	void AssertCoroutine(NULLCRef f);
}
