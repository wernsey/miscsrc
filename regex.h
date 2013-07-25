/*1 regex.h
 *#	This is a regular expression evaluator based on code from the article
 *#	"Regular Expressions: Languages, algorithms, and software"
 *#	By Brian W. Kernighan and Rob Pike in the April 1999 issue of
 *#	Dr. Dobb's Journal
 *#
 *#	I've found the article online at
 *#	http://www.ddj.com/dept/architect/184410904
 *#
 *2 License
 *[
 *# Author: Werner Stoop
 *# This software is provided under the terms of the unlicense.
 *# See http://unlicense.org/ for more details.
 *]
 *2 Usage
 *{
 ** {{~~rx_match()}} is used to match text to a pattern. 
 ** {{~~rx_search()}} is used to find the position of text matching a pattern
 *#    for further processing.
 ** {{~~rx_sub()}} is used to substitute a piece of text matching a regex with
 *#    some other text.
 ** {{~~rx_gsub()}} is used to substitute all pieces of the text matching a regex 
 *#    with some other text. 
 *}
 *2 Syntax
 *# The evaluator implements only a subset of the usual regular expression
 *# language.
 *{
 ** The {{'*'}}, {{'+'}} and {{'?'}} operators
 ** Character sets {{'[abc]'}} with ranges ({{'[a-c]'}}) and inversion ({{'[!abc]'}})
 *#		To match a {{'-'}} in the character set, place it before the closing {{']'}},
 *#		like {{'[abcde-]'}}
 ** Special characters can be escaped using the characters '\'
 ** Additionally, a '\' can be used for specific character classes:
	 *{
     ** {{\a}} - Alphabetic characters: {{[a-zA-Z]}}
     ** {{\w}} - Word characters: {{[a-zA-Z0-9]}}
     ** {{\d}} - Digits: {{[0-9]}}
     ** {{\u}} - Uppercase characters: {{[A-Z]}}
     ** {{\l}} - Lowercase characters: {{[a-z]}}
     ** {{\x}} - Hexadecimal digits: {{[0-9a-fA-F]}}
     ** {{\s}} - Whitespace characters
     *}
     *# Upper case versions of these are used to negate the class. For example,
     *# {{\A}} matches anything that is not an alphabetic character: {{[!a-zA-Z]}} 
 ** Case-insensitive matching is enabled with {{'\i'}} and disabled with {{'\I'}}
 *}
 *#	The tradeoff is a very compact implementation.
 *#
 *#	To use this module in your program, you can include "regex.h" at the top,
 *#	and link against the compiled objact file.
 *#
 *#	Features not supported, but found in other regex engines:
 *{
 ** Alternation '|'
 ** Grouping and submatch extraction '(abc)'
 **	The '{m,n}' operator
 *}
 *2 API
 */

#ifndef REGEX_H
#define REGEX_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C"
{
#endif

/*@ int ##rx_match (const char *text, const char *re)
 *# Checks whether the text {{text}} contains the regular expression {{re}}.\n
 *# It returns non-zero if the regular expression {{re}} was found in {{text}},
 *#		zero otherwise
 */
  int rx_match (const char *text, const char *re);

/*@ int ##rx_search (const char *text, const char *re, const char **beg, const char **end);
 *# Checks whether the text {{text}} contains the regular expression {{re}} and 
 *# extracts the match.\n
 *# It uses greedy matching internally to locate the leftmost longest match.
 *# If {{text}} matches the expression the char pointer pointed to
 *# 	by {{beg}} will contain the address of the first character in {{text}}
 *# 	that matched {{re}} and {{end}} will contain the address of the last 
 *# 	character in {{text}} that matched {{re}}.\n
 *# It returns non-zero if the regular expression {{re}} was found in {{text}},
 *# 	zero otherwise.
 */
  int rx_search (const char *text, const char *re, const char **beg,
                 const char **end);

/*@ char *##rx_sub (const char *text, const char *re, const char *sub)
 *# Substitutes the first occurance of {{text}} that matches 
 *# {{re}} with {{sub}}.\n
 *# Using a {{'&'}} in sub will replace that part of {{sub}} with the part of
 *# {{text}} that matched {{re}}.\n
 *# For example, {{rx_sub("#foooo#", "fo+", "|&|")}} will return {{"#|foooo|#"}}.\n
 *# Use a {{'/'}} to escape the {{'&'}} (eg {{'/&'}}) and use a {{'//'}} to have a single
 *# {{'/'}} ({{'/'}} was chosen to avoid C's use of the {{'\'}} causing confusion).\n
 *# For example {{rx_sub("#foooo#", "fo+", "// /&")}} will return {{"#/ &#"}}\n
 *# It returns the result that should be {{free()}}'d afterwards.
 *# 	It may return {{NULL}} on a {{malloc()}} failure.
 */
  char *rx_sub (const char *text, const char *re, const char *sub);

/*@ char *##rx_gsub (const char *text, const char *re, const char *sub)
 *# Substitutes all occurances of {{text}} that matches {{re}} with {{sub}}.\n
 *# Using a {{'&'}} in sub will replace that part of {{sub}} with the part of
 *# {{text}} that matched {{re}}.\n
 *# For example, {{rx_gsub("#foooo#", "fo+", "|&|")}} will return {{"#|foooo|#"}}.\n
 *# Use a {{'/'}} to escape the {{'&'}} (eg {{'/&'}}) and use a {{'//'}} to have a single
 *# {{'/'}} ({{'/'}} was chosen to avoid C's use of the {{'\'}} causing confusion).\n
 *# For example {{match_sub("#foooo#", "fo+", "// /&")}} will return {{"#/ &#"}}.\n
 *# It returns the result that should be {{free()}}'d afterwards.
 *# 	It may return {{NULL}} on a {{malloc()}} failure.
 */
  char *rx_gsub (const char *text, const char *re, const char *sub);

#if defined(__cplusplus) || defined(c_plusplus)
}                               /* extern "C" */
#endif

#endif                          /* REGEX_H */
