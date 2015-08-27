# nullc [![Build Status](https://travis-ci.org/WheretIB/nullc.svg)](https://travis-ci.org/WheretIB/nullc) [![Build status](https://ci.appveyor.com/api/projects/status/3xt4fr4s8pja2chn?svg=true)](https://ci.appveyor.com/project/WheretIB/nullc) [![codecov.io](http://codecov.io/github/WheretIB/nullc/coverage.svg?branch=master)](http://codecov.io/github/WheretIB/nullc?branch=master)
nullc is a C-like scripting language with advanced features such as function overloading, operator overloading, class member functions and properties, automatic garbage collection, closures, coroutines, local functions, type inference, runtime type information, modules, list comprehension, generic functions and classes, enum and namespaces.

Language is type-safe and there is no pointer arithmetic.

Library can execute code on VM or translate it to x86 code for fast execution.
SuperCalc is language IDE with code colorizing and simple debug compatibilities.

## September 8, 2012 ##
Changes from 0.9 to current revision

Core:
  * Type inheritance
  * Generic function specialization
  * Generic functions with explicit type specification
  * Named function arguments
  * Extended typeof expression improvements
  * Allocation of any type using 'new' expression
  * Nested comments
  * Numerous bug fixes
Misc:
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
SuperCalc:
  * Redo support
  * Persistent undo
Misc:
  * [Updated std.vector](http://svn.assembla.com/svn/SuperCalc/LanguageEN.html#std_vector)
  * [Updated std.list](http://svn.assembla.com/svn/SuperCalc/LanguageEN.html#std_list)
  * [Added std.hashmap](http://svn.assembla.com/svn/SuperCalc/LanguageEN.html#std_hashmap)

**Translation to C doesn't work in this version, use 0.8**

## September 18, 2010 ##
Changes from 0.7 to 0.8b

Misc:
  * Automatic function binding under gcc and msvs.
Core:
  * List comprehension
  * Coroutine can be used as a generator in for each expression.
  * JiT executor supports dynamic code linking and evaluation
Interface:
  * Added nullcIsStackPointer and nullcIsManagedPointer.
  * Added nullcGetFunction and nullcSetFunction.

## June 18, 2010 ##
Changes from 0.6b to 0.7:

Misc:
  * Linux x86/x64 support
  * PS3 support
  * Documentation update
Core:
  * Coroutines
  * Compile-time function evaluation
  * Better structure compatibility with C
  * Type constructor can be called in new expression.
  * added auto`[``]` type - an array with implicit size and element type. Helps GC when pointer contains objects of unspecified type.
  * nullptr can be assigned to type`[``]`
  * Various GC fixes and performance improvements
  * VM speedup
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
  * Stack oveflow exception handling in JiT
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
Interface:
  * Debugging features (call stack, stack frame contents, breakpoints (VM-only))
  * Functions for fast execution result retrieval as numeric values
  * nullcSetGlobal and nullcGetGlobal functions.
  * JiT executor parameter stack placement can be specified
  * NULLC\_NO\_EXECUTOR define that compiles NULLC without executors
Misc:
  * x64 support
  * Digitalmars C++ compiler support
  * NULLC can compile code containing characters with codes over 0x7f
SuperCalc:
  * Remote debugging
