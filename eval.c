/*
 * Mathematical expression evaluator.
 *
 * See eval.h for more info
 *
 * This is free and unencumbered software released into the public domain.
 * http://unlicense.org/
 */
 
/*
 *	You can use the following awk script to obtain the syntax of the parser in
 *	[my own] EBNF form:
 *	
 * awk '/\*#/{a=index($0,"#")+2;print(substr($0,a))}' eval.c
 *
 * The following awk script will print a list of all the functions and constants
 *	defined in the script from the comments.
 *
 * awk '/\*\$/{a=index($0,"$")+2;b=index($0,"* /");print(substr($0,a,b-a))}' eval.c
 *	
 * NOTE: there's a space in the 'index($0,"* /")' above for the sake of this comment
 */

#include <stdlib.h>
#include <math.h>  /* remember to compile with -lm */
#include <string.h>
#include <ctype.h>
#include <setjmp.h>
#include <assert.h>

#include "eval.h"

/* Special tokens used by the lexer function lex() 
 *	they've been chosen as non-printable characters
 *	so that printable characters can be used for other
 *	purposes
 */
#define TOK_END			0  /* end of text */
#define TOK_INI			1	/* Initial state */
#define TOK_ID			2  /* identifier */
#define TOK_NUM		3  /* number */

/* Types of errors */
#define ERR_MEMORY	1
#define ERR_LEXER		2
#define ERR_LONGID	3
#define ERR_VALUE		4
#define ERR_BRACKET	5
#define ERR_FUNC		6
#define ERR_ARGS		7
#define ERR_CONST		8

/* Other definitions */
#define MAX_ID_LEN	11		/* Max length of an identifier */
#define OPERATORS	"+-*/%(),^"	/* Valid operators */

#define EVAL_PI			3.141592654
#define EVAL_E			2.718281828

/* Internal structure for the parser/evaluator */
struct eval {
	
	jmp_buf j;		/* For error handling */	
	
	const char *p;	/* Position in the text being parsed */
		
	double *st;		/* Stack */
	int st_size;		/* Stack size */
	int sp;				/* Stack pointer */
			
	/* The current and next tokens identified by the lexer */
	struct {
		int type;									 /* Type of the token */
		double	n_val;						 /* Numeric value of the previous lexed token */
		char		s_val[MAX_ID_LEN];   /* String (identifier) value of the previous lexed token */
	} token[2];
	
	int cur_tok;		/* Current token, either 0 or 1 (see the comments of lex()) */
};

/* Prototypes */
static double pop(struct eval *ev);
static void push(struct eval *ev, double d);
static int lex(struct eval *ev);

/* Prototypes for the recursive descent parser */
static void expr(struct eval *ev);
static void add_expr(struct eval *ev);
static void mul_expr(struct eval *ev);
static void pow_expr(struct eval *ev);
static void uni_expr(struct eval *ev);
static void bra_expr(struct eval *ev);
static void id_expr(struct eval *ev);
static void num_expr(struct eval *ev);

/*
 *	Evaluates a mathemeatical expression
 */
double eval(const char *exp, int *ep)
{
	struct eval ev;
	double ans = 0.0;
	
	assert(ep != NULL);
	
	/* Allocate a stack */
	ev.st_size = 10;
	ev.st = calloc(ev.st_size, sizeof *ev.st);
	if(!ev.st)
	{
		*ep = ERR_MEMORY;
		return 0.0;
	}
	ev.sp = 0;
	
	/* Manage errors */				
	*ep = setjmp(ev.j);
	if(*ep != 0)
	{
		free(ev.st);
		return 0.0;
	}
	
	/* Initialize the lexer */
	ev.token[0].type = TOK_INI;
	ev.token[0].s_val[0] = '\0';
	ev.token[1].type = TOK_INI;
	ev.token[1].s_val[0] = '\0';
	ev.cur_tok = 0;
	
	/* Initialize the parser */
	ev.p = exp;	
	
	/* lex once to initialize the lexer */
	if(lex(&ev) != TOK_END)
	{	
		expr(&ev);
		ans = pop(&ev);
	}
	
	free(ev.st);
	return ans;
}

/*
 * Pushes a value onto the stack, increases the stack size if necessary 
 */
static void push(struct eval *ev, double d)
{
	if(ev->sp == ev->st_size)
	{
		/* Resize the stack by 1.5 */
		double *old = ev->st;
		int new_size = ev->st_size + (ev->st_size >> 1);
		ev->st = realloc(ev->st, new_size);			
		if(!ev->st)
		{
			ev->st = old;
			longjmp(ev->j, ERR_MEMORY);
		}
		
		ev->st_size = new_size;
	}
	
	ev->st[ev->sp++] = d;
}

/*
 *	Pops a value from the top of the stack
 */
static double pop(struct eval *ev)
{
	assert(ev->sp > 0);
	return ev->st[--ev->sp];
}

/* stricmp() is common, but not standard, so I provide my own */
static int istrcmp(const char *p, const char *q)
{
	for(; tolower(p[0]) == tolower(q[0]) && p[0]; p++, q++);
	return tolower(p[0]) - tolower(q[0]);
}

/*
 *	Returns a string describing a specific error code
 */
const char *eval_error(int err)
{
	switch(err)
	{
	case 0:						return "no error";
	case ERR_MEMORY: 	return "out of memory";
	case ERR_LEXER: 		return "unknown token";
	case ERR_LONGID:		return "identifier too long";
	case ERR_VALUE:		return "value expected";
	case ERR_BRACKET:	return "missing ')'";
	case ERR_FUNC:			return "unknown function";
	case ERR_ARGS:			return "wrong number of arguments";
	case ERR_CONST:		return "unknown constant";
	}
	return "unknown error";
}

/*
 *	Lexical analyzer function
 *
 *	In order to implement LL(1), struct eval has an array of two token structures,
 *	and its cur_tok member is used to point to the _current_ token, while the other
 *	element contains the _next_ token. This implements a 2 element ring buffer where
 *	the lexer always writes to the _next_ token so that the recursive descent parser can 
 * _peek_ at the next token.
 */
static int lex(struct eval *ev)
{
	int next_tok;
	
start:
	/* Cycle the tokens */
	next_tok = ev->cur_tok;
	ev->cur_tok = ev->cur_tok?0:1;
		
	while(isspace(ev->p[0])) ev->p++;
	
	if(!ev->p[0])
	{	
		/* End of the expression */
		ev->token[next_tok].type = TOK_END;
		goto end;
	}
	else if(isdigit(ev->p[0]) || ev->p[0] == '.')
	{
		/* Number */
		char *endp;
		ev->token[next_tok].type = TOK_NUM;
		ev->token[next_tok].n_val = strtod(ev->p, &endp);
		ev->p = endp;
		goto end;
	}
	else if(isalpha(ev->p[0]))
	{
		/* Identifier */
		int i;
		for(i = 0; isalnum(ev->p[0]) && i < MAX_ID_LEN - 1; i++, ev->p++)
			ev->token[next_tok].s_val[i] = ev->p[0];
		
		if(isalpha(ev->p[0])) longjmp(ev->j, ERR_LONGID);
		
		ev->token[next_tok].s_val[i] = '\0';
		ev->token[next_tok].type = TOK_ID;
		goto end;
	}
	else if(strchr(OPERATORS, ev->p[0]))
	{
		/* Operator */
		ev->token[next_tok].type = ev->p[0];
		ev->p++;
		goto end;
	}		
	else /* Unknown token */
		longjmp(ev->j, ERR_LEXER);
	
end:
	
	/* If this was the first call, cycle the tokens again */
	if(ev->token[ev->cur_tok].type == TOK_INI) 
		goto start;
		
	return ev->token[ev->cur_tok].type;
}

#define THIS(e)  		(e->token[e->cur_tok].type)
#define PEEK(e)  	(e->token[e->cur_tok?0:1].type)
#define GOBBLE(e) 	lex(e)
#define ERROR(c)	longjmp(ev->j, (c))

/*# expr ::= add_expr
 */
static void expr(struct eval *ev)
{
	add_expr(ev);
}

/*# add_expr ::= mul_expr [('+'|'-') mul_expr]*
 */
static void add_expr(struct eval *ev)
{
	int t;
	mul_expr(ev);	
	while((t =THIS(ev)) == '+' || t == '-')
	{
			double a,b;
			GOBBLE(ev);
			mul_expr(ev);
			b = pop(ev);
			a = pop(ev);
			
			if(t == '+')
				push(ev, a + b);
			else
				push(ev, a - b);
	}
}

/*# mul_expr ::= pow_expr [('*'|'/'|'%') pow_expr]*
 */
static void mul_expr(struct eval *ev)
{
	int t;
	pow_expr(ev);
	while((t = THIS(ev)) == '*' || t == '/' || t == '%')
	{
			double a,b;
			GOBBLE(ev);
			pow_expr(ev);
			b = pop(ev);
			a = pop(ev);
			
			if(t == '*')
				push(ev, a * b);
			else if(t == '/')
				push(ev, a / b);
			else
				push(ev, fmod(a, b));
	}
}

/*#	pow_expr ::= uni_expr ['^' pow_expr]
 */
static void pow_expr(struct eval *ev)
{
	/* Note that exponentiation is right associative:
	2^3^4 is 2^(3^4), not (2^3)^4 */
	uni_expr(ev);
	if(THIS(ev) == '^')
	{
		double a,b;
		GOBBLE(ev);
		pow_expr(ev);
		b = pop(ev);
		a = pop(ev);
		push(ev, pow(a,b));
	}
}

/*# uni_expr ::= ['+'|'-'] bra_expr
 */
static void uni_expr(struct eval *ev)
{	
	int t = '+';
	if(THIS(ev) == '-' || THIS(ev) == '+')
	{
		t = THIS(ev);
		GOBBLE(ev);
	}		
	
	bra_expr(ev);
	
	if(t == '-')
	{
		double a = pop(ev);		
		push(ev, -a);
	}
}

/*# bra_expr ::= '(' add_expr ')'
 *#                   | id_expr
 */
static void bra_expr(struct eval *ev)
{
	if(THIS(ev) == '(')
	{
		GOBBLE(ev);
		add_expr(ev);
		if(THIS(ev) != ')')
			ERROR(ERR_BRACKET);
		GOBBLE(ev);
	}
	else
		 id_expr(ev);
}

/*# id_expr ::= ID '(' add_expr [',' add_expr]* ')' 
 *#                |  ID
 *#                |  num_expr
 */
static void id_expr(struct eval *ev)
{
	if(THIS(ev) == TOK_ID)
	{		
		char id[MAX_ID_LEN];
		strcpy(id, ev->token[ev->cur_tok].s_val);
		GOBBLE(ev);
		if(THIS(ev) == '(')
		{			
			int nargs = 0;
			
			GOBBLE(ev);
			
			while(THIS(ev) != ')')
			{
				add_expr(ev);
				nargs++;
				if(THIS(ev) == ')') break;

				if(THIS(ev) != ',')
					ERROR(ERR_BRACKET);
				GOBBLE(ev);
			}
			GOBBLE(ev);
			
			/*$ abs(x) - absolute value of x */
			if(!istrcmp(id, "abs"))
			{
				if(nargs != 1) ERROR(ERR_ARGS);
				push(ev, fabs(pop(ev)));		
			}
			/*$ ceil(x) - smallest integer greater than x */
			else if(!istrcmp(id, "ceil"))
			{
				if(nargs != 1) ERROR(ERR_ARGS);
				push(ev, ceil(pop(ev)));	
			}			
			/*$ floor(x) - largest integer smaller than x */
			else if(!istrcmp(id, "floor"))
			{
				if(nargs != 1) ERROR(ERR_ARGS);
				push(ev, floor(pop(ev)));	
			}			
			/*$ sin(x) - sine of x, in radians */
			else if(!istrcmp(id, "sin"))
			{
				if(nargs != 1) ERROR(ERR_ARGS);
				push(ev, sin(pop(ev)));	
			}
			/*$ asin(x) - arcsine of x, in radians */
			else if(!istrcmp(id, "asin"))
			{
				if(nargs != 1) ERROR(ERR_ARGS);
				push(ev, asin(pop(ev)));	
			}
			/*$ cos(x) - cosine of x, in radians */
			else if(!istrcmp(id, "cos"))
			{
				if(nargs != 1) ERROR(ERR_ARGS);
				push(ev, cos(pop(ev)));	
			}
			/*$ acos(x) - arccosine of x, in radians */
			else if(!istrcmp(id, "acos"))
			{
				if(nargs != 1) ERROR(ERR_ARGS);
				push(ev, acos(pop(ev)));	
			}
			/*$ tan(x) - tangent of x, in radians */
			else if(!istrcmp(id, "tan"))
			{
				if(nargs != 1) ERROR(ERR_ARGS);
				push(ev, tan(pop(ev)));	
			}
			/*$ atan(x) - arctangent of x, in radians */
			else if(!istrcmp(id, "atan"))
			{
				if(nargs != 1) ERROR(ERR_ARGS);
				push(ev, atan(pop(ev)));	
			}
			/*$ atan(y,x) - arctangent of y/x, in radians. */
			else if(!istrcmp(id, "atan2"))
			{
				double a, b;
				if(nargs != 2) ERROR(ERR_ARGS);
				b = pop(ev);
				a = pop(ev);
				push(ev, atan2(a,b));	
			}		
			/*$ sinh(x) - hyperbolic sine of x, in radians */
			else if(!istrcmp(id, "sinh"))
			{
				if(nargs != 1) ERROR(ERR_ARGS);
				push(ev, sinh(pop(ev)));	
			}
			/*$ cosh(x) - hyperbolic cosine of x, in radians */
			else if(!istrcmp(id, "cosh"))
			{
				if(nargs != 1) ERROR(ERR_ARGS);
				push(ev, cosh(pop(ev)));	
			}
			/*$ tanh(x) - hyperbolic tangent of x, in radians */
			else if(!istrcmp(id, "tanh"))
			{
				if(nargs != 1) ERROR(ERR_ARGS);
				push(ev, tanh(pop(ev)));	
			}
			/*$ log(x) - natural logarithm of x */
			else if(!istrcmp(id, "log"))
			{
				if(nargs != 1) ERROR(ERR_ARGS);
				push(ev, log(pop(ev)));	
			}
			/*$ log10(x) - logarithm of x, base-10 */
			else if(!istrcmp(id, "log10"))
			{
				if(nargs != 1) ERROR(ERR_ARGS);
				push(ev, log10(pop(ev)));	
			}
			/*$ exp(x) - computes e^x */
			else if(!istrcmp(id, "exp"))
			{
				if(nargs != 1) ERROR(ERR_ARGS);
				push(ev, exp(pop(ev)));	
			}
			/*$ sqrt(x) - square root of x */
			else if(!istrcmp(id, "sqrt"))
			{
				if(nargs != 1) ERROR(ERR_ARGS);
				push(ev, sqrt(pop(ev)));	
			}
			/*$ rad(x) - converts x from degrees to radians */
			else if(!istrcmp(id, "rad"))
			{
				if(nargs != 1) ERROR(ERR_ARGS);
				push(ev, pop(ev)*EVAL_PI/180);	
			}
			/*$ deg(x) - converts x from radians to degrees */
			else if(!istrcmp(id, "deg"))
			{
				if(nargs != 1) ERROR(ERR_ARGS);
				push(ev, pop(ev)*180/EVAL_PI);	
			}			
			/*$ pow(x,y) - computes x^y */
			else if(!istrcmp(id, "pow"))
			{
				double a, b;
				if(nargs != 2) ERROR(ERR_ARGS);
				b = pop(ev);
				a = pop(ev);
				push(ev, pow(a,b));	
			}			
			/*$ hypot(x,y) - computes sqrt(x*x + y*y) */
			else if(!istrcmp(id, "hypot"))
			{
				double a, b;
				if(nargs != 2) ERROR(ERR_ARGS);
				b = pop(ev);
				a = pop(ev);
				push(ev, sqrt(a*a + b*b));	
			}
			else
				ERROR(ERR_FUNC);			
		}
		else
		{
			/*$ pi - 3.141592654 */
			if(!istrcmp(id, "pi"))
				push(ev, EVAL_PI);
			/*$ e - base of natural logarithms, 2.718281828 */
			else if(!istrcmp(id, "e"))
				push(ev, EVAL_E);
			else
				ERROR(ERR_CONST);	
		}
	}
	else
		num_expr(ev);
}

/*# num_expr ::= NUMBER
 */
static void num_expr(struct eval *ev)
{
	if(THIS(ev) != TOK_NUM)
		ERROR(ERR_VALUE);
		
	push(ev, ev->token[ev->cur_tok].n_val);
	GOBBLE(ev);
}

