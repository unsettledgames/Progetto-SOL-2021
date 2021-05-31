#if !defined(CLIENT_H_)

#define CLIENT_H_

#define OPT_NAME_LENGTH     5
#define OPT_VALUE_LENGTH    100

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "nodes.h"
#include "list.h"
#include "hashmap.h"
#include "utility.h"
#include "api.h"
#include "errors.h"
#include "consts.h"

/**
    \brief: Struttura dati usata per rappresentare una richiesta passata da linea di comando.
    
    \param code: Codice identificativo dell'operazione (ad esempio 'r', o 'w').
    \param arguments: Array contenente gli argomenti dell'operazione, ognuno di essi separato da virgola.
*/
typedef struct request
{
    char code;
    char* arguments;
}ArgLineRequest;

/**
    Struttura dati usata per rappresentare la configurazione di un client.
    \param socket_name: Nome del socket a cui connettersi.
    \param expelled_dir: Nome della directory in cui salvare i file espulsi dal server in caso di capacity miss.
    \param read_dir: Nome della directory in cui salvare i file letti dal server-.
    \param request_rate: Intervallo di tempo da far passare tra una richiesta e l'altra.
    \param print_op_data: Indica se stampare i dati delle operazioni eseguite o meno.
*/
typedef struct clientconfig
{
    char* socket_name;
    char* expelled_dir;
    char* read_dir;
    unsigned int request_rate;
    unsigned int print_op_data;
}ClientConfig;

/**
    \brief: parse_options si occupa di elaborare i dati passati da linea di comando. La funzione distingue 
            tra argomenti che si riferiscono a parametri di configurazione (salvandoli in config) e richieste
            da effettuare al server (che vengono salvate in requests). Viene verificato che ogni opzione
            abbia il relativo parametro (se necessario), mentre la verifica della coerenza dell'input viene 
            delegata a validate_input.
    
    \param config: Hashmap usata per salvare i parametri di configurazione del client.
    \param requests: Coda usata per salvare le richieste da inoltrare successivamente al server.
    \param args: Array degli argomenti da linea di comando.

    \return: L'esito della validazione degli argomenti (validate_input) se il parsing è avvenuto con successo,
            CLIENT_ARGS_ERROR in caso contrario.
*/
int parse_options(Hashmap* config, List* requests, int n_args, char** args);

/**
    \brief: Valida la configurazione del server e la coda delle richieste. Controlla, ad esempio, che -D sia usata
            insieme a una richiesta di tipo -w o -W. Verifica inoltre che i numeri passati come argomenti di 
            certe funzioni siano validi.
    
    \param config: Hashmap contenente la configurazione del client.
    \param requests: Coda delle richieste.

    \return: 0 se la validazione è avvenuta con successo, INCONSISTENT_INPUT_ERROR se l'input non è consistente
            (ad esempio se -D viene usata senza -w o -W), NAN_INPUT_ERROR se ci si aspettava che un parametro
            fosse un numero quando effettivamente non lo è, INVALID_NUMBER_INPUT_ERROR se il suddetto numero
            non è valido (ad esempio se è negativo quando dovrebbe esprimere una quantità).
*/
int validate_input(Hashmap config, List requests);

/**
    \brief: Inizializza il client secondo i parametri specificati in config.
    \param: Hashmap in cui le chiavi sono nomi di parametri e i valori sono il loro valore.
    \return: Struttura ClientConfig che rappresenta la configurazione del client. Se non è stato possibile 
            ottenerer uno o più parametri di configurazione, errno viene modificato.
*/
ClientConfig initialize_client(Hashmap config);

/**
    \brief: execute_requests esegue una alla volta tutte le richieste contenute in requests, utilizzando
            i parametri di configurazione contenuti in config. La funzione fa del suo meglio per soddisfare tutte
            le richieste, quindi se si verifica un errore, semplicemente si prosegue nella prossima iterazione
            e si cerca di portare a termine più richieste possibili.

    \param requests: coda delle richieste, precedentemente calcolata in base ai parametri da linea di comando.
*/
void execute_requests(ClientConfig config, List* requests);

/**
    \brief: Effettua il parsing degli argomenti da linea di comando salvati nelle richieste (già elaborati da
            parse_options). Nella maggior parte dei casi si tratta di tokenizzare una stringa che rappresenta la
            lista dei parametri di una richiesta. In aggiunta si effettuano anche dei controlli sugli errori.

    \param args: Vettore contenente gli argomenti della richiesta, ognuno separato da virgola.
*/
char** parse_request_arguments(char* args);

/**
    \brief: Implementazione di -h, stampa a schermo tutte le possibili opzioni del client.
*/
void print_client_options();

// Funzione di debug, stampa la configurazione del client
void print_client_config(ClientConfig c);

// Funzione di debug, stampa le informazioni di un nodo richiesta
void print_node_request(Node* node);

// Funzione di debug, stampa le informazioni di un nodo
void print_node_string(Node* to_print);

// Funzione di pulizia, si occupa di deallocare correttamente un nodo che rappresenta una richiesta da linea di comando.
void clean_request_node(Node* node);

/**
    \brief: Ripulisce la memoria del client, deallocando config e requests.
    \param config: L'hashmap che rappresenta la configurazione del client.
    \param requests: La coda delle richieste del client.
*/
void clean_client(Hashmap config, List requests);

/**
    \brief: Spedisce ricorsivamente tutti i file contenuti in dirpath, per un massimo di n_files se n_files > 0
            alla prima chiamata della funzione. Sostanzialmente si visita l'intero albero di directory la cui 
            radice è il dirpath su cui viene chiamata la funzione per la prima volta: la procedura viene richiamata
            se dirpath è il nome di una cartella, altrimenti viene spedito un file.

    \param dirpath: La cartella della quale si è interessati a scrivere i contenuti.
    \param n_files: Il numero massimo di file da scrivere sul server.
    \param write_dir: Cartella di scrittura degli eventuali file espulsi dal server in seguito a invocazioni 
            dell'algoritmo di rimpiazzamento.

    \return: 0 se la procedura è avvenuta con successo, FILESYSTEM_ERROR in caso non sia stato possibile aprire
            file o cartelle.
*/
int send_from_dir(const char* dirpath, int* n_files, const char* write_dir);

#endif