﻿using System;

using Microsoft.VisualStudio.Debugger.CallStack;
using Microsoft.VisualStudio.Debugger.Evaluation;
using Microsoft.VisualStudio.Debugger.ComponentInterfaces;
using Microsoft.VisualStudio.Debugger.Symbols;
using Microsoft.VisualStudio.Debugger;
using Microsoft.VisualStudio.Debugger.Native;
using Microsoft.VisualStudio.Debugger.CustomRuntimes;

namespace nullc_debugger_component
{
    namespace DkmDebugger
    {
        internal class DebugProcessDataItem : DkmDataItem
        {
            public bool nullcIsMissing = false;
            public bool nullcIsReady = false;

            public string nullcDebugGetNativeAddressLocation = null;
        }

        public class NullcDebugger : IDkmCallStackFilter
        {
            internal static T GetOrCreateDataItem<T>(DkmDataContainer container) where T : DkmDataItem, new()
            {
                T item = container.GetDataItem<T>();

                if (item != null)
                    return item;

                item = new T();

                container.SetDataItem<T>(DkmDataCreationDisposition.CreateNew, item);

                return item;
            }

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
                    var processData = GetOrCreateDataItem<DebugProcessDataItem>(input.Thread.Process);

                    InitNullcDebugFunctions(processData, input.RuntimeInstance);

                    if (processData.nullcDebugGetNativeAddressLocation == null)
                        return new DkmStackWalkFrame[1] { input };

                    string stackFrameDesc = ExecuteExpression($"((char*(*)(void*)){processData.nullcDebugGetNativeAddressLocation})((void*)0x{input.InstructionAddress.CPUInstructionPart.InstructionPointer:X}),sb", stackContext, input);

                    if (stackFrameDesc != null)
                    {
                        //stackFrameDesc = $"{input.InstructionAddress.CPUInstructionPart.InstructionPointer:X} {stackFrameDesc}";

                        DkmStackWalkFrame frame = DkmStackWalkFrame.Create(stackContext.Thread, input.InstructionAddress, input.FrameBase, input.FrameSize, input.Flags, stackFrameDesc, input.Registers, input.Annotations);

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
    }
}