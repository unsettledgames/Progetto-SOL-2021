#if !defined(UTILITY_H_)

#define UTILITY_H_

#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "consts.h"
#include "errors.h"

#define SYSCALL_EXIT(name, r, sc, str, ...)	\
    if ((r=sc) == -1) {				\
        perror(#name);				\
        int errno_copy = errno;			\
        print_error(str, __VA_ARGS__);		\
        exit(errno_copy);			\
    }


#define SYSCALL_PRINT(name, r, sc, str, ...)	\
    if ((r=sc) == -1) {				\
        perror(#name);				\
        int errno_copy = errno;			\
        print_error(str, __VA_ARGS__);		\
        errno = errno_copy;			\
    }

#define SYSCALL_RETURN(name, r, sc, str, ...)	\
    if ((r=sc) < 0) {				\
        perror(#name);				\
        int errno_copy = errno;			\
        print_error(str, __VA_ARGS__);		\
        errno = errno_copy;			\
        return -1;                               \
    }


#define LOCK(l)      if (pthread_mutex_lock(l)!=0)        { \
    fprintf(stderr, "ERRORE FATALE lock\n");		    \
    pthread_exit((void*)EXIT_FAILURE);			    \
}   

#define UNLOCK(l)    if (pthread_mutex_unlock(l)!=0)      { \
    fprintf(stderr, "ERRORE FATALE unlock\n");		    \
    pthread_exit((void*)EXIT_FAILURE);				    \
}

#define WAIT(c,l)    if (pthread_cond_wait(c,l)!=0)       { \
    fprintf(stderr, "ERRORE FATALE wait\n");		    \
    pthread_exit((void*)EXIT_FAILURE);				    \
}

#define SIGNAL(c)    if (pthread_cond_signal(c)!=0)       { \
    fprintf(stderr, "ERRORE FATALE signal\n");		    \
    pthread_exit((void*)EXIT_FAILURE);				    \
}

#define THREAD_KILL(t, s)    if (pthread_kill(t, s)!=0)       { \
    fprintf(stderr, "ERRORE FATALE kill\n");		    \
    pthread_exit((void*)EXIT_FAILURE);				    \
}

#define THREAD_JOIN(t, v)    if (pthread_join(t, v)!=0)       { \
    fprintf(stderr, "ERRORE FATALE join\n");		    \
    pthread_exit((void*)EXIT_FAILURE);				    \
}

#define SERVER_OP(op, cc) if (op < 0) cc

#define CALL_ZLIB(x) {                                                  \
        int status;                                                     \
        status = x;                                                     \
        if (status < 0) {                                               \
            fprintf (stderr,                                            \
                     "%s:%d: %s returned a bad status of %d.\n",        \
                     __FILE__, __LINE__, #x, status);                   \
            exit (EXIT_FAILURE);                                        \
        }                                                               \
    }

/**
    \brief: Converte la stringa string in un intero: se positive_constraint, allora si verifica anche che
            il numero convertito sia positivo.
    
    \param string: Stringa da convertire in intero.
    \param positive_constraint: Indica se si deve verificare che il numero sia positivo o meno.

    \return: Il numero convertito. In caso di errore, si modifica errno.
*/
int string_to_int(char* string, int positive_constraint);

/**
    \brief: Ritorna il path assoluto dato il path relativo.

    \param relative_path: Path relativo da convertire in assoluto.
    
    \return: Il path assoluto corrispondente al path relativo passato come parametro.
*/
char* get_absolute_path(char* relative_path);

/**
    \brief: Wrapper per la malloc (si verifica che non restituisca errore), a cui si aggiunge anche una
            memset dell'oggetto creato a 0 in modo da evitare scritture, letture o salti su campi non
            inizializzati. Si esce immediatamente dal programma in caso la malloc fallisca (si assume che la
            memoria sia corrotta e che sia quindi impossibile proseguire nell'esecuzione).
    
    \param size: Dimensione dell'area da allocare.
*/
void* my_malloc(size_t size);

/**
    \brief: Invia n bytes dell'oggetto puntato da ptr al socket con descrittore fd.

    \param fd: Descrittore del socket a cui inviare i bytes.
    \param ptr: Puntatore al buffer da inviare.
    \param n: Numero di bytes da scrivere.

    \return: Numero di bytes scritti, -1 in caso di errore.
*/
ssize_t writen(int fd, void *ptr, size_t n);

/**
    \brief: Legge n bytes nell'oggetto puntato da ptr dal socket con descrittore fd.

    \param fd: Descrittore del socket da cui leggere i bytes.
    \param ptr: Puntatore al buffer da usare per salvare i dati letti.
    \param n: Numero di bytes da leggere.

    \return: Numero di bytes letti, -1 in caso di errore.
*/
ssize_t readn(int fd, void *ptr, size_t n);

/**
    \brief: Sostituisce alla stringa str tutte le occorrenze di find, rimpiazzandole con replace.
    
    \param str: La stringa oggetto della sostituzione.
    \param find: Il carattere da sostituire
    \param replace: Il carattere che prenderà il posto di tutte le occorrenze di find.

    \return: La stringa modificata.
*/
char* replace_char(char* str, char find, char replace);

/**
    \brief: Crea una cartella chiamata dirname se già non esiste nel file system.

    \param dirname: Il nome della cartella da creare se non esiste.
    
    \return: 0 in caso di successo, CREATE_DIR_ERROR se non è stato possibile creare la cartella.
*/
int create_dir_if_not_exists(const char* dirname);


static inline void print_error(const char * str, ...) 
{
    const char err[]="ERROR: ";
    va_list argp;
    char * p=(char *)malloc(strlen(str)+strlen(err)+EXTRA_LEN_PRINT_ERROR);
    if (!p) {
	perror("malloc");
        fprintf(stderr,"FATAL ERROR nella funzione 'print_error'\n");
        return;
    }
    strcpy(p,err);
    strcpy(p+strlen(err), str);
    va_start(argp, str);
    vfprintf(stderr, p, argp);
    va_end(argp);
    free(p);
}

int get_file_size(char* path, int max_size);

#endif