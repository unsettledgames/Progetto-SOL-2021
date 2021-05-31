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

/**
    \brief: Struttura usata per descrivere una lista generica. In realtà, ai fini del progetto, si è ritenuto 
            inutile differenziare diversi tipi di strutture basate su liste, pertanto List supporta le normali
            operazioni sulle liste, ma anche quelle tipiche dello Stack e della Coda.
    
    \param head: Testa della lista.
    \param tail: Coda della lista.
    \param length: Lunghezza della lista.
    \param printer: Attributo opzionale che punta a una funzione usata per stampare correttamente i nodi della lista.
*/
typedef struct list
{
    Node* head;
    Node* tail;
    int length;
    void (*printer) (Node* to_print);
}List;

/**
    \brief: Inizializza la lista passata come parametro.
    
    \param list: La lista da inizializzare.
    \param printer: La funzione usata per stampare i nodi della lista.
*/
void list_initialize(List* list, void (*printer)(Node*));

/**
    \brief: Dealloca correttamente la lista passata come parametro, applicando cleaner se è specificata prima
            di deallocare normalmente il nodo.
    
    \param list: La lista da deallocare.
    \param cleaner: La funzione usata per deallocare correttamente i nodi della lista.
*/
void list_clean(List list, void (*cleaner)(Node*));

/**
    \brief: Rimuove un elemento dalla lista, dato il suo indice.
    
    \param list: La lista da cui rimuovere l'elemento.
    \param index: L'indice dell'elemento da rimuovere dalla lista.
*/
void list_remove_by_index(List* list, unsigned int index);

/**
    \brief: Rimuove un elemento dalla lista, data la sua chiave.
    
    \param list: La lista da cui rimuovere l'elemento.
    \param key: La chiave dell'elemento da rimuovere dalla lista.
*/
void list_remove_by_key(List* list, const char* key);

/**
    \brief: Fornisce i dati di un nodo dato il suo indice.
    
    \param list: La lista da cui ottenere i dati del nodo.
    \param index: L'indice del nodo desiderato.

    \return: I dati contenuti nel nodo all'indice index, NULL in caso di errore (viene modificato errno).
*/
void* list_get(List list, unsigned int index);

/**
    \brief: Inserisce i dati di un nodo a un certo indice.
    
    \param list: La lista in cui inserire i dati.
    \param index: L'indice in cui inserire i nuovi dati.
    \param data: I dati da inserire.
    \param key: La chiave del nodo da inserire.

    \return: I dati contenuti nel nodo all'indice index, NULL in caso di errore (viene modificato errno).
*/
int list_insert(List* list, unsigned int index, void* data, const char* key);

/**
    \brief: Effettua la pop di un elemento dalla lista (restituisce i dati in testa alla lista)
    
    \param list: La lista da cui ottenere i dati.

    \return: 0 se l'operazione è avvenuta con successo, INDEX_OUT_OF_BOUNDS se l'indice era fuori dai limiti
            della lista.
*/
void* list_pop(List* list);

/**
    \brief: Effettua la push di un elemento nella lista (inserisce i dati in testa alla lista)
    
    \param list: La lista in cui inserire i dati.
    \param data: I dati da inserire nella lista.
    \param key: La chiave del nodo da inserire nella lista.

    \return: 0
*/
int list_push(List* list, void* data, const char* key);

/**
    \brief: Stampa la lista to_print, assegnandole il nome name per distinguerla in caso di stampe multiple.

    \param to_print: La lista da stampare.
    \param name: Il nome della lista da stampare.
*/
void print_list(List to_print, char* name);

/**
    \brief: Inserisce in coda i dati specificati come parametro.
    
    \param list: La lista in cui inserire i dati.
    \param data: I dati da inserire nella lista.
    \param key: La chiave del nodo da inserire nella lista.

    \return: 0
*/
int list_enqueue(List* list, void* data, const char* key);

/**
    \brief: Rimuove in testa dalla lista list.
    
    \param list: La lista da cui rimuoevere i dati.

    \return: I dati rimossi dalla lista, NULL se non è stato possibile terminare l'operazione (errno viene
            modificato).
*/
void* list_dequeue(List* list);

/**
    \brief: Verifica che la lista contenga un elemento con la chiave specificata come parametro.
    
    \param list: La lista in cui verificare la presenza di un elemento dotato della chiave key.
    \param key: La chiave che si intende cercare nella lista.

    \return: TRUE se list contiene l'elemento con chiave key, FALSE altrimenti
*/
int list_contains_key(List list, const char* key);

/**
    \brief: Verifica che la lista contenga un elemento i cui dati hanno tipo char* che coincidono con str.
    
    \param list: La lista in cui verificare la presenza di un elemento i cui dati coincidono con str
    \param str: I dati da cercare nella lista.

    \return: L'indice in cui si trovano i dati se esistono, NOT_FOUND altrimenti (errno viene modificato).
*/
int list_contains_string(List list, const char* str);

#endif
