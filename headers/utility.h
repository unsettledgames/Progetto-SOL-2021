#if !defined(UTILITY_H_)

#define UTILITY_H_

#include <errno.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>

#include "errors.h"

int string_to_int(char* string, int positive_constraint);

char* get_absolute_path(char* relative_path);

void* _malloc(size_t size);


#endif