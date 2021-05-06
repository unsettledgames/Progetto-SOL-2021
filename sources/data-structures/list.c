#include "node.h"
#include "list.h"

int main(int argc, char** argv)
{
    List list;
    int data[8];
    list_initialize(&list);

    for (int i=0; i<8; i++)
    {
        data[i] = i;
        list_push(&list, (void*) &(data[i]));
    }

    list_insert(&list, 5, (void*) &(data[4]));
    list_remove_by_index(&list, 8);
    list_pop(&list);
    list_pop(&list);

    print_list(list, "sas");

    list_clean(list);

    return 0;
}

void list_clean(List list)
{
    Node* curr = list.head;
    Node* prev = NULL;

    while (curr->next != NULL)
    {
        prev = curr;
        curr = curr->next;

        free(prev);
    }

    free(curr);
}

void list_initialize(List* list)
{
    list->head = create_node(NULL);
    list->length = 0;
}

void* list_remove_by_index(List* list, unsigned int index)
{
    // Indice e nodo di scorrimento
    int i = 0;
    Node* curr = list->head;
    Node* prev = NULL;

    // Ritorno null se tento di cancellare un elemento oltre i limiti
    if (index > list->length)
        return NULL;
    // Se l'indice è 0, riciclo la pop
    else if (index == 0)
        return list_pop(list);

    // Altrimenti scorro per trovare l'indice giusto del nodo che voglio cancellare
    while (i < index && i < list->length)
    {
        prev = curr;
        curr = curr->next;

        i++;
    }

    // Aggiusto i puntatori
    prev->next = curr->next;
    // Pulisco il nodo
    clean_node(curr);

    return 0;
}

int list_insert(List* list, unsigned int index, void* data)
{
    printf("pointer value: %p\n", list);
    // Indice della lista
    int i = 0;
    // Puntatore al nodo precedente
    Node* prev = NULL;
    // Puntatore alla testa
    Node* current = list->head;
    // Nodo da inserire
    Node* to_insert = create_node(data);
    
    // Se tento di inserire al di fuori della dimensione della lista, lo segnalo
    if (index > list->length)
        return INVALID_INDEX;

    // Altrimenti posso inserire
    // Caso limite, inserisco in testa e riciclo la push
    if (index == 0)
    {
        clean_node(to_insert);
        return list_push(list, data);
    }
    // Inserimento normale
    while (i < index)
    {
        prev = current;
        current = current->next;
        i++;
    }

    prev->next = to_insert;
    to_insert->next = current;

    // Incremento il numero di elementi nella lista
    list->length++;

    return 0;
}

void* list_get(List list, unsigned int index)
{
    // Indice della lista
    int i = 0;
    // Puntatore alla testa
    Node* current = list.head;

    // Scorro finché non raggiungo l'indice corretto o non finisco la lista
    while (i < list.length && i < index) 
    {
        current = current->next;
        i++;
    }

    // Se il nodo è valido, ne restituisco i dati
    if (current != NULL)
        return current->data;

    // Altrimenti ritorno NULL
    return NULL;
}


void* list_pop(List* list)
{
    // Puntatore alla testa
    Node* curr = list->head;
    // Puntatore ai dati del nodo, così posso deallocarlo senza perderli
    void* data;

    // Se la lista ha una testa (cioè se contiene elementi)
    if (list->head != NULL)
    {
        // Imposta la nuova testa
        list->head = curr->next;
        // Riduci la lunghezza della lista
        list->length--;
        // Pulisci il vecchio nodo
        data = curr->data;
        clean_node(curr);

        return data;
    }

    return NULL;
}

int list_push(List* list, void* data)
{
    // Ritorno null se i dati non esistono
    if (data == NULL)
        return NO_DATA;

    // Altrimenti creo un nuovo nodo con i dati
    Node* to_add = create_node(data);
    // E lo imposto come nuova testa della lista
    to_add->next = list->head;
    list->head = to_add;

    // Incremento il numero di elementi nella lista
    list->length++;

    return 0;
}

void print_list(List to_print, const char* name)
{
    printf("Printing list %s\n", name);
    Node* curr = to_print.head;

    while (curr->next != NULL)
    {
        printf("%d\n", *(int*)curr->data);
        curr = curr->next;
    }
}