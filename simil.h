/** 
 * # `simil.h`
 * A Ratcliff-Obershelp style string matcher.
 *
 * The Ratcliff-Obershelp algorithm was presented in Dr Dobbs' Journal in
 * the article _"Pattern Matching: The Gestalt Approach"_ by John W. Ratcliff
 * and David E. Metzener in July 1988.
 * 
 *
 * ### License
 *
 *     Author: Werner Stoop
 *     This software is provided under the terms of the unlicense.
 *     See http://unlicense.org/ for more details.
 *
 * ## API
 */
 
#ifndef SIMIL_H
#define SIMIL_H
 
#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

/**
 * #### `int simil (const char *a, const char *b)`
 * Tests the similarity of two strings `a` and `b` using the Ratcliff-Obershelp method.
 * 
 * The return value is a value between 0 and 100 where 0 means that 
 * the two strings have nothing in common, and 100 means that they're exact
 * matches.
 */
int simil (const char *a, const char *b);

/**
 * #### `int isimil (const char *a, const char *b)`
 * Tests the case-insensitive similarity of two strings a and b using 
 * the Ratcliff-Obershelp method.
 * 
 * It copies the strings internally using `strdup()`, converts the copies
 * to uppercase, and compares those.
 * 
 * The return value is a value between 0 and 100 where 0 means that 
 * the two strings have nothing in common, and 100 means that they're exact
 * matches. It may also return zero if the calls to `strdup()` fail. 
 */
int isimil (const char *a, const char *b);

#if defined(__cplusplus) || defined(c_plusplus)
} /* extern "C" */
#endif

#endif /* SIMIL_H */
