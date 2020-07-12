using Microsoft.VisualStudio.Debugger;
using Microsoft.VisualStudio.Debugger.CallStack;
using Microsoft.VisualStudio.Debugger.ComponentInterfaces;
using Microsoft.VisualStudio.Debugger.CustomRuntimes;
using Microsoft.VisualStudio.Debugger.Evaluation;
using Microsoft.VisualStudio.Debugger.Native;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;

namespace nullc_debugger_component
{
    namespace DkmDebugger
    {
        internal class NullcStackFilterDataItem : DkmDataItem
        {
            public bool nullcIsMissing = false;
            public bool nullcIsReady = false;

            public string nullcDebugGetVmAddressLocation = null;
            public string nullcDebugGetNativeAddressLocation = null;
            public string nullcDebugGetReversedStackDataBase = null;

            public int nullcFramePosition = 1;
        }

        internal class NullcModuleDataItem : DkmDataItem
        {
            public bool nullcIsMissing = false;

            public ulong moduleBase;
            public uint moduleSize;
        }

        public class NullcStackFilter : IDkmCallStackFilter, IDkmInstructionAddressProvider
        {
            internal void InitNullcDebugFunctions(NullcStackFilterDataItem processData, DkmRuntimeInstance runtimeInstance)
            {
                if (processData.nullcIsMissing)
                {
                    return;
                }

                processData.nullcDebugGetVmAddressLocation = DebugHelpers.FindFunctionAddress(runtimeInstance, "nullcDebugGetVmAddressLocation");
                processData.nullcDebugGetNativeAddressLocation = DebugHelpers.FindFunctionAddress(runtimeInstance, "nullcDebugGetNativeAddressLocation");
                processData.nullcDebugGetReversedStackDataBase = DebugHelpers.FindFunctionAddress(runtimeInstance, "nullcDebugGetReversedStackDataBase");

                if (processData.nullcDebugGetVmAddressLocation == null || processData.nullcDebugGetNativeAddressLocation == null || processData.nullcDebugGetReversedStackDataBase == null)
                {
                    processData.nullcIsMissing = true;
                    return;
                }
            }

            internal string ExecuteExpression(string expression, DkmStackContext stackContext, DkmStackWalkFrame input, bool allowZero)
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

                        if (result != null && result.TagValue == DkmEvaluationResult.Tag.SuccessResult && (allowZero || result.Address.Value != 0))
                        {
                            resultText = result.Value;
                        }

                        res.ResultObject.Close();
                    }
                });

                workList.Execute();

                return resultText;
            }

            DkmStackWalkFrame[] IDkmCallStackFilter.FilterNextFrame(DkmStackContext stackContext, DkmStackWalkFrame input)
            {
                if (input == null) // null input frame indicates the end of the call stack. This sample does nothing on end-of-stack.
                {
                    var processData = DebugHelpers.GetOrCreateDataItem<NullcStackFilterDataItem>(stackContext.InspectionSession.Process);

                    processData.nullcFramePosition = 1;

                    return null;
                }

                if (input.InstructionAddress == null)
                    return new DkmStackWalkFrame[1] { input };

                if (input.InstructionAddress.ModuleInstance != null)
                {
                    if (input.BasicSymbolInfo != null && input.BasicSymbolInfo.MethodName == "ExecutorRegVm::RunCode")
                    {
                        var processData = DebugHelpers.GetOrCreateDataItem<NullcStackFilterDataItem>(input.Thread.Process);

                        InitNullcDebugFunctions(processData, input.RuntimeInstance);

                        if (processData.nullcIsMissing)
                            return new DkmStackWalkFrame[1] { input };

                        string vmInstructionStr = ExecuteExpression("instruction - codeBase", stackContext, input, true);

                        string ptrValue = DebugHelpers.Is64Bit(input.Thread.Process) ? "longValue" : "intValue";

                        string vmDataOffsetStr = ExecuteExpression($"(unsigned long long)regFilePtr[1].{ptrValue} - (unsigned long long)rvm->dataStack.data", stackContext, input, true);

                        if (vmInstructionStr != null && vmDataOffsetStr != null)
                        {
                            ulong vmInstruction = ulong.Parse(vmInstructionStr);

                            string stackFrameDesc = ExecuteExpression($"((char*(*)(unsigned, unsigned)){processData.nullcDebugGetVmAddressLocation})({vmInstruction}, 0),sb", stackContext, input, false);

                            var nullcCustomRuntime = input.Thread.Process.GetRuntimeInstances().OfType<DkmCustomRuntimeInstance>().FirstOrDefault(el => el.Id.RuntimeType == DebugHelpers.NullcVmRuntimeGuid);

                            if (stackFrameDesc != null && nullcCustomRuntime != null)
                            {
                                var flags = input.Flags;

                                flags = flags & ~(DkmStackWalkFrameFlags.NonuserCode | DkmStackWalkFrameFlags.UserStatusNotDetermined);
                                flags = flags | DkmStackWalkFrameFlags.InlineOptimized;

                                DkmCustomModuleInstance nullcModuleInstance = nullcCustomRuntime.GetModuleInstances().OfType<DkmCustomModuleInstance>().FirstOrDefault(el => el.Module != null && el.Module.CompilerId.VendorId == DebugHelpers.NullcCompilerGuid);

                                if (nullcModuleInstance != null)
                                {
                                    DkmInstructionAddress instructionAddress = DkmCustomInstructionAddress.Create(nullcCustomRuntime, nullcModuleInstance, null, vmInstruction, null, null);

                                    var rawAnnotations = new List<DkmStackWalkFrameAnnotation>();

                                    // Additional unique request id
                                    rawAnnotations.Add(DkmStackWalkFrameAnnotation.Create(DebugHelpers.NullcCallStackDataBaseGuid, ulong.Parse(vmDataOffsetStr)));

                                    var annotations = new ReadOnlyCollection<DkmStackWalkFrameAnnotation>(rawAnnotations);

                                    DkmStackWalkFrame frame = DkmStackWalkFrame.Create(stackContext.Thread, instructionAddress, input.FrameBase, input.FrameSize, flags, stackFrameDesc, input.Registers, annotations, nullcModuleInstance, null, null);

                                    return new DkmStackWalkFrame[2] { frame, input };
                                }
                            }
                        }
                    }

                    return new DkmStackWalkFrame[1] { input };
                }

                // Currently we want to provide info only for JiT frames
                if (!input.Flags.HasFlag(DkmStackWalkFrameFlags.UserStatusNotDetermined))
                    return new DkmStackWalkFrame[1] { input };

                try
                {
                    var processData = DebugHelpers.GetOrCreateDataItem<NullcStackFilterDataItem>(input.Thread.Process);

                    InitNullcDebugFunctions(processData, input.RuntimeInstance);

                    if (processData.nullcIsMissing)
                        return new DkmStackWalkFrame[1] { input };

                    string stackFrameDesc = ExecuteExpression($"((char*(*)(void*, unsigned)){processData.nullcDebugGetNativeAddressLocation})((void*)0x{input.InstructionAddress.CPUInstructionPart.InstructionPointer:X}, 0),sb", stackContext, input, false);

                    if (stackFrameDesc != null)
                    {
                        var flags = input.Flags;

                        flags = flags & ~(DkmStackWalkFrameFlags.NonuserCode | DkmStackWalkFrameFlags.UserStatusNotDetermined);

                        if (stackFrameDesc == "[Transition to nullc]")
                             return new DkmStackWalkFrame[1] { DkmStackWalkFrame.Create(stackContext.Thread, input.InstructionAddress, input.FrameBase, input.FrameSize, flags, stackFrameDesc, input.Registers, input.Annotations) };

                        DkmStackWalkFrame frame = null;

                        var nullcCustomRuntime = input.Thread.Process.GetRuntimeInstances().OfType<DkmCustomRuntimeInstance>().FirstOrDefault(el => el.Id.RuntimeType == DebugHelpers.NullcRuntimeGuid);
                        var nullcNativeRuntime = DebugHelpers.useDefaultRuntimeInstance ? input.Thread.Process.GetNativeRuntimeInstance() : input.Thread.Process.GetRuntimeInstances().OfType<DkmNativeRuntimeInstance>().FirstOrDefault(el => el.Id.RuntimeType == DebugHelpers.NullcRuntimeGuid);

                        if (DebugHelpers.useNativeInterfaces ? nullcNativeRuntime != null : nullcCustomRuntime != null)
                        {
                            DkmModuleInstance nullcModuleInstance;

                            if (DebugHelpers.useNativeInterfaces)
                            {
                                nullcModuleInstance = nullcNativeRuntime.GetModuleInstances().OfType<DkmNativeModuleInstance>().FirstOrDefault(el => el.Module != null && el.Module.CompilerId.VendorId == DebugHelpers.NullcCompilerGuid);
                            }
                            else
                            {
                                nullcModuleInstance = nullcCustomRuntime.GetModuleInstances().OfType<DkmCustomModuleInstance>().FirstOrDefault(el => el.Module != null && el.Module.CompilerId.VendorId == DebugHelpers.NullcCompilerGuid);
                            }

                            if (nullcModuleInstance != null)
                            {
                                // If the top of the call stack is a nullc frame, nullc call stack wont have an entry for it and we start from 0, otherwise we start from default value of 1
                                if (input.Flags.HasFlag(DkmStackWalkFrameFlags.TopFrame))
                                    processData.nullcFramePosition = 0;

                                string stackFrameBase = ExecuteExpression($"((unsigned(*)(unsigned)){processData.nullcDebugGetReversedStackDataBase})({processData.nullcFramePosition})", stackContext, input, true);

                                processData.nullcFramePosition++;

                                if (int.TryParse(stackFrameBase, out int stackFrameBaseValue))
                                {
                                    DkmInstructionAddress instructionAddress;

                                    if (DebugHelpers.useNativeInterfaces)
                                    {
                                        var rva = (uint)(input.InstructionAddress.CPUInstructionPart.InstructionPointer - nullcModuleInstance.BaseAddress);

                                        instructionAddress = DkmNativeInstructionAddress.Create(nullcNativeRuntime, nullcModuleInstance as DkmNativeModuleInstance, rva, input.InstructionAddress.CPUInstructionPart);
                                    }
                                    else
                                    {
                                        instructionAddress = DkmCustomInstructionAddress.Create(nullcCustomRuntime, nullcModuleInstance as DkmCustomModuleInstance, null, input.InstructionAddress.CPUInstructionPart.InstructionPointer, null, input.InstructionAddress.CPUInstructionPart);
                                    }

                                    var rawAnnotations = new List<DkmStackWalkFrameAnnotation>();

                                    // Additional unique request id
                                    rawAnnotations.Add(DkmStackWalkFrameAnnotation.Create(DebugHelpers.NullcCallStackDataBaseGuid, (ulong)(stackFrameBaseValue)));

                                    var annotations = new ReadOnlyCollection<DkmStackWalkFrameAnnotation>(rawAnnotations);

                                    frame = DkmStackWalkFrame.Create(stackContext.Thread, instructionAddress, input.FrameBase, input.FrameSize, flags, stackFrameDesc, input.Registers, annotations, nullcModuleInstance, null, null);
                                }
                            }
                        }

                        if (frame == null)
                            frame = DkmStackWalkFrame.Create(stackContext.Thread, input.InstructionAddress, input.FrameBase, input.FrameSize, flags, stackFrameDesc, input.Registers, input.Annotations);

                        return new DkmStackWalkFrame[1] { frame };
                    }
                }
                catch (Exception ex)
                {
                    Console.WriteLine("Failed to evaluate: " + ex.ToString());
                }

                return new DkmStackWalkFrame[1] { input };
            }

            void IDkmInstructionAddressProvider.GetInstructionAddress(DkmProcess process, DkmWorkList workList, ulong instructionPointer, DkmCompletionRoutine<DkmGetInstructionAddressAsyncResult> completionRoutine)
            {
                var processData = DebugHelpers.GetOrCreateDataItem<NullcModuleDataItem>(process);

                if (!processData.nullcIsMissing && processData.moduleBase == 0)
                {
                    processData.moduleBase = DebugHelpers.ReadPointerVariable(process, "nullcModuleStartAddress").GetValueOrDefault(0);

                    processData.moduleSize = (uint)(DebugHelpers.ReadPointerVariable(process, "nullcModuleEndAddress").GetValueOrDefault(0) - processData.moduleBase);

                    processData.nullcIsMissing = processData.moduleBase == 0;
                }

                if (processData.moduleBase != 0)
                {
                    if (instructionPointer >= processData.moduleBase && instructionPointer < processData.moduleBase + processData.moduleSize)
                    {
                        DkmInstructionAddress address;

                        if (DebugHelpers.useNativeInterfaces)
                        {
                            var nullcNativeRuntime = DebugHelpers.useDefaultRuntimeInstance ? process.GetNativeRuntimeInstance() :  process.GetRuntimeInstances().OfType<DkmNativeRuntimeInstance>().FirstOrDefault(el => el.Id.RuntimeType == DebugHelpers.NullcRuntimeGuid);
                            var nullcModuleInstance = nullcNativeRuntime.GetModuleInstances().OfType<DkmNativeModuleInstance>().FirstOrDefault(el => el.Module != null && el.Module.CompilerId.VendorId == DebugHelpers.NullcCompilerGuid); ;

                            address = DkmNativeInstructionAddress.Create(nullcNativeRuntime, nullcModuleInstance, (uint)(instructionPointer - processData.moduleBase), new DkmInstructionAddress.CPUInstruction(instructionPointer));
                        }
                        else
                        {
                            var nullcNativeRuntime = process.GetRuntimeInstances().OfType<DkmCustomRuntimeInstance>().FirstOrDefault(el => el.Id.RuntimeType == DebugHelpers.NullcRuntimeGuid);
                            var nullcModuleInstance = nullcNativeRuntime.GetModuleInstances().OfType<DkmCustomModuleInstance>().FirstOrDefault(el => el.Module != null && el.Module.CompilerId.VendorId == DebugHelpers.NullcCompilerGuid); ;

                            address = DkmCustomInstructionAddress.Create(nullcNativeRuntime, nullcModuleInstance, null, instructionPointer, null, new DkmInstructionAddress.CPUInstruction(instructionPointer));
                        }

                        completionRoutine(new DkmGetInstructionAddressAsyncResult(address, true));
                        return;
                    }
                }

                process.GetInstructionAddress(workList, instructionPointer, completionRoutine);
            }
        }
    }
}
