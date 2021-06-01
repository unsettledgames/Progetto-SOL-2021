#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <sys/select.h>
#include <pthread.h>
#include <signal.h>

#include "zlib/zlib.h"
#include "nodes.h"
#include "list.h"
#include "hashmap.h"
#include "utility.h"
#include "errors.h"
#include "errno.h"
#include "api.h"

#define MAX_CONNECTION_QUEUE_SIZE   512

/**
    \brief: Struttura usata per rappresentare un file all'interno del server

    \param content: Contenuto del file in modalità compressa.
    \param path: Path assoluto del file.
    \param lock: Lock del singolo file usata per evitare che l'elaborazione concorrente di due richieste 
                diverse sullo stesso file possa causare problemi.
    \param client_descriptor: Descrittore del client che ha caricato il file sul server.
    \param last_op: Codice dell'ultima operazione eseguita sul file (utile per verificare che l'ultima operazione
                prima di una WRITEFILE fosse una OPENFILE).
    \param is_open: Indica se il file è aperto o meno.
    \param content_size: Dimensione del contenuto del file (in formato compresso).
    \param modified: Indica se il file è stato modifcato (cioè se è stato oggetto di WRITEFILE o APPENDFILE).
    \param last_used: Timestamp che indica l'ultimo momento in cui il file è stato utilizzato (utile per l'LRU).
*/
typedef struct file 
{
    char content[MAX_FILE_SIZE];
    char path[MAX_PATH_LENGTH];
    
    pthread_mutex_t lock;

    int client_descriptor;
    int last_op;
    int is_open;
    int content_size;
    int modified;

    time_t last_used;
}File;

/**
    \brief: Struttura usata per rappresentare la configurazione del server.

    \param n_workers: Numero di worker threads.
    \param tot_space: Massimo spazio occupabile dai file nel server (in modalità compressa).
    \param max_files: Numero massimo di file presenti allo stesso momento nel server.
    \param socket_name: Nome del socket usato dai client per connettersi.
    \param log_path: Percorso della cartella usata per salvare i file di log.
*/
typedef struct serverconfig
{
    unsigned int n_workers;
    unsigned int tot_space;
    unsigned int max_files;

    char socket_name[PATH_MAX];
    char log_path[MAX_PATH_LENGTH];
}ServerConfig;


/**
    \brief: Configura il server nelle modalità specificate dal file denominato file_name. Il file di
            configurazione del server è semplicemente formato da 4 linee dotate del seguente significato:

            linea_1 -> numero di worker threads
            linea_2 -> spazio totale disponibile nel server (in bytes)
            linea_3 -> numero totale di file ospitabili dal server
            linea_4 -> nome del socket
            linea_5 -> nome della cartella contenente i file di log
    
    \param file_name, il nome del file contenente i dati di configurazione del server.

    \return: Una struttura di tipo ServerConfig contenente i parametri di configurazione del server. Nel
            caso in cui non sia stato possibile impostare correttamente i campi, errno viene impostato
            a CONFIG_FILE_ERROR (-300).

*/
ServerConfig config_server(const char* file_name);

/**
    \brief: Il connession handler si occupa di accettare le richieste di connessione da parte dei client.
            Questo thread resta perennamente in ascolto di nuove connessioni, le accetta e modifica il read set
            in maniera coerente.

            Dal momento che la accept è bloccante e, per gli scopi di questo progetto, si è preferito utilizzare
            un socket blocante, affinché il connession handler termini è necessario spedire una falsa richiesta
            di connessione dopo aver impostato la variabile must_stop a true.

*/
void* connession_handler(void* args);

/**
    \brief: Thread dispatcher. Tramite l'uso della funzione select invocata sul set di lettura, legge le 
            richieste dei client e le aggiunge alla coda delle richieste da elaborare, segnalando la presenza
            di un nuovo lavoro da svolgere ai worker tramite l'apposita variabile condizionale.
            
            Al fine di garantire un'uscita pulita dal thread, è stato aggiunto un timeout alla select, in modo
            che non sia bloccante e permetta al dispatcher di verificare se il server sta terminando.

*/
void* dispatcher(void* args);

/**
    \brief: Thread worker. Resta in attesa sulla variabile condizionale usata per comunicare se ci sono 
            richieste da elaborare, per poi svegliarsi. A questo punto si ottengono le informazioni della
            richiesta e in base al suo tipo vengono svolte azioni differenti. In particolare, le operazioni
            supportate sono quelle corrispondenti alle possibili richieste delle api (api.c).
            
            Al risveglio del worker, si controlla anche che il programma non stia terminando: in tal caso,
            si esce dal thread invocando la pthread_exit.
*/
void* worker(void* args);

/**
    \brief: Crea e inizializza il socket del server.
    \return:Il descrittore del socket creato.
*/
int initialize_socket();

/**
    \brief: Crea il file di log. Il nome del file di log è il timestamp di esecuzione del server, in modo che 
            sia facilmente comprensibile capire quale log corrisponde a quale sessione.
            La funzione crea automaticamente una cartella contenente i file se già non esiste.

    \return:0 in caso di successo, COULDNT_CREATE_LOG(-305) in caso non sia stato possibile creare il file.
*/
int create_log();

// Funzione di debug, stampa le informazioni di un nodo richiesta (un nodo che è parte della coda delle richieste)
void print_request_node(Node* to_print);

// Funzione di debug, stampa le informazioni di un nodo file (un nodo che è parte della hashmap dei file
void print_file_node(Node* to_print);

/**
    \brief: get_LRU fornisce il nome del file che è stato utilizzato meno recentemente (cioè che è stato
            oggetto di una richiesta di qualsiasi tipo).
    
    \param current_path: il path del file che ha causato l'avvio del processo di sostituzione e che quindi
            non deve essere ritornato.

    \return:il nome del file che è stato utilizzato meno recentemente. Se è NULL, allora non è stato possibile
            trovare un file da rimpiazzare.
*/
char* get_LRU(char* current_path);

/**
    \brief: Gestore dei segnali. Dopo essersi avviato, aspetta la ricezione di un segnale tra SIGINT, SIGQUIT
            e SIGHUP. In caso di SIGINT e SIGQUIT il thread chiude ogni connessione, elimina ogni richiesta residua,
            e aspetta la terminazione degli altri thread. 

            In caso di SIGHUP, invece, si aspetta che tutti i client attualmente connessi si disconnettano prima
            di far terminare gli altri thread.

            In ogni caso, la memoria delle strutture dati allocate dinamicamente viene liberata, ogni file aperto
            viene chiuso e viene eseguito lo script delle statistiche.
*/
void* sighandler(void* param);

/**
    \brief: Funzione che viene utilizzata per pulire la memoria in caso di fallimento.
*/
void clean_everything();

/**
    \brief: log_info si occupa di scrivere una stringa nel file di log. E' sostanzialmente un wrapper per
            la fprintf, a cui aggiunge il controllo degli errori e un timestamp di scrittura.

    \param fmt: stringa di formato (come quelle usate dalla printf o dalla scanf)
    \param ...: variabili da sostituire nella stringa di formato
*/
void log_info(const char* fmt, ...);

/**
    \brief: Comprime l'array data di dimensione size e lo salva in buffer.

    \param data: I dati da comprimere.
    \param buffer: Il buffer in cui salvare i dati compressi.
    \param size: La dimensione dell'array dei dati da comprimere.

    \return: La dimensione dei dati compressi.
*/
int server_compress(char* data, char* buffer, int size);

/**
    \brief: Decomprime l'array data di dimensione size e lo salva in buffer.

    \param data: I dati da decomprimere.
    \param buffer: Il buffer in cui salvare i dati decompressi.
    \param data_size: La dimensione dell'array dei dati da decomprimere.

    \return: La dimensione dei dati decompressi.
*/
int server_decompress(char* data, char* buffer, unsigned int data_size);

/**
    \brief: Funzione di sicurezza utilizzata per chiudere immediatamente una connessione da parte di un client
            che invia richieste di tipo non supportato dal server. Ciò serve a evitare che il client causi
            un'interruzione dell'esecuzione del server.

    \param fd, il file descriptor del client che deve essere disconnesso.

*/
void sec_close_connection(int fd);

void check_apply_LRU(int n_files, int file_size, ClientRequest request);