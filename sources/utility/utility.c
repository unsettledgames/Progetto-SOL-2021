#include "utility.h"

int get_file_size(char* path, int max_size)
{
    FILE* f = fopen(path, "rb");
    char* buf = malloc(max_size);
    int read;

    if (f != NULL)
    {
        read = fread(buf, sizeof(char), max_size, f);
        fclose(f);
    }
    else
        read = -1;
    
    free(buf);
    return read;
}

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
    return realpath(relative_path, NULL);
}

void* my_malloc(size_t size)
{
    void* ret = malloc(size);
    
    // Gestisco l'errore
    if (ret == NULL)
    {
        perror("Errore di allocazione della memoria: ");
        exit(EXIT_FAILURE);
    }

    // Resetto la memoria allo stesso tempo
    memset(ret, 0, size);
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

char* replace_char(char* str, char find, char replace) 
{
    char *current_pos = strchr(str,find);
    while (current_pos) 
    {
        *current_pos = replace;
        current_pos = strchr(current_pos,find);
    }
    return str;
}

int create_dir_if_not_exists(const char* dirname)
{
    // Controllo che la cartella esista
    errno = 0;
    DIR* my_dir = opendir(dirname);

    // Se non esiste la creo
    if (!my_dir || (errno == ENOENT))
    {
        if (mkdir(dirname, 0777) != 0)
            return CREATE_DIR_ERROR;
    }
    else
        closedir(my_dir);
    
    return OK;
}