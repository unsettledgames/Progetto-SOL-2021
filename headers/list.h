#if !defined(LIST_H_)
#define LIST_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "nodes.h"
#include "utility.h"

#define INVALID_INDEX   -1
#define NO_DATA         -2
#define EMPTY_LIST      -3
#define NOT_FOUND       -4

typedef struct list
{
    Node* head;
    Node* tail;
    int length;
    void (*printer) (Node* to_print);
}List;

void list_initialize(List* list, void (*printer)(Node*));

void list_clean(List list, void (*cleaner)(Node*));

void list_remove_by_index(List* list, unsigned int index);

void list_remove_by_key(List* list, const char* key);

void* list_get(List list, unsigned int index);

int list_insert(List* list, unsigned int index, void* data, const char* key);

void* list_pop(List* list);

int list_push(List* list, void* data, const char* key);

void print_list(List to_print, char* name);

int list_enqueue(List* list, void* data, const char* key);

void* list_dequeue(List* list);

int list_contains_key(List list, const char* key);

int list_contains_string(List list, const char* str);

#endif
