#include <stdio.h>
#include <stdlib.h>

#define INVALID_INDEX   -1
#define NO_DATA         -2
#define EMPTY_LIST      -3

typedef struct list
{
    Node* head;
    int length;
}List;

void list_initialize(List* list);

int list_remove(List* list, int index);

void* list_get(List list, int index);

int list_insert(List* list, int index, void* data);

void* list_pop(List* list);

int list_push(List* list, void* data);

void print_list(List to_print, const char* name);

/** TODO:

int enqueue(List* list, void* data);

void* dequeue(List* list);

*/