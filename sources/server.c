#include "server.h"

// File di log con la sua lock
FILE* log_file;
pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

// Coda delle richieste con la sua lock
List requests;
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;

// Dimensione dello spazio allocato con la sua lock
unsigned int allocated_space = 0;
pthread_mutex_t allocated_space_mutex = PTHREAD_MUTEX_INITIALIZER;


int main(int argc, char** argv)
{
    // Parsing della configurazione del server
    ServerConfig server_config = config_server();

    if (errno == 0)
    {
        initialize_socket(server_config);
    }
    else
    {
        perror("Impossibile terminare la configurazione del server.\n");
        return CONFIG_FILE_ERROR;
    }

    // Creo il socket e il file di log se già non esiste
    // Creo una cartella tmp
    // La unlinko così quando finisco viene rimossa
    // Ci metto dentro il socket
    // Apro la cartella dei log:
        // Se non esiste già, la creo
    // Creo e apro un nuovo file di log chiamato con la data attuale

    
    return 0;
}

int initialize_socket(ServerConfig config)
{
    // Creo il socket
    int socket_desc = socket(AF_UNIX, SOCK_STREAM, 0);
    // Indirizzo del socket
    struct sockaddr_un socket_addr;
    // Informazioni del client
    struct sockaddr client_info;
    socklen_t client_addr_length = sizeof(client_info);
    int client_fd;

    // Copia dell'indirizzo
    strncpy(socket_addr.sun_path, config.socket_name, MAX_SOCKET_LEN);
    socket_addr.sun_family = AF_UNIX;

    if (bind(socket_desc, (struct sockaddr*)&socket_addr, (socklen_t)sizeof(socket_addr)) != 0)
    {
        perror("Impossibile collegare il socket all'indirizzo");
        exit(EXIT_FAILURE);
    }

    listen(socket_desc, MAX_CONNECTION_QUEUE_SIZE);

    printf("In attesa...\n");
    client_fd = accept(socket_desc, &client_info, &client_addr_length);
    printf("Collegato client %d\n", client_fd);

    return 0;
}

void* worker(void* args)
{
    return NULL;
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
        char* err = fgets(line_buffer, PATH_MAX, config_file);
        if (err != NULL)
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
        err = fgets(line_buffer, PATH_MAX, config_file);
        if (err != NULL)
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
        err = fgets(line_buffer, PATH_MAX, config_file);
        if (err != NULL)
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
        err = fgets(line_buffer, PATH_MAX, config_file);
        if (err != NULL)
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
        err = fgets(line_buffer, PATH_MAX, config_file);
        if (err != NULL)
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