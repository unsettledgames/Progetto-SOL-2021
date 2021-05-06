#if !defined(LIST_H_)
#define LIST_H_

#include <stdio.h>
#include <stdlib.h>
#include "nodes.h"

#define INVALID_INDEX   -1
#define NO_DATA         -2
#define EMPTY_LIST      -3

typedef struct list
{
    Node* head;
    Node* tail;
    int length;
}List;

void list_initialize(List* list);

void list_clean(List list);

void* list_remove_by_index(List* list, unsigned int index);

void* list_remove_by_key(List* list, const char* key);

void* list_get(List list, unsigned int index);

int list_insert(List* list, unsigned int index, void* data, const char* key);

void* list_pop(List* list);

int list_push(List* list, void* data, const char* key);

void print_list(List to_print, char* name);

int list_enqueue(List* list, void* data, const char* key);

void* list_dequeue(List* list);

#endif
