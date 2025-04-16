Miscsrc
=======

![GitHub](https://img.shields.io/github/license/wernsey/miscsrc)
![GitHub code size in bytes](https://img.shields.io/github/languages/code-size/wernsey/miscsrc)
![GitHub repo file count](https://img.shields.io/github/directory-file-count/wernsey/miscsrc)

A library of miscellaneous C functions I've written over time.

| Header | Code | Description |
| --- | --- | --- |
|[json.h](json.h)|[json.c](json.c)| A [JSON][] parser and serializer. It also supports [JSON5][] - see below.|
|[csv.h](csv.h)|[csv.c](csv.c)| A set of functions to read, write and manipulate [Comma Separated Values][CSV] (CSV) files; They keep entire file file memory for manipulation
|[csvstrm.h](csvstrm.h) | no | A streaming [CSV][] parser that reads a CSV file row-by-row.|
|[strmtok.h](strmtok.h)| no | A string interning library.|
|[sintern.h](sintern.h)| no | A stream tokenizing library.|
|[ini.h](ini.h) | [ini.c](ini.c)| A parser for [INI][] configuration files|
|[eval.h](eval.h)|[eval.c](eval.c)| A mathematical expression evaluator|
|[wav.h](wav.h)|[wav.c](wav.c)| Functions to load and store [WAV][] files|
|[gc.h](gc.h)|[gc.c](gc.c)| A mark-and-sweep garbage collector|
|[refcnt.h](refcnt.h)|[refcnt.c](refcnt.c)| A reference counting garbage collector|
|[regex.h](regex.h)|[regex.c](regex.c)| A simple regular expression pattern matcher|
|[list.h](list.h)|[list.c](list.c)| A generic linked list implementation with extra helper functions|
|[hash.h](hash.h)|[hash.c](hash.c)| A simple hash table implementation|
|[simil.h](simil.h)|[simil.c](simil.c)| A function that compares the _similarity_ of two strings|
|[getarg.h](getarg.h)|[getarg.c](getarg.c)| An alternative to `getopt()` called `getarg()` for platforms without _getopt()_ (such as for WIN32)|
|[utils.h](utils.h)|[utils.c](utils.c)| A couple of utility functions.|

It is a collection of C code that I've implemented over the course of several years
and used, tested and reused in several of my hobby projects.

The [JSON parser](json.c) can parse JSON5, but will always serialize to
strict [JSON][]. JSON5 support is enabled by default, but can be disabled
through a preprocessor directive.

[JSON]:  https://www.json.org
[JSON5]: https://json5.org/
[CSV]: https://en.wikipedia.org/wiki/Comma-separated_values
[INI]: https://en.wikipedia.org/wiki/INI_file
[WAV]: https://en.wikipedia.org/wiki/WAV

-----------------------------------------

These sources are provided under the terms of the unlicense:

```
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.
```

For more information, please refer to <http://unlicense.org/>
