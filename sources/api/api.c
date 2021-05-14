#include "api.h"


int openConnection(const char* sockname, int msec, const struct timespec abstime)
{
    // Creo il socket
    int socket_desc = socket(AF_UNIX, SOCK_STREAM, 0);
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
    while ((err = connect(socket_desc, (struct sockaddr*) &address, sizeof(address))) != 0 && 
        abstime.tv_sec < curr_time.tv_sec)
    {
        printf("Provo a connettermi...\n");
        sleep(msec);
        clock_gettime(CLOCK_REALTIME, &curr_time);
    }

    if (err != -1)
    {
        printf("Connesso\n");
        return 0;
    }

    perror("Impossibile connettersi al server: ");
    exit(EXIT_FAILURE);
}

int closeConnection(const char* sockname)
{
    return 0;
}

int openFile(const char* pathname, int flags)
{
    return 0;
}

int readFile(const char* pathname, void** buf, size_t* size)
{
    // Apro il file nel server?
    // Invio la richiesta
    // Ricevo la risposta
    // Chiudo il file?
    // Scrivo il file nella cartella passata da linea di comando se necessario
    // Termino
    return 0;
}

int writeFile(const char* pathname, const char* dirname)
{
    // Apro il file nel server?
    // Invio i dati 
    // Ricevo i file espulsi
    // Chiudo il file?
    // Scrivo i file espulsi nella cartella passata da linea di comando se necessario
    // Termino
    return 0;
}

int appendToFile(const char* pathname, void* buf, size_t size, const char* dirname)
{
    return 0;
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
    return 0;
}

int removeFile(const char* pathname)
{
    return 0;
}