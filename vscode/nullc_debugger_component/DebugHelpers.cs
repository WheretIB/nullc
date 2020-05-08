using System;
using Microsoft.VisualStudio.Debugger;
using Microsoft.VisualStudio.Debugger.DefaultPort;
using Microsoft.VisualStudio.Debugger.Native;

namespace nullc_debugger_component
{
    namespace DkmDebugger
    {
        static class DebugHelpers
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

            internal static string FindFunctionAddress(DkmRuntimeInstance runtimeInstance, string name)
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
    }
}
