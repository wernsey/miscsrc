#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "../hash.h"

void
show (struct hash_tbl *h)
{
  struct hash_el *e;
  int i;

  for (i = 0; i < h->size; i++)
    if (h->buckets[i])
      for (e = h->buckets[i]; e; e = e->next)
        printf ("%s %s\n", e->key, (char *) e->value);
}

/* This is a simple but inefficient way to iterate through
 * the hash table, although it requires two lookups
 * per key
 */
static void iterate (struct hash_tbl *h) {
  const char *key = NULL;

  while ((key = ht_next (h, key)) != NULL)
    printf ("%s %s\n", key, (char *) ht_find (h, key));
}

static int printfun (const char *key, void *value, void *data) {
  printf ("%s -> %s\n", key, (char *) value);
  return 1;
}

static void dtor(const char *key, void *val) {
  free(val);
}

int main (int argc, char *argv[]) {
  struct hash_tbl *h;
  char cmd[20], key[20], data[20], *res;

  h = ht_create (8);            /* Smallish size for test purposes */
  if (!h) {
    fprintf(stderr, "error: ht_create() failed\n");
    return 1;
  }

  while (1) {
      printf (">");
      fflush (stdout);

      scanf ("%s", cmd);
      if (!strcmp (cmd, "end") || !strcmp (cmd, "quit") || !strcmp (cmd, "exit"))
        break;
      else if (!strcmp (cmd, "add"))
        {
          char *d;
          size_t len;

          scanf ("%s %s", key, data);

          len = strlen (data);
          d = malloc (len + 1);
          if (d)
            {
              memcpy (d, data, len + 1);
              ht_insert (h, key, d);
            }
          else
            fprintf (stderr, "error: %s\n", strerror (errno));
        }
      else if (!strcmp (cmd, "find"))
        {
          scanf ("%s", key);
          res = ht_find (h, key);
          if (res)
            printf ("%s ~> %s\n", key, res);
          else
            printf ("%s not found\n", key);
        }
      else if (!strcmp (cmd, "next"))
        {
          const char *next;
          scanf ("%s", key);
          next = ht_next (h, key);
          if (next)
            printf ("%s => %s %s\n", key, next, (char *) ht_find (h, next));
          else
            printf ("%s does not have a next element\n", key);
        }
      else if (!strcmp (cmd, "delete"))
        {
          scanf ("%s", key);
          res = ht_delete (h, key);
          if (res)
            {
              free (res);
              printf ("%s deleted\n", key);
            }
          else
            printf ("%s not found\n", key);
        }
      else if (!strcmp (cmd, "foreach"))
        {
          ht_foreach (h, printfun, NULL);
        }
      else if (!strcmp (cmd, "show"))
        show (h);
      else if (!strcmp (cmd, "iterate"))
        iterate (h);
      else
        fprintf (stderr, "error: unknown command\n");
      fflush (stdout);
    }

  ht_free (h, dtor);
  return 0;
}
