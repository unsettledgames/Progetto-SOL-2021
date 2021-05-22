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

void flog(void (*log_function)(const char*), const char* fmt, ...);

int string_to_int(char* string, int positive_constraint);

char* get_absolute_path(char* relative_path);

void* my_malloc(size_t size);

ssize_t writen(int fd, void *ptr, size_t n);

ssize_t readn(int fd, void *ptr, size_t n);

char* replace_char(char* str, char find, char replace);

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


#endif