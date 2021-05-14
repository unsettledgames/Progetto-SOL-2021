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

#define MAX_REQUESTCONTENT_SIZE 40000

enum Operations 
{
    OPENFILE =      0, 
    READFILE =      1, 
    WRITEFILE =     2, 
    APPENDTOFILE =  3, 
    CLOSEFILE =     4
};

typedef struct clientrequest
{
    char content[MAX_REQUESTCONTENT_SIZE];
    unsigned int content_size;

    int flags;
    time_t timestamp;
    unsigned short op_code;

    int client_descriptor;
}ClientRequest;

int openConnection(const char* sockname, int msec, const struct timespec abstime);

int closeConnection(const char* sockname);

int openFile(const char* pathname, int flags);

int readFile(const char* pathname, void** buf, size_t* size);

int writeFile(const char* pathname, const char* dirname);

int appendToFile(const char* pathname, void* buf, size_t size, const char* dirname);

int readNFiles(int n, const char* dirname);

int lockFile(const char* pathname);

int unlockFile(const char* pathname);

int closeFile(const char* pathname);

int removeFile(const char* pathname);

#endif