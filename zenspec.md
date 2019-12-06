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


