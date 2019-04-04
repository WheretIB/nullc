# nullc
[![Build Status](https://travis-ci.org/WheretIB/nullc.svg)](https://travis-ci.org/WheretIB/nullc)
[![Build status](https://ci.appveyor.com/api/projects/status/3xt4fr4s8pja2chn?svg=true)](https://ci.appveyor.com/project/WheretIB/nullc)
[![codecov.io](http://codecov.io/github/WheretIB/nullc/coverage.svg?branch=master)](http://codecov.io/github/WheretIB/nullc?branch=master)
[![Coverity Scan](https://scan.coverity.com/projects/6629/badge.svg)](https://scan.coverity.com/projects/wheretib-nullc)
[![Codacy Badge](https://api.codacy.com/project/badge/Grade/ee1882a970074461847d999ed6fd642f)](https://app.codacy.com/project/WheretIB/nullc/dashboard)
[![Language grade](https://img.shields.io/lgtm/grade/cpp/g/WheretIB/nullc.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/WheretIB/nullc/context:cpp)

---
nullc is a C-like embeddable programming language with advanced features such as function overloading, operator overloading, class member functions and properties, automatic garbage collection, closures, coroutines, local functions, type inference, runtime type information, modules, list comprehension, enums, namespaces, generic functions and classes.

Language is type-safe and memory-safe.

nullc library can execute code on a VM or translate it to x86 code for fast execution. It can also translate nullc files into C source files.

This repository builds multiple additional tools:  
* nullcl - a tool to compile nullc source files into a binary module, C source files or an executable (using gcc).  
* nullcexec - a tool to execute nullc source files.  
* nullc_ide - a simple text editor with code colorizing and simple debug compatibilities (including remote debugging for applications that execute nullc scripts).
* nullc_lang_server - a language server implementation for integration with IDEs
* nullc_lang_debugger - a debug adapter that also includes language runtime for execution of nullc programs
* nullc_lang_client - a Visual Studio Code extension adapter that includes nullc_lang_server and nullc_lang_debugger

## March 2019 Update ##

Core:
 * New two-phase (parse/analyze) compiler
 * Improved syntax and expression trees
 * Parse error recovery with multiple error reports
 * Partial analyzer error recovery
 * Extended constant evaluation
 * Additional SSA intermediate representation with optimization passes
 * Improved source location tracking
 * Language improvements:
   * Member functions defined inside class body have access to complete class definition
   * Local function external variables are captures the same way in global code, functions and coroutines: when they go out of scope
   * Support for partial type inference when only parts of target type are marked as generic
   * Support for explicit type casts
   * Variable can be defined inside an 'if' condition
   * Class accessors can be used in modify-assignment expressions
   * enum constants can reference previous constants
   * Conditional expression will find common type between arrays of different explicit size
 * Compiler fixes:
   * Numerical unary operations can no longer be performed on 'auto ref' pointers
   * Read-only array size and type members can't be changed using member functions
   * Local classes can't be used after owning scope has ended
   * Fixed missing class constructor calls
   * Fixed alignment of function pointers
   * Fixed type inference for for-each iterator values
   * Fixed lookup of names inside a namespace
   * Fixed variable visibility after shadowing of said variable has ended
   * Fixed integer literal value overflow
 * Runtime fixes:
   * Fixed crash on external function calls with non-zero terminated character arrays

Misc:
 * Fixed translation to C++
 * VS Code extension with language server and debugger
 * arm and arm64 support
 * External function calls are handled by dyncall library on all supported platforms (x86, x64, arm, arm64, powerpc)

Interface:
 * Support for multiple module search paths
 * Log file output control
 * Optimization level control

## Changes in 2015 ##

Core:
  * Improved type inference in short inline functions

Misc:
  * Fixed bugs, crashes and hangs in the compiler found by extensive fuzz-testing
  * Fixed compilation with gcc and clang on x86/x64
  * dyncall library can be used to perform external function calls

## Changes in 2013 ##

Core:
  * 'explicit' function argument types to opt-out from implicit conversions in overload resolution
  * function overload resolution based on function return type

Misc:
  * std.string module for string operations
  * std.event module for C#-style event handlers
  * Android support (armeabi, armeabi-v7a and x86 targets)

Interface:
  * nullcGetGlobalType to get nullc type id by name

## Changes in 2012 ##
Changes from 0.9 tag

Core:
  * Type inheritance
  * Generic function specialization
  * Generic functions with explicit type specification
  * Generic type specializations for a different number of template parameters
  * Static 'if' in class body
  * Named function arguments
  * Extended typeof expression improvements
  * Allocation of any type using 'new' expression
  * Nested comments
  * Numerous bug fixes

Misc:
  * Run-time type introspection improvements
  * Wrapper for binding class member functions and class members from C++.

## November 19, 2010 ##
Changes from 0.8b to 0.9

Core:
  * [Short inline function definition syntax with extended type inference](http://svn.assembla.com/svn/SuperCalc/LanguageEN.html#funcshort)
  * [Generic functions](http://svn.assembla.com/svn/SuperCalc/LanguageEN.html#generic)
  * [Extended typeof syntax](http://svn.assembla.com/svn/SuperCalc/LanguageEN.html#typeof_ext)
  * [Static if](http://svn.assembla.com/svn/SuperCalc/LanguageEN.html#static_if)
  * [Improved type inference](http://svn.assembla.com/svn/SuperCalc/LanguageEN.html#typeauto)
  * [Pointers to overloaded operators](http://svn.assembla.com/svn/SuperCalc/LanguageEN.html#funcoperators)
  * [Generic types](http://svn.assembla.com/svn/SuperCalc/LanguageEN.html#classgeneric)
  * [Object finalization](http://svn.assembla.com/svn/SuperCalc/LanguageEN.html#finalize)
  * [Automatic class constructor call](http://svn.assembla.com/svn/SuperCalc/LanguageEN.html#classconstructor)
  * [Array index operator can have any number of arguments](http://svn.assembla.com/svn/SuperCalc/LanguageEN.html#funcoperators)
  * [Additional object construction code](http://svn.assembla.com/svn/SuperCalc/LanguageEN.html#constructor)
  * Modify-assign operators %= <<= >>= &= |= ^=
  * [|| or && operator overloads with short-circuiting support](http://svn.assembla.com/svn/SuperCalc/LanguageEN.html#funcoperators)
  * [Numeric constants inside a class](http://svn.assembla.com/svn/SuperCalc/LanguageEN.html#classes)
  * bool type
  * ["in" binary operator](http://svn.assembla.com/svn/SuperCalc/LanguageEN.html#funcoperators)
  * [Non-generic class forward declaration](http://svn.assembla.com/svn/SuperCalc/LanguageEN.html#classforward)
  * [enum](http://svn.assembla.com/svn/SuperCalc/LanguageEN.html#enum)
  * User class usage as conditions in if/for/while/do while/switch expressions
  * [namespaces](http://svn.assembla.com/svn/SuperCalc/LanguageEN.html#namespace)

Misc:
  * [Updated std.vector](http://svn.assembla.com/svn/SuperCalc/LanguageEN.html#std_vector)
  * [Updated std.list](http://svn.assembla.com/svn/SuperCalc/LanguageEN.html#std_list)
  * [Added std.hashmap](http://svn.assembla.com/svn/SuperCalc/LanguageEN.html#std_hashmap)

SuperCalc:
  * Redo support
  * Persistent undo

**Translation to C doesn't work in this version, use 0.8**

## September 18, 2010 ##
Changes from 0.7 to 0.8b

Core:
  * List comprehension
  * Coroutine can be used as a generator in for each expression.
  * JiT executor supports dynamic code linking and evaluation

Misc:
  * Automatic function binding under gcc and msvs.

Interface:
  * Added nullcIsStackPointer and nullcIsManagedPointer.
  * Added nullcGetFunction and nullcSetFunction.

## June 18, 2010 ##
Changes from 0.6b to 0.7:

Core:
  * Coroutines
  * Compile-time function evaluation
  * Better structure compatibility with C
  * Type constructor can be called in new expression.
  * added auto`[``]` type - an array with implicit size and element type. Helps GC when pointer contains objects of unspecified type.
  * nullptr can be assigned to type`[``]`
  * Various GC fixes and performance improvements
  * VM speedup

Misc:
  * Linux x86/x64 support
  * PS3 support
  * Documentation update

Interface:
  * Debug break actions: step over, step into, step out.
  * Global memory limit can be set
  * Further translation to C improvements

SuperCalc:
  * Added watch
  * Debug improvements: step over, step into, step out, module source code load

## May 14, 2010 ##
Changes from 0.5 to 0.6b:

Core:
  * Stack overflow exception handling in JiT
  * x86 asm optimization
  * Function override
  * eval() (VM-only)
  * Translation of NULLC code into C++ code
  * Support for unary operator +, -, ! and ~ overloading.
  * NULLC function call when executor is running
  * NULLC function call by pointer
  * NULLC function call with parameters
  * Functions with variable argument count
  * Function call of auto ref type redirection to target type
  * Ability to define arrays of non-stack types char, short and float
  * Default function argument values are exported and imported
  * Operators can be defined and overloaded in functions and classes

Misc:
  * x64 support
  * Digitalmars C++ compiler support
  * NULLC can compile code containing characters with codes over 0x7f

Interface:
  * Debugging features (call stack, stack frame contents, breakpoints (VM-only))
  * Functions for fast execution result retrieval as numeric values
  * nullcSetGlobal and nullcGetGlobal functions.
  * JiT executor parameter stack placement can be specified
  * NULLC\_NO\_EXECUTOR define that compiles NULLC without executors

SuperCalc:
  * Remote debugging
