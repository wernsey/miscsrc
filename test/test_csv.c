#include <stdio.h>

#include "../csv.h"

int main(int argc, char *argv[])
{	
	csv_file *csv;
	int e, line;
	int r,c;
	
	char *scase [] = {"A string with a \" in it",
					"A string with a , in it",
					"A string with a newline\n in it"};
	
	if(argc > 1)
	{
		csv = csv_load(argv[1], &e, &line);
		if(!csv)
		{
			fprintf(stderr, "Error reading %s: line %d: %s\n", argv[1], line, csv_errstr(e));
			return 1;
		}
		
		for(r = 0; r < csv_rowcount(csv); r++)
		{
			for(c = 0; c < csv_colcount(csv, r); c++)
				printf("%d:%d |%s|\n",r,c, csv_get(csv,r,c));
		}
		
		return 0;
	}
	
	csv = csv_create(2,2);
	if(!csv)
	{
		fprintf(stderr, "Error: Couldn't create CSV structure\n");
		return 1;
	}
	
	if((e = csv_set(csv,0,0,"field 1")) != 1)
	{
		fprintf(stderr, "Error: Couldn't set value because %s\n", csv_errstr(e));
		return 1;
	}

	if((e = csv_set(csv,0,1,"field 2")) != 1)
	{
		fprintf(stderr, "Error: Couldn't set value because %s\n", csv_errstr(e));
		return 1;
	}
	
	if((e = csv_set(csv,0,2,"field 3")) != 1)
	{
		fprintf(stderr, "Error: Couldn't set value because %s\n", csv_errstr(e));
		return 1;
	}
	
	for(r = 1; r <= 10; r++)
		for(c = 0; c < 3; c++)
			if((e = csv_setx(csv,r,c,"%d",r*(c+1))) != 1)
			{
				fprintf(stderr, "Error: Couldn't set value because %s\n", csv_errstr(e));
				return 1;
			}
			
	/* Some special cases */
	for(c = 0; c < 3; c++)
		if((e = csv_set(csv,r,c,scase[c])) != 1)
		{
			fprintf(stderr, "Error: Couldn't set value because %s\n", csv_errstr(e));
			return 1;
		}
		
	if((e = csv_save(csv, "test.csv")) != 1)
	{
		fprintf(stderr, "Error: Couldn't save CSV file because %s\n", csv_errstr(e));
		return 1;
	}
		
	csv_free(csv);
	
	return 0;
}
