#include <stdio.h>
#include <stdlib.h>
#include "hashmap.h"

/*
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

    hashmap_put(&map, (void*) &data[5], "Notevole chiave");

    print_hashmap(map, "Riaggiunto vecchio elemento");

    printf("Valore ottenuto dalla chiave Anche questa è decisamente una chiave (expected 4): %d\n",
            *(int*)hashmap_get(map, "Anche questa è decisamente una chiave"));

    hashmap_clean(map);
    
    return 0;
}

*/

void hashmap_initialize(Hashmap* hm, int size, void (*printer) (Node* to_print))
{
    hm->lists = malloc(sizeof(List) * size);
    hm->size = size;
    hm->printer = printer;
    
    for (int i=0; i<size; i++)
        list_initialize(&(hm->lists[i]), printer);
}

void hashmap_clean(Hashmap hm, void (*cleaner)(Node*))
{
    for (int i=0; i<hm.size; i++)
    {
        list_clean(hm.lists[i], cleaner);
    }

    free(hm.lists);
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

    if (curr != NULL)
        return curr->data;
    return NULL;
}

int hashmap_put(Hashmap* hm, void* data, const char* key)
{
    // Indice della lista in cui inserire
    int index = hashmap_hash(key, hm->size);

    // Rimuovo l'entry precedente se ce n'era già una
    if (list_contains_key(hm->lists[index], key))
        list_remove_by_key(&(hm->lists[index]), key);
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

int hashmap_has_key(Hashmap hm, const char* key)
{
    for (int i=0; i<hm.size; i++)
    {
        if (hm.lists[i].length != 0)
        {
            if (list_contains_key(hm.lists[i], key))
                return TRUE;
        }
    }

    return FALSE;
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