/*
 * A quick and dirty hash table implementation.
 *
 * See hash.h for more info
 *
 * This is free and unencumbered software released into the public domain.
 * http://unlicense.org/
 */

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "hash.h"

#define DEFAULT_SIZE	 512
#define MAX_SIZE		 100000
#define FILL_FACTOR(x)	 ((x)/2)
#define RESIZE_FACTOR(x) ((x)*2)

/* The internal hash function.
 *
 * It uses the function described in section 7.6
 * of the "Dragon Book", see page 435 in particular.
 *
 * It computes the modulus through the bitwise AND
 * of the sum and (size-1), therefore the size of 
 * the table must be a power of two.
 */
static int
hash (const char *str, int size)
{
  int x = 0;
  assert (str);

  while (str[0])
    {
      x = (x * 65599) + str[0];
      str++;
    }

  return x & (size - 1);
}

/* Allocates memory for a hash table */
struct hash_tbl *
ht_create (int size)
{
  struct hash_tbl *h;
  int i;

  if (size == 0)
    size = DEFAULT_SIZE;        /*default */
  h = malloc (sizeof *h);
  if (!h)
    return 0;
  h->cnt = 0;
  h->size = size;
  h->buckets = calloc (size, sizeof *h->buckets);
  if (!h->buckets)
    {
      free (h);
      return NULL;
    }
  for (i = 0; i < size; i++)
    h->buckets[i] = NULL;
  return h;
}

int
ht_rehash (struct hash_tbl *ht, int new_size)
{
  struct hash_el **buckets, *j, *e, *k;
  int i, h;

  if (new_size >= MAX_SIZE)
    {
      if (ht->size < MAX_SIZE)
        new_size = MAX_SIZE;
      else
        return 0;
    }

  buckets = calloc (new_size, sizeof *buckets);
  if (!buckets)
    return 0;

  for (i = 0; i < ht->size; i++)
    for (j = ht->buckets[i]; j;)
      {
        h = hash (j->key, new_size);
        e = j;
        j = j->next;
        e->next = NULL;

        if (buckets[h])
          {
            for (k = buckets[h]; k->next; k = k->next);
            k->next = e;
          }
        else
          buckets[h] = e;
      }

  free (ht->buckets);
  ht->buckets = buckets;
  ht->size = new_size;
  return 1;
}

void *
ht_insert (struct hash_tbl *h, const char *key, void *value)
{
  struct hash_el *e;
  int f;
  size_t len;

  if (h->cnt > FILL_FACTOR (h->size))
    ht_rehash (h, RESIZE_FACTOR (h->size));

  e = malloc (sizeof *e);
  if (!e)
    return NULL;

  len = strlen (key);
  e->key = malloc (len + 1);
  if (!e->key)
    {
      free (e);
      return NULL;
    }
  memcpy (e->key, key, len + 1);

  e->value = value;
  e->next = NULL;

  f = hash (key, h->size);

  /* new element in the front of the bucket, for locality of reference, etc. */
  e->next = h->buckets[f];
  h->buckets[f] = e;

  h->cnt++;
  return value;
}

/* Used internally to search the table for a specific key 
 * f will contain the hash table bucket
 */
static struct hash_el *
search (struct hash_tbl *h, const char *key, int *f)
{
  struct hash_el *i;

  *f = hash (key, h->size);
  if (h->buckets[*f])
    {
      for (i = h->buckets[*f]; i; i = i->next)
        if (!strcmp (i->key, key))
          return i;
    }
  return NULL;
}

/* Returns the value associated with a specific key */
void *
ht_find (struct hash_tbl *h, const char *key)
{
  int f;
  struct hash_el *i = search (h, key, &f);
  if (i)
    return i->value;
  return NULL;
}

/* Finds the next element in the table given a specific key */
const char *
ht_next (struct hash_tbl *h, const char *key)
{
  int f;
  struct hash_el *i;

  if (key == NULL)
    {
      for (f = 0; f < h->size && !h->buckets[f]; f++);
      if (f >= h->size)
        return NULL;

      assert (h->buckets[f]);
      return h->buckets[f]->key;
    }
  i = search (h, key, &f);
  if (!i)
    return NULL;
  if (i->next)
    return i->next->key;
  else
    {
      f++;
      while (f < h->size && !h->buckets[f])
        f++;

      if (f >= h->size)
        return NULL;

      assert (h->buckets[f]);

      return h->buckets[f]->key;
    }
}

/* Deletes an element from the hash table */
void *
ht_delete (struct hash_tbl *h, const char *key)
{
  struct hash_el *i, *p = NULL;
  void *d = NULL;
  int f;

  f = hash (key, h->size);
  if (h->buckets[f])
    {
      for (i = h->buckets[f]; i; i = i->next)
        {
          if (!strcmp (i->key, key))
            {
              if (p)
                p->next = i->next;
              else
                h->buckets[f] = i->next;

              d = i->value;
              free (i->key);
              free (i);
              h->cnt--;
              break;
            }

          p = i;
        }
    }
  return d;
}

/* Deallocates an entire hash table */
void
ht_free (struct hash_tbl *h, clear_all_dtor dtor)
{
  struct hash_el *e;
  int i;

  for (i = 0; i < h->size; i++)
    if (h->buckets[i])
      {
        e = h->buckets[i];
        while (e)
          {
            h->buckets[i] = e->next;
            if (dtor)
              dtor (e->key, e->value);
            free (e->key);
            free (e);
            e = h->buckets[i];
          }
      }
  free (h->buckets);
  free (h);
}

/* Perform the function f() for each key-value pair in the hashtable h
 */
void
ht_foreach (struct hash_tbl *h,
            int (*f) (const char *key, void *value, void *data), void *data)
{
  struct hash_el *e;
  int i;

  for (i = 0; i < h->size; i++)
    if (h->buckets[i])
      {
        e = h->buckets[i];
        while (e)
          {
            if (!f (e->key, e->value, data))
              return;
            e = e->next;
          }
      }
}

