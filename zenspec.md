Zen Language Specification
==========================

This is the formal specification of the Zen programming language --- a C like
minimal systems programming language.

Input Streams
-------------



Tokens
------

Input streams are initially converted to tokens as noted below. Before breaking
in to tokens, the input stream is scrubbed of any characters between and
including /\* \*/ and // \<newline>.


### Space

Any sequence of " ", \t, \n, or \r.

### Punctuation Characters

`! " # $ % & ' ( ) * + , - . / : ; < = > ? @ [ \ ] ^ _ \` { | } ~`

### Delimiters

Any of these characters break a token as noted below: SOF, EOF, space,
punctuation.

### Keywords

One of the following, delimited as noted above: break, case, continue, const,
default, do, else, enum, fallthrough, for, func, if, return, struct, switch,
typedef, union, volatile.

### Identifiers

A delimited sequence of [a-zA-Z0-9_].

### Punctuation

The longest sequence of punctuation that is one of the following, or a single
punctuation character: --, ++, ->, !~, !=, \<\<, \>\>, <=, >=, ==, &&, ||, +=,
-=, *=, /=, %=, &=, ^=, |=, <<=, >>=

### Numeric literal

A sequence of [0-9a-fA-F-._xXoObBpP], that must start with a digit 0-9 and is
delimited. (Note: negative numbers are interepreted as a sequence of tokens '-'
'<numeric literal'). This definition may accept non-conforming numberic
expressions, which must be treated as an error. See literal parsing for details
on accepted number formats

### String literal

A sequence starting with " or ' and ending with " or '. Double quote strings
allow escaping, while single quote strings are verbatim (see String Literal
Parsing). Strings cannot contain a literal newline (\n or \r).


Literal Parsing
---------------

This section describes the method of converting text literal values to their
appropriated binary representation.

### Numeric Literal Parsing

Numeric literals must be parsed to match the binary representation of the type
to which it is assigned. Literal strings take the form: \<base> \<digits> .?
\<digits> \<exponent>. Everything besides the first digits is optional.

The \<base> sequence specifies the base of the textual representation, and
determines allow \<digits>. Allowed forms are 0x (hex), 0o (octal), 0b (binary),
or nothing for decimal.

Hex digits are [0-9a-fA-F]. Decimal digits are [0-9]. Octal digits are [0-7].
Binary digits are [01]. Only one decimal point is allowed in the stream of
digits. Underscore characters are allowed in digits as place markers, and are
ignored during parsing.

The exponent must begin with [eE] character (decimal) or [pP] character (hex) if
present. Optionally either + or - may occur to determine the sign of the
exponent. Finally, a string of either hex or decimal digits depending on the
initial exponent character. 

### String Literal Parsing

String literals produce uint8[] data in utf-8 format. (The input format for the
source is also utf-8, and so is generally a byte by byte copy of the source
text, except for escaping as noted below.) Empty string literals result in an
uint8[0] type with address 0. Except for null strings, string literals are
writable, but identical string sequences (after escapes are processed) will
point to the same memory location. For instance:

```
a := "hello";
b := "hello";
&a == &b; //true
```

Literals inside of double quotes are scanned sequentially, left to right, for
any of the following sequences which are replaced with the hex value in the
middle column. Unicode code points are converted to utf8 and have variable
length.

Sequence    Value   Name
---------   -----   ------
\n	        0A	    Newline (Line Feed)
\r	        0D	    Carriage Return
\t	        09	    Horizontal Tab
\\	        5C	    Backslash
\'	        27	    Apostrophe or single quotation mark
\"	        22	    Double quotation mark
\xhh	    hh	    The byte whose numerical value is given in hex
\uhhhh	    -       Unicode code point below 10000 hex
\Uhhhhhhhh	-       Unicode code point in hex

Consecutive string literals are concatenated as part of the parsing step, and
before string de-duplication.

Syntax
------

Now follows the 

### Top Level Structure

Zen programs or modules are composed of one or more files, which are collections
of top level definitions:

- Const 
- Enum
- Function
- Include
- Let
- Struct
- Typedef

The order of top level elements in the file(s) does not mater, nor does the
order of files. Any top level element declared within a module or program is
always visible at any other point in the module or program.

Top level elements of a module or hidden from external users (aka private)
unless explicity made visible with the `export` keyword immediatly after the top
level keyword. 

### Include

The `include` keyword pulls in an external module, specified by the path string
immediatly following, in to the global namespace. By default modules occupy the
the same identifier as their final path, although this can be overridden by
specifiying an identifier after the path.

```
include "example/mymodule";
//Access using mymodule->myfunc()

include "example/mymodule" mm;
//Access using mm->myfunc()
```

Exporing an include makes the module accessible by third parties:

```
include export "module";
//This include can be accessed using thismodule->module->module_func()
//from third party code
```

### Const and Let

Const and let define globally accessible data. Both have a similar form

```
const ident = expr;
let ident = expr;
let ident;
```

Let allocates and initializes static memory. This memory is mutable.

Const creates a compile time constant which acts much like a read only variable,
but no memory is allocated. Thus constants have no memory address and there are
no pointers to constants.

Numeric constants are treated as if text level subtition had taken place when
used, so they are guarenteed to provide maximum possible precision when used in
different numeric contexts. This is mostly useful for decimal representations
being converted to different floating point representations.

### Typedef, Struct, and Enum

These three top level elements create entries in the type namespace of the
current program or module. And are optionally made visible to third parties
using the `export` keyword.

Typedef is the simplest, and simply takes an identifier and a type expression.

```
typedef mytype typeexpr... ;
```

Typdefs are useful for creating new symantic meanings using an existing base
type. For instance, the two types created below have the same base type, but
must be explictly cast to assign a variable of one type to the other.

```
typedef utc_time int32;
typedef local_time int32;
```

Enums are use to declare constants which are automatically namespaced and can be
stored in run time:

```
enum weekday {MON, TUE, WED, THUR, FRI, SAT, SUN};
// Access as weekday->MON, etc

//Alternate form
enum weekday {
    SUN = 6
    MON = 0
    TUE
    WED, THUR
    FRI
    SAT
}
```

Enums comma (or newline) seperated list of identifiers, optionally with
assignment expressions that evaluate in compile time to integer constants.
Identifiers without expressions reevaluate the previously given  expression. If
this produces the same value, then that value + 1 is used. If the first value
has no expression, it is assigned to 0.

During enum expression evaluation, the identifier `iota` is defined as 0 and is
incremented once after each enum element:

```
enum bytesize { B = 1 << (10*itoa), KB, MB, GB, TB, PB };
```

The compiler is free to choose an approprate integer type (eg `int32`, `int16`,
etc) for runtime storage based on the range of specified values, but the size
can be queried at compile time using `weekday->size`.

Structs are used declare compound types. They take the form of a semicolon
seperated list of identefiers one or more identifiers, seperated by commas,
followed by a type expression. The final semicolon in a struct is optional.

```
struct point {x,y,z float32}
struct buffer {
    data uint8;
    capacity,   //takes int type of following member
    i,          //takes int type of following member 
    len int;
};
```

Enums and structs can also be specified anonymously as type expressions.
Therefore, the following statements are the same.

```
struct mystruct {x,y int}

typedef mystruct struct {x,y int}
```

### Functions and methods

Both functions and methods are declared using the `func` keyword. Either must be
exposed using the `export` keyword to be accessible by third party code.

Functions are procedures created in the global namespace of this module or
function. Methods are procedures created in the namespcaef the associated type,
which is specified by a type expression preceding the function name.

```
func function() void {}
func type->method() void {}
func mymodule->type->method() void {}
```

Function arguments are specified as a list of identifiers with optional types,
similar to a struct definition, surrounded by parens. Next, the return value(s)
are specified using a comma delimited list. At least one return value must be
specified, but can be void. Lastly curly braces enclose a list of expressions
that make up the body of the function.

### Type Expressions

Types are compile time entities that exist in a seperate namespace from other
identifiers. They consist of predifined type privatives and compound types
defined using typedef, struct, enum, or union.

Type expressions are used to express the type of an object in casts,
structure/union declarations, function agruments, and typedefs.

The simiplest form of type expression is simply an identifier corresponiding to
a declared type. This can be followed by '*' to indicate a pointer of that type,
or '[]' to indicate an array. Array brackets can contain an expression that can
be evaluted at compile time to an integer, to indicate the array size.

