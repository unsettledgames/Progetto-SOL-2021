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

int socket_fd;

int openConnection(const char* sockname, int msec, const struct timespec abstime)
{
    // Creo il socket
    socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    // Indirizzo del socket
    struct sockaddr_un address;
    // Valore di ritorno della connect
    int err = -1;
    // Struttura per tenere traccia del tempo
    struct timespec curr_time;
    clock_gettime(CLOCK_REALTIME, &curr_time);

    strncpy(address.sun_path, sockname, strlen(sockname));
    address.sun_family = AF_UNIX;

    // Connessione al server
    while ((err = connect(socket_fd, (struct sockaddr*) &address, sizeof(address))) != 0 && 
        abstime.tv_sec > curr_time.tv_sec)
    {
        printf("Server irraggiungibile, ritento...\n");
        sleep(msec / 1000);
        clock_gettime(CLOCK_REALTIME, &curr_time);
    }

    if (err != -1)
    {
        printf("Client connesso con successo.\n");
        return 0;
    }

    perror("Impossibile connettersi al server");
    exit(EXIT_FAILURE);
}

int closeConnection(const char* sockname)
{
    ClientRequest to_send;
    time_t timestamp;
    int reply;

    time(&timestamp);
    to_send.op_code = CLOSECONNECTION;
    to_send.timestamp = timestamp;

    // Invio la richiesta
    writen(socket_fd, &to_send, sizeof(to_send));
    // Ricevo la risposta
    readn(socket_fd, &reply, sizeof(reply));

    // Chiudo il socket
    close(socket_fd);

    return reply;
}

int openFile(const char* pathname, int flags)
{
    ClientRequest to_send;
    time_t timestamp;
    int reply = 0;

    time(&timestamp);

    strcpy(to_send.path, pathname);
    to_send.content_size = strlen(pathname);

    to_send.flags = flags;
    to_send.timestamp = timestamp;
    to_send.op_code = OPENFILE;

    writen(socket_fd, &to_send, sizeof(to_send));
    readn(socket_fd, &reply, sizeof(reply));

    return reply;
}

int writeFile(const char* pathname, const char* dirname)
{
    // Numero di file espulsi dalla write
    int n_expelled;
    // Buffer per il contenuto del file
    char* write_buffer = malloc(sizeof(char) * MAX_FILE_SIZE);
    // Buffer per una singola linea del file
    char line_buffer[MAX_FILE_SIZE];
    // Indica se il client è pronto a ricevere i file espulsi
    int ready = 1;
    // Timestamp
    time_t timestamp;
    time(&timestamp);

    // Pulisco il buffer
    memset(write_buffer, 0, MAX_FILE_SIZE);
    // Apro il file
    FILE* to_read = fopen(pathname, "r");

    if (to_read == NULL)
    {
        perror("Errore nell'apertura del file da inviare");
        free(write_buffer);
        return OPEN_FAILED;
    }

    // Leggo il contenuto del file
    while (fgets(line_buffer, sizeof(line_buffer), to_read) != NULL)
        strncat(write_buffer, line_buffer, strlen(line_buffer));

    // Creo una richiesta
    ClientRequest to_send;
    // La imposto correttamente
    strcpy(to_send.path, pathname);
    strcpy(to_send.content, write_buffer);
    to_send.op_code = WRITEFILE;
    to_send.timestamp = timestamp;

    // Invio i dati
    writen(socket_fd, &to_send, sizeof(to_send));
    // Ricevo il numero di file espulsi
    readn(socket_fd, &n_expelled, sizeof(n_expelled));

    printf("N espulsi: %d\n", n_expelled);

    while (n_expelled > 0)
    {
        // Ricevo un file dal server
        ServerResponse response;
        readn(socket_fd, &response, sizeof(response));

        printf("File espulso:\n%s\n%s\n\n", response.path, response.content);

        if (dirname != NULL)
        {
            // Scrivo nella cartella
        }
        else
        {
            // Stampo e basta
        }

        n_expelled--;
    }

    free(write_buffer);
    // Termino
    return n_expelled;
}

int readFile(const char* pathname, void** buf, size_t* size)
{
    // Timestamp
    time_t timestamp;
    // Creo la richiesta
    ClientRequest to_send;
    // Creo la risposta
    ServerResponse to_receive;

    time(&timestamp);

    to_send.timestamp = timestamp;
    strcpy(to_send.path, pathname);
    to_send.op_code = READFILE;

    // Invio la richiesta
    writen(socket_fd, &to_send, sizeof(to_send));
    // Ricevo la risposta
    readn(socket_fd, &to_receive, sizeof(to_receive));

    if (to_receive.error_code == OK)
    {
        memcpy(*buf, to_receive.content, *size);
        // Scrivo il file nella cartella passata da linea di comando se necessario
    }    

    // Termino
    return to_receive.error_code;
}

/**
    Molto nebulosa e ambigua.
    1) Non ho i nomi dei file che voglio
    2) Ritorna un intero, quindi devo per forza stampare nell'api
    3) Se il client non setta -p questa non dovrebbe stampare, quindi dovrei passare pure p
    4) Non avendo i nomi, non posso aprire i file e quindi contravvengo al principio per cui per leggere un 
       file dovrei prima aprirlo.

*/
int readNFiles(int n, const char* dirname)
{
    // Impostato tramite risposta dal server, indica se devo smettere di leggere
    int must_stop = FALSE;
    // Timestamp
    time_t timestamp;
    // Indica il numero di file che il server intende restituire
    int to_read;

    // Risposta del server
    ServerResponse response;
    // Richiesta del client
    ClientRequest request;

    time(&timestamp);

    // Mando una richiesta
    request.flags = n;
    request.op_code = PARTIALREAD;
    request.timestamp = timestamp;
    writen(socket_fd, &request, sizeof(request));

    // Leggo quanti file devo leggere
    readn(socket_fd, &to_read, sizeof(to_read));

    // Vado avanti finché ho da leggere
    for (int i=0; i<to_read && !must_stop; i++)
    {
        // Leggo un file
        readn(socket_fd, &response, sizeof(response));
        // Se non ho avuto errori, posso leggere
        if (response.error_code == OK)
        {
            printf("Contenuto del file %s\n", response.path);
            printf("%s\n\n", response.content);
        }
        // Altrimenti devo fermarmi
        else
            must_stop = TRUE;
    }

    return 0;
}

int appendToFile(const char* pathname, void* buf, size_t size, const char* dirname)
{
    ClientRequest to_send;
    int reply;
    time_t timestamp;

    time(&timestamp);

    // Creo la richiesta di append
    to_send.op_code = APPENDTOFILE;
    strcpy(to_send.path, pathname);
    strcpy(to_send.content, buf);
    to_send.content_size = size;
    to_send.timestamp = timestamp;

    // La invio
    writen(socket_fd, &to_send, sizeof(to_send));
    // Ottengo il codice di errore
    readn(socket_fd, &reply, sizeof(reply));

    printf("Ritorno\n");
    // Lo ritorno
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

int closeFile(const char* pathname)
{
    // Timestamp
    time_t timestamp;
    // Richiesta
    ClientRequest to_send;
    // Risposta
    int reply;

    time(&timestamp);

    to_send.op_code = CLOSEFILE;
    strcpy(to_send.path, pathname);

    // La invio
    write(socket_fd, &to_send, sizeof(to_send));
    // Ricevo la risposta
    read(socket_fd, &reply, sizeof(int));

    return reply;
}

int removeFile(const char* pathname)
{
    return 0;
}