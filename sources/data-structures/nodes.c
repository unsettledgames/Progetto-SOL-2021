
#include "nodes.h"

Node* create_node(void* to_set)
{
    Node* node = malloc(sizeof(Node));

    node->prev = NULL;
    node->next = NULL;
    node->data = to_set;
    node->key = NULL;

    return node;
}

Node* create_hash_node(void* to_set, const char* key)
{
    Node* node = create_node(to_set);
    node->key = key;

    return node;
}

void clean_node(Node* to_clean)
{
    free(to_clean);
}