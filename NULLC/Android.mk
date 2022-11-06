LOCAL_PATH := $(strip $(call my-dir))/..

$(info $(APP_ABI))

include $(CLEAR_VARS)

LOCAL_LDLIBS    := -llog

LOCAL_MODULE    := nullc

# nullc
LOCAL_SRC_FILES := NULLC/BinaryCache.cpp
LOCAL_SRC_FILES += NULLC/Bytecode.cpp
LOCAL_SRC_FILES += NULLC/Compiler.cpp
LOCAL_SRC_FILES += NULLC/Executor_Common.cpp
LOCAL_SRC_FILES += NULLC/Executor_RegVm.cpp
LOCAL_SRC_FILES += NULLC/ExpressionEval.cpp
LOCAL_SRC_FILES += NULLC/ExpressionGraph.cpp
LOCAL_SRC_FILES += NULLC/ExpressionTranslate.cpp
LOCAL_SRC_FILES += NULLC/ExpressionTree.cpp
LOCAL_SRC_FILES += NULLC/InstructionTreeLlvm.cpp
LOCAL_SRC_FILES += NULLC/InstructionTreeRegVm.cpp
LOCAL_SRC_FILES += NULLC/InstructionTreeRegVmLower.cpp
LOCAL_SRC_FILES += NULLC/InstructionTreeRegVmLowerGraph.cpp
LOCAL_SRC_FILES += NULLC/InstructionTreeVm.cpp
LOCAL_SRC_FILES += NULLC/InstructionTreeVmCommon.cpp
LOCAL_SRC_FILES += NULLC/InstructionTreeVmEval.cpp
LOCAL_SRC_FILES += NULLC/InstructionTreeVmGraph.cpp
LOCAL_SRC_FILES += NULLC/Lexer.cpp
LOCAL_SRC_FILES += NULLC/Linker.cpp
LOCAL_SRC_FILES += NULLC/nullc.cpp
LOCAL_SRC_FILES += NULLC/ParseGraph.cpp
LOCAL_SRC_FILES += NULLC/ParseTree.cpp
LOCAL_SRC_FILES += NULLC/stdafx.cpp
LOCAL_SRC_FILES += NULLC/StdLib.cpp
LOCAL_SRC_FILES += NULLC/StrAlgo.cpp
LOCAL_SRC_FILES += NULLC/TypeTree.cpp

# nullc x86 jit
ifeq ($(APP_ABI),x86)
	LOCAL_SRC_FILES += NULLC/CodeGen_X86.cpp
	LOCAL_SRC_FILES += NULLC/CodeGenRegVm_X86.cpp
	LOCAL_SRC_FILES += NULLC/Executor_X86.cpp
	LOCAL_SRC_FILES += NULLC/Instruction_X86.cpp
	LOCAL_SRC_FILES += NULLC/Translator_X86.cpp
else ifeq ($(APP_ABI),x86_64)
	LOCAL_SRC_FILES += NULLC/CodeGen_X86.cpp
	LOCAL_SRC_FILES += NULLC/CodeGenRegVm_X86.cpp
	LOCAL_SRC_FILES += NULLC/Executor_X86.cpp
	LOCAL_SRC_FILES += NULLC/Instruction_X86.cpp
	LOCAL_SRC_FILES += NULLC/Translator_X86.cpp
endif

# ext
LOCAL_SRC_FILES += NULLC/includes/pugi.cpp
LOCAL_SRC_FILES += external/pugixml/pugixml.cpp

# img
LOCAL_SRC_FILES += NULLC/includes/canvas.cpp

# old
LOCAL_SRC_FILES += NULLC/includes/vector.cpp

# std
LOCAL_SRC_FILES += NULLC/includes/dynamic.cpp
LOCAL_SRC_FILES += NULLC/includes/error.cpp
LOCAL_SRC_FILES += NULLC/includes/file.cpp
LOCAL_SRC_FILES += NULLC/includes/gc.cpp
LOCAL_SRC_FILES += NULLC/includes/io.cpp
LOCAL_SRC_FILES += NULLC/includes/math.cpp
LOCAL_SRC_FILES += NULLC/includes/memory.cpp
LOCAL_SRC_FILES += NULLC/includes/random.cpp
LOCAL_SRC_FILES += NULLC/includes/string.cpp
LOCAL_SRC_FILES += NULLC/includes/time.cpp
LOCAL_SRC_FILES += NULLC/includes/typeinfo.cpp

# win
# LOCAL_SRC_FILES += NULLC/includes/window.cpp

# JNI
LOCAL_SRC_FILES += NULLC/nullc_android.cpp

# dyncall
LOCAL_SRC_FILES += external/dyncall/dyncall_api.c
LOCAL_SRC_FILES += external/dyncall/dyncall_callvm.c
LOCAL_SRC_FILES += external/dyncall/dyncall_callvm_base.c
LOCAL_SRC_FILES += external/dyncall/dyncall_struct.c
LOCAL_SRC_FILES += external/dyncall/dyncall_vector.c

LOCAL_SRC_FILES += external/dyncall/dyncall_call.S

# tests
# LOCAL_CFLAGS += -DBUILD_TESTS
# LOCAL_SRC_FILES += $(wildcard $(LOCAL_PATH)/tests/Test*.cpp)
# LOCAL_SRC_FILES += tests/UnitTests.cpp

LOCAL_ARM_MODE := arm

LOCAL_CFLAGS += -DARM -fsigned-char -fno-omit-frame-pointer

include $(BUILD_SHARED_LIBRARY)
