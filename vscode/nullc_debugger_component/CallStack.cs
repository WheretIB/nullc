using Microsoft.VisualStudio.Debugger;
using System.Collections.Generic;

namespace nullc_debugger_component
{
    namespace DkmDebugger
    {
        class NullcCallStackEntry
        {
            public int instruction = 0;
            public NullcFuncInfo function = null;
            public int dataOffset = 0;
            public int dataSize = 0;
        }

        class NullcCallStack
        {
            public List<NullcCallStackEntry> callStack;

            public void UpdateFrom(DkmProcess process, ulong callStackBase, ulong callStackTop, NullcBytecode bytecode)
            {
                int count = (int)(callStackTop - callStackBase) / DebugHelpers.GetPointerSize(process);

                callStack = new List<NullcCallStackEntry>();

                int dataOffset = 0;

                for (int i = 0; i < count; i++)
                {
                    var entry = new NullcCallStackEntry();

                    entry.instruction = DebugHelpers.ReadIntVariable(process, callStackBase + (ulong)(i * 4)).GetValueOrDefault(0);

                    entry.function = bytecode.GetFunctionAtAddress(entry.instruction);

                    int alignOffset = (dataOffset % 16 != 0) ? (16 - (dataOffset % 16)) : 0;

                    dataOffset += alignOffset;

                    entry.dataOffset = dataOffset;

                    if (entry.function != null)
                        entry.dataSize = (entry.function.stackSize + 0xf) & ~0xf;
                    else
                        entry.dataSize = bytecode.globalVariableSize;

                    dataOffset += entry.dataSize;

                    callStack.Add(entry);
                }
            }
        }
    }
}
