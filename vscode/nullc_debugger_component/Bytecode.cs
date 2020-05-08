using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;

namespace nullc_debugger_component
{
    namespace DkmDebugger
    {
        class NullcTypeInfo
        {
            public string name;

            public uint offsetToName;

            public uint size;
            public uint padding;
            public uint typeCategory;
            public uint subCat;
            public byte defaultAlign;
            public byte typeFlags;
            public ushort pointerCount;

            public uint arrSizeOrMemberCount;
            public uint constantCount;
            public uint subTypeOrMemberOffset;
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
            public uint baseType;

            public void ReadFrom(BinaryReader reader)
            {
                offsetToName = reader.ReadUInt32();

                size = reader.ReadUInt32();
                padding = reader.ReadUInt32();
                typeCategory = reader.ReadUInt32();
                subCat = reader.ReadUInt32();
                defaultAlign = reader.ReadByte();
                typeFlags = reader.ReadByte();
                pointerCount = reader.ReadUInt16();

                arrSizeOrMemberCount = reader.ReadUInt32();
                constantCount = reader.ReadUInt32();
                subTypeOrMemberOffset = reader.ReadUInt32();
                nameHash = reader.ReadUInt32();

                definitionModule = reader.ReadUInt32();
                definitionLocationStart = reader.ReadUInt32();
                definitionLocationEnd = reader.ReadUInt32();
                definitionLocationName = reader.ReadUInt32();

                definitionOffsetStart = reader.ReadUInt32();
                definitionOffset = reader.ReadUInt32();
                genericTypeCount = reader.ReadUInt32();

                namespaceHash = reader.ReadUInt32();
                baseType = reader.ReadUInt32();
            }
        }
        class NullcMemberInfo
        {
            public NullcTypeInfo nullcType;

            public uint type;
            public uint offset;

            public void ReadFrom(BinaryReader reader)
            {
                type = reader.ReadUInt32();
                offset = reader.ReadUInt32();
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

            public uint offsetToName;

            public uint regVmAddress;
            public uint regVmCodeSize;
            public uint regVmRegisters;

            public uint builtinIndex;

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
            public uint funcType;  // index to the type array

            public uint startInByteCode;
            public uint parentType; // Type inside which the function is defined
            public uint contextType; // Type of the function context

            public uint offsetToFirstLocal;
            public uint paramCount;
            public uint localCount;
            public uint externCount;

            public uint namespaceHash;

            public uint bytesToPop; // Arguments size
            public uint stackSize; // Including arguments

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
                funcType = reader.ReadUInt32();

                startInByteCode = reader.ReadUInt32();
                parentType = reader.ReadUInt32();
                contextType = reader.ReadUInt32();

                offsetToFirstLocal = reader.ReadUInt32();
                paramCount = reader.ReadUInt32();
                localCount = reader.ReadUInt32();
                externCount = reader.ReadUInt32();

                namespaceHash = reader.ReadUInt32();

                bytesToPop = reader.ReadUInt32();
                stackSize = reader.ReadUInt32();

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

        class NullcLocalInfo
        {
            public string name;
            public NullcTypeInfo nullcType;

            public byte paramType;
            public byte paramFlags;
            public ushort defaultFuncId;

            public uint type;
            public uint size;
            public uint offsetOrTarget;
            public uint closeListID;
            public uint alignmentLog2; // 1 << value

            public uint offsetToName;

            public void ReadFrom(BinaryReader reader)
            {
                paramType = reader.ReadByte();
                paramFlags = reader.ReadByte();
                defaultFuncId = reader.ReadUInt16();

                type = reader.ReadUInt32();
                size = reader.ReadUInt32();
                offsetOrTarget = reader.ReadUInt32();
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
            public List<NullcMemberInfo> typeExtra;
            public List<NullcVarInfo> variables;
            public List<NullcFuncInfo> functions;
            public uint[] functionExplicitTypeArrayOffsets;
            public uint[] functionExplicitTypes;
            public List<NullcLocalInfo> locals;
            public List<NullcModuleInfo> modules;
            public char[] symbols;
            public char[] source;
            public uint[] dependencies;
            public List<NullcSourceInfo> sourceInfo;
            public uint[] constants;
            public ulong[] instructionPositions;

            internal List<T> InitList<T>(int size) where T : new()
            {
                List<T> array = new List<T>();

                for (int i = 0; i < size; i++)
                    array.Add(new T());

                return array;
            }

            public void ReadFrom(byte[] data, bool is64Bit)
            {
                using (var stream = new MemoryStream(data))
                {
                    using (var reader = new BinaryReader(stream))
                    {
                        types = InitList<NullcTypeInfo>(reader.ReadInt32());

                        foreach (var el in types)
                            el.ReadFrom(reader);

                        typeExtra = InitList<NullcMemberInfo>(reader.ReadInt32());

                        foreach (var el in typeExtra)
                            el.ReadFrom(reader);

                        variables = InitList<NullcVarInfo>(reader.ReadInt32());

                        foreach (var el in variables)
                            el.ReadFrom(reader);

                        functions = InitList<NullcFuncInfo>(reader.ReadInt32());

                        foreach (var el in functions)
                            el.ReadFrom(reader);

                        functionExplicitTypeArrayOffsets = new uint[reader.ReadInt32()];

                        for (var i = 0; i < functionExplicitTypeArrayOffsets.Length; i++)
                            functionExplicitTypeArrayOffsets[i] = reader.ReadUInt32();

                        functionExplicitTypes = new uint[reader.ReadInt32()];

                        for (var i = 0; i < functionExplicitTypes.Length; i++)
                            functionExplicitTypes[i] = reader.ReadUInt32();

                        locals = InitList<NullcLocalInfo>(reader.ReadInt32());

                        foreach (var el in locals)
                            el.ReadFrom(reader);

                        modules = InitList<NullcModuleInfo>(reader.ReadInt32());

                        foreach (var el in modules)
                            el.ReadFrom(reader);

                        symbols = reader.ReadChars(reader.ReadInt32());
                        source = reader.ReadChars(reader.ReadInt32());

                        dependencies = new uint[reader.ReadInt32()];

                        for (var i = 0; i < dependencies.Length; i++)
                            dependencies[i] = reader.ReadUInt32();

                        sourceInfo = InitList<NullcSourceInfo>(reader.ReadInt32());

                        foreach (var el in sourceInfo)
                            el.ReadFrom(reader);

                        constants = new uint[reader.ReadInt32()];

                        for (var i = 0; i < constants.Length; i++)
                            constants[i] = reader.ReadUInt32();

                        instructionPositions = new ulong[reader.ReadInt32()];

                        for (var i = 0; i < instructionPositions.Length; i++)
                            instructionPositions[i] = is64Bit ? reader.ReadUInt64() : (ulong)reader.ReadUInt32();
                    }
                }

                // Finalize
                foreach (var el in types)
                {
                    var ending = Array.FindIndex(symbols, (int)el.offsetToName, (x) => x == 0);

                    el.name = new string(symbols, (int)el.offsetToName, ending - (int)el.offsetToName);
                }

                foreach (var el in typeExtra)
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
                        upperBound = index - 1;
                    else if (address > instructionPositions[index])
                        lowerBound = index + 1;
                    else
                        break;
                }

                if (index != 0 && address < instructionPositions[index])
                    index--;

                return index;
            }

            public int GetInstructionSourceLocation(int instruction)
            {
                Debug.Assert(instruction > 0);

                for (int i = 0; i < sourceInfo.Count; i++)
                {
                    if (instruction == sourceInfo[i].instruction)
                        return sourceInfo[i].sourceOffset;

                    if (i + 1 < sourceInfo.Count && instruction < sourceInfo[i + 1].instruction)
                        return sourceInfo[i].sourceOffset;
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
                        return i;
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
                            pos++;

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
        }
    }
}
