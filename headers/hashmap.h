#if !defined(HASHMAP_H_)
#define HASHMAP_H_

#include <stdio.h>
#include <stdlib.h>
#include "list.h"
#include "utility.h"
#include "string.h"

/**
    \brief: Struttura usata per descrivere una hashmap.

    \param lists: Liste che compongono l'hashmap.
    \param size: Il numero di liste che compongono l'hashmap.
    \param curr_size: Il numero di elementi attualmente presenti nell'hashmap.
    \param printer: Puntatore alla funzione in grado di stampare i nodi dell'hashmap.
*/
typedef struct hashmap
{
    List* lists;
    int size;
    int curr_size;
    void (*printer) (Node* to_print);
}Hashmap;

/**
    \brief: Inizializza una hashmap di dimensione size, assegnandole printer come funzione di stampa.

    \param hm: Hashmap da inizializzare.
    \param size: Numero di liste della hashmap.
    \param printer: Funzione di stampa dei nodi della hashmap.
*/
void hashmap_initialize(Hashmap* hm, int size, void (*printer) (Node*));

/**
    \brief: Dealloca correttamente l'hashmap passata come parametro, usando la funzione cleaner sui nodi
            in modo da deallocare correttamente anche i dati contenuti al loro interno, prima di deallocare
            i nodi stessi.

    \param hm: Hashmap da deallocare.
    \param cleaner: Funzione usata per deallocare i dati del nodo.
*/
void hashmap_clean(Hashmap hm, void (*cleaner) (Node*));

/**
    \brief: Ritorna i dati relativi alla chiave key.

    \param hm: Hashmap in cui sono contenuti i dati da ritornare.
    \param key: Chiave del valore da ritornare.

    \return: I dati relativi alla chiave key se sono presenti all'interno di hm, NULL altrimenti (errno
            viene modificato).
*/
void* hashmap_get(Hashmap hm, const char* key);

/**
    \brief: Aggiunge all'hashmap hm i dati puntati da data.

    \param hm: Hashmap in cui inserire i dati.
    \param data: Puntatore ai dati da inserire.
    \param key: Chiave dei dati.
*/
void hashmap_put(Hashmap* hm, void* data, const char* key);

/**
    \brief: Funzione di hashing che ritorna un interno data una stringa e la sua dimensione.

    \param key: Chiave da convertire in indice.
    \param size: Lunghezza della chiave.

    \return: Valore intero compreso tra 0 (compreso) e size (non compreso) che rappresenta l'indice corrispondente
            alla chiave key.
*/
int hashmap_hash(const char* key, int size);

/**
    \brief Stampa l'hashmap hm.

    \param hm: L'hashmap da stampare.
    \param name: Il nome da assegnare ad hm in modo che più stampe consecutive siano distinguibili.
*/
void print_hashmap(Hashmap hm, char* name);

/**
    \brief: Rimuove dall'hashmap hm i dati corrispondenti alla chiave key.

    \param hm: Hashmap da cui rimuovere i dati.
    \param key: Chiave dei dati da rimuovere.

    \return: 0 se la procedura è avvenuta con successo, un codice di errore (corrispondente a errno) in caso 
        di problemi (e.g. l'elemento non è stato trovato).
*/
int hashmap_remove(Hashmap* hm, const char* key);

/**
    \brief: Verifica che l'hashmap contenga la chiave key

    \param hm: Hashmap in cui controllare la presenza di key.
    \param key: La chiave di cui si desidera verificare la presenza all'interno di hm.

    \return: TRUE se la chiave è presente, FALSE altrimenti.
*/
int hashmap_has_key(Hashmap hm, const char* key);

//List hashmap_get_values(Hashmap hm);

#endif