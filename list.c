/*
 * See list.h for more info
 *
 * This is free and unencumbered software released into the public domain.
 * http://unlicense.org/
 */
 
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "list.h"

/*****************************************************************************/

link_list *list_create()
{
	link_list *list;
	list = malloc(sizeof *list);
	if(!list)
		return NULL;
	
	list->first = NULL;
	list->last = NULL;		
	
	return list;
}

/*****************************************************************************/

void list_destroy(link_list *list, list_dtor dtor)
{
	while(list->first) {
		list_element *next = list->first->next;
		if(dtor) dtor(list->first->data);
		free(list->first);
		list->first = next;
	}	
	free(list);
}

/*****************************************************************************/

list_element *list_append(link_list *list, void *data)
{
	list_element *el;
	if(!data) return NULL;
	
	el = malloc(sizeof *el);
	if(!el)
		return NULL;
		
	el->next = NULL;
	if(!list->first)
	{
		assert(list->last == NULL);
		list->first = el;
		el->prev = NULL;
	}
	else
	{
		assert(list->last->next == NULL);
		list->last->next = el;
		el->prev = list->last;
	}
	list->last = el;	
	
	el->data = data;
		
	return el;
}

/*****************************************************************************/

list_element *list_prepend(link_list *list, void *data)
{
	list_element *el;
	if(!data) return NULL;
	
	el = malloc(sizeof *el);
	el->prev = NULL;
	if(!list->last)
	{
		assert(list->first == NULL);
		list->last = el;
		el->next = NULL;
	}
	else
	{
		assert(list->first->prev == NULL);
		list->first->prev = el;
		el->next = list->first;
	}
	list->first = el;	
		
	el->data = data;
	
	return el;
}

/*****************************************************************************/

int list_iterate(link_list *list, list_iter iter)
{
	list_element *le;
	if(!list || !iter) return 1;	
	for(le = list->first; le; le = le->next)
		if(!iter(le->data))
			return 0;	
	return 1;
}

/*****************************************************************************/

int list_iterate_reverse(link_list *list, list_iter iter)
{
	list_element *le;
	if(!list || !iter) return 1;	
	for(le = list->last; le; le = le->prev)
		if(!iter(le->data))
			return 0;	
	return 1;
}

/*****************************************************************************/

list_element *list_find(link_list *list, void *data, list_comp comp)
{
	list_element *le;
	if(!list || !data || !comp) return NULL;
	for(le = list->first; le; le = le->next)
		if(comp(le->data, data))
			return le;
	return NULL;
}

/*****************************************************************************/

void *list_remove_element(link_list *list, list_element *el)
{
	void *d = el->data;
	
	assert(list);
	if(!el) return NULL;
	
	if(!el->prev)
	{
		assert(list->first == el);
		if(!el->next)
		{
			assert(list->last == el);
			list->first = NULL;
			list->last = NULL;
		}
		else
		{				
			list->first = el->next;
			el->next->prev = NULL;
		}
	}
	else
	{	
		if(!el->next)
		{
			assert(list->last == el);
			list->last = el->prev;
			el->prev->next = NULL;
		}
		else
		{			
			el->next->prev = el->prev;
			el->prev->next = el->next;
		}
	}	
	free(el);		
	return d;
}

/*****************************************************************************/

void *list_remove(link_list *list, void *data, list_comp comp)
{
	list_element *le;
	if(!list || !data || !comp) return NULL;
	for(le = list->first; le; le = le->next)
		if(comp(le->data, data))
			return list_remove_element(list, le);
	return NULL;
}

/*****************************************************************************/

int list_strcmp(void *p, void *q)
{
	char *s1 = p, *s2 = q;
	return !strcmp(s1, s2);
}

/*****************************************************************************/

int list_stricmp(void *p, void *q)
{
	char *s1 = p, *s2 = q;
	for(;s1[0] && s2[0] && tolower(s1[0]) == tolower(s2[0]); s1++, s2++);

	return tolower(s1[0]) == tolower(s2[0]);
}

/*****************************************************************************/

void *list_pop_front(link_list *list)
{
	assert(list);
	if(!list->first)
	{
		assert(!list->last);
		return NULL;
	}
	
	return list_remove_element(list, list->first);
}

/*****************************************************************************/

void *list_pop_back(link_list *list)
{
	assert(list);
	if(!list->last)
	{
		assert(!list->first);
		return NULL;
	}
	
	return list_remove_element(list, list->last);
}

/*****************************************************************************/

int list_isempty(link_list *list)
{
	assert(list);
	return !list->first;
}

/*****************************************************************************/

int list_count(link_list *list)
{
	list_element *le;
	int count = 0;
	assert(list);
	for(le = list->first; le; le = le->next)
		count++;
	return count;
}

/*****************************************************************************/
