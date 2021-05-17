#include "server.h"

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

int debug = 0;

// Configurazione del server, globale per essere vista dalla cleanup
ServerConfig config;

int main(int argc, char** argv)
{
    // Inizializzazione delle strutture dati necessarie
    list_initialize(&requests, print_request_node);
    list_initialize(&client_fds, NULL);
    hashmap_initialize(&files, 1021, NULL);
    // Azzero la maschera dei socket da leggere
    FD_ZERO(&desc_set);

    // Parsing della configurazione del server
    config = config_server();

    if (errno == 0)
    {
        // Tid del thread che gestisce le connessioni
        pthread_t connession_handler_tid = -1;
        // Tid del thread che smista le richieste
        pthread_t dispatcher_tid = -1;

        socket_desc = initialize_socket();
        create_log();

        // Alloco spazio per i tids
        tids = malloc(sizeof(pthread_t) * config.n_workers);

        // Faccio partire il thread master, che accetta le connessioni
        pthread_create(&connession_handler_tid, NULL, &connession_handler, NULL);
        // Faccio partire il dispatcher, che riceve le richieste e le aggiunge in coda
        pthread_create(&dispatcher_tid, NULL, &dispatcher, NULL);

        // Faccio partire i thread workers
        for (int i=0; i<config.n_workers; i++)
        {
            pthread_create(&tids[i], NULL, &worker, NULL);
        }

        // Aspetto che il dispatcher finisca
        pthread_join(dispatcher_tid, NULL);
        // Aspetto che il master finisca
        pthread_join(connession_handler_tid, NULL);
    }
    else
    {
        perror("Impossibile terminare la configurazione del server.\n");
        return CONFIG_FILE_ERROR;
    }

    return 0;
}

void* worker(void* args)
{
    // Salvataggio della richiesta
    ClientRequest request;

    while (TRUE)
    {
        // Return value
        int to_send = 0;
        // Mi metto in attesa sulla variabile condizionale
        pthread_mutex_lock(&queue_mutex);
        while (requests.length <= 0)
            pthread_cond_wait(&queue_not_empty, &queue_mutex);

        // Se sono arrivato fin qui, ho una richiesta da elaborare
        ClientRequest* to_free = (ClientRequest*)list_dequeue(&requests);
        // Copio la richiesta
        memcpy(&request, to_free, sizeof(request));
        perror("Errore?");
        printf("Richiesta da elaborare | desc: %d\nOP:%d\n", request.client_descriptor, request.op_code);
        // Libero la memoria allocata
        free(to_free);
        // Sblocco la coda
        pthread_mutex_unlock(&queue_mutex);

        // Eseguo la richiesta
        switch (request.op_code)
        {
            case OPENFILE:;
                // Verifico che il file non esista o non sia già aperto
                pthread_mutex_lock(&files_mutex);
                if (hashmap_has_key(files, request.path))
                {
                    // Ottengo il file che mi interessa
                    File* to_open = (File*)hashmap_get(files, request.path);
                    pthread_mutex_unlock(&files_mutex);

                    // Se è già stato aperto, lo segnalo
                    if (to_open->is_open)
                        to_send = ALREADY_OPENED;
                    else
                    {
                        // Se non era aperto, lo apro
                        pthread_mutex_lock(&to_open->lock);
                        to_open->is_open = TRUE;
                        pthread_mutex_unlock(&to_open->lock);
                    }
                }
                else
                {
                    time_t timestamp;
                    time(&timestamp);
                    // Il file non esisteva, allora lo aggiungo
                    // Preparo il file da aprire
                    File* to_open = malloc(sizeof(File));
                    // Imposto il percorso
                    strcpy(to_open->path, request.path);
                    // Inizializzo la lock
                    pthread_mutex_init(&(to_open->lock), NULL);
                    // Imposto il descrittore del client
                    to_open->client_descriptor = request.client_descriptor;
                    // Segnalo che l'ultima operazione era una open
                    to_open->last_op = OPENFILE;
                    // Segnalo che il file è aperto
                    to_open->is_open = TRUE;
                    // Imposto il primo timestamp
                    to_open->last_used = timestamp;

                    // Infine aggiungo il file alla tabella
                    hashmap_put(&files, to_open, to_open->path);

                    printf("Aggiunto file\n");

                    pthread_mutex_unlock(&files_mutex);
                }                

                writen(request.client_descriptor, &to_send, sizeof(to_send));
                break;
            case READFILE:
                break;
            case WRITEFILE:
                // Prendo il file dalla tabella
                pthread_mutex_lock(&files_mutex);
                if (hashmap_has_key(files, request.path))
                {
                    // Dato che esiste, prendo il file corrispondente al path
                    File* to_write = (File*)hashmap_get(files, request.path);
                    pthread_mutex_unlock(&files_mutex);

                    pthread_mutex_lock(&(to_write->lock));
                    // Se il file è aperto e l'ultima operazione era una open da parte dello stesso client,
                    // allora posso scrivere
                    if (to_write->is_open)
                    {
                        if (to_write->last_op == OPENFILE && to_write->client_descriptor == request.client_descriptor)
                        {
                            strcpy(to_write->content, request.content);
                            to_write->last_op = WRITEFILE;
                        }
                        else
                            to_send = INVALID_LAST_OPERATION;
                    }
                    else
                        to_send = NOT_OPENED;

                    pthread_mutex_unlock(&(to_write->lock));
                }
                // Se il file non esiste nel server, lo segnalo
                else
                {
                    to_send = FILE_NOT_FOUND;
                    pthread_mutex_unlock(&files_mutex);
                }

                // Invio della risposta
                writen(request.client_descriptor, &to_send, sizeof(to_send));

                break;
            case APPENDTOFILE:
                break;
            case CLOSEFILE:
                pthread_mutex_lock(&files_mutex);
                if (hashmap_has_key(files, request.path))
                {
                    File* to_close = (File*)hashmap_get(files, request.path);
                    pthread_mutex_unlock(&files_mutex);

                    pthread_mutex_lock(&(to_close->lock));
                    // Se è aperto lo chiudo
                    if (to_close->is_open)
                    {
                        to_close->is_open = FALSE;
                        to_close->last_op = CLOSEFILE;
                    }
                    // Altrimenti segnalo quello che stavo cercando di fare
                    else
                        to_send = ALREADY_CLOSED;
                        
                    pthread_mutex_unlock(&(to_close->lock));
                }
                else
                {
                    to_send = FILE_NOT_FOUND;
                    pthread_mutex_unlock(&files_mutex);
                }

                // Invio la risposta
                writen(request.client_descriptor, &to_send, sizeof(int));
                    
                break;
            case CLOSECONNECTION:;
                // Descrittore in formato di stringa per poterlo rimuovere
                char desc_string[20];
                sprintf(desc_string, "%d", request.client_descriptor);

                // Rimuovo il descrittore dal set
                pthread_mutex_lock(&desc_set_lock);
                FD_CLR(request.client_descriptor, &desc_set);
                pthread_mutex_unlock(&desc_set_lock);

                // Rimuovo il descrittore dalla lista dei descrittori attivi
                pthread_mutex_lock(&client_fds_lock);
                list_remove_by_key(&client_fds, desc_string);
                pthread_mutex_unlock(&client_fds_lock);
                
                writen(request.client_descriptor, &to_send, sizeof(to_send));
                break;
            default:
                fprintf(stderr, "Codice richiesta %d non supportato dal server\n", request.op_code);
                break;
        }

        // Resetto il valore di ritorno
        to_send = 0;
        // Una volta esaudita la richiesta, posso ricominciare ad ascoltare quel client
        pthread_mutex_lock(&desc_set_lock);
        FD_SET(request.client_descriptor, &desc_set);
        pthread_mutex_unlock(&desc_set_lock);
    }
        
    debug++;
    printf("Salve sono il thread %d e sto consumando 5 tipi di media differenti per evitare che un solo pensiero si formi nella mia testa\n", debug);
    
    return NULL;
}

void* dispatcher(void* args)
{
    // Read set, è locale e serve solo al dispatcher
    fd_set read_set;

    while (TRUE)
    {
        FD_ZERO(&read_set);
        // Salvo il numero massimo dei fd
        pthread_mutex_lock(&max_fd_lock);
        int loc_max_fd = max_fd;
        pthread_mutex_unlock(&max_fd_lock);

        if (loc_max_fd > 0)
        {
            // Copio il set totale
            pthread_mutex_lock(&desc_set_lock);
            read_set = desc_set;
            pthread_mutex_unlock(&desc_set_lock);

            // Calcolo il numero attuale di connessioni attive
            pthread_mutex_lock(&client_fds_lock);
            int list_len = client_fds.length;
            pthread_mutex_unlock(&client_fds_lock);
            
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
                    pthread_mutex_lock(&client_fds_lock);
                    // Ottengo il fd corrente
                    void* res = list_get(client_fds, i);
                    if (res == NULL)
                    {
                        pthread_mutex_unlock(&client_fds_lock);
                        continue;
                    }
                        
                    int curr_fd = *((int*)res);
                    pthread_mutex_unlock(&client_fds_lock);          

                    // Controllo che sia settato e che abbia qualcosa da leggere
                    if (curr_fd <= loc_max_fd && FD_ISSET(curr_fd, &read_set))
                    {
                        // Puntatore per salvare la richiesta: è allocata sullo heap perché altrimenti
                        // la lista ne perderebbe traccia una volta usciti da questa funzione
                        ClientRequest* request = malloc(sizeof(ClientRequest));
                        // Dimensione dei dati letti per controllare errori
                        int read_size = readn(curr_fd, request, sizeof(*request));

                        printf("Letti: %d\n", read_size);

                        if (read_size == -1)
                            perror("Errore nella lettura della richiesta del client");
                        else
                        {
                            // Aggiungo il file descriptor del client alla richiesta ricevuta
                            request->client_descriptor = curr_fd;
                            // Impedisco che la select possa leggere duplicati nelle richieste
                            pthread_mutex_lock(&desc_set_lock);
                            FD_CLR(curr_fd, &desc_set);
                            pthread_mutex_unlock(&desc_set_lock);
                            // La aggiungo alla coda delle richieste
                            pthread_mutex_lock(&queue_mutex);
                            list_enqueue(&requests, request, NULL);

                            // Segnalo che la coda delle richieste ha un elemento da elaborare
                            pthread_cond_signal(&queue_not_empty);
                            pthread_mutex_unlock(&queue_mutex);
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
    // Informazioni del client
    struct sockaddr client_info;
    socklen_t client_addr_length = sizeof(client_info);
    // Dati dei nodi delle lsite
    int* client_fd = malloc(sizeof(int));
    char* key = malloc(sizeof(char) * 20);

    while (TRUE)
    {
        // Attendo una richiesta di connessione
        if ((*client_fd = accept(socket_desc, &client_info, &client_addr_length)) > 0)
        {
            // Uso come chiave l'fd del thread
            sprintf(key, "%d", *client_fd);

            // Aggiungo l'fd alla lista, accedo in mutua esclusione perché potrei ricevere una 
            // richiesta di disconnessione che modifica la lista
            pthread_mutex_lock(&client_fds_lock);
            list_push(&client_fds, client_fd, key);
            pthread_mutex_unlock(&client_fds_lock);

            // Aggiungo il descrittore al read set
            pthread_mutex_lock(&desc_set_lock);
            FD_SET(*client_fd, &desc_set);
            pthread_mutex_unlock(&desc_set_lock);

            // Aggiorno il massimo fd se necessario
            pthread_mutex_lock(&max_fd_lock);
            if (max_fd < *client_fd)
                max_fd = *client_fd;
            pthread_mutex_unlock(&max_fd_lock);

            // Rialloco così i dati puntano a una locazione differente
            client_fd = malloc(sizeof(int));
            key = malloc(sizeof(char) * 20);
        }
    }

    return NULL;
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
        closedir(logs);
        sprintf(log_path, "%s/%s", config.log_path, log_name);
    }

    // Creo e apro un file chiamato come la data attuale
    log_file = fopen(log_path, "w");

    return 0;
}

int initialize_socket()
{
    // Pulisco eventuali socket precedenti
    cleanup(config.socket_name);
    // Registro il cleanup come evento di uscita
    atexit(cleanup);

    // Creo il socket
    int socket_desc = socket(AF_UNIX, SOCK_STREAM, 0);
    // Indirizzo del socket
    struct sockaddr_un socket_addr;

    // Copia dell'indirizzo
    memset(&socket_addr, '0', sizeof(socket_addr));
    strncpy(socket_addr.sun_path, config.socket_name, strlen(config.socket_name) + 1);
    socket_addr.sun_family = AF_UNIX;

    // Binding del socket
    if (bind(socket_desc, (struct sockaddr*) &socket_addr, (socklen_t)sizeof(socket_addr)) != 0)
    {
        perror("Impossibile collegare il socket all'indirizzo");
        exit(EXIT_FAILURE);
    }

    // Mi metto in ascolto
    listen(socket_desc, MAX_CONNECTION_QUEUE_SIZE);

    // Ritorno l'fd del server
    return socket_desc;
}

void cleanup()
{
    unlink(config.socket_name);

    if (log_file != NULL)
        fclose(log_file);
}

ServerConfig config_server()
{
    ServerConfig ret;
    FILE* config_file;
    char line_buffer[PATH_MAX];
    
    ret.n_workers = -1;
    ret.tot_space = -1;
    ret.max_files = -1;
    ret.socket_name[0] = '\0';
    ret.log_path[0] = '\0';

    config_file = fopen("config.txt", "r");

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

void print_request_node(Node* to_print)
{
    ClientRequest r = *(ClientRequest*)to_print->data;
    
    printf("Op: %d\nContent:%s\n\n", r.op_code, r.content);
}