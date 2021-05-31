#if !defined(NODES_H_)
#define NODES_H_

#include <stdio.h>
#include <stdlib.h>
#include "consts.h"
#include "utility.h"

/**
    \brief: Struttura usata per descrivere un nodo. Per le finalità del progetto, si è ritenuto opportuno
            fornire il nodo di una chiave, in modo da facilitare la ricerca del nodo corretto in caso
            di collisioni nella Hashmap.

    \param next: Nodo successivo.
    \param key: Chiave identificativa del nodo.
    \param data: Dati contenuti dal nodo.
*/
typedef struct node 
{
    struct node* next;

    const char* key;
    void* data;
}Node;

/**
    \brief: Crea un nodo contenente i dati to_set e con chiave key.

    \param to_set: I dati da assegnare al nodo.
    \param key: La chiave identificativa del nodo.

    \return: Il puntatore al nodo creato secondo i parametri passati.
*/
Node* create_node(void* to_set, const char* key);

/**
    \brief: Dealloca un nodo.

    \param to_clean: Il nodo da deallocare.
    \param clean_data: Indica se si devono deallocare anche i dati o no.
*/
void clean_node(Node* to_clean, int clean_data);

#endif