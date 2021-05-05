#include <stdio.h>
#include <stdlib.h>

typedef struct node 
{
    struct node* prev;
    struct node* next;

    void* data;
}Node;

Node* create_node(void* to_set)
{
    Node* node = malloc(sizeof(Node));

    node->prev = NULL;
    node->next = NULL;
    node->data = to_set;

    return node;
}

void clean_node(Node* to_clean)
{
    free(to_clean);
}