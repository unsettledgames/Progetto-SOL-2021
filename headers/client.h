#if !defined(CLIENT_H_)

#define CLIENT_H_

#define OPT_NAME_LENGTH     5
#define OPT_VALUE_LENGTH    100

#define INPUT_TYPE_ERROR            -1
#define INCONSISTENT_INPUT_ERROR    -2
#define NAN_INPUT_ERROR             -3
#define INVALID_NUMBER_INPUT_ERROR  -4

#define MISSING_SOCKET_NAME     -1

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include "nodes.h"
#include "list.h"
#include "hashmap.h"

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

int execute_requests(List* requests);

void print_client_options();

void print_node_request(Node* node);

void print_node_string(Node* to_print);

void clean_request_node(Node* node);

#endif