#if !defined(_API_H_)

#define _API_H_

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>

#include "errors.h"
#include "consts.h"
#include "utility.h"

#define O_CREATE    1
#define O_LOCK      2


/**
    PROTOCOLLO DI COMUNICAZIONE TRA CLIENT E SERVER

    E' stata definita una struttura ClientRequest, che contiene i campi necessari alla corretta elaborazione delle
    richieste. I campi hanno ovvia interpretazione, ma è interessante soffermarsi sul fatto che il campo
    client_descriptor venga impostato in modo sensato solamente dal server al fine di differenziare le richieste
    che riceve: il client imposta client_descriptor a un valore negativo così da denotare il fatto che tale
    attributo è rimasto non inizializzato.

    Ove indicato, alcune APIs ritorneranno dei valori che potranno essere interpretati e gestiti appropriatamente dai client.

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

enum Operations
{
    OPENFILE =          0, 
    READFILE =          1, 
    WRITEFILE =         2, 
    APPENDTOFILE =      3, 
    CLOSEFILE =         4,
    CLOSECONNECTION =   5,
    PARTIALREAD =       6
};

/**
    \brief: Struttura usata per rappresentare una richiesta da parte del client.

    \param path: Path del file oggetto della richiesta.
    \param content: Contenuto del file.
    \param content_size: Dimensione (in formato non compresso) del contenuto del file.
    \param flags: Usato per implementare O_CREATE e O_LOCK.
    \param timestamp: Timestamp dell'invio della richiesta
*/
typedef struct clientrequest
{
    char path[MAX_PATH_LENGTH];
    char content[MAX_FILE_SIZE];
    unsigned int content_size;

    int flags;
    unsigned short op_code;

    int client_descriptor;
}ClientRequest;

/**
    \brief: Struttura usata per rappresentare una risposta da parte del server (usata in quei casi in cui
            mandare un semplice codice di errore non è sufficiente, come nel caso della writeFile, che causa
            l'espulsione di file).
    
    \param path: Percorso del file ritornato (perché letto o espulso).
    \param content: Contenuto del file ritornato (in formato decompresso).
    \param content_size: Dimensione del file ritornato (in formato decompresso).
    \param error_code: Codice di errore, restituito in caso il server abbia avuto problemi a esaudire la richiesta
            del server (sostituisce il codice di errore normalmente ritornato dal server).
*/
typedef struct serverresponse
{
    char path[MAX_PATH_LENGTH];
    char content[MAX_FILE_SIZE];
    unsigned int content_size;
    unsigned int error_code;
}ServerResponse;

/**
    \brief: Apre una connessione verso il server.

    \param sockname: Il nome del socket a cui connettersi.
    \param msec: Il numero di millisecondi da aspettare tra un tentativo di connessione e l'altro.
    \param abstime: Tempo assoluto oltre il quale si smette di provare a connettersi.

    \return: 0 se la connessione è avvenuta con successo, CONNECTION_TIMEOUT altrimenti.
*/
int openConnection(const char* sockname, int msec, const struct timespec abstime);

/**
    \brief: Chiude la connessione verso il server.

    \param sockname: Il nome del socket da cui disconnettersi.

    \return: 0 se è stato possibile disconnettersi, -1 altrimenti (errno viene modificato).
*/
int closeConnection(const char* sockname);

/**
    \brief: Richiede l'apertura di un file e gestisce eventuali file espulsi dal server.

    \param pathname: Il path relativo del file da aprire. La funzione lo converte in un path assoluto prima di
            spedire la richiesta al server.
    \param flags: Flag di apertura del file (O_CREATE, O_LOCK)

    \return: 0 se è stato possibile aprire il file, -1 altrimenti (errno viene modificato). Viene ritornato 
            -1 e viene modificato errno anche se ci sono stati problemi nella gestione dei file espulsi.
*/
int openFile(const char* pathname, int flags);

/**
    \brief: Richiede la lettura di un file e salva il numero di bytes letti in size.

    \param pathname: Il path relativo del file da leggere. La funzione lo converte in un path assoluto prima di
            spedire la richiesta al server.
    \param buf: Il buffer usato per salvare il contenuto del file da leggere.
    \param size: Puntatore al numero di bytes letti.

    \return: 0 se è stato possibile leggere il file, -1 altrimenti (errno viene modificato).
*/
int readFile(const char* pathname, void** buf, size_t* size);

/**
    \brief: Richiede la scrittura di un file.

    \param pathname: Il path relativo del file da inviare al server. La funzione lo converte in un path assoluto 
            prima di spedire la richiesta.
    \param dirname: Nome della cartella in cui salvare eventuali file espulsi.

    \return: 0 se è stato possibile scrivere il file, -1 altrimenti (errno viene modificato). Viene ritornato 
            -1 e viene modificato errno anche se ci sono stati problemi nella gestione dei file espulsi.
*/
int writeFile(const char* pathname, const char* dirname);

/**
    \brief: Appende buf al file pathname memorizzato sul server. Gestisce eventuali file espulsi dal server.

    \param pathname: Il path relativo del file da inviare al server. La funzione lo converte in un path assoluto 
            prima di spedire la richiesta.
    \param dirname: Nome della cartella in cui salvare eventuali file espulsi.

    \return: 0 se è stato possibile scrivere il file, -1 altrimenti (errno viene modificato). Viene ritornato 
            -1 e viene modificato errno anche se ci sono stati problemi nella gestione dei file espulsi.
*/
int appendToFile(const char* pathname, void* buf, size_t size, const char* dirname);

/**
    \brief: Legge n files dal server, se n < 0 allora li legge tutti. I file letti vengono gestiti e salvati 
            nella cartella dirname se dirname != NULL.

    \param n: Numero di files da leggere dal server: se è minore di 0, allora vengono letti tutti.
    \param dirname: Nome della cartella in cui salvare eventuali file espulsi.

    \return: 0 se è stato possibile eggere i files, -1 altrimenti (errno viene modificato). Viene ritornato 
            -1 e viene modificato errno anche se ci sono stati problemi nella gestione dei file letti.
*/
int readNFiles(int n, const char* dirname);

/**
    \brief: Chiude il file con path pathname salvato nel server. Pathname viene convertito in path assoluto
            prima di inviare la richiesta.
    
    \param pathname: Il path del file da chiudere.

    \return: 0 se la chiusura è avvenuta con successo, -1 altrimenti (in tal caso errno viene modificato).
*/
int closeFile(const char* pathname);

// Non implementata
int lockFile(const char* pathname);

// Non implementata
int unlockFile(const char* pathname);

// Non implementata
int removeFile(const char* pathname);

#endif