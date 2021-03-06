#include "list.h"

/*
int main(int argc, char** argv)
{
    List list;
    int data[8];
    list_initialize(&list);

    for (int i=0; i<8; i++)
    {
        data[i] = i;
        list_push(&list, (void*) &(data[i]), NULL);
    }

    list_insert(&list, 5, (void*) &(data[4]), NULL);
    list_remove_by_index(&list, 8);
    list_pop(&list);
    list_pop(&list);

    list_insert(&list, 6, (void*) &(data[0]), NULL);
    list_insert(&list, 7, (void*) &(data[1]), NULL);

    print_list(list, "sas");

    list_remove_by_data(&list, (void*) &data[1]);

    print_list(list, "sas");

    list_clean(list);

    return 0;
}
*/

void list_clean(List list, void (*cleaner)(Node*))
{
    Node* curr = list.head;
    Node* prev = NULL;

    while (curr != NULL)
    {
        prev = curr;
        curr = curr->next;

        if (cleaner != NULL)
            cleaner(prev);
        clean_node(prev, TRUE);
    }
}

void list_initialize(List* list, void (*printer)(Node*))
{
    list->head = NULL;
    list->tail = NULL;
    list->length = 0;
    list->printer = printer;
}

int list_enqueue(List* list, void* data, const char* key)
{
    // Creo il nodo
    Node* to_add = create_node(data, key);

    // Se non ho la coda, non ho mai inserito niente, quindi la lista è vuota
    if (list->tail == NULL)
    {
        list->head = to_add;
        list->tail = list->head;
    }
    else
    {
        // Lo metto in fondo e aggiorno la coda
        list->tail->next = to_add;
        list->tail = list->tail->next; 
    }

    // La lunghezza della lista è aumentata
    list->length++;

    return OK;
}

void* list_dequeue(List* list)
{
    // Equivale a una pop
    return list_pop(list);
}

void list_remove_by_index(List* list, unsigned int index)
{
    errno = 0;

    // Indice e nodo di scorrimento
    int i = 0;
    Node* curr = list->head;
    Node* prev = NULL;

    // Ritorno se tento di cancellare un elemento oltre i limiti
    if (index >= list->length)
    {
        errno = INDEX_OUT_OF_BOUNDS;
        return;
    }
    // Se l'indice è 0, riciclo la pop
    else if (index == 0)
        list_pop(list);

    // Altrimenti scorro per trovare l'indice giusto del nodo che voglio cancellare
    while (i < index && i < list->length)
    {
        prev = curr;
        curr = curr->next;

        i++;
    }

    // Se ho rimosso l'ultimo elemento, aggiorno la coda
    if (index == (list->length -1))
        list->tail = prev;

    // Aggiusto i puntatori
    prev->next = curr->next;
    // Pulisco il nodo
    clean_node(curr, TRUE);
    // Decremento la lunghezza
    list->length--;
}

void list_remove_by_key(List* list, const char* key)
{
    errno = 0;
    // Nodi di scorrimento
    Node* curr = list->head;
    Node* prev = NULL;

    // Scorro per trovare il nodo che voglio cancellare
    while (curr != NULL && curr->key != NULL && strcmp(curr->key, key) != 0)
    {
        prev = curr;
        curr = curr->next;
    }

    // Se il nodo corrente è null, non ho trovato la chiave
    if (curr == NULL)
    {
        errno = KEY_NOT_FOUND;
        return;
    }

    // Se ho rimosso l'ultimo elemento, aggiorno la coda
    if (curr != NULL && curr == (list->tail))
        list->tail = prev;

    // Aggiusto i puntatori
    if (prev == NULL)
        list->head = list->head->next;
    else
        prev->next = curr->next;
    // Pulisco il nodo
    clean_node(curr, TRUE);
    // Decremento la lunghezza
    list->length--;
}

int list_insert(List* list, unsigned int index, void* data, const char* key)
{
    errno = 0;
    // Indice della lista
    int i = 0;
    // Puntatore al nodo precedente
    Node* prev = NULL;
    // Puntatore alla testa
    Node* current = list->head;
    // Nodo da inserire
    Node* to_insert = create_node(data, key);
    
    // Se tento di inserire al di fuori della dimensione della lista, lo segnalo
    if (index > list->length)
    {
        errno = INDEX_OUT_OF_BOUNDS;
        return INDEX_OUT_OF_BOUNDS;
    }

    // Altrimenti posso inserire
    // Caso limite, inserisco in testa e riciclo la push
    if (index == 0)
    {
        clean_node(to_insert, TRUE);
        return list_push(list, data, key);
    }
    // Caso limite, inserisco in coda e riciclo l'enqueue
    else if (index == list->length)
    {
        clean_node(to_insert, TRUE);
        return list_enqueue(list, data, key);
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
    
    return OK;
}

void* list_get(List list, unsigned int index)
{
    errno = 0;
    // Indice della lista
    int i = 0;
    // Puntatore alla testa
    Node* current = list.head;

    // Ritorno se l'indice è troppo grande
    if (index >= list.length)
    {
        errno = INDEX_OUT_OF_BOUNDS;
        return NULL;
    }

    // Scorro finché non raggiungo l'indice corretto o non finisco la lista
    while (i < index) 
    {
        current = current->next;
        i++;
    }

    // Se il nodo è valido, ne restituisco i dati
    if (current != NULL)
        return current->data;

    // Altrimenti ritorno NULL
    errno = DATA_NOT_FOUND;
    return NULL;
}


void* list_pop(List* list)
{
    errno = OK;
    // Puntatore alla testa
    Node* curr = list->head;
    // Puntatore ai dati del nodo, così posso deallocarlo senza perderli
    void* data;

    // Se la lista ha una testa (cioè se contiene elementi)
    if (list->head != NULL)
    {
        // Salvo i dati
        data = curr->data;
        // Imposta la nuova testa
        list->head = curr->next;
        // Riduci la lunghezza della lista
        list->length--;

        if (list->length == 0)
            list->tail = NULL;
        
        clean_node(curr, FALSE);

        return data;
    }

    errno = DATA_NOT_FOUND;
    return NULL;
}

int list_push(List* list, void* data, const char* key)
{
    // Altrimenti creo un nuovo nodo con i dati
    Node* to_add = create_node(data, key);
    // E lo imposto come nuova testa della lista
    to_add->next = list->head;
    list->head = to_add;

    // Incremento il numero di elementi nella lista
    list->length++;
    // Se ho aggiunto il primo elemento, aggiorno la coda
    if (list->length == 1)
        list->tail = list->head;

    return OK;
}

int list_contains_key(List list, const char* key)
{
    Node* curr = list.head;

    while (curr != NULL)
    {
        if (curr->key != NULL && strcmp(curr->key, key) == 0)
            return TRUE;

        curr = curr->next;
    }

    return FALSE;
}

int list_contains_string(List list, const char* str)
{
    errno = 0;
    Node* curr = list.head;
    int i = 0;

    while (curr != NULL && str != NULL)
    {
        if (curr->data != NULL && strcmp((char*)(curr->data), str) == 0)
        {
            return i;
        }

        i++;
        curr = curr->next;
    }

    errno = NOT_FOUND;
    return NOT_FOUND;
}

void print_list(List to_print, char* name)
{
    if (to_print.head != NULL)
        printf("Printing list %s with size %d\n", name, to_print.length);
    else
    {
        printf("List %s is empty (size = %d)\n", name, to_print.length);
        return;
    }   

    Node* curr = to_print.head;

    while (curr != NULL)
    {
        to_print.printer(curr);
        curr = curr->next;
    }
}