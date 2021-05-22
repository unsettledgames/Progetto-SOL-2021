#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <sys/select.h>
#include <pthread.h>
#include <signal.h>

#include "nodes.h"
#include "list.h"
#include "hashmap.h"
#include "utility.h"
#include "errors.h"
#include "errno.h"
#include "api.h"

#define MAX_CONNECTION_QUEUE_SIZE   512

typedef struct file 
{
    char content[MAX_FILE_SIZE];
    char path[MAX_PATH_LENGTH];
    
    pthread_mutex_t lock;

    int client_descriptor;
    int last_op;
    int is_open;
    int content_size;
    int modified;

    time_t last_used;
}File;

typedef struct serverconfig
{
    unsigned int n_workers;
    unsigned int tot_space;
    unsigned int max_files;

    char socket_name[PATH_MAX];
    char log_path[MAX_PATH_LENGTH];
}ServerConfig;

ServerConfig config_server(const char* file_name);

void* connession_handler(void* args);

void* dispatcher(void* args);

void* worker(void* args);

int initialize_socket();

int create_log();

void cleanup();

void print_request_node(Node* to_print);

void print_file_node(Node* to_print);

char* get_LRU(char* current_path);

void* sighandler(void* param);

void clean_everything();

void log_info(const char* fmt, ...);