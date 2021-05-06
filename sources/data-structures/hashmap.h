#pragma once

#include <stdio.h>
#include <stdlib.h>

typedef struct hashmap
{
    List* lists;
    int size;
}Hashmap;