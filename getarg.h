/**
 * # getarg.h: 
 * Replacement for the POSIX `getopt()` function for processing
 * command-line options. I use it for compiling programs that require command-
 * line arguments under Windows, but my goal is to have it portable to use 
 * in programs compiled on other platforms.
 *
 * Although I've consulted the relavant `getopt()` manual pages, this 
 * implementation is entirely my own.
 *
 * Use it as you would use `getopt()`, but replace all the "opt" prefixes
 * with "arg"
 *
 * ### License
 *
 *     Author: Werner Stoop
 *     This software is provided under the terms of the unlicense.
 *     See http://unlicense.org/ for more details.
 *
 * ## API
 */
#ifndef GETARG_H
#define GETARG_H
#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

/**
 * ### Variables
 * 
 * * `argind` - Replaces `getopt()`'s `optind`
 * * `argopt` - Replaces `getopt()`'s `optopt`
 * * `argarg` - Replaces `getopt()`'s `optarg`
 */
extern char *argarg;
extern int argind, argopt;
 
/**
 * ### Functions
 * #### `char getarg(int argc, char * const argv[], const char *opts)`
 * Use this as you would use `getopt()`
 */
char getarg(int argc, char * const argv[], const char *opts);

#if defined(__cplusplus) || defined(c_plusplus)
} /* extern "C" */
#endif
#endif /* GETARG_H */
