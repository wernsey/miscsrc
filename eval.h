/*! eval.h
 *# A mathematical expression evaluator.\n
 *# It uses a recursive descent parser internally.
 *#
 *-
 *[
 *# Copyright (c) 2007-2009 Werner Stoop, zlib/libpng license.
 *]
 *-
 */
#ifndef EVAL_H
#define EVAL_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

/*@ double eval(const char *expr, int *ep);
 *#	Evaluates a mathematical expression.
 *#	{{expr}} is the expression to valuate.
 *#	{{ep}} is a pointer to an integer that will contain an error code if
 *#		any errors were encountered during parsing. {{ep}} will be 0 if
 *#		the evaluation was successful.
 *#	It returns the result of evaluating the expression.
 */
double eval(const char *expr, int *ep);
	
/*@ const char *eval_error(int err);
 *#	Returns a string describing a specific error code.
 *#	{{err}} is the value returned in eval()'s {{ep}} parameter.
 *# It returns a string describing the error.
 */
const char *eval_error(int err);

#if defined(__cplusplus) || defined(c_plusplus)
} /* extern "C" */
#endif

#endif /* EVAL_H */
