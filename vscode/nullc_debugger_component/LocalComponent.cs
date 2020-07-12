using Microsoft.VisualStudio.Debugger;
using Microsoft.VisualStudio.Debugger.CallStack;
using Microsoft.VisualStudio.Debugger.ComponentInterfaces;
using Microsoft.VisualStudio.Debugger.CustomRuntimes;
using Microsoft.VisualStudio.Debugger.Evaluation;
using Microsoft.VisualStudio.Debugger.FunctionResolution;
using Microsoft.VisualStudio.Debugger.Native;
using Microsoft.VisualStudio.Debugger.Symbols;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;

namespace nullc_debugger_component
{
    namespace DkmDebugger
    {
        internal class NullcJitContextData
        {
            public ulong location = 0;

            public ulong dataStackBase = 0;
            public ulong dataStackTop = 0;

            public ulong callStackBase = 0;
            public ulong callStackTop = 0;
        }

        internal class NullcVmContextData
        {
            public ulong location = 0;

            public ulong dataStackBase = 0;
            public int dataStackCount = 0;

            public ulong codeBase = 0;

            public ulong callStackBase = 0;
            public int callStackCount = 0;
        }

        internal class NullcLocalProcessDataItem : DkmDataItem
        {
            public ulong moduleBytecodeLocation = 0;
            public ulong moduleBytecodeSize = 0;
            public byte[] moduleBytecodeRaw;

            public NullcBytecode bytecode;

            public NullcJitContextData jitContext = new NullcJitContextData();
            public NullcVmContextData vmContext = new NullcVmContextData();

            public ulong dataStackBase = 0;

            public NullcCallStack callStack = new NullcCallStack();

            public List<string> activeDocumentPaths = new List<string>();

            public void UpdateContextData(DkmProcess process)
            {
                jitContext.location = DebugHelpers.ReadPointerVariable(process, "nullcJitContextMainDataAddress").GetValueOrDefault(0);

                if (jitContext.location != 0)
                {
                    jitContext.dataStackBase = DebugHelpers.ReadPointerVariable(process, jitContext.location + (ulong)DebugHelpers.GetPointerSize(process) * 0).GetValueOrDefault(0);
                    jitContext.dataStackTop = DebugHelpers.ReadPointerVariable(process, jitContext.location + (ulong)DebugHelpers.GetPointerSize(process) * 1).GetValueOrDefault(0);

                    jitContext.callStackBase = DebugHelpers.ReadPointerVariable(process, jitContext.location + (ulong)DebugHelpers.GetPointerSize(process) * 3).GetValueOrDefault(0);
                    jitContext.callStackTop = DebugHelpers.ReadPointerVariable(process, jitContext.location + (ulong)DebugHelpers.GetPointerSize(process) * 4).GetValueOrDefault(0);
                }

                vmContext.location = DebugHelpers.ReadPointerVariable(process, "nullcVmContextMainDataAddress").GetValueOrDefault(0);

                if (vmContext.location != 0)
                {
                    vmContext.dataStackBase = DebugHelpers.ReadPointerVariable(process, vmContext.location).GetValueOrDefault(0);
                    vmContext.dataStackCount = DebugHelpers.ReadIntVariable(process, vmContext.location + (ulong)DebugHelpers.GetPointerSize(process)).GetValueOrDefault(0);

                    vmContext.codeBase = DebugHelpers.ReadPointerVariable(process, vmContext.location + (ulong)DebugHelpers.GetPointerSize(process) + 8).GetValueOrDefault(0);

                    vmContext.callStackBase = DebugHelpers.ReadPointerVariable(process, vmContext.location + (ulong)DebugHelpers.GetPointerSize(process) * 2 + 8).GetValueOrDefault(0);
                    vmContext.callStackCount = DebugHelpers.ReadIntVariable(process, vmContext.location + (ulong)DebugHelpers.GetPointerSize(process) * 3 + 8).GetValueOrDefault(0);
                }
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
            public bool isVmModule;

            public int moduleIndex;
        }

        public class NullcLocalComponent : IDkmSymbolCompilerIdQuery, IDkmSymbolDocumentCollectionQuery, IDkmSymbolDocumentSpanQuery, IDkmSymbolQuery, IDkmSymbolFunctionResolver, IDkmLanguageFrameDecoder, IDkmModuleInstanceLoadNotification, IDkmLanguageExpressionEvaluator, IDkmLanguageInstructionDecoder, IDkmCustomMessageCallbackReceiver
        {
            DkmCompilerId IDkmSymbolCompilerIdQuery.GetCompilerId(DkmInstructionSymbol instruction, DkmInspectionSession inspectionSession)
            {
                if (instruction.Module.Name != "nullc.embedded.code")
                    return new DkmCompilerId(Guid.Empty, Guid.Empty);

                return new DkmCompilerId(Guid.Empty, Guid.Empty);
            }

            DkmResolvedDocument[] IDkmSymbolDocumentCollectionQuery.FindDocuments(DkmModule module, DkmSourceFileId sourceFileId)
            {
                bool isVmModule = module.Name == "nullc.vm.code";

                DkmModuleInstance nullcModuleInstance;

                if (isVmModule && DebugHelpers.checkVmModuleDocuments)
                {
                    nullcModuleInstance = module.GetModuleInstances().OfType<DkmCustomModuleInstance>().FirstOrDefault(el => el.Module.CompilerId.VendorId == DebugHelpers.NullcCompilerGuid);
                }
                else
                {
                    if (DebugHelpers.useNativeInterfaces)
                        nullcModuleInstance = module.GetModuleInstances().OfType<DkmNativeModuleInstance>().FirstOrDefault(el => el.Module.CompilerId.VendorId == DebugHelpers.NullcCompilerGuid);
                    else
                        nullcModuleInstance = module.GetModuleInstances().OfType<DkmCustomModuleInstance>().FirstOrDefault(el => el.Module.CompilerId.VendorId == DebugHelpers.NullcCompilerGuid);
                }

                if (nullcModuleInstance == null)
                    return module.FindDocuments(sourceFileId);

                var processData = DebugHelpers.GetOrCreateDataItem<NullcLocalProcessDataItem>(nullcModuleInstance.Process);

                if (processData.bytecode != null)
                {
                    var processPath = nullcModuleInstance.Process.Path;

                    var dataItem = new NullResolvedDocumentDataItem();

                    dataItem.bytecode = processData.bytecode;
                    dataItem.moduleBase = nullcModuleInstance.BaseAddress;
                    dataItem.isVmModule = isVmModule;

                    DkmResolvedDocument[] MatchAgainstModule(string moduleName, int moduleIndex)
                    {
                        foreach (var importPath in processData.bytecode.importPaths)
                        {
                            var finalPath = importPath.Replace('/', '\\');

                            if (finalPath.Length == 0)
                                finalPath = $"{Path.GetDirectoryName(processPath)}\\";
                            else if (!Path.IsPathRooted(finalPath))
                                finalPath = Path.GetFullPath(Path.Combine(Path.GetDirectoryName(processPath), finalPath));

                            var modulePath = moduleName.Replace('/', '\\');

                            var combined = $"{finalPath}{modulePath}";

                            if (combined == sourceFileId.DocumentName)
                            {
                                dataItem.moduleIndex = moduleIndex;

                                processData.activeDocumentPaths.Add(sourceFileId.DocumentName);

                                return new DkmResolvedDocument[1] { DkmResolvedDocument.Create(module, sourceFileId.DocumentName, null, DkmDocumentMatchStrength.FullPath, DkmResolvedDocumentWarning.None, false, dataItem) };
                            }

                            if (combined.ToLowerInvariant().EndsWith(sourceFileId.DocumentName.ToLowerInvariant()))
                            {
                                if (File.Exists(combined))
                                {
                                    dataItem.moduleIndex = moduleIndex;

                                    processData.activeDocumentPaths.Add(combined);

                                    return new DkmResolvedDocument[1] { DkmResolvedDocument.Create(module, sourceFileId.DocumentName, null, DkmDocumentMatchStrength.SubPath, DkmResolvedDocumentWarning.None, false, dataItem) };
                                }
                            }
                        }

                        return null;
                    }

                    foreach (var nullcModule in processData.bytecode.modules)
                    {
                        int moduleIndex = processData.bytecode.modules.IndexOf(nullcModule);

                        var result = MatchAgainstModule(nullcModule.name, moduleIndex);

                        if (result != null)
                            return result;
                    }

                    {
                        var result = MatchAgainstModule(processData.bytecode.mainModuleName, -1);

                        if (result != null)
                            return result;
                    }

                    Debug.WriteLine($"Failed to find nullc document using '{sourceFileId.DocumentName}' name");
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
                    {
                        continue;
                    }

                    int instruction = documentData.bytecode.ConvertLineToInstruction(moduleSourceLocation, line - 1);

                    if (instruction == 0)
                    {
                        continue;
                    }

                    var sourceFileId = DkmSourceFileId.Create(resolvedDocument.DocumentName, null, null, null);

                    var resultSpan = new DkmTextSpan(line, line, 0, 0);

                    symbolLocation = new DkmSourcePosition[1] { DkmSourcePosition.Create(sourceFileId, resultSpan) };

                    if (documentData.isVmModule)
                    {
                        return new DkmInstructionSymbol[1] { DkmCustomInstructionSymbol.Create(resolvedDocument.Module, DebugHelpers.NullcRuntimeGuid, null, (ulong)instruction, null) };
                    }
                    else
                    {
                        ulong nativeInstruction = documentData.bytecode.ConvertInstructionToNativeAddress(instruction);

                        Debug.Assert(nativeInstruction >= documentData.moduleBase);

                        if (DebugHelpers.useNativeInterfaces)
                        {
                            return new DkmInstructionSymbol[1] { DkmNativeInstructionSymbol.Create(resolvedDocument.Module, (uint)(nativeInstruction - documentData.moduleBase)) };
                        }
                        else
                        {
                            return new DkmInstructionSymbol[1] { DkmCustomInstructionSymbol.Create(resolvedDocument.Module, DebugHelpers.NullcRuntimeGuid, null, nativeInstruction, null) };
                        }
                    }
                }

                return resolvedDocument.FindSymbols(textSpan, text, out symbolLocation);
            }

            string TryFindModuleFilePath(NullcLocalProcessDataItem processData, string processPath, string moduleName)
            {
                foreach (var importPath in processData.bytecode.importPaths)
                {
                    var finalPath = importPath.Replace('/', '\\');

                    if (finalPath.Length == 0)
                    {
                        finalPath = $"{Path.GetDirectoryName(processPath)}\\";
                    }
                    else if (!Path.IsPathRooted(finalPath))
                    {
                        finalPath = Path.GetFullPath(Path.Combine(Path.GetDirectoryName(processPath), finalPath));
                    }

                    var modulePath = moduleName.Replace('/', '\\');

                    var combined = $"{finalPath}{modulePath}";

                    foreach (var activeDocumentPath in processData.activeDocumentPaths)
                    {
                        if (combined == activeDocumentPath)
                            return combined;
                    }

                    if (File.Exists(combined))
                    {
                        processData.activeDocumentPaths.Add(combined);

                        return combined;
                    }
                }

                return null;
            }

            DkmSourcePosition GetSourcePosition(NullcLocalProcessDataItem processData, string processPath, int nullcInstruction, out bool startOfLine)
            {
                int sourceLocation = processData.bytecode.GetInstructionSourceLocation(nullcInstruction);

                int moduleIndex = processData.bytecode.GetSourceLocationModuleIndex(sourceLocation);

                int column = 0;
                int line = processData.bytecode.GetSourceLocationLineAndColumn(sourceLocation, moduleIndex, out column);

                string moduleName = moduleIndex != -1 ? processData.bytecode.modules[moduleIndex].name : processData.bytecode.mainModuleName;

                string path = TryFindModuleFilePath(processData, processPath, moduleName);

                // Let Visual Studio find it using a partial name
                if (path == null)
                    path = moduleName;

                startOfLine = true;
                return DkmSourcePosition.Create(DkmSourceFileId.Create(path, null, null, null), new DkmTextSpan(line, line, 0, 0));
            }

            DkmSourcePosition IDkmSymbolQuery.GetSourcePosition(DkmInstructionSymbol instruction, DkmSourcePositionFlags flags, DkmInspectionSession inspectionSession, out bool startOfLine)
            {
                if (instruction.RuntimeType == DebugHelpers.NullcVmRuntimeGuid)
                {
                    DkmCustomModuleInstance vmModuleInstance = instruction.Module.GetModuleInstances().OfType<DkmCustomModuleInstance>().FirstOrDefault(el => el.Module.CompilerId.VendorId == DebugHelpers.NullcCompilerGuid);

                    if (vmModuleInstance == null)
                        return instruction.GetSourcePosition(flags, inspectionSession, out startOfLine);

                    var vmProcessData = DebugHelpers.GetOrCreateDataItem<NullcLocalProcessDataItem>(vmModuleInstance.Process);

                    if (vmProcessData.bytecode != null)
                    {
                        var instructionSymbol = instruction as DkmCustomInstructionSymbol;

                        int nullcInstruction = (int)instructionSymbol.Offset;

                        if (nullcInstruction != 0)
                        {
                            return GetSourcePosition(vmProcessData, vmModuleInstance.Process.Path, nullcInstruction, out startOfLine);
                        }
                    }

                    return instruction.GetSourcePosition(flags, inspectionSession, out startOfLine);
                }

                DkmModuleInstance nullcModuleInstance;

                if (DebugHelpers.useNativeInterfaces)
                    nullcModuleInstance = instruction.Module.GetModuleInstances().OfType<DkmNativeModuleInstance>().FirstOrDefault(el => el.Module.CompilerId.VendorId == DebugHelpers.NullcCompilerGuid);
                else
                    nullcModuleInstance = instruction.Module.GetModuleInstances().OfType<DkmCustomModuleInstance>().FirstOrDefault(el => el.Module.CompilerId.VendorId == DebugHelpers.NullcCompilerGuid);

                if (nullcModuleInstance == null)
                    return instruction.GetSourcePosition(flags, inspectionSession, out startOfLine);

                var processData = DebugHelpers.GetOrCreateDataItem<NullcLocalProcessDataItem>(nullcModuleInstance.Process);

                if (processData.bytecode != null)
                {
                    int nullcInstruction = 0;

                    if (DebugHelpers.useNativeInterfaces)
                    {
                        var instructionSymbol = instruction as DkmNativeInstructionSymbol;

                        if (instructionSymbol != null)
                        {
                            nullcInstruction = processData.bytecode.ConvertNativeAddressToInstruction(nullcModuleInstance.BaseAddress + instructionSymbol.RVA);
                        }
                    }
                    else
                    {
                        var instructionSymbol = instruction as DkmCustomInstructionSymbol;

                        if (instructionSymbol != null)
                        {
                            nullcInstruction = processData.bytecode.ConvertNativeAddressToInstruction(instructionSymbol.Offset);
                        }
                    }

                    if (nullcInstruction != 0)
                        return GetSourcePosition(processData, nullcModuleInstance.Process.Path, nullcInstruction, out startOfLine);
                }

                return instruction.GetSourcePosition(flags, inspectionSession, out startOfLine);
            }

            object IDkmSymbolQuery.GetSymbolInterface(DkmModule module, Guid interfaceID)
            {
                if (module.Name != "nullc.embedded.code")
                    throw new NotImplementedException();

                throw new NotImplementedException();
            }

            string FormatFunction(NullcFuncInfo function, DkmVariableInfoFlags argumentFlags)
            {
                string result = $"{function.name}";

                if (argumentFlags.HasFlag(DkmVariableInfoFlags.Types) || argumentFlags.HasFlag(DkmVariableInfoFlags.Names))
                {
                    result += "(";

                    for (int i = 0; i < function.paramCount; i++)
                    {
                        var localInfo = function.arguments[i];

                        if (argumentFlags.HasFlag(DkmVariableInfoFlags.Types))
                        {
                            if (i != 0)
                                result += ", ";

                            result += $"{localInfo.nullcType.name}";

                            if (argumentFlags.HasFlag(DkmVariableInfoFlags.Names))
                                result += $" {localInfo.name}";
                        }
                        else if (argumentFlags.HasFlag(DkmVariableInfoFlags.Names))
                        {
                            if (i != 0)
                                result += ", ";

                            result += $"{localInfo.name}";
                        }
                    }

                    result += ")";
                }

                return result;
            }

            void IDkmLanguageFrameDecoder.GetFrameName(DkmInspectionContext inspectionContext, DkmWorkList workList, DkmStackWalkFrame frame, DkmVariableInfoFlags argumentFlags, DkmCompletionRoutine<DkmGetFrameNameAsyncResult> completionRoutine)
            {
                var process = frame.Process;

                var processData = DebugHelpers.GetOrCreateDataItem<NullcLocalProcessDataItem>(process);

                NullcFuncInfo function;

                if (inspectionContext.RuntimeInstance.Id.RuntimeType == DebugHelpers.NullcVmRuntimeGuid)
                    function = processData.bytecode.GetFunctionAtAddress((int)(frame.InstructionAddress as DkmCustomInstructionAddress).Offset);
                else
                    function = processData.bytecode.GetFunctionAtNativeAddress(frame.InstructionAddress.CPUInstructionPart.InstructionPointer);

                if (function != null)
                {
                    completionRoutine(new DkmGetFrameNameAsyncResult(FormatFunction(function, argumentFlags)));
                    return;
                }

                int nullcInstruction;

                if (inspectionContext.RuntimeInstance.Id.RuntimeType == DebugHelpers.NullcVmRuntimeGuid)
                    nullcInstruction = (int)(frame.InstructionAddress as DkmCustomInstructionAddress).Offset;
                else
                    nullcInstruction = processData.bytecode.ConvertNativeAddressToInstruction(frame.InstructionAddress.CPUInstructionPart.InstructionPointer);

                if (nullcInstruction != 0)
                {
                    completionRoutine(new DkmGetFrameNameAsyncResult("nullcGlobal()"));
                    return;
                }

                inspectionContext.GetFrameName(workList, frame, argumentFlags, completionRoutine);
            }

            void IDkmLanguageFrameDecoder.GetFrameReturnType(DkmInspectionContext inspectionContext, DkmWorkList workList, DkmStackWalkFrame frame, DkmCompletionRoutine<DkmGetFrameReturnTypeAsyncResult> completionRoutine)
            {
                // Not provided at the moment
                inspectionContext.GetFrameReturnType(workList, frame, completionRoutine);
            }

            string IDkmLanguageInstructionDecoder.GetMethodName(DkmLanguageInstructionAddress languageInstructionAddress, DkmVariableInfoFlags argumentFlags)
            {
                var processData = DebugHelpers.GetOrCreateDataItem<NullcLocalProcessDataItem>(languageInstructionAddress.Address.Process);

                NullcFuncInfo function;

                if (languageInstructionAddress.RuntimeInstance.Id.RuntimeType == DebugHelpers.NullcVmRuntimeGuid)
                    function = processData.bytecode.GetFunctionAtAddress((int)(languageInstructionAddress.Address as DkmCustomInstructionAddress).Offset);
                else
                    function = processData.bytecode.GetFunctionAtNativeAddress(languageInstructionAddress.Address.CPUInstructionPart.InstructionPointer);

                if (function != null)
                    return FormatFunction(function, argumentFlags);

                int nullcInstruction;

                if (languageInstructionAddress.RuntimeInstance.Id.RuntimeType == DebugHelpers.NullcVmRuntimeGuid)
                    nullcInstruction = (int)(languageInstructionAddress.Address as DkmCustomInstructionAddress).Offset;
                else
                    nullcInstruction = processData.bytecode.ConvertNativeAddressToInstruction(languageInstructionAddress.Address.CPUInstructionPart.InstructionPointer);

                if (nullcInstruction != 0)
                {
                    if (argumentFlags.HasFlag(DkmVariableInfoFlags.Types) || argumentFlags.HasFlag(DkmVariableInfoFlags.Names))
                        return "nullcGlobal()";

                    return "nullcGlobal";
                }

                return languageInstructionAddress.GetMethodName(argumentFlags);
            }

            void UpdateModuleBytecode(DkmProcess process)
            {
                var processData = DebugHelpers.GetOrCreateDataItem<NullcLocalProcessDataItem>(process);

                processData.moduleBytecodeLocation = DebugHelpers.ReadPointerVariable(process, "nullcModuleBytecodeLocation").GetValueOrDefault(0);
                processData.moduleBytecodeSize = DebugHelpers.ReadPointerVariable(process, "nullcModuleBytecodeSize").GetValueOrDefault(0);

                if (processData.moduleBytecodeLocation != 0)
                {
                    processData.moduleBytecodeRaw = new byte[processData.moduleBytecodeSize];
                    process.ReadMemory(processData.moduleBytecodeLocation, DkmReadMemoryFlags.None, processData.moduleBytecodeRaw);

                    processData.bytecode = new NullcBytecode();
                    processData.bytecode.ReadFrom(processData.moduleBytecodeRaw, DebugHelpers.Is64Bit(process));
                }
            }

            void IDkmModuleInstanceLoadNotification.OnModuleInstanceLoad(DkmModuleInstance moduleInstance, DkmWorkList workList, DkmEventDescriptorS eventDescriptor)
            {
                var process = moduleInstance.Process;

                var processData = DebugHelpers.GetOrCreateDataItem<NullcLocalProcessDataItem>(process);

                if (processData.moduleBytecodeLocation == 0)
                {
                    UpdateModuleBytecode(process);
                }
            }

            DkmCustomMessage IDkmCustomMessageCallbackReceiver.SendHigher(DkmCustomMessage customMessage)
            {
                var processData = DebugHelpers.GetOrCreateDataItem<NullcLocalProcessDataItem>(customMessage.Process);

                if (customMessage.SourceId == DebugHelpers.NullcReloadSymbolsMessageGuid && customMessage.MessageCode == 1)
                {
                    UpdateModuleBytecode(customMessage.Process);
                }

                return null;
            }

            string EvaluateValueAtAddress(DkmProcess process, NullcBytecode bytecode, NullcTypeInfo type, ulong address, out string editableValue, ref DkmEvaluationResultFlags flags, out DkmDataAddress dataAddress)
            {
                editableValue = null;
                dataAddress = null;

                if (type.subCat == NullcTypeSubCategory.Pointer)
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

                if (type.subCat == NullcTypeSubCategory.Class)
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

                if (type.subCat == NullcTypeSubCategory.Array)
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

                if (type.subCat == NullcTypeSubCategory.Function)
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
                if (type.subCat == NullcTypeSubCategory.Pointer)
                {
                    error = "Can't modify pointer value";
                    return;
                }

                if (type.subCat == NullcTypeSubCategory.Class)
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

                if (type.subCat == NullcTypeSubCategory.Array)
                {
                    error = "Can't modify array value";
                    return;
                }

                if (type.subCat == NullcTypeSubCategory.Function)
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

            bool PrepareContext(DkmProcess process, Guid RuntimeInstanceId, out string error)
            {
                var processData = DebugHelpers.GetOrCreateDataItem<NullcLocalProcessDataItem>(process);

                if (processData.bytecode == null)
                {
                    error = "Missing nullc bytecode";
                    return false;
                }

                processData.UpdateContextData(process);

                if (RuntimeInstanceId == DebugHelpers.NullcVmRuntimeGuid)
                {
                    if (processData.vmContext.dataStackBase == 0)
                    {
                        error = "Missing nullc context";
                        return false;
                    }

                    processData.dataStackBase = processData.vmContext.dataStackBase;

                    // VM call stack is currently not required in the debug component
                }
                else
                {
                    if (processData.jitContext.dataStackBase == 0)
                    {
                        error = "Missing nullc context";
                        return false;
                    }

                    processData.dataStackBase = processData.jitContext.dataStackBase;

                    processData.callStack.UpdateFrom(process, processData.jitContext.callStackBase, processData.jitContext.callStackTop, processData.bytecode);
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
                    NullcFuncInfo actualFunction;

                    if (stackFrame.InstructionAddress.RuntimeInstance.Id.RuntimeType == DebugHelpers.NullcVmRuntimeGuid)
                        actualFunction = processData.bytecode.GetFunctionAtAddress((int)(stackFrame.InstructionAddress as DkmCustomInstructionAddress).Offset);
                    else
                        actualFunction = processData.bytecode.GetFunctionAtNativeAddress(stackFrame.InstructionAddress.CPUInstructionPart.InstructionPointer);

                    var dataBase = stackFrame.Annotations.FirstOrDefault((x) => x.Id == DebugHelpers.NullcCallStackDataBaseGuid);

                    activeEntry = new NullcCallStackEntry();

                    activeEntry.function = actualFunction;
                    activeEntry.dataOffset = (int)dataBase.Value;

                    error = null;
                    return activeEntry;
                }

                // Best-effort based fallback: find the first matching function
                if (activeEntry == null && stackFrame.InstructionAddress.RuntimeInstance.Id.RuntimeType != DebugHelpers.NullcVmRuntimeGuid)
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

                if (!PrepareContext(process, inspectionContext.RuntimeInstance.Id.RuntimeType, out string error))
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

                if (evalData.type.subCat == NullcTypeSubCategory.Pointer)
                {
                    int finalInitialSize = initialRequestSize < 1 ? initialRequestSize : 1;

                    DkmEvaluationResult[] initialResults = new DkmEvaluationResult[finalInitialSize];

                    var targetAddress = DebugHelpers.ReadPointerVariable(process, evalData.address);

                    if (!targetAddress.HasValue)
                    {
                        completionRoutine(new DkmGetChildrenAsyncResult(new DkmEvaluationResult[0], DkmEvaluationResultEnumContext.Create(0, result.StackFrame, inspectionContext, null)));
                        return;
                    }

                    var targetResult = EvaluateDataAtAddress(inspectionContext, result.StackFrame, "", $"*{result.FullName}", evalData.type.nullcSubType, targetAddress.Value, DkmEvaluationResultFlags.None, DkmEvaluationResultAccessType.Public, DkmEvaluationResultStorageType.None);

                    if (initialRequestSize != 0)
                        initialResults[0] = targetResult;

                    // If pointer points to a class, inline class member display
                    if (evalData.type.nullcSubType.subCat == NullcTypeSubCategory.Class && targetAddress.HasValue && targetAddress.Value != 0 && (targetResult as DkmSuccessEvaluationResult) != null)
                    {
                        result = targetResult;

                        var finalEvalData = new NullEvaluationDataItem();

                        finalEvalData.address = targetAddress.Value;
                        finalEvalData.fullName = $"(*{result.FullName})";
                        finalEvalData.type = evalData.type.nullcSubType;

                        evalData = finalEvalData;
                    }
                    else
                    {
                        var enumerator = DkmEvaluationResultEnumContext.Create(1, result.StackFrame, inspectionContext, evalData);

                        completionRoutine(new DkmGetChildrenAsyncResult(initialResults, enumerator));
                        return;
                    }
                }

                if (evalData.type.subCat == NullcTypeSubCategory.Class)
                {
                    int actualSize = evalData.type.memberCount;

                    bool isExtendable = evalData.type.typeFlags.HasFlag(NullcTypeFlags.IsExtendable);

                    NullcTypeInfo classType = evalData.type;

                    if (isExtendable)
                    {
                        int actualType = DebugHelpers.ReadIntVariable(inspectionContext.Thread.Process, evalData.address).GetValueOrDefault(0);

                        if (actualType != 0 && actualType < bytecode.types.Count)
                        {
                            classType = bytecode.types[actualType];

                            actualSize = 1 + classType.memberCount;
                        }
                    }

                    int finalInitialSize = initialRequestSize < actualSize ? initialRequestSize : actualSize;

                    DkmEvaluationResult[] initialResults = new DkmEvaluationResult[finalInitialSize];

                    for (int i = 0; i < initialResults.Length; i++)
                    {
                        if (isExtendable && i == 0)
                        {
                            ulong address = evalData.address;

                            var memberType = bytecode.types[(int)NullcTypeIndex.TypeId];

                            initialResults[i] = EvaluateDataAtAddress(inspectionContext, result.StackFrame, "typeid", $"{result.FullName}.typeid", memberType, address, DkmEvaluationResultFlags.None, DkmEvaluationResultAccessType.None, DkmEvaluationResultStorageType.None);
                        }
                        else
                        {
                            var memberIndex = isExtendable ? i - 1 : i;

                            var member = classType.nullcMembers[memberIndex];

                            ulong address = evalData.address + member.offset;

                            if (memberIndex < evalData.type.nullcMembers.Count)
                                initialResults[i] = EvaluateDataAtAddress(inspectionContext, result.StackFrame, member.name, $"{result.FullName}.{member.name}", member.nullcType, address, DkmEvaluationResultFlags.None, DkmEvaluationResultAccessType.None, DkmEvaluationResultStorageType.None);
                            else
                                initialResults[i] = EvaluateDataAtAddress(inspectionContext, result.StackFrame, member.name, $"(({classType.name} ref){result.FullName}).{member.name}", member.nullcType, address, DkmEvaluationResultFlags.None, DkmEvaluationResultAccessType.None, DkmEvaluationResultStorageType.None);
                        }
                    }

                    var finalEvalData = new NullEvaluationDataItem();

                    finalEvalData.address = evalData.address;
                    finalEvalData.fullName = $"(({classType.name} ref){result.FullName})";
                    finalEvalData.type = classType;

                    var enumerator = DkmEvaluationResultEnumContext.Create(actualSize, result.StackFrame, inspectionContext, finalEvalData);

                    completionRoutine(new DkmGetChildrenAsyncResult(initialResults, enumerator));
                    return;
                }

                if (evalData.type.subCat == NullcTypeSubCategory.Array)
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

                if (!PrepareContext(process, inspectionContext.RuntimeInstance.Id.RuntimeType, out string error))
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

                if (!PrepareContext(process, inspectionContext.RuntimeInstance.Id.RuntimeType, out string error))
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

                if (evalData.type.subCat == NullcTypeSubCategory.Pointer)
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

                if (evalData.type.subCat == NullcTypeSubCategory.Class)
                {
                    bool isExtendable = evalData.type.typeFlags.HasFlag(NullcTypeFlags.IsExtendable);

                    var results = new DkmEvaluationResult[count];

                    for (int i = startIndex; i < startIndex + count; i++)
                    {
                        if (isExtendable && i == 0)
                        {
                            ulong address = evalData.address;

                            var memberType = bytecode.types[(int)NullcTypeIndex.TypeId];

                            results[i - startIndex] = EvaluateDataAtAddress(enumContext.InspectionContext, enumContext.StackFrame, "typeid", $"{evalData.fullName}.typeid", memberType, address, DkmEvaluationResultFlags.None, DkmEvaluationResultAccessType.None, DkmEvaluationResultStorageType.None);
                        }
                        else
                        {
                            var memberIndex = isExtendable ? i - 1 : i;

                            var member = evalData.type.nullcMembers[i];

                            ulong address = evalData.address + member.offset;

                            results[i - startIndex] = EvaluateDataAtAddress(enumContext.InspectionContext, enumContext.StackFrame, member.name, $"{evalData.fullName}.{member.name}", member.nullcType, address, DkmEvaluationResultFlags.None, DkmEvaluationResultAccessType.None, DkmEvaluationResultStorageType.None);
                        }
                    }

                    completionRoutine(new DkmEvaluationEnumAsyncResult(results));
                    return;
                }

                if (evalData.type.subCat == NullcTypeSubCategory.Array)
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

            DkmInstructionSymbol[] IDkmSymbolFunctionResolver.Resolve(DkmSymbolFunctionResolutionRequest symbolFunctionResolutionRequest)
            {
                var processData = DebugHelpers.GetOrCreateDataItem<NullcLocalProcessDataItem>(symbolFunctionResolutionRequest.Process);

                var nullcCustomRuntime = symbolFunctionResolutionRequest.Process.GetRuntimeInstances().OfType<DkmCustomRuntimeInstance>().FirstOrDefault(el => el.Id.RuntimeType == DebugHelpers.NullcRuntimeGuid);
                var nullcNativeRuntime = DebugHelpers.useDefaultRuntimeInstance ? symbolFunctionResolutionRequest.Process.GetNativeRuntimeInstance() : symbolFunctionResolutionRequest.Process.GetRuntimeInstances().OfType<DkmNativeRuntimeInstance>().FirstOrDefault(el => el.Id.RuntimeType == DebugHelpers.NullcRuntimeGuid);

                if (DebugHelpers.useNativeInterfaces ? nullcNativeRuntime != null : nullcCustomRuntime != null)
                {
                    DkmModuleInstance nullcModuleInstance;

                    if (DebugHelpers.useNativeInterfaces)
                        nullcModuleInstance = nullcNativeRuntime.GetModuleInstances().OfType<DkmNativeModuleInstance>().FirstOrDefault(el => el.Module != null && el.Module.CompilerId.VendorId == DebugHelpers.NullcCompilerGuid);
                    else
                        nullcModuleInstance = nullcCustomRuntime.GetModuleInstances().OfType<DkmCustomModuleInstance>().FirstOrDefault(el => el.Module != null && el.Module.CompilerId.VendorId == DebugHelpers.NullcCompilerGuid);

                    if (nullcModuleInstance == null)
                        return symbolFunctionResolutionRequest.Resolve();

                    foreach (var function in processData.bytecode.functions)
                    {
                        if (function.name == symbolFunctionResolutionRequest.FunctionName)
                        {
                            // External function
                            if (function.regVmAddress == ~0u)
                                return symbolFunctionResolutionRequest.Resolve();

                            ulong nativeInstruction = processData.bytecode.ConvertInstructionToNativeAddress((int)function.regVmAddress);

                            return new DkmInstructionSymbol[1] { DkmNativeInstructionSymbol.Create(nullcModuleInstance.Module, (uint)(nativeInstruction - nullcModuleInstance.BaseAddress)) };
                        }
                    }
                }

                return symbolFunctionResolutionRequest.Resolve();
            }
        }
    }
}
