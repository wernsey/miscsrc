/* getarg.c: 
 *	Replacement for the POSIX getopt() function for processing
 *	command-line options. I use it for compiling programs that require command-
 *	line arguments under Windows, but my goal is to have it portable to use 
 *	in programs compiled on other platforms.
 *
 *	Although I've consulted the relavant getopt() manual pages, this 
 *	implementation is entirely my own.
 *
 *	Use it as you would use getopt(), but replace all the "opt" prefixes
 *	with "arg"
 *
 * See getarg.h for more info
 *
 * This is free and unencumbered software released into the public domain.
 * http://unlicense.org/
 */
 
#include <stdio.h>

char *argarg = 0;
int argind = 1, argopt, argerr = 1;
 
char getarg(int argc, char * const argv[], const char *opts)
{
	static char *a = 0;
	const char *c;
	
	argopt = 0;
	argarg = 0;
	
	if(!opts || !argv) return -1;
	
	if(!a) 
	{
		if(argind >= argc) return -1;
		a = argv[argind];
		if(a[0] != '-') 
			return -1;
		else
			a++;
	}
	else if(a[0] == '\0')
	{
		if(++argind >= argc) return -1;
		
		a = argv[argind];
		if(a[0] != '-') 
			return -1;
		else
			a++;
	}	
	
	argopt = a[0];	
	
	if(a[0] == '-')
	{
		argind++;
		return -1;
	}
	
	for(c = opts; c[0]; c++)
	{		
		if(c[0] == a[0])
		{			
			if(c[1] == ':')
			{
				if(a[1] != '\0')
				{
					argarg = a + 1;
					
					while(a[0]) a++;
					return argopt;
				}
				else
				{
					a++;
					if(++argind >= argc) 
					{
						if(argerr) fprintf(stderr, "argument '-%c' missing a value\n", a[0]);
						return ':';
					}
					argarg = argv[argind];	
					return argopt;				
				}
			}
			else
			{
				a++;				
				return argopt;
			}
		}
	}
	
	if(argerr) fprintf(stderr, "unknown option '-%c'\n", a[0]);
	return '?';
}
