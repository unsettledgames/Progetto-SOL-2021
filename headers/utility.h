#if !defined(UTILITY_H_)

#define UTILITY_H_

#include <errno.h>
#include <stdlib.h>

#define NAN_ERROR       -1
#define INVALID_NUMBER  -2

int string_to_int(char* string, int positive_constraint);

#endif