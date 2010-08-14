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
  temp_lib/canvas.o \
  temp_lib/dynamic.o \
  temp_lib/file.o \
  temp_lib/gc.o \
  temp_lib/hashmap.o \
  temp_lib/io.o \
  temp_lib/list.o \
  temp_lib/map.o \
  temp_lib/math.o \
  temp_lib/pugi.o \
  temp_lib/random.o \
  temp_lib/string.o \
  temp_lib/time.o \
  temp_lib/typeinfo.o \
  temp_lib/vector.o

PUGIXML_TARGETS = \
  temp/pugixml.o

all: temp/.dummy temp_compiler/.dummy temp_lib/.dummy \
     bin/nullcl TestRun bin/ConsoleCalc

temp_lib/%.o: NULLC/includes/%.cpp
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

temp_compiler/.dummy:
	mkdir -p temp_compiler
	touch temp_compiler/.dummy

temp_lib/.dummy:
	mkdir -p temp_lib
	touch temp_lib/.dummy


bin/libnullc.a: ${LIB_TARGETS} ${STDLIB_TARGETS} ${PUGIXML_TARGETS}
	$(AR) rcs $@ $^

clean:
	rm -f temp/* temp_lib/* temp_compiler/*

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
  temp_compiler/BinaryCache.o \
  temp_compiler/Bytecode.o \
  temp_compiler/CodeInfo.o \
  temp_compiler/nullc.o \
  temp_compiler/stdafx.o \
  temp_compiler/StrAlgo.o \
  temp_compiler/Callbacks.o \
  temp_compiler/Compiler.o \
  temp_compiler/Lexer.o \
  temp_compiler/Parser.o \
  temp_compiler/SyntaxTree.o

temp_compiler/%.o: NULLC/%.cpp
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
  UnitTests.cpp

TEST_OBJECTS = \
  TestRun.o \
  UnitTests.o


%.o: %.cpp
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
