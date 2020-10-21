using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;

namespace nullc_debugger_component
{
    namespace DkmDebugger
    {
        enum NullcTypeIndex
        {
            Void,
            Bool,
            Char,
            Short,
            Int,
            Long,
            Float,
            Double,
            TypeId,
            Function,
            Nullptr,
            Generic,
            Auto,
            AutoRef,
            VoidRef,
            AutoArray,
        }

        enum NullcTypeCategory
        {
            Complex,
            Void,
            Int,
            Float,
            Long,
            Double,
            Short,
            Char
        };

        enum NullcTypeSubCategory
        {
            None,
            Array,
            Pointer,
            Function,
            Class
        };

        enum NullcTypeFlags
        {
            HasFinalizer = 1 << 0,
            DependsOnGeneric = 1 << 1,
            IsExtendable = 1 << 2,
            Internal = 1 << 3
        }

        class NullcTypeInfo
        {
            public string name;

            public NullcTypeInfo nullcSubType;
            public List<NullcMemberInfo> nullcMembers = new List<NullcMemberInfo>();
            public List<NullcConstantInfo> nullcConstants = new List<NullcConstantInfo>();
            public NullcTypeInfo nullcBaseType;

            public NullcTypeIndex index;

            public uint offsetToName;

            public uint size;
            public uint padding;
            public NullcTypeCategory typeCategory;
            public NullcTypeSubCategory subCat;
            public byte defaultAlign;
            public NullcTypeFlags typeFlags;
            public ushort pointerCount;

            public int arrSize;
            public int memberCount;

            public int constantCount;
            public int constantOffset;

            public int subType;
            public int memberOffset;
            public uint nameHash;

            public uint definitionModule; // Index of the module containing the definition
            public uint definitionLocationStart;
            public uint definitionLocationEnd;
            public uint definitionLocationName;

            // For generic types
            public uint definitionOffsetStart; // Offset in a lexeme stream to the point of class definition start
            public uint definitionOffset; // Offset in a lexeme stream to the point of type argument list
            public uint genericTypeCount;

            public uint namespaceHash;
            public int baseType;

            public void ReadFrom(BinaryReader reader)
            {
                offsetToName = reader.ReadUInt32();

                size = reader.ReadUInt32();
                padding = reader.ReadUInt32();
                typeCategory = (NullcTypeCategory)reader.ReadUInt32();
                subCat = (NullcTypeSubCategory)reader.ReadUInt32();
                defaultAlign = reader.ReadByte();
                typeFlags = (NullcTypeFlags)reader.ReadByte();
                pointerCount = reader.ReadUInt16();

                arrSize = reader.ReadInt32();
                memberCount = arrSize;

                constantCount = reader.ReadInt32();
                constantOffset = reader.ReadInt32();

                subType = reader.ReadInt32();
                memberOffset = subType;

                nameHash = reader.ReadUInt32();

                definitionModule = reader.ReadUInt32();
                definitionLocationStart = reader.ReadUInt32();
                definitionLocationEnd = reader.ReadUInt32();
                definitionLocationName = reader.ReadUInt32();

                definitionOffsetStart = reader.ReadUInt32();
                definitionOffset = reader.ReadUInt32();
                genericTypeCount = reader.ReadUInt32();

                namespaceHash = reader.ReadUInt32();
                baseType = reader.ReadInt32();
            }
        }
        class NullcMemberInfo
        {
            public NullcTypeInfo nullcType;
            public string name;

            public uint type;
            public uint offset;

            public void ReadFrom(BinaryReader reader)
            {
                type = reader.ReadUInt32();
                offset = reader.ReadUInt32();
            }
        }

        class NullcConstantInfo
        {
            public NullcTypeInfo nullcType;
            public string name;

            public uint type;
            public long value;

            public void ReadFrom(BinaryReader reader)
            {
                type = reader.ReadUInt32();
                value = reader.ReadInt64();
            }
        }

        class NullcVarInfo
        {
            public string name;
            public NullcTypeInfo nullcType;

            public uint offsetToName;
            public uint nameHash;

            public uint type;  // index in type array
            public uint offset;

            public void ReadFrom(BinaryReader reader)
            {
                offsetToName = reader.ReadUInt32();
                nameHash = reader.ReadUInt32();
                type = reader.ReadUInt32();
                offset = reader.ReadUInt32();
            }
        }

        class NullcFuncInfo
        {
            public string name;

            public List<NullcLocalInfo> arguments = new List<NullcLocalInfo>();
            public List<NullcLocalInfo> locals = new List<NullcLocalInfo>();
            public List<NullcLocalInfo> externs = new List<NullcLocalInfo>();

            public NullcTypeInfo nullcFunctionType;
            public NullcTypeInfo nullcParentType;
            public NullcTypeInfo nullcContextType;

            public uint offsetToName;

            public uint regVmAddress;
            public uint regVmCodeSize;
            public uint regVmRegisters;

            public uint builtinIndex;
            public uint attributes;

            public uint isVisible;

            public ulong funcPtrRaw;
            public ulong funcPtrWrapTarget;
            public ulong funcPtrWrap;

            public byte retType;  // one of the ReturnType enumeration values
            public byte funcCat;
            public byte isGenericInstance;
            public byte isOperator;
            public byte returnShift; // Amount of dwords to remove for result type after raw external function call

            public uint returnSize; // Return size of the wrapped external function call
            public int funcType;  // index to the type array

            public uint startInByteCode;
            public int parentType; // Type inside which the function is defined
            public int contextType; // Type of the function context

            public int offsetToFirstLocal;
            public int paramCount;
            public int localCount;
            public int externCount;

            public uint namespaceHash;

            public int bytesToPop; // Arguments size
            public int stackSize; // Including arguments

            // For generic functions
            public uint genericOffsetStart; // Position in the lexeme stream of the definition
            public uint genericOffset;
            public uint genericReturnType;

            // Size of the explicit type list for generic functions and generic function instances
            public uint explicitTypeCount;

            public uint nameHash;

            public uint definitionModule; // Index of the module containing the definition

            public uint definitionLocationModule;
            public uint definitionLocationStart;
            public uint definitionLocationEnd;
            public uint definitionLocationName;

            public void ReadFrom(BinaryReader reader)
            {
                offsetToName = reader.ReadUInt32();

                regVmAddress = reader.ReadUInt32();
                regVmCodeSize = reader.ReadUInt32();
                regVmRegisters = reader.ReadUInt32();

                builtinIndex = reader.ReadUInt32();
                attributes = reader.ReadUInt32();

                isVisible = reader.ReadUInt32();

                funcPtrRaw = reader.ReadUInt64();
                funcPtrWrapTarget = reader.ReadUInt64();
                funcPtrWrap = reader.ReadUInt64();

                retType = reader.ReadByte();
                funcCat = reader.ReadByte();
                isGenericInstance = reader.ReadByte();
                isOperator = reader.ReadByte();
                returnShift = reader.ReadByte();

                reader.ReadBytes(3); // Padding

                returnSize = reader.ReadUInt32();
                funcType = reader.ReadInt32();

                startInByteCode = reader.ReadUInt32();
                parentType = reader.ReadInt32();
                contextType = reader.ReadInt32();

                offsetToFirstLocal = reader.ReadInt32();
                paramCount = reader.ReadInt32();
                localCount = reader.ReadInt32();
                externCount = reader.ReadInt32();

                namespaceHash = reader.ReadUInt32();

                bytesToPop = reader.ReadInt32();
                stackSize = reader.ReadInt32();

                genericOffsetStart = reader.ReadUInt32();
                genericOffset = reader.ReadUInt32();
                genericReturnType = reader.ReadUInt32();

                explicitTypeCount = reader.ReadUInt32();

                nameHash = reader.ReadUInt32();

                definitionModule = reader.ReadUInt32();

                definitionLocationModule = reader.ReadUInt32();
                definitionLocationStart = reader.ReadUInt32();
                definitionLocationEnd = reader.ReadUInt32();
                definitionLocationName = reader.ReadUInt32();
            }
        }

        enum NullcLocalType
        {
            Parameter,
            Local,
            External,
        }

        class NullcLocalInfo
        {
            public string name;
            public NullcTypeInfo nullcType;

            public NullcLocalType paramType;
            public byte paramFlags;
            public ushort defaultFuncId;

            public uint type;
            public uint size;
            public uint offset;
            public uint closeListID;
            public uint alignmentLog2; // 1 << value

            public uint offsetToName;

            public void ReadFrom(BinaryReader reader)
            {
                paramType = (NullcLocalType)reader.ReadByte();
                paramFlags = reader.ReadByte();
                defaultFuncId = reader.ReadUInt16();

                type = reader.ReadUInt32();
                size = reader.ReadUInt32();
                offset = reader.ReadUInt32();
                closeListID = reader.ReadUInt32();
                alignmentLog2 = reader.ReadUInt32();

                offsetToName = reader.ReadUInt32();
            }
        }

        class NullcModuleInfo
        {
            public string name;

            public uint nameHash;
            public uint nameOffset;

            public uint funcStart;
            public uint funcCount;

            public uint variableOffset;

            public int sourceOffset;
            public int sourceSize;

            public uint dependencyStart;
            public uint dependencyCount;

            public void ReadFrom(BinaryReader reader)
            {
                nameHash = reader.ReadUInt32();
                nameOffset = reader.ReadUInt32();

                funcStart = reader.ReadUInt32();
                funcCount = reader.ReadUInt32();

                variableOffset = reader.ReadUInt32();

                sourceOffset = reader.ReadInt32();
                sourceSize = reader.ReadInt32();

                dependencyStart = reader.ReadUInt32();
                dependencyCount = reader.ReadUInt32();
            }
        }
        enum NullcInstructionCode
        {
            rviNop,

            rviLoadByte,
            rviLoadWord,
            rviLoadDword,
            rviLoadLong,
            rviLoadFloat,
            rviLoadDouble,

            rviLoadImm,

            rviStoreByte,
            rviStoreWord,
            rviStoreDword,
            rviStoreLong,
            rviStoreFloat,
            rviStoreDouble,

            rviCombinedd,
            rviBreakupdd,
            rviMov,
            rviMovMult,

            rviDtoi,
            rviDtol,
            rviDtof,
            rviItod,
            rviLtod,
            rviItol,
            rviLtoi,

            rviIndex,

            rviGetAddr,

            rviSetRange,
            rviMemCopy,

            rviJmp,
            rviJmpz,
            rviJmpnz,

            rviCall,
            rviCallPtr,

            rviReturn,

            rviAddImm,

            rviAdd,
            rviSub,
            rviMul,
            rviDiv,

            rviPow,
            rviMod,

            rviLess,
            rviGreater,
            rviLequal,
            rviGequal,
            rviEqual,
            rviNequal,

            rviShl,
            rviShr,

            rviBitAnd,
            rviBitOr,
            rviBitXor,

            rviAddImml,

            rviAddl,
            rviSubl,
            rviMull,
            rviDivl,

            rviPowl,
            rviModl,

            rviLessl,
            rviGreaterl,
            rviLequall,
            rviGequall,
            rviEquall,
            rviNequall,

            rviShll,
            rviShrl,

            rviBitAndl,
            rviBitOrl,
            rviBitXorl,

            rviAddd,
            rviSubd,
            rviMuld,
            rviDivd,

            rviAddf,
            rviSubf,
            rviMulf,
            rviDivf,

            rviPowd,
            rviModd,

            rviLessd,
            rviGreaterd,
            rviLequald,
            rviGequald,
            rviEquald,
            rviNequald,

            rviNeg,
            rviNegl,
            rviNegd,

            rviBitNot,
            rviBitNotl,

            rviLogNot,
            rviLogNotl,

            rviConvertPtr,

            // Temporary instructions, no execution
            rviFuncAddr,
            rviTypeid,
        };

        class NullcInstruction
        {
            public NullcInstructionCode code;
            public byte rA;
            public byte rB;
            public byte rC;
            public uint argument;

            public void ReadFrom(BinaryReader reader)
            {
                code = (NullcInstructionCode)reader.ReadByte();
                rA = reader.ReadByte();
                rB = reader.ReadByte();
                rC = reader.ReadByte();
                argument = reader.ReadUInt32();
            }
        }
        class NullcSourceInfo
        {
            public uint instruction;
            public uint definitionModule; // Index of the module containing the definition
            public int sourceOffset;

            public void ReadFrom(BinaryReader reader)
            {
                instruction = reader.ReadUInt32();
                definitionModule = reader.ReadUInt32();
                sourceOffset = reader.ReadInt32();
            }
        }

        class NullcBytecode
        {
            public List<NullcTypeInfo> types;
            public List<NullcMemberInfo> typeMembers;
            public List<NullcConstantInfo> typeConstants;
            public List<NullcVarInfo> variables;
            public List<NullcFuncInfo> functions;
            public uint[] functionExplicitTypeArrayOffsets;
            public uint[] functionExplicitTypes;
            public List<NullcLocalInfo> locals;
            public List<NullcModuleInfo> modules;
            public char[] symbols;
            public char[] source;
            public uint[] dependencies;
            public string[] importPaths;
            public string mainModuleName;
            public List<NullcInstruction> instructions;
            public List<NullcSourceInfo> sourceInfo;
            public uint[] constants;
            public ulong[] instructionPositions;
            public int globalVariableSize = 0;

            internal List<T> InitList<T>(int size) where T : new()
            {
                List<T> array = new List<T>();

                for (int i = 0; i < size; i++)
                {
                    array.Add(new T());
                }

                return array;
            }

            public void ReadFrom(byte[] data, bool is64Bit)
            {
                char[] rawImportPaths;

                using (var stream = new MemoryStream(data))
                {
                    using (var reader = new BinaryReader(stream))
                    {
                        types = InitList<NullcTypeInfo>(reader.ReadInt32());

                        int typeIndex = 0;

                        foreach (var el in types)
                        {
                            el.index = (NullcTypeIndex)typeIndex++;
                            el.ReadFrom(reader);
                        }

                        typeMembers = InitList<NullcMemberInfo>(reader.ReadInt32());

                        foreach (var el in typeMembers)
                        {
                            el.ReadFrom(reader);
                        }

                        typeConstants = InitList<NullcConstantInfo>(reader.ReadInt32());

                        foreach (var el in typeConstants)
                        {
                            el.ReadFrom(reader);
                        }

                        variables = InitList<NullcVarInfo>(reader.ReadInt32());

                        foreach (var el in variables)
                        {
                            el.ReadFrom(reader);
                        }

                        functions = InitList<NullcFuncInfo>(reader.ReadInt32());

                        foreach (var el in functions)
                        {
                            el.ReadFrom(reader);
                        }

                        functionExplicitTypeArrayOffsets = new uint[reader.ReadInt32()];

                        for (var i = 0; i < functionExplicitTypeArrayOffsets.Length; i++)
                        {
                            functionExplicitTypeArrayOffsets[i] = reader.ReadUInt32();
                        }

                        functionExplicitTypes = new uint[reader.ReadInt32()];

                        for (var i = 0; i < functionExplicitTypes.Length; i++)
                        {
                            functionExplicitTypes[i] = reader.ReadUInt32();
                        }

                        locals = InitList<NullcLocalInfo>(reader.ReadInt32());

                        foreach (var el in locals)
                        {
                            el.ReadFrom(reader);
                        }

                        modules = InitList<NullcModuleInfo>(reader.ReadInt32());

                        foreach (var el in modules)
                        {
                            el.ReadFrom(reader);
                        }

                        symbols = reader.ReadChars(reader.ReadInt32());
                        source = reader.ReadChars(reader.ReadInt32());

                        dependencies = new uint[reader.ReadInt32()];

                        for (var i = 0; i < dependencies.Length; i++)
                        {
                            dependencies[i] = reader.ReadUInt32();
                        }

                        rawImportPaths = reader.ReadChars(reader.ReadInt32());

                        mainModuleName = new string(reader.ReadChars(reader.ReadInt32()));

                        instructions = InitList<NullcInstruction>(reader.ReadInt32());

                        foreach (var el in instructions)
                        {
                            el.ReadFrom(reader);
                        }

                        sourceInfo = InitList<NullcSourceInfo>(reader.ReadInt32());

                        foreach (var el in sourceInfo)
                        {
                            el.ReadFrom(reader);
                        }

                        constants = new uint[reader.ReadInt32()];

                        for (var i = 0; i < constants.Length; i++)
                        {
                            constants[i] = reader.ReadUInt32();
                        }

                        instructionPositions = new ulong[reader.ReadInt32()];

                        for (var i = 0; i < instructionPositions.Length; i++)
                        {
                            instructionPositions[i] = is64Bit ? reader.ReadUInt64() : (ulong)reader.ReadUInt32();
                        }

                        globalVariableSize = reader.ReadInt32();
                    }
                }

                // Finalize
                foreach (var el in types)
                {
                    var ending = Array.FindIndex(symbols, (int)el.offsetToName, (x) => x == 0);

                    el.name = new string(symbols, (int)el.offsetToName, ending - (int)el.offsetToName);

                    if (el.subCat == NullcTypeSubCategory.Array || el.subCat == NullcTypeSubCategory.Pointer)
                    {
                        el.nullcSubType = el.subType == 0 ? null : types[el.subType];
                    }

                    if (el.subCat == NullcTypeSubCategory.Function || el.subCat == NullcTypeSubCategory.Class)
                    {
                        uint memberNameOffset = el.offsetToName + (uint)el.name.Length + 1;

                        // One extra member for function return type
                        for (int i = 0; i < el.memberCount + (el.subCat == NullcTypeSubCategory.Function ? 1 : 0); i++)
                        {
                            var member = typeMembers[el.memberOffset + i];

                            member.name = new string(symbols, (int)memberNameOffset, Array.FindIndex(symbols, (int)memberNameOffset, (x) => x == 0) - (int)memberNameOffset);

                            memberNameOffset = memberNameOffset + (uint)member.name.Length + 1;

                            el.nullcMembers.Add(member);
                        }

                        for (int i = 0; i < el.constantCount; i++)
                        {
                            var constant = typeConstants[el.constantOffset + i];

                            constant.name = new string(symbols, (int)memberNameOffset, Array.FindIndex(symbols, (int)memberNameOffset, (x) => x == 0) - (int)memberNameOffset);

                            memberNameOffset = memberNameOffset + (uint)constant.name.Length + 1;

                            el.nullcConstants.Add(constant);
                        }
                    }

                    el.nullcBaseType = el.baseType == 0 ? null : types[el.baseType];
                }

                foreach (var el in typeMembers)
                {
                    el.nullcType = types[(int)el.type];
                }

                foreach (var el in typeConstants)
                {
                    el.nullcType = types[(int)el.type];
                }

                foreach (var el in variables)
                {
                    var ending = Array.FindIndex(symbols, (int)el.offsetToName, (x) => x == 0);

                    el.name = new string(symbols, (int)el.offsetToName, ending - (int)el.offsetToName);

                    el.nullcType = types[(int)el.type];
                }

                foreach (var el in functions)
                {
                    var ending = Array.FindIndex(symbols, (int)el.offsetToName, (x) => x == 0);

                    el.name = new string(symbols, (int)el.offsetToName, ending - (int)el.offsetToName);

                    for (int i = 0; i < el.localCount + el.externCount; i++)
                    {
                        var local = locals[el.offsetToFirstLocal + i];

                        switch (local.paramType)
                        {
                            case NullcLocalType.Parameter:
                                el.arguments.Add(local);
                                break;
                            case NullcLocalType.Local:
                                el.locals.Add(local);
                                break;
                            case NullcLocalType.External:
                                el.externs.Add(local);
                                break;
                        }
                    }

                    el.nullcFunctionType = types[el.funcType];
                    el.nullcParentType = el.parentType == -1 ? null : types[el.parentType];
                    el.nullcContextType = el.contextType == 0 ? null : types[el.contextType];
                }

                foreach (var el in locals)
                {
                    var ending = Array.FindIndex(symbols, (int)el.offsetToName, (x) => x == 0);

                    el.name = new string(symbols, (int)el.offsetToName, ending - (int)el.offsetToName);

                    el.nullcType = types[(int)el.type];
                }

                foreach (var el in modules)
                {
                    var ending = Array.FindIndex(symbols, (int)el.nameOffset, (x) => x == 0);

                    el.name = new string(symbols, (int)el.nameOffset, ending - (int)el.nameOffset);
                }

                importPaths = (new string(rawImportPaths)).Split(';');
            }

            public int ConvertNativeAddressToInstruction(ulong address)
            {
                int lowerBound = 0;
                int upperBound = instructionPositions.Length - 1;
                int index = 0;

                while (lowerBound <= upperBound)
                {
                    index = (lowerBound + upperBound) >> 1;

                    if (address < instructionPositions[index])
                    {
                        upperBound = index - 1;
                    }
                    else if (address > instructionPositions[index])
                    {
                        lowerBound = index + 1;
                    }
                    else
                    {
                        break;
                    }
                }

                if (index != 0 && address < instructionPositions[index])
                {
                    index--;
                }

                if (index == instructionPositions.Length - 1 && address > instructionPositions[index])
                {
                    return 0;
                }

                return index;
            }

            public ulong ConvertInstructionToNativeAddress(int instruction)
            {
                return instructionPositions[instruction];
            }

            public NullcFuncInfo GetFunctionAtAddress(int instruction)
            {
                for (int i = 0; i < functions.Count; i++)
                {
                    var function = functions[i];

                    if (instruction >= function.regVmAddress && instruction < (function.regVmAddress + function.regVmCodeSize))
                    {
                        return function;
                    }
                }

                return null;
            }

            public NullcFuncInfo GetFunctionAtNativeAddress(ulong address)
            {
                return GetFunctionAtAddress(ConvertNativeAddressToInstruction(address));
            }

            public int GetInstructionSourceLocation(int instruction)
            {
                Debug.Assert(instruction > 0);

                for (int i = 0; i < sourceInfo.Count; i++)
                {
                    if (instruction == sourceInfo[i].instruction)
                    {
                        return sourceInfo[i].sourceOffset;
                    }

                    if (i + 1 < sourceInfo.Count && instruction < sourceInfo[i + 1].instruction)
                    {
                        return sourceInfo[i].sourceOffset;
                    }
                }

                return sourceInfo[sourceInfo.Count - 1].sourceOffset;
            }

            public int GetSourceLocationModuleIndex(int sourceLocation)
            {
                Debug.Assert(sourceLocation != -1);

                for (int i = 0; i < modules.Count; i++)
                {
                    var moduleInfo = modules[i];

                    int start = moduleInfo.sourceOffset;
                    int end = start + moduleInfo.sourceSize;

                    if (sourceLocation >= start && sourceLocation < end)
                    {
                        return i;
                    }
                }

                return -1;
            }

            public int GetSourceLocationLineAndColumn(int sourceLocation, int moduleIndex, out int column)
            {
                Debug.Assert(sourceLocation != -1);

                var moduleCount = modules.Count;

                int sourceStart = moduleIndex != -1 ? modules[moduleIndex].sourceOffset : modules[moduleCount - 1].sourceOffset + modules[moduleCount - 1].sourceSize;

                int line = 0;

                int pos = sourceStart;
                int lastLineStart = pos;

                while (pos < sourceLocation)
                {
                    if (source[pos] == '\r')
                    {
                        line++;

                        pos++;

                        if (source[pos] == '\n')
                        {
                            pos++;
                        }

                        lastLineStart = pos;
                    }
                    else if (source[pos] == '\n')
                    {
                        line++;

                        pos++;

                        lastLineStart = pos;
                    }
                    else
                    {
                        pos++;
                    }
                }

                column = pos - lastLineStart + 1;

                return line + 1;
            }

            public int GetInstructionSourceLocationLine(int instruction, out int moduleIndex)
            {
                int sourceLocation = GetInstructionSourceLocation(instruction);

                moduleIndex = GetSourceLocationModuleIndex(sourceLocation);

                return GetSourceLocationLineAndColumn(sourceLocation, moduleIndex, out _);
            }

            int GetLineStartOffset(int moduleSourceCodeOffset, int line)
            {
                int start = moduleSourceCodeOffset;
                int startLine = 0;

                while (source[start] != 0 && startLine < line)
                {
                    if (source[start] == '\r')
                    {
                        start++;

                        if (source[start] == '\n')
                        {
                            start++;
                        }

                        startLine++;
                    }
                    else if (source[start] == '\n')
                    {
                        start++;

                        startLine++;
                    }
                    else
                    {
                        start++;
                    }
                }

                return start;
            }

            int GetLineEndOffset(int lineStartOffset)
            {
                int pos = lineStartOffset;

                while (source[pos] != 0)
                {
                    if (source[pos] == '\r')
                    {
                        return pos;
                    }

                    if (source[pos] == '\n')
                    {
                        return pos;
                    }

                    pos++;
                }

                return pos;
            }

            int ConvertLinePositionRangeToInstruction(int lineStartOffset, int lineEndOffset)
            {
                // Find instruction
                for (int i = 0; i < sourceInfo.Count; i++)
                {
                    if (sourceInfo[i].sourceOffset >= lineStartOffset && sourceInfo[i].sourceOffset <= lineEndOffset)
                    {
                        return (int)sourceInfo[i].instruction;
                    }
                }

                return 0;
            }

            public int ConvertLineToInstruction(int moduleSourceCodeOffset, int line)
            {
                int lineStartOffset = GetLineStartOffset(moduleSourceCodeOffset, line);

                if (lineStartOffset == 0)
                {
                    return 0;
                }

                int lineEndOffset = GetLineEndOffset(lineStartOffset);

                return ConvertLinePositionRangeToInstruction(lineStartOffset, lineEndOffset);
            }

            public int GetModuleSourceLocation(int moduleIndex)
            {
                Debug.Assert(moduleIndex >= -1 && moduleIndex < modules.Count);

                if (moduleIndex == -1)
                {
                    return modules.Last().sourceOffset + modules.Last().sourceSize;
                }

                if (moduleIndex < modules.Count)
                {
                    return modules[moduleIndex].sourceOffset;
                }

                return -1;
            }
        }
    }
}
