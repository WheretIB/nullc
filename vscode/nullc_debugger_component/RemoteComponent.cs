using Microsoft.VisualStudio.Debugger;
using Microsoft.VisualStudio.Debugger.Breakpoints;
using Microsoft.VisualStudio.Debugger.CallStack;
using Microsoft.VisualStudio.Debugger.ComponentInterfaces;
using Microsoft.VisualStudio.Debugger.CustomRuntimes;
using Microsoft.VisualStudio.Debugger.Evaluation;
using Microsoft.VisualStudio.Debugger.Exceptions;
using Microsoft.VisualStudio.Debugger.Native;
using Microsoft.VisualStudio.Debugger.Stepping;
using Microsoft.VisualStudio.Debugger.Symbols;
using System;
using System.Collections.Generic;
using System.Diagnostics;

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

            public ulong moduleBytecodeLocation = 0;
            public ulong moduleBytecodeSize = 0;
            public byte[] moduleBytecodeRaw;

            public NullcBytecode bytecode;

            // A list of addresses where breakpoints are enabled so when we receive a breakpoint exception we can schedule a restore of the debug break instruction
            public List<ulong> activeBreakpointLocations = new List<ulong>();

            // Address of the instruction when the last breakpoint was hit in order to schedule a single-step followed by the debug break instruction restore
            public ulong lastHitBreakpointLocation = 0;

            // Thread where single step must be performed when process continues execution
            public DkmThread singleStepThread;

            // In the stepper implementation, these are the active internal breakpoints
            public DkmRuntimeInstructionBreakpoint stepBreakpoint = null;
            public DkmRuntimeInstructionBreakpoint secondaryStepBreakpoint = null;
        }

        // Since we have a custom runtime, we must manualy implement 'simple' x86 breakpoints
        // Breakpoint handling is performed in 4 stages
        // When breakpoint is enabled, a debug break instruction is placed at the target (int 3)
        // When breakpoint is hit, IDkmDebugMonitorExceptionNotification.OnDebugMonitorException is raised, but we will suppress it while still notifying the debugger that an embedded breakpoint was hit and will mark lastHitBreakpointLocation and thread
        // At this stage the process will break, and someone restores the original instruction value (can't tell who)
        // When process is resumed by user, IDkmProcessExecutionNotification.OnProcessResume is raised and we will schedule a single step on the thread where breakpoint is hit
        // When single step is completed, IDkmSingleStepCompleteReceived.OnSingleStepCompleteReceived is raised and we will restore the debug break instruction

        public class NullcRemoteComponent : IDkmProcessExecutionNotification, IDkmModuleUserCodeDeterminer, IDkmRuntimeMonitorBreakpointHandler, IDkmRuntimeStepper, IDkmDebugMonitorExceptionNotification, IDkmSingleStepCompleteReceived
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

                    if (processData.moduleBytecodeLocation == 0)
                    {
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
                }
                catch (Exception ex)
                {
                    Console.WriteLine("OnProcessPause failed with: " + ex.ToString());
                }
            }

            void IDkmProcessExecutionNotification.OnProcessResume(DkmProcess process, DkmProcessExecutionCounters processCounters)
            {
                var processData = DebugHelpers.GetOrCreateDataItem<NullcRemoteProcessDataItem>(process);

                // Third stage of breakpoint handling - schedule a single step so we will move from the instruction location where debug break instruction must be restored (int 3)
                if (processData.lastHitBreakpointLocation != 0)
                {
                    var singleStepRequest = DkmSingleStepRequest.Create(DebugHelpers.NullcStepperBreakpointSourceId, processData.singleStepThread);

                    singleStepRequest.EnableSingleStep();
                }
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

            internal class NullcBreakpointDataItem : DkmDataItem
            {
                public DkmNativeInstructionAddress instructionAddress = null;
                public byte[] prevValue = null;
            }

            void IDkmRuntimeMonitorBreakpointHandler.EnableRuntimeBreakpoint(DkmRuntimeBreakpoint runtimeBreakpoint)
            {
                var instructionBreakpoint = (runtimeBreakpoint as DkmRuntimeInstructionBreakpoint);

                if (instructionBreakpoint != null)
                {
                    var nativeInstructionAddress = instructionBreakpoint.InstructionAddress as DkmNativeInstructionAddress;

                    if (nativeInstructionAddress == null)
                    {
                        Debug.WriteLine("Missing breakpoint address");
                        return;
                    }

                    ulong address = instructionBreakpoint.InstructionAddress.CPUInstructionPart.InstructionPointer;

                    byte[] prevValue = new byte[1];

                    if (runtimeBreakpoint.Process.ReadMemory(address, DkmReadMemoryFlags.ExecutableOnly, prevValue) == 0)
                    {
                        Debug.WriteLine("Failed to read current instruction");
                        return;
                    }

                    var breakpointData = DebugHelpers.GetOrCreateDataItem<NullcBreakpointDataItem>(runtimeBreakpoint);

                    breakpointData.instructionAddress = nativeInstructionAddress;
                    breakpointData.prevValue = prevValue;

                    // Write an 'int 3' instruction
                    runtimeBreakpoint.Process.InvisibleWriteMemory(address, new byte[1] { 0xcc });

                    // Remember locations of the active user breakpoint instructions, skip internal breakpoints used in stepper
                    if (runtimeBreakpoint.SourceId != DebugHelpers.NullcStepperBreakpointSourceId)
                    {
                        var processData = DebugHelpers.GetOrCreateDataItem<NullcRemoteProcessDataItem>(instructionBreakpoint.Process);

                        if (!processData.activeBreakpointLocations.Contains(address))
                            processData.activeBreakpointLocations.Add(address);
                    }

                    return;
                }

                Debug.WriteLine("Failed to enable the breakpoint");
            }

            void IDkmRuntimeMonitorBreakpointHandler.TestRuntimeBreakpoint(DkmRuntimeBreakpoint runtimeBreakpoint)
            {
                // Don't undestand what is expected here, there is no return value and no completion handler
            }

            void IDkmRuntimeMonitorBreakpointHandler.DisableRuntimeBreakpoint(DkmRuntimeBreakpoint runtimeBreakpoint)
            {
                var breakpointData = runtimeBreakpoint.GetDataItem<NullcBreakpointDataItem>();

                if (breakpointData != null)
                {
                    var processData = DebugHelpers.GetOrCreateDataItem<NullcRemoteProcessDataItem>(runtimeBreakpoint.Process);

                    var address = breakpointData.instructionAddress.CPUInstructionPart.InstructionPointer;

                    runtimeBreakpoint.Process.InvisibleWriteMemory(breakpointData.instructionAddress.CPUInstructionPart.InstructionPointer, breakpointData.prevValue);

                    // Skip internal breakpoints used in stepper
                    if (runtimeBreakpoint.SourceId != DebugHelpers.NullcStepperBreakpointSourceId)
                    {
                        if (processData.activeBreakpointLocations.Contains(address))
                            processData.activeBreakpointLocations.Remove(address);
                    }
                }
            }

            DkmRuntimeInstructionBreakpoint PlaceBreakpointAtAddress(NullcRemoteProcessDataItem processData, DkmThread thread, ulong instructionAddress)
            {
                var dkmInstruction = DkmNativeInstructionAddress.Create(processData.nativeRuntimeInstance, processData.nativeModuleInstance, (uint)(instructionAddress - processData.nativeModuleInstance.BaseAddress), new DkmInstructionAddress.CPUInstruction(instructionAddress));

                var stepBreakpoint = DkmRuntimeInstructionBreakpoint.Create(DebugHelpers.NullcStepperBreakpointSourceId, thread, dkmInstruction, false, null);

                stepBreakpoint.Enable();

                return stepBreakpoint;
            }

            void ClearStepBreakpoints(NullcRemoteProcessDataItem processData)
            {
                if (processData.stepBreakpoint != null)
                {
                    // Even though documentation for Close says that the breakpoint will be implicitly closed it doesn't, so we disable it manually
                    processData.stepBreakpoint.Disable();

                    processData.stepBreakpoint.Close();
                    processData.stepBreakpoint = null;
                }

                if (processData.secondaryStepBreakpoint != null)
                {
                    processData.secondaryStepBreakpoint.Disable();

                    processData.secondaryStepBreakpoint.Close();
                    processData.secondaryStepBreakpoint = null;
                }
            }

            ulong GetReturnAddress(DkmThread thread)
            {
                // GetCurrentFrameInfo should provide the return address but the result is actually the current instruction address... so we have to go and lookup stack frame data at [ebp+4]
                var frameRegs = thread.GetCurrentRegisters(new DkmUnwoundRegister[0]);

                ulong address = 0;

                var x86Regs = frameRegs as DkmX86FrameRegisters;

                if (x86Regs != null)
                {
                    byte[] ebpData = new byte[4];
                    if (x86Regs.GetRegisterValue(22, ebpData) == 4)
                        address = DebugHelpers.ReadPointerVariable(thread.Process, BitConverter.ToUInt32(ebpData, 0) + 4).GetValueOrDefault(0);
                }

                if (address == 0)
                    thread.GetCurrentFrameInfo(out address, out _, out _);

                return address;
            }

            void IDkmRuntimeStepper.BeforeEnableNewStepper(DkmRuntimeInstance runtimeInstance, DkmStepper stepper)
            {
                // Don't have anything to do here
            }

            bool IDkmRuntimeStepper.OwnsCurrentExecutionLocation(DkmRuntimeInstance runtimeInstance, DkmStepper stepper, DkmStepArbitrationReason reason)
            {
                var processData = DebugHelpers.GetOrCreateDataItem<NullcRemoteProcessDataItem>(runtimeInstance.Process);

                // Can't handle steps without an address
                if (stepper.StartingAddress == null)
                    return false;

                var instructionAddress = stepper.StartingAddress.CPUInstructionPart.InstructionPointer;

                if (DebugHelpers.useNativeInterfaces)
                {
                    if (processData.nativeModuleInstance != null)
                    {
                        if (instructionAddress >= processData.nativeModuleInstance.BaseAddress && instructionAddress < processData.nativeModuleInstance.BaseAddress + processData.nativeModuleInstance.Size)
                            return processData.bytecode.ConvertNativeAddressToInstruction(instructionAddress) != 0;
                    }
                }
                else
                {
                    if (processData.moduleInstance != null)
                    {
                        if (instructionAddress >= processData.moduleInstance.BaseAddress && instructionAddress < processData.moduleInstance.BaseAddress + processData.moduleInstance.Size)
                            return processData.bytecode.ConvertNativeAddressToInstruction(instructionAddress) != 0;
                    }
                }

                return false;
            }

            void IDkmRuntimeStepper.Step(DkmRuntimeInstance runtimeInstance, DkmStepper stepper, DkmStepArbitrationReason reason)
            {
                if (!(this as IDkmRuntimeStepper).OwnsCurrentExecutionLocation(runtimeInstance, stepper, reason))
                {
                    runtimeInstance.Step(stepper, reason);
                    return;
                }

                var processData = DebugHelpers.GetOrCreateDataItem<NullcRemoteProcessDataItem>(runtimeInstance.Process);

                if (stepper.StepKind == DkmStepKind.StepIntoSpecific)
                    throw new NotSupportedException();

                if (stepper.StepKind == DkmStepKind.Over || stepper.StepKind == DkmStepKind.Into)
                {
                    int nullcInstruction = processData.bytecode.ConvertNativeAddressToInstruction(stepper.StartingAddress.CPUInstructionPart.InstructionPointer);

                    int line = processData.bytecode.GetInstructionSourceLocationLine(nullcInstruction, out int moduleIndex);

                    // Instruction based stepping
                    int nextNullcInstruction = nullcInstruction;
                    int secondaryNullcInstruction = 0;

                    while (nextNullcInstruction < processData.bytecode.instructions.Count)
                    {
                        // Walk forward until an instruction with a different line is hit
                        if (processData.bytecode.GetInstructionSourceLocationLine(nextNullcInstruction, out _) != line)
                            break;

                        var nextInstruction = processData.bytecode.instructions[nextNullcInstruction];

                        // If there is a jump, place a breakpoint at the target (even if it's on the same line)
                        if (nextInstruction.code == NullcInstructionCode.rviJmp)
                        {
                            nextNullcInstruction = (int)nextInstruction.argument;
                            break;
                        }

                        // If there is a conditional jump, place two breakpoints - one after it and one at the target (even if either of them is on the same line)
                        if (nextInstruction.code == NullcInstructionCode.rviJmpz || nextInstruction.code == NullcInstructionCode.rviJmpnz)
                        {
                            secondaryNullcInstruction = nextNullcInstruction + 1;
                            nextNullcInstruction = (int)nextInstruction.argument;
                            break;
                        }

                        // If there is a return, I wish I could just fallback to parent runtime to handle it, may be required to place a breakpoint at the call site
                        if (nextInstruction.code == NullcInstructionCode.rviReturn)
                        {
                            ulong address = GetReturnAddress(stepper.Thread);

                            // Address must be within nullc module or we will have to cancel the step
                            if (address >= processData.nativeModuleInstance.BaseAddress && address < processData.nativeModuleInstance.BaseAddress + processData.nativeModuleInstance.Size)
                            {
                                processData.stepBreakpoint = PlaceBreakpointAtAddress(processData, stepper.Thread, address);
                            }
                            else
                            {
                                stepper.OnStepComplete(stepper.Thread, false);
                            }

                            return;
                        }

                        if (stepper.StepKind == DkmStepKind.Into)
                        {
                            if (nextInstruction.code == NullcInstructionCode.rviCall && stepper.StepKind == DkmStepKind.Into)
                            {
                                NullcFuncInfo function = processData.bytecode.functions[(int)nextInstruction.argument];

                                if (function.regVmAddress != ~0u)
                                {
                                    secondaryNullcInstruction = nextNullcInstruction + 1;
                                    nextNullcInstruction = (int)function.regVmAddress;
                                    break;
                                }
                                else
                                {
                                    // TODO: Step into an external call
                                    throw new NotImplementedException();
                                }
                            }
                            else if (nextInstruction.code == NullcInstructionCode.rviCallPtr)
                            {
                                // TODO: Step into a function pointer call
                                throw new NotImplementedException();
                            }
                        }

                        nextNullcInstruction++;
                    }

                    ClearStepBreakpoints(processData);

                    if (nextNullcInstruction != 0)
                    {
                        ulong instructionAddress = processData.bytecode.ConvertInstructionToNativeAddress(nextNullcInstruction);

                        processData.stepBreakpoint = PlaceBreakpointAtAddress(processData, stepper.Thread, instructionAddress);
                    }

                    if (secondaryNullcInstruction != 0)
                    {
                        ulong instructionAddress = processData.bytecode.ConvertInstructionToNativeAddress(secondaryNullcInstruction);

                        processData.secondaryStepBreakpoint = PlaceBreakpointAtAddress(processData, stepper.Thread, instructionAddress);
                    }
                }
                else if (stepper.StepKind == DkmStepKind.Out)
                {
                    ulong address = GetReturnAddress(stepper.Thread);

                    processData.stepBreakpoint = PlaceBreakpointAtAddress(processData, stepper.Thread, address);
                }
                else
                {
                    throw new NotImplementedException();
                }
            }

            void IDkmRuntimeStepper.StopStep(DkmRuntimeInstance runtimeInstance, DkmStepper stepper)
            {
                var processData = DebugHelpers.GetOrCreateDataItem<NullcRemoteProcessDataItem>(runtimeInstance.Process);

                ClearStepBreakpoints(processData);
            }

            void IDkmRuntimeStepper.AfterSteppingArbitration(DkmRuntimeInstance runtimeInstance, DkmStepper stepper, DkmStepArbitrationReason reason, DkmRuntimeInstance newControllingRuntimeInstance)
            {
                // Don't have anything to do here
            }

            void IDkmRuntimeStepper.OnNewControllingRuntimeInstance(DkmRuntimeInstance runtimeInstance, DkmStepper stepper, DkmStepArbitrationReason reason, DkmRuntimeInstance controllingRuntimeInstance)
            {
                // Don't have anything to do here
            }

            bool IDkmRuntimeStepper.StepControlRequested(DkmRuntimeInstance runtimeInstance, DkmStepper stepper, DkmStepArbitrationReason reason, DkmRuntimeInstance callingRuntimeInstance)
            {
                return true;
            }

            void IDkmRuntimeStepper.TakeStepControl(DkmRuntimeInstance runtimeInstance, DkmStepper stepper, bool leaveGuardsInPlace, DkmStepArbitrationReason reason, DkmRuntimeInstance callingRuntimeInstance)
            {
                runtimeInstance.StopStep(stepper);
            }

            void IDkmRuntimeStepper.NotifyStepComplete(DkmRuntimeInstance runtimeInstance, DkmStepper stepper)
            {
                var processData = DebugHelpers.GetOrCreateDataItem<NullcRemoteProcessDataItem>(runtimeInstance.Process);

                ClearStepBreakpoints(processData);
            }

            void IDkmDebugMonitorExceptionNotification.OnDebugMonitorException(DkmExceptionInformation exception, DkmWorkList workList, DkmEventDescriptorS eventDescriptor)
            {
                var processData = DebugHelpers.GetOrCreateDataItem<NullcRemoteProcessDataItem>(exception.Process);

                ulong address = exception.InstructionAddress.CPUInstructionPart.InstructionPointer;

                // Breakpoint
                if (exception.Code == 0x80000003)
                {
                    // Internal stepper breakpoints are silenced but te debugger is notified to break the process
                    if (processData.stepBreakpoint != null && processData.stepBreakpoint.InstructionAddress.CPUInstructionPart.InstructionPointer == address)
                    {
                        eventDescriptor.Suppress();

                        exception.Thread.OnEmbeddedBreakpointHit(exception.InstructionAddress, false);
                    }

                    if (processData.secondaryStepBreakpoint != null && processData.secondaryStepBreakpoint.InstructionAddress.CPUInstructionPart.InstructionPointer == address)
                    {
                        eventDescriptor.Suppress();

                        exception.Thread.OnEmbeddedBreakpointHit(exception.InstructionAddress, false);
                    }

                    // Second stage of breakpoint handling - supress exception, notify debugger to break the process and remember what breakpoint has to be resotred later
                    if (processData.activeBreakpointLocations.Contains(address))
                    {
                        eventDescriptor.Suppress();

                        exception.Thread.OnEmbeddedBreakpointHit(exception.InstructionAddress, false);

                        // Schedule restore of this breakpoint when process continues
                        processData.lastHitBreakpointLocation = address;

                        processData.singleStepThread = exception.Thread;
                    }
                }
            }

            void IDkmSingleStepCompleteReceived.OnSingleStepCompleteReceived(DkmSingleStepRequest singleStepRequest, DkmEventDescriptorS eventDescriptor)
            {
                var processData = DebugHelpers.GetOrCreateDataItem<NullcRemoteProcessDataItem>(singleStepRequest.Thread.Process);

                // Last stage of breakpoint handling - restore debug break instruction now that we have moved from the breakpoint location
                if (processData.lastHitBreakpointLocation != 0)
                {
                    // Restore breakpoint instruction
                    singleStepRequest.Thread.Process.InvisibleWriteMemory(processData.lastHitBreakpointLocation, new byte[1] { 0xcc });

                    processData.lastHitBreakpointLocation = 0;

                    eventDescriptor.Suppress();
                }
            }
        }
    }
}
