#include <stdio.h>

#include "../simil.h"

int
main (int argc, char *argv[])
{
  if (argc != 3)
    {
      fprintf (stderr, "Usage: %s string1 string2", argv[0]);
      return 1;
    }

  printf ("Case sensitive Similarity....: %d\n", simil (argv[1], argv[2]));
  printf ("Case-insensitive Similarity..: %d", isimil (argv[1], argv[2]));

  return 0;
}
