#include <stdio.h>

#include "../eval.h"

int main(int argc, char *argv[])
{
	int i, err;
	double e;
		
	for(i = 1; i < argc; i++)
	{
		e = eval(argv[i], &err);
		if(err)
			fprintf(stderr, "Error in expression %s: %s\n", argv[i], eval_error(err));
		else
			printf("%s = %g\n", argv[i], e);
	}
	
	return 0;
}
