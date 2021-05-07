#if !defined(CLIENT_H_)

#define CLIENT_H_

#define OPT_NAME_LENGTH     5
#define OPT_VALUE_LENGTH    100

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

void parse_options(Hashmap* config, List* requests, int n_args, char** args);

void print_client_options();

void print_node_request(Node* node);

void print_node_string(Node* to_print);

#endif