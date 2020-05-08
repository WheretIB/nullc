using System;

using Microsoft.VisualStudio.Debugger.CallStack;
using Microsoft.VisualStudio.Debugger.Evaluation;
using Microsoft.VisualStudio.Debugger.ComponentInterfaces;
using Microsoft.VisualStudio.Debugger.Symbols;
using Microsoft.VisualStudio.Debugger;
using Microsoft.VisualStudio.Debugger.Native;
using Microsoft.VisualStudio.Debugger.CustomRuntimes;
using System.Runtime.InteropServices;
using System.IO;
using System.Linq;
using System.Collections.ObjectModel;
using Microsoft.VisualStudio.Debugger.DefaultPort;
using System.Runtime.Remoting;
using System.Collections;
using System.Collections.Generic;
using System.Diagnostics;

namespace nullc_debugger_component
{
    namespace DkmDebugger
    {
        internal class DebugProcessDataItem : DkmDataItem
        {
            //public DkmCustomRuntimeInstance runtime = null;

            public bool nullcIsMissing = false;
            public bool nullcIsReady = false;

            public string nullcDebugGetNativeAddressLocation = null;
            public string nullcDebugGetNativeModuleBase = null;
            public string nullcDebugGetNativeModuleSize = null;
        }

        internal class RemoteProcessDataItem : DkmDataItem
        {
            public DkmRuntimeInstanceId runtimeId;
            public DkmCustomRuntimeInstance runtimeInstance = null;

            public DkmModuleId moduleId;
            public DkmCompilerId compilerId;

            public DkmLanguage language;

            public DkmModule module = null;
            public DkmCustomModuleInstance moduleInstance = null;
        }

        internal class LocalProcessDataItem : DkmDataItem
        {
            public ulong moduleBytecodeLocation = 0;
            public ulong moduleBytecodeSize = 0;
            public byte[] moduleBytecodeRaw;

            public NullcBytecode bytecode;
        }

        static class NullcDebuggerHelpers
        {
            public static readonly Guid NullcSymbolProviderGuid = new Guid("BF13BE48-BE1A-4424-B961-BFC40C71E58A");
            public static readonly Guid NullcCompilerGuid = new Guid("A7CB5F2B-CD45-4CF4-9CB6-61A30968EFB5");
            public static readonly Guid NullcLanguageGuid = new Guid("9221BA37-3FB0-483A-BD6A-0E5DD22E107E");
            public static readonly Guid NullcRuntimeGuid = new Guid("3AF14FEA-CB31-4DBB-90E5-74BF685CA7B8");

            //public static readonly Guid NullcSymbolProviderFilterGuid = new Guid("0B2E28CC-D574-461C-90F4-0C251AA71DE8");

            internal static T GetOrCreateDataItem<T>(DkmDataContainer container) where T : DkmDataItem, new()
            {
                T item = container.GetDataItem<T>();

                if (item != null)
                    return item;

                item = new T();

                container.SetDataItem<T>(DkmDataCreationDisposition.CreateNew, item);

                return item;
            }

            internal static ulong? ReadUlongVariable(DkmProcess process, string name)
            {
                var runtimeInstance = process.GetNativeRuntimeInstance();

                if (runtimeInstance != null)
                {
                    foreach (var module in runtimeInstance.GetModuleInstances())
                    {
                        var nativeModule = module as DkmNativeModuleInstance;

                        var variableAddress = nativeModule?.FindExportName(name, IgnoreDataExports: false);

                        if (variableAddress != null)
                        {
                            if ((process.SystemInformation.Flags & DkmSystemInformationFlags.Is64Bit) == 0)
                            {
                                byte[] variableAddressData = new byte[4];

                                process.ReadMemory(variableAddress.CPUInstructionPart.InstructionPointer, DkmReadMemoryFlags.None, variableAddressData);

                                return (ulong)BitConverter.ToUInt32(variableAddressData, 0);
                            }
                            else
                            {
                                byte[] variableAddressData = new byte[8];

                                process.ReadMemory(variableAddress.CPUInstructionPart.InstructionPointer, DkmReadMemoryFlags.None, variableAddressData);

                                return BitConverter.ToUInt64(variableAddressData, 0);
                            }
                        }
                    }
                }

                return null;
            }
        }

        public class NullcDebugger : IDkmCallStackFilter
        {
            internal string FindFunctionAddress(DkmRuntimeInstance runtimeInstance, string name)
            {
                string result = null;

                foreach (var module in runtimeInstance.GetModuleInstances())
                {
                    var address = (module as DkmNativeModuleInstance)?.FindExportName(name, IgnoreDataExports: true);

                    if (address != null)
                    {
                        result = $"0x{address.CPUInstructionPart.InstructionPointer:X}";
                        break;
                    }
                }

                return result;
            }

            internal void InitNullcDebugFunctions(DebugProcessDataItem processData, DkmRuntimeInstance runtimeInstance)
            {
                if (processData.nullcIsMissing)
                    return;

                processData.nullcDebugGetNativeAddressLocation = FindFunctionAddress(runtimeInstance, "nullcDebugGetNativeAddressLocation");

                if (processData.nullcDebugGetNativeAddressLocation == null)
                {
                    processData.nullcIsMissing = true;
                    return;
                }

                processData.nullcDebugGetNativeModuleBase = FindFunctionAddress(runtimeInstance, "nullcDebugGetNativeModuleBase");
                processData.nullcDebugGetNativeModuleSize = FindFunctionAddress(runtimeInstance, "nullcDebugGetNativeModuleSize");
            }

            internal string ExecuteExpression(string expression, DkmStackContext stackContext, DkmStackWalkFrame input)
            {
                var compilerId = new DkmCompilerId(DkmVendorId.Microsoft, DkmLanguageId.Cpp);
                var language = DkmLanguage.Create("C++", compilerId);
                var languageExpression = DkmLanguageExpression.Create(language, DkmEvaluationFlags.None, expression, null);

                var inspectionContext = DkmInspectionContext.Create(stackContext.InspectionSession, input.RuntimeInstance, stackContext.Thread, 200, DkmEvaluationFlags.None, DkmFuncEvalFlags.None, 10, language, null);

                var workList = DkmWorkList.Create(null);
                string resultText = null;

                inspectionContext.EvaluateExpression(workList, languageExpression, input, res =>
                {
                    if (res.ErrorCode == 0)
                    {
                        var result = res.ResultObject as DkmSuccessEvaluationResult;

                        if (result != null && result.TagValue == DkmEvaluationResult.Tag.SuccessResult && result.Address.Value != 0)
                            resultText = result.Value;

                        res.ResultObject.Close();
                    }
                });

                workList.Execute();

                return resultText;
            }

            public DkmStackWalkFrame[] FilterNextFrame(DkmStackContext stackContext, DkmStackWalkFrame input)
            {
                if (input == null) // null input frame indicates the end of the call stack. This sample does nothing on end-of-stack.
                    return null;

                if (input.InstructionAddress == null)
                    return new DkmStackWalkFrame[1] { input };

                if (input.InstructionAddress.ModuleInstance != null)
                    return new DkmStackWalkFrame[1] { input };

                // Currently we want to provide info only for JiT frames
                if (!input.Flags.HasFlag(DkmStackWalkFrameFlags.UserStatusNotDetermined))
                    return new DkmStackWalkFrame[1] { input };

                try
                {
                    var processData = NullcDebuggerHelpers.GetOrCreateDataItem<DebugProcessDataItem>(input.Thread.Process);

                    InitNullcDebugFunctions(processData, input.RuntimeInstance);

                    if (processData.nullcDebugGetNativeAddressLocation == null)
                        return new DkmStackWalkFrame[1] { input };

                    string stackFrameDesc = ExecuteExpression($"((char*(*)(void*)){processData.nullcDebugGetNativeAddressLocation})((void*)0x{input.InstructionAddress.CPUInstructionPart.InstructionPointer:X}),sb", stackContext, input);

                    if (stackFrameDesc != null)
                    {
                        var flags = input.Flags;

                        flags = flags & ~(DkmStackWalkFrameFlags.NonuserCode | DkmStackWalkFrameFlags.UserStatusNotDetermined);

                        DkmStackWalkFrame frame = null;

                        var nullcRuntime = input.Thread.Process.GetRuntimeInstances().OfType<DkmCustomRuntimeInstance>().FirstOrDefault(el => el.Id.RuntimeType == NullcDebuggerHelpers.NullcRuntimeGuid);

                        if (nullcRuntime != null)
                        {
                            var nullcModuleInstance = nullcRuntime.GetModuleInstances().OfType<DkmCustomModuleInstance>().FirstOrDefault(el => el.Module.CompilerId.VendorId == NullcDebuggerHelpers.NullcCompilerGuid);

                            if (nullcModuleInstance != null)
                            {
                                var instructionAddress = DkmCustomInstructionAddress.Create(nullcRuntime, nullcModuleInstance, null, input.InstructionAddress.CPUInstructionPart.InstructionPointer, null, input.InstructionAddress.CPUInstructionPart);

                                frame = DkmStackWalkFrame.Create(stackContext.Thread, instructionAddress, input.FrameBase, input.FrameSize, flags, stackFrameDesc, input.Registers, input.Annotations, nullcModuleInstance, null, null);
                            }
                        }

                        if (frame == null)
                            frame = DkmStackWalkFrame.Create(stackContext.Thread, input.InstructionAddress, input.FrameBase, input.FrameSize, flags, stackFrameDesc, input.Registers, input.Annotations/*, stackData.module, null, null*/);

                        return new DkmStackWalkFrame[1] { frame };
                    }
                }
                catch (Exception ex)
                {
                    Console.WriteLine("Failed to evaluate: " + ex.ToString());
                }

                return new DkmStackWalkFrame[1] { input };
            }
        }

        public class NullcSymbolProvider : IDkmProcessExecutionNotification, IDkmModuleUserCodeDeterminer
        {
            void IDkmProcessExecutionNotification.OnProcessPause(DkmProcess process, DkmProcessExecutionCounters processCounters)
            {
                try
                {
                    ulong moduleBase = NullcDebuggerHelpers.ReadUlongVariable(process, "nullcModuleStartAddress").GetValueOrDefault(0);

                    uint moduleSize = (uint)NullcDebuggerHelpers.ReadUlongVariable(process, "nullcModuleEndAddress").GetValueOrDefault(0);

                    var processData = NullcDebuggerHelpers.GetOrCreateDataItem<RemoteProcessDataItem>(process);

                    if (moduleBase == 0 || moduleSize == 0)
                        return;

                    if (processData.runtimeInstance == null)
                    {
                        processData.runtimeId = new DkmRuntimeInstanceId(NullcDebuggerHelpers.NullcRuntimeGuid, 0);

                        processData.runtimeInstance = DkmCustomRuntimeInstance.Create(process, processData.runtimeId, null);
                    }

                    if (processData.module == null)
                    {
                        processData.moduleId = new DkmModuleId(Guid.NewGuid(), NullcDebuggerHelpers.NullcSymbolProviderGuid);

                        processData.compilerId = new DkmCompilerId(NullcDebuggerHelpers.NullcCompilerGuid, NullcDebuggerHelpers.NullcLanguageGuid);

                        processData.language = DkmLanguage.Create("nullc", processData.compilerId);

                        processData.module = DkmModule.Create(processData.moduleId, "nullc.embedded.code", processData.compilerId, process.Connection, null);
                    }

                    if (processData.moduleInstance == null)
                    {
                        DkmDynamicSymbolFileId symbolFileId = DkmDynamicSymbolFileId.Create(NullcDebuggerHelpers.NullcSymbolProviderGuid);

                        processData.moduleInstance = DkmCustomModuleInstance.Create("nullc", "nullc.embedded.code", 0, processData.runtimeInstance, null, symbolFileId, DkmModuleFlags.None, DkmModuleMemoryLayout.Unknown, moduleBase, 1, moduleSize, "nullc embedded code", false, null, null, null);

                        processData.moduleInstance.SetModule(processData.module, true); // Can use reload?
                    }
                }
                catch (Exception ex)
                {
                    Console.WriteLine("OnProcessPause failed with: " + ex.ToString());
                }
            }

            void IDkmProcessExecutionNotification.OnProcessResume(DkmProcess process, DkmProcessExecutionCounters processCounters)
            {
            }

            bool IDkmModuleUserCodeDeterminer.IsUserCode(DkmModuleInstance moduleInstance)
            {
                var processData = NullcDebuggerHelpers.GetOrCreateDataItem<RemoteProcessDataItem>(moduleInstance.Process);

                if (processData != null)
                {
                    if (moduleInstance.LoadContext == "nullc embedded code")
                        return true;
                }

                return moduleInstance.IsUserCode();
            }
        }

        public class NullcLocalSymbolProvider : IDkmSymbolCompilerIdQuery, IDkmSymbolDocumentCollectionQuery, IDkmSymbolDocumentSpanQuery, IDkmSymbolQuery, IDkmLanguageFrameDecoder, IDkmModuleInstanceLoadNotification
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

                return module.FindDocuments(sourceFileId);
                //return new DkmResolvedDocument[0];
                /*return new[] {
                    DkmResolvedDocument.Create(module, module.Name, null, DkmDocumentMatchStrength.FullPath, DkmResolvedDocumentWarning.None, false, null)
                };*/
            }

            DkmInstructionSymbol[] IDkmSymbolDocumentSpanQuery.FindSymbols(DkmResolvedDocument resolvedDocument, DkmTextSpan textSpan, string text, out DkmSourcePosition[] symbolLocation)
            {
                var sourceFileId = DkmSourceFileId.Create(resolvedDocument.DocumentName, null, null, null);
                var resultSpan = new DkmTextSpan(textSpan.StartLine, textSpan.StartLine, 0, 0);
                symbolLocation = new DkmSourcePosition[1] { DkmSourcePosition.Create(sourceFileId, resultSpan) };

                return new DkmCustomInstructionSymbol[1] { DkmCustomInstructionSymbol.Create(resolvedDocument.Module, DkmRuntimeId.Native, null, 0, null) };
            }

            DkmSourcePosition IDkmSymbolQuery.GetSourcePosition(DkmInstructionSymbol instruction, DkmSourcePositionFlags flags, DkmInspectionSession inspectionSession, out bool startOfLine)
            {
                var processData = NullcDebuggerHelpers.GetOrCreateDataItem<LocalProcessDataItem>(inspectionSession.Process);

                if (processData.bytecode != null)
                {
                    var customInstructionSymbol = instruction as DkmCustomInstructionSymbol;

                    if (customInstructionSymbol != null)
                    {
                        int nullcInstruction = processData.bytecode.ConvertNativeAddressToInstruction(customInstructionSymbol.Offset);

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

                //throw new NotImplementedException();
            }

            void IDkmLanguageFrameDecoder.GetFrameName(DkmInspectionContext inspectionContext, DkmWorkList workList, DkmStackWalkFrame frame, DkmVariableInfoFlags argumentFlags, DkmCompletionRoutine<DkmGetFrameNameAsyncResult> completionRoutine)
            {
                inspectionContext.GetFrameName(workList, frame, argumentFlags, completionRoutine);
                //completionRoutine(new DkmGetFrameNameAsyncResult("FakeTestResultFunction"));
            }

            void IDkmLanguageFrameDecoder.GetFrameReturnType(DkmInspectionContext inspectionContext, DkmWorkList workList, DkmStackWalkFrame frame, DkmCompletionRoutine<DkmGetFrameReturnTypeAsyncResult> completionRoutine)
            {
                inspectionContext.GetFrameReturnType(workList, frame, completionRoutine);
                // Not provided at the moment
                //completionRoutine(new DkmGetFrameReturnTypeAsyncResult(null));
            }

            void IDkmModuleInstanceLoadNotification.OnModuleInstanceLoad(DkmModuleInstance moduleInstance, DkmWorkList workList, DkmEventDescriptorS eventDescriptor)
            {
                var processData = NullcDebuggerHelpers.GetOrCreateDataItem<LocalProcessDataItem>(moduleInstance.Process);

                if (processData.moduleBytecodeLocation == 0)
                {
                    processData.moduleBytecodeLocation = NullcDebuggerHelpers.ReadUlongVariable(moduleInstance.Process, "nullcModuleBytecodeLocation").GetValueOrDefault(0);
                    processData.moduleBytecodeSize = NullcDebuggerHelpers.ReadUlongVariable(moduleInstance.Process, "nullcModuleBytecodeSize").GetValueOrDefault(0);

                    if (processData.moduleBytecodeLocation != 0)
                    {
                        processData.moduleBytecodeRaw = new byte[processData.moduleBytecodeSize];
                        moduleInstance.Process.ReadMemory(processData.moduleBytecodeLocation, DkmReadMemoryFlags.None, processData.moduleBytecodeRaw);

                        processData.bytecode = new NullcBytecode();
                        processData.bytecode.ReadFrom(processData.moduleBytecodeRaw, (moduleInstance.Process.SystemInformation.Flags & DkmSystemInformationFlags.Is64Bit) != 0);
                    }
                }
            }
        }
    }
}
