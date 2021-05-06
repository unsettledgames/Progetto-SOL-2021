#include <stdio.h>
#include <stdlib.h>
#include "hashmap.h"

int main(int argc, char** argv)
{
    Hashmap map;
    int data[10];

    for (int i=0; i<10; i++)
        data[i] = i;

    hashmap_initialize(&map, 1021);

    hashmap_put(&map, (void*) &data[0], "Sì questa è decisamente una chiave");
    hashmap_put(&map, (void*) &data[1], "Sì questa è una chiave");
    hashmap_put(&map, (void*) &data[2], "Sì questa è proprio una chiave");
    hashmap_put(&map, (void*) &data[3], "Questa è decisamente una chiave");
    hashmap_put(&map, (void*) &data[4], "Anche questa è decisamente una chiave");
    hashmap_put(&map, (void*) &data[5], "Notevole chiave");

    print_hashmap(map, "Base");

    hashmap_remove(&map, "Notevole chiave");

    print_hashmap(map, "Dopo rimozione");
    
    return 0;
}

void hashmap_initialize(Hashmap* hm, int size)
{
    hm->lists = malloc(sizeof(List) * size);
    hm->size = size;
    
    for (int i=0; i<size; i++)
        list_initialize(&(hm->lists[i]));
}

void hashmap_clean(Hashmap* hm)
{
    for (int i=0; i<hm->size; i++)
        list_clean(hm->lists[i]);
}

int hashmap_hash(const char* key, int size)
{
    unsigned long hash = 5381;
    int c;

    while (c = *key++)
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash % size;
}


void* hashmap_get(Hashmap hm, const char* key)
{
    // Nodo per lo scorrimento della lista
    Node* curr = NULL;
    // Indice della lista a partire dalla chiave
    int index = hashmap_hash(key, hm.size);
    // Salvo la lista indicizzata
    List container = hm.lists[index];

    curr = container.head;

    // Scorro finché non finisco la lista o finché non ho trovato la chiave corretta
    while (strcmp(key, curr->key) != 0 && curr != NULL)
    {
        curr = curr->next;
    }

    return curr->data;
}

int hashmap_put(Hashmap* hm, void* data, const char* key)
{
    // Indice della lista in cui inserire
    int index = hashmap_hash(key, hm->size);
    // Inserisco nella lista
    list_push(&(hm->lists[index]), data, key);

    return 0;
}

int hashmap_remove(Hashmap* hm, const char* key)
{
    // Indice della lista da cui rimuovere
    int index = hashmap_hash(key, hm->size);
    // Rimuovo dalla lista
    list_remove_by_key(&(hm->lists[index]), key);

    return 0;
}

void print_hashmap(Hashmap hm, char* name)
{
    char* list_name = malloc(sizeof(char) * 1024);
    printf("Printing hashmap %s\n", name);

    for (int i=0; i<hm.size; i++)
    {
        sprintf(list_name, "%d", i);
        print_list(hm.lists[i], list_name);
    }

    free(list_name);
}