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
