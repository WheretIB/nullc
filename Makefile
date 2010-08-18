# Makefile for NULLC
# Conrad Steenberg <conrad.steenberg@gmail.com>
# Aug 3, 2010

REG_CFLAGS=-g -W
COMP_CFLAGS=-g -W -D NULLC_NO_EXECUTOR

LIB_SOURCES = \
  NULLC/BinaryCache.cpp \
  NULLC/Bytecode.cpp \
  NULLC/Callbacks.cpp \
  NULLC/CodeGen_X86.cpp \
  NULLC/CodeInfo.cpp \
  NULLC/Compiler.cpp \
  NULLC/Executor_Common.cpp \
  NULLC/Executor.cpp \
  NULLC/Executor_X86.cpp \
  NULLC/Lexer.cpp \
  NULLC/Linker.cpp \
  NULLC/nullc.cpp \
  NULLC/Parser.cpp \
  NULLC/stdafx.cpp \
  NULLC/StdLib.cpp \
  NULLC/StrAlgo.cpp \
  NULLC/SyntaxTree.cpp \
  NULLC/Translator_X86.cpp

LIB_TARGETS = \
  temp/BinaryCache.o \
  temp/Bytecode.o \
  temp/Callbacks.o \
  temp/CodeGen_X86.o \
  temp/CodeInfo.o \
  temp/Compiler.o \
  temp/Executor_Common.o \
  temp/Executor.o \
  temp/Executor_X86.o \
  temp/Lexer.o \
  temp/Linker.o \
  temp/nullc.o \
  temp/Parser.o \
  temp/stdafx.o \
  temp/StdLib.o \
  temp/StrAlgo.o \
  temp/SyntaxTree.o \
  temp/Translator_X86.o


STDLIB_SOURCES = \
  NULLC/includes/canvas.cpp \
  NULLC/includes/dynamic.cpp \
  NULLC/includes/file.cpp \
  NULLC/includes/gc.cpp \
  NULLC/includes/hashmap.cpp \
  NULLC/includes/io.cpp \
  NULLC/includes/list.cpp \
  NULLC/includes/map.cpp \
  NULLC/includes/math.cpp \
  NULLC/includes/pugi.cpp \
  NULLC/includes/random.cpp \
  NULLC/includes/string.cpp \
  NULLC/includes/time.cpp \
  NULLC/includes/typeinfo.cpp \
  NULLC/includes/vector.cpp

PUGIXML_SOURCES = \
  external/pugixml/pugixml.cpp


STDLIB_TARGETS = \
  temp/lib/canvas.o \
  temp/lib/dynamic.o \
  temp/lib/file.o \
  temp/lib/gc.o \
  temp/lib/hashmap.o \
  temp/lib/io.o \
  temp/lib/list.o \
  temp/lib/map.o \
  temp/lib/math.o \
  temp/lib/pugi.o \
  temp/lib/random.o \
  temp/lib/string.o \
  temp/lib/time.o \
  temp/lib/typeinfo.o \
  temp/lib/vector.o

PUGIXML_TARGETS = \
  temp/pugixml.o

all: temp/.dummy temp/compiler/.dummy temp/lib/.dummy temp/tests/.dummy \
     bin/nullcl TestRun bin/ConsoleCalc

temp/lib/%.o: NULLC/includes/%.cpp
	$(CXX) $(REG_CFLAGS) -c $< -o $@

temp/%.o: NULLC/%.cpp
	$(CXX) $(REG_CFLAGS) -c $< -o $@

${PUGIXML_TARGETS}: $(PUGIXML_SOURCES)
	$(CXX) $(REG_CFLAGS) -c $< -o $@

#~ ${LIB_TARGETS}: ${LIB_SOURCES}
#~ $(CXX) $(REG_CFLAGS) -c $^ -o $@
#~
#~ ${STDLIB_TARGETS}: ${STDLIB_SOURCES}
#~ $(CXX) $(REG_CFLAGS) -c $^ -o $@

temp/.dummy:
	mkdir -p temp
	touch temp/.dummy

temp/compiler/.dummy:
	mkdir -p temp/compiler
	touch temp/compiler/.dummy

temp/lib/.dummy:
	mkdir -p temp/lib
	touch temp/lib/.dummy

temp/tests/.dummy:
	mkdir -p temp/tests
	touch temp/tests/.dummy

bin/libnullc.a: ${LIB_TARGETS} ${STDLIB_TARGETS} ${PUGIXML_TARGETS}
	$(AR) rcs $@ $^

clean:
	rm -rf temp/
	rm -f bin/*.a

# Compiling NULLC compiler-only lib
COMPILERLIB_SOURCES = \
  NULLC/BinaryCache.cpp \
  NULLC/Bytecode.cpp \
  NULLC/CodeInfo.cpp \
  NULLC/nullc.cpp \
  NULLC/stdafx.cpp \
  NULLC/StrAlgo.cpp \
  NULLC/Callbacks.cpp \
  NULLC/Compiler.cpp \
  NULLC/Lexer.cpp \
  NULLC/Parser.cpp \
  NULLC/SyntaxTree.cpp

COMPILERLIB_TARGETS = \
  temp/compiler/BinaryCache.o \
  temp/compiler/Bytecode.o \
  temp/compiler/CodeInfo.o \
  temp/compiler/nullc.o \
  temp/compiler/stdafx.o \
  temp/compiler/StrAlgo.o \
  temp/compiler/Callbacks.o \
  temp/compiler/Compiler.o \
  temp/compiler/Lexer.o \
  temp/compiler/Parser.o \
  temp/compiler/SyntaxTree.o

temp/compiler/%.o: NULLC/%.cpp
	$(CXX) $(COMP_CFLAGS) -c $< -o $@

bin/libnullc_cl.a: ${COMPILERLIB_TARGETS}
	$(AR) rcs $@ $^

temp/ConsoleCalc.o: ConsoleCalc/ConsoleCalc.cpp
	$(CXX) $(REG_CFLAGS) -c $< -o $@

bin/ConsoleCalc: temp/ConsoleCalc.o bin/libnullc.a
	$(CXX) $(REG_CFLAGS) -o $@ $< -Lbin  -lnullc

temp/main.o: nullcl/main.cpp bin/libnullc.a
	$(CXX) -c $(REG_CFLAGS) -o $@ $<

bin/nullcl: temp/main.o bin/libnullc.a bin/libnullc_cl.a
	$(CXX) $(REG_CFLAGS) -o $@ $<  -Lbin -lnullc_cl -lnullc

TEST_SOURCES = \
	TestRun.cpp \
	UnitTests.cpp \
	tests/TestAccessors.cpp          tests/TestInference.cpp \
	tests/TestArray.cpp              tests/TestInterface.cpp \
	tests/TestArraySpecial.cpp       tests/TestListComprehension.cpp \
	tests/TestAutoArray.cpp          tests/TestLocalClass.cpp \
	tests/TestAutoRefCall.cpp        tests/TestLongNames.cpp \
	tests/TestAutoRef.cpp            tests/TestLValue.cpp \
	tests/TestBase.cpp               tests/TestMembers.cpp \
	tests/TestCallTransitions.cpp    tests/TestMisc.cpp \
	tests/TestClasses.cpp            tests/TestModules.cpp \
	tests/TestClosures.cpp           tests/TestNew.cpp \
	tests/TestCompileFail.cpp        tests/TestNumerical.cpp \
	tests/TestConversions.cpp        tests/TestOverload.cpp \
	tests/TestCoroutine.cpp          tests/TestOverride.cpp \
	tests/TestCoroutineIterator.cpp  tests/TestParseFail.cpp \
	tests/TestCycles.cpp             tests/TestPointers.cpp \
	tests/TestDefault.cpp            tests/TestPostExpr.cpp \
	tests/TestExternalCall.cpp       tests/TestRuntimeFail.cpp \
	tests/TestExtra.cpp              tests/TestScope.cpp \
	tests/TestForEach.cpp            tests/TestSpecial.cpp \
	tests/TestFromReference.cpp      tests/TestSpecialOp.cpp \
	tests/TestFunctions.cpp          tests/TestSpeed.cpp \
	tests/TestGC.cpp                 tests/TestStackRealloc.cpp \
	tests/TestImplicitArray.cpp      tests/TestVarargs.cpp \
	tests/TestIndirectCall.cpp       tests/TestVariables.cpp

TEST_OBJECTS = \
  TestRun.o \
  UnitTests.o \
	temp/tests/TestAccessors.o          temp/tests/TestInference.o \
	temp/tests/TestArray.o              temp/tests/TestInterface.o \
	temp/tests/TestArraySpecial.o       temp/tests/TestListComprehension.o \
	temp/tests/TestAutoArray.o          temp/tests/TestLocalClass.o \
	temp/tests/TestAutoRefCall.o        temp/tests/TestLongNames.o \
	temp/tests/TestAutoRef.o            temp/tests/TestLValue.o \
	temp/tests/TestBase.o               temp/tests/TestMembers.o \
	temp/tests/TestCallTransitions.o    temp/tests/TestMisc.o \
	temp/tests/TestClasses.o            temp/tests/TestModules.o \
	temp/tests/TestClosures.o           temp/tests/TestNew.o \
	temp/tests/TestCompileFail.o        temp/tests/TestNumerical.o \
	temp/tests/TestConversions.o        temp/tests/TestOverload.o \
	temp/tests/TestCoroutine.o          temp/tests/TestOverride.o \
	temp/tests/TestCoroutineIterator.o  temp/tests/TestParseFail.o \
	temp/tests/TestCycles.o             temp/tests/TestPointers.o \
	temp/tests/TestDefault.o            temp/tests/TestPostExpr.o \
	temp/tests/TestExternalCall.o       temp/tests/TestRuntimeFail.o \
	temp/tests/TestExtra.o              temp/tests/TestScope.o \
	temp/tests/TestForEach.o            temp/tests/TestSpecial.o \
	temp/tests/TestFromReference.o      temp/tests/TestSpecialOp.o \
	temp/tests/TestFunctions.o          temp/tests/TestSpeed.o \
	temp/tests/TestGC.o                 temp/tests/TestStackRealloc.o \
	temp/tests/TestImplicitArray.o      temp/tests/TestVarargs.o \
	temp/tests/TestIndirectCall.o       temp/tests/TestVariables.o

%.o: %.cpp
	$(CXX) $(REG_CFLAGS) -o $@ -c $<

temp/tests/%.o: tests/%.cpp
	$(CXX) $(REG_CFLAGS) -o $@ -c $<

TestRun: ${TEST_OBJECTS} bin/libnullc.a
	$(CXX) $(REG_CFLAGS) -o $@ $(TEST_OBJECTS) -Lbin -lnullc



#~ g++ -c -g -W -D NULLC_NO_EXECUTOR
#~
#~ Building NULLC compiler-only lib
#~ ar rcs bin/libnullc_cl.a temp/BinaryCache.o temp/Bytecode.o temp/Callbacks.o temp/CodeInfo.o temp/Compiler.o temp/Lexer.o temp/nullc.o temp/Parser.o temp/stdafx.o temp/StrAlgo.o temp/SyntaxTree.o
#~ Building ConsoleCalc application
#~ g++ -g -W -Lbin -o bin/ConsoleCalc.elf ConsoleCalc/ConsoleCalc.cpp -lnullc
#~ Building nullcl application
#~ g++ -g -W -Lbin -o bin/nullcl.elf nullcl/main.cpp -lnullc_cl
#~ Building UnitTest application
#~ g++ -g -W -Lbin -o TestRun.elf TestRun.cpp UnitTests.cpp -lnullc
