#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "../list.h"
#include "../utils.h"

static int print_node(void *data)
{
	char *s = data;
	printf("- %s\n", s);
	return 1;
}

int main(int argc, char *argv[])
{
	link_list *list;
	char cmd[21], arg[41];

	printf("Type 'help' for help\n");

	list = list_create();

	for(;;)
	{
		printf(">>");
		fflush(stdout);

		if(scanf("%10s", cmd) != 1) continue;

		if(!strcmp(cmd, "quit")) break;
		else if(!strcmp(cmd, "help"))
		{
			printf("help - Shows this message\n");
			printf("quit - exits the test program\n");
			printf("add <item> - Add item (at the end)\n");
			printf("addf <item> - Add item to the front.\n");
			printf("show - shows all items.\n");
			printf("showr - shows items in revers order.\n");
			printf("find <item> - Searches for an item in the list.\n");
			printf("findi <item> - Case insensitive find.\n");
			printf("rem <item> - removes an item in the list.\n");
			printf("remi <item> - Case insensitive remove.\n");
			printf("popf - Pops the front of the list.\n");
			printf("popb - Pops the back of the list.\n");
			printf("empty - Determines whether the list is empty\n");
			printf("count - Counts the number of elements in the list\n");
		}
		else if(!strcmp(cmd, "add"))
		{
			if(scanf("%40s", arg) != 1) continue;
			list_append(list, my_strdup(arg));
		}
		else if(!strcmp(cmd, "addf"))
		{
			if(scanf("%40s", arg) != 1) continue;
			list_prepend(list, my_strdup(arg));
		}
		else if(!strcmp(cmd, "show"))
		{
			list_iterate(list, print_node);
		}
		else if(!strcmp(cmd, "showr"))
		{
			list_iterate_reverse(list, print_node);
		}
		else if(!strcmp(cmd, "find"))
		{
			if(scanf("%40s", arg) != 1) continue;
			if(list_find(list, arg, list_strcmp))
				printf("%s is in the list\n", arg);
			else
				printf("%s is not in the list\n", arg);
		}
		else if(!strcmp(cmd, "findi"))
		{
			if(scanf("%40s", arg) != 1) continue;
			if(list_find(list, arg, list_stricmp))
				printf("%s is in the list\n", arg);
			else
				printf("%s is not in the list\n", arg);
		}
		else if(!strcmp(cmd, "rem"))
		{
			char *c;
			if(scanf("%40s", arg) != 1) continue;
			c = list_remove(list, arg, list_strcmp);
			free(c);
		}
		else if(!strcmp(cmd, "remi"))
		{
			char *c;
			if(scanf("%40s", arg) != 1) continue;
			c = list_remove(list, arg, list_stricmp);
			free(c);
		}
		else if(!strcmp(cmd, "popf"))
		{
			char *c;
			c = list_pop_front(list);
			printf("Popped %s\n", c);
			free(c);
		}
		else if(!strcmp(cmd, "popb"))
		{
			char *c;
			c = list_pop_back(list);
			printf("Popped %s\n", c);
			free(c);
		}
		else if(!strcmp(cmd, "empty"))
		{
			printf("List is %s\n", list_isempty(list)?"empty":"not empty");
		}
		else if(!strcmp(cmd, "count"))
		{
			printf("List has %d items\n", list_count(list));
		}
		else
		{
			fprintf(stderr, "Unknown command %s\n", cmd);
			fflush(stdout);
		}
	}

	list_destroy(list, free);

	return 0;
}
