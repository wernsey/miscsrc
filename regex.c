/*
 *	This is a regular expression evaluator based on code from the article
 *	"Regular Expressions: Languages, algorithms, and software"
 *	By Brian W. Kernighan and Rob Pike in the April 1999 issue of
 *	Dr. Dobb's Journal
 *
 *	I've found the article online at
 *	http://www.ddj.com/dept/architect/184410904
 *
 * See regex.h for more details
 */

#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>

#define CASE_INSENSITIVE 1
#define GREEDY			 2

/* Special escape char in matches */
#define ESCAPE_CHAR		'\\'

/* Character to be used to place the matchin part of the
 * text in the substituted text in the rx_sub() and rx_gsub() functions
 */
#define GSUB_SUBMATCH	'&'

/* Character to be used to escape GSUB_SUBMATCH in 
 * the rx_sub() and rx_gsub() functions
 */
#define GSUB_ESCAPE		'/'

/* This structure is carried through the internals to carry 
 * match information and other parameters
 */
struct mstruct
{
  char flags;
  const char *start;            /* Start of matching text */
  const char *end;             /* End of matching text */
};

/* Prototypes for static functions used internally: */
static int re_match (const char *text, const char *re, struct mstruct *m);
static int expr (const char *text, const char *re, struct mstruct *m);
static int star (const char *text, const char *s, const char *re,
                 struct mstruct *m);
static int plus (const char *text, const char *s, const char *re,
                 struct mstruct *m);
static int ques (const char *text, const char *s, const char *re,
                 struct mstruct *m);
static int atom (const char *text, const char *re, struct mstruct *m);

/*
 * Matches text against the regular expression re.
 * It will return non-zero if text matches the pattern re, otherwise it 
 * returns 0.
 */
int
rx_match (const char *text, const char *re)
{
  struct mstruct m;
  m.flags = 0;

  return re_match (text, re, &m);
}

/*
 *	Matches text against the regular expression re and extracts the position
 *	of the matching text. 
 *	If the text matches the pattern re, the pointers pointed to by beg and end
 *	will be set to point to the begining and end of the matching substring in 
 *	text.
 */
int
rx_search (const char *text, const char *re, const char **beg,
           const char **end)
{
  struct mstruct m;
  m.flags = GREEDY;
  m.start = 0;
  m.end = 0;

  if (re_match (text, re, &m))
    {
      if (beg)
        *beg = m.start;
      if (end)
        *end = m.end;
      return 1;
    }

  if (beg)
    *beg = 0;
  if (end)
    *end = 0;
  return 0;
}

/* 
 * Internal function that does all the hard work.
 * It matches the '^' anchor and calls the other matching functions.
 */
static int
re_match (const char *text, const char *re, struct mstruct *m)
{
  if (re[0] == '^')
    {
      m->start = text;
      return expr (text, re + 1, m);
    }
  else
    do
      {
        m->start = text;
        if (expr (text, re, m))
          return 1;
      }
    while ((text++)[0] != '\0');

  return 0;
}

/*
 * Matches the first part of text against the first part of re 
 */
static int
expr (const char *text, const char *re, struct mstruct *m)
{
  int i = 0;
  if (re[0] == '\0')
    {
      m->end = text;
      return 1;
    }
  if (re[0] == '$' && re[1] == '\0')
    {
      m->end = text;
      return text[0] == '\0';
    }

  if (re[0] == ESCAPE_CHAR)
    {
      if (re[1] == 'i')
        {
          m->flags |= CASE_INSENSITIVE;
          return expr (text, re + 2, m);
        }
      else if (re[1] == 'I')
        {
          m->flags &= ~CASE_INSENSITIVE;
          return expr (text, re + 2, m);
        }

      i++;
    }
  else if (re[0] == '[' && ++i)
    while (re[i] != ']')
      {
        if (re[i] == '\0')
          return 0;             /* syntax error */
        if (re[i] == ESCAPE_CHAR)
          i++;
        i++;
      }

  if (re[i + 1] == '*')
    return star (text, re, re + i + 2, m);
  if (re[i + 1] == '+')
    return plus (text, re, re + i + 2, m);
  if (re[i + 1] == '?')
    return ques (text, re, re + i + 2, m);

  if (text[0] && atom (text, re, m))
    return expr (text + 1, re + i + 1, m);
  return 0;
}

/*
 * Matches the '*' operator in a regular expression 
 */
static int
star (const char *text, const char *s, const char *re, struct mstruct *m)
{
  if (m->flags & GREEDY)
    {
      const char *t;
      for (t = text; t[0] && atom (t, s, m); t++);
      do
        {
          if (expr (t, re, m))
            return 1;
        }
      while (t-- > text);
    }
  else
    {
      do
        {
          if (expr (text, re, m))
            return 1;
        }
      while (text[0] != '\0' && atom (text++, s, m));
    }

  return 0;
}

/*
 * Matches the '+' operator in a regular expression 
 */
static int
plus (const char *text, const char *s, const char *re, struct mstruct *m)
{
  if (m->flags & GREEDY)
    {
      const char *t;
      for (t = text; t[0] && atom (t, s, m); t++);
      while (t > text)
        {
          if (expr (t, re, m))
            return 1;
          t--;
        }
    }
  else
    {
      while (text[0] != '\0' && atom (text++, s, m))
        if (expr (text, re, m))
          return 1;
    }
  return 0;
}

/*
 * Matches the '?' operator in a regular expression 
 */
static int
ques (const char *text, const char *s, const char *re, struct mstruct *m)
{
  if (m->flags & GREEDY)
    {
      if (text[0] != '\0' && atom (text, s, m))
        if (expr (text + 1, re, m))
          return 1;
      if (expr (text, re, m))
        return 1;
    }
  else
    {
      if (expr (text, re, m))
        return 1;

      if (text[0] != '\0' && atom (text++, s, m))
        if (expr (text, re, m))
          return 1;
    }
  return 0;
}

/*
 * Matches the smallest elements in the regular expression: the character 
 * classes, the individual characters and the '.' operator
 */
static int
atom (const char *text, const char *re, struct mstruct *m)
{
  const char *set;

  if (re[0] == '[')
    {
      /* Character class */
      int f = 0;
      if (re[1] == '!')
        re++;
      for (set = re + 1; set[0] != ']'; set++)
        {
          if (set[0] == '\0')
            return 0;           /* syntax error */

          if (set[0] == ESCAPE_CHAR)
            set++;
          else if (set[1] == '-')
            {
              if (set[2] == '\0')
                return 0;       /* syntax error */
			  else if(set[2] == ']') /* As in [abcde-] */
				  goto check1;
			  
              if (m->flags & CASE_INSENSITIVE)
                {
                  if (tolower (text[0]) >= tolower (set[0]) &&
                      tolower (text[0]) <= tolower (set[2]))
                    f = 1;
                }
              else if (text[0] >= set[0] && text[0] <= set[2])
                f = 1;

              set += 2;
              continue;
            }
check1:
          if (m->flags & CASE_INSENSITIVE)
            {
              if (tolower (set[0]) == tolower (text[0]))
                f = 1;
            }
          else if (set[0] == text[0])
            f = 1;
        }

      return (re[0] == '!') ? (!f) : (f);
    }
  else if (re[0] == ESCAPE_CHAR)
    {
      /* This implementation use ESCAPE_CHAR followed by a character to denote 
       * special character classes: \a for alphabetic, \d for digits, etc.
       * So '\a' is the same as '[[:alpha:]]' in other (POSIX) 
       * implementations, for example.
       */
      switch (re[1])
        {
        case 'a':
          return isalpha (text[0]);
        case 'w':
          return isalnum (text[0]);
        case 'd':
          return isdigit (text[0]);
        case 'u':
          return isupper (text[0]);
        case 'l':
          return islower (text[0]);
        case 'x':
          return isxdigit (text[0]);
        case 's':
          return isspace (text[0]);
        case 'A':
          return !isalpha (text[0]);
        case 'W':
          return !isalnum (text[0]);
        case 'D':
          return !isdigit (text[0]);
        case 'U':
          return !isupper (text[0]);
        case 'L':
          return !islower (text[0]);
        case 'X':
          return !isxdigit (text[0]);
        case 'S':
          return !isspace (text[0]);        
        default:
          re++;
          break;
        }
    }

  /* Compare an actual character or a '.' */
  if (m->flags & CASE_INSENSITIVE)
    return tolower (re[0]) == tolower (text[0]) || re[0] == '.';

  return re[0] == text[0] || re[0] == '.';
}

#define RESIZE(q, o, len)  do{\
			if(q - o >= (int)len) \
			{\
				char *t;\
				len += (len>2)?len >> 1:2; \
				t = realloc(o, len + 1);\
				if(!t) \
				{\
					free(o);\
					return NULL;	\
				}\
				q = t + (q - o);\
				o = t;\
			}\
			} while(0)

/* Substitutes the first part of 'text' that matches 're' with 'sub'.
 * Using a '&' in sub will replace that part of 'sub' with the part of
 * Text that matched the 
 *  For example, match_sub("#foooo#", "fo+", "|&|") will return "#|foooo|#"
 *  use a '/' to escape the '&' (eg '/&') and use a '//' to have a single
 *  '/' (I chose '/' to avoid C's use of the '\' causing confusion)
 *  For example match_sub("#foooo#", "fo+", "// /&") will return "#/ &#"
 * It returns the result that should be free()'d afterwards.
 * It may return NULL on a malloc() failure
 */
static char *
int_match_sub (const char *text, const char *re, const char *sub, char once)
{
  const char *beg, *end, *p = text, *r, *u;
  char *o, *q;
  size_t len;

  if (!rx_search (text, re, &beg, &end))
    {
      char *a;
      size_t len = strlen (text);
      a = malloc (len + 1);
      if (!a)
        return NULL;
      memcpy (a, text, len + 1);
      return a;
    }

  assert (beg <= end);

  len = strlen (text);
  o = malloc (len + 1);
  if (!o)
    return NULL;
  q = o;

  do
    {
      for (; p < beg && p[0]; p++, q++)
        {
          RESIZE (q, o, len);
          q[0] = p[0];
        }

      for (r = sub; r[0]; r++, q++)
        {
          RESIZE (q, o, len);     /* resize o? */
          if (r[0] == GSUB_ESCAPE)
            {
              r++;
              q[0] = r[0];
            }
          else if (r[0] == GSUB_SUBMATCH)
            {
              for (u = beg; u < end; u++, q++)
                {
                  RESIZE (q, o, len);     /* resize o? */
                  q[0] = u[0];
                }
              q--;
            }
          else
            q[0] = r[0];
        }
      p = end;
    }
  while (rx_search (end, re, &beg, &end) && !once);

  for (; p[0]; p++, q++)
    {
      RESIZE (q, o, len);         /* resize o? */
      q[0] = p[0];
    }

  q[0] = '\0';

  /* If this fails something's wrong with the RESIZE above */
  assert (strlen (o) <= len);

  return o;
}

char *
rx_sub (const char *text, const char *re, const char *sub)
{
  return int_match_sub (text, re, sub, 1);
}

char *
rx_gsub (const char *text, const char *re, const char *sub)
{
  return int_match_sub (text, re, sub, 0);
}

