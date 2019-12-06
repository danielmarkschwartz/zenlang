The Zen Language
================

The Zen language (zenlang) is a minimal systems programming language, in the
spirit of C and Go.

Why a New Language
------------------

Rather than a revolutionary new language, Zen seeks to be a concise realization
of a small subset of existing and well understood programming paradigms.
Particularly, good ideas found in C, C++, and Go are extracted, and refined to
fit the mold of low level systems programming language with unrestricted memory
access for the next decade.

Ideally, programmers with some experience in C (or C like) programming language
should understand most programs on sight, and have a complete grasp of the
feature set in 30 min or less.

Features
--------

- C like
- Simplified syntax
- Updated fundamental types
- Go like function name spacing (methods) via types
- Module system 
- True contestants 
- Compile time execution
- Switch cases break by default (fallthrough keyword added)

- Unity build, but with module level name spacing

Modernizing the C Preprocessor
------------------------------

The C preprocessor was originally a hack on C, bolted on the side and eventually
taking root in to the C echo system. Like most modern languages, Zen replaces
the functionality of the preprocessor and header files with first class language
features that simplify and extend the overall language.

### Modules

Zen controls complexity by packaging code in modules. A module is a collection
of Zen source files that share a top level namespace. So all functions, types,
variables defined globally in any module file are available to all other.

User modules or "standard library" features can be accessed by the `include
"/img/png" as png;` statement, which takes a module path. In this example a
name space called `png` is created with functionality inherited from the PNG
module of the standard library.

Module paths are a feature of the build system, and highly configurable, but
typically the correspond to directories of Zen files. In this case absolute
paths (starting with '/') are system or "standard" modules, and relative paths
represent a module defined in the current project.

By default, all top level definitions in a module are not visible outside the
module (unexported). Including the `export` keyword before top level definitions
allows exposes the feature for use by client applications, thus defining an API.

### Constants

Using the `const` keyword is in `const PI = 3.14159;` defines a compile time
constant that maintains full precision and acts just like a literal when used in
expressions.

### Compile Time Execution

Prefixing an expression with `#`  at the start of each line causes it to be run
at compile time, if possible. Compiler flags allow for setting of compile time
constants, and some default constants are set by default, including `FILE` and
`LINE`.

Run time variables cannot be accessed at compile time, but the value of compile
time constants at that point in the source can be accessed at run time.

Additionally, the functions `warn(str, ...)` and `err(str, ...)` are defined
only during compile time, causing compiler warnings and errors restively. The
`source(str, ...)` function inserts the given string in to the compiled source.
For instance:

```
#PI := 3 + 0.14159;
#source("i := %s;", PI);
```

is equivalent to

```
i := 3.14159;
```

Components of compile time expressions not prefixed with `#` are only compiled
if that portion of the expression would be run during compile time. So:

```
#if(1==0){
    msg := "opps";
#err("Should not reach this point");
#}else
    msg := "correct";
```

is the same as:

```
#if(1==0){
#source("msg := \"opps\";)
#err("Should not reach this point");
#}else
#source("msg := \"correct\";");
```

Access compile time constants by using the `#` operator as well.

Improved Types
--------------

The C type system here is cleansed of a little crustyness, and slightly extended
to be more useful and expressive.

C numeric types are normilized to float#, complex#, int#, and uint# where the
optional number is the number of bits in the type. SIMD vect types of form
v#type are supported where available, where # is the number of elements and type
is the size/type of the elements. Arthimetic operators generate SIMD
instructions for SIMD types where possible.

Bool is a fundamental type with value true or false. Outside of structs bool are
equivalent to int. Inside of structs, subsequent bool values are compressed to
bits of an int, with LSB mapped to the first element. true == 1, and false == 0
in the context of arithmetic operators.

Structs, unions, and enums exist like in C. Named types exist directly in the
current namespace as types (structs/enums don't have their own namespace like in
C).  Enums are represented by the least uint# type that covers all
possibilities, and enum members are namespaced to the type:

```
enum weekday { MON, TUE, WED, THU, FRI, SAT, SUN };
today := weekday->MON;
```

Function pointers are replaced by a function type:

```
struct something {
    callback func(x, y int) int;
};

func f(x, y int) int { x+y }

e := (something){0};
e.callback = f;
```

### Arrays

Array types are identical to their pointer, unlike C where arrays have subtly
different behavior from the pointer.

### String Literals

String literals are arrays of uint8, encoded in utf8, and do not include a
terminating NULL. String length can be inferred from the type (see `->` below).

### Type Conversions

Numeric types are automatically converted up in size, but not between int, uint,
or float without explicit conversion. Arrays can be used as their equivalent
pointer, but loose array size information at the point of conversion. 

Type definitions of equivalent base type do not automatically convert, and must
be explicitly converted. Conversion between non-compatible base types is not
allowed.For instance:

```
typedef time_utc int;
typdefe time_lst int;

u := (time_utc)123;
l := (time_lst)0;
l = (time_lst)u;  //l = u would throw an error

Simplified Syntax
-----------------

Here we address several peculiarities of the C syntax.

### Variable Declaration

Variables are declared and assigned in scope by the assignment operator `:=`.
There is no way do declare a variable without assignment. The variable type is
inferred from the type of the right hand operand, which can be forced by a C
style cast if ambiguous.

In parameter lists and struct/union member lists, variables are declared by an
identifier followed by an optional type. If type is omitted, then the variable
takes the type of the following argument / member. The last argument/member must
have a type expression.

### Switches

Switches break by default. The `fallthough` keyword is added in case where fall
through is behavior is desired.

### All Statements Are Expressions

Zenlang attempts to simplify the language grammar and some language forms
(particularly lambda functions), by making most statements an expression. This
mean every language structure implicitly has a value, and could appear on the
right side of an assignment operator.

Blocks, which are sequences of expressions, take the value of the last
expression. This is similar to the `,` operator in C, which can be used
interchangeably with `;` as an expression separator, but has a lower precedence.

Conditionals `if`, `else`, and `switch` take the value of the
expression for the branch taken. Loops take the value of the of the expression
after the final loop cycle.

This means the following expression evaluates as 1:

```
x = (if(true) 1; else 0;);
```

Functions automatically return the value of it's final expression. Empty
expressions evaluate as `void`, so ending a void function with `;;` will suppress
warnings. The `return` keyword mains it's previous functionality, and can be
used as a final expression if desired.

`break` and `continue` can optionally precede an expression, and they take the
value of that expression. Otherwise, they evaluate as void.

`fallthough` is void, but should never be the last expression of a switch.

Evaluating `goto` or `return` in an expression that is on the right hand side of
an assignment operator causes the assignment to never evaluate.

### Dots and Arrows

Now `.` will deference a pointer if necessary, removing the need for a `->`
operator as an element accessor. 

Instead `->` is used to access compile time type information. The left hand side
of the arrow must be a type name. This is the only operator that directly
accesses the type namespace, except `==` and `!=`, which is distinct from other
identifiers. The right hand is one of the following:

- id --- a integer id specific to this type, in this build only
- size --- size of type in bytes
- name --- a canonical type name
- num --- number of elements (array)
- type --- type of elements (array)
- \<member identifier> --- used to access type information about member elements
- offset --- offset of member in bytes (only compound type member)
- \<enum element> --- provides enum value for given option as int


New Functionality
-----------------

### Replacing Goto

Much maligned `goto` has legitimate use in C in several situations, most
importantly error handling clean up and occasionally exiting nested loops where
`break` would not suffice.

Zenlang removes `goto` in order to keep elements of the expression syntax
(particularly avoiding a jump out of the right hand side of an assignment)
unambiguous.

### Defer

Instead, zen provides a Go like `defer` keyword which guarantees an
expression will be run after a function return, but before giving control to the
calling code. Multiple `defer` expression will run in reverse order. Inside a
defer expression `return` will conclude execution of the current `defer`
expression, but will not skip other `defer` expressions that have yet to be run.

### Multiple Breaks

Additionally, zen allows multiple `break` keywords in a given expression, which
will exit the same number of loops:

```
for(x := array[])
    for(y := array[x][])
        if(found(y))
            break break;
```

### Multiple Assignment

Multiple assignment is allowed with `=` or `:=`. Functions are allowed to return
multiple values. `_` acts as a don't care place holder on the left hand side.
Variable swapping is valid, as the rhs is evaluated first before assignment.

Desugering expressions `.(,)`, `[,]`, and `[:]` are supported. The first form
allows one to access a list of compound type elements in order. Use of `.()` on
the results of any of these operators applies to it every element of that list.

`[,]` and `[:]` allow one to access elements of an array. `:` inside an array
index indicates a range of elements from the left element to the right
inclusive. The left hand side may be bigger than the right, indicating a reverse
listing. `,` is used to separate a list of element id's to access. These
elements may also be ranges.

### Lambda Functions

Functions can be defined inside other functions, and only have local scope.
Function bodies can access function scope variables, but the value of variable
is stored at the time of function definition. (IE, all function local variables
are closures.)

Using the `\(...) ...` form allows for creation of anonymous functions.

### Methods

Functions defined at module scope can be defined as members of a particular type
using the `->` operator:

```
typdef time int;
func time->string() char* { "time string"; }

func main() {
    t = (time)1234;
    io->print(t->string());
}
```
