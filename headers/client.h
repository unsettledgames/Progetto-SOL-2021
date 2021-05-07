#if !defined(CLIENT_H_)

#define CLIENT_H_

#define OPT_NAME_LENGTH     5
#define OPT_VALUE_LENGTH    100

#define INPUT_TYPE_ERROR            -1
#define INCONSISTENT_INPUT_ERROR    -2

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "nodes.h"
#include "list.h"
#include "hashmap.h"

typedef struct request
{
    char code;
    char* arguments;
}Request;

int parse_options(Hashmap* config, List* requests, int n_args, char** args);

int validate_input(Hashmap config, List requests);

int initialize_client(Hashmap config);

void print_client_options();

void print_node_request(Node* node);

void print_node_string(Node* to_print);

#endif