using Microsoft.VisualStudio.Debugger;
using Microsoft.VisualStudio.Debugger.ComponentInterfaces;
using Microsoft.VisualStudio.Debugger.CustomRuntimes;
using Microsoft.VisualStudio.Debugger.Evaluation;
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

            public DkmModuleId moduleId;
            public DkmCompilerId compilerId;

            public DkmLanguage language;

            public DkmModule module = null;
            public DkmCustomModuleInstance moduleInstance = null;
        }

        public class NullcRemoteComponent : IDkmProcessExecutionNotification, IDkmModuleUserCodeDeterminer
        {
            void IDkmProcessExecutionNotification.OnProcessPause(DkmProcess process, DkmProcessExecutionCounters processCounters)
            {
                try
                {
                    ulong moduleBase = DebugHelpers.ReadUlongVariable(process, "nullcModuleStartAddress").GetValueOrDefault(0);

                    uint moduleSize = (uint)DebugHelpers.ReadUlongVariable(process, "nullcModuleEndAddress").GetValueOrDefault(0);

                    var processData = DebugHelpers.GetOrCreateDataItem<NullcRemoteProcessDataItem>(process);

                    if (moduleBase == 0 || moduleSize == 0)
                        return;

                    if (processData.runtimeInstance == null)
                    {
                        processData.runtimeId = new DkmRuntimeInstanceId(DebugHelpers.NullcRuntimeGuid, 0);

                        processData.runtimeInstance = DkmCustomRuntimeInstance.Create(process, processData.runtimeId, null);
                    }

                    if (processData.module == null)
                    {
                        processData.moduleId = new DkmModuleId(Guid.NewGuid(), DebugHelpers.NullcSymbolProviderGuid);

                        processData.compilerId = new DkmCompilerId(DebugHelpers.NullcCompilerGuid, DebugHelpers.NullcLanguageGuid);

                        processData.language = DkmLanguage.Create("nullc", processData.compilerId);

                        processData.module = DkmModule.Create(processData.moduleId, "nullc.embedded.code", processData.compilerId, process.Connection, null);
                    }

                    if (processData.moduleInstance == null)
                    {
                        DkmDynamicSymbolFileId symbolFileId = DkmDynamicSymbolFileId.Create(DebugHelpers.NullcSymbolProviderGuid);

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
