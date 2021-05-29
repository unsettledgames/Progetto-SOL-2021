#include "server.h"

// Id del thread (usato nel log per le statistiche)
int tid = 0;

// Uscita dal programma
volatile sig_atomic_t must_stop = 0;

// File di log con la sua lock
FILE* log_file = NULL;
pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

// Coda delle richieste con la sua lock e variabile condizionale
List requests;
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t queue_not_empty = PTHREAD_COND_INITIALIZER;

// Dimensione dello spazio allocato con la sua lock
unsigned int allocated_space = 0;
pthread_mutex_t allocated_space_mutex = PTHREAD_MUTEX_INITIALIZER;

// Hashmap dei file con la sua lock
Hashmap files;
pthread_mutex_t files_mutex = PTHREAD_MUTEX_INITIALIZER;

// Thread ids
pthread_t* tids;

// Client connessi al momento e lock
List client_fds;
pthread_mutex_t client_fds_lock = PTHREAD_MUTEX_INITIALIZER;

// Descrittore del socket del server
int socket_desc = -1;

// Desc set e lock
fd_set desc_set;
pthread_mutex_t desc_set_lock = PTHREAD_MUTEX_INITIALIZER;
// Valore massimo tra i fd dei client, usato nella select (con la sua lock)
int max_fd = -1;
pthread_mutex_t max_fd_lock = PTHREAD_MUTEX_INITIALIZER;

// Handlers
pthread_t connession_handler_tid;
pthread_t dispatcher_tid;
pthread_t sighandler_tid;

// Maschera dei segnali
sigset_t mask;

int debug = 0;

// Configurazione del server
ServerConfig config;

int main(int argc, char** argv)
{
    int err;
    // Maschero i segnali
    sigset_t oldmask;
    SYSCALL_EXIT("sigemptyset", err, sigemptyset(&mask), "Errore in sigemptyset", "");
    SYSCALL_EXIT("sigaddset", err, sigaddset(&mask, SIGINT), "Errore in sigaddset", "");
    SYSCALL_EXIT("sigaddset", err, sigaddset(&mask, SIGQUIT), "Errore in sigaddset", "");
    SYSCALL_EXIT("sigaddset", err, sigaddset(&mask, SIGHUP), "Errore in sigaddset", "");

    // Applico la maschera appena definita
    SYSCALL_EXIT("pthread_sigmask", err, pthread_sigmask(SIG_SETMASK, &mask, &oldmask), 
        "Errore in pthread_sigmask", "");
    // Thread di gestione dei segnali
    SYSCALL_EXIT("pthread_create", err, pthread_create(&sighandler_tid, NULL, &sighandler, NULL), 
        "Errore in pthread_create", "");

    // Inizializzazione delle strutture dati necessarie
    list_initialize(&requests, print_request_node);
    list_initialize(&client_fds, NULL);
    hashmap_initialize(&files, 1021, NULL);

    // Azzero la maschera dei socket da leggere
    FD_ZERO(&desc_set);

    // Parsing della configurazione del server
    if (argc == 1)
    {
        clean_everything();
        fprintf(stderr, "File di configurazione non passato\n");
        return CONFIG_FILE_ERROR;
    }

    config = config_server(argv[1]);

    if (errno == 0)
    {
        // Tid del thread che gestisce le connessioni
        connession_handler_tid = -1;
        // Tid del thread che smista le richieste
        dispatcher_tid = -1;

        socket_desc = initialize_socket();
        create_log();

        log_info("Configurazione del server acquisita correttamente");
        log_info("[NTH] %d", config.n_workers);

        // Alloco spazio per i tids
        tids = my_malloc(sizeof(pthread_t) * config.n_workers);

        // Faccio partire il thread master, che accetta le connessioni
        SYSCALL_EXIT("pthread_create", err, pthread_create(&connession_handler_tid, NULL, &connession_handler, NULL), 
        "Errore in pthread_create dell'handler delle connessioni", "");
        // Faccio partire il dispatcher, che riceve le richieste e le aggiunge in coda
        SYSCALL_EXIT("pthread_create", err, pthread_create(&dispatcher_tid, NULL, &dispatcher, NULL), 
        "Errore in pthread_create del dispatcher", "");

        log_info("Inizializzato dispatcher delle richieste");

        // Faccio partire i thread workers
        for (int i=0; i<config.n_workers; i++)
        {
            SYSCALL_EXIT("pthread_create", err, pthread_create(&tids[i], NULL, &worker, NULL), 
            "Errore in pthread_create del worker %d", i);
        }

        // Aspetto che il signal handler finisca
        THREAD_JOIN(sighandler_tid, NULL);
    }
    else
    {
        perror("Impossibile terminare la configurazione del server");
        clean_everything();
        exit(CONFIG_FILE_ERROR);
    }

    return 0;
}

void* worker(void* args)
{
    int my_tid = tid++;

    // Salvataggio della richiesta
    ClientRequest request;
    // Timestamp
    time_t timestamp;

    while (!must_stop)
    {
        // Return value
        int to_send = 0;        
        // Numero di elementi da spedire al client
        int n_expelled = 0;
        // File espulsi dal server da spedire al client
        File* files_to_send;
        // Dimensione del file corrente
        int file_size;

        // Mi metto in attesa sulla variabile condizionale
        LOCK(&queue_mutex);
        while (!must_stop && requests.length <= 0)
            WAIT(&queue_not_empty, &queue_mutex);
        
        // Se sono arrivato fin qui, o è perché devo terminare
        if (must_stop)
        {
            UNLOCK(&queue_mutex);
            pthread_exit(NULL);
        }

        log_info("[RQ] %d", my_tid);
        // O è perché ho una richiesta da elaborare
        ClientRequest* to_free = (ClientRequest*)list_dequeue(&requests);
        // Copio la richiesta
        memcpy(&request, to_free, sizeof(request));
        // Loggo
        log_info("Ricevuta richiesta da elaborare\n\tClient descriptor: %d\n\tCodice operazione:%d\n", request.client_descriptor, request.op_code);
        // Libero la memoria allocata
        free(to_free);
        // Sblocco la coda
        UNLOCK(&queue_mutex);

        // Ottengo il timestamp
        time(&timestamp);
        // Eseguo la richiesta
        switch (request.op_code)
        {
            case OPENFILE:;
                log_info("Tentativo di apertura del file %s", request.path);
                // Verifico che il file non esista o non sia già aperto
                LOCK(&files_mutex);
                if (hashmap_has_key(files, request.path) && !((request.flags >> O_CREATE) & 1))
                {
                    // Ottengo il file che mi interessa
                    File* to_open = (File*)hashmap_get(files, request.path);
                    UNLOCK(&files_mutex);

                    // Lo apro
                    LOCK(&to_open->lock);
                    to_open->is_open = TRUE;
                    UNLOCK(&to_open->lock);

                    log_info("File aperto con successo.");
                    log_info("[OP] open");
                }
                else if ((request.flags >> O_CREATE) & 1)
                {
                    LOCK(&allocated_space_mutex);
                    while (files.curr_size > config.max_files)
                    {
                        log_info("Chiamato algoritmo di rimpiazzamento\n\t(Spazio: %d / %d)\n\t(N files: %d / %d)", 
                            allocated_space, config.tot_space, files.curr_size, config.max_files);
                        log_info("[LRU] called");
                        // Calcolo il path del file da rimuovere
                        char* to_remove = get_LRU(request.path);
                        
                        // Se la LRU ha ritornato qualcosa, allora rimuovo quel file
                        if (to_remove != NULL)
                        {
                            log_info("Rimuovo il file %s", to_remove);
                            // Salvo il file perché la remove dealloca i dati
                            // Aggiorno lo spazio e rimuovo il file
                            allocated_space -= ((File*)hashmap_get(files, to_remove))->content_size;                            
                            if (hashmap_remove(&files, to_remove) != OK)
                                perror("Impossibile rimuovere il file");

                            free(to_remove);
                        }
                        // Altrimenti ho fallito e non posso aggiungere il file
                        else
                            to_send = LRU_FAILURE;
                    }
                    UNLOCK(&allocated_space_mutex);

                    int err = to_send;
                    if (err < 0)
                        SERVER_OP(writen(request.client_descriptor, &to_send, sizeof(to_send)), break);
                    // Il file non esisteva, allora lo aggiungo
                    // Preparo il file da aprire
                    File* to_open = my_malloc(sizeof(File));
                    // Chiave del file da aprire
                    char* file_key = my_malloc(sizeof(char) * MAX_PATH_LENGTH);

                    // Imposto il percorso
                    strncpy(to_open->path, request.path, MAX_PATH_LENGTH);
                    strncpy(file_key, to_open->path, MAX_PATH_LENGTH);

                    // Inizializzo la lock
                    SYSCALL_EXIT("pthread_mutex_init", err, pthread_mutex_init(&(to_open->lock), NULL),
                    "Impossibile inizializzare la lock del file", "");
                    // Imposto il descrittore del client
                    to_open->client_descriptor = request.client_descriptor;
                    // Segnalo che l'ultima operazione era una open
                    to_open->last_op = OPENFILE;
                    // Segnalo che il file è aperto
                    to_open->is_open = TRUE;
                    // Imposto il primo timestamp
                    to_open->last_used = timestamp;
                    // Il file non è stato modificato inizialmente
                    to_open->modified = FALSE;
                    // Il contenuto iniziale ha dimensione 0
                    to_open->content_size = 0;

                    // Infine aggiungo il file alla tabella
                    hashmap_put(&files, to_open, file_key);
                    UNLOCK(&files_mutex);

                    log_info("File creato e aperto con successo");
                    log_info("[OP] open");
                }    
                else
                {
                    to_send = INCONSISTENT_FLAGS;            
                    UNLOCK(&files_mutex);
                }

                SERVER_OP(writen(request.client_descriptor, &to_send, sizeof(to_send)), sec_close_connection(request.client_descriptor));
                break;
            case READFILE:;
                log_info("Tentativo di apertura del file %s", request.path);

                ServerResponse response;
                memset(&response, 0, sizeof(response));

                LOCK(&files_mutex);
                if (hashmap_has_key(files, request.path))
                {
                    // Ottengo il file
                    File* to_read = (File*)hashmap_get(files, request.path);
                    UNLOCK(&files_mutex);

                    // Se è aperto, leggo, altrimenti segnalo l'errore
                    LOCK(&(to_read->lock));
                    to_read->last_used = timestamp;
                    if (to_read->is_open)
                    {
                        // Copio il contenuto
                        memcpy(response.content, to_read->content, to_read->content_size);
                        // Lo decomprimo
                        response.content_size = server_decompress(response.content, response.content, to_read->content_size);
                    }
                    else
                        response.error_code = NOT_OPENED;
                    UNLOCK(&(to_read->lock));
                }
                else
                {
                    response.error_code = FILE_NOT_FOUND;
                    UNLOCK(&files_mutex);
                }

                if (response.error_code == OK)
                    log_info("File letto, dimensione contenuto: %ld", response.content_size);
                log_info("[RD] %ld", response.content_size);

                // Invio la risposta al client
                SERVER_OP(writen(request.client_descriptor, &response, sizeof(response)), sec_close_connection(request.client_descriptor));
                break;
            case PARTIALREAD:;
                int finished = FALSE;
                int sent = 0;
                
                LOCK(&files_mutex);

                // Calcolo quanti file devo effettivamente spedire
                if (request.flags < 0 || files.curr_size < request.flags)
                    to_send = files.curr_size;
                else   
                    to_send = request.flags;
                
                if (response.error_code == OK)
                    log_info("Tentativo di leggere %d file", to_send);

                // Indico al client quanti file sto per ritornare
                SERVER_OP(writen(request.client_descriptor, &to_send, sizeof(to_send)), sec_close_connection(request.client_descriptor));

                // Spedisco i file
                for (int i=0; i<files.size && !finished; i++)
                {
                    for (int j=0; j<files.lists[i].length && !finished; i++)
                    {
                        ServerResponse response;
                        memset(&response, 0, sizeof(response));
                        File* curr_file = (File*)list_get(files.lists[i], j);

                        if (curr_file == NULL)
                            response.error_code = FILE_NOT_FOUND;
                        else
                        {
                            curr_file->last_used = timestamp;
                            strcpy(response.path, curr_file->path);
                            memcpy(response.content, curr_file->content, curr_file->content_size);
                            // Decomprimo il contenuto
                            response.content_size = server_decompress(response.content, response.content, curr_file->content_size);
                        }
                        
                        // Invio la risposta
                        SERVER_OP(writen(request.client_descriptor, &response, sizeof(response)), sec_close_connection(request.client_descriptor));
                        log_info("File letto : %s, dimensione contenuto: %ld", response.path, response.content_size);
                        log_info("[RD] %ld",  response.content_size);
                        sent++;

                        if (sent >= to_send)
                            finished = TRUE;
                    }
                }
                
                UNLOCK(&files_mutex);
                break;
            case WRITEFILE:;
                // File espulsi da rispedire al client
                files_to_send = NULL;
                // Prendo il file dalla tabella
                LOCK(&files_mutex);
                // File da scrivere
                File* to_write = NULL;
                // Numero di file nell'hashmap
                int n_files = files.curr_size;
                // Dimensione del file da scrivere
                file_size = -1;

                if (hashmap_has_key(files, request.path))
                {
                    // Contenuto compresso del file e sua dimensione
                    char compressed[MAX_FILE_SIZE];
                    memset(compressed, 0, MAX_FILE_SIZE);
                    // Comprimo il file
                    file_size = server_compress(request.content, request.content, request.content_size);
                    // Dato che esiste, prendo il file corrispondente al path
                    to_write = (File*)hashmap_get(files, request.path);
                    UNLOCK(&files_mutex);

                    LOCK(&(to_write->lock));
                    // Se il file è aperto e l'ultima operazione era una open da parte dello stesso client,
                    // allora posso scrivere
                    if (to_write->is_open)
                    {
                        if (to_write->last_op == OPENFILE && to_write->client_descriptor == request.client_descriptor)
                        {
                            // Scrivo solo se ho spazio per farlo
                            if (file_size < config.tot_space)
                            {
                                // Aggiorno lo spazio allocato dai file
                                LOCK(&allocated_space_mutex);
                                allocated_space += file_size;

                                // Se non posso tenere altri file in cache, ne rimuovo finché non ho 
                                // spazio disponibile
                                LOCK(&files_mutex);

                                // Nel peggiore dei casi, i file da spedire indietro sono tutti (TODO: usare una lista)
                                files_to_send = my_malloc(sizeof(File) * files.curr_size);

                                // Rimpiazzo finché non ho abbastanza spazio o finché non ho tolto abbastanza files
                                while ((allocated_space > config.tot_space && to_send >= 0) || (n_files > config.max_files && to_send >= 0))
                                {
                                    log_info("Chiamato algoritmo di rimpiazzamento\n\t(Spazio: %d / %d)\n\t(N files: %d / %d)", 
                                        allocated_space, config.tot_space, n_files, config.max_files);
                                    log_info("[LRU] called");
                                    // Calcolo il path del file da rimuovere
                                    char* to_remove = get_LRU(request.path);
                                    
                                    // Se la LRU ha ritornato qualcosa, allora rimuovo quel file
                                    if (to_remove != NULL)
                                    {
                                        log_info("Rimuovo il file %s", to_remove);
                                        // Salvo il file perché la remove dealloca i dati
                                        File* LRUed = (File*)hashmap_get(files, to_remove);
                                        memcpy(&(files_to_send[n_expelled]), LRUed, sizeof(*LRUed));

                                        // Aggiorno lo spazio e rimuovo il file
                                        allocated_space -= LRUed->content_size;
                                        if (hashmap_remove(&files, to_remove) != OK)
                                            perror("Impossibile rimuovere il file");
                                        
                                        to_send++;
                                        n_files--;

                                        free(to_remove);
                                    }
                                    // Altrimenti ho fallito e non posso aggiungere il file
                                    else
                                        to_send = LRU_FAILURE;
                                }
                                UNLOCK(&files_mutex);
                                UNLOCK(&allocated_space_mutex);
                            }
                            else
                                to_send = FILE_TOO_BIG;
                            
                            LOCK(&files_mutex);
                            if (files.curr_size > config.max_files)
                                to_send = FILE_AMOUNT_LIMIT;
                            UNLOCK(&files_mutex);
                        }
                        else
                            to_send = INVALID_LAST_OPERATION;
                    }
                    else
                        to_send = NOT_OPENED;

                    UNLOCK(&(to_write->lock));
                }
                // Se il file non esiste nel server, lo segnalo
                else
                {
                    to_send = FILE_NOT_FOUND;
                    UNLOCK(&files_mutex);
                }

                log_info("Esito LRU: %d", to_send);
                // Invio to_send al client
                SERVER_OP(writen(request.client_descriptor, &to_send, sizeof(int)), sec_close_connection(request.client_descriptor));

                // E invio anche i file rimossi
                for (int i=0; i<to_send; i++)
                {
                    log_info("Invio il file rimosso %s", request.path);
                    // Creo la risposta
                    ServerResponse response;
                    memset(&response, 0, sizeof(response));

                    response.error_code = 0;
                    memcpy(response.path, files_to_send[i].path, sizeof(files_to_send[i].path));
                    memcpy(response.content, files_to_send[i].content, files_to_send[i].content_size);

                    // Decomprimo i dati prima di spedirli
                    response.content_size = server_decompress(response.content, response.content, files_to_send[i].content_size);

                    // Invio la risposta
                    SERVER_OP(writen(request.client_descriptor, &response, sizeof(response)), sec_close_connection(request.client_descriptor));
                }

                // Solo ora posso scrivere i dati inviati dal client
                memcpy(to_write->content, request.content, file_size);
                to_write->last_op = WRITEFILE;
                to_write->last_used = timestamp;
                to_write->content_size = file_size;
                to_write->modified = TRUE;

                LOCK(&allocated_space_mutex);
                log_info("[SIZE] %d", allocated_space);
                UNLOCK(&allocated_space_mutex);

                LOCK(&files_mutex);
                log_info("[NFILES] %d", files.curr_size);
                UNLOCK(&files_mutex);

                log_info("Scrittura del file %s di dimensione %d", request.path, file_size);
                log_info("[WT] %ld", file_size);

                // Dealloco
                if (files_to_send != NULL)
                    free(files_to_send);

                break;
            case APPENDTOFILE:;
                log_info("Tentativo di append al file %s", request.path);
                // Prendo il file a cui appendere i dati
                LOCK(&files_mutex);
                File* file = (File*)hashmap_get(files, request.path);
                // Array dei file da espellere
                files_to_send = my_malloc(sizeof(File) * files.curr_size);
                // Dimensione del contenuto da appendere
                file_size = server_compress(request.content, request.content, request.content_size);

                if (file != NULL)
                {
                    LOCK(&allocated_space_mutex);
                    // Aggiungo la lunghezza dei dati da concatenare al file
                    allocated_space += file_size;

                    // Se sto cercando di avere un file più grande dell'intero sistema, non ho speranze di aggiungerlo
                    if ((file_size + file->content_size) > config.tot_space)
                        to_send = FILE_TOO_BIG;
                    else
                    {
                        // Altrimenti espello file finché non ho abbastanza spazio
                        while (allocated_space > config.tot_space && to_send >= 0)
                        {
                            log_info("[LRU] called");
                            log_info("Invocato algoritmo di rimpiazzamento\n\t(Spazio: %d / %d)",
                                allocated_space, config.tot_space);
                            // Provo a ottnere il path del lru
                            char* expelled_path = get_LRU(request.path);

                            if (expelled_path != NULL && strcmp(expelled_path, "") != 0)
                            {
                                log_info("Target del rimpiazzamento: %s", expelled_path);
                                // Copio il file da eliminare così da poterlo spedire al client
                                File* to_remove = hashmap_get(files, expelled_path);
                                memcpy(&(files_to_send[to_send]), to_remove, sizeof(File));

                                // Rimuovo il file dal sistema
                                if (hashmap_remove(&files, expelled_path) != OK)
                                    perror("Impossibile rimuovere il file");
                                // Ho dello spazio in meno
                                allocated_space -= files_to_send[to_send].content_size;
                                // Ho un file da spedire al client in più
                                to_send++;
                            }
                            else
                                to_send = LRU_FAILURE;
                            if (expelled_path != NULL)
                                free(expelled_path);
                        }
                    }
                    UNLOCK(&allocated_space_mutex);
                }
                else
                    to_send = FILE_NOT_FOUND;
                
                UNLOCK(&files_mutex);

                // Invio il risultato dell'operazione al client
                SERVER_OP(writen(request.client_descriptor, &to_send, sizeof(to_send)), sec_close_connection(request.client_descriptor));
                
                // Se non ci sono stati errori, invio i file al client
                if (to_send >= 0)
                {
                    for (int i=0; i<to_send; i++)
                    {
                        log_info("Invio il file rimosso %s", files_to_send[i].path);
                        // Creo la risposta
                        ServerResponse response;

                        response.error_code = 0;
                        memcpy(response.path, files_to_send[i].path, sizeof(response.path));
                        memcpy(response.content, files_to_send[i].content, file_size);
                        response.content_size = server_decompress(response.content, response.content, files_to_send[i].content_size);

                        // Invio la risposta
                        SERVER_OP(writen(request.client_descriptor, &response, sizeof(response)), sec_close_connection(request.client_descriptor));
                    }
                }

                LOCK(&allocated_space_mutex);
                log_info("[SIZE] %d", allocated_space);
                UNLOCK(&allocated_space_mutex);

                LOCK(&files_mutex);
                log_info("[NFILES] %d", files.curr_size);
                UNLOCK(&files_mutex);

                // Adesso posso appendere
                file->last_used = timestamp;
                file->content_size += file_size;
                file->modified = TRUE;
                free(files_to_send);
                strncat(file->content, request.content, file_size);

                log_info("Appendo il contenuto di dimensione %d al file %s", file_size, file->path);
                log_info("[WT] %d", file_size);

                break;
            case CLOSEFILE:
                log_info("Tentativo di chiusura del file %s", request.path);
                LOCK(&files_mutex);
                
                if (hashmap_has_key(files, request.path))
                {
                    File* to_close = (File*)hashmap_get(files, request.path);
                    UNLOCK(&files_mutex);

                    LOCK(&(to_close->lock));
                    // Se è aperto lo chiudo
                    if (to_close->is_open)
                    {
                        to_close->is_open = FALSE;
                        to_close->last_op = CLOSEFILE;

                        log_info("Chiusura del file avvenuta con successo");
                        log_info("[CL] closed");
                    }
                    // Altrimenti segnalo quello che stavo cercando di fare
                    else
                        to_send = ALREADY_CLOSED;
                        
                    UNLOCK(&(to_close->lock));
                }
                else
                {
                    log_info("File da chiudere non trovato\n");
                    to_send = FILE_NOT_FOUND;
                    UNLOCK(&files_mutex);
                }

                // Invio la risposta
                SERVER_OP(writen(request.client_descriptor, &to_send, sizeof(int)), sec_close_connection(request.client_descriptor));
                break;
            case CLOSECONNECTION:;
                log_info("Chiusura della connessione da parte del client %d", request.client_descriptor);
                // Descrittore in formato di stringa per poterlo rimuovere
                char desc_string[20];
                sprintf(desc_string, "%d", request.client_descriptor);

                // Rimuovo il descrittore dal set
                LOCK(&desc_set_lock);
                FD_CLR(request.client_descriptor, &desc_set);
                UNLOCK(&desc_set_lock);

                // Rimuovo il descrittore dalla lista dei descrittori attivi
                LOCK(&client_fds_lock);
                list_remove_by_key(&client_fds, desc_string);
                if (errno != OK)
                    perror("Fallita chiusura della connessione");
                UNLOCK(&client_fds_lock);
                
                SERVER_OP(writen(request.client_descriptor, &to_send, sizeof(to_send)), sec_close_connection(request.client_descriptor));
                break;
            default:;
                log_info("Ricevuta richiesta non supportata, chiusura della connessione al client %d", request.client_descriptor);
                fprintf(stderr, "Codice richiesta %d non supportato dal server\n", request.op_code);
                // Chiudo la connessione con quel client
                sec_close_connection(request.client_descriptor);
                break;
        }

        log_info("Terminata gestione della richiesta.");

        // Resetto il valore di ritorno
        to_send = 0;
        // Una volta esaudita la richiesta, posso ricominciare ad ascoltare quel client
        LOCK(&desc_set_lock);
        FD_SET(request.client_descriptor, &desc_set);
        UNLOCK(&desc_set_lock);
    }

    pthread_exit(NULL);
}

void sec_close_connection(int fd)
{
    printf("Errore di lettura o scrittura, chiusura della connessione con il client %d\n", fd);
    char to_remove[20];
    sprintf(to_remove, "%d", fd);

    LOCK(&client_fds_lock);
    list_remove_by_key(&client_fds, to_remove);
    if (errno != OK)
        perror("Fallita chiusura della connessione");
    UNLOCK(&client_fds_lock);

    LOCK(&desc_set_lock);
    FD_CLR(fd, &desc_set);
    UNLOCK(&desc_set_lock);
}

void* dispatcher(void* args)
{
    log_info("Inizializzazione del dispatcher");
    // Read set, è locale e serve solo al dispatcher
    fd_set read_set;

    while (!must_stop)
    {
        FD_ZERO(&read_set);
        // Salvo il numero massimo dei fd
        LOCK(&max_fd_lock);
        int loc_max_fd = max_fd;
        UNLOCK(&max_fd_lock);

        if (loc_max_fd > 0)
        {
            // Copio il set totale
            LOCK(&desc_set_lock);
            read_set = desc_set;
            UNLOCK(&desc_set_lock);

            // Calcolo il numero attuale di connessioni attive
            LOCK(&client_fds_lock);
            int list_len = client_fds.length;
            UNLOCK(&client_fds_lock);
            
            // Uso la select per gestire le connessioni che necessitano di attenzione
            if (list_len != 0)
            {
                struct timeval timeout;
                timeout.tv_sec = 0;
                timeout.tv_usec = 1;

                if (select(loc_max_fd + 1, &read_set, NULL, NULL, &timeout) == -1)
                {
                    perror("Errore durante la select");
                    exit(EXIT_FAILURE);
                }

                // Ciclo nei set e verifico quali sono pronti
                for (int i=0; i<list_len; i++)
                {
                    LOCK(&client_fds_lock);
                    // Ottengo il fd corrente
                    void* res = list_get(client_fds, i);
                    if (res == NULL)
                    {
                        UNLOCK(&client_fds_lock);
                        continue;
                    }
                        
                    int curr_fd = *((int*)res);
                    UNLOCK(&client_fds_lock);          

                    // Controllo che sia settato e che abbia qualcosa da leggere
                    if (curr_fd <= loc_max_fd && FD_ISSET(curr_fd, &read_set))
                    {
                        // Puntatore per salvare la richiesta: è allocata sullo heap perché altrimenti
                        // la lista ne perderebbe traccia una volta usciti da questa funzione
                        ClientRequest* request = my_malloc(sizeof(ClientRequest));
                        // Dimensione dei dati letti per controllare errori
                        int read_size = readn(curr_fd, request, sizeof(*request));

                        if (read_size == -1)
                        {
                            perror("Errore nella lettura della richiesta del client");
                            sec_close_connection(request->client_descriptor);
                        }
                        else
                        {
                            // Aggiungo il file descriptor del client alla richiesta ricevuta
                            request->client_descriptor = curr_fd;
                            // Impedisco che la select possa leggere duplicati nelle richieste
                            LOCK(&desc_set_lock);
                            FD_CLR(curr_fd, &desc_set);
                            UNLOCK(&desc_set_lock);
                            // La aggiungo alla coda delle richieste
                            LOCK(&queue_mutex);
                            list_enqueue(&requests, request, NULL);

                            // Segnalo che la coda delle richieste ha un elemento da elaborare
                            SIGNAL(&queue_not_empty);
                            UNLOCK(&queue_mutex);
                        }
                    }
                }
            }
        }
    }

    pthread_exit(NULL);
}

void* connession_handler(void* args)
{
    log_info("Inizializzazione dell'handler delle connessioni");
    // Informazioni del client
    struct sockaddr client_info;
    socklen_t client_addr_length = sizeof(client_info);
    // Dati dei nodi delle lsite
    int client_fd;
    int* to_add;
    char* key;

    while (!must_stop)
    {
        // Attendo una richiesta di connessione
        if ((client_fd = accept(socket_desc, &client_info, &client_addr_length)) > 0)
        {
            log_info("Ricevuta richiesta di connessione dal client %d", client_fd);
            // Rialloco così i dati puntano a una locazione differente
            to_add = my_malloc(sizeof(int));
            *to_add = client_fd;

            key = my_malloc(sizeof(char) * 20);

            // Uso come chiave l'fd del thread
            sprintf(key, "%d", client_fd);

            // Aggiungo l'fd alla lista, accedo in mutua esclusione perché potrei ricevere una 
            // richiesta di disconnessione che modifica la lista
            LOCK(&client_fds_lock);
            list_push(&client_fds, to_add, key);
            log_info("[CN] %d", client_fds.length);
            UNLOCK(&client_fds_lock);

            // Aggiungo il descrittore al read set
            LOCK(&desc_set_lock);
            FD_SET(client_fd, &desc_set);
            UNLOCK(&desc_set_lock);

            // Aggiorno il massimo fd se necessario
            LOCK(&max_fd_lock);
            if (max_fd < client_fd)
                max_fd = client_fd;
            UNLOCK(&max_fd_lock);
        }
    }
    
    pthread_exit(NULL);
}

int create_log()
{
    // Nome del file di log
    char log_name[MAX_TIME_LENGTH];
    // Path completo del file di log
    char log_path[MAX_PATH_LENGTH * 2];
    // Timestamp attuale
    time_t raw_time;
    struct tm* time_info;

    // Ottengo il timestamp attuale
    time(&raw_time);
    time_info = localtime(&raw_time);
    
    // Converto il timestamp attuale in formato leggibile 
    strftime(log_name, MAX_TIME_LENGTH, "%Y-%m-%d | %H:%M:%S.txt", time_info);

    // Verifico se esiste la cartella dei log
    DIR* logs = opendir(config.log_path);
    // Se non esiste la creo
    if (logs == NULL)
    {
        if (mkdir(config.log_path, 0777) != 0)
        {
            perror("Impossibile creare la directory per i log, il file di log verrà salvato nella cwd");
            sprintf(log_path, "%s", log_name);
        }
        else
            sprintf(log_path, "%s/%s", config.log_path, log_name);
    }
    else
    {
        int err = 0;
        SYSCALL_EXIT("closedir", err, closedir(logs), "Impossibile chiudere la directory dei log\n", "");
        sprintf(log_path, "%s/%s", config.log_path, log_name);
    }

    // Creo e apro un file chiamato come la data attuale
    log_file = fopen(log_path, "w");
    // Se log_file non è NULL, sposto stdout sul log_file così nel caso sia stato impossibile avere un file
    // di log, le informazioni vengono almeno stampate a schermo sul server
    if (log_file != NULL)
    {
        int log_fd;
        int err;

        SYSCALL_RETURN("fileno", log_fd, fileno(log_file), "Impossibile ricavare l'fd del file di log\n", "");
        SYSCALL_RETURN("dup2", err, dup2(log_fd, 1), "Impossibile redirigere stdout sul file di log\n", "");
    }

    return 0;
}

int initialize_socket()
{
    int err = 0;
    // Pulisco eventuali socket precedenti
    unlink(config.socket_name);

    // Creo il socket
    int socket_desc;
    SYSCALL_EXIT("socket", socket_desc, socket(AF_UNIX, SOCK_STREAM, 0), "Impossibile creare il socket.\n", "");
    // Indirizzo del socket
    struct sockaddr_un socket_addr;

    // Copia dell'indirizzo
    memset(&socket_addr, '0', sizeof(socket_addr));
    strncpy(socket_addr.sun_path, config.socket_name, strlen(config.socket_name) + 1);
    socket_addr.sun_family = AF_UNIX;

    // Binding dell'indirizzo al socket
    SYSCALL_EXIT("bind", err, bind(socket_desc, (struct sockaddr*) &socket_addr, (socklen_t)sizeof(socket_addr)), 
        "Impossibile terminare il binding del socket.\n", "");
    // Mi metto in ascolto
    SYSCALL_EXIT("listen", err, listen(socket_desc, MAX_CONNECTION_QUEUE_SIZE), 
        "Impossibile invocare la listen sul socket.\n", "");

    // Ritorno l'fd del server
    return socket_desc;
}

ServerConfig config_server(const char* file_name)
{
    ServerConfig ret;
    FILE* config_file;
    char line_buffer[PATH_MAX];
    
    ret.n_workers = -1;
    ret.tot_space = -1;
    ret.max_files = -1;
    ret.socket_name[0] = '\0';
    ret.log_path[0] = '\0';

    config_file = fopen(file_name, "r");

    if (config_file == NULL)
    {
        perror("Impossibile aprire il file di configurazione.");
        errno = CONFIG_FILE_ERROR;
        return ret;
    }
    else
    {
        // Numero di thread workers
        if (fscanf(config_file, "%s\n", line_buffer) != 0)
        {
            ret.n_workers = string_to_int(line_buffer, TRUE);
            if (errno != 0)
            {
                perror("Il numero di thread workers non e' valido.");
                errno = CONFIG_FILE_ERROR;
                return ret;
            }
        }
        else
        {
            perror("Impossibile leggere la prima riga del file di configurazione.");
            errno = CONFIG_FILE_ERROR;
            return ret;
        }

        // Spazio totale del server
        if (fscanf(config_file, "%s\n", line_buffer) != 0)
        {
            ret.tot_space = string_to_int(line_buffer, TRUE);
            if (errno != 0)
            {
                perror("Il valore dello spazio totale del server non e' valido.");
                errno = CONFIG_FILE_ERROR;
                return ret;
            }
        }
        else
        {
            perror("Impossibile leggere la seconda riga del file di configurazione.");
            errno = CONFIG_FILE_ERROR;
            return ret;
        }

        // Numero massimo di file conservabili nel server
        if (fscanf(config_file, "%s\n", line_buffer) != 0)
        {
            ret.max_files = string_to_int(line_buffer, TRUE);
            if (errno != 0)
            {
                perror("Il valore dello spazio totale del server non e' valido.");
                errno = CONFIG_FILE_ERROR;
                return ret;
            }
        }
        else
        {
            perror("Impossibile leggere la terza riga del file di configurazione.");
            errno = CONFIG_FILE_ERROR;
            return ret;
        }

        // Nome del socket
        if (fscanf(config_file, "%s\n", line_buffer) != 0)
        {
            strcpy(ret.socket_name, line_buffer);
        }
        else
        {
            perror("Impossibile leggere la quarta riga del file di configurazione.");
            errno = CONFIG_FILE_ERROR;
            return ret;
        }

        // Path del file di log
        if (fscanf(config_file, "%s\n", line_buffer) != 0)
        {
            strcpy(ret.log_path, line_buffer);
        }
        else
        {
            perror("Impossibile leggere la quinta riga del file di configurazione.");
            errno = CONFIG_FILE_ERROR;
            return ret;
        }

        fclose(config_file);
        return ret;
    }   
}

char* get_LRU(char* current_path)
{
    // Timestamp iniziale, lo setto al momento attuale così sicuramente almeno un file sarà stato usato prima
    time_t timestamp;
    // Path del file da eliminare
    char* to_ret = my_malloc(sizeof(char) * MAX_PATH_LENGTH);

    time(&timestamp);

    for (int i=0; i<files.size; i++)
    {
        for (int j=0; j<files.lists[i].length; j++)
        {
            File* curr_file = (File*)list_get(files.lists[i], j);

            // Se il file è stato modificato, è stato usato meno recentemente del file corrente e 
            // non è il file che devo inserire
            if (curr_file->modified && curr_file->last_used <= timestamp && 
                strcmp(curr_file->path, current_path) != 0)
            {
                // Allora potrebbe essere un possibile file da rimuovere secondo la LRU
                timestamp = curr_file->last_used;
                strncpy(to_ret, curr_file->path, MAX_PATH_LENGTH);
            }
        }
    }

    return to_ret;
}

void print_request_node(Node* to_print)
{
    ClientRequest r = *(ClientRequest*)to_print->data;
    
    printf("Op: %d\nContent:%s\n\n", r.op_code, r.content);
}

void print_file_node(Node* to_print)
{
    File r = *(File*)to_print->data;
    
    printf("Path: %s\nContent:%s\n\n", r.path, r.content);
}

void* sighandler(void* param)
{
    int signal = -1;
    
    while (TRUE)
    {
        sigwait(&mask, &signal);

        if (!must_stop)
        {
            // Segnalo che ho terminato
            must_stop = TRUE;

            if (signal == SIGINT || signal == SIGQUIT)
            {
                // Creo le statistiche
                // Chiudo tutte le connessioni
                Node* curr = client_fds.head;
                while (curr != NULL)
                {
                    close(*((int*)curr->data));
                    curr = curr->next;
                }
                // Coda richieste
                list_clean(requests, NULL);

                // Aspetto che il dispatcher finisca
                THREAD_JOIN(dispatcher_tid, NULL);

                // Mando una finta connessione per sbloccare il gestore delle connessioni
                int socket_fd;
                socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);
                // Indirizzo del socket
                struct sockaddr_un address;
                memset(&address, 0, sizeof(address));
                strncpy(address.sun_path, config.socket_name, strlen(config.socket_name));
                address.sun_family = AF_UNIX;
                connect(socket_fd, (struct sockaddr*) &address, sizeof(address));

                // Aspetto che il master finisca
                THREAD_JOIN(connession_handler_tid, NULL);

                printf("Aspetto\n");
                // Risveglio tutti i worker
                pthread_cond_broadcast(&queue_not_empty);

                // Aspetto che i worker finiscano
                for (int i=0; i<config.n_workers; i++)
                {
                    THREAD_JOIN(tids[i], NULL);
                }
            }
            else
            {
                // SIGHUP
                // Evito di ricevere nuove connessioni
                // Mando una finta connessione per sbloccare il gestore delle connessioni e farlo terminare
                int socket_fd;
                socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);
                // Indirizzo del socket
                struct sockaddr_un address;
                memset(&address, 0, sizeof(address));
                strncpy(address.sun_path, config.socket_name, strlen(config.socket_name));
                address.sun_family = AF_UNIX;
                connect(socket_fd, (struct sockaddr*) &address, sizeof(address));

                THREAD_JOIN(connession_handler_tid, NULL);

                // Finché la coda ha elementi
                LOCK(&files_mutex);
                while (files.curr_size > 0)
                {
                    // Sveglio thread
                    UNLOCK(&files_mutex);
                    SIGNAL(&queue_not_empty);
                    LOCK(&files_mutex);
                }
                UNLOCK(&files_mutex);

                // Chiudo tutte le connessioni
                Node* curr = client_fds.head;
                while (curr != NULL)
                {
                    close(*((int*)curr->data));
                    curr = curr->next;
                }

                // Coda richieste
                list_clean(requests, NULL);

                for (int i=0; i<config.n_workers; i++)
                {
                    THREAD_JOIN(tids[i], NULL);
                }

                // Aspetto che il dispatcher finisca
                THREAD_JOIN(dispatcher_tid, NULL);
            }

            // Pulisco tutte le strutture dati
            fclose(log_file);
            // Lista dei client
            list_clean(client_fds, NULL);
            // Tabella dei file
            hashmap_clean(files, NULL);
            // Tids
            free(tids);
            // Elimino il socket
            unlink(config.socket_name);

            // Stampo le statistiche
            printf("finite stats\n");
        }
        
        printf("Server terminato\n");
        pthread_exit(NULL);
    }
}

void clean_everything()
{
    // Chiudo tutte le connessioni
    Node* curr = client_fds.head;
    while (curr != NULL)
    {
        close(*((int*)curr->data));
        curr = curr->next;
    }
    // Coda richieste
    list_clean(requests, NULL);
    // Pulisco tutte le strutture dati
    if (log_file != NULL)
        fclose(log_file);
    // Lista dei client
    list_clean(client_fds, NULL);
    // Tabella dei file
    hashmap_clean(files, NULL);
    // Tids
    free(tids);
    // Elimino il socket
    unlink(config.socket_name);
    // Esco dall'handler
    THREAD_KILL(sighandler_tid, SIGKILL);
    THREAD_JOIN(sighandler_tid, NULL);
}

void log_info(const char* fmt, ...)
{
    char str_time[MAX_TIME_LENGTH];
    // Timestamp attuale
    time_t raw_time;
    struct tm* time_info;
    va_list valist;
    char buf[MAX_FILE_SIZE];

    va_start(valist, fmt);
    vsnprintf(buf, MAX_FILE_SIZE, fmt, valist);
    va_end(valist);

    // Ottengo il timestamp attuale
    time(&raw_time);
    time_info = localtime(&raw_time);
    
    LOCK(&log_mutex);

    // Converto il timestamp attuale in formato leggibile 
    strftime(str_time, MAX_TIME_LENGTH, "%H:%M:%S", time_info);
    // Scrivo nel file di log
    printf("%s | -> %s\n", str_time, buf);
    fflush(stdout);

    UNLOCK(&log_mutex);
}

int server_compress(char* data, char* buffer, int size)
{
    // Buffer sorgente
    char* a = data;
    // Buffer intermedio che contiene a in formato compresso
    char b[MAX_FILE_SIZE];
    memset(b, 0, MAX_FILE_SIZE);

    // zlib struct
    z_stream defstream;
    defstream.zalloc = Z_NULL;
    defstream.zfree = Z_NULL;
    defstream.opaque = Z_NULL;
    // Dimensione dell'input
    defstream.avail_in = (uInt)size+1;
    // Input
    defstream.next_in = (Bytef *)a;
    // Dimensione dell'output
    defstream.avail_out = (uInt)sizeof(b);
    // Output
    defstream.next_out = (Bytef *)b; // output char array
    
    // Invoco le funzioni di compressione
    deflateInit(&defstream, Z_BEST_COMPRESSION);
    deflate(&defstream, Z_FINISH);
    deflateEnd(&defstream);

    // Copio i dati nel buffer di ritorno
    memcpy(buffer, b, defstream.total_out);

    // Ritorno la dimensione dei dati compressi
    return defstream.total_out;
}

int server_decompress(char* data, char* buffer, unsigned int data_size)
{
    // Buffer provvisorio per la decompressione
    char c[MAX_FILE_SIZE];

    // zlib struct
    z_stream infstream;
    infstream.zalloc = Z_NULL;
    infstream.zfree = Z_NULL;
    infstream.opaque = Z_NULL;
    // Dimensione input (dato da parametro)
    infstream.avail_in = data_size;
    // Input
    infstream.next_in = (Bytef *)data; 
    // Dimensione output
    infstream.avail_out = (uInt)sizeof(c);
    // Output
    infstream.next_out = (Bytef *)c; 
     
    // Invoco le funzioni di decompressione
    inflateInit(&infstream);
    inflate(&infstream, Z_NO_FLUSH);
    inflateEnd(&infstream);

    // Copio nel buffer di ritorno
    memcpy(buffer, c, infstream.total_out);
    // Ritorno la dimensione della stringa decompressa
    return infstream.total_out;
}