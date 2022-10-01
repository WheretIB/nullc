import arrayview;
import binarycache;
import bytecode;
import expressiontree;
import parser;
import compilercontext;
import analyzer;
import typetree;
import vectorview;
import parsegraph;
import expressiongraph;
import std.string;
import std.memory;

auto nullcBaseCode = @"
void assert(int val);
void assert(int val, char[] message);

int operator ==(char[] a, b);
int operator !=(char[] a, b);
char[] operator +(char[] a, b);
char[] operator +=(char[] ref a, char[] b);

bool bool(bool a);
char char(char a);
short short(short a);
int int(int a);
long long(long a);
float float(float a);
double double(double a);
void bool::bool(bool a){ *this = a; }
void char::char(char a){ *this = a; }
void short::short(short a){ *this = a; }
void int::int(int a){ *this = a; }
void long::long(long a){ *this = a; }
void float::float(float a){ *this = a; }
void double::double(double a){ *this = a; }

int as_unsigned(char a);
int as_unsigned(short a);
long as_unsigned(int a);

short short(char[] str);
char[] short::str();
int int(char[] str);
char[] int::str();
long long(char[] str);
char[] long::str();
float float(char[] str);
char[] float::str(int precision = 6, bool showExponent = false);
double double(char[] str);
char[] double::str(int precision = 6, bool showExponent = false);

void ref __newS(int size, int type);
int[] __newA(int size, int count, int type);
auto ref	duplicate(auto ref obj);
void		__duplicate_array(auto[] ref dst, auto[] src);
auto[]		duplicate(auto[] arr){ auto[] r; __duplicate_array(&r, arr); return r; }
auto ref	replace(auto ref l, r);
void		swap(auto ref l, r);
int			equal(auto ref l, r);
void		assign(explicit auto ref l, auto ref r);
void		array_copy(auto[] l, r);
void		array_copy(generic dst, int offsetDst, generic src, int offsetSrc, count)
{
	for({ int i = offsetDst, k = offsetSrc; }; i < offsetDst + count; {i++; k++; })
		dst[i] = src[k];
}

void ref() __redirect(auto ref r, __function[] ref f);
void ref() __redirect_ptr(auto ref r, __function[] ref f);
// char inline array definition support
auto operator=(char[] ref dst, int[] src)
{
	if(dst.size < src.size)
		*dst = new char[src.size];
	for(int i = 0; i < src.size; i++)
		dst[i] = src[i];
	return dst;
}
// short inline array definition support
auto operator=(short[] ref dst, int[] src)
{
	if(dst.size < src.size)
		*dst = new short[src.size];
	for(int i = 0; i < src.size; i++)
		dst[i] = src[i];
	return dst;
}
// float inline array definition support
auto operator=(float[] ref dst, double[] src)
{
	if(dst.size < src.size)
		*dst = new float[src.size];
	for(int i = 0; i < src.size; i++)
		dst[i] = src[i];
	return dst;
}
// typeid retrieval from auto ref
typeid typeid(auto ref type);
int typeid.size();
// typeid comparison
int operator==(typeid a, b);
int operator!=(typeid a, b);

int __rcomp(auto ref a, b);
int __rncomp(auto ref a, b);

bool operator<(explicit auto ref a, b);
bool operator<=(explicit auto ref a, b);
bool operator>(explicit auto ref a, b);
bool operator>=(explicit auto ref a, b);

int hash_value(explicit auto ref x);

int __pcomp(void ref(int) a, void ref(int) b);
int __pncomp(void ref(int) a, void ref(int) b);

int __acomp(auto[] a, b);
int __ancomp(auto[] a, b);

int __typeCount();

auto[] ref operator=(explicit auto[] ref l, explicit auto ref r);
auto ref __aaassignrev(auto ref l, auto[] ref r);
auto ref operator[](explicit auto[] ref l, int index);

int isStackPointer(auto ref x);

// list comprehension helper functions
void auto_array_impl(auto[] ref arr, typeid type, int count);
auto[] auto_array(typeid type, int count)
{
	auto[] res;
	auto_array_impl(&res, type, count);
	return res;
}
typedef auto[] auto_array;
// function will set auto[] element to the specified one, with data reallocation if neccessary
void auto_array::set(auto ref x, int pos);
void __force_size(auto[] ref s, int size);

int isCoroutineReset(auto ref f);
void __assertCoroutine(auto ref f);
auto ref[] __getFinalizeList();
class __FinalizeProxy{ void finalize(){} }
void	__finalizeObjects()
{
	auto l = __getFinalizeList();
	for(i in l)
		i.finalize();
}
bool operator in(generic x, typeof(x)[] arr)
{
	for(i in arr)
		if(i == x)
			return true;
	return false;
}
void ref assert_derived_from_base(void ref derived, typeid base);
auto __gen_list(@T ref() y)
{
	auto[] res = auto_array(T, 1);
	int pos = 0;
	for(T x in y)
		res.set(&x, pos++);
	__force_size(&res, pos);
	
	T[] r = res;
	return r;
}
void __init_array(@T[] arr)
{
	for(int i = 0; i < arr.size; i++)
	{
		@if(T.isArray)
			__init_array(arr[i]);
		else
			arr[i].T();
	}
}
void __closeUpvalue(void ref ref l, void ref v, int offset, int size);";

bool BuildBaseModule(int optimizationLevel);

ExprModule ref AnalyzeModuleFromSource(CompilerContext ref ctx);

bool CompileModuleFromSource(CompilerContext ref ctx);

ByteCode ref GetBytecode(CompilerContext ref ctx);

bool SaveListing(CompilerContext ref ctx, string fileName);

bool TranslateToC(CompilerContext ref ctx, string fileName, string mainName, void ref(string) addDependency);

ByteCode ref BuildModuleFromSource(string modulePath, string moduleRoot, string code, StringRef ref errorPos, string ref errorBuf, int optimizationLevel, ArrayView<InplaceStr> activeImports);
ByteCode ref BuildModuleFromPath(string moduleName, string moduleRoot, bool addExtension, StringRef ref errorPos, string ref errorBuf, int optimizationLevel, ArrayView<InplaceStr> activeImports);

bool BuildBaseModule(int optimizationLevel)
{
    StringRef errorPos;
	string errorBuf;

	if(!BuildModuleFromSource(string("$base$.nc"), string(), string(nullcBaseCode), &errorPos, &errorBuf, optimizationLevel, ArrayView<InplaceStr>()))
	{
		assert(false, "Failed to compile base NULLC module");
		return false;
	}

/*#ifndef NULLC_NO_EXECUTOR
	nullcBindModuleFunctionHelper("$base$", NULLC.Assert, "assert", 0);
	nullcBindModuleFunctionHelper("$base$", NULLC.Assert2, "assert", 1);

	nullcBindModuleFunctionHelper("$base$", NULLC.StrEqual, "==", 0);
	nullcBindModuleFunctionHelper("$base$", NULLC.StrNEqual, "!=", 0);
	nullcBindModuleFunctionHelper("$base$", NULLC.StrConcatenate, "+", 0);
	nullcBindModuleFunctionHelper("$base$", NULLC.StrConcatenateAndSet, "+=", 0);

	nullcBindModuleFunctionHelper("$base$", NULLC.Int, "bool", 0);
	nullcBindModuleFunctionHelper("$base$", NULLC.Char, "char", 0);
	nullcBindModuleFunctionHelper("$base$", NULLC.Short, "short", 0);
	nullcBindModuleFunctionHelper("$base$", NULLC.Int, "int", 0);
	nullcBindModuleFunctionHelper("$base$", NULLC.Long, "long", 0);
	nullcBindModuleFunctionHelper("$base$", NULLC.Float, "float", 0);
	nullcBindModuleFunctionHelper("$base$", NULLC.Double, "double", 0);

	nullcBindModuleFunctionHelper("$base$", NULLC.UnsignedValueChar, "as_unsigned", 0);
	nullcBindModuleFunctionHelper("$base$", NULLC.UnsignedValueShort, "as_unsigned", 1);
	nullcBindModuleFunctionHelper("$base$", NULLC.UnsignedValueInt, "as_unsigned", 2);

	nullcBindModuleFunctionHelper("$base$", NULLC.StrToShort, "short", 1);
	nullcBindModuleFunctionHelper("$base$", NULLC.ShortToStr, "short.str", 0);
	nullcBindModuleFunctionHelper("$base$", NULLC.StrToInt, "int", 1);
	nullcBindModuleFunctionHelper("$base$", NULLC.IntToStr, "int.str", 0);
	nullcBindModuleFunctionHelper("$base$", NULLC.StrToLong, "long", 1);
	nullcBindModuleFunctionHelper("$base$", NULLC.LongToStr, "long.str", 0);
	nullcBindModuleFunctionHelper("$base$", NULLC.StrToFloat, "float", 1);
	nullcBindModuleFunctionHelper("$base$", NULLC.FloatToStr, "float.str", 0);
	nullcBindModuleFunctionHelper("$base$", NULLC.StrToDouble, "double", 1);
	nullcBindModuleFunctionHelper("$base$", NULLC.DoubleToStr, "double.str", 0);

	nullcBindModuleFunctionHelper("$base$", NULLC.AllocObject, "__newS", 0);
	nullcBindModuleFunctionHelper("$base$", NULLC.AllocArray, "__newA", 0);
	nullcBindModuleFunctionHelper("$base$", NULLC.CopyObject, "duplicate", 0);
	nullcBindModuleFunctionHelper("$base$", NULLC.CopyArray, "__duplicate_array", 0);
	nullcBindModuleFunctionHelper("$base$", NULLC.ReplaceObject, "replace", 0);
	nullcBindModuleFunctionHelper("$base$", NULLC.SwapObjects, "swap", 0);
	nullcBindModuleFunctionHelper("$base$", NULLC.CompareObjects, "equal", 0);
	nullcBindModuleFunctionHelper("$base$", NULLC.AssignObject, "assign", 0);

	nullcBindModuleFunctionHelper("$base$", NULLC.ArrayCopy, "array_copy", 0);

	nullcBindModuleFunctionHelper("$base$", NULLC.FunctionRedirect, "__redirect", 0);
	nullcBindModuleFunctionHelper("$base$", NULLC.FunctionRedirectPtr, "__redirect_ptr", 0);

	nullcBindModuleFunctionHelper("$base$", NULLC.Typeid, "typeid", 0);
	nullcBindModuleFunctionHelper("$base$", NULLC.TypeSize, "typeid.size$", 0);
	nullcBindModuleFunctionHelper("$base$", NULLC.TypesEqual, "==", 1);
	nullcBindModuleFunctionHelper("$base$", NULLC.TypesNEqual, "!=", 1);

	nullcBindModuleFunctionHelper("$base$", NULLC.RefCompare, "__rcomp", 0);
	nullcBindModuleFunctionHelper("$base$", NULLC.RefNCompare, "__rncomp", 0);
	nullcBindModuleFunctionHelper("$base$", NULLC.RefLCompare, "<", 0);
	nullcBindModuleFunctionHelper("$base$", NULLC.RefLECompare, "<=", 0);
	nullcBindModuleFunctionHelper("$base$", NULLC.RefGCompare, ">", 0);
	nullcBindModuleFunctionHelper("$base$", NULLC.RefGECompare, ">=", 0);

	nullcBindModuleFunctionHelper("$base$", NULLC.RefHash, "hash_value", 0);

	nullcBindModuleFunctionHelper("$base$", NULLC.FuncCompare, "__pcomp", 0);
	nullcBindModuleFunctionHelper("$base$", NULLC.FuncNCompare, "__pncomp", 0);

	nullcBindModuleFunctionHelper("$base$", NULLC.ArrayCompare, "__acomp", 0);
	nullcBindModuleFunctionHelper("$base$", NULLC.ArrayNCompare, "__ancomp", 0);

	nullcBindModuleFunctionHelper("$base$", NULLC.TypeCount, "__typeCount", 0);

	nullcBindModuleFunctionHelper("$base$", NULLC.AutoArrayAssign, "=", 3);
	nullcBindModuleFunctionHelper("$base$", NULLC.AutoArrayAssignRev, "__aaassignrev", 0);
	nullcBindModuleFunctionHelper("$base$", NULLC.AutoArrayIndex, "[]", 0);

	nullcBindModuleFunctionHelper("$base$", IsPointerUnmanaged, "isStackPointer", 0);

	nullcBindModuleFunctionHelper("$base$", NULLC.AutoArray, "auto_array_impl", 0);
	nullcBindModuleFunctionHelper("$base$", NULLC.AutoArraySet, "auto[].set", 0);
	nullcBindModuleFunctionHelper("$base$", NULLC.ShrinkAutoArray, "__force_size", 0);

	nullcBindModuleFunctionHelper("$base$", NULLC.IsCoroutineReset, "isCoroutineReset", 0);
	nullcBindModuleFunctionHelper("$base$", NULLC.AssertCoroutine, "__assertCoroutine", 0);

	nullcBindModuleFunctionHelper("$base$", NULLC.GetFinalizationList, "__getFinalizeList", 0);

	nullcBindModuleFunctionHelper("$base$", NULLC.AssertDerivedFromBase, "assert_derived_from_base", 0);

	nullcBindModuleFunctionHelper("$base$", NULLC.CloseUpvalue, "__closeUpvalue", 0);
#endif*/

	return true;
}

int GetErrorLocationLineNumber(string codeStart, int errorPos)
{
	if(codeStart.empty())
		return 0;

	if(!errorPos)
		return 0;

	int line = 1;
	for(int pos = 0; pos < errorPos; pos++)
		line += codeStart[pos] == '\n';

	return line;
}

void AddErrorLocationInfo(string codeStart, int errorPos, string ref errorBuf)
{
	/*if(!errorBuf || !errorBufSize)
		return;

	char *errorCurr = errorBuf + strlen(errorBuf);

	if(!codeStart || !errorPos)
	{
		errorCurr += NULLC.SafeSprintf(errorCurr, errorBufSize - int(errorCurr - errorBuf), "\n");
		return;
	}

	const char *codeEnd = codeStart + strlen(codeStart);

	if(errorPos < codeStart || errorPos > codeEnd)
	{
		errorCurr += NULLC.SafeSprintf(errorCurr, errorBufSize - int(errorCurr - errorBuf), "\n");
		return;
	}

	const char *start = errorPos;
	const char *end = start == codeEnd ? start : start + 1;

	while(start > codeStart && *(start - 1) != '\r' && *(start - 1) != '\n')
		start--;

	while(*end && *end != '\r' && *end != '\n')
		end++;

	if(errorBufSize > int(errorCurr - errorBuf))
		*(errorCurr++) = '\n';

	char *errorCurrBefore = errorCurr;

	if(int line = GetErrorLocationLineNumber(codeStart, errorPos))
		errorCurr += NULLC.SafeSprintf(errorCurr, errorBufSize - int(errorCurr - errorBuf), " at line %d: '", line);
	else
		errorCurr += NULLC.SafeSprintf(errorCurr, errorBufSize - int(errorCurr - errorBuf), " at '");

	int spacing = int(errorCurr - errorCurrBefore);

	for(const char *pos = start; *pos && pos < end; pos++)
	{
		if(errorBufSize > int(errorCurr - errorBuf))
			*(errorCurr++) = *pos == '\t' ? ' ' : *pos;
	}

	errorCurr += NULLC.SafeSprintf(errorCurr, errorBufSize - int(errorCurr - errorBuf), "'\n");

	for(int i = 0; i < spacing + int(errorPos - start); i++)
	{
		if(errorBufSize > int(errorCurr - errorBuf))
			*(errorCurr++) = ' ';
	}

	errorCurr += NULLC.SafeSprintf(errorCurr, errorBufSize - int(errorCurr - errorBuf), "^\n");

	errorBuf[errorBufSize - 1] = '\0';*/
}
/*
bool HasSourceCode(ByteCode ref bytecode, const char *position)
{
	char *code = FindSource(bytecode);
	int length = bytecode.sourceSize;

	if(position >= code && position <= code + length)
		return true;

	return false;
}

ModuleData ref FindModuleWithSourceLocation(ExpressionContext ref ctx, const char *position)
{
	for(int i = 0; i < ctx.imports.size(); i++)
	{
		if(HasSourceCode(ctx.imports[i].bytecode, position))
			return ctx.imports[i];
	}

	return nullptr;
}

InplaceStr FindModuleNameWithSourceLocation(ExpressionContext ref ctx, const char *position)
{
	if(ModuleData ref moduleData = FindModuleWithSourceLocation(ctx, position))
		return moduleData.name;

	return InplaceStr();
}

const char* FindModuleCodeWithSourceLocation(ExpressionContext ref ctx, const char *position)
{
	if(position >= ctx.code && position <= ctx.code + strlen(ctx.code))
		return ctx.code;

	for(int i = 0; i < ctx.imports.size(); i++)
	{
		if(HasSourceCode(ctx.imports[i].bytecode, position))
			return FindSource(ctx.imports[i].bytecode);
	}

	return nullptr;
}
*/
ExprModule ref AnalyzeModuleFromSource(CompilerContext ref ctx)
{
	//TRACE_SCOPE("compiler", "AnalyzeModuleFromSource");

	ParseContext ref parseCtx = &ctx.parseCtx;

	parseCtx.bytecodeBuilder = BuildModuleFromPath;

	parseCtx.errorBuf = ctx.errorBuf;

	ctx.synModule = Parse(parseCtx, ctx.code/*, ctx.moduleRoot*/);

	if(ctx.enableLogFiles && ctx.synModule)
	{
		//TRACE_SCOPE("compiler", "Debug.syntax_graph");

		ParseGraphContext parseGraphCtx;

		parseGraphCtx.output.Open("syntax_graph.txt", "w");

		PrintGraph(parseGraphCtx, ctx.synModule, "");

		parseGraphCtx.output.Close();
	}

	if(!ctx.synModule)
	{
		if(parseCtx.errorPos.string)
			ctx.errorPos = parseCtx.errorPos;

		return nullptr;
	}

	//printf("# Parse memory %dkb\n", ctx.allocator.requested() / 1024);

	ExpressionContext ref exprCtx = &ctx.exprCtx;

	exprCtx.memoryLimit = ctx.exprMemoryLimit;

	exprCtx.errorBuf = ctx.errorBuf;

	ctx.exprModule = Analyze(exprCtx, ctx.synModule, ctx.code/*, ctx.moduleRoot*/);

	if(ctx.enableLogFiles && ctx.exprModule)
	{
		ExpressionGraphContext exprGraphCtx;

		exprGraphCtx.output.Open("expr_graph.txt", "w");

		PrintGraph(exprGraphCtx, ctx.exprModule, "");

		exprGraphCtx.output.Close();
	}

	if(!ctx.exprModule || exprCtx.errorCount != 0 || parseCtx.errorCount != 0)
	{
		if(parseCtx.errorPos.string)
			ctx.errorPos = parseCtx.errorPos;
		else if(exprCtx.errorPos.string)
			ctx.errorPos = exprCtx.errorPos;

		return nullptr;
	}

	//printf("# Compile memory %dkb\n", ctx.allocator.requested() / 1024);

	return ctx.exprModule;
}

bool CompileModuleFromSource(CompilerContext ref ctx)
{
	//TRACE_SCOPE("compiler", "CompileModuleFromSource");

	if(!AnalyzeModuleFromSource(ctx))
		return false;

	ExpressionContext ref exprCtx = &ctx.exprCtx;

	/*ctx.vmModule = CompileVm(exprCtx, ctx.exprModule, ctx.code);

	if(!ctx.vmModule)
	{
		ctx.errorPos = nullptr;

		if(ctx.errorBuf && ctx.errorBufSize)
			NULLC.SafeSprintf(ctx.errorBuf, ctx.errorBufSize, "ERROR: internal compiler error: failed to create VmModule");

		return false;
	}*/

	//printf("# Instruction memory %dkb\n", pool.GetSize() / 1024);

	if(ctx.enableLogFiles)
	{
		/*TRACE_SCOPE("compiler", "Debug.inst_graph");

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
			ctx.outputCtx.stream = nullptr;
		}*/
	}

	// Build LLVM module is support is enabled or just to test execution paths in debug build
/*#if defined(NULLC_LLVM_SUPPORT)
	ctx.llvmModule = CompileLlvm(exprCtx, ctx.exprModule);
#elif !defined(NDEBUG)
	ctx.llvmModule = CompileLlvm(exprCtx, ctx.exprModule);
#endif*/

	/*for(VmFunction ref function = ctx.vmModule.functions.head; function; function = function.next)
	{
		if(!function.firstBlock)
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

			for(int i = 0; i < 6; i++)
			{
				TRACE_SCOPE("compiler", "iteration");

				int before = ctx.vmModule.peepholeOptimizations + ctx.vmModule.constantPropagations + ctx.vmModule.deadCodeEliminations + ctx.vmModule.controlFlowSimplifications + ctx.vmModule.loadStorePropagations + ctx.vmModule.commonSubexprEliminations + ctx.vmModule.deadAllocaStoreEliminations;

				RunVmPass(exprCtx, ctx.vmModule, function, VM_PASS_OPT_CONSTANT_PROPAGATION);
				RunVmPass(exprCtx, ctx.vmModule, function, VM_PASS_OPT_LOAD_STORE_PROPAGATION);
				RunVmPass(exprCtx, ctx.vmModule, function, VM_PASS_OPT_COMMON_SUBEXPRESSION_ELIMINATION);
				RunVmPass(exprCtx, ctx.vmModule, function, VM_PASS_OPT_PEEPHOLE);
				RunVmPass(exprCtx, ctx.vmModule, function, VM_PASS_OPT_DEAD_CODE_ELIMINATION);
				RunVmPass(exprCtx, ctx.vmModule, function, VM_PASS_OPT_CONTROL_FLOW_SIPLIFICATION);
				RunVmPass(exprCtx, ctx.vmModule, function, VM_PASS_OPT_DEAD_CODE_ELIMINATION);
				RunVmPass(exprCtx, ctx.vmModule, function, VM_PASS_OPT_DEAD_ALLOCA_STORE_ELIMINATION);

				int after = ctx.vmModule.peepholeOptimizations + ctx.vmModule.constantPropagations + ctx.vmModule.deadCodeEliminations + ctx.vmModule.controlFlowSimplifications + ctx.vmModule.loadStorePropagations + ctx.vmModule.commonSubexprEliminations + ctx.vmModule.deadAllocaStoreEliminations;

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

	RunVmPass(exprCtx, ctx.vmModule, VM_PASS_UPDATE_LIVE_SETS);

	if(ctx.optimizationLevel >= 2)
		RunVmPass(exprCtx, ctx.vmModule, VM_PASS_PREPARE_SSA_EXIT);

	RunVmPass(exprCtx, ctx.vmModule, VM_PASS_LEGALIZE_BITCASTS);
	RunVmPass(exprCtx, ctx.vmModule, VM_PASS_LEGALIZE_EXTRACTS);

	RunVmPass(exprCtx, ctx.vmModule, VM_PASS_CREATE_ALLOCA_STORAGE);

	if(ctx.enableLogFiles)
	{
		TRACE_SCOPE("compiler", "Debug.inst_graph_opt");

		assert(!ctx.outputCtx.stream);
		ctx.outputCtx.stream = ctx.outputCtx.openStream("inst_graph_opt.txt");

		if(ctx.outputCtx.stream)
		{
			InstructionVMGraphContext instGraphCtx = InstructionVMGraphContext(ctx.outputCtx);

			instGraphCtx.showUsers = true;
			instGraphCtx.displayAsTree = false;
			instGraphCtx.showFullTypes = false;
			instGraphCtx.showSource = true;

			PrintGraph(instGraphCtx, ctx.vmModule);

			ctx.outputCtx.closeStream(ctx.outputCtx.stream);
			ctx.outputCtx.stream = nullptr;
		}
	}

	ctx.regVmLoweredModule = RegVmLowerModule(exprCtx, ctx.vmModule);

	if(!ctx.regVmLoweredModule.functions.empty() && ctx.regVmLoweredModule.functions.back().hasRegisterOverflow)
	{
		RegVmLoweredFunction ref function = ctx.regVmLoweredModule.functions.back();

		if(function.registerOverflowLocation && function.registerOverflowLocation.source)
			ctx.errorPos = function.registerOverflowLocation.source.begin.pos;
		else
			ctx.errorPos = nullptr;

		if(ctx.errorBuf && ctx.errorBufSize)
		{
			NULLC.SafeSprintf(ctx.errorBuf, ctx.errorBufSize, "ERROR: internal compiler error: register count overflow");

			if(ctx.errorPos)
				AddErrorLocationInfo(FindModuleCodeWithSourceLocation(ctx.exprCtx, ctx.errorPos), ctx.errorPos, ctx.errorBuf, ctx.errorBufSize);
		}

		return false;
	}

	RegVmFinalizeModule(ctx.instRegVmFinalizeCtx, ctx.regVmLoweredModule);*/

	if(ctx.enableLogFiles)
	{
		/*TRACE_SCOPE("compiler", "Debug.inst_graph_reg_low");

		assert(!ctx.outputCtx.stream);
		ctx.outputCtx.stream = ctx.outputCtx.openStream("inst_graph_reg_low.txt");

		if(ctx.outputCtx.stream)
		{
			InstructionRegVmLowerGraphContext instRegVmLowerGraphCtx(ctx.outputCtx);

			instRegVmLowerGraphCtx.showSource = true;

			PrintGraph(instRegVmLowerGraphCtx, ctx.regVmLoweredModule);

			ctx.outputCtx.closeStream(ctx.outputCtx.stream);
			ctx.outputCtx.stream = nullptr;
		}*/
	}

	return true;
}

ByteCode ref GetBytecode(CompilerContext ref ctx)
{
	//TRACE_SCOPE("compiler", "GetBytecode");

	// find out the size of generated bytecode
	int size = sizeof(ByteCode);

	size += ctx.exprCtx.types.size() * sizeof(ExternTypeInfo);

	int symbolStorageSize = 0;
	int allMemberCount = 0, allConstantCount = 0;
	int typedefCount = 0;

	for(int i = 0; i < ctx.exprCtx.types.size(); i++)
	{
		TypeBase ref type = ctx.exprCtx.types[i];

		type.typeIndex = i;

		symbolStorageSize += type.name.length() + 1;

		if(isType with<TypeUnsizedArray>(type))
			continue;

		else if(TypeClass ref typeClass = getType with<TypeClass>(type))
		{
			for(MatchData ref curr = typeClass.generics.head; curr; curr = curr.next)
			{
				typedefCount++;

				symbolStorageSize += curr.name.name.length() + 1;
			}

			for(MatchData ref curr = typeClass.aliases.head; curr; curr = curr.next)
			{
				typedefCount++;

				symbolStorageSize += curr.name.name.length() + 1;
			}

			for(MemberHandle ref curr = typeClass.members.head; curr; curr = curr.next)
			{
				if(curr.variable.type.hasPointers)
					allMemberCount++;

				if(curr.variable.name.name[0] == '$')
					continue;

				allMemberCount++;

				symbolStorageSize += curr.variable.name.name.length() + 1;
			}

			for(ConstantData ref curr = typeClass.constants.head; curr; curr = curr.next)
			{
				allConstantCount++;

				symbolStorageSize += curr.name.name.length() + 1;
			}
		}
		else if(TypeStruct ref typeStruct = getType with<TypeStruct>(type))
		{
			for(MemberHandle ref curr = typeStruct.members.head; curr; curr = curr.next)
			{
				if(curr.variable.type.hasPointers)
					allMemberCount++;

				if(curr.variable.name.name[0] == '$')
					continue;

				allMemberCount++;

				symbolStorageSize += curr.variable.name.name.length() + 1;
			}

			for(ConstantData ref curr = typeStruct.constants.head; curr; curr = curr.next)
			{
				allConstantCount++;

				symbolStorageSize += curr.name.name.length() + 1;
			}
		}
		else if(TypeFunction ref typeFunction = getType with<TypeFunction>(type))
		{
			allMemberCount += typeFunction.arguments.size() + 1;
		}
		else if(TypeGenericClass ref typeGenericClass = getType with<TypeGenericClass>(type))
		{
			for(TypeHandle ref curr = typeGenericClass.generics.head; curr; curr = curr.next)
			{
				typedefCount++;

				symbolStorageSize += 1;
			}
		}
	}

	size += allMemberCount * sizeof(ExternMemberInfo);

	int offsetToConstants = size;
	size += allConstantCount * sizeof(ExternConstantInfo);

	int offsetToModule = size;
	size += ctx.exprCtx.imports.size() * sizeof(ExternModuleInfo);

	for(int i = 0; i < ctx.exprCtx.imports.size(); i++)
	{
		ModuleData ref module = ctx.exprCtx.imports[i];

		symbolStorageSize += module.name.length() + 1;
	}

	int offsetToVar = size;
	int globalVarCount = 0, exportVarCount = 0;
	int externVariableInfoCount = 0;

	for(int i = 0; i < ctx.exprCtx.variables.size(); i++)
	{
		VariableData ref variable = ctx.exprCtx.variables[i];

		if(variable.importModule != nullptr)
			continue;

		if(!ctx.exprCtx.GlobalScopeFrom(variable.scope))
			continue;

		externVariableInfoCount++;

		globalVarCount++;

		if(variable.scope == ctx.exprCtx.globalScope || variable.scope.ownerNamespace)
			exportVarCount++;

		symbolStorageSize += variable.name.name.length() + 1;
	}

	for(int i = 0; i < ctx.exprCtx.functions.size(); i++)
	{
		FunctionData ref function = ctx.exprCtx.functions[i];

		if(function.importModule != nullptr)
			continue;

		for(MatchData ref curr = function.generics.head; curr; curr = curr.next)
		{
			externVariableInfoCount++;

			symbolStorageSize += curr.name.name.length() + 1;
		}
	}

	size += externVariableInfoCount * sizeof(ExternVarInfo);

	int offsetToFunc = size;

	int localCount = 0;

	int exportedFunctionCount = 0;

	for(int i = 0; i < ctx.exprCtx.functions.size(); i++)
	{
		FunctionData ref function = ctx.exprCtx.functions[i];

		if(function.importModule != nullptr)
			continue;

		exportedFunctionCount++;

		symbolStorageSize += function.name.name.length() + 1;

		localCount += function.arguments.size();

		for(int i = 0; i < function.arguments.size(); i++)
		{
			ArgumentData argument = function.arguments[i];

			symbolStorageSize += argument.name.name.length() + 1;
		}

		if(VariableData ref variable = function.contextArgument)
		{
			localCount++;

			symbolStorageSize += variable.name.name.length() + 1;
		}

		if (!ctx.exprCtx.IsGenericFunction(function))
		{
			for(int i = 0; i < function.functionScope.allVariables.size(); i++)
			{
				VariableData ref variable = function.functionScope.allVariables[i];

				VariableHandle ref argumentVariable = nullptr;

				for(VariableHandle ref curr = function.argumentVariables.head; curr; curr = curr.next)
				{
					if(curr.variable == variable)
					{
						argumentVariable = curr;
						break;
					}
				}

				if(argumentVariable || variable == function.contextArgument)
					continue;

				if(variable.isAlloca && variable.users.empty())
					continue;

				if(variable.lookupOnly)
					continue;

				localCount++;

				symbolStorageSize += variable.name.name.length() + 1;
			}
		}

		localCount += function.upvalues.size();

		for(int i = 0; i < function.upvalues.size(); i++)
		{
			UpvalueData ref upvalue = &function.upvalues[i];

			symbolStorageSize += upvalue.variable.name.name.length() + 1;
		}
	}

	size += exportedFunctionCount * sizeof(ExternFuncInfo);

	int offsetToFirstLocal = size;

	size += localCount * sizeof(ExternLocalInfo);

	for(int i = 0; i < ctx.exprCtx.globalScope.aliases.size(); i++)
	{
		AliasData ref alias = ctx.exprCtx.globalScope.aliases[i];

		typedefCount++;

		symbolStorageSize += alias.name.name.length() + 1;
	}

	int offsetToTypedef = size;
	size += typedefCount * sizeof(ExternTypedefInfo);

	for(int i = 0; i < ctx.exprCtx.namespaces.size(); i++)
	{
		NamespaceData ref nameSpace = ctx.exprCtx.namespaces[i];

		symbolStorageSize += nameSpace.name.name.length() + 1;
	}

	int offsetToNamespace = size;
	size += ctx.exprCtx.namespaces.size() * sizeof(ExternNamespaceInfo);

	int offsetToRegVmCode = size;
	//size += ctx.instRegVmFinalizeCtx.cmds.size() * sizeof(RegVmCmd);

	int sourceLength = ctx.code.size;

	int regVmInfoCount = 0;

	/*for(int i = 0; i < ctx.instRegVmFinalizeCtx.locations.size(); i++)
	{
		SynBase ref location = ctx.instRegVmFinalizeCtx.locations[i];

		if(location && !location.isInternal)
			regVmInfoCount++;
	}*/

	int offsetToRegVmInfo = size;
	size += sizeof(ExternSourceInfo) * regVmInfoCount;

	int offsetToRegVmConstants = size;
	//size += ctx.instRegVmFinalizeCtx.constants.size() * sizeof(ctx.instRegVmFinalizeCtx.constants[0]);

	int offsetToRegVmRegKillInfo = size;
	//size += ctx.instRegVmFinalizeCtx.regKillInfo.size() * sizeof(ctx.instRegVmFinalizeCtx.regKillInfo[0]);

	int offsetToSymbols = size;
	size += symbolStorageSize;

	int offsetToSource = size;
	size += sourceLength;

	int offsetToLLVM = size;
	//size += ctx.llvmModule ? ctx.llvmModule.moduleSize : 0;

/*#ifdef VERBOSE_DEBUG_OUTPUT
	printf("Statistics. Overall: %d bytes\r\n", size);
	printf("Types: %db, ", offsetToModule - sizeof(ByteCode));
	printf("Modules: %db, ", offsetToVar - offsetToModule);
	printf("Variables: %db, ", offsetToFunc - offsetToVar);
	printf("Functions: %db\r\n", offsetToFirstLocal - offsetToFunc);
	printf("Locals: %db, ", offsetToCode - offsetToFirstLocal);
	printf("Code: %db, ", offsetToInfo - offsetToCode);
	printf("Code info: %db, ", offsetToSymbols - offsetToInfo);
	printf("Symbols: %db, ", offsetToSource - offsetToSymbols);
	printf("Source: %dbr\n", size - offsetToSource);
#endif*/

	ByteCode ref code = new ByteCode();
	code.size = size;

	code.typeCount = ctx.exprCtx.types.size();

	code.dependsCount = ctx.exprCtx.imports.size();
	code.offsetToFirstModule = offsetToModule;

	code.globalVarSize = ctx.exprCtx.globalScope.dataSize;
	// Make sure modules that are linked together will not break global variable alignment
	code.globalVarSize = (code.globalVarSize + 0xf) & ~0xf;

	code.variableCount = globalVarCount;
	code.variableExportCount = exportVarCount;
	code.offsetToFirstVar = offsetToVar;

	code.functionCount = ctx.exprCtx.functions.size();
	code.moduleFunctionCount = ctx.exprCtx.functions.size() - exportedFunctionCount;
	code.offsetToFirstFunc = offsetToFunc;

	code.localCount = localCount;
	code.offsetToLocals = offsetToFirstLocal;

	code.closureListCount = 0;

	//code.regVmCodeSize = ctx.instRegVmFinalizeCtx.cmds.size();
	code.regVmOffsetToCode = offsetToRegVmCode;

	//code.regVmConstantCount = ctx.instRegVmFinalizeCtx.constants.size();
	code.regVmOffsetToConstants = offsetToRegVmConstants;

	//code.regVmRegKillInfoCount = ctx.instRegVmFinalizeCtx.regKillInfo.size();
	code.regVmOffsetToRegKillInfo = offsetToRegVmRegKillInfo;

	code.symbolLength = symbolStorageSize;
	code.offsetToSymbols = offsetToSymbols;

	code.constantCount = allConstantCount;
	code.offsetToConstants = offsetToConstants;

	code.namespaceCount = ctx.exprCtx.namespaces.size();
	code.offsetToNamespaces = offsetToNamespace;

	code.debugSymbols.resize(symbolStorageSize);
	code.types.resize(code.typeCount);
	code.typeMembers.resize(allMemberCount);
	code.typeConstants.resize(allConstantCount);

	VectorView<char> debugSymbols = VectorView<char>(FindSymbols(code), symbolStorageSize);

	VectorView<ExternTypeInfo> tInfo = VectorView<ExternTypeInfo>(FindFirstType(code), code.typeCount);
	VectorView<ExternMemberInfo> memberList = VectorView<ExternMemberInfo>(FindFirstMember(code), allMemberCount);
	VectorView<ExternConstantInfo> constantList = VectorView<ExternConstantInfo>(FindFirstConstant(code), allConstantCount);

	for(int i = 0; i < ctx.exprCtx.types.size(); i++)
	{
		TypeBase ref type = ctx.exprCtx.types[i];

		ExternTypeInfo ref target = tInfo.push_back();

		target.offsetToName = debugSymbols.count;
		debugSymbols.push_back(type.name.data, type.name.begin, type.name.length());
		debugSymbols.push_back(0);

		target.nameHash = type.nameHash;

		target.size = type.size;
		target.defaultAlign = type.alignment;
		target.padding = type.padding;

		target.constantOffset = 0;
		target.constantCount = 0;

		target.arrSizeOrMemberCount = 0;
		target.subTypeOrMemberOffset = 0;

		target.pointerCount = type.hasPointers ? 1 : 0;

		if(ModuleData ref moduleData = type.importModule)
			target.definitionModule = moduleData.dependencyIndex;
		else
			target.definitionModule = 0;

		target.definitionLocationStart = 0;
		target.definitionLocationEnd = 0;
		target.definitionLocationName = 0;

		Lexeme[] sourceStreamStart = type.importModule ? type.importModule.lexStream : ctx.parseCtx.lexer.lexems.data;

		if(TypeClass ref typeClass = getType with<TypeClass>(type))
		{
			if(typeClass.source.begin.owner.lexems.data == sourceStreamStart)
			{
				target.definitionLocationStart = int(typeClass.source.begin.index);
				target.definitionLocationEnd = int(typeClass.source.end.index);

				if(typeClass.identifier.begin)
					target.definitionLocationName = int(typeClass.identifier.begin.index);
			}
		}
		else if(TypeEnum ref typeEnum = getType with<TypeEnum>(type))
		{
			if(typeEnum.source.begin.owner.lexems.data == sourceStreamStart)
			{
				target.definitionLocationStart = int(typeEnum.source.begin.index);
				target.definitionLocationEnd = int(typeEnum.source.end.index);

				if(typeEnum.identifier.begin)
					target.definitionLocationName = int(typeEnum.identifier.begin.index);
			}
		}
		else if(TypeGenericClassProto ref typeGenericClassProto = getType with<TypeGenericClassProto>(type))
		{
			if(typeGenericClassProto.source.begin.owner.lexems.data == sourceStreamStart)
			{
				target.definitionLocationStart = int(typeGenericClassProto.source.begin.index);
				target.definitionLocationEnd = int(typeGenericClassProto.source.end.index);

				if(typeGenericClassProto.identifier.begin)
					target.definitionLocationName = int(typeGenericClassProto.identifier.begin.index);
			}
		}

		target.definitionOffsetStart = -1;
		target.definitionOffset = -1;
		target.genericTypeCount = 0;

		target.typeFlags = 0;

		if(type.isGeneric)
			target.typeFlags |= int(TypeFlags.TYPE_DEPENDS_ON_GENERIC);

		target.namespaceHash = -1;

		if(TypeFunction ref typeFunction = getType with<TypeFunction>(type))
		{
			target.type = TypeCategory.TYPE_COMPLEX;

			target.subCat = SubCategory.CAT_FUNCTION;
			target.arrSizeOrMemberCount = typeFunction.arguments.size();
			target.subTypeOrMemberOffset = memberList.count;

			ExternMemberInfo ref member = memberList.push_back();

			member.type = typeFunction.returnType.typeIndex;
			member.offset = 0;

			for(TypeHandle ref curr = typeFunction.arguments.head; curr; curr = curr.next)
			{
				ExternMemberInfo ref member = memberList.push_back();

				member.type = curr.type.typeIndex;
				member.offset = 0;
			}
		}
		else if(TypeArray ref typeArray = getType with<TypeArray>(type))
		{
			target.type = TypeCategory.TYPE_COMPLEX;

			target.subCat = SubCategory.CAT_ARRAY;
			target.arrSizeOrMemberCount = typeArray.length;
			target.subTypeOrMemberOffset = typeArray.subType.typeIndex;
		}
		else if(TypeUnsizedArray ref typeUnsizedArray = getType with<TypeUnsizedArray>(type))
		{
			target.type = TypeCategory.TYPE_COMPLEX;

			target.subCat = SubCategory.CAT_ARRAY;
			target.arrSizeOrMemberCount = -1;
			target.subTypeOrMemberOffset = typeUnsizedArray.subType.typeIndex;
		}
		else if(TypeRef ref typeRef = getType with<TypeRef>(type))
		{
			target.type = NULLC_PTR_SIZE == 4 ? TypeCategory.TYPE_INT : TypeCategory.TYPE_LONG;

			target.subCat = SubCategory.CAT_POINTER;
			target.subTypeOrMemberOffset = typeRef.subType.typeIndex;
		}
		else if(TypeStruct ref typeStruct = getType with<TypeStruct>(type))
		{
			target.type = TypeCategory.TYPE_COMPLEX;

			if(TypeEnum ref typeEnum = getType with<TypeEnum>(type))
			{
				target.type = TypeCategory.TYPE_INT;

				if(ScopeData ref scope = ctx.exprCtx.NamespaceScopeFrom(typeEnum.scope))
					target.namespaceHash = scope.ownerNamespace.fullNameHash;
			}
			else if(TypeClass ref typeClass = getType with<TypeClass>(type))
			{
				if(ScopeData ref scope = ctx.exprCtx.NamespaceScopeFrom(typeClass.scope))
					target.namespaceHash = scope.ownerNamespace.fullNameHash;

				if(typeClass.isExtendable)
					target.typeFlags |= int(TypeFlags.TYPE_IS_EXTENDABLE);

				if(typeClass.hasFinalizer)
					target.typeFlags |= int(TypeFlags.TYPE_HAS_FINALIZER);

				if(typeClass.isInternal)
					target.typeFlags |= int(TypeFlags.TYPE_IS_INTERNAL);

				if(typeClass.completed)
					target.typeFlags |= int(TypeFlags.TYPE_IS_COMPLETED);

				if(typeClass.baseClass)
					target.baseType = typeClass.baseClass.typeIndex;

				if(!typeClass.generics.empty())
				{
					target.definitionOffset = 0x80000000 | typeClass.proto.typeIndex;

					target.genericTypeCount = typeClass.generics.size();
				}
			}

			target.subCat = SubCategory.CAT_CLASS;

			target.arrSizeOrMemberCount = 0;
			target.subTypeOrMemberOffset = memberList.count;

			for(MemberHandle ref curr = typeStruct.members.head; curr; curr = curr.next)
			{
				if(curr.variable.name.name[0] == '$')
					continue;

				target.arrSizeOrMemberCount++;

				ExternMemberInfo ref member = memberList.push_back();

				member.type = curr.variable.type.typeIndex;
				member.offset = curr.variable.offset;

				debugSymbols.push_back(curr.variable.name.name.data, curr.variable.name.name.begin, curr.variable.name.name.length());
				debugSymbols.push_back(0);
			}

			target.constantCount = 0;
			target.constantOffset = constantList.count;

			for(ConstantData ref curr = typeStruct.constants.head; curr; curr = curr.next)
			{
				target.constantCount++;

				ExternConstantInfo ref constant = constantList.push_back();

				constant.type = curr.value.type.typeIndex;

				if(ExprBoolLiteral ref node = getType with<ExprBoolLiteral>(curr.value))
				{
					constant.value = node.value;
				}
				else if(ExprIntegerLiteral ref node = getType with<ExprIntegerLiteral>(curr.value))
				{
					constant.value = node.value;
				}
				else if(ExprRationalLiteral ref node = getType with<ExprRationalLiteral>(curr.value))
				{
					constant.value = memory::as_long(node.value);
				}

				debugSymbols.push_back(curr.name.name.data, curr.name.name.begin, curr.name.name.length());
				debugSymbols.push_back(0);
			}

			// Export type pointer members for GC
			target.pointerCount = 0;

			for(MemberHandle ref curr = typeStruct.members.head; curr; curr = curr.next)
			{
				if(curr.variable.type.hasPointers)
				{
					target.pointerCount++;

					ExternMemberInfo ref member = memberList.push_back();

					member.type = curr.variable.type.typeIndex;
					member.offset = curr.variable.offset;
				}
			}
		}
		else if(TypeGenericClassProto ref typeGenericClassProto = getType with<TypeGenericClassProto>(type))
		{
			if(ScopeData ref scope = ctx.exprCtx.NamespaceScopeFrom(typeGenericClassProto.scope))
				target.namespaceHash = scope.ownerNamespace.fullNameHash;

			target.type = TypeCategory.TYPE_COMPLEX;

			target.subCat = SubCategory.CAT_CLASS;

			if(ModuleData ref moduleData = typeGenericClassProto.importModule)
			{
				target.definitionOffsetStart = int(typeGenericClassProto.definition.begin.index);
				assert(target.definitionOffsetStart < moduleData.lexStream.size);
			}
			else
			{
				target.definitionOffsetStart = int(typeGenericClassProto.definition.begin.index);
				assert(target.definitionOffsetStart < ctx.parseCtx.lexer.lexems.size());
			}

			target.definitionOffset = -1;
		}
		else if(TypeGenericClass ref typeGenericClass = getType with<TypeGenericClass>(type))
		{
			target.type = TypeCategory.TYPE_COMPLEX;

			target.subCat = SubCategory.CAT_CLASS;

			target.definitionOffset = 0x80000000 | typeGenericClass.proto.typeIndex;

			target.genericTypeCount = typeGenericClass.generics.size();
		}
		else if(isType with<TypeVoid>(type))
		{
			target.type = TypeCategory.TYPE_VOID;

			target.subCat = SubCategory.CAT_NONE;
		}
		else if(isType with<TypeBool>(type))
		{
			target.type = TypeCategory.TYPE_CHAR;

			target.subCat = SubCategory.CAT_NONE;
		}
		else if(isType with<TypeChar>(type))
		{
			target.type = TypeCategory.TYPE_CHAR;

			target.subCat = SubCategory.CAT_NONE;
		}
		else if(isType with<TypeShort>(type))
		{
			target.type = TypeCategory.TYPE_SHORT;

			target.subCat = SubCategory.CAT_NONE;
		}
		else if(isType with<TypeInt>(type))
		{
			target.type = TypeCategory.TYPE_INT;

			target.subCat = SubCategory.CAT_NONE;
		}
		else if(isType with<TypeLong>(type))
		{
			target.type = TypeCategory.TYPE_LONG;

			target.subCat = SubCategory.CAT_NONE;
		}
		else if(isType with<TypeFloat>(type))
		{
			target.type = TypeCategory.TYPE_FLOAT;

			target.subCat = SubCategory.CAT_NONE;
		}
		else if(isType with<TypeDouble>(type))
		{
			target.type = TypeCategory.TYPE_DOUBLE;

			target.subCat = SubCategory.CAT_NONE;
		}
		else
		{
			target.subCat = SubCategory.CAT_NONE;
		}
	}

	code.modules.resize(ctx.exprCtx.imports.size());

	VectorView<ExternModuleInfo> mInfo = VectorView<ExternModuleInfo>(FindFirstModule(code), ctx.exprCtx.imports.size());

	for(int i = 0; i < ctx.exprCtx.imports.size(); i++)
	{
		ModuleData ref moduleData = ctx.exprCtx.imports[i];

		ExternModuleInfo ref moduleInfo = mInfo.push_back();

		moduleInfo.nameOffset = debugSymbols.count;
		debugSymbols.push_back(moduleData.name.data, moduleData.name.begin, moduleData.name.length());
		debugSymbols.push_back(0);

		moduleInfo.funcStart = moduleData.startingFunctionIndex;
		moduleInfo.funcCount = moduleData.moduleFunctionCount;

		moduleInfo.variableOffset = 0;
		moduleInfo.nameHash = -1;
	}

	code.variables.resize(externVariableInfoCount);

	VectorView<ExternVarInfo> vInfo = VectorView<ExternVarInfo>(FindFirstVar(code), externVariableInfoCount);

	// Handle visible global variables first
	for(int i = 0; i < ctx.exprCtx.variables.size(); i++)
	{
		VariableData ref variable = ctx.exprCtx.variables[i];

		if(variable.importModule != nullptr)
			continue;

		if(!ctx.exprCtx.GlobalScopeFrom(variable.scope))
			continue;

		if(variable.scope != ctx.exprCtx.globalScope && !variable.scope.ownerNamespace)
			continue;

		ExternVarInfo ref varInfo = vInfo.push_back();

		varInfo.offsetToName = debugSymbols.count;
		debugSymbols.push_back(variable.name.name.data, variable.name.name.begin, variable.name.name.length());
		debugSymbols.push_back(0);

		varInfo.nameHash = variable.nameHash;

		varInfo.type = variable.type.typeIndex;
		varInfo.offset = variable.offset;
	}

	for(int i = 0; i < ctx.exprCtx.variables.size(); i++)
	{
		VariableData ref variable = ctx.exprCtx.variables[i];

		if(variable.importModule != nullptr)
			continue;

		if(!ctx.exprCtx.GlobalScopeFrom(variable.scope))
			continue;

		if(variable.scope == ctx.exprCtx.globalScope || variable.scope.ownerNamespace)
			continue;

		ExternVarInfo ref varInfo = vInfo.push_back();

		varInfo.offsetToName = debugSymbols.count;
		debugSymbols.push_back(variable.name.name.data, variable.name.name.begin, variable.name.name.length());
		debugSymbols.push_back(0);

		varInfo.nameHash = variable.nameHash;

		varInfo.type = variable.type.typeIndex;
		varInfo.offset = variable.offset;
	}

	code.functions.resize(exportedFunctionCount);
	code.locals.resize(localCount);

	VectorView<ExternFuncInfo> fInfo = VectorView<ExternFuncInfo>(FindFirstFunc(code), exportedFunctionCount);
	VectorView<ExternLocalInfo> localInfo = VectorView<ExternLocalInfo>(FindFirstLocal(code), localCount);

	for(int i = 0; i < ctx.exprCtx.functions.size(); i++)
	{
		FunctionData ref function = ctx.exprCtx.functions[i];

		if(function.importModule != nullptr)
			continue;

		ExternFuncInfo ref funcInfo = fInfo.push_back();

		if(function.isPrototype && function.implementation)
		{
			//funcInfo.regVmAddress = function.implementation.vmFunction.regVmAddress;
			funcInfo.regVmCodeSize = function.implementation.functionIndex | 0x80000000;
			//funcInfo.regVmRegisters = function.implementation.vmFunction.regVmRegisters;
		}
		else if(function.isPrototype)
		{
			funcInfo.regVmAddress = 0;
			funcInfo.regVmCodeSize = 0;
			funcInfo.regVmRegisters = 0;
		}
		else if(function.vmFunction)
		{
			//funcInfo.regVmAddress = function.vmFunction.regVmAddress;
			//funcInfo.regVmCodeSize = function.vmFunction.regVmCodeSize;
			//funcInfo.regVmRegisters = function.vmFunction.regVmRegisters;
		}
		else
		{
			funcInfo.regVmAddress = -1;
			funcInfo.regVmCodeSize = 0;
			funcInfo.regVmRegisters = 0;
		}

		//funcInfo.funcPtrRaw = nullptr;
		//funcInfo.funcPtrWrapTarget = nullptr;
		//funcInfo.funcPtrWrap = nullptr;

		funcInfo.builtinIndex = 0;

		// Only functions in global or namesapce scope remain visible
		funcInfo.isVisible = (function.scope == ctx.exprCtx.globalScope || function.scope.ownerNamespace) && !function.isHidden;

		funcInfo.nameHash = function.nameHash;

		if(ModuleData ref moduleData = function.importModule)
			funcInfo.definitionModule = moduleData.dependencyIndex;
		else
			funcInfo.definitionModule = 0;

		funcInfo.definitionLocationStart = 0;
		funcInfo.definitionLocationEnd = 0;
		funcInfo.definitionLocationName = 0;

		Lexeme[] sourceStreamStart = function.importModule ? function.importModule.lexStream : ctx.parseCtx.lexer.lexems.data;

		if(function.source.begin.owner.lexems.data == sourceStreamStart)
		{
			funcInfo.definitionLocationStart = int(function.source.begin.index);
			funcInfo.definitionLocationEnd = int(function.source.end.index);

			if(function.name.begin)
				funcInfo.definitionLocationName = int(function.name.begin.index);
		}

		if(ScopeData ref scope = ctx.exprCtx.NamespaceScopeFrom(function.scope))
			funcInfo.namespaceHash = scope.ownerNamespace.fullNameHash;
		else
			funcInfo.namespaceHash = -1;

		funcInfo.funcCat = int(FunctionCategory.NORMAL);

		if(function.scope.ownerType)
			funcInfo.funcCat = int(FunctionCategory.THISCALL);
		else if(function.isCoroutine)
			funcInfo.funcCat = int(FunctionCategory.COROUTINE);
		else if(function.isHidden)
			funcInfo.funcCat = int(FunctionCategory.LOCAL);

		funcInfo.isGenericInstance = ctx.exprCtx.IsGenericInstance(function);

		funcInfo.isOperator = function.isOperator;

		funcInfo.funcType = function.type.typeIndex;

		if(function.scope.ownerType)
			funcInfo.parentType = function.scope.ownerType.typeIndex;
		else
			funcInfo.parentType = -1;

		funcInfo.contextType = function.contextType ? function.contextType.typeIndex : -1;

		funcInfo.offsetToFirstLocal = localInfo.count;
		funcInfo.paramCount = function.arguments.size();

		bool isGenericFunction = ctx.exprCtx.IsGenericFunction(function);

		if(!isGenericFunction)
		{
			TypeBase ref returnType = function.type.returnType;

			funcInfo.retType = int(ReturnType.RETURN_UNKNOWN);

/*			if(returnType == ctx.exprCtx.typeFloat || returnType == ctx.exprCtx.typeDouble)
				funcInfo.retType = ReturnType.RETURN_DOUBLE;
#if defined(__linux) && !defined(_M_X64)
			else if(returnType == ctx.exprCtx.typeVoid)
				funcInfo.retType = ReturnType.RETURN_VOID;
#else
			else if(returnType == ctx.exprCtx.typeVoid || returnType.size == 0)
				funcInfo.retType = ReturnType.RETURN_VOID;
#endif
#if defined(__linux)
			else if(returnType == ctx.exprCtx.typeBool || returnType == ctx.exprCtx.typeChar || returnType == ctx.exprCtx.typeShort || returnType == ctx.exprCtx.typeInt || (sizeof(void*) == 4 && isType with<TypeRef>(returnType)))
				funcInfo.retType = ReturnType.RETURN_INT;
			else if(returnType == ctx.exprCtx.typeLong || (sizeof(void*) == 8 && isType with<TypeRef>(returnType)))
				funcInfo.retType = ReturnType.RETURN_LONG;
#else
			else if(returnType == ctx.exprCtx.typeInt || returnType.size <= 4)
				funcInfo.retType = ReturnType.RETURN_INT;
			else if(returnType == ctx.exprCtx.typeLong || returnType.size == 8)
				funcInfo.retType = ReturnType.RETURN_LONG;
#endif*/

			funcInfo.returnShift = (returnType.size / 4);

			funcInfo.returnSize = funcInfo.returnShift * 4;

			if(funcInfo.retType == int(ReturnType.RETURN_VOID))
				funcInfo.returnSize = 0;
			else if(funcInfo.retType == int(ReturnType.RETURN_INT))
				funcInfo.returnSize = 4;
			else if(funcInfo.retType == int(ReturnType.RETURN_DOUBLE))
				funcInfo.returnSize = 8;
			else if(funcInfo.retType == int(ReturnType.RETURN_LONG))
				funcInfo.returnSize = 8;

			funcInfo.bytesToPop = function.argumentsSize;
			funcInfo.stackSize = function.stackSize;

			funcInfo.genericOffsetStart = -1;
			funcInfo.genericOffset = -1;
			funcInfo.genericReturnType = 0;
		}
		else
		{
			TypeBase ref returnType = function.type.returnType;

			funcInfo.regVmAddress = -1;

			funcInfo.retType = int(ReturnType.RETURN_VOID);

			if(ModuleData ref moduleData = function.importModule)
			{
				funcInfo.genericOffsetStart = int(function.declaration.source.begin.index);
				assert(funcInfo.genericOffsetStart < moduleData.lexStream.size);
			}
			else
			{
				funcInfo.genericOffsetStart = int(function.declaration.source.begin.index);
				assert(funcInfo.genericOffsetStart < ctx.parseCtx.lexer.lexems.size());
			}

			funcInfo.genericOffset = -1;
			funcInfo.genericReturnType = returnType.typeIndex;
		}

		VariableHandle ref argumentVariable = function.argumentVariables.head;

		for(int i = 0; i < function.arguments.size(); i++)
		{
			ArgumentData ref argument = &function.arguments[i];

			ExternLocalInfo ref local = localInfo.push_back();

			local.paramType = int(LocalType.PARAMETER);
			local.paramFlags = (argument.isExplicit ? int(LocalFlags.IS_EXPLICIT) : 0);

			if(argument.valueFunction)
				local.defaultFuncId = argument.valueFunction.functionIndex;
			else
				local.defaultFuncId = -1;

			local.type = argument.type.typeIndex;
			local.size = argument.type.size;
			local.offset = !isGenericFunction ? argumentVariable.variable.offset : 0;
			local.closeListID = 0;

			local.offsetToName = debugSymbols.count;
			debugSymbols.push_back(argument.name.name.data, argument.name.name.begin, argument.name.name.length());
			debugSymbols.push_back(0);

			if (!isGenericFunction)
				argumentVariable = argumentVariable.next;
		}

		if(VariableData ref variable = function.contextArgument)
		{
			ExternLocalInfo ref local = localInfo.push_back();

			local.paramType = int(LocalType.PARAMETER);
			local.paramFlags = 0;
			local.defaultFuncId = -1;

			local.type = variable.type.typeIndex;
			local.size = variable.type.size;
			local.offset = !isGenericFunction ? variable.offset : 0;
			local.closeListID = 0;

			local.offsetToName = debugSymbols.count;
			debugSymbols.push_back(variable.name.name.data, variable.name.name.begin, variable.name.name.length());
			debugSymbols.push_back(0);
		}

		if (!isGenericFunction)
		{
			for(int i = 0; i < function.functionScope.allVariables.size(); i++)
			{
				VariableData ref variable = function.functionScope.allVariables[i];

				VariableHandle ref argumentVariable = nullptr;

				for(VariableHandle ref curr = function.argumentVariables.head; curr; curr = curr.next)
				{
					if(curr.variable == variable)
					{
						argumentVariable = curr;
						break;
					}
				}

				if(argumentVariable || variable == function.contextArgument)
					continue;

				if(variable.isAlloca && variable.users.empty())
					continue;

				if(variable.lookupOnly)
					continue;

				ExternLocalInfo ref local = localInfo.push_back();

				local.paramType = int(LocalType.LOCAL);
				local.paramFlags = 0;
				local.defaultFuncId = -1;

				local.type = variable.type.typeIndex;
				local.size = variable.type.size;
				local.offset = variable.offset;
				local.closeListID = 0;

				local.offsetToName = debugSymbols.count;
				debugSymbols.push_back(variable.name.name.data, variable.name.name.begin, variable.name.name.length());
				debugSymbols.push_back(0);
			}
		}

		funcInfo.localCount = localInfo.count - funcInfo.offsetToFirstLocal;

		for(int i = 0; i < function.upvalues.size(); i++)
		{
			UpvalueData ref upvalue = &function.upvalues[i];

			ExternLocalInfo ref local = localInfo.push_back();

			local.paramType = int(LocalType.EXTERNAL);
			local.type = upvalue.variable.type.typeIndex;
			local.size = upvalue.variable.type.size;
			local.offset = upvalue.target.offset;

			local.offsetToName = debugSymbols.count;
			debugSymbols.push_back(upvalue.variable.name.name.data, upvalue.variable.name.name.begin, upvalue.variable.name.name.length());
			debugSymbols.push_back(0);
		}

		funcInfo.externCount = function.upvalues.size();

		funcInfo.offsetToName = debugSymbols.count;
		debugSymbols.push_back(function.name.name.data, function.name.name.begin, function.name.name.length());
		debugSymbols.push_back(0);

		for(MatchData ref curr = function.generics.head; curr; curr = curr.next)
		{
			funcInfo.explicitTypeCount++;

			ExternVarInfo ref varInfo = vInfo.push_back();

			varInfo.offsetToName = debugSymbols.count;
			debugSymbols.push_back(curr.name.name.data, curr.name.name.begin, curr.name.name.length());
			debugSymbols.push_back(0);

			varInfo.nameHash = curr.name.name.hash();

			varInfo.type = curr.type.typeIndex;
			varInfo.offset = 0;
		}
	}

	code.regVmOffsetToInfo = offsetToRegVmInfo;
	code.regVmInfoSize = regVmInfoCount;

	code.regVmSourceInfo.resize(regVmInfoCount);

	VectorView<ExternSourceInfo> regVmInfoArray = VectorView<ExternSourceInfo>(FindRegVmSourceInfo(code), regVmInfoCount);

	/*for(int i = 0; i < ctx.instRegVmFinalizeCtx.locations.size(); i++)
	{
		SynBase ref location = &ctx.instRegVmFinalizeCtx.locations[i];

		if(location && !location.isInternal)
		{
			ExternSourceInfo info;

			info.instruction = i;

			if(ModuleData ref importModule = ctx.exprCtx.GetSourceOwner(location.begin))
			{
				char[] code = FindSource(importModule.bytecode);

				assert(location.pos.begin >= code && location.pos.begin < code + importModule.bytecode.sourceSize);

				info.definitionModule = importModule.dependencyIndex;
				info.sourceOffset = int(location.begin.pos - code);

				regVmInfoArray.push_back(info);
			}
			else
			{
				char[] code = ctx.code;

				assert(location.pos.begin >= code && location.pos.begin < code + sourceLength);

				info.definitionModule = 0;
				info.sourceOffset = int(location.begin.pos - code);

				regVmInfoArray.push_back(info);
			}
		}
	}*/

	/*code.regVmGlobalCodeStart = ctx.vmModule.regVmGlobalCodeStart;

	if(ctx.instRegVmFinalizeCtx.cmds.size())
		memcpy(FindRegVmCode(code), ctx.instRegVmFinalizeCtx.cmds.data, ctx.instRegVmFinalizeCtx.cmds.size() * sizeof(RegVmCmd));

	if(ctx.instRegVmFinalizeCtx.constants.size())
		memcpy(FindRegVmConstants(code), ctx.instRegVmFinalizeCtx.constants.data, ctx.instRegVmFinalizeCtx.constants.size() * sizeof(ctx.instRegVmFinalizeCtx.constants[0]));

	if(ctx.instRegVmFinalizeCtx.regKillInfo.size())
		memcpy(FindRegVmRegKillInfo(code), ctx.instRegVmFinalizeCtx.regKillInfo.data, ctx.instRegVmFinalizeCtx.regKillInfo.size() * sizeof(ctx.instRegVmFinalizeCtx.regKillInfo[0]));*/

	code.source = ctx.code;

	code.offsetToSource = offsetToSource;
	code.sourceSize = sourceLength;

	//code.llvmSize = ctx.llvmModule ? ctx.llvmModule.moduleSize : 0;
	code.llvmOffset = offsetToLLVM;

	/*if(ctx.llvmModule)
		memcpy(((char*)(code) + code.llvmOffset), ctx.llvmModule.moduleData, ctx.llvmModule.moduleSize);*/

	code.typedefCount = typedefCount;
	code.offsetToTypedef = offsetToTypedef;

	code.typedefs.resize(typedefCount);

	ExternTypedefInfo[] currAliasList = FindFirstTypedef(code);
	int cullAliasPos = 0;

	for(int i = 0; i < ctx.exprCtx.globalScope.aliases.size(); i++)
	{
		AliasData ref alias = ctx.exprCtx.globalScope.aliases[i];

		auto currAlias = &currAliasList[cullAliasPos];

		currAlias.offsetToName = debugSymbols.count;
		debugSymbols.push_back(alias.name.name.data, alias.name.name.begin, alias.name.name.length());
		debugSymbols.push_back(0);

		currAlias.targetType = alias.type.typeIndex;
		currAlias.parentType = -1;

		cullAliasPos++;
	}

	for(int i = 0; i < ctx.exprCtx.types.size(); i++)
	{
		TypeBase ref type = ctx.exprCtx.types[i];

		if(TypeClass ref typeClass = getType with<TypeClass>(type))
		{
			for(MatchData ref curr = typeClass.generics.head; curr; curr = curr.next)
			{
				auto currAlias = &currAliasList[cullAliasPos];
				
				currAlias.offsetToName = debugSymbols.count;
				debugSymbols.push_back(curr.name.name.data, curr.name.name.begin, curr.name.name.length());
				debugSymbols.push_back(0);

				currAlias.targetType = curr.type.typeIndex;
				currAlias.parentType = type.typeIndex;
				cullAliasPos++;
			}

			for(MatchData ref curr = typeClass.aliases.head; curr; curr = curr.next)
			{
				auto currAlias = &currAliasList[cullAliasPos];

				currAlias.offsetToName = debugSymbols.count;
				debugSymbols.push_back(curr.name.name.data, curr.name.name.begin, curr.name.name.length());
				debugSymbols.push_back(0);

				currAlias.targetType = curr.type.typeIndex;
				currAlias.parentType = type.typeIndex;
				cullAliasPos++;
			}
		}
		else if(TypeGenericClass ref typeGenericClass = getType with<TypeGenericClass>(type))
		{
			for(TypeHandle ref curr = typeGenericClass.generics.head; curr; curr = curr.next)
			{
				auto currAlias = &currAliasList[cullAliasPos];

				currAlias.offsetToName = debugSymbols.count;
				debugSymbols.push_back(0); // No name

				currAlias.targetType = curr.type.typeIndex;
				currAlias.parentType = type.typeIndex;
				cullAliasPos++;
			}
		}
	}

	code.namespaces.resize(ctx.exprCtx.namespaces.size());

	VectorView<ExternNamespaceInfo> namespaceList = VectorView<ExternNamespaceInfo>(FindFirstNamespace(code), ctx.exprCtx.namespaces.size());

	for(int i = 0; i < ctx.exprCtx.namespaces.size(); i++)
	{
		NamespaceData ref nameSpace = ctx.exprCtx.namespaces[i];

		ExternNamespaceInfo ref namespaceInfo = namespaceList.push_back();

		namespaceInfo.offsetToName = debugSymbols.count;
		debugSymbols.push_back(nameSpace.name.name.data, nameSpace.name.name.begin, nameSpace.name.name.length());
		debugSymbols.push_back(0);

		namespaceInfo.parentHash = nameSpace.parent ? nameSpace.parent.fullNameHash : -1;
	}

	assert(debugSymbols.count == symbolStorageSize);
	assert(tInfo.count == code.typeCount);
	assert(memberList.count == allMemberCount);
	assert(constantList.count == allConstantCount);
	assert(mInfo.count == ctx.exprCtx.imports.size());
	assert(vInfo.count == externVariableInfoCount);
	assert(fInfo.count == exportedFunctionCount);
	assert(localInfo.count == localCount);
	assert(regVmInfoArray.count == regVmInfoCount);
	assert(namespaceList.count == ctx.exprCtx.namespaces.size());

	return code;
}

bool SaveListing(CompilerContext ref ctx, string fileName)
{
	//TRACE_SCOPE("compiler", "SaveListing");

	/*assert(!ctx.outputCtx.stream);
	ctx.outputCtx.stream = ctx.outputCtx.openStream(fileName);

	if(!ctx.outputCtx.stream)
	{
		NULLC.SafeSprintf(ctx.errorBuf, ctx.errorBufSize, "ERROR: failed to open file");
		return false;
	}

	InstructionRegVmLowerGraphContext instLowerGraphCtx = InstructionRegVmLowerGraphContext(ctx.outputCtx);

	instLowerGraphCtx.showSource = true;

	PrintGraph(instLowerGraphCtx, ctx.regVmLoweredModule);

	ctx.outputCtx.closeStream(ctx.outputCtx.stream);
	ctx.outputCtx.stream = nullptr;*/

	return true;
}

bool TranslateToC(CompilerContext ref ctx, string fileName, string mainName, void ref(string) addDependency)
{
	//TRACE_SCOPE("compiler", "TranslateToC");

	/*assert(!ctx.outputCtx.stream);
	ctx.outputCtx.stream = ctx.outputCtx.openStream(fileName);

	if(!ctx.outputCtx.stream)
	{
		NULLC.SafeSprintf(ctx.errorBuf, ctx.errorBufSize, "ERROR: failed to open file");
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
		ctx.outputCtx.stream = nullptr;

		return false;
	}

	ctx.outputCtx.closeStream(ctx.outputCtx.stream);
	ctx.outputCtx.stream = nullptr;

	if(addDependency)
	{
		for(int i = 0; i < dependencies.size(); i++)
			addDependency(dependencies[i]);
	}*/

	return true;
}

ByteCode ref BuildModuleFromSource(string modulePath, string moduleRoot, string code, StringRef ref errorPos, string ref errorBuf, int optimizationLevel, ArrayView<InplaceStr> activeImports)
{
	//TRACE_SCOPE("compiler", "BuildModuleFromSource");
	//TRACE_LABEL(modulePath);

	CompilerContext ctx = CompilerContext(optimizationLevel, activeImports);

	ctx.errorBuf = errorBuf;

	ctx.code = code.data;
	ctx.moduleRoot = moduleRoot;

	ctx.enableLogFiles = true;

	if(!CompileModuleFromSource(ctx))
	{
		*errorPos = ctx.errorPos;

		/*if(errorBuf && errorBufSize)
		{
			int currLen = (int)strlen(errorBuf);
			NULLC.SafeSprintf(errorBuf + currLen, errorBufSize - currLen, " [in module '%s']", modulePath);
		}*/

		return nullptr;
	}

	ByteCode ref bytecode = GetBytecode(ctx);

	Lexer ref lexer = &ctx.parseCtx.lexer;

	auto newStart = FindSource(bytecode);

	// We have to fix lexeme positions to the code that is saved in bytecode
	/*for(Lexeme ref c = lexer.GetStreamStart(), *e = lexer.GetStreamStart() + lexer.GetStreamSize(); c != e; c++)
	{
		// Exit fix up if lexemes exited scope of the current file
		if(c.pos < code || c.pos > (code + codeSize))
			break;

		c.pos = newStart + (c.pos - code);
	}*/

	BinaryCache::PutBytecode(modulePath, bytecode, lexer.lexems.data);

	return bytecode;
}

char[] FindFileContentInImportPaths(string moduleName, string moduleRoot, bool addExtension, string ref resultPathBuf, int ref fileSize)
{
	char[] fileContent = nullptr;

	int modulePathPos = 0;
	string ref modulePath = BinaryCache::EnumImportPath(modulePathPos++);
	while(modulePath)
	{
		string path = modulePath + moduleRoot + (moduleRoot.empty() ? "" : "/") + moduleName;

		if(addExtension)
		{
			for(int i = modulePath.length(), e = path.length(); i != e; i++)
			{
				if(path[i] == '.')
					path[i] = '/';
			}

			path += ".nc";
		}

		//fileContent = NULLC.fileLoad(resultPathBuf, &fileSize);

		if(fileContent)
			break;

		modulePath = BinaryCache::EnumImportPath(modulePathPos++);
	}

	return fileContent;
}

ByteCode ref BuildModuleFromPath(string moduleName, string moduleRoot, bool addExtension, StringRef ref errorPos, string ref errorBuf, int optimizationLevel, ArrayView<InplaceStr> activeImports)
{
	//NULLC.TraceDump();

	//TRACE_SCOPE("compiler", "BuildModuleFromPath");
	//TRACE_LABEL2(moduleName.begin, moduleName.end);

	//assert(*moduleName.end == 0);

	if(addExtension)
		assert(moduleName.find(".nc") == -1);
	else
		assert(moduleName.find(".nc") != -1);

	string nextModuleRoot = moduleRoot;

	string path;

	int fileSize = 0;
	char[] fileContent = nullptr;

	if(!moduleRoot.empty())
	{
		fileContent = FindFileContentInImportPaths(moduleName, moduleRoot, addExtension, &path, fileSize);

		if(fileContent)
		{
			int pos = moduleName.rfind('/');

			if(pos != -1)
			{
				string nextModuleRootBuf = moduleRoot + "/" + moduleName.substr(0, pos);

				nextModuleRoot = nextModuleRootBuf;
			}
		}
	}

	if(fileContent == nullptr)
	{
		fileContent = FindFileContentInImportPaths(moduleName, string(), addExtension, &path, fileSize);

		if(fileContent)
		{
			int pos = moduleName.rfind('/');

			if(pos != -1)
			{
				string nextModuleRootBuf = moduleName.substr(0, pos);

				nextModuleRoot = nextModuleRootBuf;
			}
		}
	}

	if(fileContent == nullptr)
	{
		*errorPos = StringRef();

		//if(errorBuf && errorBufSize)
		//	NULLC.SafeSprintf(errorBuf, errorBufSize, "ERROR: module file '%.*s' could not be opened", moduleName.length(), moduleName.begin);

		return nullptr;
	}

	ByteCode ref bytecode = BuildModuleFromSource(path, nextModuleRoot, string(fileContent), errorPos, errorBuf, optimizationLevel, activeImports);

	//NULLC.fileFree(fileContent);

	return bytecode;
}
/*
bool AddModuleFunction(const char* module, void (*ptrRaw)(), void *funcWrap, void (*ptrWrap)(void *func, char* retBuf, char* argBuf), const char* name, int index, const char **errorPos, char *errorBuf, int errorBufSize, int optimizationLevel)
{
	const char *bytecode = BinaryCache::FindBytecode(module, true);

	// Create module if not found
	if(!bytecode)
		bytecode = BuildModuleFromPath(allocator, InplaceStr(module), nullptr, true, errorPos, errorBuf, errorBufSize, optimizationLevel, ArrayView<InplaceStr>());

	if(!bytecode)
		return false;

	int hash = NULLC.GetStringHash(name);
	ByteCode ref code = (ByteCode*)bytecode;

	// Find function and set pointer
	ExternFuncInfo ref fInfo = FindFirstFunc(code);

	int end = code.functionCount - code.moduleFunctionCount;
	for(int i = 0; i < end; i++)
	{
		if(hash == fInfo.nameHash)
		{
			if(index == 0)
			{
				fInfo.regVmAddress = -1;

				fInfo.funcPtrRaw = ptrRaw;
				fInfo.funcPtrWrapTarget = funcWrap;
				fInfo.funcPtrWrap = ptrWrap;

				index--;
				break;
			}

			index--;
		}

		fInfo++;
	}

	if(index != -1)
	{
		*errorPos = nullptr;

		NULLC.SafeSprintf(errorBuf, errorBufSize, "ERROR: function '%s' or one of it's overload is not found in module '%s'", name, module);

		return false;
	}

	return true;
}
*/