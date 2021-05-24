#include "api.h"

/**
    PROTOCOLLO DI COMUNICAZIONE TRA CLIENT E SERVER

    E' stata definita una struttura ClientRequest, che contiene i campi necessari alla corretta elaborazione delle
    richieste. I campi hanno ovvia interpretazione, ma è interessante soffermarsi sul fatto che il campo
    client_descriptor venga impostato in modo sensato solamente dal server al fine di differenziare le richieste
    che riceve: il client imposta client_descriptor a un valore negativo così da denotare il fatto che tale
    attributo è rimasto non inizializzato.

    I vari tipi di errore sono definiti in api.h. Ove indicato, alcune APIs ritorneranno dei valori che potranno
    essere interpretati e gestiti appropriatamente dai client.

    E' stata definita un'enum Operations che indica quali sono i tipi di operazioni che il client è in grado di
    gestire. In tal modo il server è in grado di differenziare le richieste da parte dei client.

    openFile:   il client invia una richiesta formattata opportunamente. In particolare, op_code viene 
                impostato a OPENFILE. Il server risponde con un solo int, che esprime il successo
                o meno dell'operazione.

    closeFile:  il client invia una richiesta formattata opportunamente e riceve dal server un solo unsigned int
                che esprime il successo o meno dell'operazione.

    readFile:   la funzione formatta una richiesta in modo opportuno (op_code = READFILE) e aspetta 
                una risposta dal server.
                
                Il server invia per prima la dimensione della risposta, readFile invia un bit di acknowledgement
                e il server procede a inviare il contenuto effettivo del file.
                
                Se la dimensione della risposta è un valore negativo, si è verificato un errore.

    writeFile:  la funzione apre il file specificato dal client, occupandosi quindi di verificare la presenza
                di eventuali errori e delegandone la gestione al client. Dopodiché, writeFile estrae il contenuto
                dal file specificato e lo include in una ClientRequest che invia al server.

                Il server risponde con un solo intero che rappresenta un codice di errore.

    appendToFile: la funzione formatta una richiesta includendo nel campo "content" i dati da appendere al file.
                Il server risponde con un solo intero che rappresenta un codice di errore.

*/

static int socket_fd;


static void get_right_path(const char* path, char* buffer, int len)
{
    char* result = realpath(path, buffer);

    if (result == NULL)
        memcpy(buffer, path, len);
}

static int handle_expelled_files(int to_read, const char* dirname)
{
    int err = 0;
    char expelled_path[MAX_PATH_LENGTH + 20];
    char curr_path[MAX_PATH_LENGTH];

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
            if (dirname != NULL)
            {
                // Aggiungo 'expelled' al nome del file espulso
                strncpy(&expelled_path[1], response.path, MAX_PATH_LENGTH);
                // Evito di creare una cartella
                replace_char(expelled_path, '/', '-');

                // Scrivo nella cartella
                FILE* file = fopen(expelled_path, "wb");

                if (fwrite(response.content, sizeof(char), sizeof(response.content), file) <= 0)
                {
                    fclose(file);
                    return WRITE_FILE_ERROR;
                }
                    
                fclose(file);
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
        printf("Server irraggiungibile, ritento...\n");
        sleep(msec / 1000);
        err = 0;
        SYSCALL_RETURN("clock_gettime", err, clock_gettime(CLOCK_REALTIME, &curr_time), "Impossibile ottenere il tempo corrente.\n", "");
    }

    if (err != -1)
    {
        printf("Client connesso con successo.\n");
        return OK;
    }

    return CONNECTION_TIMEOUT;
}

int closeConnection(const char* sockname)
{
    ClientRequest to_send;
    time_t timestamp;
    int reply;
    int n_written, n_read;

    memset(&to_send, 0, sizeof(to_send));
    time(&timestamp);
    to_send.op_code = CLOSECONNECTION;
    to_send.timestamp = timestamp;

    errno = 0;
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
    time_t timestamp;
    int reply = 0;
    int n_read, n_written;

    // Ottengo il path corretto (relativo o assoluto)
    get_right_path(pathname, path, MAX_PATH_LENGTH);

    memset(&to_send, 0, sizeof(to_send));
    time(&timestamp);

    memcpy(to_send.path, path, strlen(path) + 1);
    to_send.content_size = strlen(path);

    to_send.flags = flags;
    to_send.timestamp = timestamp;
    to_send.op_code = OPENFILE;
    
    SYSCALL_RETURN("writen", n_written, writen(socket_fd, &to_send, sizeof(to_send), 
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
    char* write_buffer = my_malloc(sizeof(char) * MAX_FILE_SIZE);
    // Timestamp
    time_t timestamp;
    // return
    int ret = 0;
    // Path da utilizzare per l'invio
    char path[MAX_PATH_LENGTH];
    int n_read, n_written;

    time(&timestamp);

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
        fclose(to_read);
        return errno;
    }

    // Creo una richiesta
    ClientRequest to_send;
    memset(&to_send, 0, sizeof(to_send));
    // La imposto correttamente
    strcpy(to_send.path, path);
    strcpy(to_send.content, write_buffer);
    to_send.op_code = WRITEFILE;
    to_send.timestamp = timestamp;

    // Invio i dati
    SYSCALL_RETURN("writen", n_written, writen(socket_fd, &to_send, sizeof(to_send)), 
        "Invio della richiesta di scrittura fallito.\n", "");
    // Ricevo il numero di file espulsi
    SYSCALL_RETURN("writen", n_read, readn(socket_fd, &n_expelled, sizeof(n_expelled)), 
        "Ricezione del numero di file espulsi dalla cache fallito.\n", "");

    if (n_expelled < 0)
    {
        fprintf(stderr, "Errore lato server nell'invio dei file espulsi.\n");
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
    // Timestamp
    time_t timestamp;
    // Creo la richiesta
    ClientRequest to_send;
    // Creo la risposta
    ServerResponse to_receive;
    // Path del file
    char path[MAX_PATH_LENGTH];
    int n_read, n_written;

    get_right_path(pathname, path, MAX_PATH_LENGTH);

    memset(&to_send, 0, sizeof(to_send));
    time(&timestamp);

    to_send.timestamp = timestamp;
    strcpy(to_send.path, path);
    to_send.op_code = READFILE;

    // Invio la richiesta
    SYSCALL_RETURN("writen", n_written, writen(socket_fd, &to_send, sizeof(to_send)),
        "Errore nell'invio della richiesta di apertura del file.\n", "");
    // Ricevo la risposta
    SYSCALL_RETURN("writen", n_read, readn(socket_fd, &to_receive, sizeof(to_receive)),
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
    // Timestamp
    time_t timestamp;
    // Indica il numero di file che il server intende restituire
    int to_read;
    // Valore di ritorno
    int ret = 0;

    // Richiesta del client
    ClientRequest request;

    memset(&request, 0, sizeof(request));
    time(&timestamp);

    // Mando una richiesta
    request.flags = n;
    request.op_code = PARTIALREAD;
    request.timestamp = timestamp;
    writen(socket_fd, &request, sizeof(request));

    // Leggo quanti file devo leggere
    ret = readn(socket_fd, &to_read, sizeof(to_read));

    if (ret <= 0)
        return READ_FILE_ERROR;
    // Posso riciclare la funzione per leggere i file espulsi
    if (to_read >= 0)
        to_read = handle_expelled_files(to_read, dirname);
    return ret;
}

int appendToFile(const char* pathname, void* buf, size_t size, const char* dirname)
{
    ClientRequest to_send;
    int reply;
    time_t timestamp;
    // Path del file
    char path[MAX_PATH_LENGTH];

    get_right_path(pathname, path, MAX_PATH_LENGTH);
    memset(&to_send, 0, sizeof(to_send));

    time(&timestamp);

    // Creo la richiesta di append
    to_send.op_code = APPENDTOFILE;
    strcpy(to_send.path, path);
    strcpy(to_send.content, buf);
    to_send.content_size = size;
    to_send.timestamp = timestamp;

    // La invio
    writen(socket_fd, &to_send, sizeof(to_send));
    // Ottengo il codice di errore
    readn(socket_fd, &reply, sizeof(reply));

    if (reply < 0)
        return reply;

    reply = handle_expelled_files(reply, dirname);
    // Lo ritorno
    return reply;
}

int closeFile(const char* pathname)
{
    printf("Chiamata\n");
    // Timestamp
    time_t timestamp;
    // Richiesta
    ClientRequest to_send;
    // Risposta
    int reply = 0;
    // Path del file
    char path[MAX_PATH_LENGTH];

    get_right_path(pathname, path, MAX_PATH_LENGTH);

    memset(&to_send, 0, sizeof(to_send));

    time(&timestamp);

    to_send.op_code = CLOSEFILE;
    memcpy(to_send.path, path, strlen(path));

    // La invio
    write(socket_fd, &to_send, sizeof(to_send));
    // Ricevo la risposta
    read(socket_fd, &reply, sizeof(int));
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