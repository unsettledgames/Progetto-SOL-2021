#include <stdio.h>
#include <stdlib.h>

typedef struct node 
{
    struct node* prev;
    struct node* next;

    const char* key;
    void* data;
}Node;

Node* create_node(void* to_set);

Node* create_hash_node(void* to_set, const char* key);

void clean_node(Node* to_clean);