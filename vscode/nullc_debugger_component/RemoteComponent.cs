using Microsoft.VisualStudio.Debugger;
using Microsoft.VisualStudio.Debugger.Breakpoints;
using Microsoft.VisualStudio.Debugger.ComponentInterfaces;
using Microsoft.VisualStudio.Debugger.CustomRuntimes;
using Microsoft.VisualStudio.Debugger.Evaluation;
using Microsoft.VisualStudio.Debugger.Native;
using Microsoft.VisualStudio.Debugger.Symbols;
using System;

namespace nullc_debugger_component
{
    namespace DkmDebugger
    {
        internal class NullcRemoteProcessDataItem : DkmDataItem
        {
            public DkmRuntimeInstanceId runtimeId;
            public DkmCustomRuntimeInstance runtimeInstance = null;
            public DkmNativeRuntimeInstance nativeRuntimeInstance = null;

            public DkmModuleId moduleId;
            public DkmCompilerId compilerId;

            public DkmLanguage language;

            public DkmModule module = null;
            public DkmCustomModuleInstance moduleInstance = null;
            public DkmNativeModuleInstance nativeModuleInstance = null;
        }

        public class NullcRemoteComponent : IDkmProcessExecutionNotification, IDkmModuleUserCodeDeterminer
        {
            void IDkmProcessExecutionNotification.OnProcessPause(DkmProcess process, DkmProcessExecutionCounters processCounters)
            {
                try
                {
                    ulong moduleBase = DebugHelpers.ReadPointerVariable(process, "nullcModuleStartAddress").GetValueOrDefault(0);

                    uint moduleSize = (uint)(DebugHelpers.ReadPointerVariable(process, "nullcModuleEndAddress").GetValueOrDefault(0) - moduleBase);

                    var processData = DebugHelpers.GetOrCreateDataItem<NullcRemoteProcessDataItem>(process);

                    if (moduleBase == 0 || moduleSize == 0)
                        return;

                    if (processData.runtimeInstance == null && processData.nativeRuntimeInstance == null)
                    {
                        processData.runtimeId = new DkmRuntimeInstanceId(DebugHelpers.NullcRuntimeGuid, 0);

                        if (DebugHelpers.useNativeInterfaces)
                            processData.nativeRuntimeInstance = DkmNativeRuntimeInstance.Create(process, processData.runtimeId, DkmRuntimeCapabilities.None, process.GetNativeRuntimeInstance(), null);
                        else
                            processData.runtimeInstance = DkmCustomRuntimeInstance.Create(process, processData.runtimeId, null);
                    }

                    if (processData.module == null)
                    {
                        processData.moduleId = new DkmModuleId(Guid.NewGuid(), DebugHelpers.NullcSymbolProviderGuid);

                        processData.compilerId = new DkmCompilerId(DebugHelpers.NullcCompilerGuid, DebugHelpers.NullcLanguageGuid);

                        processData.language = DkmLanguage.Create("nullc", processData.compilerId);

                        processData.module = DkmModule.Create(processData.moduleId, "nullc.embedded.code", processData.compilerId, process.Connection, null);
                    }

                    if (processData.moduleInstance == null && processData.nativeModuleInstance == null)
                    {
                        DkmDynamicSymbolFileId symbolFileId = DkmDynamicSymbolFileId.Create(DebugHelpers.NullcSymbolProviderGuid);

                        if (DebugHelpers.useNativeInterfaces)
                        {
                            processData.nativeModuleInstance = DkmNativeModuleInstance.Create("nullc", "nullc.embedded.code", 0, null, symbolFileId, DkmModuleFlags.None, DkmModuleMemoryLayout.Unknown, 1, "nullc embedded code", processData.nativeRuntimeInstance, moduleBase, moduleSize, Microsoft.VisualStudio.Debugger.Clr.DkmClrHeaderStatus.NativeBinary, false, null, null, null);

                            processData.nativeModuleInstance.SetModule(processData.module, true); // Can use reload?
                        }
                        else
                        {
                            processData.moduleInstance = DkmCustomModuleInstance.Create("nullc", "nullc.embedded.code", 0, processData.runtimeInstance, null, symbolFileId, DkmModuleFlags.None, DkmModuleMemoryLayout.Unknown, moduleBase, 1, moduleSize, "nullc embedded code", false, null, null, null);

                            processData.moduleInstance.SetModule(processData.module, true); // Can use reload?
                        }
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
                var processData = DebugHelpers.GetOrCreateDataItem<NullcRemoteProcessDataItem>(moduleInstance.Process);

                if (processData != null)
                {
                    if (moduleInstance.LoadContext == "nullc embedded code")
                        return true;
                }

                return moduleInstance.IsUserCode();
            }
        }
    }
}
