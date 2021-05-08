#if !defined(HASHMAP_H_)
#define HASHMAP_H_

#include <stdio.h>
#include <stdlib.h>
#include "list.h"
#include "string.h"

typedef struct hashmap
{
    List* lists;
    int size;
    void (*printer) (Node* to_print);
}Hashmap;

void hashmap_initialize(Hashmap* hm, int size, void (*printer) (Node*));

void hashmap_clean(Hashmap hm, void (*cleaner) (Node*));

void* hashmap_get(Hashmap hm, const char* key);

int hashmap_put(Hashmap* hm, void* data, const char* key);

int hashmap_hash(const char* key, int size);

void print_hashmap(Hashmap hm, char* name);

int hashmap_remove(Hashmap* hm, const char* key);

int hashmap_has_key(Hashmap hm, const char* key);

#endif