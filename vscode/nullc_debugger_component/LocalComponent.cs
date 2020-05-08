using Microsoft.VisualStudio.Debugger;
using Microsoft.VisualStudio.Debugger.CallStack;
using Microsoft.VisualStudio.Debugger.ComponentInterfaces;
using Microsoft.VisualStudio.Debugger.CustomRuntimes;
using Microsoft.VisualStudio.Debugger.DefaultPort;
using Microsoft.VisualStudio.Debugger.Evaluation;
using Microsoft.VisualStudio.Debugger.Symbols;
using System;

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
        }

        public class NullcLocalComponent : IDkmSymbolCompilerIdQuery, IDkmSymbolDocumentCollectionQuery, IDkmSymbolDocumentSpanQuery, IDkmSymbolQuery, IDkmLanguageFrameDecoder, IDkmModuleInstanceLoadNotification
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
                var processData = DebugHelpers.GetOrCreateDataItem<NullcLocalProcessDataItem>(inspectionSession.Process);

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
                var processData = DebugHelpers.GetOrCreateDataItem<NullcLocalProcessDataItem>(moduleInstance.Process);

                if (processData.moduleBytecodeLocation == 0)
                {
                    processData.moduleBytecodeLocation = DebugHelpers.ReadUlongVariable(moduleInstance.Process, "nullcModuleBytecodeLocation").GetValueOrDefault(0);
                    processData.moduleBytecodeSize = DebugHelpers.ReadUlongVariable(moduleInstance.Process, "nullcModuleBytecodeSize").GetValueOrDefault(0);

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
