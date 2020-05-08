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
            //public const string NullcRuntimeId = "57CDD231-583D-4083-BFCD-26637F0ACFBF";
            //public static readonly Guid NullcRuntimeGuid = new Guid(NullcRuntimeId);



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
                        //if (processData.runtime == null)
                        //    processData.runtime = DkmCustomRuntimeInstance.Create(input.Thread.Process, new DkmRuntimeInstanceId(NullcRuntimeGuid, 0), null);

                        //var stackData = NullcDebuggerHelpers.GetOrCreateDataItem<DebugStackDataItem>(stackContext);

                        /*if (stackData.module == null && processData.nullcDebugGetNativeModuleBase != null && processData.nullcDebugGetNativeModuleSize != null)
                        {
                            string result = ExecuteExpression($"((unsigned long long(*)()){processData.nullcDebugGetNativeModuleBase})()", stackContext, input);

                            ulong moduleBase = 0;

                            if (result != null)
                                ulong.TryParse(result, out moduleBase);

                            result = ExecuteExpression($"((unsigned(*)()){processData.nullcDebugGetNativeModuleSize})()", stackContext, input);

                            uint moduleSize = 0;

                            if (result != null)
                                uint.TryParse(result, out moduleSize);

                            if (moduleBase != 0 && moduleSize != 0)
                                stackData.module = DkmCustomModuleInstance.Create("nullc", "nullc.embedded.code", 0, input.RuntimeInstance, null, null, DkmModuleFlags.None, DkmModuleMemoryLayout.Unknown, moduleBase, 1, moduleSize, "nullc embedded code", false, null, null, null);
                        }*/

                        //stackFrameDesc = $"{input.InstructionAddress.CPUInstructionPart.InstructionPointer:X} {stackFrameDesc}";

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

        public class NullcSymbolProvider : IDkmSymbolCompilerIdQuery, IDkmSymbolDocumentCollectionQuery, IDkmSymbolQuery, IDkmAsyncBreakCompleteReceived, IDkmProcessExecutionNotification/*, IDkmRuntimeInstanceLoadNotification*/, IDkmModuleUserCodeDeterminer
        {
            DkmCompilerId IDkmSymbolCompilerIdQuery.GetCompilerId(DkmInstructionSymbol instruction, DkmInspectionSession inspectionSession)
            {
                return new DkmCompilerId(Guid.Empty, Guid.Empty);
            }

            DkmResolvedDocument[] IDkmSymbolDocumentCollectionQuery.FindDocuments(DkmModule module, DkmSourceFileId sourceFileId)
            {
                if (module.Name != "nullc.embedded.code")
                    throw new NotSupportedException();

                //throw new NotSupportedException();
                return new[] {
                    DkmResolvedDocument.Create(module, module.Name, null, DkmDocumentMatchStrength.FullPath, DkmResolvedDocumentWarning.None, false, null)
                };
            }

            DkmSourcePosition IDkmSymbolQuery.GetSourcePosition(DkmInstructionSymbol instruction, DkmSourcePositionFlags flags, DkmInspectionSession inspectionSession, out bool startOfLine)
            {
                startOfLine = false;

                return null;
            }

            object IDkmSymbolQuery.GetSymbolInterface(DkmModule module, Guid interfaceID)
            {
                throw new NotImplementedException();
            }

            void IDkmAsyncBreakCompleteReceived.OnAsyncBreakCompleteReceived(DkmProcess process, DkmAsyncBreakStatus status, DkmThread thread, DkmEventDescriptorS eventDescriptor)
            {
                /*try
                {
                    ulong moduleBase = 0;

                    uint moduleSize = 0;

                    var processData = NullcDebuggerHelpers.GetOrCreateDataItem<DebugProcessDataItem>(thread.Process);

                    DkmCustomModuleInstance.Create("nullc", "nullc.embedded.code", 0, process.GetNativeRuntimeInstance(), null, null, DkmModuleFlags.None, DkmModuleMemoryLayout.Unknown, moduleBase, 1, moduleSize, "nullc embedded code", false, null, null, null);
                }
                catch (Exception ex)
                {
                    Console.WriteLine("OnAsyncBreakCompleteReceived failed with: " + ex.ToString());
                }*/
            }

            void IDkmProcessExecutionNotification.OnProcessPause(DkmProcess process, DkmProcessExecutionCounters processCounters)
            {
                try
                {
                    ulong moduleBase = NullcDebuggerHelpers.ReadUlongVariable(process, "nullcModuleStartAddress").GetValueOrDefault(0);

                    uint moduleSize = (uint)NullcDebuggerHelpers.ReadUlongVariable(process, "nullcModuleEndAddress").GetValueOrDefault(0);

                    var processData = NullcDebuggerHelpers.GetOrCreateDataItem<RemoteProcessDataItem>(process);

                    //foreach (var runtimeInstance in process.GetRuntimeInstances())
                    /*var runtimeInstance = process.GetNativeRuntimeInstance();

                    if (runtimeInstance != null)
                    {
                        foreach (var module in runtimeInstance.GetModuleInstances())
                        {
                            var nativeModule = module as DkmNativeModuleInstance;

                            var nullcModuleStartAddress = nativeModule?.FindExportName("nullcModuleStartAddress", IgnoreDataExports: false);
                            var nullcModuleEndAddress = nativeModule?.FindExportName("nullcModuleEndAddress", IgnoreDataExports: false);

                            if (nullcModuleStartAddress != null && nullcModuleEndAddress != null)
                            {
                                if ((process.SystemInformation.Flags & DkmSystemInformationFlags.Is64Bit) == 0)
                                {
                                    byte[] nullcModuleStartAddressData = new byte[4];
                                    byte[] nullcModuleEndAddressData = new byte[4];

                                    process.ReadMemory(nullcModuleStartAddress.CPUInstructionPart.InstructionPointer, DkmReadMemoryFlags.None, nullcModuleStartAddressData);
                                    process.ReadMemory(nullcModuleEndAddress.CPUInstructionPart.InstructionPointer, DkmReadMemoryFlags.None, nullcModuleEndAddressData);

                                    moduleBase = (ulong)BitConverter.ToUInt32(nullcModuleStartAddressData, 0);
                                    var moduleEnd = BitConverter.ToUInt32(nullcModuleEndAddressData, 0);

                                    moduleSize = (uint)(moduleEnd - moduleBase);
                                }
                                else
                                {
                                    byte[] nullcModuleStartAddressData = new byte[8];
                                    byte[] nullcModuleEndAddressData = new byte[8];

                                    process.ReadMemory(nullcModuleStartAddress.CPUInstructionPart.InstructionPointer, DkmReadMemoryFlags.None, nullcModuleStartAddressData);
                                    process.ReadMemory(nullcModuleEndAddress.CPUInstructionPart.InstructionPointer, DkmReadMemoryFlags.None, nullcModuleEndAddressData);

                                    moduleBase = BitConverter.ToUInt64(nullcModuleStartAddressData, 0);
                                    var moduleEnd = BitConverter.ToUInt64(nullcModuleEndAddressData, 0);

                                    moduleSize = (uint)(moduleEnd - moduleBase);
                                }

                                break;
                            }
                        }
                    }*/

                    if (moduleBase == 0 || moduleSize == 0)
                        return;

                    /*if (processData.runtimeInstance == null)
                    {
                        processData.runtimeId = new DkmRuntimeInstanceId(NullcDebuggerHelpers.NullcRuntimeGuid, 0);

                        processData.runtimeInstance = DkmCustomRuntimeInstance.Create(process, processData.runtimeId, null);//DkmRuntimeCapabilities.None, process.GetNativeRuntimeInstance(), null);
                    }*/

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

            public void OnProcessResume(DkmProcess process, DkmProcessExecutionCounters processCounters)
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

            /*void IDkmRuntimeInstanceLoadNotification.OnRuntimeInstanceLoad(DkmRuntimeInstance runtimeInstance, DkmEventDescriptor eventDescriptor)
            {
                var processData = NullcDebuggerHelpers.GetOrCreateDataItem<RemoteProcessDataItem>(runtimeInstance.Process);

                foreach (var module in runtimeInstance.GetModuleInstances())
                {
                    var nativeModule = module as DkmNativeModuleInstance;

                    var nullcModuleStartAddress = nativeModule?.FindExportName("nullcModuleStartAddress", IgnoreDataExports: false);
                    var nullcModuleEndAddress = nativeModule?.FindExportName("nullcModuleEndAddress", IgnoreDataExports: false);

                    if (nullcModuleStartAddress != null && nullcModuleEndAddress != null)
                    {
                        if (processData.runtimeInstance == null)
                        {
                            processData.runtimeId = new DkmRuntimeInstanceId(NullcDebuggerHelpers.NullcRuntimeGuid, 0);

                            var temp = DkmRuntimeId.Native;

                            processData.runtimeInstance = DkmCustomRuntimeInstance.Create(runtimeInstance.Process, processData.runtimeId, null);
                        }

                        break;
                    }
                }
            }*/
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
