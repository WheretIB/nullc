echo Compiling NULLC lib
mkdir temp
for i in NULLC/*.cpp; do
	u=`expr length $i`;
	u=`expr $u - 10`;
	echo ${i:6:$u};
	g++ -c -g -W -o temp/${i:6:$u}.o $i
done
for i in NULLC/includes/*.cpp; do
	u=`expr length $i`;
	u=`expr $u - 19`;
	echo ${i:15:$u};
	g++ -c -g -W -o temp/std_${i:15:$u}.o $i
done

g++ -c -g -W -o temp/ext_pugixml.o external/pugixml/pugixml.cpp

echo Building NULLC lib
ar rcs bin/libnullc.a temp/BinaryCache.o temp/Bytecode.o temp/Callbacks.o temp/CodeInfo.o temp/Compiler.o temp/Executor.o temp/Executor_Common.o temp/Lexer.o temp/Linker.o temp/nullc.o temp/Parser.o temp/stdafx.o temp/StdLib.o temp/StrAlgo.o temp/SyntaxTree.o temp/std_file.o temp/std_math.o temp/std_typeinfo.o temp/std_vector.o temp/std_list.o temp/std_list.o temp/std_dynamic.o temp/std_random.o temp/std_gc.o temp/std_io.o temp/std_canvas.o temp/std_map.o temp/std_hashmap.o temp/std_string.o temp/std_time.o temp/std_pugi.o temp/ext_pugixml.o temp/CodeGen_X86.o temp/Executor_X86.o temp/Translator_X86.o

echo Compiling NULLC compiler-only lib
for i in NULLC/BinaryCache.cpp NULLC/Bytecode.cpp NULLC/CodeInfo.cpp NULLC/nullc.cpp NULLC/stdafx.cpp NULLC/StrAlgo.cpp NULLC/Callbacks.cpp NULLC/Compiler.cpp NULLC/Lexer.cpp NULLC/Parser.cpp NULLC/SyntaxTree.cpp; do
	u=`expr length $i`;
	u=`expr $u - 10`;
	echo ${i:6:$u};
	g++ -c -g -W -D NULLC_NO_EXECUTOR -o temp/${i:6:$u}.o $i
done

echo Building NULLC compiler-only lib
ar rcs bin/libnullc_cl.a temp/BinaryCache.o temp/Bytecode.o temp/Callbacks.o temp/CodeInfo.o temp/Compiler.o temp/Lexer.o temp/nullc.o temp/Parser.o temp/stdafx.o temp/StrAlgo.o temp/SyntaxTree.o

echo Building ConsoleCalc application
g++ -g -W -Lbin -o bin/ConsoleCalc.elf ConsoleCalc/ConsoleCalc.cpp -lnullc

echo Building nullcl application
g++ -g -W -Lbin -o bin/nullcl.elf nullcl/main.cpp -lnullc_cl

echo Building UnitTest application
g++ -g -W -Lbin -o TestRun.elf TestRun.cpp UnitTests.cpp -lnullc

