import arrayview;
import stringutil;
import parser;
import expressioncontext;
import std.string;

class CompilerContext
{
	void CompilerContext(int optimizationLevel, ArrayView<InplaceStr> activeImports)
	{
        parseCtx = ParseContext(optimizationLevel, activeImports);
        
        exprCtx = ExpressionContext(optimizationLevel);
        
        //instRegVmFinalizeCtx = (exprCtx);

        this.optimizationLevel = optimizationLevel;
	}

	char[] code;

	string moduleRoot;

	StringRef errorPos;
	string ref errorBuf;

	//OutputContext outputCtx;

	ParseContext parseCtx;
	SynModule ref synModule;

	ExpressionContext exprCtx;
	ExprModule ref exprModule;
	int exprMemoryLimit;

	//VmModule ref vmModule;

	//LlvmModule ref llvmModule;

	//RegVmLoweredModule ref regVmLoweredModule;

	//InstructionRegVmFinalizeContext instRegVmFinalizeCtx;

	bool enableLogFiles;

	int optimizationLevel;
}
