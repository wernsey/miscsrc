#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

/* Case insensitive strcmp()
 */ 
int my_stricmp(const char *p, const char *q) 
{ 
	for(;*p && tolower(*p) == tolower(*q); p++, q++);
	return tolower(*p) - tolower(*q);
} 

/* strdup() is not ANSI C */
char *my_strdup(const char *s)
{
	char *a;
	size_t len = strlen(s);
	a = malloc(len + 1);
	if(!a) return NULL;
	memcpy(a, s, len + 1);	
	return a;
}

/* converts a string to lowercase */
char *my_strlower (char *p)
{
  char *s;
  for (s = p; s[0]; s++)
    s[0] = tolower (s[0]);

  return p;
}

/* converts a string to lowercase */
char *my_strupper (char *p)
{
  char *s;
  for (s = p; s[0]; s++)
    s[0] = toupper (s[0]);

  return p;
}

/* Reads an entire file into a dynamically allocated memory buffer.
 * The returned buffer needs to be free()d afterwards
 */
char *my_readfile(const char *fname)
{
	FILE *f;
	long len,r;
	char *str;
	
	if(!(f = fopen(fname, "rb")))	
		return NULL;
	
	fseek(f, 0, SEEK_END);
	len = ftell(f);
	rewind(f);
	
	if(!(str = malloc(len+2)))
		return NULL;	
	r = fread(str, 1, len, f);
	
	if(r != len)
	{
		free(str);
		return NULL;
	}
	
	fclose(f);	
	str[len] = '\0';
	return str;
}
