#include "utility.h"

int string_to_int(char* string, int positive_constraint)
{   
    char* endptr = NULL;
    int result = 0;

    errno = 0;
    result = strtol(string, &endptr, 10);

    if ((result == 0 && errno != 0) || (result == 0 && endptr == string))
        errno = NAN_ERROR;
    else if (result < 0 && positive_constraint)
        errno = INVALID_NUMBER;
    
    return result;
}