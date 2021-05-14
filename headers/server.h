#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <pthread.h>

#include "nodes.h"
#include "list.h"
#include "hashmap.h"
#include "utility.h"
#include "errors.h"
#include "errno.h"

#define MAX_CONNECTION_QUEUE_SIZE   512

typedef struct serverconfig
{
    unsigned int n_workers;
    unsigned int tot_space;
    unsigned int max_files;

    char socket_name[PATH_MAX];
    char log_path[PATH_MAX];
}ServerConfig;

ServerConfig config_server();

void* worker(void* args);

int initialize_socket(ServerConfig config);