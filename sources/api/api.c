#include "api.h"

// Descrittore del socket usato per connettersi al server
static int socket_fd;

/**
    \brief: Funzione che ritorna il "giusto" percorso di un file, ovvero se path è già un percorso assoluto, allora
            viene semplicemente copiato in buffer. Altrimenti, se è un path relativo viene prima convertito in
            path assoluto usando realpath.

    \param path: Percorso del file da convertire.
    \param buffer: Buffer usato per salvare il percorso corretto del file.
    \param len: Lunghezza di buffer.
*/
static void get_right_path(const char* path, char* buffer, int len)
{
    char* result = realpath(path, buffer);

    if (result == NULL)
        memcpy(buffer, path, len);
}

/**
    \brief: Funzione generica che si occupa di gestire i file espulsi o letti dal server. La funzione legge
            to_read risposte dal server e se dirname != NULL, salva i contenuti ritornati nella cartella chiamata
            dirname. handle_expelled_files si ferma non appena incontra un errore dal server, per evitare di 
            bloccare il client in attesa di una lettura che non avverrà mai.
    
    \param to_read: Numero di risposte da attendere da parte del server.
    \param dirname: Nome della cartella in cui salvare i contenuti dei file inviati dal server.
*/
static int handle_expelled_files(int to_read, const char* dirname)
{
    int err = 0;
    char expelled_path[MAX_PATH_LENGTH + 20];
    char curr_path[MAX_PATH_LENGTH];

    // Per indicare che sto salvando un file espulso, aggiungo una E all'inizio del nome del file.
    expelled_path[0] = 'E';

    getcwd(curr_path, MAX_PATH_LENGTH);

    if (dirname != NULL && strcmp(dirname, "") != 0 && create_dir_if_not_exists(dirname) == 0)
        // Mi sposto nella cartella
        chdir(dirname);
    else if (dirname != NULL && strcmp(dirname, "") != 0)
        return CREATE_DIR_ERROR;

    while (to_read > 0 && err >= 0)
    {
        // Ricevo un file dal server
        ServerResponse response;
        memset(&response, 0, sizeof(response));
        err = readn(socket_fd, &response, sizeof(response));

        if (err > 0 && response.error_code == OK)
        {
            if (dirname != NULL && response.content_size != 0)
            {
                // Aggiungo 'expelled' al nome del file espulso
                strncpy(&expelled_path[1], response.path, MAX_PATH_LENGTH);
                // Evito di creare una cartella
                replace_char(expelled_path, '/', '-');

                // Scrivo nella cartella
                FILE* file = fopen(expelled_path, "wb");

                if (fwrite(response.content, sizeof(char), response.content_size, file) < 0)
                {
                    SYSCALL_EXIT("fclose", err, fclose(file), "Impossibile chiudere il file", "");
                    return WRITE_FILE_ERROR;
                }
                    
                SYSCALL_EXIT("fclose", err, fclose(file), "Impossibile chiudere il file", "");
            }
        }
        else
        {
            fprintf(stderr, "E' stato impossibile espellere un file.\n");
            err = EXPELLED_FILE_FAILED;
        }

        to_read--;
    }

    // Mi risposto nella cartella originaria se dirname != NULL
    if (dirname != NULL)
        chdir(curr_path);

    if (err > 0)
        err = 0;

    return err;
}

int openConnection(const char* sockname, int msec, const struct timespec abstime)
{
    errno = 0;
    // Codice di errore
    int err = 0;
    // Creo il socket
    SYSCALL_RETURN("socket", socket_fd, socket(AF_UNIX, SOCK_STREAM, 0), "Errore creazione del socket.\n", "");
    // Indirizzo del socket
    struct sockaddr_un address;
    
    // Struttura per tenere traccia del tempo
    struct timespec curr_time;
    SYSCALL_RETURN("clock_gettime", err, clock_gettime(CLOCK_REALTIME, &curr_time), "Impossibile ottenere il tempo corrente.\n", "");

    strncpy(address.sun_path, sockname, strlen(sockname));
    address.sun_family = AF_UNIX;

    // Connessione al server
    while ((err = connect(socket_fd, (struct sockaddr*) &address, sizeof(address))) != 0 && 
        abstime.tv_sec > curr_time.tv_sec)
    {
        sleep(msec / 1000);
        err = 0;
        SYSCALL_RETURN("clock_gettime", err, clock_gettime(CLOCK_REALTIME, &curr_time), "Impossibile ottenere il tempo corrente.\n", "");
    }

    if (err != -1)
        return OK;

    errno = CONNECTION_TIMEOUT;
    return CONNECTION_TIMEOUT;
}

int closeConnection(const char* sockname)
{
    ClientRequest to_send;
    int reply;
    int n_written, n_read;

    errno = 0;

    memset(&to_send, 0, sizeof(to_send));
    to_send.op_code = CLOSECONNECTION;

    // Invio la richiesta
    SYSCALL_RETURN("writen", n_written, writen(socket_fd, &to_send, sizeof(to_send)), 
        "Impossibile terminare la connessione (writen)", "");
    // Ricevo la risposta
    SYSCALL_RETURN("readn", n_read, readn(socket_fd, &reply, sizeof(reply)), 
        "Impossibile terminare la connessione (writen)", "");

    // Chiudo il socket
    SYSCALL_RETURN("close", n_read, close(socket_fd), "Impossibile chiudere il socket", "");

    return OK;
}

int openFile(const char* pathname, int flags)
{
    // Path del file da aprire
    char path[MAX_PATH_LENGTH];
    // Richiesta
    ClientRequest to_send;
    int reply = 0;
    int n_read, n_written;

    errno = 0;

    // Ottengo il path corretto (relativo o assoluto)
    get_right_path(pathname, path, MAX_PATH_LENGTH);
    memset(&to_send, 0, sizeof(to_send));

    memcpy(to_send.path, path, strlen(path) + 1);
    to_send.content_size = strlen(path);

    to_send.flags = flags;
    to_send.op_code = OPENFILE;
    
    SYSCALL_RETURN("writen", n_written, writen(socket_fd, &to_send, sizeof(to_send)), 
        "Impossibile inviare la richiesta di apertura", "");
    SYSCALL_RETURN("readn", n_read, readn(socket_fd, &reply, sizeof(reply)), 
        "Impossibile ricevere l'esito della richiesta di apertura", "");

    return reply;
}

int writeFile(const char* pathname, const char* dirname)
{
    // Numero di file espulsi dalla write
    int n_expelled = 0;
    // Buffer per il contenuto del file
    // return
    int ret = 0;
    // Path da utilizzare per l'invio
    char path[MAX_PATH_LENGTH];
    // Buffer per il contenuto del file
    char* write_buffer = my_malloc(sizeof(char) * MAX_FILE_SIZE);
    int n_read, n_written;

    errno = 0;

    get_right_path(pathname, path, MAX_PATH_LENGTH);
    // Pulisco il buffer
    memset(write_buffer, 0, MAX_FILE_SIZE);
    // Apro il file
    FILE* to_read = fopen(path, "rb");

    if (to_read == NULL)
    {
        fprintf(stderr, "Errore nell'apertura del file da scrivere (errore %d)\n", errno);
        free(write_buffer);
        return OPEN_FAILED;
    }

    // Leggo il contenuto del file
    n_read = fread(write_buffer, sizeof(char), MAX_FILE_SIZE, to_read);
    if (n_read < 0) {
        fprintf(stderr, "Lettura del file da inviare fallita (errore %d)\n", errno);
        SYSCALL_EXIT("fclose", ret, fclose(to_read), "Impossibile chiudere il file", "");
        return errno;
    }

    SYSCALL_EXIT("fclose", ret, fclose(to_read), "Impossibile chiudere il file", "");

    // Creo una richiesta
    ClientRequest to_send;
    memset(&to_send, 0, sizeof(to_send));
    // La imposto correttamente
    strcpy(to_send.path, path);
    memcpy(to_send.content, write_buffer, n_read);
    to_send.content_size = n_read;
    to_send.op_code = WRITEFILE;

    // Invio i dati
    SYSCALL_RETURN("writen", n_written, writen(socket_fd, &to_send, sizeof(to_send)), 
        "Invio della richiesta di scrittura fallito.\n", "");
    // Ricevo il numero di file espulsi
    SYSCALL_RETURN("writen", n_read, readn(socket_fd, &n_expelled, sizeof(n_expelled)), 
        "Ricezione del numero di file espulsi dalla cache fallito.\n", "");

    if (n_expelled < 0)
    {
        fprintf(stderr, "Errore lato server nell'invio dei file espulsi. (%d)\n", n_expelled);
        return n_expelled;
    }
    // Gestisco i file espulsi
    ret = handle_expelled_files(n_expelled, dirname);

    free(write_buffer);
    // Termino
    return ret;
}

int readFile(const char* pathname, void** buf, size_t* size)
{
    // Creo la richiesta
    ClientRequest to_send;
    // Creo la risposta
    ServerResponse to_receive;
    // Path del file
    char path[MAX_PATH_LENGTH];
    int n_read, n_written;

    errno = 0;

    get_right_path(pathname, path, MAX_PATH_LENGTH);

    memset(&to_send, 0, sizeof(to_send));
    strcpy(to_send.path, path);
    to_send.op_code = READFILE;

    // Invio la richiesta
    SYSCALL_RETURN("writen", n_written, writen(socket_fd, &to_send, sizeof(to_send)),
        "Errore nell'invio della richiesta di apertura del file.\n", "");
    // Ricevo la risposta
    SYSCALL_RETURN("writen", *size, readn(socket_fd, &to_receive, sizeof(to_receive)),
        "Errore nell'invio della richiesta di apertura del file.\n", "");

    if (to_receive.error_code == OK)
        memcpy(*buf, to_receive.content, *size);
    else
    {
        fprintf(stderr, "Errore lato server nella lettura del file.\n");
        return to_receive.error_code;
    }

    // Termino
    return OK;
}

int readNFiles(int n, const char* dirname)
{
    // Indica il numero di file che il server intende restituire
    int to_read;
    // Valore di ritorno
    int ret = 0;
    int n_read, n_written;

    errno = 0;

    // Richiesta del client
    ClientRequest request;
    memset(&request, 0, sizeof(request));

    // Mando una richiesta
    request.flags = n;
    request.op_code = PARTIALREAD;

    // Invio richiesta
    SYSCALL_RETURN("writen", n_written, writen(socket_fd, &request, sizeof(request)), 
        "Errore nell'invio della richiesta di lettura di N files.\n", "");
    // Leggo quanti file devo leggere
    SYSCALL_RETURN("readn", n_read, readn(socket_fd, &to_read, sizeof(to_read)), 
        "Errore nella ricezione del numero di file ritornati.\n", "");

    if (to_read < 0)
    {
        fprintf(stderr, "Errore di lettura lato server (errore %d)\n", ret);
        return ret;
    }
    // Posso riciclare la funzione per leggere i file espulsi
    to_read = handle_expelled_files(to_read, dirname);
    
    if (to_read < 0)
        fprintf(stderr, "Errore di gestione dei file espulsi (errore %d)\n", to_read);

    return to_read;
}

int appendToFile(const char* pathname, void* buf, size_t size, const char* dirname)
{
    ClientRequest to_send;
    int reply;
    // Path del file
    char path[MAX_PATH_LENGTH];
    int n_read, n_written;

    errno = 0;

    get_right_path(pathname, path, MAX_PATH_LENGTH);
    memset(&to_send, 0, sizeof(to_send));

    // Creo la richiesta di append
    to_send.op_code = APPENDTOFILE;
    strcpy(to_send.path, path);
    memcpy(to_send.content, buf, size);
    to_send.content_size = size;

    // La invio
    SYSCALL_RETURN("writen", n_written, writen(socket_fd, &to_send, sizeof(to_send)),
        "Errore nell'invio della richiesta di append\n", "");
    // Ottengo il codice di errore
    SYSCALL_RETURN("readn", n_read, readn(socket_fd, &reply, sizeof(reply)),
        "Errore nella ricezione del risultato dell'append\n", "");

    if (reply < 0)
    {
        fprintf(stderr, "Errore lato server nell'append (codice %d)\n", reply);
        return reply;
    }

    reply = handle_expelled_files(reply, dirname);
    if (reply < 0)
        fprintf(stderr, "Errore di gestione dei file espulsi (errore %d)\n", reply);
    // Lo ritorno
    return reply;
}

int closeFile(const char* pathname)
{
    // Richiesta
    ClientRequest to_send;
    // Risposta
    int reply = 0;
    // Path del file
    char path[MAX_PATH_LENGTH];
    int n_read, n_written;

    errno = 0;

    get_right_path(pathname, path, MAX_PATH_LENGTH);
    memset(&to_send, 0, sizeof(to_send));

    to_send.op_code = CLOSEFILE;
    memcpy(to_send.path, path, strlen(path));

    // La invio
    SYSCALL_RETURN("writen", n_written, writen(socket_fd, &to_send, sizeof(to_send)), 
        "Errore nell'invio della richiesta di chiusura.\n", "");
    // Ricevo la risposta
    SYSCALL_RETURN("writen", n_read, readn(socket_fd, &reply, sizeof(int)), 
        "Errore nella ricezione dell'esito della chiusura.\n", "");

    return reply;
}

int lockFile(const char* pathname)
{
    return 0;
}

int unlockFile(const char* pathname)
{
    return 0;
}

int removeFile(const char* pathname)
{
    return 0;
}