
#include "nodes.h"

Node* create_node(void* to_set, const char* key)
{
    Node* node = my_malloc(sizeof(Node));

    node->next = NULL;
    node->data = to_set;
    node->key = key;

    return node;
}

void clean_node(Node* to_clean, int clean_data)
{
    if (to_clean->data != NULL && clean_data == TRUE)
        free(to_clean->data);
    if (to_clean->key != NULL)
        free((void*)to_clean->key);

    free(to_clean);
}