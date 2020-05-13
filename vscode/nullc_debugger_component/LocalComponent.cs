using Microsoft.VisualStudio.Debugger;
using Microsoft.VisualStudio.Debugger.CallStack;
using Microsoft.VisualStudio.Debugger.ComponentInterfaces;
using Microsoft.VisualStudio.Debugger.CustomRuntimes;
using Microsoft.VisualStudio.Debugger.Evaluation;
using Microsoft.VisualStudio.Debugger.Native;
using Microsoft.VisualStudio.Debugger.Symbols;
using System;
using System.Diagnostics;
using System.IO;
using System.Linq;

namespace nullc_debugger_component
{
    namespace DkmDebugger
    {
        internal class NullcLocalProcessDataItem : DkmDataItem
        {
            public ulong moduleBytecodeLocation = 0;
            public ulong moduleBytecodeSize = 0;
            public byte[] moduleBytecodeRaw;

            public NullcBytecode bytecode;

            public ulong moduleContextMainDataLocation = 0;

            public ulong dataStackBase = 0;
            public ulong dataStackTop = 0;

            public ulong callStackBase = 0;
            public ulong callStackTop = 0;

            public ulong regFileArrayBase = 0;
            public ulong regFileLastPtr = 0;
            public ulong regFileLastTop = 0;

            public NullcCallStack callStack = new NullcCallStack();

            public bool UpdateContextData(DkmProcess process)
            {
                if (moduleContextMainDataLocation == 0)
                    return false;

                dataStackBase = DebugHelpers.ReadPointerVariable(process, moduleContextMainDataLocation + (ulong)DebugHelpers.GetPointerSize(process) * 0).GetValueOrDefault(0);
                dataStackTop = DebugHelpers.ReadPointerVariable(process, moduleContextMainDataLocation + (ulong)DebugHelpers.GetPointerSize(process) * 1).GetValueOrDefault(0);

                callStackBase = DebugHelpers.ReadPointerVariable(process, moduleContextMainDataLocation + (ulong)DebugHelpers.GetPointerSize(process) * 3).GetValueOrDefault(0);
                callStackTop = DebugHelpers.ReadPointerVariable(process, moduleContextMainDataLocation + (ulong)DebugHelpers.GetPointerSize(process) * 4).GetValueOrDefault(0);

                regFileArrayBase = DebugHelpers.ReadPointerVariable(process, moduleContextMainDataLocation + (ulong)DebugHelpers.GetPointerSize(process) * 6).GetValueOrDefault(0);
                regFileLastPtr = DebugHelpers.ReadPointerVariable(process, moduleContextMainDataLocation + (ulong)DebugHelpers.GetPointerSize(process) * 7).GetValueOrDefault(0);
                regFileLastTop = DebugHelpers.ReadPointerVariable(process, moduleContextMainDataLocation + (ulong)DebugHelpers.GetPointerSize(process) * 8).GetValueOrDefault(0);

                return dataStackBase != 0 && callStackBase != 0 && regFileArrayBase != 0;
            }

            public bool UpdateCallStack(DkmProcess process)
            {
                if (bytecode == null)
                    return false;

                callStack.UpdateFrom(process, callStackBase, callStackTop, bytecode);

                return true;
            }
        }

        internal class NullEvaluationDataItem : DkmDataItem
        {
            public ulong address;
            public NullcTypeInfo type;
            public string fullName;
        }

        internal class NullFrameLocalsDataItem : DkmDataItem
        {
            public NullcCallStackEntry activeEntry;
        }

        internal class NullResolvedDocumentDataItem : DkmDataItem
        {
            public NullcBytecode bytecode;
            public ulong moduleBase;

            public int moduleIndex;
        }

        public class NullcLocalComponent : IDkmSymbolCompilerIdQuery, IDkmSymbolDocumentCollectionQuery, IDkmSymbolDocumentSpanQuery, IDkmSymbolQuery, IDkmLanguageFrameDecoder, IDkmModuleInstanceLoadNotification, IDkmLanguageExpressionEvaluator
        {
            DkmCompilerId IDkmSymbolCompilerIdQuery.GetCompilerId(DkmInstructionSymbol instruction, DkmInspectionSession inspectionSession)
            {
                if (instruction.Module.Name != "nullc.embedded.code")
                    return new DkmCompilerId(Guid.Empty, Guid.Empty);

                return new DkmCompilerId(Guid.Empty, Guid.Empty);
            }

            DkmResolvedDocument[] IDkmSymbolDocumentCollectionQuery.FindDocuments(DkmModule module, DkmSourceFileId sourceFileId)
            {
                if (module.Name != "nullc.embedded.code")
                    return module.FindDocuments(sourceFileId);

                var nullcModuleInstance = module.GetModuleInstances().OfType<DkmCustomModuleInstance>().FirstOrDefault(el => el.Module.CompilerId.VendorId == DebugHelpers.NullcCompilerGuid);

                if (nullcModuleInstance == null)
                    return module.FindDocuments(sourceFileId);

                var processData = DebugHelpers.GetOrCreateDataItem<NullcLocalProcessDataItem>(nullcModuleInstance.Process);

                if (processData.bytecode != null)
                {
                    var processPath = nullcModuleInstance.Process.Path;

                    var dataItem = new NullResolvedDocumentDataItem();

                    dataItem.bytecode = processData.bytecode;
                    dataItem.moduleBase = nullcModuleInstance.BaseAddress;

                    foreach (var nullcModule in processData.bytecode.modules)
                    {
                        foreach (var importPath in processData.bytecode.importPaths)
                        {
                            var finalPath = importPath.Replace('/', '\\');

                            if (finalPath.Length == 0)
                                finalPath = $"{Path.GetDirectoryName(processPath)}\\";
                            else if (!Path.IsPathRooted(finalPath))
                                finalPath = Path.GetFullPath(Path.Combine(Path.GetDirectoryName(processPath), finalPath));

                            var modulePath = nullcModule.name.Replace('/', '\\');

                            var combined = $"{finalPath}{modulePath}";

                            if (combined == sourceFileId.DocumentName)
                            {
                                dataItem.moduleIndex = processData.bytecode.modules.IndexOf(nullcModule);

                                return new DkmResolvedDocument[1] { DkmResolvedDocument.Create(module, sourceFileId.DocumentName, null, DkmDocumentMatchStrength.FullPath, DkmResolvedDocumentWarning.None, false, dataItem) };
                            }

                            if (combined.ToLowerInvariant() == sourceFileId.DocumentName.ToLowerInvariant())
                            {
                                dataItem.moduleIndex = processData.bytecode.modules.IndexOf(nullcModule);

                                return new DkmResolvedDocument[1] { DkmResolvedDocument.Create(module, sourceFileId.DocumentName, null, DkmDocumentMatchStrength.SubPath, DkmResolvedDocumentWarning.None, false, dataItem) };
                            }

                            if (combined.ToLowerInvariant().EndsWith(sourceFileId.DocumentName.ToLowerInvariant()))
                            {
                                if (File.Exists(combined))
                                {
                                    dataItem.moduleIndex = processData.bytecode.modules.IndexOf(nullcModule);

                                    return new DkmResolvedDocument[1] { DkmResolvedDocument.Create(module, sourceFileId.DocumentName, null, DkmDocumentMatchStrength.SubPath, DkmResolvedDocumentWarning.None, false, dataItem) };
                                }
                            }
                        }
                    }
                }

                return module.FindDocuments(sourceFileId);
            }

            DkmInstructionSymbol[] IDkmSymbolDocumentSpanQuery.FindSymbols(DkmResolvedDocument resolvedDocument, DkmTextSpan textSpan, string text, out DkmSourcePosition[] symbolLocation)
            {
                var documentData = DebugHelpers.GetOrCreateDataItem<NullResolvedDocumentDataItem>(resolvedDocument);

                if (documentData == null)
                    return resolvedDocument.FindSymbols(textSpan, text, out symbolLocation);

                for (int line = textSpan.StartLine; line < textSpan.EndLine; line++)
                {
                    int moduleSourceLocation = documentData.bytecode.GetModuleSourceLocation(documentData.moduleIndex);

                    if (moduleSourceLocation == -1)
                        continue;

                    int instruction = documentData.bytecode.ConvertLineToInstruction(moduleSourceLocation, line - 1);

                    if (instruction == 0)
                        continue;

                    ulong nativeInstruction = documentData.bytecode.ConvertInstructionToNativeAddress(instruction);

                    Debug.Assert(nativeInstruction >= documentData.moduleBase);

                    var sourceFileId = DkmSourceFileId.Create(resolvedDocument.DocumentName, null, null, null);

                    var resultSpan = new DkmTextSpan(line, line + 1, 0, 0);

                    symbolLocation = new DkmSourcePosition[1] { DkmSourcePosition.Create(sourceFileId, resultSpan) };

                    return new DkmInstructionSymbol[1] { DkmNativeInstructionSymbol.Create(resolvedDocument.Module, (uint)(nativeInstruction - documentData.moduleBase)) };
                }

                return resolvedDocument.FindSymbols(textSpan, text, out symbolLocation);
            }

            DkmSourcePosition IDkmSymbolQuery.GetSourcePosition(DkmInstructionSymbol instruction, DkmSourcePositionFlags flags, DkmInspectionSession inspectionSession, out bool startOfLine)
            {
                var nullcModuleInstance = instruction.Module.GetModuleInstances().OfType<DkmCustomModuleInstance>().FirstOrDefault(el => el.Module.CompilerId.VendorId == DebugHelpers.NullcCompilerGuid);

                if (nullcModuleInstance == null)
                    return instruction.GetSourcePosition(flags, inspectionSession, out startOfLine);

                var processData = DebugHelpers.GetOrCreateDataItem<NullcLocalProcessDataItem>(nullcModuleInstance.Process);

                if (processData.bytecode != null)
                {
                    var instructionSymbol = instruction as DkmCustomInstructionSymbol;

                    if (instructionSymbol != null)
                    {
                        int nullcInstruction = processData.bytecode.ConvertNativeAddressToInstruction(instructionSymbol.Offset);

                        if (nullcInstruction != 0)
                        {
                            int sourceLocation = processData.bytecode.GetInstructionSourceLocation(nullcInstruction);

                            int moduleIndex = processData.bytecode.GetSourceLocationModuleIndex(sourceLocation);

                            int column = 0;
                            int line = processData.bytecode.GetSourceLocationLineAndColumn(sourceLocation, moduleIndex, out column);

                            string moduleName = moduleIndex != -1 ? processData.bytecode.modules[moduleIndex].name : "nbody.nc"; // TODO: main module name

                            // TODO: correct path resolve
                            string path = "L:\\dev\\nullc_debug_test\\bin\\" + moduleName;

                            startOfLine = true;
                            return DkmSourcePosition.Create(DkmSourceFileId.Create(path, null, null, null), new DkmTextSpan(line, line, column, column));
                        }
                    }
                }

                return instruction.GetSourcePosition(flags, inspectionSession, out startOfLine);
            }

            object IDkmSymbolQuery.GetSymbolInterface(DkmModule module, Guid interfaceID)
            {
                if (module.Name != "nullc.embedded.code")
                    throw new NotImplementedException();

                throw new NotImplementedException();
            }

            void IDkmLanguageFrameDecoder.GetFrameName(DkmInspectionContext inspectionContext, DkmWorkList workList, DkmStackWalkFrame frame, DkmVariableInfoFlags argumentFlags, DkmCompletionRoutine<DkmGetFrameNameAsyncResult> completionRoutine)
            {
                var process = frame.Process;

                var processData = DebugHelpers.GetOrCreateDataItem<NullcLocalProcessDataItem>(process);

                var function = processData.bytecode.GetFunctionAtNativeAddress(frame.InstructionAddress.CPUInstructionPart.InstructionPointer);

                int nullcInstruction = processData.bytecode.ConvertNativeAddressToInstruction(frame.InstructionAddress.CPUInstructionPart.InstructionPointer);

                if (function != null)
                {
                    string result = $"{function.name}(";

                    for (int i = 0; i < function.paramCount; i++)
                    {
                        var localInfo = function.arguments[i];

                        if (i != 0)
                            result += ", ";

                        result += $"{localInfo.nullcType.name} {localInfo.name}";
                    }

                    result += ")";

                    completionRoutine(new DkmGetFrameNameAsyncResult(result));
                    return;
                }
                else if (nullcInstruction != 0)
                {
                    completionRoutine(new DkmGetFrameNameAsyncResult("nullcGlobal()"));
                    return;
                }

                inspectionContext.GetFrameName(workList, frame, argumentFlags, completionRoutine);
            }

            void IDkmLanguageFrameDecoder.GetFrameReturnType(DkmInspectionContext inspectionContext, DkmWorkList workList, DkmStackWalkFrame frame, DkmCompletionRoutine<DkmGetFrameReturnTypeAsyncResult> completionRoutine)
            {
                inspectionContext.GetFrameReturnType(workList, frame, completionRoutine);
                // Not provided at the moment
                //completionRoutine(new DkmGetFrameReturnTypeAsyncResult(null));
            }

            void IDkmModuleInstanceLoadNotification.OnModuleInstanceLoad(DkmModuleInstance moduleInstance, DkmWorkList workList, DkmEventDescriptorS eventDescriptor)
            {
                var process = moduleInstance.Process;

                var processData = DebugHelpers.GetOrCreateDataItem<NullcLocalProcessDataItem>(process);

                if (processData.moduleBytecodeLocation == 0)
                {
                    processData.moduleBytecodeLocation = DebugHelpers.ReadPointerVariable(process, "nullcModuleBytecodeLocation").GetValueOrDefault(0);
                    processData.moduleBytecodeSize = DebugHelpers.ReadPointerVariable(process, "nullcModuleBytecodeSize").GetValueOrDefault(0);

                    if (processData.moduleBytecodeLocation != 0)
                    {
                        processData.moduleBytecodeRaw = new byte[processData.moduleBytecodeSize];
                        process.ReadMemory(processData.moduleBytecodeLocation, DkmReadMemoryFlags.None, processData.moduleBytecodeRaw);

                        processData.bytecode = new NullcBytecode();
                        processData.bytecode.ReadFrom(processData.moduleBytecodeRaw, DebugHelpers.Is64Bit(process));
                    }

                    processData.moduleContextMainDataLocation = DebugHelpers.ReadPointerVariable(process, "nullcModuleContextMainDataAddress").GetValueOrDefault(0);
                }
            }

            string EvaluateValueAtAddress(DkmProcess process, NullcBytecode bytecode, NullcTypeInfo type, ulong address, out string editableValue, ref DkmEvaluationResultFlags flags, out DkmDataAddress dataAddress)
            {
                editableValue = null;
                dataAddress = null;

                if (type.subCat == NullTypeSubCategory.Pointer)
                {
                    var value = DebugHelpers.ReadPointerVariable(process, address);

                    if (value.HasValue)
                    {
                        flags |= DkmEvaluationResultFlags.Address | DkmEvaluationResultFlags.Expandable;
                        editableValue = $"{value.Value}";
                        dataAddress = DkmDataAddress.Create(process.GetNativeRuntimeInstance(), value.Value, null);
                        return $"0x{value.Value:x8}";
                    }

                    return null;
                }

                if (type.subCat == NullTypeSubCategory.Class)
                {
                    if (type.typeCategory == NullcTypeCategory.Int)
                    {
                        var value = DebugHelpers.ReadIntVariable(process, address);

                        if (value.HasValue)
                        {
                            for (int i = 0; i < type.nullcConstants.Count; i++)
                            {
                                var constant = type.nullcConstants[i];

                                if (constant.value == value.Value)
                                {
                                    editableValue = $"{value.Value}";
                                    return $"{constant.name} ({value.Value})";
                                }
                            }

                            editableValue = $"{value.Value}";
                            return $"({type.name}){value.Value}";
                        }

                        return null;
                    }

                    flags |= DkmEvaluationResultFlags.Expandable | DkmEvaluationResultFlags.ReadOnly;
                    return "{}";
                }

                if (type.subCat == NullTypeSubCategory.Array)
                {
                    if (type.arrSize == -1)
                    {
                        var pointer = DebugHelpers.ReadPointerVariable(process, address);
                        var length = DebugHelpers.ReadIntVariable(process, address + (ulong)DebugHelpers.GetPointerSize(process));

                        if (pointer.HasValue && length.HasValue)
                        {
                            flags |= DkmEvaluationResultFlags.IsBuiltInType | DkmEvaluationResultFlags.Expandable | DkmEvaluationResultFlags.ReadOnly;
                            return $"0x{pointer.Value:x8} Size: {length.Value}";
                        }

                        return null;
                    }

                    flags |= DkmEvaluationResultFlags.Expandable | DkmEvaluationResultFlags.ReadOnly;
                    return $"Size: {type.arrSize}";
                }

                if (type.subCat == NullTypeSubCategory.Function)
                {
                    var context = DebugHelpers.ReadPointerVariable(process, address);
                    var id = DebugHelpers.ReadIntVariable(process, address + (ulong)DebugHelpers.GetPointerSize(process));

                    if (context.HasValue && id.HasValue)
                    {
                        var function = bytecode.functions[id.Value];
                        var returnType = function.nullcFunctionType.nullcMembers.First().nullcType;

                        string result = "";

                        result += $"{returnType.name} {function.name}(";

                        for (int i = 0; i < function.paramCount; i++)
                        {
                            var localInfo = function.arguments[i];

                            if (i != 0)
                                result += ", ";

                            result += $"{localInfo.nullcType.name} {localInfo.name}";
                        }

                        result += ")";

                        flags |= DkmEvaluationResultFlags.ReadOnly;
                        return result;
                    }

                    return null;
                }

                if (type.index == NullcTypeIndex.Bool)
                {
                    var value = DebugHelpers.ReadByteVariable(process, address);

                    if (value.HasValue)
                    {
                        if (value.Value != 0)
                        {
                            flags |= DkmEvaluationResultFlags.Boolean | DkmEvaluationResultFlags.BooleanTrue;
                            editableValue = $"{value.Value}";
                            return "true";
                        }
                        else
                        {
                            flags |= DkmEvaluationResultFlags.Boolean;
                            editableValue = $"{value.Value}";
                            return "false";
                        }
                    }

                    return null;
                }

                if (type.index == NullcTypeIndex.TypeId)
                {
                    var value = DebugHelpers.ReadIntVariable(process, address);

                    if (value.HasValue)
                    {
                        flags |= DkmEvaluationResultFlags.IsBuiltInType;
                        editableValue = $"{value.Value}";
                        return bytecode.types[value.Value].name;
                    }

                    return null;
                }

                switch (type.typeCategory)
                {
                    case NullcTypeCategory.Char:
                        {
                            var value = DebugHelpers.ReadByteVariable(process, address);

                            if (value.HasValue)
                            {
                                if (value > 0 && value <= 127)
                                {
                                    flags |= DkmEvaluationResultFlags.IsBuiltInType;
                                    editableValue = $"{value.Value}";
                                    return $"'{(char)value.Value}' ({value.Value:d})";
                                }
                                else
                                {
                                    flags |= DkmEvaluationResultFlags.IsBuiltInType;
                                    editableValue = $"{value.Value}";
                                    return $"{value.Value:d}";
                                }
                            }
                        }
                        break;
                    case NullcTypeCategory.Short:
                        {
                            var value = DebugHelpers.ReadShortVariable(process, address);

                            if (value.HasValue)
                            {
                                flags |= DkmEvaluationResultFlags.IsBuiltInType;
                                editableValue = $"{value.Value}";
                                return $"{value.Value}";
                            }
                        }
                        break;
                    case NullcTypeCategory.Int:
                        {
                            var value = DebugHelpers.ReadIntVariable(process, address);

                            if (value.HasValue)
                            {
                                flags |= DkmEvaluationResultFlags.IsBuiltInType;
                                editableValue = $"{value.Value}";
                                return $"{value.Value}";
                            }
                        }
                        break;
                    case NullcTypeCategory.Long:
                        {
                            var value = DebugHelpers.ReadLongVariable(process, address);

                            if (value.HasValue)
                            {
                                flags |= DkmEvaluationResultFlags.IsBuiltInType;
                                editableValue = $"{value.Value}";
                                return $"{value.Value}";
                            }
                        }
                        break;
                    case NullcTypeCategory.Float:
                        {
                            var value = DebugHelpers.ReadFloatVariable(process, address);

                            if (value.HasValue)
                            {
                                flags |= DkmEvaluationResultFlags.IsBuiltInType;
                                editableValue = $"{value.Value}";
                                return $"{value.Value}";
                            }
                        }
                        break;
                    case NullcTypeCategory.Double:
                        {
                            var value = DebugHelpers.ReadDoubleVariable(process, address);

                            if (value.HasValue)
                            {
                                flags |= DkmEvaluationResultFlags.IsBuiltInType;
                                editableValue = $"{value.Value}";
                                return $"{value.Value}";
                            }
                        }
                        break;
                    default:
                        break;
                }

                return null;
            }

            void SetValueAtAddress(DkmProcess process, NullcBytecode bytecode, NullcTypeInfo type, ulong address, string value, out string error)
            {
                if (type.subCat == NullTypeSubCategory.Pointer)
                {
                    error = "Can't modify pointer value";
                    return;
                }

                if (type.subCat == NullTypeSubCategory.Class)
                {
                    if (type.typeCategory == NullcTypeCategory.Int)
                    {
                        if (!DebugHelpers.TryWriteVariable(process, address, int.Parse(value)))
                        {
                            error = "Failed to modify target process memory";
                            return;
                        }

                        error = null;
                        return;
                    }

                    error = "Can't modify class value";
                    return;
                }

                if (type.subCat == NullTypeSubCategory.Array)
                {
                    error = "Can't modify array value";
                    return;
                }

                if (type.subCat == NullTypeSubCategory.Function)
                {
                    error = "Can't modify function value";
                    return;
                }

                if (type.index == NullcTypeIndex.Bool)
                {
                    if (value == "true")
                    {
                        if (!DebugHelpers.TryWriteVariable(process, address, (byte)1))
                        {
                            error = "Failed to modify target process memory";
                            return;
                        }
                    }
                    else if (value == "false")
                    {
                        if (!DebugHelpers.TryWriteVariable(process, address, (byte)0))
                        {
                            error = "Failed to modify target process memory";
                            return;
                        }
                    }
                    else
                    {
                        if (!DebugHelpers.TryWriteVariable(process, address, (byte)(int.Parse(value) != 0 ? 1 : 0)))
                        {
                            error = "Failed to modify target process memory";
                            return;
                        }
                    }

                    error = null;
                    return;
                }

                switch (type.typeCategory)
                {
                    case NullcTypeCategory.Char:
                        if (!DebugHelpers.TryWriteVariable(process, address, byte.Parse(value)))
                        {
                            error = "Failed to modify target process memory";
                            return;
                        }
                        break;
                    case NullcTypeCategory.Short:
                        if (!DebugHelpers.TryWriteVariable(process, address, short.Parse(value)))
                        {
                            error = "Failed to modify target process memory";
                            return;
                        }
                        break;
                    case NullcTypeCategory.Int:
                        if (!DebugHelpers.TryWriteVariable(process, address, int.Parse(value)))
                        {
                            error = "Failed to modify target process memory";
                            return;
                        }
                        break;
                    case NullcTypeCategory.Long:
                        if (!DebugHelpers.TryWriteVariable(process, address, long.Parse(value)))
                        {
                            error = "Failed to modify target process memory";
                            return;
                        }
                        break;
                    case NullcTypeCategory.Float:
                        if (!DebugHelpers.TryWriteVariable(process, address, float.Parse(value)))
                        {
                            error = "Failed to modify target process memory";
                            return;
                        }
                        break;
                    case NullcTypeCategory.Double:
                        if (!DebugHelpers.TryWriteVariable(process, address, double.Parse(value)))
                        {
                            error = "Failed to modify target process memory";
                            return;
                        }
                        break;
                    default:
                        error = "Unknown value type";
                        return;
                }

                error = null;
            }

            DkmEvaluationResult EvaluateDataAtAddress(DkmInspectionContext inspectionContext, DkmStackWalkFrame stackFrame, string name, string fullName, NullcTypeInfo type, ulong address, DkmEvaluationResultFlags flags, DkmEvaluationResultAccessType access, DkmEvaluationResultStorageType storage)
            {
                var process = stackFrame.Process;

                var processData = DebugHelpers.GetOrCreateDataItem<NullcLocalProcessDataItem>(process);

                if (address == 0)
                    return DkmFailedEvaluationResult.Create(inspectionContext, stackFrame, name, fullName, "Null pointer address", DkmEvaluationResultFlags.Invalid, null);

                string editableValue;
                DkmDataAddress dataAddress;
                string value = EvaluateValueAtAddress(process, processData.bytecode, type, address, out editableValue, ref flags, out dataAddress);

                if (value == null)
                    return DkmFailedEvaluationResult.Create(inspectionContext, stackFrame, name, fullName, "Failed to read value", DkmEvaluationResultFlags.Invalid, null);

                DkmEvaluationResultCategory category = DkmEvaluationResultCategory.Data;
                DkmEvaluationResultTypeModifierFlags typeModifiers = DkmEvaluationResultTypeModifierFlags.None;

                NullEvaluationDataItem dataItem = new NullEvaluationDataItem();

                dataItem.address = address;
                dataItem.type = type;
                dataItem.fullName = fullName;

                return DkmSuccessEvaluationResult.Create(inspectionContext, stackFrame, name, fullName, flags, value, editableValue, type.name, category, access, storage, typeModifiers, dataAddress, null, null, dataItem);
            }

            bool PrepareContext(DkmProcess process, out string error)
            {
                var processData = DebugHelpers.GetOrCreateDataItem<NullcLocalProcessDataItem>(process);

                if (!processData.UpdateContextData(process))
                {
                    error = "Missing nullc context";
                    return false;
                }

                if (!processData.UpdateCallStack(process))
                {
                    error = "Missing nullc call stack";
                    return false;
                }

                error = null;
                return true;
            }

            NullcCallStackEntry GetStackFrameFunction(DkmProcess process, DkmStackWalkFrame stackFrame, out string error)
            {
                var processData = DebugHelpers.GetOrCreateDataItem<NullcLocalProcessDataItem>(process);

                NullcCallStackEntry activeEntry = null;

                if (stackFrame.Annotations != null)
                {
                    var actualFunction = processData.bytecode.GetFunctionAtNativeAddress(stackFrame.InstructionAddress.CPUInstructionPart.InstructionPointer);

                    var dataBase = stackFrame.Annotations.FirstOrDefault((x) => x.Id == DebugHelpers.NullcCallStackDataBaseGuid);

                    activeEntry = new NullcCallStackEntry();

                    activeEntry.function = actualFunction;
                    activeEntry.dataOffset = (int)dataBase.Value;

                    error = null;
                    return activeEntry;
                }

                // Best-effort based fallback: find the first matching function
                if (activeEntry == null)
                {
                    var function = processData.bytecode.GetFunctionAtNativeAddress(stackFrame.InstructionAddress.CPUInstructionPart.InstructionPointer);

                    activeEntry = processData.callStack.callStack.LastOrDefault((x) => x.function == function);
                }

                if (activeEntry == null)
                {
                    error = "Failed to match nullc frame";
                    return null;
                }

                error = null;
                return activeEntry;
            }

            void IDkmLanguageExpressionEvaluator.EvaluateExpression(DkmInspectionContext inspectionContext, DkmWorkList workList, DkmLanguageExpression expression, DkmStackWalkFrame stackFrame, DkmCompletionRoutine<DkmEvaluateExpressionAsyncResult> completionRoutine)
            {
                var process = stackFrame.Process;

                if (!PrepareContext(process, out string error))
                {
                    completionRoutine(new DkmEvaluateExpressionAsyncResult(DkmFailedEvaluationResult.Create(inspectionContext, stackFrame, expression.Text, expression.Text, error, DkmEvaluationResultFlags.Invalid, null)));
                    return;
                }

                NullcCallStackEntry activeEntry = GetStackFrameFunction(process, stackFrame, out error);

                if (activeEntry == null)
                {
                    completionRoutine(new DkmEvaluateExpressionAsyncResult(DkmFailedEvaluationResult.Create(inspectionContext, stackFrame, expression.Text, expression.Text, error, DkmEvaluationResultFlags.Invalid, null)));
                    return;
                }

                var processData = DebugHelpers.GetOrCreateDataItem<NullcLocalProcessDataItem>(process);

                string finalExpression = expression.Text;

                // Lets start by simple stand-alone variables

                // Since we don't handle composite expressions yet, we can at least clean-up access to class members (even if this can lead to a wrong result if member is shadowed)
                if (finalExpression.StartsWith("this."))
                    finalExpression = finalExpression.Substring(5);

                // TODO: DkmEvaluationResultCategory Method/Class
                // TODO: DkmEvaluationResultAccessType Internal
                // TODO: DkmEvaluationResultTypeModifierFlags Constant (enums, constants)

                // Check function locals and arguments
                // TODO: but which locals are still live at current point?
                if (activeEntry.function != null)
                {
                    foreach (var el in activeEntry.function.locals)
                    {
                        if (el.name == finalExpression)
                        {
                            var address = processData.dataStackBase + (ulong)(activeEntry.dataOffset + el.offset);

                            var result = EvaluateDataAtAddress(inspectionContext, stackFrame, el.name, el.name, el.nullcType, address, DkmEvaluationResultFlags.None, DkmEvaluationResultAccessType.None, DkmEvaluationResultStorageType.None);

                            completionRoutine(new DkmEvaluateExpressionAsyncResult(result));
                            return;

                        }
                    }

                    foreach (var el in activeEntry.function.arguments)
                    {
                        if (el.name == finalExpression)
                        {
                            var address = processData.dataStackBase + (ulong)(activeEntry.dataOffset + el.offset);

                            var result = EvaluateDataAtAddress(inspectionContext, stackFrame, el.name, el.name, el.nullcType, address, DkmEvaluationResultFlags.None, DkmEvaluationResultAccessType.None, DkmEvaluationResultStorageType.None);

                            completionRoutine(new DkmEvaluateExpressionAsyncResult(result));
                            return;
                        }
                    }

                    if (activeEntry.function.nullcParentType != null)
                    {
                        var thisArgument = activeEntry.function.arguments.FirstOrDefault((x) => x.name == "this");

                        if (thisArgument != null)
                        {
                            var thisaddress = processData.dataStackBase + (ulong)(activeEntry.dataOffset + thisArgument.offset);

                            var thisValue = DebugHelpers.ReadPointerVariable(process, thisaddress);

                            if (thisValue.HasValue)
                            {
                                var classType = thisArgument.nullcType.nullcSubType;

                                foreach (var el in classType.nullcMembers)
                                {
                                    if (el.name == finalExpression)
                                    {
                                        var address = thisValue.Value + el.offset;

                                        var result = EvaluateDataAtAddress(inspectionContext, stackFrame, el.name, $"this.{el.name}", el.nullcType, address, DkmEvaluationResultFlags.None, DkmEvaluationResultAccessType.None, DkmEvaluationResultStorageType.None);

                                        completionRoutine(new DkmEvaluateExpressionAsyncResult(result));
                                    }
                                }
                            }
                        }
                    }
                }

                // Check globals
                foreach (var el in processData.bytecode.variables)
                {
                    if (el.name == finalExpression)
                    {
                        var address = processData.dataStackBase + (ulong)(el.offset);

                        var result = EvaluateDataAtAddress(inspectionContext, stackFrame, el.name, el.name, el.nullcType, address, DkmEvaluationResultFlags.None, DkmEvaluationResultAccessType.Public, DkmEvaluationResultStorageType.Global);

                        completionRoutine(new DkmEvaluateExpressionAsyncResult(result));
                        return;
                    }
                }

                completionRoutine(new DkmEvaluateExpressionAsyncResult(DkmFailedEvaluationResult.Create(inspectionContext, stackFrame, expression.Text, expression.Text, "Failed to evaluate", DkmEvaluationResultFlags.Invalid, null)));
            }

            void IDkmLanguageExpressionEvaluator.GetChildren(DkmEvaluationResult result, DkmWorkList workList, int initialRequestSize, DkmInspectionContext inspectionContext, DkmCompletionRoutine<DkmGetChildrenAsyncResult> completionRoutine)
            {
                var evalData = result.GetDataItem<NullEvaluationDataItem>();

                // Shouldn't happen
                if (evalData == null)
                {
                    completionRoutine(new DkmGetChildrenAsyncResult(new DkmEvaluationResult[0], DkmEvaluationResultEnumContext.Create(0, result.StackFrame, inspectionContext, null)));
                    return;
                }

                var process = result.StackFrame.Process;
                var bytecode = DebugHelpers.GetOrCreateDataItem<NullcLocalProcessDataItem>(process).bytecode;

                if (evalData.type.subCat == NullTypeSubCategory.Pointer)
                {
                    int finalInitialSize = initialRequestSize < 1 ? initialRequestSize : 1;

                    DkmEvaluationResult[] initialResults = new DkmEvaluationResult[finalInitialSize];

                    if (initialRequestSize != 0)
                    {
                        var targetAddress = DebugHelpers.ReadPointerVariable(process, evalData.address);

                        if (!targetAddress.HasValue)
                        {
                            completionRoutine(new DkmGetChildrenAsyncResult(new DkmEvaluationResult[0], DkmEvaluationResultEnumContext.Create(0, result.StackFrame, inspectionContext, null)));
                            return;
                        }

                        initialResults[0] = EvaluateDataAtAddress(inspectionContext, result.StackFrame, "", $"*{result.FullName}", evalData.type.nullcSubType, targetAddress.Value, DkmEvaluationResultFlags.None, DkmEvaluationResultAccessType.Public, DkmEvaluationResultStorageType.None);
                    }

                    var enumerator = DkmEvaluationResultEnumContext.Create(1, result.StackFrame, inspectionContext, evalData);

                    completionRoutine(new DkmGetChildrenAsyncResult(initialResults, enumerator));
                    return;
                }

                if (evalData.type.subCat == NullTypeSubCategory.Class)
                {
                    int finalInitialSize = initialRequestSize < evalData.type.memberCount ? initialRequestSize : evalData.type.memberCount;

                    DkmEvaluationResult[] initialResults = new DkmEvaluationResult[finalInitialSize];

                    for (int i = 0; i < initialResults.Length; i++)
                    {
                        var member = evalData.type.nullcMembers[i];

                        ulong address = evalData.address + member.offset;

                        initialResults[i] = EvaluateDataAtAddress(inspectionContext, result.StackFrame, member.name, $"{result.FullName}.{member.name}", member.nullcType, address, DkmEvaluationResultFlags.None, DkmEvaluationResultAccessType.None, DkmEvaluationResultStorageType.None);
                    }

                    var enumerator = DkmEvaluationResultEnumContext.Create(evalData.type.memberCount, result.StackFrame, inspectionContext, evalData);

                    completionRoutine(new DkmGetChildrenAsyncResult(initialResults, enumerator));
                    return;
                }

                if (evalData.type.subCat == NullTypeSubCategory.Array)
                {
                    if (evalData.type.arrSize == -1)
                    {
                        ulong sizeAddress = evalData.address + (ulong)DebugHelpers.GetPointerSize(process);

                        var dataAddress = DebugHelpers.ReadPointerVariable(process, evalData.address);
                        var arrSize = DebugHelpers.ReadIntVariable(process, sizeAddress);

                        if (dataAddress.HasValue && arrSize.HasValue)
                        {
                            int finalInitialSize = initialRequestSize < arrSize.Value + 1 ? initialRequestSize : arrSize.Value + 1; // Extra slot for array size

                            DkmEvaluationResult[] initialResults = new DkmEvaluationResult[finalInitialSize];

                            if (finalInitialSize != 0)
                            {
                                var type = bytecode.types[(int)NullcTypeIndex.Int];

                                initialResults[0] = EvaluateDataAtAddress(inspectionContext, result.StackFrame, "size", $"{result.FullName}.size", type, sizeAddress, DkmEvaluationResultFlags.ReadOnly, DkmEvaluationResultAccessType.Internal, DkmEvaluationResultStorageType.None);
                            }

                            for (int i = 1; i < initialResults.Length; i++)
                            {
                                int index = i - 1;

                                var type = evalData.type.nullcSubType;

                                var elemAddress = dataAddress.Value + (ulong)(index * type.size);

                                initialResults[i] = EvaluateDataAtAddress(inspectionContext, result.StackFrame, $"[{index}]", $"{result.FullName}[{index}]", type, elemAddress, DkmEvaluationResultFlags.None, DkmEvaluationResultAccessType.None, DkmEvaluationResultStorageType.None);
                            }

                            var enumerator = DkmEvaluationResultEnumContext.Create(arrSize.Value + 1, result.StackFrame, inspectionContext, evalData);

                            completionRoutine(new DkmGetChildrenAsyncResult(initialResults, enumerator));
                            return;
                        }
                    }
                    else
                    {
                        int finalInitialSize = initialRequestSize < evalData.type.arrSize ? initialRequestSize : evalData.type.arrSize;

                        DkmEvaluationResult[] initialResults = new DkmEvaluationResult[finalInitialSize];

                        for (int i = 0; i < initialResults.Length; i++)
                        {
                            var type = evalData.type.nullcSubType;

                            var elemAddress = evalData.address + (ulong)(i * type.size);

                            initialResults[i] = EvaluateDataAtAddress(inspectionContext, result.StackFrame, $"[{i}]", $"{result.FullName}[{i}]", type, elemAddress, DkmEvaluationResultFlags.None, DkmEvaluationResultAccessType.None, DkmEvaluationResultStorageType.None);
                        }

                        var enumerator = DkmEvaluationResultEnumContext.Create(evalData.type.arrSize, result.StackFrame, inspectionContext, evalData);

                        completionRoutine(new DkmGetChildrenAsyncResult(initialResults, enumerator));
                        return;
                    }
                }

                // Shouldn't happen
                completionRoutine(new DkmGetChildrenAsyncResult(new DkmEvaluationResult[0], DkmEvaluationResultEnumContext.Create(0, result.StackFrame, inspectionContext, null)));
            }

            void IDkmLanguageExpressionEvaluator.GetFrameLocals(DkmInspectionContext inspectionContext, DkmWorkList workList, DkmStackWalkFrame stackFrame, DkmCompletionRoutine<DkmGetFrameLocalsAsyncResult> completionRoutine)
            {
                var process = stackFrame.Process;

                if (!PrepareContext(process, out string error))
                {
                    completionRoutine(new DkmGetFrameLocalsAsyncResult(DkmEvaluationResultEnumContext.Create(0, stackFrame, inspectionContext, null)));
                    return;
                }

                NullcCallStackEntry activeEntry = GetStackFrameFunction(process, stackFrame, out error);

                if (activeEntry == null)
                {
                    completionRoutine(new DkmGetFrameLocalsAsyncResult(DkmEvaluationResultEnumContext.Create(0, stackFrame, inspectionContext, null)));
                    return;
                }

                var processData = DebugHelpers.GetOrCreateDataItem<NullcLocalProcessDataItem>(process);

                var frameLocalsData = new NullFrameLocalsDataItem();

                frameLocalsData.activeEntry = activeEntry;

                if (activeEntry.function != null)
                {
                    var count = activeEntry.function.arguments.Count + activeEntry.function.locals.Count;

                    completionRoutine(new DkmGetFrameLocalsAsyncResult(DkmEvaluationResultEnumContext.Create(count, stackFrame, inspectionContext, frameLocalsData)));
                    return;
                }

                completionRoutine(new DkmGetFrameLocalsAsyncResult(DkmEvaluationResultEnumContext.Create(processData.bytecode.variables.Count, stackFrame, inspectionContext, frameLocalsData)));
            }

            void IDkmLanguageExpressionEvaluator.GetFrameArguments(DkmInspectionContext inspectionContext, DkmWorkList workList, DkmStackWalkFrame stackFrame, DkmCompletionRoutine<DkmGetFrameArgumentsAsyncResult> completionRoutine)
            {
                var process = stackFrame.Process;

                if (!PrepareContext(process, out string error))
                {
                    completionRoutine(new DkmGetFrameArgumentsAsyncResult(new DkmEvaluationResult[0]));
                    return;
                }

                NullcCallStackEntry activeEntry = GetStackFrameFunction(process, stackFrame, out error);

                if (activeEntry == null)
                {
                    completionRoutine(new DkmGetFrameArgumentsAsyncResult(new DkmEvaluationResult[0]));
                    return;
                }

                var processData = DebugHelpers.GetOrCreateDataItem<NullcLocalProcessDataItem>(process);

                var frameLocalsData = new NullFrameLocalsDataItem();

                frameLocalsData.activeEntry = activeEntry;

                if (activeEntry.function != null)
                {
                    var results = new DkmEvaluationResult[activeEntry.function.arguments.Count];

                    for (int i = 0; i < results.Length; i++)
                    {
                        var el = activeEntry.function.arguments[i];

                        var address = processData.dataStackBase + (ulong)(frameLocalsData.activeEntry.dataOffset + el.offset);

                        results[i] = EvaluateDataAtAddress(inspectionContext, stackFrame, el.name, el.name, el.nullcType, address, DkmEvaluationResultFlags.None, DkmEvaluationResultAccessType.None, DkmEvaluationResultStorageType.None);
                    }

                    completionRoutine(new DkmGetFrameArgumentsAsyncResult(results));
                    return;
                }

                completionRoutine(new DkmGetFrameArgumentsAsyncResult(new DkmEvaluationResult[0]));
            }

            void IDkmLanguageExpressionEvaluator.GetItems(DkmEvaluationResultEnumContext enumContext, DkmWorkList workList, int startIndex, int count, DkmCompletionRoutine<DkmEvaluationEnumAsyncResult> completionRoutine)
            {
                var process = enumContext.StackFrame.Process;

                var processData = DebugHelpers.GetOrCreateDataItem<NullcLocalProcessDataItem>(process);

                var bytecode = processData.bytecode;

                var frameLocalsData = enumContext.GetDataItem<NullFrameLocalsDataItem>();

                if (frameLocalsData != null)
                {
                    var function = frameLocalsData.activeEntry.function;

                    if (function != null)
                    {
                        // Visual Studio doesn't respect enumeration size for GetFrameLocals, so we need to limit it back
                        var actualCount = function.arguments.Count + function.locals.Count;

                        int finalCount = actualCount - startIndex;

                        finalCount = finalCount < 0 ? 0 : (finalCount < count ? finalCount : count);

                        var results = new DkmEvaluationResult[finalCount];

                        for (int i = startIndex; i < startIndex + finalCount; i++)
                        {
                            if (i < function.arguments.Count)
                            {
                                var el = function.arguments[i];

                                var address = processData.dataStackBase + (ulong)(frameLocalsData.activeEntry.dataOffset + el.offset);

                                results[i - startIndex] = EvaluateDataAtAddress(enumContext.InspectionContext, enumContext.StackFrame, el.name, el.name, el.nullcType, address, DkmEvaluationResultFlags.None, DkmEvaluationResultAccessType.None, DkmEvaluationResultStorageType.None);
                            }
                            else
                            {
                                var el = function.locals[i - function.arguments.Count];

                                var address = processData.dataStackBase + (ulong)(frameLocalsData.activeEntry.dataOffset + el.offset);

                                results[i - startIndex] = EvaluateDataAtAddress(enumContext.InspectionContext, enumContext.StackFrame, el.name, el.name, el.nullcType, address, DkmEvaluationResultFlags.None, DkmEvaluationResultAccessType.None, DkmEvaluationResultStorageType.None);
                            }
                        }

                        completionRoutine(new DkmEvaluationEnumAsyncResult(results));
                    }
                    else
                    {
                        // Visual Studio doesn't respect enumeration size for GetFrameLocals, so we need to limit it back
                        var actualCount = bytecode.variables.Count;

                        int finalCount = actualCount - startIndex;

                        finalCount = finalCount < 0 ? 0 : (finalCount < count ? finalCount : count);

                        var results = new DkmEvaluationResult[finalCount];

                        for (int i = startIndex; i < startIndex + finalCount; i++)
                        {
                            var el = bytecode.variables[i];

                            var address = processData.dataStackBase + (ulong)(frameLocalsData.activeEntry.dataOffset + el.offset);

                            results[i - startIndex] = EvaluateDataAtAddress(enumContext.InspectionContext, enumContext.StackFrame, el.name, el.name, el.nullcType, address, DkmEvaluationResultFlags.None, DkmEvaluationResultAccessType.None, DkmEvaluationResultStorageType.Global);

                        }

                        completionRoutine(new DkmEvaluationEnumAsyncResult(results));
                    }

                    return;
                }

                var evalData = enumContext.GetDataItem<NullEvaluationDataItem>();

                // Shouldn't happen
                if (evalData == null)
                {
                    completionRoutine(new DkmEvaluationEnumAsyncResult(new DkmEvaluationResult[0]));
                    return;
                }

                if (evalData.type.subCat == NullTypeSubCategory.Pointer)
                {
                    if (startIndex != 0)
                    {
                        completionRoutine(new DkmEvaluationEnumAsyncResult());
                        return;
                    }

                    var results = new DkmEvaluationResult[1];

                    var targetAddress = DebugHelpers.ReadPointerVariable(process, evalData.address);

                    if (!targetAddress.HasValue)
                    {
                        completionRoutine(new DkmEvaluationEnumAsyncResult(new DkmEvaluationResult[0]));
                        return;

                    }
                    results[0] = EvaluateDataAtAddress(enumContext.InspectionContext, enumContext.StackFrame, "", $"*{evalData.fullName}", evalData.type.nullcSubType, targetAddress.Value, DkmEvaluationResultFlags.None, DkmEvaluationResultAccessType.Public, DkmEvaluationResultStorageType.None);

                    completionRoutine(new DkmEvaluationEnumAsyncResult(results));
                    return;
                }

                if (evalData.type.subCat == NullTypeSubCategory.Class)
                {
                    var results = new DkmEvaluationResult[count];

                    for (int i = startIndex; i < startIndex + count; i++)
                    {
                        var member = evalData.type.nullcMembers[i];

                        ulong address = evalData.address + member.offset;

                        results[i - startIndex] = EvaluateDataAtAddress(enumContext.InspectionContext, enumContext.StackFrame, member.name, $"{evalData.fullName}.{member.name}", member.nullcType, address, DkmEvaluationResultFlags.None, DkmEvaluationResultAccessType.None, DkmEvaluationResultStorageType.None);
                    }

                    completionRoutine(new DkmEvaluationEnumAsyncResult(results));
                    return;
                }

                if (evalData.type.subCat == NullTypeSubCategory.Array)
                {
                    if (evalData.type.arrSize == -1)
                    {
                        ulong sizeAddress = evalData.address + (ulong)DebugHelpers.GetPointerSize(process);

                        var dataAddress = DebugHelpers.ReadPointerVariable(process, evalData.address);
                        var arrSize = DebugHelpers.ReadIntVariable(process, sizeAddress);

                        if (dataAddress.HasValue && arrSize.HasValue)
                        {
                            var results = new DkmEvaluationResult[count];

                            for (int i = startIndex; i < startIndex + count; i++)
                            {
                                if (i == 0)
                                {
                                    var type = bytecode.types[(int)NullcTypeIndex.Int];

                                    results[i - startIndex] = EvaluateDataAtAddress(enumContext.InspectionContext, enumContext.StackFrame, "size", $"{evalData.fullName}.size", type, sizeAddress, DkmEvaluationResultFlags.ReadOnly, DkmEvaluationResultAccessType.Internal, DkmEvaluationResultStorageType.None);
                                }
                                else
                                {
                                    int index = i - 1;

                                    var type = evalData.type.nullcSubType;

                                    var elemAddress = dataAddress.Value + (ulong)(index * type.size);

                                    results[i - startIndex] = EvaluateDataAtAddress(enumContext.InspectionContext, enumContext.StackFrame, $"[{index}]", $"{evalData.fullName}[{index}]", type, elemAddress, DkmEvaluationResultFlags.None, DkmEvaluationResultAccessType.None, DkmEvaluationResultStorageType.None);
                                }
                            }

                            completionRoutine(new DkmEvaluationEnumAsyncResult(results));
                            return;
                        }
                    }
                    else
                    {
                        var results = new DkmEvaluationResult[count];

                        for (int i = startIndex; i < startIndex + count; i++)
                        {
                            var type = evalData.type.nullcSubType;

                            var elemAddress = evalData.address + (ulong)(i * type.size);

                            results[i - startIndex] = EvaluateDataAtAddress(enumContext.InspectionContext, enumContext.StackFrame, $"[{i}]", $"{evalData.fullName}[{i}]", type, elemAddress, DkmEvaluationResultFlags.None, DkmEvaluationResultAccessType.None, DkmEvaluationResultStorageType.None);
                        }

                        completionRoutine(new DkmEvaluationEnumAsyncResult(results));
                        return;
                    }
                }

                completionRoutine(new DkmEvaluationEnumAsyncResult(new DkmEvaluationResult[0]));
            }

            void IDkmLanguageExpressionEvaluator.SetValueAsString(DkmEvaluationResult result, string value, int timeout, out string errorText)
            {
                var evalData = result.GetDataItem<NullEvaluationDataItem>();

                // Shouldn't happen
                if (evalData == null)
                {
                    errorText = "Missing evaluation data";
                    return;
                }

                var process = result.StackFrame.Process;
                var bytecode = DebugHelpers.GetOrCreateDataItem<NullcLocalProcessDataItem>(process).bytecode;

                try
                {
                    SetValueAtAddress(process, bytecode, evalData.type, evalData.address, value, out errorText);
                }
                catch (Exception e)
                {
                    errorText = e.Message;
                }
            }

            string IDkmLanguageExpressionEvaluator.GetUnderlyingString(DkmEvaluationResult result)
            {
                // RawString is not used at the moment
                return "";
            }
        }
    }
}
