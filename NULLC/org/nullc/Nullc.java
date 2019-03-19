package org.nullc;

public class Nullc
{
	public class NULLCArray
	{
		long ptr;
		int length;
	}
	public class NULLCRef
	{
		int typeID;
		long ptr;
	}
	public class NULLCFuncPtr
	{
		long context;
		int id;
	}
	public class NULLCAutoArray
	{
		int typeID;
		long ptr;
		int length;
	}
	
	public interface FileReadHandler
	{
		byte[] Read(String name);
	}
	
	/*	When java Object is passed to and returned from null, the supported types are:
	 *		Boolean for bool,
	 *		Character for char
	 *		Short for short
	 *		Integer for int
	 *		Long for long
	 *		NULLCArray for type[]
	 * 		NULLCRef for auto ref
	 * 		NULLCFuncPtr for function pointers
	 * 		NULLCAutoArray for auto[] */
	
	public static native boolean	nullcInit();

    public static native void	 nullcClearImportPaths();
	public static native void	nullcAddImportPath(String importPath);

	public static native void	nullcSetFileReadHandler(FileReadHandler handler);
    public static native void	nullcSetGlobalMemoryLimit(int limit);
    public static native void	nullcSetEnableLogFiles(int enable);
    public static native void	nullcSetOptimizationLevel(int level);

	public static native void	nullcTerminate();

	/************************************************************************/
	/*				NULLC execution settings and environment				*/

	/*	Change current executor to either NULLC_VM or NULLC_X86	*/
	enum ExecutorType
	{
		NULLC_VM,
		NULLC_X86
	}
	public static native void		nullcSetExecutor(ExecutorType id);

	/*	Used to bind unresolved module functions to external C functions. Function index is the number of a function overload	*/
	//public static native boolean	nullcBindModuleFunction(String module, void (NCDECL *ptr)(), String name, int index);

	/*	Builds module and saves its binary into binary cache	*/
	public static native boolean	nullcLoadModuleBySource(String module, String code);

	/*	Loads module into binary cache	*/
	public static native boolean	nullcLoadModuleByBinary(String module, byte[] binary);

	/* Removes module from binary cache	*/
	public static native void		nullcRemoveModule(String module);

	/*	Returns name of a module at index 'id'. Null pointer is returned if a module at index 'id' doesn't exist.
		To get all module names, start with 'id' = 0 and go up until null pointer is returned	*/
	public static native String		nullcEnumerateModules(int id);

	/************************************************************************/
	/*							Basic functions								*/

	/*	Compiles and links code	*/
	public static native boolean		nullcBuild(String code);

	/*	Run global code	*/
	public static native boolean		nullcRun();
	/*	Run function code */
	static native boolean				nullcRunFunction(String funcName, Object[] arguments);

	/*	Retrieve result	*/
	public static native String			nullcGetResult();
	public static native int			nullcGetResultInt();
	public static native double			nullcGetResultDouble();
	public static native long			nullcGetResultLong();

	/*	Returns last error description	*/
	public static native String			nullcGetLastError();

	public static native boolean		nullcFinalize();

	/************************************************************************/
	/*							Interaction functions						*/

	// A list of indexes for NULLC build-in types
	public static int	NULLC_TYPE_VOID =		0;
	public static int	NULLC_TYPE_BOOL =		1;
	public static int	NULLC_TYPE_CHAR =		2;
	public static int	NULLC_TYPE_SHORT =		3;
	public static int	NULLC_TYPE_INT =		4;
	public static int	NULLC_TYPE_LONG =		5;
	public static int	NULLC_TYPE_FLOAT =		6;
	public static int	NULLC_TYPE_DOUBLE =		7;
	public static int	NULLC_TYPE_TYPEID =		8;
	public static int	NULLC_TYPE_FUNCTION =	9;
	public static int	NULLC_TYPE_NULLPTR =	10;
	public static int	NULLC_TYPE_GENERIC =	11;
	public static int	NULLC_TYPE_AUTO =		12;
	public static int	NULLC_TYPE_AUTO_REF =	13;
	public static int	NULLC_TYPE_VOID_REF =	14;
	public static int	NULLC_TYPE_AUTO_ARRAY =	15;

	/*	Allocates memory block that is managed by GC	*/
	public static native long		nullcAllocate(int size);
	public static native long		nullcAllocateTyped(int typeID);
	public static native NULLCArray	nullcAllocateArrayTyped(int typeID, int count);

	/*	Abort NULLC program execution with specified error code	*/
	public static native void			nullcThrowError(String error);

	/*	Call function using NULLCFuncPtr */
	public static native boolean		nullcCallFunction(NULLCFuncPtr ptr, Object[] arguments);

	/*	Get global variable value	*/
	public static native Object			nullcGetGlobal(String name);

	/*	Set global variable value	*/
	public static native boolean		nullcSetGlobal(String name, Object data);

	/*	Get function pointer	*/
	public static native boolean		nullcGetFunction(String name, NULLCFuncPtr func);

	/*	Set function using function pointer	*/
	public static native boolean		nullcSetFunction(String name, NULLCFuncPtr func);

	/*	Function returns 1 if passed pointer points to NULLC stack; otherwise, the return value is 0	*/
	public static native boolean		nullcIsStackPointer(long ptr);

	/*	Function returns 1 if passed pointer points to a memory managed by NULLC GC; otherwise, the return value is 0	*/
	public static native boolean		nullcIsManagedPointer(long ptr);

	/************************************************************************/
	/*							Special modules								*/

	public static native int			nullcInitTypeinfoModule();
	public static native int			nullcInitDynamicModule();

	/************************************************************************/
	/*							Extended functions							*/

    /*	Analyzes the code and returns 1 on success	*/
    public static native boolean		nullcAnalyze(String code);

	/*	Compiles the code and returns 1 on success	*/
	public static native boolean		nullcCompile(String code);

	/*	compiled bytecode to be used for linking and executing can be retrieved with this function
		function returns bytecode size, and memory to which 'bytecode' points can be freed at any time	*/
	public static native byte[]			nullcGetBytecode();
	public static native byte[]			nullcGetBytecodeNoCache();

	/*	This function saves disassembly of last compiled code into file	*/
	public static native boolean		nullcSaveListing(String fileName);

	/*	This function saved analog of C++ code of last compiled code into file	*/
	public static native void			nullcTranslateToC(String fileName, String mainName);

	/*	Clean all accumulated bytecode	*/
	public static native void			nullcClean();

	/*	Link new chunk of code.
		Type or function redefinition generates an error.
		Global variables with the same name are ok. */
	public static native boolean		nullcLinkCode(byte[] arr);
	
	/*	Run tests if they are build	*/
	public static native void			nullcRunTests(boolean verbose);
	
	static
    {
    	System.loadLibrary("nullc");
    }
}
