#include "Compiler.h"

#include "nullc.h"
#include "nullbind.h"
#include "nullc_internal.h"
#include "BinaryCache.h"
#include "Bytecode.h"
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
auto __aassign_itoc(char[] ref dst, int[] src)\r\n\
{\r\n\
	if(dst.size < src.size)\r\n\
		*dst = new char[src.size];\r\n\
	for(int i = 0; i < src.size; i++)\r\n\
		dst[i] = src[i];\r\n\
	return dst;\r\n\
}\r\n\
// short inline array definition support\r\n\
auto __aassign_itos(short[] ref dst, int[] src)\r\n\
{\r\n\
	if(dst.size < src.size)\r\n\
		*dst = new short[src.size];\r\n\
	for(int i = 0; i < src.size; i++)\r\n\
		dst[i] = src[i];\r\n\
	return dst;\r\n\
}\r\n\
// float inline array definition support\r\n\
auto __aassign_dtof(float[] ref dst, double[] src)\r\n\
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

bool BuildBaseModule(Allocator *allocator, int optimizationLevel)
{
	const char *errorPos = NULL;
	char errorBuf[256];

	if(!BuildModuleFromSource(allocator, "$base$.nc", NULL, nullcBaseCode, unsigned(strlen(nullcBaseCode)), &errorPos, errorBuf, 256, optimizationLevel, ArrayView<InplaceStr>(), NULL))
	{
		assert(!"Failed to compile base NULLC module");
		return false;
	}

#ifndef NULLC_NO_EXECUTOR
#define nullcBindModuleFunctionHelperNoMemWrite(moduleName, func, name, index) nullcBindModuleFunctionHelper(moduleName, func, name, index); nullcSetModuleFunctionAttribute(moduleName, name, index, NULLC_ATTRIBUTE_NO_MEMORY_WRITE, 1);

	nullcBindModuleFunctionHelperNoMemWrite("$base$", NULLC::Assert, "assert", 0);
	nullcBindModuleFunctionHelperNoMemWrite("$base$", NULLC::Assert2, "assert", 1);

	nullcBindModuleFunctionHelperNoMemWrite("$base$", NULLC::StrEqual, "==", 0);
	nullcBindModuleFunctionHelperNoMemWrite("$base$", NULLC::StrNEqual, "!=", 0);
	nullcBindModuleFunctionHelper("$base$", NULLC::StrConcatenate, "+", 0);
	nullcBindModuleFunctionHelper("$base$", NULLC::StrConcatenateAndSet, "+=", 0);

	nullcBindModuleFunctionHelperNoMemWrite("$base$", NULLC::Int, "bool", 0);
	nullcBindModuleFunctionHelperNoMemWrite("$base$", NULLC::Char, "char", 0);
	nullcBindModuleFunctionHelperNoMemWrite("$base$", NULLC::Short, "short", 0);
	nullcBindModuleFunctionHelperNoMemWrite("$base$", NULLC::Int, "int", 0);
	nullcBindModuleFunctionHelperNoMemWrite("$base$", NULLC::Long, "long", 0);
	nullcBindModuleFunctionHelperNoMemWrite("$base$", NULLC::Float, "float", 0);
	nullcBindModuleFunctionHelperNoMemWrite("$base$", NULLC::Double, "double", 0);

	nullcBindModuleFunctionHelperNoMemWrite("$base$", NULLC::UnsignedValueChar, "as_unsigned", 0);
	nullcBindModuleFunctionHelperNoMemWrite("$base$", NULLC::UnsignedValueShort, "as_unsigned", 1);
	nullcBindModuleFunctionHelperNoMemWrite("$base$", NULLC::UnsignedValueInt, "as_unsigned", 2);

	nullcBindModuleFunctionHelperNoMemWrite("$base$", NULLC::StrToShort, "short", 1);
	nullcBindModuleFunctionHelperNoMemWrite("$base$", NULLC::ShortToStr, "short::str", 0);
	nullcBindModuleFunctionHelperNoMemWrite("$base$", NULLC::StrToInt, "int", 1);
	nullcBindModuleFunctionHelperNoMemWrite("$base$", NULLC::IntToStr, "int::str", 0);
	nullcBindModuleFunctionHelperNoMemWrite("$base$", NULLC::StrToLong, "long", 1);
	nullcBindModuleFunctionHelperNoMemWrite("$base$", NULLC::LongToStr, "long::str", 0);
	nullcBindModuleFunctionHelperNoMemWrite("$base$", NULLC::StrToFloat, "float", 1);
	nullcBindModuleFunctionHelperNoMemWrite("$base$", NULLC::FloatToStr, "float::str", 0);
	nullcBindModuleFunctionHelperNoMemWrite("$base$", NULLC::StrToDouble, "double", 1);
	nullcBindModuleFunctionHelperNoMemWrite("$base$", NULLC::DoubleToStr, "double::str", 0);

	nullcBindModuleFunctionHelperNoMemWrite("$base$", NULLC::AllocObject, "__newS", 0);
	nullcBindModuleFunctionHelperNoMemWrite("$base$", NULLC::AllocArray, "__newA", 0);
	nullcBindModuleFunctionHelper("$base$", NULLC::CopyObject, "duplicate", 0);
	nullcBindModuleFunctionHelper("$base$", NULLC::CopyArray, "__duplicate_array", 0);
	nullcBindModuleFunctionHelper("$base$", NULLC::ReplaceObject, "replace", 0);
	nullcBindModuleFunctionHelper("$base$", NULLC::SwapObjects, "swap", 0);
	nullcBindModuleFunctionHelper("$base$", NULLC::CompareObjects, "equal", 0);
	nullcBindModuleFunctionHelper("$base$", NULLC::AssignObject, "assign", 0);

	nullcBindModuleFunctionHelper("$base$", NULLC::ArrayCopy, "array_copy", 0);

	nullcBindModuleFunctionHelper("$base$", NULLC::FunctionRedirect, "__redirect", 0);
	nullcBindModuleFunctionHelper("$base$", NULLC::FunctionRedirectPtr, "__redirect_ptr", 0);

	nullcBindModuleFunctionHelperNoMemWrite("$base$", NULLC::Typeid, "typeid", 0);
	nullcBindModuleFunctionHelperNoMemWrite("$base$", NULLC::TypeSize, "typeid::size$", 0);
	nullcBindModuleFunctionHelperNoMemWrite("$base$", NULLC::TypesEqual, "==", 1);
	nullcBindModuleFunctionHelperNoMemWrite("$base$", NULLC::TypesNEqual, "!=", 1);

	nullcBindModuleFunctionHelperNoMemWrite("$base$", NULLC::RefCompare, "__rcomp", 0);
	nullcBindModuleFunctionHelperNoMemWrite("$base$", NULLC::RefNCompare, "__rncomp", 0);
	nullcBindModuleFunctionHelperNoMemWrite("$base$", NULLC::RefLCompare, "<", 0);
	nullcBindModuleFunctionHelperNoMemWrite("$base$", NULLC::RefLECompare, "<=", 0);
	nullcBindModuleFunctionHelperNoMemWrite("$base$", NULLC::RefGCompare, ">", 0);
	nullcBindModuleFunctionHelperNoMemWrite("$base$", NULLC::RefGECompare, ">=", 0);

	nullcBindModuleFunctionHelperNoMemWrite("$base$", NULLC::RefHash, "hash_value", 0);

	nullcBindModuleFunctionHelperNoMemWrite("$base$", NULLC::FuncCompare, "__pcomp", 0);
	nullcBindModuleFunctionHelperNoMemWrite("$base$", NULLC::FuncNCompare, "__pncomp", 0);

	nullcBindModuleFunctionHelperNoMemWrite("$base$", NULLC::ArrayCompare, "__acomp", 0);
	nullcBindModuleFunctionHelperNoMemWrite("$base$", NULLC::ArrayNCompare, "__ancomp", 0);

	nullcBindModuleFunctionHelperNoMemWrite("$base$", NULLC::TypeCount, "__typeCount", 0);

	nullcBindModuleFunctionHelper("$base$", NULLC::AutoArrayAssign, "=", 0);
	nullcBindModuleFunctionHelper("$base$", NULLC::AutoArrayAssignRev, "__aaassignrev", 0);
	nullcBindModuleFunctionHelper("$base$", NULLC::AutoArrayIndex, "[]", 0);

	nullcBindModuleFunctionHelperNoMemWrite("$base$", GC::IsPointerUnmanaged, "isStackPointer", 0);

	nullcBindModuleFunctionHelper("$base$", NULLC::AutoArray, "auto_array_impl", 0);
	nullcBindModuleFunctionHelper("$base$", NULLC::AutoArraySet, "auto[]::set", 0);
	nullcBindModuleFunctionHelper("$base$", NULLC::ShrinkAutoArray, "__force_size", 0);

	nullcBindModuleFunctionHelperNoMemWrite("$base$", NULLC::IsCoroutineReset, "isCoroutineReset", 0);
	nullcBindModuleFunctionHelperNoMemWrite("$base$", NULLC::AssertCoroutine, "__assertCoroutine", 0);

	nullcBindModuleFunctionHelper("$base$", NULLC::GetFinalizationList, "__getFinalizeList", 0);

	nullcBindModuleFunctionHelperNoMemWrite("$base$", NULLC::AssertDerivedFromBase, "assert_derived_from_base", 0);

	nullcBindModuleFunctionHelper("$base$", NULLC::CloseUpvalue, "__closeUpvalue", 0);

#undef nullcBindModuleFunctionHelperNoMemAccess
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
	if(!errorBuf || !errorBufSize)
		return;

	char *errorCurr = errorBuf + strlen(errorBuf);

	if(!codeStart || !errorPos)
	{
		errorCurr += NULLC::SafeSprintf(errorCurr, errorBufSize - unsigned(errorCurr - errorBuf), "\n");
		return;
	}

	const char *codeEnd = codeStart + strlen(codeStart);

	if(errorPos < codeStart || errorPos > codeEnd)
	{
		errorCurr += NULLC::SafeSprintf(errorCurr, errorBufSize - unsigned(errorCurr - errorBuf), "\n");
		return;
	}

	const char *start = errorPos;
	const char *end = start == codeEnd ? start : start + 1;

	while(start > codeStart && *(start - 1) != '\r' && *(start - 1) != '\n')
		start--;

	while(*end && *end != '\r' && *end != '\n')
		end++;

	if(errorBufSize > unsigned(errorCurr - errorBuf))
		*(errorCurr++) = '\n';

	char *errorCurrBefore = errorCurr;

	if(unsigned line = GetErrorLocationLineNumber(codeStart, errorPos))
		errorCurr += NULLC::SafeSprintf(errorCurr, errorBufSize - unsigned(errorCurr - errorBuf), "  at line %d: '", line);
	else
		errorCurr += NULLC::SafeSprintf(errorCurr, errorBufSize - unsigned(errorCurr - errorBuf), "  at '");

	unsigned spacing = unsigned(errorCurr - errorCurrBefore);

	for(const char *pos = start; *pos && pos < end; pos++)
	{
		if(errorBufSize > unsigned(errorCurr - errorBuf))
			*(errorCurr++) = *pos == '\t' ? ' ' : *pos;
	}

	errorCurr += NULLC::SafeSprintf(errorCurr, errorBufSize - unsigned(errorCurr - errorBuf), "'\n");

	for(unsigned i = 0; i < spacing + unsigned(errorPos - start); i++)
	{
		if(errorBufSize > unsigned(errorCurr - errorBuf))
			*(errorCurr++) = ' ';
	}

	errorCurr += NULLC::SafeSprintf(errorCurr, errorBufSize - unsigned(errorCurr - errorBuf), "^\n");

	errorBuf[errorBufSize - 1] = '\0';
}

bool HasSourceCode(ByteCode *bytecode, const char *position)
{
	char *code = FindSource(bytecode);
	unsigned length = bytecode->sourceSize;

	if(position >= code && position <= code + length)
		return true;

	return false;
}

ModuleData* FindModuleWithSourceLocation(ExpressionContext &ctx, const char *position)
{
	for(unsigned i = 0; i < ctx.imports.size(); i++)
	{
		if(HasSourceCode(ctx.imports[i]->bytecode, position))
			return ctx.imports[i];
	}

	return NULL;
}

InplaceStr FindModuleNameWithSourceLocation(ExpressionContext &ctx, const char *position)
{
	if(ModuleData *moduleData = FindModuleWithSourceLocation(ctx, position))
		return moduleData->name;

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

ExprModule* AnalyzeModuleFromSource(CompilerContext &ctx)
{
	TRACE_SCOPE("compiler", "AnalyzeModuleFromSource");

	ParseContext &parseCtx = ctx.parseCtx;

	parseCtx.bytecodeBuilder = BuildModuleFromPath;

	parseCtx.errorBuf = ctx.errorBuf;
	parseCtx.errorBufSize = ctx.errorBufSize;

	ctx.synModule = Parse(parseCtx, ctx.code, ctx.moduleRoot);

	ctx.statistics.Add(ctx.parseCtx.statistics);

	ctx.statistics.Start(NULLCTime::clockMicro());

	if(ctx.enableLogFiles && ctx.synModule)
	{
		TRACE_SCOPE("compiler", "Debug::syntax_graph");

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

	ctx.statistics.Finish("Logging", NULLCTime::clockMicro());

	if(!ctx.synModule)
	{
		if(parseCtx.errorPos)
			ctx.errorPos = parseCtx.errorPos;

		return NULL;
	}

	//printf("# Parse memory %dkb\n", ctx.allocator->requested() / 1024);

	ExpressionContext &exprCtx = ctx.exprCtx;

	exprCtx.memoryLimit = ctx.exprMemoryLimit;

	exprCtx.errorBuf = ctx.errorBuf;
	exprCtx.errorBufSize = ctx.errorBufSize;

	if(parseCtx.errorPos)
	{
		unsigned errorLength = unsigned(strlen(exprCtx.errorBuf));

		exprCtx.errorBuf += errorLength;
		exprCtx.errorBufSize -= errorLength;
	}

	ctx.exprModule = Analyze(exprCtx, ctx.synModule, ctx.code, ctx.moduleRoot);

	ctx.statistics.Add(ctx.exprCtx.statistics);

	ctx.statistics.Start(NULLCTime::clockMicro());

	if(ctx.enableLogFiles && ctx.exprModule)
	{
		TRACE_SCOPE("compiler", "Debug::expr_graph");

		assert(!ctx.outputCtx.stream);
		ctx.outputCtx.stream = ctx.outputCtx.openStream("expr_graph.txt");

		if(ctx.outputCtx.stream)
		{
			ExpressionGraphContext exprGraphCtx(ctx.exprCtx, ctx.outputCtx);

			PrintGraph(exprGraphCtx, ctx.exprModule, "");

			ctx.outputCtx.closeStream(ctx.outputCtx.stream);
			ctx.outputCtx.stream = NULL;
		}
	}

	ctx.statistics.Finish("Logging", NULLCTime::clockMicro());

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

bool CompileModuleFromSource(CompilerContext &ctx)
{
	TRACE_SCOPE("compiler", "CompileModuleFromSource");

	if(!AnalyzeModuleFromSource(ctx))
		return false;

	ExpressionContext &exprCtx = ctx.exprCtx;

	ctx.statistics.Start(NULLCTime::clockMicro());

	ctx.vmModule = CompileVm(exprCtx, ctx.exprModule, ctx.code);

	ctx.statistics.Finish("IrCodeGen", NULLCTime::clockMicro());

	if(!ctx.vmModule)
	{
		ctx.errorPos = NULL;

		if(ctx.errorBuf && ctx.errorBufSize)
			NULLC::SafeSprintf(ctx.errorBuf, ctx.errorBufSize, "ERROR: internal compiler error: failed to create VmModule");

		return false;
	}

	//printf("# Instruction memory %dkb\n", pool.GetSize() / 1024);

	ctx.statistics.Start(NULLCTime::clockMicro());

	if(ctx.enableLogFiles)
	{
		TRACE_SCOPE("compiler", "Debug::inst_graph");

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

	ctx.statistics.Finish("Logging", NULLCTime::clockMicro());

	ctx.statistics.Start(NULLCTime::clockMicro());

	// Build LLVM module is support is enabled or just to test execution paths in debug build
#if defined(NULLC_LLVM_SUPPORT)
	ctx.llvmModule = CompileLlvm(exprCtx, ctx.exprModule);
#elif !defined(NDEBUG)
	ctx.llvmModule = CompileLlvm(exprCtx, ctx.exprModule);
#endif

	ctx.statistics.Finish("LlvmCodeGen", NULLCTime::clockMicro());

	ctx.statistics.Start(NULLCTime::clockMicro());

	for(VmFunction *function = ctx.vmModule->functions.head; function; function = function->next)
	{
		if(!function->firstBlock)
			continue;

		// Dead code elimination is required for correct register allocation
		if(ctx.optimizationLevel == 0)
			RunVmPass(exprCtx, ctx.vmModule, function, VM_PASS_OPT_DEAD_CODE_ELIMINATION);

		if(ctx.optimizationLevel >= 1)
		{
			TRACE_SCOPE("compiler", "OptimizationLevel1");

			RunVmPass(exprCtx, ctx.vmModule, function, VM_PASS_OPT_PEEPHOLE);
			RunVmPass(exprCtx, ctx.vmModule, function, VM_PASS_OPT_CONSTANT_PROPAGATION);
			RunVmPass(exprCtx, ctx.vmModule, function, VM_PASS_OPT_DEAD_CODE_ELIMINATION);
			RunVmPass(exprCtx, ctx.vmModule, function, VM_PASS_OPT_CONTROL_FLOW_SIPLIFICATION);
			RunVmPass(exprCtx, ctx.vmModule, function, VM_PASS_OPT_DEAD_CODE_ELIMINATION);
			RunVmPass(exprCtx, ctx.vmModule, function, VM_PASS_OPT_LOAD_STORE_PROPAGATION);
			RunVmPass(exprCtx, ctx.vmModule, function, VM_PASS_OPT_ARRAY_TO_ELEMENTS);
			RunVmPass(exprCtx, ctx.vmModule, function, VM_PASS_OPT_DEAD_CODE_ELIMINATION);
		}

		RunVmPass(exprCtx, ctx.vmModule, function, VM_PASS_LEGALIZE_ARRAY_VALUES);

		if(ctx.optimizationLevel >= 2)
		{
			TRACE_SCOPE("compiler", "OptimizationLevel2");

			for(unsigned i = 0; i < 6; i++)
			{
				TRACE_SCOPE("compiler", "iteration");

				unsigned before = ctx.vmModule->peepholeOptimizations + ctx.vmModule->constantPropagations + ctx.vmModule->deadCodeEliminations + ctx.vmModule->controlFlowSimplifications + ctx.vmModule->loadStorePropagations + ctx.vmModule->commonSubexprEliminations + ctx.vmModule->deadAllocaStoreEliminations;

				RunVmPass(exprCtx, ctx.vmModule, function, VM_PASS_OPT_CONSTANT_PROPAGATION);
				RunVmPass(exprCtx, ctx.vmModule, function, VM_PASS_OPT_LOAD_STORE_PROPAGATION);
				RunVmPass(exprCtx, ctx.vmModule, function, VM_PASS_OPT_COMMON_SUBEXPRESSION_ELIMINATION);
				RunVmPass(exprCtx, ctx.vmModule, function, VM_PASS_OPT_PEEPHOLE);
				RunVmPass(exprCtx, ctx.vmModule, function, VM_PASS_OPT_DEAD_CODE_ELIMINATION);
				RunVmPass(exprCtx, ctx.vmModule, function, VM_PASS_OPT_CONTROL_FLOW_SIPLIFICATION);
				RunVmPass(exprCtx, ctx.vmModule, function, VM_PASS_OPT_DEAD_CODE_ELIMINATION);
				RunVmPass(exprCtx, ctx.vmModule, function, VM_PASS_OPT_DEAD_ALLOCA_STORE_ELIMINATION);

				unsigned after = ctx.vmModule->peepholeOptimizations + ctx.vmModule->constantPropagations + ctx.vmModule->deadCodeEliminations + ctx.vmModule->controlFlowSimplifications + ctx.vmModule->loadStorePropagations + ctx.vmModule->commonSubexprEliminations + ctx.vmModule->deadAllocaStoreEliminations;

				// Reached fixed point
				if(before == after)
					break;
			}

			RunVmPass(exprCtx, ctx.vmModule, function, VM_PASS_OPT_MEMORY_TO_REGISTER);
			RunVmPass(exprCtx, ctx.vmModule, function, VM_PASS_OPT_DEAD_CODE_ELIMINATION);

			RunVmPass(exprCtx, ctx.vmModule, function, VM_PASS_OPT_LATE_PEEPHOLE);
			RunVmPass(exprCtx, ctx.vmModule, function, VM_PASS_OPT_DEAD_CODE_ELIMINATION);

			RunVmPass(exprCtx, ctx.vmModule, function, VM_PASS_OPT_DEAD_ALLOCA_STORE_ELIMINATION);
		}
	}

	ctx.statistics.Finish("IrOptimization", NULLCTime::clockMicro());

	ctx.statistics.Start(NULLCTime::clockMicro());

	RunVmPass(exprCtx, ctx.vmModule, VM_PASS_UPDATE_LIVE_SETS);

	if(ctx.optimizationLevel >= 2)
		RunVmPass(exprCtx, ctx.vmModule, VM_PASS_PREPARE_SSA_EXIT);

	RunVmPass(exprCtx, ctx.vmModule, VM_PASS_LEGALIZE_BITCASTS);
	RunVmPass(exprCtx, ctx.vmModule, VM_PASS_LEGALIZE_EXTRACTS);

	RunVmPass(exprCtx, ctx.vmModule, VM_PASS_CREATE_ALLOCA_STORAGE);

	ctx.statistics.Finish("IrFinalization", NULLCTime::clockMicro());

	ctx.statistics.Start(NULLCTime::clockMicro());

	if(ctx.enableLogFiles)
	{
		TRACE_SCOPE("compiler", "Debug::inst_graph_opt");

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

	ctx.statistics.Finish("Logging", NULLCTime::clockMicro());

	ctx.statistics.Start(NULLCTime::clockMicro());

	ctx.regVmLoweredModule = RegVmLowerModule(exprCtx, ctx.vmModule);

	if(!ctx.regVmLoweredModule->functions.empty() && ctx.regVmLoweredModule->functions.back()->hasRegisterOverflow)
	{
		RegVmLoweredFunction *function = ctx.regVmLoweredModule->functions.back();

		if(function->registerOverflowLocation && function->registerOverflowLocation->source)
			ctx.errorPos = function->registerOverflowLocation->source->begin->pos;
		else
			ctx.errorPos = NULL;

		if(ctx.errorBuf && ctx.errorBufSize)
		{
			NULLC::SafeSprintf(ctx.errorBuf, ctx.errorBufSize, "ERROR: internal compiler error: register count overflow");

			if(ctx.errorPos)
				AddErrorLocationInfo(FindModuleCodeWithSourceLocation(ctx.exprCtx, ctx.errorPos), ctx.errorPos, ctx.errorBuf, ctx.errorBufSize);
		}

		return false;
	}

	RegVmFinalizeModule(ctx.instRegVmFinalizeCtx, ctx.regVmLoweredModule);

	ctx.statistics.Finish("IrLowering", NULLCTime::clockMicro());

	ctx.statistics.Start(NULLCTime::clockMicro());

	if(ctx.enableLogFiles)
	{
		TRACE_SCOPE("compiler", "Debug::inst_graph_reg_low");

		assert(!ctx.outputCtx.stream);
		ctx.outputCtx.stream = ctx.outputCtx.openStream("inst_graph_reg_low.txt");

		if(ctx.outputCtx.stream)
		{
			InstructionRegVmLowerGraphContext instRegVmLowerGraphCtx(ctx.outputCtx);

			instRegVmLowerGraphCtx.showSource = true;

			PrintGraph(instRegVmLowerGraphCtx, ctx.regVmLoweredModule);

			ctx.outputCtx.closeStream(ctx.outputCtx.stream);
			ctx.outputCtx.stream = NULL;
		}
	}

	ctx.statistics.Finish("Logging", NULLCTime::clockMicro());

	return true;
}

unsigned GetBytecode(CompilerContext &ctx, char **bytecode)
{
	TRACE_SCOPE("compiler", "GetBytecode");

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

			for(MemberHandle *curr = typeClass->members.head; curr; curr = curr->next)
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
			for(MemberHandle *curr = typeStruct->members.head; curr; curr = curr->next)
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

	unsigned offsetToRegVmCode = size;
	size += ctx.instRegVmFinalizeCtx.cmds.size() * sizeof(RegVmCmd);

	unsigned sourceLength = (unsigned)strlen(ctx.code) + 1;

	unsigned regVmInfoCount = 0;

	for(unsigned i = 0; i < ctx.instRegVmFinalizeCtx.locations.size(); i++)
	{
		SynBase *location = ctx.instRegVmFinalizeCtx.locations[i];

		if(location && !location->isInternal)
			regVmInfoCount++;
	}

	unsigned offsetToRegVmInfo = size;
	size += sizeof(ExternSourceInfo) * regVmInfoCount;

	unsigned offsetToRegVmConstants = size;
	size += ctx.instRegVmFinalizeCtx.constants.size() * sizeof(ctx.instRegVmFinalizeCtx.constants[0]);

	unsigned offsetToRegVmRegKillInfo = size;
	size += ctx.instRegVmFinalizeCtx.regKillInfo.size() * sizeof(ctx.instRegVmFinalizeCtx.regKillInfo[0]);

	unsigned offsetToSymbols = size;
	size += symbolStorageSize;

	unsigned offsetToSource = size;
	size += sourceLength;

	unsigned int offsetToLLVM = size;
	size += ctx.llvmModule ? ctx.llvmModule->moduleSize : 0;

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

	code->regVmCodeSize = ctx.instRegVmFinalizeCtx.cmds.size();
	code->regVmOffsetToCode = offsetToRegVmCode;

	code->regVmConstantCount = ctx.instRegVmFinalizeCtx.constants.size();
	code->regVmOffsetToConstants = offsetToRegVmConstants;

	code->regVmRegKillInfoCount = ctx.instRegVmFinalizeCtx.regKillInfo.size();
	code->regVmOffsetToRegKillInfo = offsetToRegVmRegKillInfo;

	code->symbolLength = symbolStorageSize;
	code->offsetToSymbols = offsetToSymbols;

	code->constantCount = allConstantCount;
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

		target.constantOffset = 0;
		target.constantCount = 0;

		target.memberCount = 0;
		target.memberOffset = 0;

		target.pointerCount = type->hasPointers ? 1 : 0;

		if(ModuleData *moduleData = type->importModule)
		{
			for(unsigned i = 0; i < ctx.exprCtx.imports.size(); i++)
			{
				if(ctx.exprCtx.imports[i]->name == moduleData->name)
				{
					target.definitionModule = i + 1;
					break;
				}
			}
		}
		else
		{
			target.definitionModule = 0;
		}

		target.definitionLocationStart = 0;
		target.definitionLocationEnd = 0;
		target.definitionLocationName = 0;

		Lexeme *sourceStreamStart = type->importModule ? type->importModule->lexStream : ctx.parseCtx.lexer.GetStreamStart();
		unsigned sourceStreamSize = type->importModule ? type->importModule->lexStreamSize : ctx.parseCtx.lexer.GetStreamSize();

		if(TypeClass *typeClass = getType<TypeClass>(type))
		{
			if(typeClass->source->begin >= sourceStreamStart && typeClass->source->begin < sourceStreamStart + sourceStreamSize)
			{
				target.definitionLocationStart = unsigned(typeClass->source->begin - sourceStreamStart);
				target.definitionLocationEnd = unsigned(typeClass->source->end - sourceStreamStart);

				assert(target.definitionLocationStart < sourceStreamSize);
				assert(target.definitionLocationEnd < sourceStreamSize);

				if(typeClass->identifier.begin)
				{
					target.definitionLocationName = unsigned(typeClass->identifier.begin - sourceStreamStart);
					assert(target.definitionLocationName < sourceStreamSize);
				}
			}
		}
		else if(TypeEnum *typeEnum = getType<TypeEnum>(type))
		{
			if(typeEnum->source->begin >= sourceStreamStart && typeEnum->source->begin < sourceStreamStart + sourceStreamSize)
			{
				target.definitionLocationStart = unsigned(typeEnum->source->begin - sourceStreamStart);
				target.definitionLocationEnd = unsigned(typeEnum->source->end - sourceStreamStart);

				assert(target.definitionLocationStart < sourceStreamSize);
				assert(target.definitionLocationEnd < sourceStreamSize);

				if(typeEnum->identifier.begin)
				{
					target.definitionLocationName = unsigned(typeEnum->identifier.begin - sourceStreamStart);
					assert(target.definitionLocationName < sourceStreamSize);
				}
			}
		}
		else if(TypeGenericClassProto *typeGenericClassProto = getType<TypeGenericClassProto>(type))
		{
			if(typeGenericClassProto->source->begin >= sourceStreamStart && typeGenericClassProto->source->begin < sourceStreamStart + sourceStreamSize)
			{
				target.definitionLocationStart = unsigned(typeGenericClassProto->source->begin - sourceStreamStart);
				target.definitionLocationEnd = unsigned(typeGenericClassProto->source->end - sourceStreamStart);

				assert(target.definitionLocationStart < sourceStreamSize);
				assert(target.definitionLocationEnd < sourceStreamSize);

				if(typeGenericClassProto->identifier.begin)
				{
					target.definitionLocationName = unsigned(typeGenericClassProto->identifier.begin - sourceStreamStart);
					assert(target.definitionLocationName < sourceStreamSize);
				}
			}
		}

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
			member.alignment = 0;
			member.offset = 0;

			for(TypeHandle *curr = typeFunction->arguments.head; curr; curr = curr->next)
			{
				ExternMemberInfo &member = memberList.push_back();

				member.type = curr->type->typeIndex;
				member.alignment = 0;
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

				if(typeClass->isInternal)
					target.typeFlags |= ExternTypeInfo::TYPE_IS_INTERNAL;

				if(typeClass->completed)
					target.typeFlags |= ExternTypeInfo::TYPE_IS_COMPLETED;

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

			for(MemberHandle *curr = typeStruct->members.head; curr; curr = curr->next)
			{
				if(*curr->variable->name->name.begin == '$')
					continue;

				target.memberCount++;

				ExternMemberInfo &member = memberList.push_back();

				member.type = curr->variable->type->typeIndex;
				member.alignment = curr->variable->alignment;
				member.offset = curr->variable->offset;

				debugSymbols.push_back(curr->variable->name->name.begin, curr->variable->name->name.length());
				debugSymbols.push_back(0);
			}

			target.constantCount = 0;
			target.constantOffset = constantList.count;

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

			for(MemberHandle *curr = typeStruct->members.head; curr; curr = curr->next)
			{
				if(curr->variable->type->hasPointers)
				{
					target.pointerCount++;

					ExternMemberInfo &member = memberList.push_back();

					member.type = curr->variable->type->typeIndex;
					member.alignment = curr->variable->alignment;
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
				target.definitionOffsetStart = unsigned(typeGenericClassProto->definition->begin - moduleData->lexStream);
				assert(target.definitionOffsetStart < moduleData->lexStreamSize);
			}
			else
			{
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
		moduleInfo.funcCount = moduleData->moduleFunctionCount;

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
			funcInfo.regVmAddress = function->implementation->vmFunction->regVmAddress;
			funcInfo.regVmCodeSize = function->implementation->functionIndex | 0x80000000;
			funcInfo.regVmRegisters = function->implementation->vmFunction->regVmRegisters;
		}
		else if(function->isPrototype)
		{
			funcInfo.regVmAddress = 0;
			funcInfo.regVmCodeSize = 0;
			funcInfo.regVmRegisters = 0;
		}
		else if(function->vmFunction)
		{
			funcInfo.regVmAddress = function->vmFunction->regVmAddress;
			funcInfo.regVmCodeSize = function->vmFunction->regVmCodeSize;
			funcInfo.regVmRegisters = function->vmFunction->regVmRegisters;
		}
		else
		{
			funcInfo.regVmAddress = ~0u;
			funcInfo.regVmCodeSize = 0;
			funcInfo.regVmRegisters = 0;
		}

		funcInfo.funcPtrRaw = NULL;
		funcInfo.funcPtrWrapTarget = NULL;
		funcInfo.funcPtrWrap = NULL;

		funcInfo.builtinIndex = 0;
		funcInfo.attributes = function->attributes;

		// Only functions in global or namespace scope remain visible
		funcInfo.isVisible = (function->scope == ctx.exprCtx.globalScope || function->scope->ownerNamespace) && !function->isHidden;

		funcInfo.nameHash = function->nameHash;

		if(ModuleData *moduleData = function->importModule)
		{
			for(unsigned i = 0; i < ctx.exprCtx.imports.size(); i++)
			{
				if(ctx.exprCtx.imports[i]->name == moduleData->name)
				{
					funcInfo.definitionModule = i + 1;
					break;
				}
			}
		}
		else
		{
			funcInfo.definitionModule = 0;
		}

		funcInfo.definitionLocationStart = 0;
		funcInfo.definitionLocationEnd = 0;
		funcInfo.definitionLocationName = 0;

		Lexeme *sourceStreamStart = function->importModule ? function->importModule->lexStream : ctx.parseCtx.lexer.GetStreamStart();
		unsigned sourceStreamSize = function->importModule ? function->importModule->lexStreamSize : ctx.parseCtx.lexer.GetStreamSize();

		if(function->source->begin >= sourceStreamStart && function->source->begin < sourceStreamStart + sourceStreamSize)
		{
			funcInfo.definitionLocationStart = unsigned(function->source->begin - sourceStreamStart);
			funcInfo.definitionLocationEnd = unsigned(function->source->end - sourceStreamStart);

			assert(funcInfo.definitionLocationStart < sourceStreamSize);
			assert(funcInfo.definitionLocationEnd < sourceStreamSize);

			if(function->name->begin)
			{
				funcInfo.definitionLocationName = unsigned(function->name->begin - sourceStreamStart);
				assert(funcInfo.definitionLocationName < sourceStreamSize);
			}
		}

		if(ScopeData *scope = ctx.exprCtx.NamespaceScopeFrom(function->scope))
			funcInfo.namespaceHash = scope->ownerNamespace->fullNameHash;
		else
			funcInfo.namespaceHash = ~0u;

		funcInfo.funcCat = ExternFuncInfo::NORMAL;

		if(function->scope->ownerType)
			funcInfo.funcCat = ExternFuncInfo::THISCALL;
		else if(function->coroutine)
			funcInfo.funcCat = ExternFuncInfo::COROUTINE;
		else if(!funcInfo.isVisible)
			funcInfo.funcCat = ExternFuncInfo::LOCAL;

		funcInfo.isGenericInstance = ctx.exprCtx.IsGenericInstance(function);

		funcInfo.isOperator = function->isOperator;

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
#if defined(__linux)
			else if(returnType == ctx.exprCtx.typeBool || returnType == ctx.exprCtx.typeChar || returnType == ctx.exprCtx.typeShort || returnType == ctx.exprCtx.typeInt || (sizeof(void*) == 4 && isType<TypeRef>(returnType)))
				funcInfo.retType = ExternFuncInfo::RETURN_INT;
			else if(returnType == ctx.exprCtx.typeLong || (sizeof(void*) == 8 && isType<TypeRef>(returnType)))
				funcInfo.retType = ExternFuncInfo::RETURN_LONG;
#else
			else if(returnType == ctx.exprCtx.typeInt || returnType->size <= 4)
				funcInfo.retType = ExternFuncInfo::RETURN_INT;
			else if(returnType == ctx.exprCtx.typeLong || returnType->size == 8)
				funcInfo.retType = ExternFuncInfo::RETURN_LONG;
#endif

			funcInfo.returnShift = (unsigned char)(returnType->size / 4);

			funcInfo.returnSize = funcInfo.returnShift * 4;

			if(funcInfo.retType == ExternFuncInfo::RETURN_VOID)
				funcInfo.returnSize = 0;
			else if(funcInfo.retType == ExternFuncInfo::RETURN_INT)
				funcInfo.returnSize = 4;
			else if(funcInfo.retType == ExternFuncInfo::RETURN_DOUBLE)
				funcInfo.returnSize = 8;
			else if(funcInfo.retType == ExternFuncInfo::RETURN_LONG)
				funcInfo.returnSize = 8;

			funcInfo.bytesToPop = (unsigned)function->argumentsSize;
			funcInfo.stackSize = (unsigned)function->stackSize;

			funcInfo.genericOffsetStart = ~0u;
			funcInfo.genericOffset = ~0u;
			funcInfo.genericReturnType = 0;
		}
		else
		{
			TypeBase *returnType = function->type->returnType;

			funcInfo.regVmAddress = ~0u;

			funcInfo.retType = ExternFuncInfo::RETURN_VOID;

			if(ModuleData *moduleData = function->importModule)
			{
				funcInfo.genericOffsetStart = unsigned(function->declaration->source->begin - moduleData->lexStream);
				assert(funcInfo.genericOffsetStart < moduleData->lexStreamSize);
			}
			else
			{
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
			local.offset = (unsigned)upvalue->target->offset;

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

	code->regVmOffsetToInfo = offsetToRegVmInfo;
	code->regVmInfoSize = regVmInfoCount;

	VectorView<ExternSourceInfo> regVmInfoArray(FindRegVmSourceInfo(code), regVmInfoCount);

	for(unsigned i = 0; i < ctx.instRegVmFinalizeCtx.locations.size(); i++)
	{
		SynBase *location = ctx.instRegVmFinalizeCtx.locations[i];

		if(location && !location->isInternal)
		{
			ExternSourceInfo info;

			info.instruction = i;

			if(ModuleData *importModule = ctx.exprCtx.GetSourceOwner(location->begin))
			{
				const char *code = FindSource(importModule->bytecode);

				assert(location->pos.begin >= code && location->pos.begin < code + importModule->bytecode->sourceSize);

				for(unsigned i = 0; i < ctx.exprCtx.imports.size(); i++)
				{
					if(ctx.exprCtx.imports[i]->name == importModule->name)
					{
						info.definitionModule = i + 1;
						break;
					}
				}

				info.sourceOffset = unsigned(location->begin->pos - code);

				regVmInfoArray.push_back(info);
			}
			else
			{
				const char *code = ctx.code;

				assert(location->pos.begin >= code && location->pos.begin < code + sourceLength);

				info.definitionModule = 0;
				info.sourceOffset = unsigned(location->begin->pos - code);

				regVmInfoArray.push_back(info);
			}
		}
	}

	code->regVmGlobalCodeStart = ctx.vmModule->regVmGlobalCodeStart;

	if(ctx.instRegVmFinalizeCtx.cmds.size())
		memcpy(FindRegVmCode(code), ctx.instRegVmFinalizeCtx.cmds.data, ctx.instRegVmFinalizeCtx.cmds.size() * sizeof(RegVmCmd));

	if(ctx.instRegVmFinalizeCtx.constants.size())
		memcpy(FindRegVmConstants(code), ctx.instRegVmFinalizeCtx.constants.data, ctx.instRegVmFinalizeCtx.constants.size() * sizeof(ctx.instRegVmFinalizeCtx.constants[0]));

	if(ctx.instRegVmFinalizeCtx.regKillInfo.size())
		memcpy(FindRegVmRegKillInfo(code), ctx.instRegVmFinalizeCtx.regKillInfo.data, ctx.instRegVmFinalizeCtx.regKillInfo.size() * sizeof(ctx.instRegVmFinalizeCtx.regKillInfo[0]));

	char *sourceCode = (char*)code + offsetToSource;
	memcpy(sourceCode, ctx.code, sourceLength);

	code->offsetToSource = offsetToSource;
	code->sourceSize = sourceLength;

	code->llvmSize = ctx.llvmModule ? ctx.llvmModule->moduleSize : 0;
	code->llvmOffset = offsetToLLVM;

	if(ctx.llvmModule)
		memcpy(((char*)(code) + code->llvmOffset), ctx.llvmModule->moduleData, ctx.llvmModule->moduleSize);

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
	assert(regVmInfoArray.count == regVmInfoCount);
	assert(namespaceList.count == ctx.exprCtx.namespaces.size());

	return size;
}

bool SaveListing(CompilerContext &ctx, const char *fileName)
{
	TRACE_SCOPE("compiler", "SaveListing");

	assert(!ctx.outputCtx.stream);
	ctx.outputCtx.stream = ctx.outputCtx.openStream(fileName);

	if(!ctx.outputCtx.stream)
	{
		NULLC::SafeSprintf(ctx.errorBuf, ctx.errorBufSize, "ERROR: failed to open file");
		return false;
	}

	InstructionRegVmLowerGraphContext instLowerGraphCtx(ctx.outputCtx);

	instLowerGraphCtx.showSource = true;

	PrintGraph(instLowerGraphCtx, ctx.regVmLoweredModule);

	ctx.outputCtx.closeStream(ctx.outputCtx.stream);
	ctx.outputCtx.stream = NULL;

	return true;
}

bool TranslateToC(CompilerContext &ctx, const char *fileName, const char *mainName, void (*addDependency)(const char *fileName))
{
	TRACE_SCOPE("compiler", "TranslateToC");

	assert(!ctx.outputCtx.stream);
	ctx.outputCtx.stream = ctx.outputCtx.openStream(fileName);

	if(!ctx.outputCtx.stream)
	{
		NULLC::SafeSprintf(ctx.errorBuf, ctx.errorBufSize, "ERROR: failed to open file");
		return false;
	}

	ExpressionTranslateContext exprTranslateCtx(ctx.exprCtx, ctx.outputCtx, ctx.allocator);

	exprTranslateCtx.mainName = mainName;

	exprTranslateCtx.errorBuf = ctx.errorBuf;
	exprTranslateCtx.errorBufSize = ctx.errorBufSize;

	SmallArray<const char*, 32> dependencies(ctx.allocator);

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

char* BuildModuleFromSource(Allocator *allocator, const char *modulePath, const char *moduleRoot, const char *code, unsigned codeSize, const char **errorPos, char *errorBuf, unsigned errorBufSize, int optimizationLevel, ArrayView<InplaceStr> activeImports, CompilerStatistics *statistics)
{
	TRACE_SCOPE("compiler", "BuildModuleFromSource");
	TRACE_LABEL(modulePath);

	CompilerContext ctx(allocator, optimizationLevel, activeImports);

	ctx.errorBuf = errorBuf;
	ctx.errorBufSize = errorBufSize;

	ctx.code = code;
	ctx.moduleRoot = moduleRoot;

	if(!CompileModuleFromSource(ctx))
	{
		*errorPos = ctx.errorPos;

		if(errorBuf && errorBufSize)
		{
			unsigned currLen = (unsigned)strlen(errorBuf);
			NULLC::SafeSprintf(errorBuf + currLen, errorBufSize - currLen, " [in module '%s']", modulePath);
		}

		return NULL;
	}

	ctx.statistics.Start(NULLCTime::clockMicro());

	char *bytecode = NULL;
	GetBytecode(ctx, &bytecode);

	ctx.statistics.Finish("Bytecode", NULLCTime::clockMicro());

	ctx.statistics.Start(NULLCTime::clockMicro());

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

	ctx.statistics.Finish("BytecodeCache", NULLCTime::clockMicro());

	if(statistics)
		statistics->Add(ctx.statistics);

	return bytecode;
}

const char* FindFileContentInImportPaths(InplaceStr moduleName, const char *moduleRoot, bool addExtension, char *resultPathBuf, unsigned resultPathBufSize, unsigned &fileSize)
{
	const char *fileContent = NULL;

	unsigned modulePathPos = 0;
	while(const char *modulePath = BinaryCache::EnumImportPath(modulePathPos++))
	{
		char *pathEnd = resultPathBuf + NULLC::SafeSprintf(resultPathBuf, resultPathBufSize, "%s%s%s%.*s", modulePath, moduleRoot ? moduleRoot : "", moduleRoot ? "/" : "", moduleName.length(), moduleName.begin);

		if(addExtension)
		{
			char *pathNoImport = resultPathBuf + strlen(modulePath);

			for(unsigned i = 0, e = (unsigned)strlen(pathNoImport); i != e; i++)
			{
				if(pathNoImport[i] == '.')
					pathNoImport[i] = '/';
			}

			NULLC::SafeSprintf(pathEnd, resultPathBufSize - int(pathEnd - resultPathBuf), ".nc");
		}

		fileContent = NULLC::fileLoad(resultPathBuf, &fileSize);

		if(fileContent)
			break;
	}

	return fileContent;
}

char* BuildModuleFromPath(Allocator *allocator, InplaceStr moduleName, const char *moduleRoot, bool addExtension, const char **errorPos, char *errorBuf, unsigned errorBufSize, int optimizationLevel, ArrayView<InplaceStr> activeImports, CompilerStatistics *statistics)
{
	if(statistics)
		statistics->Start(NULLCTime::clockMicro());

	NULLC::TraceDump();

	if(statistics)
	{
		statistics->Finish("Tracing", NULLCTime::clockMicro());
		statistics->Start(NULLCTime::clockMicro());
	}

	TRACE_SCOPE("compiler", "BuildModuleFromPath");
	TRACE_LABEL2(moduleName.begin, moduleName.end);

	assert(*moduleName.end == 0);

	if(addExtension)
		assert(strstr(moduleName.begin, ".nc") == NULL);
	else
		assert(strstr(moduleName.begin, ".nc") != NULL);

	const unsigned nextModuleRootLength = 1024;
	char nextModuleRootBuf[nextModuleRootLength];

	const char *nextModuleRoot = moduleRoot;

	const unsigned pathLength = 1024;
	char path[pathLength];

	unsigned fileSize = 0;
	const char *fileContent = NULL;

	if(moduleRoot)
	{
		fileContent = FindFileContentInImportPaths(moduleName, moduleRoot, addExtension, path, pathLength, fileSize);

		if(fileContent)
		{
			if(const char *pos = moduleName.rfind('/'))
			{
				NULLC::SafeSprintf(nextModuleRootBuf, nextModuleRootLength, "%s/%.*s", moduleRoot, unsigned(pos - moduleName.begin), moduleName.begin);

				nextModuleRoot = nextModuleRootBuf;
			}
		}
	}

	if(!fileContent)
	{
		fileContent = FindFileContentInImportPaths(moduleName, NULL, addExtension, path, pathLength, fileSize);

		if(fileContent)
		{
			if(const char *pos = moduleName.rfind('/'))
			{
				NULLC::SafeSprintf(nextModuleRootBuf, nextModuleRootLength, "%.*s", unsigned(pos - moduleName.begin), moduleName.begin);

				nextModuleRoot = nextModuleRootBuf;
			}
		}
	}

	if(!fileContent)
	{
		*errorPos = NULL;

		if(errorBuf && errorBufSize)
			NULLC::SafeSprintf(errorBuf, errorBufSize, "ERROR: module file '%.*s' could not be opened", moduleName.length(), moduleName.begin);

		return NULL;
	}

	if(statistics)
		statistics->Finish("Extra", NULLCTime::clockMicro());

	char *bytecode = BuildModuleFromSource(allocator, path, nextModuleRoot, fileContent, fileSize, errorPos, errorBuf, errorBufSize, optimizationLevel, activeImports, statistics);

	NULLC::fileFree(fileContent);

	return bytecode;
}

bool AddModuleFunction(Allocator *allocator, const char* module, void (*ptrRaw)(), void *funcWrap, void (*ptrWrap)(void *func, char* retBuf, char* argBuf), const char* name, int index, const char **errorPos, char *errorBuf, unsigned errorBufSize, int optimizationLevel)
{
	const char *bytecode = BinaryCache::FindBytecode(module, true);

	// Create module if not found
	if(!bytecode)
		bytecode = BuildModuleFromPath(allocator, InplaceStr(module), NULL, true, errorPos, errorBuf, errorBufSize, optimizationLevel, ArrayView<InplaceStr>(), NULL);

	if(!bytecode)
		return false;

	unsigned hash = NULLC::GetStringHash(name);
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
				fInfo->regVmAddress = -1;

				fInfo->funcPtrRaw = ptrRaw;
				fInfo->funcPtrWrapTarget = funcWrap;
				fInfo->funcPtrWrap = ptrWrap;

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

		NULLC::SafeSprintf(errorBuf, errorBufSize, "ERROR: function '%s' or one of it's overload is not found in module '%s'", name, module);

		return false;
	}

	return true;
}

void OutputCompilerStatistics(CompilerStatistics &statistics, unsigned outerTotalMicros)
{
	OutputContext outputCtx;

	outputCtx.stream = OutputContext::FileOpen("time_stats.txt");
	outputCtx.writeStream = OutputContext::FileWrite;

	unsigned total = statistics.Total();

	for(unsigned i = 0; i < statistics.timers.size(); i++)
	{
		CompilerStatistics::Timer &timer = statistics.timers[i];

		outputCtx.Printf("%15s %6dms (%3d%%)\n", timer.outputName.begin, timer.total / 1000, int(timer.total * 100.0 / total));
	}

	outputCtx.Printf("%15s %6dms (%6dms)\n", "total", total / 1000, outerTotalMicros / 1000);

	outputCtx.Flush();

	OutputContext::FileClose(outputCtx.stream);
	outputCtx.stream = NULL;
}
