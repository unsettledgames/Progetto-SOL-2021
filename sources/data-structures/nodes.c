
#include "nodes.h"

Node* create_node(void* to_set, const char* key)
{
    Node* node = malloc(sizeof(Node));

    node->prev = NULL;
    node->next = NULL;
    node->data = to_set;
    node->key = key;

    return node;
}

void clean_node(Node* to_clean)
{
    if (to_clean->data != NULL)
        free(to_clean->data);
    if (to_clean->key != NULL)
        free((void*)to_clean->key);

    free(to_clean);
}