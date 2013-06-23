/*
 *	A Ratcliff-Obershelp style string matcher.
 *
 *	The Ratcliff-Obershelp algorithm was presented in Dr Dobbs' Journal in
 *	the article "Pattern Matching: The Gestalt Approach" by John W. Ratcliff
 *	and David E. Metzener in July 1988. 
 *	  Unfortunately the original implementation was in assembly, limiting its 
 *	portability and readability, and its relevance to modern architectures 
 *	(although its performance was extremely optimized). 
 *	  In Nov 1988 Joe Preston sent a C implementation of the algorithm to 
 *	Dr. Dobbs' letters to the editor. The C implementation used a recursive 
 *	algorithm. Unfortunately, according to the author it ran about twice as slow
 *	as the original. My implementation is inspired by that C implementation.
 *
 * 	The algorithm returns a value between 0 and 100 which indicates how alike
 *	two strings are. 0 means the strings have nothing in common, and 100 means
 *	they're exactly alike.
 *
 *	The typical use of the algorithm is for validating user input. The original
 *	article described educational software that can deal with spelling errors in
 *	answers. Other uses are applications that could supply a user with feedback
 *	along the lines of "The command 'fobo' is invalid. Did you mean 'foo'?" or
 *	intelligent compilers that could make suggestions when identifiers or 
 *	keywords are used incorrectly.
 */
 
/*
 * See simil.h for more info
 */
 
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

static int rsimil (const char *a, int alen, const char *b, int blen, int cs);

/*
 *	Tests the similarity of two strings a and b using the Ratcliff-Obershelp
 *	method. 	
 *	The return value is a value between 0 and 100 where 0 means that the
 *	two strings have nothing in common, and 100 means that they're exact
 *	matches.
 */
int
simil (const char *a, const char *b)
{
  int alen, blen;

  alen = strlen (a);
  blen = strlen (b);

  if (alen == 0 || blen == 0)
    return 0;

  return (rsimil (a, alen, b, blen, 1) * 200) / (alen + blen);
}

/*
 *	Case insensitive version of simil().
 *	It copies the strings internally using strdup(), converts the copies
 *	to uppercase, and compares those.
 *	It returns the same values as simil(), but it may also return zero if 
 *	the calls to strdup() fail. 
 */
int
isimil (const char *a, const char *b)
{
  int alen, blen;

  alen = strlen (a);
  blen = strlen (b);

  if (alen == 0 || blen == 0)
    return 0;

  return (rsimil (a, alen, b, blen, 0) * 200) / (alen + blen);
}

/*
 *	This is the core of the algorithm. It finds the longest matching substring
 *	and then recursively matches the left and right remaining strings.
 *	cs - Case sensitive
 */
static int
rsimil (const char *a, int alen, const char *b, int blen, int cs)
{
  int i, j, k, l, p = 0, q = 0, len = 0, left = 0, right = 0;

  /* Find a matching substring */
  for (i = 0; i < alen - len; i++)
    for (j = 0; j < blen - len; j++)
		{
			if(cs)
			{
				if (a[i] == b[j] && a[i + len] == b[j + len])
				{
					/* Find out whether this is the longest match */
					for (k = i + 1, l = j + 1; a[k] == b[l] && k < alen && l < blen; k++, l++);

					if (k - i > len)
					{
						p = i;
						q = j;
						len = k - i;
					}
				}
			} else {
				if (tolower(a[i]) == tolower(b[j]) && tolower(a[i + len]) == tolower(b[j + len]))
				{
					/* Find out whether this is the longest match */
					for (k = i + 1, l = j + 1; tolower(a[k]) == tolower(b[l]) && k < alen && l < blen; k++, l++);

					if (k - i > len)
					{
						p = i;
						q = j;
						len = k - i;
					}
				}
			}
		}

  /* No match */
  if (len == 0)
    return 0;

  /* Match the strings to the left */
  if (p != 0 && q != 0)
    left = rsimil (a, p, b, q, cs);

  i = (p + len);
  alen -= i;
  j = (q + len);
  blen -= j;

  /* Match the strings to the right */
  if (alen != 0 && blen != 0)
    right = rsimil (a + i, alen, b + j, blen, cs);

  /* Return the score */
  return len + left + right;
}
