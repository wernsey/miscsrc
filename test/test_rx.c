/*
 *	Test program for the rx_sub and rx_gsub functions. Compile like so:
 *	gcc -Wall -Werror -pedantic -O2 -s -fno-exceptions -DTEST_GSUB regex.c
 */

#include <stdio.h>
#include <stdlib.h>

#include "../regex.h"

int
main (int argc, char *argv[])
{
  char *res;
  if (argc < 4)
    {
      printf ("Usage: %s string pattern substitution\n", argv[0]);
      return 1;
    }

  res = rx_sub (argv[1], argv[2], argv[3]);
  if (!res)
    {
      fprintf (stderr, "error: out of memory\n");
      return 1;
    }
  printf ("sub(%s,%s,%s) = %s\n", argv[1], argv[2], argv[3], res);

  res = rx_gsub (argv[1], argv[2], argv[3]);
  if (!res)
    {
      fprintf (stderr, "error: out of memory\n");
      return 1;
    }
  printf ("gsub(%s,%s,%s) = %s\n", argv[1], argv[2], argv[3], res);
  free (res);

  return 0;
}
