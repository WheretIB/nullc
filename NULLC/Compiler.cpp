#include "Compiler.h"

#include "nullc.h"
#include "BinaryCache.h"
#include "Executor_Common.h"
#include "StdLib.h"

const char *nullcBaseCode = "\
void assert(int val);\r\n\
void assert(int val, char[] message);\r\n\
\r\n\
int operator ==(char[] a, b);\r\n\
int operator !=(char[] a, b);\r\n\
char[] operator +(char[] a, b);\r\n\
char[] operator +=(char[] ref a, char[] b);\r\n\
\r\n\
bool bool(bool a);\r\n\
char char(char a);\r\n\
short short(short a);\r\n\
int int(int a);\r\n\
long long(long a);\r\n\
float float(float a);\r\n\
double double(double a);\r\n\
void bool:bool(bool a){ *this = a; }\r\n\
void char:char(char a){ *this = a; }\r\n\
void short:short(short a){ *this = a; }\r\n\
void int:int(int a){ *this = a; }\r\n\
void long:long(long a){ *this = a; }\r\n\
void float:float(float a){ *this = a; }\r\n\
void double:double(double a){ *this = a; }\r\n\
\r\n\
int as_unsigned(char a);\r\n\
int as_unsigned(short a);\r\n\
long as_unsigned(int a);\r\n\
\r\n\
short short(char[] str);\r\n\
char[] short:str();\r\n\
int int(char[] str);\r\n\
char[] int:str();\r\n\
long long(char[] str);\r\n\
char[] long:str();\r\n\
float float(char[] str);\r\n\
char[] float:str(int precision = 6, bool showExponent = false);\r\n\
double double(char[] str);\r\n\
char[] double:str(int precision = 6, bool showExponent = false);\r\n\
\r\n\
void ref __newS(int size, int type);\r\n\
int[] __newA(int size, int count, int type);\r\n\
auto ref	duplicate(auto ref obj);\r\n\
void		__duplicate_array(auto[] ref dst, auto[] src);\r\n\
auto[]		duplicate(auto[] arr){ auto[] r; __duplicate_array(&r, arr); return r; }\r\n\
auto ref	replace(auto ref l, r);\r\n\
void		swap(auto ref l, r);\r\n\
int			equal(auto ref l, r);\r\n\
void		assign(explicit auto ref l, auto ref r);\r\n\
void		array_copy(auto[] l, r);\r\n\
void		array_copy(generic dst, int offsetDst, generic src, int offsetSrc, count)\r\n\
{\r\n\
	for({ int i = offsetDst, k = offsetSrc; }; i < offsetDst + count; {i++; k++; })\r\n\
		dst[i] = src[k];\r\n\
}\r\n\
\r\n\
void ref() __redirect(auto ref r, __function[] ref f);\r\n\
void ref() __redirect_ptr(auto ref r, __function[] ref f);\r\n\
// char inline array definition support\r\n\
auto operator=(char[] ref dst, int[] src)\r\n\
{\r\n\
	if(dst.size < src.size)\r\n\
		*dst = new char[src.size];\r\n\
	for(int i = 0; i < src.size; i++)\r\n\
		dst[i] = src[i];\r\n\
	return dst;\r\n\
}\r\n\
// short inline array definition support\r\n\
auto operator=(short[] ref dst, int[] src)\r\n\
{\r\n\
	if(dst.size < src.size)\r\n\
		*dst = new short[src.size];\r\n\
	for(int i = 0; i < src.size; i++)\r\n\
		dst[i] = src[i];\r\n\
	return dst;\r\n\
}\r\n\
// float inline array definition support\r\n\
auto operator=(float[] ref dst, double[] src)\r\n\
{\r\n\
	if(dst.size < src.size)\r\n\
		*dst = new float[src.size];\r\n\
	for(int i = 0; i < src.size; i++)\r\n\
		dst[i] = src[i];\r\n\
	return dst;\r\n\
}\r\n\
// typeid retrieval from auto ref\r\n\
typeid typeid(auto ref type);\r\n\
int typeid.size();\r\n\
// typeid comparison\r\n\
int operator==(typeid a, b);\r\n\
int operator!=(typeid a, b);\r\n\
\r\n\
int __rcomp(auto ref a, b);\r\n\
int __rncomp(auto ref a, b);\r\n\
\r\n\
bool operator<(explicit auto ref a, b);\n\r\
bool operator<=(explicit auto ref a, b);\n\r\
bool operator>(explicit auto ref a, b);\n\r\
bool operator>=(explicit auto ref a, b);\n\r\
\r\n\
int hash_value(explicit auto ref x);\r\n\
\r\n\
int __pcomp(void ref(int) a, void ref(int) b);\r\n\
int __pncomp(void ref(int) a, void ref(int) b);\r\n\
\r\n\
int __acomp(auto[] a, b);\r\n\
int __ancomp(auto[] a, b);\r\n\
\r\n\
int __typeCount();\r\n\
\r\n\
auto[] ref operator=(explicit auto[] ref l, explicit auto ref r);\r\n\
auto ref __aaassignrev(auto ref l, auto[] ref r);\r\n\
auto ref operator[](explicit auto[] ref l, int index);\r\n\
// const string implementation\r\n\
class const_string\r\n\
{\r\n\
	char[] arr;\r\n\
	int size{ get{ return arr.size; } };\r\n\
}\r\n\
auto operator=(const_string ref l, char[] arr)\r\n\
{\r\n\
	l.arr = arr;\r\n\
	return l;\r\n\
}\r\n\
const_string const_string(char[] arr)\r\n\
{\r\n\
	const_string ret = arr;\r\n\
	return ret;\r\n\
}\r\n\
char operator[](const_string ref l, int index)\r\n\
{\r\n\
	return l.arr[index];\r\n\
}\r\n\
int operator==(const_string a, b){ return a.arr == b.arr; }\r\n\
int operator==(const_string a, char[] b){ return a.arr == b; }\r\n\
int operator==(char[] a, const_string b){ return a == b.arr; }\r\n\
int operator!=(const_string a, b){ return a.arr != b.arr; }\r\n\
int operator!=(const_string a, char[] b){ return a.arr != b; }\r\n\
int operator!=(char[] a, const_string b){ return a != b.arr; }\r\n\
const_string operator+(const_string a, b){ return const_string(a.arr + b.arr); }\r\n\
const_string operator+(const_string a, char[] b){ return const_string(a.arr + b); }\r\n\
const_string operator+(char[] a, const_string b){ return const_string(a + b.arr); }\r\n\
\r\n\
int isStackPointer(auto ref x);\r\n\
\r\n\
// list comprehension helper functions\r\n\
void auto_array_impl(auto[] ref arr, typeid type, int count);\r\n\
auto[] auto_array(typeid type, int count)\r\n\
{\r\n\
	auto[] res;\r\n\
	auto_array_impl(&res, type, count);\r\n\
	return res;\r\n\
}\r\n\
typedef auto[] auto_array;\r\n\
// function will set auto[] element to the specified one, with data reallocation if neccessary\r\n\
void auto_array:set(auto ref x, int pos);\r\n\
void __force_size(auto[] ref s, int size);\r\n\
\r\n\
int isCoroutineReset(auto ref f);\r\n\
void __assertCoroutine(auto ref f);\r\n\
auto ref[] __getFinalizeList();\r\n\
class __FinalizeProxy{ void finalize(){} }\r\n\
void	__finalizeObjects()\r\n\
{\r\n\
	auto l = __getFinalizeList();\r\n\
	for(i in l)\r\n\
		i.finalize();\r\n\
}\r\n\
bool operator in(generic x, typeof(x)[] arr)\r\n\
{\r\n\
	for(i in arr)\r\n\
		if(i == x)\r\n\
			return true;\r\n\
	return false;\r\n\
}\r\n\
void ref assert_derived_from_base(void ref derived, typeid base);\r\n\
auto __gen_list(@T ref() y)\r\n\
{\r\n\
	auto[] res = auto_array(T, 1);\r\n\
	int pos = 0;\r\n\
	for(T x in y)\r\n\
		res.set(&x, pos++);\r\n\
	__force_size(&res, pos);\r\n\
	\r\n\
	T[] r = res;\r\n\
	return r;\r\n\
}\r\n\
void __init_array(@T[] arr)\r\n\
{\r\n\
	for(int i = 0; i < arr.size; i++)\r\n\
	{\r\n\
		@if(T.isArray)\r\n\
			__init_array(arr[i]);\r\n\
		else\r\n\
			arr[i].T();\r\n\
	}\r\n\
}\r\n\
void __closeUpvalue(void ref ref l, void ref v, int offset, int size);";

bool BuildBaseModule(Allocator *allocator)
{
	const char *errorPos = NULL;
	char errorBuf[256];

	if(!BuildModuleFromSource(allocator, "$base$.nc", nullcBaseCode, unsigned(strlen(nullcBaseCode)), &errorPos, errorBuf, 256, ArrayView<InplaceStr>()))
	{
		assert("Failed to compile base NULLC module");
		return false;
	}

#ifndef NULLC_NO_EXECUTOR
	nullcBindModuleFunction("$base$", (void (*)())NULLC::Assert, "assert", 0);
	nullcBindModuleFunction("$base$", (void (*)())NULLC::Assert2, "assert", 1);

	nullcBindModuleFunction("$base$", (void (*)())NULLC::StrEqual, "==", 0);
	nullcBindModuleFunction("$base$", (void (*)())NULLC::StrNEqual, "!=", 0);
	nullcBindModuleFunction("$base$", (void (*)())NULLC::StrConcatenate, "+", 0);
	nullcBindModuleFunction("$base$", (void (*)())NULLC::StrConcatenateAndSet, "+=", 0);

	nullcBindModuleFunction("$base$", (void (*)())NULLC::Int, "bool", 0);
	nullcBindModuleFunction("$base$", (void (*)())NULLC::Char, "char", 0);
	nullcBindModuleFunction("$base$", (void (*)())NULLC::Short, "short", 0);
	nullcBindModuleFunction("$base$", (void (*)())NULLC::Int, "int", 0);
	nullcBindModuleFunction("$base$", (void (*)())NULLC::Long, "long", 0);
	nullcBindModuleFunction("$base$", (void (*)())NULLC::Float, "float", 0);
	nullcBindModuleFunction("$base$", (void (*)())NULLC::Double, "double", 0);

	nullcBindModuleFunction("$base$", (void (*)())NULLC::UnsignedValueChar, "as_unsigned", 0);
	nullcBindModuleFunction("$base$", (void (*)())NULLC::UnsignedValueShort, "as_unsigned", 1);
	nullcBindModuleFunction("$base$", (void (*)())NULLC::UnsignedValueInt, "as_unsigned", 2);

	nullcBindModuleFunction("$base$", (void (*)())NULLC::StrToShort, "short", 1);
	nullcBindModuleFunction("$base$", (void (*)())NULLC::ShortToStr, "short::str", 0);
	nullcBindModuleFunction("$base$", (void (*)())NULLC::StrToInt, "int", 1);
	nullcBindModuleFunction("$base$", (void (*)())NULLC::IntToStr, "int::str", 0);
	nullcBindModuleFunction("$base$", (void (*)())NULLC::StrToLong, "long", 1);
	nullcBindModuleFunction("$base$", (void (*)())NULLC::LongToStr, "long::str", 0);
	nullcBindModuleFunction("$base$", (void (*)())NULLC::StrToFloat, "float", 1);
	nullcBindModuleFunction("$base$", (void (*)())NULLC::FloatToStr, "float::str", 0);
	nullcBindModuleFunction("$base$", (void (*)())NULLC::StrToDouble, "double", 1);
	nullcBindModuleFunction("$base$", (void (*)())NULLC::DoubleToStr, "double::str", 0);

	nullcBindModuleFunction("$base$", (void (*)())NULLC::AllocObject, "__newS", 0);
	nullcBindModuleFunction("$base$", (void (*)())(NULLCArray	(*)(unsigned, unsigned, unsigned))NULLC::AllocArray, "__newA", 0);
	nullcBindModuleFunction("$base$", (void (*)())NULLC::CopyObject, "duplicate", 0);
	nullcBindModuleFunction("$base$", (void (*)())NULLC::CopyArray, "__duplicate_array", 0);
	nullcBindModuleFunction("$base$", (void (*)())NULLC::ReplaceObject, "replace", 0);
	nullcBindModuleFunction("$base$", (void (*)())NULLC::SwapObjects, "swap", 0);
	nullcBindModuleFunction("$base$", (void (*)())NULLC::CompareObjects, "equal", 0);
	nullcBindModuleFunction("$base$", (void (*)())NULLC::AssignObject, "assign", 0);

	nullcBindModuleFunction("$base$", (void (*)())NULLC::ArrayCopy, "array_copy", 0);

	nullcBindModuleFunction("$base$", (void (*)())NULLC::FunctionRedirect, "__redirect", 0);
	nullcBindModuleFunction("$base$", (void (*)())NULLC::FunctionRedirectPtr, "__redirect_ptr", 0);

	nullcBindModuleFunction("$base$", (void (*)())NULLC::Typeid, "typeid", 0);
	nullcBindModuleFunction("$base$", (void (*)())NULLC::TypeSize, "typeid::size$", 0);
	nullcBindModuleFunction("$base$", (void (*)())NULLC::TypesEqual, "==", 1);
	nullcBindModuleFunction("$base$", (void (*)())NULLC::TypesNEqual, "!=", 1);

	nullcBindModuleFunction("$base$", (void (*)())NULLC::RefCompare, "__rcomp", 0);
	nullcBindModuleFunction("$base$", (void (*)())NULLC::RefNCompare, "__rncomp", 0);
	nullcBindModuleFunction("$base$", (void (*)())NULLC::RefLCompare, "<", 0);
	nullcBindModuleFunction("$base$", (void (*)())NULLC::RefLECompare, "<=", 0);
	nullcBindModuleFunction("$base$", (void (*)())NULLC::RefGCompare, ">", 0);
	nullcBindModuleFunction("$base$", (void (*)())NULLC::RefGECompare, ">=", 0);

	nullcBindModuleFunction("$base$", (void (*)())NULLC::RefHash, "hash_value", 0);

	nullcBindModuleFunction("$base$", (void (*)())NULLC::FuncCompare, "__pcomp", 0);
	nullcBindModuleFunction("$base$", (void (*)())NULLC::FuncNCompare, "__pncomp", 0);

	nullcBindModuleFunction("$base$", (void (*)())NULLC::ArrayCompare, "__acomp", 0);
	nullcBindModuleFunction("$base$", (void (*)())NULLC::ArrayNCompare, "__ancomp", 0);

	nullcBindModuleFunction("$base$", (void (*)())NULLC::TypeCount, "__typeCount", 0);

	nullcBindModuleFunction("$base$", (void (*)())NULLC::AutoArrayAssign, "=", 3);
	nullcBindModuleFunction("$base$", (void (*)())NULLC::AutoArrayAssignRev, "__aaassignrev", 0);
	nullcBindModuleFunction("$base$", (void (*)())NULLC::AutoArrayIndex, "[]", 0);

	nullcBindModuleFunction("$base$", (void (*)())IsPointerUnmanaged, "isStackPointer", 0);

	nullcBindModuleFunction("$base$", (void (*)())NULLC::AutoArray, "auto_array_impl", 0);
	nullcBindModuleFunction("$base$", (void (*)())NULLC::AutoArraySet, "auto[]::set", 0);
	nullcBindModuleFunction("$base$", (void (*)())NULLC::ShrinkAutoArray, "__force_size", 0);

	nullcBindModuleFunction("$base$", (void (*)())NULLC::IsCoroutineReset, "isCoroutineReset", 0);
	nullcBindModuleFunction("$base$", (void (*)())NULLC::AssertCoroutine, "__assertCoroutine", 0);

	nullcBindModuleFunction("$base$", (void (*)())NULLC::GetFinalizationList, "__getFinalizeList", 0);

	nullcBindModuleFunction("$base$", (void (*)())NULLC::AssertDerivedFromBase, "assert_derived_from_base", 0);

	nullcBindModuleFunction("$base$", (void (*)())NULLC::CloseUpvalue, "__closeUpvalue", 0);
#endif

	return true;
}

unsigned GetErrorLocationLineNumber(const char *codeStart, const char *errorPos)
{
	if(!codeStart)
		return 0;

	if(!errorPos)
		return 0;

	int line = 1;
	for(const char *pos = codeStart; pos < errorPos; pos++)
		line += *pos == '\n';

	return line;
}

void AddErrorLocationInfo(const char *codeStart, const char *errorPos, char *errorBuf, unsigned errorBufSize)
{
	if(!codeStart)
		return;

	if(!errorPos)
		return;

	if(!errorBuf)
		return;

	const char *codeEnd = codeStart + strlen(codeStart);

	if(errorPos < codeStart || errorPos > codeEnd)
		return;

	const char *start = errorPos;
	const char *end = start == codeEnd ? start : start + 1;

	while(start > codeStart && *(start - 1) != '\r' && *(start - 1) != '\n')
		start--;

	while(*end && *end != '\r' && *end != '\n')
		end++;

	char *errorCurr = errorBuf + strlen(errorBuf);

	if(errorBufSize > unsigned(errorCurr - errorBuf))
		*(errorCurr++) = '\n';

	char *errorCurrBefore = errorCurr;

	if(unsigned line = GetErrorLocationLineNumber(codeStart, errorPos))
		errorCurr += SafeSprintf(errorCurr, errorBufSize - unsigned(errorCurr - errorBuf), "  at line %d: '", line);
	else
		errorCurr += SafeSprintf(errorCurr, errorBufSize - unsigned(errorCurr - errorBuf), "  at '");

	unsigned spacing = unsigned(errorCurr - errorCurrBefore);

	for(const char *pos = start; *pos && pos < end; pos++)
	{
		if(errorBufSize > unsigned(errorCurr - errorBuf))
			*(errorCurr++) = *pos == '\t' ? ' ' : *pos;
	}

	errorCurr += SafeSprintf(errorCurr, errorBufSize - unsigned(errorCurr - errorBuf), "'\n");

	for(unsigned i = 0; i < spacing + unsigned(errorPos - start); i++)
	{
		if(errorBufSize > unsigned(errorCurr - errorBuf))
			*(errorCurr++) = ' ';
	}

	errorCurr += SafeSprintf(errorCurr, errorBufSize - unsigned(errorCurr - errorBuf), "^\n");
}

bool HasSourceCode(ByteCode *bytecode, const char *position)
{
	char *code = FindSource(bytecode);
	unsigned length = bytecode->sourceSize;

	if(position >= code && position <= code + length)
		return true;

	return false;
}

InplaceStr FindModuleNameWithSourceLocation(ExpressionContext &ctx, const char *position)
{
	for(unsigned i = 0; i < ctx.imports.size(); i++)
	{
		if(HasSourceCode(ctx.imports[i]->bytecode, position))
			return ctx.imports[i]->name;
	}

	return InplaceStr();
}

const char* FindModuleCodeWithSourceLocation(ExpressionContext &ctx, const char *position)
{
	if(position >= ctx.code && position <= ctx.code + strlen(ctx.code))
		return ctx.code;

	for(unsigned i = 0; i < ctx.imports.size(); i++)
	{
		if(HasSourceCode(ctx.imports[i]->bytecode, position))
			return FindSource(ctx.imports[i]->bytecode);
	}

	return NULL;
}

ExprModule* AnalyzeModuleFromSource(CompilerContext &ctx, const char *code)
{
	ctx.code = code;

	ParseContext &parseCtx = ctx.parseCtx;

	parseCtx.bytecodeBuilder = BuildModuleFromPath;

	parseCtx.errorBuf = ctx.errorBuf;
	parseCtx.errorBufSize = ctx.errorBufSize;

	ctx.synModule = Parse(parseCtx, ctx.code);

	if(ctx.enableLogFiles && ctx.synModule)
	{
		assert(!ctx.outputCtx.stream);
		ctx.outputCtx.stream = ctx.outputCtx.openStream("syntax_graph.txt");
		
		if(ctx.outputCtx.stream)
		{
			ParseGraphContext parseGraphCtx(ctx.outputCtx);

			PrintGraph(parseGraphCtx, ctx.synModule, "");

			ctx.outputCtx.closeStream(ctx.outputCtx.stream);
			ctx.outputCtx.stream = NULL;
		}
	}

	if(!ctx.synModule)
	{
		if(parseCtx.errorPos)
			ctx.errorPos = parseCtx.errorPos;

		return NULL;
	}

	//printf("# Parse memory %dkb\n", ctx.allocator->requested() / 1024);

	ExpressionContext &exprCtx = ctx.exprCtx;

	exprCtx.errorBuf = ctx.errorBuf;
	exprCtx.errorBufSize = ctx.errorBufSize;

	if(parseCtx.errorPos)
	{
		unsigned errorLength = unsigned(strlen(exprCtx.errorBuf));

		exprCtx.errorBuf += errorLength;
		exprCtx.errorBufSize -= errorLength;
	}

	ctx.exprModule = Analyze(exprCtx, ctx.synModule, ctx.code);

	if(ctx.enableLogFiles && ctx.exprModule)
	{
		assert(!ctx.outputCtx.stream);
		ctx.outputCtx.stream = ctx.outputCtx.openStream("expr_graph.txt");

		if(ctx.outputCtx.stream)
		{
			ExpressionGraphContext exprGraphCtx(ctx.outputCtx);

			PrintGraph(exprGraphCtx, ctx.exprModule, "");

			ctx.outputCtx.closeStream(ctx.outputCtx.stream);
			ctx.outputCtx.stream = NULL;
		}
	}

	if(!ctx.exprModule || exprCtx.errorCount != 0 || parseCtx.errorCount != 0)
	{
		if(parseCtx.errorPos)
			ctx.errorPos = parseCtx.errorPos;
		else if(exprCtx.errorPos)
			ctx.errorPos = exprCtx.errorPos;

		return NULL;
	}

	//printf("# Compile memory %dkb\n", ctx.allocator->requested() / 1024);

	return ctx.exprModule;
}

bool CompileModuleFromSource(CompilerContext &ctx, const char *code)
{
	if(!AnalyzeModuleFromSource(ctx, code))
		return false;

	ExpressionContext &exprCtx = ctx.exprCtx;

	ctx.vmModule = CompileVm(exprCtx, ctx.exprModule, ctx.code);

	if(!ctx.vmModule)
	{
		ctx.errorPos = NULL;

		if(ctx.errorBuf && ctx.errorBufSize)
			SafeSprintf(ctx.errorBuf, ctx.errorBufSize, "ERROR: internal compiler error: failed to create VmModule");

		return false;
	}

	//printf("# Instruction memory %dkb\n", pool.GetSize() / 1024);

	if(ctx.enableLogFiles)
	{
		assert(!ctx.outputCtx.stream);
		ctx.outputCtx.stream = ctx.outputCtx.openStream("inst_graph.txt");

		if(ctx.outputCtx.stream)
		{
			InstructionVMGraphContext instGraphCtx(ctx.outputCtx);

			instGraphCtx.showUsers = true;
			instGraphCtx.displayAsTree = false;
			instGraphCtx.showFullTypes = false;
			instGraphCtx.showSource = true;

			PrintGraph(instGraphCtx, ctx.vmModule);

			ctx.outputCtx.closeStream(ctx.outputCtx.stream);
			ctx.outputCtx.stream = NULL;
		}
	}

	RunVmPass(exprCtx, ctx.vmModule, VM_PASS_OPT_PEEPHOLE);
	RunVmPass(exprCtx, ctx.vmModule, VM_PASS_OPT_CONSTANT_PROPAGATION);
	RunVmPass(exprCtx, ctx.vmModule, VM_PASS_OPT_DEAD_CODE_ELIMINATION);
	RunVmPass(exprCtx, ctx.vmModule, VM_PASS_OPT_CONTROL_FLOW_SIPLIFICATION);
	RunVmPass(exprCtx, ctx.vmModule, VM_PASS_OPT_DEAD_CODE_ELIMINATION);

	for(unsigned i = 0; i < 6; i++)
	{
		RunVmPass(exprCtx, ctx.vmModule, VM_PASS_OPT_CONSTANT_PROPAGATION);
		RunVmPass(exprCtx, ctx.vmModule, VM_PASS_OPT_LOAD_STORE_PROPAGATION);
		RunVmPass(exprCtx, ctx.vmModule, VM_PASS_OPT_COMMON_SUBEXPRESSION_ELIMINATION);
		RunVmPass(exprCtx, ctx.vmModule, VM_PASS_OPT_PEEPHOLE);
		RunVmPass(exprCtx, ctx.vmModule, VM_PASS_OPT_DEAD_CODE_ELIMINATION);
		RunVmPass(exprCtx, ctx.vmModule, VM_PASS_OPT_CONTROL_FLOW_SIPLIFICATION);
		RunVmPass(exprCtx, ctx.vmModule, VM_PASS_OPT_DEAD_CODE_ELIMINATION);
	}

	RunVmPass(exprCtx, ctx.vmModule, VM_PASS_CREATE_ALLOCA_STORAGE);

	if(ctx.enableLogFiles)
	{
		assert(!ctx.outputCtx.stream);
		ctx.outputCtx.stream = ctx.outputCtx.openStream("inst_graph_opt.txt");

		if(ctx.outputCtx.stream)
		{
			InstructionVMGraphContext instGraphCtx(ctx.outputCtx);

			instGraphCtx.showUsers = true;
			instGraphCtx.displayAsTree = false;
			instGraphCtx.showFullTypes = false;
			instGraphCtx.showSource = true;

			PrintGraph(instGraphCtx, ctx.vmModule);

			ctx.outputCtx.closeStream(ctx.outputCtx.stream);
			ctx.outputCtx.stream = NULL;
		}
	}

	RunVmPass(exprCtx, ctx.vmModule, VM_PASS_LEGALIZE_VM);

	if(ctx.enableLogFiles)
	{
		assert(!ctx.outputCtx.stream);
		ctx.outputCtx.stream = ctx.outputCtx.openStream("inst_graph_legal.txt");

		if(ctx.outputCtx.stream)
		{
			InstructionVMGraphContext instGraphCtx(ctx.outputCtx);

			instGraphCtx.showUsers = false;
			instGraphCtx.displayAsTree = false;
			instGraphCtx.showFullTypes = false;
			instGraphCtx.showSource = true;

			PrintGraph(instGraphCtx, ctx.vmModule);

			ctx.outputCtx.closeStream(ctx.outputCtx.stream);
			ctx.outputCtx.stream = NULL;
		}
	}

	ctx.vmLoweredModule = LowerModule(exprCtx, ctx.vmModule);

	if(ctx.enableLogFiles)
	{
		assert(!ctx.outputCtx.stream);
		ctx.outputCtx.stream = ctx.outputCtx.openStream("inst_graph_low.txt");

		if(ctx.outputCtx.stream)
		{
			InstructionVmLowerGraphContext instLowerGraphCtx(ctx.outputCtx);

			instLowerGraphCtx.showSource = true;

			PrintGraph(instLowerGraphCtx, ctx.vmLoweredModule);

			ctx.outputCtx.closeStream(ctx.outputCtx.stream);
			ctx.outputCtx.stream = NULL;
		}
	}

	OptimizeTemporaryRegisterSpills(ctx.vmLoweredModule);

	if(ctx.enableLogFiles)
	{
		assert(!ctx.outputCtx.stream);
		ctx.outputCtx.stream = ctx.outputCtx.openStream("inst_graph_low_opt.txt");

		if(ctx.outputCtx.stream)
		{
			InstructionVmLowerGraphContext instLowerGraphCtx(ctx.outputCtx);

			instLowerGraphCtx.showSource = true;

			PrintGraph(instLowerGraphCtx, ctx.vmLoweredModule);

			ctx.outputCtx.closeStream(ctx.outputCtx.stream);
			ctx.outputCtx.stream = NULL;
		}
	}

	FinalizeRegisterSpills(ctx.exprCtx, ctx.vmLoweredModule);

	InstructionVmFinalizeContext &instFinalizeCtx = ctx.instFinalizeCtx;

	FinalizeModule(instFinalizeCtx, ctx.vmLoweredModule);

	if(ctx.enableLogFiles)
	{
		assert(!ctx.outputCtx.stream);
		ctx.outputCtx.stream = ctx.outputCtx.openStream("inst_vm.txt");

		if(ctx.outputCtx.stream)
		{
			InstructionVmLowerGraphContext instLowerGraphCtx(ctx.outputCtx);

			instLowerGraphCtx.showSource = true;

			PrintInstructions(instLowerGraphCtx, instFinalizeCtx, ctx.code);

			ctx.outputCtx.closeStream(ctx.outputCtx.stream);
			ctx.outputCtx.stream = NULL;
		}
	}

	return true;
}

bool CreateExternalInfo(ExternFuncInfo &fInfo, FunctionData &refFunc, ExternTypeInfo *typeArray)
{
	unsigned int rCount = 0, fCount = 0;
	unsigned int rMaxCount = sizeof(fInfo.rOffsets) / sizeof(fInfo.rOffsets[0]);
	unsigned int fMaxCount = sizeof(fInfo.fOffsets) / sizeof(fInfo.fOffsets[0]);

	// parse all parameters, fill offsets
	unsigned int offset = 0;

	fInfo.ps3Callable = 1;

	for(unsigned i = 0; i < refFunc.arguments.size(); i++)
	{
		ArgumentData &argument = refFunc.arguments[i];

		ExternTypeInfo &type = typeArray[argument.type->typeIndex];

		ExternTypeInfo::TypeCategory category = type.type;

		if(category == ExternTypeInfo::TYPE_COMPLEX)
		{
			if(type.size <= 4)
				category = ExternTypeInfo::TYPE_INT;
			else if(type.size <= 8)
				category = ExternTypeInfo::TYPE_LONG;
		}

		if(fCount >= fMaxCount || rCount >= rMaxCount) // too many f/r parameters
		{
			fInfo.ps3Callable = 0;
			break;
		}

		switch(category)
		{
		case ExternTypeInfo::TYPE_CHAR:
		case ExternTypeInfo::TYPE_SHORT:
		case ExternTypeInfo::TYPE_INT:
		case ExternTypeInfo::TYPE_LONG:
			if(type.subCat == ExternTypeInfo::CAT_POINTER)
				fInfo.rOffsets[rCount++] = offset | 2u << 30u;
			else
				fInfo.rOffsets[rCount++] = offset | (category == ExternTypeInfo::TYPE_LONG ? 1u : 0u) << 30u;

			offset += category == ExternTypeInfo::TYPE_LONG ? 2 : 1;
			break;
		case ExternTypeInfo::TYPE_FLOAT:
		case ExternTypeInfo::TYPE_DOUBLE:
			fInfo.rOffsets[rCount++] = offset;
			fInfo.fOffsets[fCount++] = offset | (category == ExternTypeInfo::TYPE_FLOAT ? 1u : 0u) << 31u;

			offset += category == ExternTypeInfo::TYPE_DOUBLE ? 2 : 1;
			break;
		default:
			if(rCount + type.size / 8 >= rMaxCount)
			{
				fInfo.ps3Callable = 0;
				break;
			}

			for(unsigned i = 0; i < type.size / 8; i++)
			{
				fInfo.rOffsets[rCount++] = offset | 1u << 30u;
				offset += 2;
			}
			break;
		}
	}

	if(refFunc.contextVariable)
	{
		if(fCount >= fMaxCount || rCount >= rMaxCount) // too many f/r parameters
		{
			fInfo.ps3Callable = 0;
		}
		else
		{
			fInfo.rOffsets[rCount++] = offset | 2u << 30u;
			offset += 1;
		}
	}

	// clear remaining offsets
	for(unsigned int i = rCount; i < rMaxCount; ++i)
		fInfo.rOffsets[i] = 0;

	for(unsigned int i = fCount; i < fMaxCount; ++i)
		fInfo.fOffsets[i] = 0;

	return true;
}

unsigned GetBytecode(CompilerContext &ctx, char **bytecode)
{
	// find out the size of generated bytecode
	unsigned size = sizeof(ByteCode);

	size += ctx.exprCtx.types.size() * sizeof(ExternTypeInfo);

	unsigned symbolStorageSize = 0;
	unsigned allMemberCount = 0, allConstantCount = 0;
	unsigned typedefCount = 0;

	for(unsigned i = 0; i < ctx.exprCtx.types.size(); i++)
	{
		TypeBase *type = ctx.exprCtx.types[i];

		type->typeIndex = i;

		symbolStorageSize += type->name.length() + 1;

		if(isType<TypeUnsizedArray>(type))
			continue;

		else if(TypeClass *typeClass = getType<TypeClass>(type))
		{
			for(MatchData *curr = typeClass->generics.head; curr; curr = curr->next)
			{
				typedefCount++;

				symbolStorageSize += curr->name->name.length() + 1;
			}

			for(MatchData *curr = typeClass->aliases.head; curr; curr = curr->next)
			{
				typedefCount++;

				symbolStorageSize += curr->name->name.length() + 1;
			}

			for(VariableHandle *curr = typeClass->members.head; curr; curr = curr->next)
			{
				if(curr->variable->type->hasPointers)
					allMemberCount++;

				if(*curr->variable->name->name.begin == '$')
					continue;

				allMemberCount++;

				symbolStorageSize += curr->variable->name->name.length() + 1;
			}

			for(ConstantData *curr = typeClass->constants.head; curr; curr = curr->next)
			{
				allConstantCount++;

				symbolStorageSize += curr->name->name.length() + 1;
			}
		}
		else if(TypeStruct *typeStruct = getType<TypeStruct>(type))
		{
			for(VariableHandle *curr = typeStruct->members.head; curr; curr = curr->next)
			{
				if(curr->variable->type->hasPointers)
					allMemberCount++;

				if(*curr->variable->name->name.begin == '$')
					continue;

				allMemberCount++;

				symbolStorageSize += curr->variable->name->name.length() + 1;
			}

			for(ConstantData *curr = typeStruct->constants.head; curr; curr = curr->next)
			{
				allConstantCount++;

				symbolStorageSize += curr->name->name.length() + 1;
			}
		}
		else if(TypeFunction *typeFunction = getType<TypeFunction>(type))
		{
			allMemberCount += typeFunction->arguments.size() + 1;
		}
		else if(TypeGenericClass *typeGenericClass = getType<TypeGenericClass>(type))
		{
			for(TypeHandle *curr = typeGenericClass->generics.head; curr; curr = curr->next)
			{
				typedefCount++;

				symbolStorageSize += 1;
			}
		}
	}

	size += allMemberCount * sizeof(ExternMemberInfo);

	unsigned offsetToConstants = size;
	size += allConstantCount * sizeof(ExternConstantInfo);

	unsigned offsetToModule = size;
	size += ctx.exprCtx.imports.size() * sizeof(ExternModuleInfo);

	for(unsigned i = 0; i < ctx.exprCtx.imports.size(); i++)
	{
		ModuleData *module = ctx.exprCtx.imports[i];

		symbolStorageSize += module->name.length() + 1;
	}

	unsigned offsetToVar = size;
	unsigned globalVarCount = 0, exportVarCount = 0;
	unsigned externVariableInfoCount = 0;

	for(unsigned i = 0; i < ctx.exprCtx.variables.size(); i++)
	{
		VariableData *variable = ctx.exprCtx.variables[i];

		if(variable->importModule != NULL)
			continue;

		if(!ctx.exprCtx.GlobalScopeFrom(variable->scope))
			continue;

		externVariableInfoCount++;

		globalVarCount++;

		if(variable->scope == ctx.exprCtx.globalScope || variable->scope->ownerNamespace)
			exportVarCount++;

		symbolStorageSize += variable->name->name.length() + 1;
	}

	for(unsigned i = 0; i < ctx.exprCtx.functions.size(); i++)
	{
		FunctionData *function = ctx.exprCtx.functions[i];

		if(function->importModule != NULL)
			continue;

		for(MatchData *curr = function->generics.head; curr; curr = curr->next)
		{
			externVariableInfoCount++;

			symbolStorageSize += curr->name->name.length() + 1;
		}
	}

	size += externVariableInfoCount * sizeof(ExternVarInfo);

	unsigned offsetToFunc = size;

	unsigned localCount = 0;

	unsigned exportedFunctionCount = 0;

	for(unsigned i = 0; i < ctx.exprCtx.functions.size(); i++)
	{
		FunctionData *function = ctx.exprCtx.functions[i];

		if(function->importModule != NULL)
			continue;

		exportedFunctionCount++;

		symbolStorageSize += function->name->name.length() + 1;

		localCount += function->arguments.size();

		for(unsigned i = 0; i < function->arguments.size(); i++)
		{
			ArgumentData argument = function->arguments[i];

			symbolStorageSize += argument.name->name.length() + 1;
		}

		if(VariableData *variable = function->contextArgument)
		{
			localCount++;

			symbolStorageSize += variable->name->name.length() + 1;
		}

		if (!ctx.exprCtx.IsGenericFunction(function))
		{
			for(unsigned i = 0; i < function->functionScope->allVariables.size(); i++)
			{
				VariableData *variable = function->functionScope->allVariables[i];

				VariableHandle *argumentVariable = NULL;

				for(VariableHandle *curr = function->argumentVariables.head; curr; curr = curr->next)
				{
					if(curr->variable == variable)
					{
						argumentVariable = curr;
						break;
					}
				}

				if(argumentVariable || variable == function->contextArgument)
					continue;

				if(variable->isAlloca && variable->users.empty())
					continue;

				if(variable->lookupOnly)
					continue;

				localCount++;

				symbolStorageSize += variable->name->name.length() + 1;
			}
		}

		localCount += function->upvalues.size();

		for(unsigned i = 0; i < function->upvalues.size(); i++)
		{
			UpvalueData *upvalue = function->upvalues[i];

			symbolStorageSize += upvalue->variable->name->name.length() + 1;
		}
	}

	size += exportedFunctionCount * sizeof(ExternFuncInfo);

	unsigned offsetToFirstLocal = size;

	size += localCount * sizeof(ExternLocalInfo);

	for(unsigned i = 0; i < ctx.exprCtx.globalScope->aliases.size(); i++)
	{
		AliasData *alias = ctx.exprCtx.globalScope->aliases[i];

		typedefCount++;

		symbolStorageSize += alias->name->name.length() + 1;
	}

	unsigned offsetToTypedef = size;
	size += typedefCount * sizeof(ExternTypedefInfo);

	for(unsigned i = 0; i < ctx.exprCtx.namespaces.size(); i++)
	{
		NamespaceData *nameSpace = ctx.exprCtx.namespaces[i];

		symbolStorageSize += nameSpace->name.name.length() + 1;
	}

	unsigned offsetToNamespace = size;
	size += ctx.exprCtx.namespaces.size() * sizeof(ExternNamespaceInfo);

	unsigned offsetToCode = size;
	size += ctx.instFinalizeCtx.cmds.size() * sizeof(VMCmd);

	unsigned sourceLength = (unsigned)strlen(ctx.code) + 1;

	unsigned infoCount = 0;

	for(unsigned i = 0; i < ctx.instFinalizeCtx.locations.size(); i++)
	{
		SynBase *location = ctx.instFinalizeCtx.locations[i];

		if(location && location->pos.begin >= ctx.code && location->pos.begin < ctx.code + sourceLength)
			infoCount++;
	}

	unsigned offsetToInfo = size;
	size += sizeof(unsigned) * 2 * infoCount;

	unsigned offsetToSymbols = size;
	size += symbolStorageSize;

	unsigned offsetToSource = size;
	size += sourceLength;

#ifdef NULLC_LLVM_SUPPORT
	// TODO: llvm support
#endif

#ifdef VERBOSE_DEBUG_OUTPUT
	printf("Statistics. Overall: %d bytes\r\n", size);
	printf("Types: %db, ", offsetToModule - sizeof(ByteCode));
	printf("Modules: %db, ", offsetToVar - offsetToModule);
	printf("Variables: %db, ", offsetToFunc - offsetToVar);
	printf("Functions: %db\r\n", offsetToFirstLocal - offsetToFunc);
	printf("Locals: %db, ", offsetToCode - offsetToFirstLocal);
	printf("Code: %db, ", offsetToInfo - offsetToCode);
	printf("Code info: %db, ", offsetToSymbols - offsetToInfo);
	printf("Symbols: %db, ", offsetToSource - offsetToSymbols);
	printf("Source: %db\r\n\r\n", size - offsetToSource);
#endif

	*bytecode = new char[size];
	memset(*bytecode, 0, size);

	ByteCode *code = (ByteCode*)(*bytecode);
	code->size = size;

	code->typeCount = (unsigned)ctx.exprCtx.types.size();

	code->dependsCount = ctx.exprCtx.imports.size();
	code->offsetToFirstModule = offsetToModule;

	code->globalVarSize = (unsigned)ctx.exprCtx.globalScope->dataSize;
	// Make sure modules that are linked together will not break global variable alignment
	code->globalVarSize = (code->globalVarSize + 0xf) & ~0xf;

	code->variableCount = globalVarCount;
	code->variableExportCount = exportVarCount;
	code->offsetToFirstVar = offsetToVar;

	code->functionCount = ctx.exprCtx.functions.size();
	code->moduleFunctionCount = ctx.exprCtx.functions.size() - exportedFunctionCount;
	code->offsetToFirstFunc = offsetToFunc;

	code->localCount = localCount;
	code->offsetToLocals = offsetToFirstLocal;

	code->closureListCount = 0;

	code->codeSize = ctx.instFinalizeCtx.cmds.size();
	code->offsetToCode = offsetToCode;

	code->symbolLength = symbolStorageSize;
	code->offsetToSymbols = offsetToSymbols;

	code->offsetToConstants = offsetToConstants;

	code->namespaceCount = ctx.exprCtx.namespaces.size();
	code->offsetToNamespaces = offsetToNamespace;

	VectorView<char> debugSymbols(FindSymbols(code), symbolStorageSize);

	VectorView<ExternTypeInfo> tInfo(FindFirstType(code), code->typeCount);
	VectorView<ExternMemberInfo> memberList(FindFirstMember(code), allMemberCount);
	VectorView<ExternConstantInfo> constantList(FindFirstConstant(code), allConstantCount);

	for(unsigned i = 0; i < ctx.exprCtx.types.size(); i++)
	{
		TypeBase *type = ctx.exprCtx.types[i];

		ExternTypeInfo &target = tInfo.push_back();

		target.offsetToName = debugSymbols.count;
		debugSymbols.push_back(type->name.begin, type->name.length());
		debugSymbols.push_back(0);

		target.nameHash = type->nameHash;

		target.size = (unsigned)type->size;
		target.defaultAlign = (unsigned char)type->alignment;
		target.padding = type->padding;

		target.constantCount = 0;

		target.memberCount = 0;
		target.memberOffset = 0;

		target.pointerCount = type->hasPointers ? 1 : 0;

		target.definitionModule = ~0u;
		target.definitionOffsetStart = ~0u;
		target.definitionOffset = ~0u;
		target.genericTypeCount = 0;

		target.typeFlags = 0;

		if(type->isGeneric)
			target.typeFlags |= ExternTypeInfo::TYPE_DEPENDS_ON_GENERIC;

		target.namespaceHash = ~0u;

		if(TypeFunction *typeFunction = getType<TypeFunction>(type))
		{
			target.type = ExternTypeInfo::TYPE_COMPLEX;

			target.subCat = ExternTypeInfo::CAT_FUNCTION;
			target.memberCount = typeFunction->arguments.size();
			target.memberOffset = memberList.count;

			ExternMemberInfo &member = memberList.push_back();

			member.type = typeFunction->returnType->typeIndex;
			member.offset = 0;

			for(TypeHandle *curr = typeFunction->arguments.head; curr; curr = curr->next)
			{
				ExternMemberInfo &member = memberList.push_back();

				member.type = curr->type->typeIndex;
				member.offset = 0;
			}
		}
		else if(TypeArray *typeArray = getType<TypeArray>(type))
		{
			target.type = ExternTypeInfo::TYPE_COMPLEX;

			target.subCat = ExternTypeInfo::CAT_ARRAY;
			target.arrSize = (unsigned)typeArray->length;
			target.subType = typeArray->subType->typeIndex;
		}
		else if(TypeUnsizedArray *typeUnsizedArray = getType<TypeUnsizedArray>(type))
		{
			target.type = ExternTypeInfo::TYPE_COMPLEX;

			target.subCat = ExternTypeInfo::CAT_ARRAY;
			target.arrSize = ~0u;
			target.subType = typeUnsizedArray->subType->typeIndex;
		}
		else if(TypeRef *typeRef = getType<TypeRef>(type))
		{
			target.type = sizeof(void*) == 4 ? ExternTypeInfo::TYPE_INT : ExternTypeInfo::TYPE_LONG;

			target.subCat = ExternTypeInfo::CAT_POINTER;
			target.subType = typeRef->subType->typeIndex;
		}
		else if(TypeStruct *typeStruct = getType<TypeStruct>(type))
		{
			target.type = ExternTypeInfo::TYPE_COMPLEX;

			if(TypeEnum *typeEnum = getType<TypeEnum>(type))
			{
				target.type = ExternTypeInfo::TYPE_INT;

				if(ScopeData *scope = ctx.exprCtx.NamespaceScopeFrom(typeEnum->scope))
					target.namespaceHash = scope->ownerNamespace->fullNameHash;
			}
			else if(TypeClass *typeClass = getType<TypeClass>(type))
			{
				if(ScopeData *scope = ctx.exprCtx.NamespaceScopeFrom(typeClass->scope))
					target.namespaceHash = scope->ownerNamespace->fullNameHash;

				if(typeClass->extendable)
					target.typeFlags |= ExternTypeInfo::TYPE_IS_EXTENDABLE;

				if(typeClass->hasFinalizer)
					target.typeFlags |= ExternTypeInfo::TYPE_HAS_FINALIZER;

				if(typeClass->baseClass)
					target.baseType = typeClass->baseClass->typeIndex;

				if(!typeClass->generics.empty())
				{
					target.definitionOffset = 0x80000000 | typeClass->proto->typeIndex;

					target.genericTypeCount = typeClass->generics.size();
				}
			}

			target.subCat = ExternTypeInfo::CAT_CLASS;

			target.memberCount = 0;
			target.memberOffset = memberList.count;

			for(VariableHandle *curr = typeStruct->members.head; curr; curr = curr->next)
			{
				if(*curr->variable->name->name.begin == '$')
					continue;

				target.memberCount++;

				ExternMemberInfo &member = memberList.push_back();

				member.type = curr->variable->type->typeIndex;
				member.offset = curr->variable->offset;

				debugSymbols.push_back(curr->variable->name->name.begin, curr->variable->name->name.length());
				debugSymbols.push_back(0);
			}

			for(ConstantData *curr = typeStruct->constants.head; curr; curr = curr->next)
			{
				target.constantCount++;

				ExternConstantInfo &constant = constantList.push_back();

				constant.type = curr->value->type->typeIndex;

				if(ExprBoolLiteral *node = getType<ExprBoolLiteral>(curr->value))
				{
					constant.value = node->value;
				}
				else if(ExprIntegerLiteral *node = getType<ExprIntegerLiteral>(curr->value))
				{
					constant.value = node->value;
				}
				else if(ExprRationalLiteral *node = getType<ExprRationalLiteral>(curr->value))
				{
					memcpy(&constant.value, &node->value, sizeof(node->value));
				}

				debugSymbols.push_back(curr->name->name.begin, curr->name->name.length());
				debugSymbols.push_back(0);
			}

			// Export type pointer members for GC
			target.pointerCount = 0;

			for(VariableHandle *curr = typeStruct->members.head; curr; curr = curr->next)
			{
				if(curr->variable->type->hasPointers)
				{
					target.pointerCount++;

					ExternMemberInfo &member = memberList.push_back();

					member.type = curr->variable->type->typeIndex;
					member.offset = curr->variable->offset;
				}
			}
		}
		else if(TypeGenericClassProto *typeGenericClassProto = getType<TypeGenericClassProto>(type))
		{
			if(ScopeData *scope = ctx.exprCtx.NamespaceScopeFrom(typeGenericClassProto->scope))
				target.namespaceHash = scope->ownerNamespace->fullNameHash;

			target.type = ExternTypeInfo::TYPE_COMPLEX;

			target.subCat = ExternTypeInfo::CAT_CLASS;

			if(ModuleData *moduleData = typeGenericClassProto->importModule)
			{
				target.definitionModule = moduleData->dependencyIndex;
				target.definitionOffsetStart = unsigned(typeGenericClassProto->definition->begin - moduleData->lexStream);
				assert(target.definitionOffsetStart < moduleData->lexStreamSize);
			}
			else
			{
				target.definitionModule = 0;
				target.definitionOffsetStart = unsigned(typeGenericClassProto->definition->begin - ctx.parseCtx.lexer.GetStreamStart());
				assert(target.definitionOffsetStart < ctx.parseCtx.lexer.GetStreamSize());
			}

			target.definitionOffset = ~0u;
		}
		else if(TypeGenericClass *typeGenericClass = getType<TypeGenericClass>(type))
		{
			target.type = ExternTypeInfo::TYPE_COMPLEX;

			target.subCat = ExternTypeInfo::CAT_CLASS;

			target.definitionOffset = 0x80000000 | typeGenericClass->proto->typeIndex;

			target.genericTypeCount = typeGenericClass->generics.size();
		}
		else if(isType<TypeVoid>(type))
		{
			target.type = ExternTypeInfo::TYPE_VOID;

			target.subCat = ExternTypeInfo::CAT_NONE;
		}
		else if(isType<TypeBool>(type))
		{
			target.type = ExternTypeInfo::TYPE_CHAR;

			target.subCat = ExternTypeInfo::CAT_NONE;
		}
		else if(isType<TypeChar>(type))
		{
			target.type = ExternTypeInfo::TYPE_CHAR;

			target.subCat = ExternTypeInfo::CAT_NONE;
		}
		else if(isType<TypeShort>(type))
		{
			target.type = ExternTypeInfo::TYPE_SHORT;

			target.subCat = ExternTypeInfo::CAT_NONE;
		}
		else if(isType<TypeInt>(type))
		{
			target.type = ExternTypeInfo::TYPE_INT;

			target.subCat = ExternTypeInfo::CAT_NONE;
		}
		else if(isType<TypeLong>(type))
		{
			target.type = ExternTypeInfo::TYPE_LONG;

			target.subCat = ExternTypeInfo::CAT_NONE;
		}
		else if(isType<TypeFloat>(type))
		{
			target.type = ExternTypeInfo::TYPE_FLOAT;

			target.subCat = ExternTypeInfo::CAT_NONE;
		}
		else if(isType<TypeDouble>(type))
		{
			target.type = ExternTypeInfo::TYPE_DOUBLE;

			target.subCat = ExternTypeInfo::CAT_NONE;
		}
		else
		{
			target.subCat = ExternTypeInfo::CAT_NONE;
		}
	}

	VectorView<ExternModuleInfo> mInfo(FindFirstModule(code), ctx.exprCtx.imports.size());

	for(unsigned i = 0; i < ctx.exprCtx.imports.size(); i++)
	{
		ModuleData *moduleData = ctx.exprCtx.imports[i];

		ExternModuleInfo &moduleInfo = mInfo.push_back();

		moduleInfo.nameOffset = debugSymbols.count;
		debugSymbols.push_back(moduleData->name.begin, moduleData->name.length());
		debugSymbols.push_back(0);

		moduleInfo.funcStart = moduleData->startingFunctionIndex;
		moduleInfo.funcCount = moduleData->functionCount;

		moduleInfo.variableOffset = 0;
		moduleInfo.nameHash = ~0u;
	}

	VectorView<ExternVarInfo> vInfo(FindFirstVar(code), externVariableInfoCount);

	// Handle visible global variables first
	for(unsigned i = 0; i < ctx.exprCtx.variables.size(); i++)
	{
		VariableData *variable = ctx.exprCtx.variables[i];

		if(variable->importModule != NULL)
			continue;

		if(!ctx.exprCtx.GlobalScopeFrom(variable->scope))
			continue;

		if(variable->scope != ctx.exprCtx.globalScope && !variable->scope->ownerNamespace)
			continue;

		ExternVarInfo &varInfo = vInfo.push_back();

		varInfo.offsetToName = debugSymbols.count;
		debugSymbols.push_back(variable->name->name.begin, variable->name->name.length());
		debugSymbols.push_back(0);

		varInfo.nameHash = variable->nameHash;

		varInfo.type = variable->type->typeIndex;
		varInfo.offset = variable->offset;
	}

	for(unsigned i = 0; i < ctx.exprCtx.variables.size(); i++)
	{
		VariableData *variable = ctx.exprCtx.variables[i];

		if(variable->importModule != NULL)
			continue;

		if(!ctx.exprCtx.GlobalScopeFrom(variable->scope))
			continue;

		if(variable->scope == ctx.exprCtx.globalScope || variable->scope->ownerNamespace)
			continue;

		ExternVarInfo &varInfo = vInfo.push_back();

		varInfo.offsetToName = debugSymbols.count;
		debugSymbols.push_back(variable->name->name.begin, variable->name->name.length());
		debugSymbols.push_back(0);

		varInfo.nameHash = variable->nameHash;

		varInfo.type = variable->type->typeIndex;
		varInfo.offset = variable->offset;
	}

	VectorView<ExternFuncInfo> fInfo(FindFirstFunc(code), exportedFunctionCount);
	VectorView<ExternLocalInfo> localInfo(FindFirstLocal(code), localCount);

	for(unsigned i = 0; i < ctx.exprCtx.functions.size(); i++)
	{
		FunctionData *function = ctx.exprCtx.functions[i];

		if(function->importModule != NULL)
			continue;

		ExternFuncInfo &funcInfo = fInfo.push_back();

		if(function->isPrototype && function->implementation)
		{
			funcInfo.address = function->implementation->vmFunction->address;
			funcInfo.codeSize = function->implementation->functionIndex | 0x80000000;
		}
		else if(function->isPrototype)
		{
			funcInfo.address = 0;
			funcInfo.codeSize = 0;
		}
		else if(function->vmFunction)
		{
			funcInfo.address = function->vmFunction->address;
			funcInfo.codeSize = function->vmFunction->codeSize;
		}
		else
		{
			funcInfo.address = ~0u;
			funcInfo.codeSize = 0;
		}

		funcInfo.funcPtr = 0;

		// Only functions in global or namesapce scope remain visible
		funcInfo.isVisible = (function->scope == ctx.exprCtx.globalScope || function->scope->ownerNamespace) && !function->isHidden;

		funcInfo.nameHash = function->nameHash;

		if(ScopeData *scope = ctx.exprCtx.NamespaceScopeFrom(function->scope))
			funcInfo.namespaceHash = scope->ownerNamespace->fullNameHash;
		else
			funcInfo.namespaceHash = ~0u;

		funcInfo.funcCat = ExternFuncInfo::NORMAL;

		if(function->scope->ownerType)
			funcInfo.funcCat = ExternFuncInfo::THISCALL;
		else if(function->coroutine)
			funcInfo.funcCat = ExternFuncInfo::COROUTINE;
		else if(function->isHidden)
			funcInfo.funcCat = ExternFuncInfo::LOCAL;

		funcInfo.isGenericInstance = ctx.exprCtx.IsGenericInstance(function);

		funcInfo.funcType = function->type->typeIndex;

		if(function->scope->ownerType)
			funcInfo.parentType = function->scope->ownerType->typeIndex;
		else
			funcInfo.parentType = ~0u;

		funcInfo.contextType = function->contextType ? function->contextType->typeIndex : ~0u;

		funcInfo.offsetToFirstLocal = localInfo.count;
		funcInfo.paramCount = function->arguments.size();

		bool isGenericFunction = ctx.exprCtx.IsGenericFunction(function);

		if(!isGenericFunction)
		{
			TypeBase *returnType = function->type->returnType;

			funcInfo.retType = ExternFuncInfo::RETURN_UNKNOWN;

			if(returnType == ctx.exprCtx.typeFloat || returnType == ctx.exprCtx.typeDouble)
				funcInfo.retType = ExternFuncInfo::RETURN_DOUBLE;
#if defined(__linux) && !defined(_M_X64)
			else if(returnType == ctx.exprCtx.typeVoid)
				funcInfo.retType = ExternFuncInfo::RETURN_VOID;
#else
			else if(returnType == ctx.exprCtx.typeVoid || returnType->size == 0)
				funcInfo.retType = ExternFuncInfo::RETURN_VOID;
#endif
#if defined(NULLC_COMPLEX_RETURN) && defined(__linux)
			else if(returnType == ctx.exprCtx.typeBool || returnType == ctx.exprCtx.typeChar || returnType == ctx.exprCtx.typeShort || returnType == ctx.exprCtx.typeInt)
				funcInfo.retType = ExternFuncInfo::RETURN_INT;
			else if(returnType == ctx.exprCtx.typeLong)
				funcInfo.retType = ExternFuncInfo::RETURN_LONG;
#else
			else if(returnType == ctx.exprCtx.typeInt || returnType->size <= 4)
				funcInfo.retType = ExternFuncInfo::RETURN_INT;
			else if(returnType == ctx.exprCtx.typeLong || returnType->size == 8)
				funcInfo.retType = ExternFuncInfo::RETURN_LONG;
#endif

			funcInfo.returnShift = (unsigned char)(returnType->size / 4);

			funcInfo.bytesToPop = (unsigned)function->argumentsSize;

			CreateExternalInfo(funcInfo, *function, FindFirstType(code));

			memset(funcInfo.rOffsets, 0, 8 * sizeof(unsigned));
			memset(funcInfo.fOffsets, 0, 8 * sizeof(unsigned));
			funcInfo.ps3Callable = false;

			funcInfo.genericModuleIndex = ~0u;
			funcInfo.genericOffsetStart = ~0u;
			funcInfo.genericOffset = ~0u;
			funcInfo.genericReturnType = 0;
		}
		else
		{
			TypeBase *returnType = function->type->returnType;

			funcInfo.address = ~0u;
			funcInfo.retType = ExternFuncInfo::RETURN_VOID;

			memset(funcInfo.rOffsets, 0, 8 * sizeof(unsigned));
			memset(funcInfo.fOffsets, 0, 8 * sizeof(unsigned));
			funcInfo.ps3Callable = false;

			if(ModuleData *moduleData = function->importModule)
			{
				funcInfo.genericModuleIndex = moduleData->dependencyIndex;
				funcInfo.genericOffsetStart = unsigned(function->declaration->source->begin - moduleData->lexStream);
				assert(funcInfo.genericOffsetStart < moduleData->lexStreamSize);
			}
			else
			{
				funcInfo.genericModuleIndex = 0;
				funcInfo.genericOffsetStart = unsigned(function->declaration->source->begin - ctx.parseCtx.lexer.GetStreamStart());
				assert(funcInfo.genericOffsetStart < ctx.parseCtx.lexer.GetStreamSize());
			}

			funcInfo.genericOffset = ~0u;
			funcInfo.genericReturnType = returnType->typeIndex;
		}

		VariableHandle *argumentVariable = function->argumentVariables.head;

		for(unsigned i = 0; i < function->arguments.size(); i++)
		{
			ArgumentData &argument = function->arguments[i];

			ExternLocalInfo &local = localInfo.push_back();

			local.paramType = ExternLocalInfo::PARAMETER;
			local.paramFlags = (unsigned char)(argument.isExplicit ? ExternLocalInfo::IS_EXPLICIT : 0);

			if(argument.valueFunction)
				local.defaultFuncId = (unsigned short)argument.valueFunction->functionIndex;
			else
				local.defaultFuncId = 0xffff;

			local.type = argument.type->typeIndex;
			local.size = (unsigned)argument.type->size;
			local.offset = !isGenericFunction ? argumentVariable->variable->offset : 0;
			local.closeListID = 0;

			local.offsetToName = debugSymbols.count;
			debugSymbols.push_back(argument.name->name.begin, argument.name->name.length());
			debugSymbols.push_back(0);

			if (!isGenericFunction)
				argumentVariable = argumentVariable->next;
		}

		if(VariableData *variable = function->contextArgument)
		{
			ExternLocalInfo &local = localInfo.push_back();

			local.paramType = ExternLocalInfo::PARAMETER;
			local.paramFlags = 0;
			local.defaultFuncId = 0xffff;

			local.type = variable->type->typeIndex;
			local.size = (unsigned)variable->type->size;
			local.offset = !isGenericFunction ? variable->offset : 0;
			local.closeListID = 0;

			local.offsetToName = debugSymbols.count;
			debugSymbols.push_back(variable->name->name.begin, variable->name->name.length());
			debugSymbols.push_back(0);
		}

		if (!isGenericFunction)
		{
			for(unsigned i = 0; i < function->functionScope->allVariables.size(); i++)
			{
				VariableData *variable = function->functionScope->allVariables[i];

				VariableHandle *argumentVariable = NULL;

				for(VariableHandle *curr = function->argumentVariables.head; curr; curr = curr->next)
				{
					if(curr->variable == variable)
					{
						argumentVariable = curr;
						break;
					}
				}

				if(argumentVariable || variable == function->contextArgument)
					continue;

				if(variable->isAlloca && variable->users.empty())
					continue;

				if(variable->lookupOnly)
					continue;

				ExternLocalInfo &local = localInfo.push_back();

				local.paramType = ExternLocalInfo::LOCAL;
				local.paramFlags = 0;
				local.defaultFuncId = 0xffff;

				local.type = variable->type->typeIndex;
				local.size = (unsigned)variable->type->size;
				local.offset = variable->offset;
				local.closeListID = 0;

				local.offsetToName = debugSymbols.count;
				debugSymbols.push_back(variable->name->name.begin, variable->name->name.length());
				debugSymbols.push_back(0);
			}
		}

		funcInfo.localCount = localInfo.count - funcInfo.offsetToFirstLocal;

		for(unsigned i = 0; i < function->upvalues.size(); i++)
		{
			UpvalueData *upvalue = function->upvalues[i];

			ExternLocalInfo &local = localInfo.push_back();

			local.paramType = ExternLocalInfo::EXTERNAL;
			local.type = upvalue->variable->type->typeIndex;
			local.size = (unsigned)upvalue->variable->type->size;

			local.offsetToName = debugSymbols.count;
			debugSymbols.push_back(upvalue->variable->name->name.begin, upvalue->variable->name->name.length());
			debugSymbols.push_back(0);
		}

		funcInfo.externCount = function->upvalues.size();

		funcInfo.offsetToName = debugSymbols.count;
		debugSymbols.push_back(function->name->name.begin, function->name->name.length());
		debugSymbols.push_back(0);

		for(MatchData *curr = function->generics.head; curr; curr = curr->next)
		{
			funcInfo.explicitTypeCount++;

			ExternVarInfo &varInfo = vInfo.push_back();

			varInfo.offsetToName = debugSymbols.count;
			debugSymbols.push_back(curr->name->name.begin, curr->name->name.length());
			debugSymbols.push_back(0);

			varInfo.nameHash = curr->name->name.hash();

			varInfo.type = curr->type->typeIndex;
			varInfo.offset = 0;
		}
	}

	code->offsetToInfo = offsetToInfo;
	code->offsetToSource = offsetToSource;

	code->infoSize = infoCount;
	VectorView<unsigned> infoArray(FindSourceInfo(code), infoCount * 2);

	for(unsigned i = 0; i < ctx.instFinalizeCtx.locations.size(); i++)
	{
		SynBase *location = ctx.instFinalizeCtx.locations[i];

		if(location && location->pos.begin >= ctx.code && location->pos.begin < ctx.code + sourceLength)
		{
			infoArray.push_back(i);
			infoArray.push_back(unsigned(location->pos.begin - ctx.code));
		}
	}

	char *sourceCode = (char*)code + offsetToSource;
	memcpy(sourceCode, ctx.code, sourceLength);
	code->sourceSize = sourceLength;

	code->globalCodeStart = ctx.vmModule->globalCodeStart;

	if(ctx.instFinalizeCtx.cmds.size())
		memcpy(FindCode(code), ctx.instFinalizeCtx.cmds.data, ctx.instFinalizeCtx.cmds.size() * sizeof(VMCmd));

#ifdef NULLC_LLVM_SUPPORT
	code->llvmSize = llvmSize;
	code->llvmOffset = offsetToLLVM;
	memcpy(((char*)(code) + code->llvmOffset), llvmBinary, llvmSize);
#endif

	code->typedefCount = typedefCount;
	code->offsetToTypedef = offsetToTypedef;

	ExternTypedefInfo *currAlias = FindFirstTypedef(code);

	for(unsigned i = 0; i < ctx.exprCtx.globalScope->aliases.size(); i++)
	{
		AliasData *alias = ctx.exprCtx.globalScope->aliases[i];

		currAlias->offsetToName = debugSymbols.count;
		debugSymbols.push_back(alias->name->name.begin, alias->name->name.length());
		debugSymbols.push_back(0);

		currAlias->targetType = alias->type->typeIndex;
		currAlias->parentType = ~0u;

		currAlias++;
	}

	for(unsigned i = 0; i < ctx.exprCtx.types.size(); i++)
	{
		TypeBase *type = ctx.exprCtx.types[i];

		if(TypeClass *typeClass = getType<TypeClass>(type))
		{
			for(MatchData *curr = typeClass->generics.head; curr; curr = curr->next)
			{
				currAlias->offsetToName = debugSymbols.count;
				debugSymbols.push_back(curr->name->name.begin, curr->name->name.length());
				debugSymbols.push_back(0);

				currAlias->targetType = curr->type->typeIndex;
				currAlias->parentType = type->typeIndex;
				currAlias++;
			}

			for(MatchData *curr = typeClass->aliases.head; curr; curr = curr->next)
			{
				currAlias->offsetToName = debugSymbols.count;
				debugSymbols.push_back(curr->name->name.begin, curr->name->name.length());
				debugSymbols.push_back(0);

				currAlias->targetType = curr->type->typeIndex;
				currAlias->parentType = type->typeIndex;
				currAlias++;
			}
		}
		else if(TypeGenericClass *typeGenericClass = getType<TypeGenericClass>(type))
		{
			for(TypeHandle *curr = typeGenericClass->generics.head; curr; curr = curr->next)
			{
				currAlias->offsetToName = debugSymbols.count;
				debugSymbols.push_back(0); // No name

				currAlias->targetType = curr->type->typeIndex;
				currAlias->parentType = type->typeIndex;
				currAlias++;
			}
		}
	}

	VectorView<ExternNamespaceInfo> namespaceList(FindFirstNamespace(code), ctx.exprCtx.namespaces.size());

	for(unsigned i = 0; i < ctx.exprCtx.namespaces.size(); i++)
	{
		NamespaceData *nameSpace = ctx.exprCtx.namespaces[i];

		ExternNamespaceInfo &namespaceInfo = namespaceList.push_back();

		namespaceInfo.offsetToName = debugSymbols.count;
		debugSymbols.push_back(nameSpace->name.name.begin, nameSpace->name.name.length());
		debugSymbols.push_back(0);

		namespaceInfo.parentHash = nameSpace->parent ? nameSpace->parent->fullNameHash : ~0u;
	}

	assert(debugSymbols.count == symbolStorageSize);
	assert(tInfo.count == code->typeCount);
	assert(memberList.count == allMemberCount);
	assert(constantList.count == allConstantCount);
	assert(mInfo.count == ctx.exprCtx.imports.size());
	assert(vInfo.count == externVariableInfoCount);
	assert(fInfo.count == exportedFunctionCount);
	assert(localInfo.count == localCount);
	assert(infoArray.count == infoCount * 2);
	assert(namespaceList.count == ctx.exprCtx.namespaces.size());

	return size;
}

bool SaveListing(CompilerContext &ctx, const char *fileName)
{
	assert(!ctx.outputCtx.stream);
	ctx.outputCtx.stream = ctx.outputCtx.openStream(fileName);

	if(!ctx.outputCtx.stream)
	{
		SafeSprintf(ctx.errorBuf, ctx.errorBufSize, "ERROR: failed to open file");
		return false;
	}

	InstructionVmLowerGraphContext instLowerGraphCtx(ctx.outputCtx);

	instLowerGraphCtx.showSource = true;

	PrintInstructions(instLowerGraphCtx, ctx.instFinalizeCtx, ctx.code);

	ctx.outputCtx.closeStream(ctx.outputCtx.stream);
	ctx.outputCtx.stream = NULL;

	return true;
}

bool TranslateToC(CompilerContext &ctx, const char *fileName, const char *mainName, void (*addDependency)(const char *fileName))
{
	assert(!ctx.outputCtx.stream);
	ctx.outputCtx.stream = ctx.outputCtx.openStream(fileName);

	if(!ctx.outputCtx.stream)
	{
		SafeSprintf(ctx.errorBuf, ctx.errorBufSize, "ERROR: failed to open file");
		return false;
	}

	ExpressionTranslateContext exprTranslateCtx(ctx.exprCtx, ctx.outputCtx, ctx.allocator);

	exprTranslateCtx.mainName = mainName;

	exprTranslateCtx.errorBuf = ctx.errorBuf;
	exprTranslateCtx.errorBufSize = ctx.errorBufSize;

	SmallArray<const char*, 32> dependencies;

	if(!TranslateModule(exprTranslateCtx, ctx.exprModule, dependencies))
	{
		ctx.outputCtx.closeStream(ctx.outputCtx.stream);
		ctx.outputCtx.stream = NULL;

		return false;
	}

	ctx.outputCtx.closeStream(ctx.outputCtx.stream);
	ctx.outputCtx.stream = NULL;

	if(addDependency)
	{
		for(unsigned i = 0; i < dependencies.size(); i++)
			addDependency(dependencies[i]);
	}

	return true;
}

char* BuildModuleFromSource(Allocator *allocator, const char *modulePath, const char *code, unsigned codeSize, const char **errorPos, char *errorBuf, unsigned errorBufSize, ArrayView<InplaceStr> activeImports)
{
	CompilerContext ctx(allocator, activeImports);

	ctx.errorBuf = errorBuf;
	ctx.errorBufSize = errorBufSize;

	if(!CompileModuleFromSource(ctx, code))
	{
		*errorPos = ctx.errorPos;

		if(errorBuf && errorBufSize)
		{
			unsigned currLen = (unsigned)strlen(errorBuf);
			SafeSprintf(errorBuf + currLen, errorBufSize - currLen, " [in module '%s']", modulePath);
		}

		return NULL;
	}

	char *bytecode = NULL;
	GetBytecode(ctx, &bytecode);

	Lexer &lexer = ctx.parseCtx.lexer;

	const char *newStart = FindSource((ByteCode*)bytecode);

	// We have to fix lexeme positions to the code that is saved in bytecode
	for(Lexeme *c = lexer.GetStreamStart(), *e = lexer.GetStreamStart() + lexer.GetStreamSize(); c != e; c++)
	{
		// Exit fix up if lexemes exited scope of the current file
		if(c->pos < code || c->pos > (code + codeSize))
			break;

		c->pos = newStart + (c->pos - code);
	}

	BinaryCache::PutBytecode(modulePath, bytecode, lexer.GetStreamStart(), lexer.GetStreamSize());

	return bytecode;
}

char* BuildModuleFromPath(Allocator *allocator, InplaceStr path, InplaceStr pathNoImport, const char **errorPos, char *errorBuf, unsigned errorBufSize, ArrayView<InplaceStr> activeImports)
{
	char filePath[1024];

	unsigned fileSize = 0;
	int needDelete = false;

	assert(path.length() < 1024);
	SafeSprintf(filePath, 1024, "%.*s", path.length(), path.begin);

	char *fileContent = (char*)NULLC::fileLoad(filePath, &fileSize, &needDelete);

	if(!fileContent)
	{
		assert(pathNoImport.length() < 1024);
		SafeSprintf(filePath, 1024, "%.*s", pathNoImport.length(), pathNoImport.begin);

		fileContent = (char*)NULLC::fileLoad(filePath, &fileSize, &needDelete);
	}

	if(!fileContent)
	{
		*errorPos = NULL;

		if(errorBuf && errorBufSize)
			SafeSprintf(errorBuf, errorBufSize, "ERROR: module file '%s' could not be opened", filePath);

		return NULL;
	}

	char *bytecode = BuildModuleFromSource(allocator, filePath, fileContent, fileSize, errorPos, errorBuf, errorBufSize, activeImports);

	if(needDelete)
		NULLC::dealloc(fileContent);

	return bytecode;
}

bool AddModuleFunction(Allocator *allocator, const char* module, void (*ptr)(), const char* name, int index, const char **errorPos, char *errorBuf, unsigned errorBufSize)
{
	const char *importPath = BinaryCache::GetImportPath();

	// Find module
	char path[256], *pathNoImport = path, *cPath = path;
	cPath += SafeSprintf(path, 256, "%s%s", importPath ? importPath : "", module);
	if(importPath)
		pathNoImport = path + strlen(importPath);

	for(unsigned i = 0, e = (unsigned)strlen(pathNoImport); i != e; i++)
	{
		if(pathNoImport[i] == '.')
			pathNoImport[i] = '/';
	}

	SafeSprintf(cPath, 256 - int(cPath - path), ".nc");

	const char *bytecode = BinaryCache::GetBytecode(path);
	if(!bytecode && importPath)
		bytecode = BinaryCache::GetBytecode(pathNoImport);

	// Create module if not found
	if(!bytecode)
		bytecode = BuildModuleFromPath(allocator, InplaceStr(path), InplaceStr(pathNoImport), errorPos, errorBuf, errorBufSize, ArrayView<InplaceStr>());

	if(!bytecode)
		return false;

	unsigned hash = GetStringHash(name);
	ByteCode *code = (ByteCode*)bytecode;

	// Find function and set pointer
	ExternFuncInfo *fInfo = FindFirstFunc(code);

	unsigned end = code->functionCount - code->moduleFunctionCount;
	for(unsigned i = 0; i < end; i++)
	{
		if(hash == fInfo->nameHash)
		{
			if(index == 0)
			{
				fInfo->address = -1;
				fInfo->funcPtr = (void*)ptr;
				index--;
				break;
			}

			index--;
		}

		fInfo++;
	}

	if(index != -1)
	{
		*errorPos = NULL;

		SafeSprintf(errorBuf, errorBufSize, "ERROR: function '%s' or one of it's overload is not found in module '%s'", name, module);

		return false;
	}

	return true;
}
