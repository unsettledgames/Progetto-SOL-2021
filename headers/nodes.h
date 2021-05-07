#if !defined(NODES_H_)
#define NODES_H_

#include <stdio.h>
#include <stdlib.h>
#include "consts.h"

typedef struct node 
{
    struct node* prev;
    struct node* next;

    const char* key;
    void* data;
}Node;

Node* create_node(void* to_set, const char* key);

void clean_node(Node* to_clean);

#endif