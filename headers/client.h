#if !defined(CLIENT_H_)

#define CLIENT_H_

#define OPT_NAME_LENGTH     5
#define OPT_VALUE_LENGTH    100

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "nodes.h"
#include "list.h"
#include "hashmap.h"
#include "utility.h"
#include "errors.h"

typedef struct request
{
    char code;
    char* arguments;
}Request;

typedef struct clientconfig
{
    char* socket_name;
    char* expelled_dir;
    char* read_dir;
    unsigned int request_rate;
    unsigned int print_op_data;
}ClientConfig;

int parse_options(Hashmap* config, List* requests, int n_args, char** args);

int validate_input(Hashmap config, List requests);

ClientConfig initialize_client(Hashmap config);

int connect_to_server();

int execute_requests(ClientConfig config, List* requests);

char** parse_request_arguments(char* args);

void print_client_options();

void print_client_config(ClientConfig c);

void print_node_request(Node* node);

void print_node_string(Node* to_print);

void clean_request_node(Node* node);

void clean_client(Hashmap config, List requests);

int is_file(const char* path);

int send_from_dir(const char* dirpath, int* n_files, const char* write_dir);

// API
int openConnection(const char* sockname, int msec, const struct timespec abstime);

int closeConnection(const char* sockname);

int openFile(const char* pathname, int flags);

int readFile(const char* pathname, void** buf, size_t* size);

int writeFile(const char* pathname, const char* dirname);

int appendToFile(const char* pathname, void* buf, size_t size, const char* dirname);

int lockFile(const char* pathname);

int unlockFile(const char* pathname);

int closeFile(const char* pathname);

int removeFile(const char* pathname);

#endif