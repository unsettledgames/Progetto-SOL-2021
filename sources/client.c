#include <stdio.h>
#include <stdlib.h>

#include "nodes.h"
#include "list.h"

int main(int argc, char** argv)
{
    int a = 4;
    Node* nodo = create_node((void*)&a, "sas");
    List lista;

    printf("%d\n", *(int*)nodo->data);
    printf("Tutto ok?\n");
    return 0;
}