#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "../ini.h"

/*## TODO: Make the test program nicer ##*/

#define TEST_FILE "initest.ini"

int write_test_file();

/*
 *	Compile like so with GCC
 *	>gcc -Wall -Werror -pedantic -O0 -g -DTEST ini.c
 */
int main(int argc, char *argv[])
{
	int i, err, line;
	struct ini_file *ini;	
	
	if(argc > 1) {
		/* read all the INI files and print them: */
		for(i = 1; i < argc; i++) {
			/* Read the INI file */
			ini = ini_read(argv[i], &err, &line);
			if(!ini) {
				fprintf(stderr, "Error: Unable to read \"%s\": %s on line %d\n", TEST_FILE, ini_errstr(err), line);
				return 1;
			}
						
			/* Print the read file to stdout */		
			ini_write(ini, NULL);
			printf("-------------\n");
		}
		return 0;
	}
	
	srand(time(NULL)); /* To make things interesting :) */
	
	/* Create a test file: You can look at its contents for the
	supported syntax */
	if(!write_test_file()) {
		fprintf(stderr, "Unable to create test file\n");
		return 1;
	}
	
	/* Read the INI file */
	ini = ini_read(TEST_FILE, &err, &line);
	if(!ini) {
		fprintf(stderr, "Error: Unable to read \"%s\": %s on line %d\n", TEST_FILE, ini_errstr(err), line);
		return 1;
	}
	printf("-------------\n");
	
	/* Print the read file to stdout */	
	ini_write(ini, NULL);
	
	printf("-------------\n");	
	/* Extract values from the config file */	
	/* Use NULL as the section in ini_get() to get global values: */
	printf("Global1: \"%s\"\n", ini_get(ini, NULL, "global1", "UNDEF"));
	/* Parameter names are not case sensitive: */
	printf("Global2: \"%s\"\n", ini_get(ini, NULL, "GLOBAL2", "UNDEF"));
	
	/* To get the value of a parameter within a section: */
	printf("first_section/parameter1: \"%s\"\n", 
		ini_get(ini, "first_section", "parameter1", "UNDEF"));	
	/* Section names aren't case sensitive either */
	printf("first_section/parameter2: \"%s\"\n", 
		ini_get(ini, "FIRST_SECTION", "PARAMETER2", "UNDEF"));
	/* first_section does not have a parameter3. It will return "UNDEF" */
	printf("first_section/parameter3: \"%s\"\n", 
		ini_get(ini, "first_section", "parameter3", "UNDEF"));
	
	/* For completeness */
	printf("second_section/parameter1: \"%s\"\n", 
		ini_get(ini, "second_section", "parameter1", "UNDEF"));
	printf("second_section/parameter2: \"%s\"\n", 
		ini_get(ini, "second_section", "parameter2", "UNDEF"));
	printf("second_section/parameter_3: \"%s\"\n", 
		ini_get(ini, "second_section", "parameter_3", "UNDEF"));
	printf("second_section/parameter-4: \"%s\"\n",
		ini_get(ini, "second_section", "parameter-4", "UNDEF"));
	printf("second_section/parameter.5: \"%s\"\n", 
		ini_get(ini, "second_section", "parameter.5", "UNDEF"));
		
	/* Set some parameters: */
	/* Change the value of global 2 */
	if(!ini_put(ini, NULL, "global2", "New value for global 2"))
		fprintf(stderr, "Unable to set value\n");
	/* Add a new global parameter: */
	if(!ini_put(ini, NULL, "global3", "New global parameter"))
		fprintf(stderr, "Unable to set value\n");
	/* Give a new value for first_section/parameter1 */
	if(!ini_put(ini, "first_section", "parameter1", "New value for parameter 1"))
		fprintf(stderr, "Unable to set value\n");
	
	/* Create a new section called "third_section" and parameters */
	if(!ini_put(ini, "third_section", "parameter1", "Value 1"))
		fprintf(stderr, "Unable to set value\n");
	if(!ini_put(ini, "third_section", "parameter2", "Value 2"))
		fprintf(stderr, "Unable to set value\n");
	if(!ini_put(ini, "third_section", "parameter3", "Value 3;"))
		fprintf(stderr, "Unable to set value\n");	
		
	/* You can use ini_putf() to place non-string data in the ini_file */
	if(!ini_putf(ini, NULL, "time", "%d", time(NULL)))
		fprintf(stderr, "Unable to set value\n");
	
	if(!ini_putf(ini, "random", "integer", "%d", rand()))
		fprintf(stderr, "Unable to set value\n");
	
	if(!ini_putf(ini, "random", "double", "%f", ((float)rand())/100.0f))
		fprintf(stderr, "Unable to set value\n");		
	
	/* Write the contents of the ini_file object to a file "out.ini" */		
	if(ini_write(ini, "out.ini") != 1)
		fprintf(stderr, "Error: Unable to save \"out.ini\"\n");		
	
	/* Deallocate the ini_file */
	ini_free(ini);			
	
	/*****************
	 *	This section demonstrates generating a INI file programatically:
	 *****************/
	
	/* Create an empty ini_file object by passing NULL to ini_read() */
	ini = ini_read(NULL, &err, &line);
	if(!ini) {
		/* The line is irrelevant here */
		fprintf(stderr, "Error: Unable to create empty file: %s\n", ini_errstr(err));		
		return 1;
	}
	
	/* Set some parameters */
	ini_put(ini, "foo", "foo1", "Foo Value 1");
	ini_put(ini, "foo", "foo2", "Foo Value 2");
	ini_put(ini, "foo", "foo3", "Foo Value 3");
	ini_put(ini, "bar", "bar1", "Bar's Value 1#");
	ini_put(ini, "BAR", "bar2", "Bar's Value 2;");
	ini_put(ini, "bar", "bar3", "Bar's Value 3;");
	ini_put(ini, NULL, "global1", "Global Value 1");
	ini_put(ini, NULL, "global2", "Global Value 2");
	
	/* Save the new INI file */
	if(ini_write(ini, "new.ini") != 1)
		fprintf(stderr, "Error: Unable to save new.ini\n");
	
	/* Deallocate the ini_file */
	ini_free(ini);
	
	return 0;
}

/*
 *	Creates a simple INI file to demonstrate the syntax supported.
 *	I'd rather recommend ini_write() to create INI files programatically,
 *	but that does not demonstrate what I want to demonstrate
 */
int write_test_file() {
	FILE *f;
	f = fopen(TEST_FILE, "w");
	if(!f) return 0;
	
	fprintf(f,	"; This is a sample INI file to demonstrate the syntax\n"
				"; lines starting with semicolons are comments\n"
				"# lines starting with hashes are also comments\n"
				"\n"
				"; Parameters outside of sections are considered global\n"
				"; Parameters are specified like \"parameter = value\""
				" as follows:\n"
				"global1 = value1 ; Comments may follow values\n"
				"global2 = \"another global variable\" \n"
				"a-long-string : \"\"\"This String is very long,\nspans multiple lines,\nand uses \\\"\\\"\\\" to delimit it. \"\"\"\n"
				"\n");
	fprintf(f,	"[ Numbers ]\n"
				"x : 3.5\n"
				"y = 7.25\n"
				"\n");
	fprintf(f,	"; Sections are specified within square brackets\n"
				"[first_section]\n"
				" parameter1 = value1\n"
				" # To use semicolons and hashes within values:\n"
				" ; specify them within quotes:\n"
				" parameter2 = \"#value 2;\"\n"
				" ; Duplicate parameters are allowed, but only the first\n"
				" ; is \"seen\" by ini_get():\n"
				" parameter2 = foo\n" 
				"\n");
				
	fprintf(f,	"  ; Whitespace is insignificant: \n"
				"  [  second_section  ]\n"
				" parameter1 = value1\n"
				" ; C type escapes can be used within quoted values: \n"
				" parameter2 = \"To specify a backslash: \\\\\"\n"				
				" parameter_3 = \"To specify a tab: \\t\"\n"
				" parameter-4 = \"To specify a newline: \\n\"\n"
				" parameter.5 = \"To specify a carriage return: \\r\"\n");
	fclose(f);
	return 1;
}
