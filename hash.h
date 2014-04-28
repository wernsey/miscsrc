/*1 Hash.h
 *# A quick and dirty hash table implementation.\n
 *{
 ** {{struct hash_tbl}} is created with {{~~ht_create()}}
 ** {{struct hash_tbl}} is destroyed with {{~~ht_free()}}
 ** Resize hash tables with {{~~ht_rehash()}}
 ** Insert entries into the table with {{~~ht_insert()}}
 ** Search for entries with {{~~ht_find()}}
 ** Remove entries with {{~~ht_delete()}}
 ** Iterate through the table with {{~~ht_next()}}
 *}
 *2 License
 *[
 *# Author: Werner Stoop
 *# This software is provided under the terms of the unlicense.
 *# See http://unlicense.org/ for more details.
 *]
 *2 API
 */

#ifndef HASH_H
#define HASH_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C"
{
#endif

/*@ typedef void (*clear_all_dtor) (void *)
 *# A pointer to a function that can clean up values in the hash table.\n
 *# Clean up implies freeing memory allocated to the hash table or the
 *# freeing of other resources allocated to the elements.\n
 *# If, for example, the hash table stores {{FILE}} handles, this function
 *# can be used to close all open files referenced in the hash table.
 */
  typedef void (*clear_all_dtor) (const char *key, void *val);

/*@ struct hash_el
 *# Element stored within the hash table 
 */
  struct hash_el
  {
    char *key;
    void *value;
    struct hash_el *next;
  };

/*@ struct ##hash_tbl
 *# Structure for managing tha hash table.\n
 *# Allocate this structure through {{~~ht_create()}}
 *# and deallocate it with {{~~ht_free()}}
 */
  struct hash_tbl
  {
    struct hash_el **buckets;
    int size;
    int cnt;
  };

/*@ struct hash_tbl *##ht_create (int size)
 *# Allocates memory for a hash table.\n
 *# The hash table size must be a power of two, because
 *# the mod operation is performed by the bitwise AND of
 *# the sum and (size - 1).\n
 *# Setting size to 0 specifies a default value.
 */
  struct hash_tbl *ht_create (int size);

/*@ int ##ht_rehash (struct hash_tbl *ht, int new_size)
 *# Resizes the hashtable {{ht}} to the {{new_size}}, by rehashing 
 *# each key in the table.\n
 *# The new size must be a power of two.\n
 *# This function is normally called automatically in {{~~ht_insert()}}
 *# if the table reaches a certain size.
 */
  int ht_rehash (struct hash_tbl *ht, int new_size);

/*@ void *##ht_insert (struct hash_tbl *h, const char *key, void *value)
 *# Inserts a {{value}} referenced by the string {{key}} 
 *# into the hash table {{h}}.\n
 *# It returns {{value}} on success, or {{NULL}} if an internal {{malloc()}}
 *# failed.
 */
  void *ht_insert (struct hash_tbl *h, const char *key, void *value);

/*@ void *##ht_find (struct hash_tbl *h, const char *key)
 *# Finds an element referenced by {{key}} in the table {{h}}.\n
 *# It will return {{NULL}} if the key was not found in the table
 */
  void *ht_find (struct hash_tbl *h, const char *key);

/*@ const char *##ht_next (struct hash_tbl *h, const char *key)
 *# Given a specific {{key}}, this finds the key of the next element in 
 *# a particular hash table {{h}}. This can be used to iterate through 
 *# the table.\n
 *# Specifying a value of {{NULL}} as the key retrieves the first key.\n
 *# It returns {{NULL}} if there are no more entries in the table.\n
 *# Keys in the table are not sorted.
 */
  const char *ht_next (struct hash_tbl *h, const char *key);

/*@ void *##ht_delete (struct hash_tbl *h, const char *key)
 *# Deletes an element indexed by {{key}} from the hash table {{h}}\n
 *# It returns the value associated with the key.
 */
  void *ht_delete (struct hash_tbl *h, const char *key);

/*@ void ##ht_free (struct hash_tbl *h, clear_all_dtor dtor)
 *# Deletes a hash table {{h}}.\n
 *# The parameter {{dtor}} can point to a function that will free
 *# resources (memory, handles, etc) allocated to the values in the table.
 *# This function will be called for each value in the table.
 */
  void ht_free (struct hash_tbl *h, clear_all_dtor dtor);

/*@ void ##ht_foreach(struct hash_tbl *h, int (*f)(const char *key, void *value, void *data), void *data)
 *# Perform the function {{f()}} for each {{key-value}} pair in the hashtable {{h}}.\n
 *# The {{data}} parameter is passed to {{f()}} unmodified.\n
 *# If {{f()}} returns 0, the iteration is terminated.
 */
  void ht_foreach (struct hash_tbl *h,
                   int (*f) (const char *key, void *value, void *data),
                   void *data);

#if defined(__cplusplus) || defined(c_plusplus)
}                               /* extern "C" */
#endif

#endif                          /* HASH_H */
