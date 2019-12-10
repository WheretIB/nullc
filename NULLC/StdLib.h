#pragma once

#include "nullcdef.h"

class Linker;

struct NULLCArray;
struct NULLCRef;
struct NULLCFuncPtr;
struct NULLCAutoArray;

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
	int			Char(char a);
	int			Short(short a);
	int			Int(int a);
	long long	Long(long long a);
	float		Float(float a);
	double		Double(double a);

	int			UnsignedValueChar(unsigned char a);
	int			UnsignedValueShort(unsigned short a);
	long long	UnsignedValueInt(unsigned int a);

	int			StrToShort(NULLCArray str);
	NULLCArray	ShortToStr(short* r);
	int			StrToInt(NULLCArray str);
	NULLCArray	IntToStr(int* r);
	long long	StrToLong(NULLCArray str);
	NULLCArray	LongToStr(long long* r);
	float		StrToFloat(NULLCArray str);
	NULLCArray	FloatToStr(int precision, bool exponent, float* r);
	double		StrToDouble(NULLCArray str);
	NULLCArray	DoubleToStr(int precision, bool exponent, double* r);
	
	void*		AllocObject(int size, unsigned type = 0);
	NULLCArray	AllocArray(unsigned size, unsigned count, unsigned type = 0);
	NULLCRef	CopyObject(NULLCRef ptr);
	void		CopyArray(NULLCAutoArray* dst, NULLCAutoArray src);
	NULLCRef	ReplaceObject(NULLCRef l, NULLCRef r);
	void		SwapObjects(NULLCRef l, NULLCRef r);
	int			CompareObjects(NULLCRef l, NULLCRef r);
	void		AssignObject(NULLCRef l, NULLCRef r);

	void		MarkMemory(unsigned int number);

	bool		IsBasePointer(void* ptr);
	void*		GetBasePointer(void* ptr);

	void		CollectMemory();
	unsigned int	UsedMemory();
	double		MarkTime();
	double		CollectTime();

	void		FinalizeMemory();
	void		ClearMemory();
	void		ResetMemory();

	void		SetGlobalLimit(unsigned int limit);

	NULLCFuncPtr	FunctionRedirect(NULLCRef r, NULLCArray* arr);
	NULLCFuncPtr	FunctionRedirectPtr(NULLCRef r, NULLCArray* arr);

	struct TypeIDHelper
	{
		int id;
	};
	TypeIDHelper Typeid(NULLCRef r);
	int TypeSize(int* a);
	int TypesEqual(int a, int b);
	int TypesNEqual(int a, int b);

	int RefCompare(NULLCRef a, NULLCRef b);
	int RefNCompare(NULLCRef a, NULLCRef b);
	int RefLCompare(NULLCRef a, NULLCRef b);
	int RefLECompare(NULLCRef a, NULLCRef b);
	int RefGCompare(NULLCRef a, NULLCRef b);
	int RefGECompare(NULLCRef a, NULLCRef b);
	int RefHash(NULLCRef a);

	int FuncCompare(NULLCFuncPtr a, NULLCFuncPtr b);
	int FuncNCompare(NULLCFuncPtr a, NULLCFuncPtr b);

	int ArrayCompare(NULLCAutoArray a, NULLCAutoArray b);
	int ArrayNCompare(NULLCAutoArray a, NULLCAutoArray b);

	int TypeCount();

	NULLCAutoArray* AutoArrayAssign(NULLCAutoArray* left, NULLCRef right);
	NULLCRef AutoArrayAssignRev(NULLCRef left, NULLCAutoArray *right);
	NULLCRef AutoArrayIndex(NULLCAutoArray* left, unsigned int index);

	void	AutoArray(NULLCAutoArray* arr, int type, unsigned count);
	void	AutoArraySet(NULLCRef x, unsigned pos, NULLCAutoArray* arr);
	void	ShrinkAutoArray(NULLCAutoArray* arr, unsigned size);

	int	IsCoroutineReset(NULLCRef f);
	void AssertCoroutine(NULLCRef f);

	NULLCArray GetFinalizationList();

	void	ArrayCopy(NULLCAutoArray dst, NULLCAutoArray src);

	void*	AssertDerivedFromBase(unsigned* derived, unsigned base);

	void	CloseUpvalue(void **upvalueList, void *variable, int offset, int size);
}
