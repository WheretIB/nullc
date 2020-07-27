import std.vector;

enum TypeCategory
{
    TYPE_COMPLEX, TYPE_VOID, TYPE_INT, TYPE_FLOAT, TYPE_LONG, TYPE_DOUBLE, TYPE_SHORT, TYPE_CHAR
}

enum SubCategory
{
    CAT_NONE, CAT_ARRAY, CAT_POINTER, CAT_FUNCTION, CAT_CLASS
}

enum TypeFlags
{
    TYPE_HAS_FINALIZER = 1 << 0,
    TYPE_DEPENDS_ON_GENERIC = 1 << 1,
    TYPE_IS_EXTENDABLE = 1 << 2,
    TYPE_IS_INTERNAL = 1 << 3,
    TYPE_IS_COMPLETED = 1 << 4
}

class ExternTypeInfo
{
	int	offsetToName;

	int	size;	// sizeof(type)
	int	padding;
	TypeCategory type;
	SubCategory subCat;

	char defaultAlign;
	char typeFlags; // TypeFlags
	short pointerCount;

	int	arrSizeOrMemberCount;

	int	constantCount;
	int	constantOffset;

	int	subTypeOrMemberOffset;

	int	nameHash;

	int	definitionModule; // Index of the module containing the definition

	int	definitionLocationStart;
	int	definitionLocationEnd;
	int	definitionLocationName;

	// For generic types
	int	definitionOffsetStart; // Offset in a lexeme stream to the point of class definition start
	int	definitionOffset; // Offset in a lexeme stream to the point of type argument list
	int	genericTypeCount;

	int	namespaceHash;
	int	baseType;
}

class ExternVarInfo
{
	int	offsetToName;
	int	nameHash;

	int	type;	// index in type array
	int	offset;
}

enum LocalType
{
    PARAMETER,
    LOCAL,
    EXTERNAL
}

enum LocalFlags
{
    IS_EXPLICIT = 1 << 0
}

class ExternLocalInfo
{
	char paramType;
	char paramFlags;
	short defaultFuncId;

	int	type, size;
	int	offset;
	int	closeListID;
	char alignmentLog2; // 1 << value

	int	offsetToName;
}

enum ReturnType
{
    RETURN_UNKNOWN,
    RETURN_VOID,
    RETURN_INT,
    RETURN_DOUBLE,
    RETURN_LONG
}

enum FunctionCategory
{
    NORMAL,
    LOCAL,
    THISCALL,
    COROUTINE
}

// Object copy storage is splaced after the upvalue
/*class Upvalue
{
    int	*ptr;
    Upvalue			*next;
}*/

class ExternFuncInfo
{
	int	offsetToName;

	int regVmAddress;
	int regVmCodeSize;
	int regVmRegisters;

	int builtinIndex;

	int isVisible;

	/*void (*funcPtrRaw)();

	void *funcPtrWrapTarget;

	void (*funcPtrWrap)(void*, char*, char*);*/

	char retType;	// one of the ReturnType enumeration values
	
	char funcCat;
	char isGenericInstance;
	char isOperator;
	char returnShift; // Amount of dwords to remove for result type after raw external function call
	int	returnSize; // Return size of the wrapped external function call
	int	funcType;	// index to the type array

	int	startInByteCode;
	int	parentType; // Type inside which the function is defined
	int	contextType; // Type of the function context

	int	offsetToFirstLocal;
	int	paramCount;
	int	localCount;
	int	externCount;

	int	namespaceHash;

	int	bytesToPop; // Arguments size
	int	stackSize; // Including arguments

	// For generic functions
	int	genericOffsetStart; // Position in the lexeme stream of the definition
	int	genericOffset;
	int	genericReturnType;

	// Size of the explicit type list for generic functions and generic function instances
	int	explicitTypeCount;

	int	nameHash;

	int	definitionModule; // Index of the module containing the definition

	int	definitionLocationModule;
	int	definitionLocationStart;
	int	definitionLocationEnd;
	int	definitionLocationName;
}

class ExternTypedefInfo
{
	int offsetToName;
	int targetType;
	int parentType;
}

class ExternModuleInfo
{
	int	nameHash;
	int	nameOffset;

	int	funcStart;
	int	funcCount;

	int	variableOffset;

	int	sourceOffset;
	int	sourceSize;

	int	dependencyStart;
	int	dependencyCount;
}

class ExternMemberInfo
{
	int	type;
	int	offset;
}

class ExternConstantInfo
{
	int	type;
	long value;
}

class ExternNamespaceInfo
{
	int	offsetToName;
	int	parentHash;
}

class ExternSourceInfo
{
	int	instruction;
	int	definitionModule; // Index of the module containing the definition
	int	sourceOffset;
}

class ByteCode
{
	int	size;	// Overall size

	int	typeCount;

	int	dependsCount;
	int	offsetToFirstModule;
	
	int	globalVarSize;	// size of all global variables, in bytes
	int	variableCount;	// variable info count
	int	variableExportCount;	// exported variable count
	int	offsetToFirstVar;

	int	functionCount;
	int	moduleFunctionCount;
	int	offsetToFirstFunc;	// Offset from the beginning of a structure to the first ExternFuncInfo data

	int	localCount;
	int	offsetToLocals;

	int	closureListCount;

	int	regVmInfoSize;
	int	regVmOffsetToInfo;

	int	regVmCodeSize;
	int	regVmOffsetToCode;
	int	regVmGlobalCodeStart;

	int	regVmConstantCount;
	int	regVmOffsetToConstants;

	int	regVmRegKillInfoCount;
	int	regVmOffsetToRegKillInfo;

	int	symbolLength;
	int	offsetToSymbols;

	int	sourceSize;
	int	offsetToSource;

	int	llvmSize;
	int	llvmOffset;

	int	typedefCount;
	int	offsetToTypedef;

	int	constantCount;
	int	offsetToConstants;

	int	namespaceCount;
	int	offsetToNamespaces;

	vector<ExternTypeInfo> types;
	vector<ExternMemberInfo> typeMembers;
	vector<ExternConstantInfo> typeConstants;
	vector<ExternModuleInfo> modules;
	vector<ExternVarInfo> variables;
	vector<ExternFuncInfo> functions;
	vector<ExternLocalInfo> locals;
	vector<ExternTypedefInfo> typedefs;
	vector<ExternNamespaceInfo> namespaces;
	vector<char> regVmCode;
	vector<ExternSourceInfo> regVmSourceInfo;
	vector<char> debugSymbols;
	char[] source;
	vector<char> llvmCode;
	vector<int> regVmConstants;
	vector<char> regVmKillInfo;
}

ExternTypeInfo[] FindFirstType(ByteCode ref code)
{
	return code.types.data;
}

ExternMemberInfo[] FindFirstMember(ByteCode ref code)
{
	return code.typeMembers.data;
}

ExternConstantInfo[] FindFirstConstant(ByteCode ref code)
{
	return code.typeConstants.data;
}

ExternModuleInfo[] FindFirstModule(ByteCode ref code)
{
	return code.modules.data;
}

ExternVarInfo[] FindFirstVar(ByteCode ref code)
{
	return code.variables.data;
}

ExternFuncInfo[] FindFirstFunc(ByteCode ref code)
{
	return code.functions.data;
}

ExternLocalInfo[] FindFirstLocal(ByteCode ref code)
{
	return code.locals.data;
}

ExternTypedefInfo[] FindFirstTypedef(ByteCode ref code)
{
	return code.typedefs.data;
}

ExternNamespaceInfo[] FindFirstNamespace(ByteCode ref code)
{
	return code.namespaces.data;
}

char[] FindRegVmCode(ByteCode ref code)
{
	return code.regVmCode.data;
}

ExternSourceInfo[] FindRegVmSourceInfo(ByteCode ref code)
{
	return code.regVmSourceInfo.data;
}

char[] FindSymbols(ByteCode ref code)
{
	return code.debugSymbols.data;
}

char[] FindSource(ByteCode ref code)
{
	return code.source;
}

int[] FindRegVmConstants(ByteCode ref code)
{
	return code.regVmConstants.data;
}

char[] FindRegVmRegKillInfo(ByteCode ref code)
{
	return code.regVmKillInfo.data;
}
