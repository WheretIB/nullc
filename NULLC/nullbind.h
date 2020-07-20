#pragma once

#include <string.h>

#include "nullc.h"

namespace NULLC
{
	// for functions with no arguments
	template<class T, typename R, R (T::*pmem)()>
	R ProxyFunc(T* p){ return (p->*pmem)(); }
	template<typename T, typename R>
	struct CreateClosureHelper0{ typedef R (*functionType)(T*); template<R (T::*f)()> functionType Init(){ return ProxyFunc<T, R, f>; } };
	template<typename T, typename R>
	CreateClosureHelper0<T, R> CreateClosure(R (T::*)()){ return CreateClosureHelper0<T, R>(); }

	// for functions with arguments
#define LIST(...) __VA_ARGS__
#define GEN_CREATOR(argC, argList, typeList, typenameList, callList)\
	template<class T, typename R, typenameList, R (T::*pmem)(typeList)>\
	R ProxyFunc(argList, T* p){ return (p->*pmem)(callList); }\
	template<typename T, typename R, typenameList>\
	struct CreateClosureHelper##argC{ typedef R (*functionType)(typeList, T*); template<R (T::*f)(typeList)> functionType Init(){ return ProxyFunc<T, R, typeList, f>; } };\
	template<typename T, typename R, typenameList>\
	CreateClosureHelper##argC<T, R, typeList> CreateClosure(R (T::*)(typeList)){ return CreateClosureHelper##argC<T, R, typeList>(); }

	GEN_CREATOR(1, LIST(A1 a1), LIST(A1), LIST(typename A1), LIST(a1));
	GEN_CREATOR(2, LIST(A1 a1, A2 a2), LIST(A1, A2), LIST(typename A1, typename A2), LIST(a1, a2));
	GEN_CREATOR(3, LIST(A1 a1, A2 a2, A3 a3), LIST(A1, A2, A3), LIST(typename A1, typename A2, typename A3), LIST(a1, a2, a3));
	GEN_CREATOR(4, LIST(A1 a1, A2 a2, A3 a3, A4 a4), LIST(A1, A2, A3, A4), LIST(typename A1, typename A2, typename A3, typename A4), LIST(a1, a2, a3, a4));
	GEN_CREATOR(5, LIST(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5), LIST(A1, A2, A3, A4, A5), LIST(typename A1, typename A2, typename A3, typename A4, typename A5), LIST(a1, a2, a3, a4, a5));
#undef LIST

	// class member getter
	template<typename T, typename R, R (T::*f)>
	R ProxyGetter(T* x){ return x->*f; }
	template<typename T, typename R>
	struct CreateGetterHelper{ typedef R (*functionType)(T*); template<R (T::*f)> functionType Init(){ return ProxyGetter<T, R, f>; } };
	template<typename T, typename R>
	CreateGetterHelper<T, R> CreateGetter(R (T::*)){ return CreateGetterHelper<T, R>(); }

	// class member setter
	template<typename T, typename R, R (T::*f)>
	R ProxySetter(R& v, T* x){ return x->*f = v; }
	template<typename T, typename R>
	struct CreateSetterHelper{ typedef R (*functionType)(R&, T*); template<R (T::*f)> functionType Init(){ return ProxySetter<T, R, f>; } };
	template<typename T, typename R>
	CreateSetterHelper<T, R> CreateSetter(R (T::*)){ return CreateSetterHelper<T, R>(); }
}
// Wrappers for binding
#define BIND_CLASS_FUNCTION(module, function, name, index) nullcBindModuleFunction(module, (void(*)())NULLC::CreateClosure(function).Init<function>(), name, index)
#define BIND_CLASS_MEMBER_GET(module, member, name, index) nullcBindModuleFunction(module, (void(*)())NULLC::CreateGetter(member).Init<member>(), name, index)
#define BIND_CLASS_MEMBER_SET(module, member, name, index) nullcBindModuleFunction(module, (void(*)())NULLC::CreateSetter(member).Init<member>(), name, index)

/************************************************************************/
/*				Wrapped function calls									*/

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4800)	// forcing value to bool 'true' or 'false' (performance warning)
#endif

// SFINAE
template<typename T>
struct nullc_is_void
{
	static const bool value = false;
};

template<>
struct nullc_is_void<void>
{
	static const bool value = true;
};

template<typename T>
struct nullc_is_small
{
	static const bool value = sizeof(T) == 1;
};

template<>
struct nullc_is_small<void>
{
	static const bool value = false;
};

template<bool Cond, typename T>
struct nullc_enable_if
{
};

template<typename T>
struct nullc_enable_if<true, T>
{
	typedef T type;
};

// Type promotion helpers
template<typename T, bool IsReturn = false>
struct NullcCallBaseType
{
	typedef T type;

	// All nullc types are round to 4 bytes
	static const unsigned size = sizeof(T) == 0 ? 4 : (sizeof(T) + 3) & ~3;
};

template<typename T, bool IsReturn>
struct NullcCallBaseType<T&, IsReturn>
{
};

template<bool IsReturn>
struct NullcCallBaseType<void, IsReturn>
{
	typedef void type;
	static const unsigned size = sizeof(int);
};

template<bool IsReturn>
struct NullcCallBaseType<bool, IsReturn>
{
	typedef int type;
	static const unsigned size = sizeof(int);
};

template<bool IsReturn>
struct NullcCallBaseType<char, IsReturn>
{
	typedef int type;
	static const unsigned size = sizeof(int);
};

template<bool IsReturn>
struct NullcCallBaseType<unsigned char, IsReturn>
{
	typedef int type;
	static const unsigned size = sizeof(int);
};

template<bool IsReturn>
struct NullcCallBaseType<short, IsReturn>
{
	typedef int type;
	static const unsigned size = sizeof(int);
};

template<bool IsReturn>
struct NullcCallBaseType<unsigned short, IsReturn>
{
	typedef int type;
	static const unsigned size = sizeof(int);
};

template<>
struct NullcCallBaseType<float, true>
{
	typedef double type;
	static const unsigned size = sizeof(double);
};

template<>
struct NullcCallBaseType<float, false>
{
	typedef float type;
	static const unsigned size = sizeof(float);
};

#define OFF_1 0
#define OFF_2 A1s
#define OFF_3 OFF_2 + A2s
#define OFF_4 OFF_3 + A3s
#define OFF_5 OFF_4 + A4s
#define OFF_6 OFF_5 + A5s
#define OFF_7 OFF_6 + A6s
#define OFF_8 OFF_7 + A7s
#define OFF_9 OFF_8 + A8s
#define OFF_10 OFF_9 + A9s
#define OFF_11 OFF_10 + A10s
#define OFF_12 OFF_11 + A11s
#define OFF_13 OFF_12 + A12s
#define OFF_14 OFF_13 + A13s
#define OFF_15 OFF_14 + A14s

#define STYPE(X, N) typedef typename NullcCallBaseType<X>::type X##u; X##u X##v; memcpy(&X##v, argBuf + OFF_##N, sizeof(X##v))
#define SSIZE(X) const unsigned X##s = NullcCallBaseType<X>::size;

#define SHORTS_0 (void)argBuf
#define SHORTS_1 STYPE(A1, 1)
#define SHORTS_2 SHORTS_1; SSIZE(A1) STYPE(A2, 2)
#define SHORTS_3 SHORTS_2; SSIZE(A2) STYPE(A3, 3)
#define SHORTS_4 SHORTS_3; SSIZE(A3) STYPE(A4, 4)
#define SHORTS_5 SHORTS_4; SSIZE(A4) STYPE(A5, 5)
#define SHORTS_6 SHORTS_5; SSIZE(A5) STYPE(A6, 6)
#define SHORTS_7 SHORTS_6; SSIZE(A6) STYPE(A7, 7)
#define SHORTS_8 SHORTS_7; SSIZE(A7) STYPE(A8, 8)
#define SHORTS_9 SHORTS_8; SSIZE(A8) STYPE(A9, 9)
#define SHORTS_10 SHORTS_9; SSIZE(A9) STYPE(A10, 10)
#define SHORTS_11 SHORTS_10; SSIZE(A10) STYPE(A11, 11)
#define SHORTS_12 SHORTS_11; SSIZE(A11) STYPE(A12, 12)
#define SHORTS_13 SHORTS_12; SSIZE(A12) STYPE(A13, 13)
#define SHORTS_14 SHORTS_13; SSIZE(A13) STYPE(A14, 14)
#define SHORTS_15 SHORTS_14; SSIZE(A14) STYPE(A15, 15)

#define ARG(X) (A##X)A##X##v

#define ARGS_0
#define ARGS_1 ARG(1)
#define ARGS_2 ARGS_1, ARG(2)
#define ARGS_3 ARGS_2, ARG(3)
#define ARGS_4 ARGS_3, ARG(4)
#define ARGS_5 ARGS_4, ARG(5)
#define ARGS_6 ARGS_5, ARG(6)
#define ARGS_7 ARGS_6, ARG(7)
#define ARGS_8 ARGS_7, ARG(8)
#define ARGS_9 ARGS_8, ARG(9)
#define ARGS_10 ARGS_9, ARG(10)
#define ARGS_11 ARGS_10, ARG(11)
#define ARGS_12 ARGS_11, ARG(12)
#define ARGS_13 ARGS_12, ARG(13)
#define ARGS_14 ARGS_13, ARG(14)
#define ARGS_15 ARGS_14, ARG(15)

#define ATYPE_0
#define ATYPE_1 A1
#define ATYPE_2 ATYPE_1, A2
#define ATYPE_3 ATYPE_2, A3
#define ATYPE_4 ATYPE_3, A4
#define ATYPE_5 ATYPE_4, A5
#define ATYPE_6 ATYPE_5, A6
#define ATYPE_7 ATYPE_6, A7
#define ATYPE_8 ATYPE_7, A8
#define ATYPE_9 ATYPE_8, A9
#define ATYPE_10 ATYPE_9, A10
#define ATYPE_11 ATYPE_10, A11
#define ATYPE_12 ATYPE_11, A12
#define ATYPE_13 ATYPE_12, A13
#define ATYPE_14 ATYPE_13, A14
#define ATYPE_15 ATYPE_14, A15

#define TTYPE_0
#define TTYPE_1 , typename A1
#define TTYPE_2 TTYPE_1, typename A2
#define TTYPE_3 TTYPE_2, typename A3
#define TTYPE_4 TTYPE_3, typename A4
#define TTYPE_5 TTYPE_4, typename A5
#define TTYPE_6 TTYPE_5, typename A6
#define TTYPE_7 TTYPE_6, typename A7
#define TTYPE_8 TTYPE_7, typename A8
#define TTYPE_9 TTYPE_8, typename A9
#define TTYPE_10 TTYPE_9, typename A10
#define TTYPE_11 TTYPE_10, typename A11
#define TTYPE_12 TTYPE_11, typename A12
#define TTYPE_13 TTYPE_12, typename A13
#define TTYPE_14 TTYPE_13, typename A14
#define TTYPE_15 TTYPE_14, typename A15

#define VOID_RET typename nullc_enable_if<nullc_is_void<R>::value, void>::type
#define NON_VOID_RET typename nullc_enable_if<!nullc_is_void<R>::value && nullc_is_small<R>::value, void>::type
#define NON_VOID_RET_A typename nullc_enable_if<!nullc_is_void<R>::value && !nullc_is_small<R>::value, void>::type

// Raw function wrappers
template<typename R TTYPE_0>
VOID_RET nullcWrapCall0(void *context, char* retBuf, char* argBuf)
{
	(void)retBuf;

	SHORTS_0;

	((R(*)(ATYPE_0))context)(ARGS_0);
}

template<typename R TTYPE_0>
NON_VOID_RET nullcWrapCall0(void *context, char* retBuf, char* argBuf)
{
	typedef typename NullcCallBaseType<R, true>::type Ru;

	SHORTS_0;

	*(Ru*)retBuf = (Ru)((R(*)(ATYPE_0))context)(ARGS_0);
}

template<typename R TTYPE_0>
NON_VOID_RET_A nullcWrapCall0(void *context, char* retBuf, char* argBuf)
{
	typedef typename NullcCallBaseType<R, true>::type Ru;

	SHORTS_0;

	Ru result = (Ru)((R(*)(ATYPE_0))context)(ARGS_0);
	memcpy(retBuf, &result, sizeof(Ru));
}

template<typename R TTYPE_1>
VOID_RET nullcWrapCall1(void *context, char* retBuf, char* argBuf)
{
	(void)retBuf;

	SHORTS_1;

	((R(*)(ATYPE_1))context)(ARGS_1);
}

template<typename R TTYPE_1>
NON_VOID_RET nullcWrapCall1(void *context, char* retBuf, char* argBuf)
{
	typedef typename NullcCallBaseType<R, true>::type Ru;

	SHORTS_1;

	*(Ru*)retBuf = (Ru)((R(*)(ATYPE_1))context)(ARGS_1);
}

template<typename R TTYPE_1>
NON_VOID_RET_A nullcWrapCall1(void *context, char* retBuf, char* argBuf)
{
	typedef typename NullcCallBaseType<R, true>::type Ru;

	SHORTS_1;

	Ru result = (Ru)((R(*)(ATYPE_1))context)(ARGS_1);
	memcpy(retBuf, &result, sizeof(Ru));
}

template<typename R TTYPE_2>
VOID_RET nullcWrapCall2(void *context, char* retBuf, char* argBuf)
{
	(void)retBuf;

	SHORTS_2;

	((R(*)(ATYPE_2))context)(ARGS_2);
}

template<typename R TTYPE_2>
NON_VOID_RET nullcWrapCall2(void *context, char* retBuf, char* argBuf)
{
	typedef typename NullcCallBaseType<R, true>::type Ru;

	SHORTS_2;

	*(Ru*)retBuf = (Ru)((R(*)(ATYPE_2))context)(ARGS_2);
}

template<typename R TTYPE_2>
NON_VOID_RET_A nullcWrapCall2(void *context, char* retBuf, char* argBuf)
{
	typedef typename NullcCallBaseType<R, true>::type Ru;

	SHORTS_2;

	Ru result = (Ru)((R(*)(ATYPE_2))context)(ARGS_2);
	memcpy(retBuf, &result, sizeof(Ru));
}

template<typename R TTYPE_3>
VOID_RET nullcWrapCall3(void *context, char* retBuf, char* argBuf)
{
	(void)retBuf;

	SHORTS_3;

	((R(*)(ATYPE_3))context)(ARGS_3);
}

template<typename R TTYPE_3>
NON_VOID_RET nullcWrapCall3(void *context, char* retBuf, char* argBuf)
{
	typedef typename NullcCallBaseType<R, true>::type Ru;

	SHORTS_3;

	*(Ru*)retBuf = (Ru)((R(*)(ATYPE_3))context)(ARGS_3);
}

template<typename R TTYPE_3>
NON_VOID_RET_A nullcWrapCall3(void *context, char* retBuf, char* argBuf)
{
	typedef typename NullcCallBaseType<R, true>::type Ru;

	SHORTS_3;

	Ru result = (Ru)((R(*)(ATYPE_3))context)(ARGS_3);
	memcpy(retBuf, &result, sizeof(Ru));
}

template<typename R TTYPE_4>
VOID_RET nullcWrapCall4(void *context, char* retBuf, char* argBuf)
{
	(void)retBuf;

	SHORTS_4;

	((R(*)(ATYPE_4))context)(ARGS_4);
}

template<typename R TTYPE_4>
NON_VOID_RET nullcWrapCall4(void *context, char* retBuf, char* argBuf)
{
	typedef typename NullcCallBaseType<R, true>::type Ru;

	SHORTS_4;

	*(Ru*)retBuf = (Ru)((R(*)(ATYPE_4))context)(ARGS_4);
}

template<typename R TTYPE_4>
NON_VOID_RET_A nullcWrapCall4(void *context, char* retBuf, char* argBuf)
{
	typedef typename NullcCallBaseType<R, true>::type Ru;

	SHORTS_4;

	Ru result = (Ru)((R(*)(ATYPE_4))context)(ARGS_4);
	memcpy(retBuf, &result, sizeof(Ru));
}

template<typename R TTYPE_5>
VOID_RET nullcWrapCall5(void *context, char* retBuf, char* argBuf)
{
	(void)retBuf;

	SHORTS_5;

	((R(*)(ATYPE_5))context)(ARGS_5);
}

template<typename R TTYPE_5>
NON_VOID_RET nullcWrapCall5(void *context, char* retBuf, char* argBuf)
{
	typedef typename NullcCallBaseType<R, true>::type Ru;

	SHORTS_5;

	*(Ru*)retBuf = (Ru)((R(*)(ATYPE_5))context)(ARGS_5);
}

template<typename R TTYPE_5>
NON_VOID_RET_A nullcWrapCall5(void *context, char* retBuf, char* argBuf)
{
	typedef typename NullcCallBaseType<R, true>::type Ru;

	SHORTS_5;

	Ru result = (Ru)((R(*)(ATYPE_5))context)(ARGS_5);
	memcpy(retBuf, &result, sizeof(Ru));
}

template<typename R TTYPE_6>
VOID_RET nullcWrapCall6(void *context, char* retBuf, char* argBuf)
{
	(void)retBuf;

	SHORTS_6;

	((R(*)(ATYPE_6))context)(ARGS_6);
}

template<typename R TTYPE_6>
NON_VOID_RET nullcWrapCall6(void *context, char* retBuf, char* argBuf)
{
	typedef typename NullcCallBaseType<R, true>::type Ru;

	SHORTS_6;

	*(Ru*)retBuf = (Ru)((R(*)(ATYPE_6))context)(ARGS_6);
}

template<typename R TTYPE_6>
NON_VOID_RET_A nullcWrapCall6(void *context, char* retBuf, char* argBuf)
{
	typedef typename NullcCallBaseType<R, true>::type Ru;

	SHORTS_6;

	Ru result = (Ru)((R(*)(ATYPE_6))context)(ARGS_6);
	memcpy(retBuf, &result, sizeof(Ru));
}

template<typename R TTYPE_7>
VOID_RET nullcWrapCall7(void *context, char* retBuf, char* argBuf)
{
	(void)retBuf;

	SHORTS_7;

	((R(*)(ATYPE_7))context)(ARGS_7);
}

template<typename R TTYPE_7>
NON_VOID_RET nullcWrapCall7(void *context, char* retBuf, char* argBuf)
{
	typedef typename NullcCallBaseType<R, true>::type Ru;

	SHORTS_7;

	*(Ru*)retBuf = (Ru)((R(*)(ATYPE_7))context)(ARGS_7);
}

template<typename R TTYPE_7>
NON_VOID_RET_A nullcWrapCall7(void *context, char* retBuf, char* argBuf)
{
	typedef typename NullcCallBaseType<R, true>::type Ru;

	SHORTS_7;

	Ru result = (Ru)((R(*)(ATYPE_7))context)(ARGS_7);
	memcpy(retBuf, &result, sizeof(Ru));
}

template<typename R TTYPE_8>
VOID_RET nullcWrapCall8(void *context, char* retBuf, char* argBuf)
{
	(void)retBuf;

	SHORTS_8;

	((R(*)(ATYPE_8))context)(ARGS_8);
}

template<typename R TTYPE_8>
NON_VOID_RET nullcWrapCall8(void *context, char* retBuf, char* argBuf)
{
	typedef typename NullcCallBaseType<R, true>::type Ru;

	SHORTS_8;

	*(Ru*)retBuf = (Ru)((R(*)(ATYPE_8))context)(ARGS_8);
}

template<typename R TTYPE_8>
NON_VOID_RET_A nullcWrapCall8(void *context, char* retBuf, char* argBuf)
{
	typedef typename NullcCallBaseType<R, true>::type Ru;

	SHORTS_8;

	Ru result = (Ru)((R(*)(ATYPE_8))context)(ARGS_8);
	memcpy(retBuf, &result, sizeof(Ru));
}

template<typename R TTYPE_9>
VOID_RET nullcWrapCall9(void *context, char* retBuf, char* argBuf)
{
	(void)retBuf;

	SHORTS_9;

	((R(*)(ATYPE_9))context)(ARGS_9);
}

template<typename R TTYPE_9>
NON_VOID_RET nullcWrapCall9(void *context, char* retBuf, char* argBuf)
{
	typedef typename NullcCallBaseType<R, true>::type Ru;

	SHORTS_9;

	*(Ru*)retBuf = (Ru)((R(*)(ATYPE_9))context)(ARGS_9);
}

template<typename R TTYPE_9>
NON_VOID_RET_A nullcWrapCall9(void *context, char* retBuf, char* argBuf)
{
	typedef typename NullcCallBaseType<R, true>::type Ru;

	SHORTS_9;

	Ru result = (Ru)((R(*)(ATYPE_9))context)(ARGS_9);
	memcpy(retBuf, &result, sizeof(Ru));
}

template<typename R TTYPE_10>
VOID_RET nullcWrapCall10(void *context, char* retBuf, char* argBuf)
{
	(void)retBuf;

	SHORTS_10;

	((R(*)(ATYPE_10))context)(ARGS_10);
}

template<typename R TTYPE_10>
NON_VOID_RET nullcWrapCall10(void *context, char* retBuf, char* argBuf)
{
	typedef typename NullcCallBaseType<R, true>::type Ru;

	SHORTS_10;

	*(Ru*)retBuf = (Ru)((R(*)(ATYPE_10))context)(ARGS_10);
}

template<typename R TTYPE_10>
NON_VOID_RET_A nullcWrapCall10(void *context, char* retBuf, char* argBuf)
{
	typedef typename NullcCallBaseType<R, true>::type Ru;

	SHORTS_10;

	Ru result = (Ru)((R(*)(ATYPE_10))context)(ARGS_10);
	memcpy(retBuf, &result, sizeof(Ru));
}

template<typename R TTYPE_11>
VOID_RET nullcWrapCall11(void *context, char* retBuf, char* argBuf)
{
	(void)retBuf;

	SHORTS_11;

	((R(*)(ATYPE_11))context)(ARGS_11);
}

template<typename R TTYPE_11>
NON_VOID_RET nullcWrapCall11(void *context, char* retBuf, char* argBuf)
{
	typedef typename NullcCallBaseType<R, true>::type Ru;

	SHORTS_11;

	*(Ru*)retBuf = (Ru)((R(*)(ATYPE_11))context)(ARGS_11);
}

template<typename R TTYPE_11>
NON_VOID_RET_A nullcWrapCall11(void *context, char* retBuf, char* argBuf)
{
	typedef typename NullcCallBaseType<R, true>::type Ru;

	SHORTS_11;

	Ru result = (Ru)((R(*)(ATYPE_11))context)(ARGS_11);
	memcpy(retBuf, &result, sizeof(Ru));
}

template<typename R TTYPE_12>
VOID_RET nullcWrapCall12(void *context, char* retBuf, char* argBuf)
{
	(void)retBuf;

	SHORTS_12;

	((R(*)(ATYPE_12))context)(ARGS_12);
}

template<typename R TTYPE_12>
NON_VOID_RET nullcWrapCall12(void *context, char* retBuf, char* argBuf)
{
	typedef typename NullcCallBaseType<R, true>::type Ru;

	SHORTS_12;

	*(Ru*)retBuf = (Ru)((R(*)(ATYPE_12))context)(ARGS_12);
}

template<typename R TTYPE_12>
NON_VOID_RET_A nullcWrapCall12(void *context, char* retBuf, char* argBuf)
{
	typedef typename NullcCallBaseType<R, true>::type Ru;

	SHORTS_12;

	Ru result = (Ru)((R(*)(ATYPE_12))context)(ARGS_12);
	memcpy(retBuf, &result, sizeof(Ru));
}

template<typename R TTYPE_13>
VOID_RET nullcWrapCall13(void *context, char* retBuf, char* argBuf)
{
	(void)retBuf;

	SHORTS_13;

	((R(*)(ATYPE_13))context)(ARGS_13);
}

template<typename R TTYPE_13>
NON_VOID_RET nullcWrapCall13(void *context, char* retBuf, char* argBuf)
{
	typedef typename NullcCallBaseType<R, true>::type Ru;

	SHORTS_13;

	*(Ru*)retBuf = (Ru)((R(*)(ATYPE_13))context)(ARGS_13);
}

template<typename R TTYPE_13>
NON_VOID_RET_A nullcWrapCall13(void *context, char* retBuf, char* argBuf)
{
	typedef typename NullcCallBaseType<R, true>::type Ru;

	SHORTS_13;

	Ru result = (Ru)((R(*)(ATYPE_13))context)(ARGS_13);
	memcpy(retBuf, &result, sizeof(Ru));
}

template<typename R TTYPE_14>
VOID_RET nullcWrapCall14(void *context, char* retBuf, char* argBuf)
{
	(void)retBuf;

	SHORTS_14;

	((R(*)(ATYPE_14))context)(ARGS_14);
}

template<typename R TTYPE_14>
NON_VOID_RET nullcWrapCall14(void *context, char* retBuf, char* argBuf)
{
	typedef typename NullcCallBaseType<R, true>::type Ru;

	SHORTS_14;

	*(Ru*)retBuf = (Ru)((R(*)(ATYPE_14))context)(ARGS_14);
}

template<typename R TTYPE_14>
NON_VOID_RET_A nullcWrapCall14(void *context, char* retBuf, char* argBuf)
{
	typedef typename NullcCallBaseType<R, true>::type Ru;

	SHORTS_14;

	Ru result = (Ru)((R(*)(ATYPE_14))context)(ARGS_14);
	memcpy(retBuf, &result, sizeof(Ru));
}

template<typename R TTYPE_15>
VOID_RET nullcWrapCall15(void *context, char* retBuf, char* argBuf)
{
	(void)retBuf;

	SHORTS_15;

	((R(*)(ATYPE_15))context)(ARGS_15);
}

template<typename R TTYPE_15>
NON_VOID_RET nullcWrapCall15(void *context, char* retBuf, char* argBuf)
{
	typedef typename NullcCallBaseType<R, true>::type Ru;

	SHORTS_15;

	*(Ru*)retBuf = (Ru)((R(*)(ATYPE_15))context)(ARGS_15);
}

template<typename R TTYPE_15>
NON_VOID_RET_A nullcWrapCall15(void *context, char* retBuf, char* argBuf)
{
	typedef typename NullcCallBaseType<R, true>::type Ru;

	SHORTS_15;

	Ru result = (Ru)((R(*)(ATYPE_15))context)(ARGS_15);
	memcpy(retBuf, &result, sizeof(Ru));
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif

// Function wrapper creation
typedef void (*nullcFunc)(void*, char*, char*);

#define COMMA_0
#define COMMA_1 ,
#define COMMA(cond) COMMA_##cond

#define DEFINE_CALL_WRAPPER(X, Y) template<typename R TTYPE_##X> nullcFunc nullcGetCallWrapper(R(*)(ATYPE_##X)){ return nullcWrapCall##X<R COMMA(Y) ATYPE_##X>; }

DEFINE_CALL_WRAPPER(0, 0)
DEFINE_CALL_WRAPPER(1, 1)
DEFINE_CALL_WRAPPER(2, 1)
DEFINE_CALL_WRAPPER(3, 1)
DEFINE_CALL_WRAPPER(4, 1)
DEFINE_CALL_WRAPPER(5, 1)
DEFINE_CALL_WRAPPER(6, 1)
DEFINE_CALL_WRAPPER(7, 1)
DEFINE_CALL_WRAPPER(8, 1)
DEFINE_CALL_WRAPPER(9, 1)
DEFINE_CALL_WRAPPER(10, 1)
DEFINE_CALL_WRAPPER(11, 1)
DEFINE_CALL_WRAPPER(12, 1)
DEFINE_CALL_WRAPPER(13, 1)
DEFINE_CALL_WRAPPER(14, 1)
DEFINE_CALL_WRAPPER(15, 1)

// Bind functions directly
#define DEFINE_HELPER_WRAPPER(X) template<typename R TTYPE_##X> nullres nullcBindModuleFunctionHelper(const char* module, R(*f)(ATYPE_##X), const char* name, int index){ return nullcBindModuleFunctionWrapper(module, (void*)f, nullcGetCallWrapper(f), name, index); }

DEFINE_HELPER_WRAPPER(0)
DEFINE_HELPER_WRAPPER(1)
DEFINE_HELPER_WRAPPER(2)
DEFINE_HELPER_WRAPPER(3)
DEFINE_HELPER_WRAPPER(4)
DEFINE_HELPER_WRAPPER(5)
DEFINE_HELPER_WRAPPER(6)
DEFINE_HELPER_WRAPPER(7)
DEFINE_HELPER_WRAPPER(8)
DEFINE_HELPER_WRAPPER(9)
DEFINE_HELPER_WRAPPER(10)
DEFINE_HELPER_WRAPPER(11)
DEFINE_HELPER_WRAPPER(12)
DEFINE_HELPER_WRAPPER(13)
DEFINE_HELPER_WRAPPER(14)
DEFINE_HELPER_WRAPPER(15)
