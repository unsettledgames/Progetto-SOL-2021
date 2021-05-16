#include "server.h"

// File di log con la sua lock
FILE* log_file = NULL;
pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

// Coda delle richieste con la sua lock e variabile condizionale
List requests;
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t queue_empty = PTHREAD_COND_INITIALIZER;

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
    list_initialize(&requests, NULL);
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

        // Aspetto che il master finisca
        pthread_join(connession_handler_tid, NULL);
        // Aspetto che il dispatcher finisca
        pthread_join(dispatcher_tid, NULL);
    }
    else
    {
        perror("Impossibile terminare la configurazione del server.\n");
        return CONFIG_FILE_ERROR;
    }

    return 0;
}

void* dispatcher(void* args)
{
    // Read set, è locale e serve solo al dispatcher
    fd_set read_set;

    while (TRUE)
    {
        FD_ZERO(&read_set);

        // Calcolo il numero attuale di connessioni attive
        pthread_mutex_lock(&client_fds_lock);
        int list_len = client_fds.length;
        pthread_mutex_unlock(&client_fds_lock);

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
            
            // Uso la select per gestire le connessioni che necessitano di attenzione
            if (select(loc_max_fd + 1, &read_set, NULL, NULL, NULL) == -1)
            {
                perror("Errore durante la select");
                exit(EXIT_FAILURE);
            }

            // Ciclo nei set e verifico quali sono pronti
            for (int i=0; i<list_len; i++)
            {
                // Ottengo il fd corrente
                pthread_mutex_lock(&client_fds_lock);
                int curr_fd = *((int*)list_get(client_fds, i));
                pthread_mutex_unlock(&client_fds_lock);

                // Controllo che sia settato e che abbia qualcosa da leggere
                if (curr_fd <= loc_max_fd && FD_ISSET(curr_fd, &read_set))
                {
                    // Leggo dal client e aggiungo alla coda delle richieste
                    ClientRequest request;
                    int read_size = read(curr_fd, &request, sizeof(request));
                    if (read_size == -1)
                        perror("Errore nella lettura della richiesta del client");
                    else
                    {
                        printf("FD: %d\n Codice richiesta: %d\nContenuto: %s\n", curr_fd, request.op_code, request.content);
                        perror("error");
                        if (request.op_code == CLOSECONNECTION)
                        {
                            pthread_mutex_lock(&client_fds_lock);
                            list_remove_by_index(&client_fds, i);
                            pthread_mutex_unlock(&client_fds_lock);

                            int to_send = 0;
                            write(curr_fd, &to_send, sizeof(to_send));
                        }
                        else if (request.op_code == OPENFILE)
                        {
                            int to_send = 0;
                            write(curr_fd, &to_send, sizeof(to_send));
                        }
                    }
                    printf("%d e' settato\n", curr_fd);
                }
            }
        }
    }

    return NULL;
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

void* worker(void* args)
{
    // Mi metto in attesa sulla variabile condizionale
        // Prendo una richiesta, la eseguo
            // Switch case potrebbe essere sufficiente
        // Mi rimetto in attesa
        
    debug++;
    printf("Salve sono il thread %d e sto consumando 5 tipi di media differenti per evitare che un solo pensiero si formi nella mia testa\n", debug);
    
    return NULL;
}

int create_log()
{
    // Nome del file di log
    char log_name[MAX_TIME_LENGTH];
    // Path completo del file di log
    char log_path[MAX_LOGPATH_LENGTH * 2];
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
    printf("CHIAMATA\n");
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