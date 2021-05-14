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

    perror("Impossibile connettersi al server: ");
    exit(EXIT_FAILURE);
}

int closeConnection(const char* sockname)
{
    return 0;
}

int openFile(const char* pathname, int flags)
{
    ClientRequest to_send;
    time_t timestamp;
    int reply = 0;

    time(&timestamp);

    //strcpy(to_send.content, pathname);    
    to_send.content_size = strlen(pathname);

    to_send.flags = flags;
    to_send.timestamp = timestamp;
    to_send.op_code = OPENFILE;

    write(socket_fd, &to_send, sizeof(to_send));
    read(socket_fd, &reply, sizeof(reply));

    printf("Risposta alla open: %d\n", reply);

    return reply;
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

int readFile(const char* pathname, void** buf, size_t* size)
{
    // Invio la richiesta
    // Ricevo la risposta
    // Scrivo il file nella cartella passata da linea di comando se necessario
    // Termino
    return 0;
}

int readNFiles(int n, const char* dirname)
{
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