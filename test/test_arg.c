#include <stdio.h>

#include "../getarg.h"

int main(int argc, char *argv[])
{	
	char c;
	printf("hello world\n");
	
	while((c = getarg(argc, argv, "abcd:e:?")) != -1)
	{
		switch(c)
		{
			case 'a': printf("Option 'a' chosen\n"); break;
			case 'b': printf("Option 'b' chosen\n"); break;
			case 'c': printf("Option 'c' chosen\n"); break;
			case 'd': printf("Option 'd' chosen: \"%s\"\n", argarg); break;
			case 'e': printf("Option 'e' chosen: \"%s\"\n", argarg); break;
			case '?':
			{
				if(argopt == '?')
				{
					printf("Option '?' chosen\n");
				}
				else
				{
					fprintf(stderr, "Unknown option '%c'\n", argopt);								
					return 1;
				}
			} break;
			case ':':
			{
				fprintf(stderr, "Option '%c' missing argument\n", argopt);
				return 1;
			}
			default: break;
		}		
	}
	
	if(argind < argc)
	{
		printf("The remaining parameters are:\n");
		while(argind < argc) printf("- \"%s\"\n", argv[argind++]);
	}
	
	return 0;
}
