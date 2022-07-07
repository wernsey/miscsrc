Miscsrc
=======

A library of miscellaneous C functions I've written over time.

The modules include:

 * A JSON parser and serializer: [json.c](json.c) and [json.h](json.h),
 * A set of functions to read, write and manipulate __Comma Separated Values__ (CSV) files,
   * [csv.c](csv.c) and [csv.h](csv.h) will read an entire file into memory for manipulation.
   * [csvstrm.h](csvstrm.h) is a streaming parser that reads a CSV file row-by-row.
 * A parser for __INI__ configuration files: [ini.c](ini.c) and [ini.h](ini.h),
 * A mathematical expression evaluator: [eval.c](eval.c) and [eval.h](eval.h),
 * Functions to load and store WAV files: [wav.c](wav.c) and [wav.h](wav.h),
 * A mark-and-sweep garbage collector: [gc.c](gc.c) and [gc.h](gc.h),
 * A reference counting garbage collector: [refcnt.c](refcnt.c) and [refcnt.h](refcnt.h),
 * A simple regular expression pattern matcher: [regex.c](regex.c) and [regex.h](regex.h),
 * A generic linked list implementation with extra helper functions:
   [list.c](list.c) and [list.h](list.h),
 * A simple hash table implementation: [hash.c](hash.c) and [hash.h](hash.h),
 * A function that compares the _similarity_ of two strings:
   [simil.c](simil.c) and [simil.h](simil.h),
 * An alternative to `getopt()` called `getarg()` for platforms without _getopt()_ 
   (such as for WIN32): [getarg.c](getarg.c) and [getarg.h](getarg.h), and
 * A couple of utility functions.

It is a collection of C code that I've implemented over the course of several years
and used, tested and reused in several of my hobby projects.

-----------------------------------------

These sources are provided under the terms of the unlicense:

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

For more information, please refer to <http://unlicense.org/>
