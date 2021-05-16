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

char* get_absolute_path(char* relative_path)
{
    char buffer[PATH_MAX];

    return realpath(relative_path, buffer);
}

void* my_malloc(size_t size)
{
    void* ret = malloc(size);
    
    if (ret == NULL)
    {
        perror("Errore di allocazione della memoria: ");
        exit(EXIT_FAILURE);
    }

    return ret;
}

/* Read "n" bytes from a descriptor */
ssize_t readn(int fd, void *ptr, size_t n) 
{  
   size_t   nleft;
   ssize_t  nread;
 
   nleft = n;
   while (nleft > 0) {
     if((nread = read(fd, ptr, nleft)) < 0) {
        if (nleft == n) return -1; /* error, return -1 */
        else break; /* error, return amount read so far */
     } else if (nread == 0) break; /* EOF */
     nleft -= nread;
     ptr   += nread;
   }
   return(n - nleft); /* return >= 0 */
}
 
/* Write "n" bytes to a descriptor */
ssize_t writen(int fd, void *ptr, size_t n) 
{  
   size_t   nleft;
   ssize_t  nwritten;
 
   nleft = n;
   while (nleft > 0) {
     if((nwritten = write(fd, ptr, nleft)) < 0) {
        if (nleft == n) return -1; /* error, return -1 */
        else break; /* error, return amount written so far */
     } else if (nwritten == 0) break; 
     nleft -= nwritten;
     ptr   += nwritten;
   }
   return(n - nleft); /* return >= 0 */
}