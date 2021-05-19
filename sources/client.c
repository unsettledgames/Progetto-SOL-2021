#include "client.h"

ClientConfig client_configuration;

int main(int argc, char** argv)
{
    // Orario corrente per aggiungerci qualche secondo in modo da impostare un limite per il tempo di connessione
    struct timespec time;
    // Creo l'hashmap che contiene la configurazione del client
    Hashmap config;
    // E la coda di richieste
    List requests;

    // Inizializzo le strutture dati
    hashmap_initialize(&config, 1021, print_node_string);;
    list_initialize(&requests, print_node_request);

    // Parsing dei dati da input
    int parse_err = parse_options(&config, &requests, argc, argv);

    // Parsing e validazione dei dati avvenuta con successo
    if (parse_err == 0)
    {
        // Inizializzazione del client sulla base dei parametri di configurazione
        client_configuration = initialize_client(config);
        
        if (errno == 0)
        {
            // Ottenimento dell'orario, aggiungo 10 secondi
            clock_gettime(CLOCK_REALTIME, &time);
            time.tv_sec += 10;

            // Apertura della connessione
            openConnection(client_configuration.socket_name, 1000, time);
            // Esecuzione delle richieste
            execute_requests(client_configuration, &requests);
            // Chiusura della connessione
            closeConnection("LSOfilestorage.sk");
        }
        else
        {
            fprintf(stderr, "Errore nell'inizializzazione della configurazione del client\n");
            clean_client(config, requests);
            return errno;
        }
    }

    clean_client(config, requests);
    return 0;
}

int execute_requests(ClientConfig config, List* requests)
{
    // Debug della append
/*
    openFile("/mnt/c/Users/nicol/OneDrive/Desktop/Git projects/Progetto-SOL-2021/TestDir/file1.txt", 0);
    writeFile("/mnt/c/Users/nicol/OneDrive/Desktop/Git projects/Progetto-SOL-2021/TestDir/file1.txt", NULL);
    appendToFile("/mnt/c/Users/nicol/OneDrive/Desktop/Git projects/Progetto-SOL-2021/TestDir/file1.txt", "\nContenuto da appendere al file1\n", sizeof("\nContenuto da appendere al file1\n"), NULL);
    closeFile("/mnt/c/Users/nicol/OneDrive/Desktop/Git projects/Progetto-SOL-2021/TestDir/file1.txt");
*/
    // Finché non ho esaurito le richieste
    while (requests->head != NULL)
    {
        // Requests->head contiene la prossima richiesta da eseguire nei dati
        ArgLineRequest* curr_request = (ArgLineRequest*) list_dequeue(requests);
        // Codice operazione
        char op = curr_request->code;
        // Argomenti
        char** args = parse_request_arguments(curr_request->arguments);
        // Indice per scorrere i cicli
        int i = 0;

        // Devo fare cose diverse in base all'operazione
        switch (op)
        {
            case 'w':;
                // Primo argomento: cartella
                char* directory = args[0];
                // Secondo argomento (se c'è): numero massimo di file da inviare
                int n_files = -1;

                if (args[1] != NULL)
                {
                    n_files = string_to_int(args[1], TRUE);

                    if (errno != 0)
                    {
                        fprintf(stderr, "Errore nella conversione del secondo parametro di -w\n");
                        return errno;
                    }
                }

                if (n_files == 0)
                    n_files--;
                // Visito ricorsivamente la directory finché non ho spedito il numero corretto di file
                send_from_dir(directory, &n_files, config.expelled_dir);
                
                break;
            case 'W':;
                // Ogni file da scrivere è una stringa nell'array di argomenti
                i = 0;

                while (args[i] != NULL)
                {
                    char* real_path = get_absolute_path(args[i]);
                    int err;

                    if (real_path == NULL)
                        perror("Impossibile trovare il percorso del file");
                    else if ((err = openFile(real_path, 0)) != 0)
                        fprintf(stderr, "Impossibile scrivere il file %s, operazione annullata (errore %d).\n", real_path, err);
                    else
                    {
                        if ((err = writeFile(real_path, config.expelled_dir)) != 0)
                            fprintf(stderr, "Impossibile scrivere il file (errore %d)\n", err);
                        closeFile(real_path);
                    }

                    free(real_path);
                    
                    i++;
                }

                break;
            case 'a':;
                // Il primo argomento è il nome del file a cui appendere, il secondo è ciò che voglio appendere
                char* real_path = get_absolute_path(args[0]);
                int err;
                
                if (real_path == NULL)
                    perror("Impossibile appendere a un file non esistente");
                else if ((err = openFile(real_path, 0)) != 0)
                    perror("Errore nell'apertura del file");
                else
                {
                    err = appendToFile(real_path, args[1], strlen(args[1]), config.expelled_dir);
                    if (err != 0)
                        perror("Impossibile appendere al file");
                    closeFile(real_path);
                }

                break;
            case 'r':
                // Ogni file da leggere è una stringa nell'array di argomenti
                i = 0;

                while (args[i] != NULL)
                {
                    char write_path[MAX_PATH_LENGTH * 2];
                    char curr_path[MAX_PATH_LENGTH];

                    char* real_path = get_absolute_path(args[i]);
                    size_t n_to_read = MAX_FILE_SIZE;
                    char* file_buffer = malloc(sizeof(char) * MAX_FILE_SIZE);

                    memset(file_buffer, 0, MAX_FILE_SIZE);

                    write_path[0] = 'E';
                    getcwd(curr_path, MAX_PATH_LENGTH);

                    if (real_path == NULL)
                        return FILE_NOT_FOUND;

                    if (openFile(real_path, 0) != 0)
                        fprintf(stderr, "Impossibile aprire il file %s, operazione annullata.\n", real_path);
                    else
                    {
                        // Leggo il file dal server
                        readFile(real_path, (void**)(&file_buffer), &n_to_read);

                        // Controllo se devo scriverlo in una cartella
                        if (config.read_dir != NULL)
                        {
                            // Creo la cartella se non esiste
                            if (create_dir_if_not_exists(config.read_dir) == 0)
                            {
                                chdir(config.read_dir);
                                
                                // Aggiungo 'expelled' al nome del file espulso
                                strncpy(&write_path[1], (char*)real_path, MAX_PATH_LENGTH);
                                replace_char(write_path, '/', '-');
                                // Scrivo nella cartella
                                FILE* file = fopen(write_path, "wb");
                                printf("Path: %s\nContent:%s\n", write_path, file_buffer);
                                if (fwrite(file_buffer, sizeof(char), n_to_read, file) <= 0) 
                                {
                                    fclose(file);
                                    return WRITE_FILE_ERROR;
                                }
                                fclose(file);
                            }
                            else
                                perror("Impossibile scrivere il file letto nella cartella");
                        }

                        printf("Contenuto del file %s\n", real_path);
                        printf("%s\n\n", file_buffer);
                        closeFile(real_path);
                    }

                    free(file_buffer);
                    free(real_path);
                    i++;
                }

                break;
            case 'R':;
                // Argomento: numero di file da leggere dal server, se <0 li legge tutti
                int to_read = string_to_int(args[0], FALSE);                

                readNFiles(to_read, config.read_dir);
                free(curr_request->arguments);
                break;
            case 'l':
                // Ogni file su cui abilitare la lock è una stringa nell'array di argomenti
                i = 0;

                while (args[i] != NULL)
                {
                    char* real_path = get_absolute_path(args[i]);

                    if (real_path == NULL)
                        return FILE_NOT_FOUND;

                    lockFile(real_path);
                    i++;
                }
                
                break;
            case 'u':
                // Ogni file su cui disabilitare la lock è una stringa nell'array di argomenti
                i = 0;

                while (args[i] != NULL)
                {
                    char* real_path = get_absolute_path(args[i]);

                    if (real_path == NULL)
                        return FILE_NOT_FOUND;

                    unlockFile(real_path);
                    i++;
                }
                break;
            case 'c':
                // Argomenti: file da rimuovere dal server
                break;
            default:
                fprintf(stderr, "Operazione %c non riconosciuta, impossibile eseguirla.\n", op);
                break;
        }

        // Ho esaurito una richiesta, passo alla prossima
        free(curr_request);
        // Pulisco la lista degli argomenti
        free(args);
    }

    return 0;
}

int send_from_dir(const char* dirpath, int* n_files, const char* write_dir)
{
    // Se ho finito, ritorno
    if (n_files == 0)
        return 0;
    // Controllo che il parametro sia una directory
    struct stat dir_info;

    if (stat(dirpath, &dir_info) == 0) 
    {
        // Se dirpath è una cartella
        if (S_ISDIR(dir_info.st_mode))
        {
            DIR * dir;
            // Cerco di aprire la cartella
            if ((dir = opendir(dirpath)) == NULL) 
            {
                fprintf(stderr, "Impossibile aprire la directory %s\n", dirpath);
                return FILESYSTEM_ERROR;
            } 
            else 
            {
                struct dirent *file;
                
                // Finché non ho errore, ho qualcosa da leggere e ho ancora file da leggere
                while((errno=0, file = readdir(dir)) != NULL && *n_files != 0) 
                {
                    // Ignoro . e .., altrimenti trovo file inesistenti o spedisco l'intero filesystem
                    if (strcmp(file->d_name, ".") != 0 && strcmp(file->d_name, "..") != 0)
                    {
                        // Aggiungo il nome del file corrente 
                        char filename[PATH_MAX];
                        
                        int dir_len = strlen(dirpath);
                        int file_len = strlen(file->d_name);
                        
                        if ((dir_len + file_len + 2) > PATH_MAX) 
                        {
                            fprintf(stderr, "Path del file da spedire troppo lungo\n");
                            return FILESYSTEM_ERROR;
                        }
                        
                        // Aggiungo il nome del file al percorso corrente
                        strncpy(filename, dirpath, PATH_MAX - 1);
                        strncat(filename, "/", PATH_MAX - 1);
                        strncat(filename, file->d_name, PATH_MAX - 1);
                        
                        // Richiamo la funzione sul nuovo percorso
                        send_from_dir(filename, n_files, write_dir);
                    }
                }

                // Posso chiudere la directory
                closedir(dir);

                return 0;
            }
        }
        // Se invece è un file
        else if (S_ISREG(dir_info.st_mode))
        {
            // Ottengo il path completo
            char full_path[PATH_MAX];
            // Se l'invio è avvenuto con successo, segnalo che ho scritto un file
            if (writeFile(realpath(dirpath, full_path), write_dir) == 0)
                (*n_files)--;
            return 0;
        }
        else
            // Errore
            return NOT_A_FOLDER;
    }
    else
    {
        fprintf(stderr, "Errore nell'esecuzione di stat sulla directory %s\n", dirpath);
        return FILESYSTEM_ERROR;
    }
}

char** parse_request_arguments(char* args)
{
    char** ret;
    int n_args = 1;
    int i = 0; 

    // Calcolo il numero di argomenti, ogni virgola è un argomento in più
    while (args[i] != '\0')
    {
        if (args[i] == ',')
            n_args++;
        i++;
    }
    n_args++;

    // Alloco correttamente il vettore di argomenti
    ret = malloc(sizeof(char*) * n_args);
    memset(ret, 0, n_args);
    
    i = 0;

    // Prendo la prima stringa
    ret[0] = strtok(args, ",");
    i++;

    // Prendo le restanti
    while ((ret[i] = strtok(NULL, ",")) != NULL)
        i++;

    return ret;
}

int connect_to_server()
{
    return 0;
}

void clean_client(Hashmap config, List requests)
{
    // Pulisco la memoria delle strutture dati
    hashmap_clean(config, NULL);
    list_clean(requests, clean_request_node);
}

ClientConfig initialize_client(Hashmap config)
{
    ClientConfig ret;
    errno = 0;

    ret.socket_name = NULL;
    ret.expelled_dir = NULL;
    ret.read_dir = NULL;
    ret.request_rate = 0;
    ret.print_op_data = 0;

    if (hashmap_has_key(config, "f"))
    {
        ret.socket_name = (char*)hashmap_get(config, "f");
    }
    else
    {
        fprintf(stderr, "Specificare il nome del socket a cui collegarsi tramite l'opzione -f name.\n");
        errno = MISSING_SOCKET_NAME;
        
        return ret;
    }

    if (hashmap_has_key(config, "D"))
    {
        ret.expelled_dir = (char*)hashmap_get(config, "D");
    }

    if (hashmap_has_key(config, "d"))
    {
        ret.read_dir = (char*)hashmap_get(config, "d");
        printf("Read dir: %s\n", ret.read_dir);
    }

    if (hashmap_has_key(config, "t"))
    {
        ret.request_rate = atol((char*)hashmap_get(config, "t"));
    }

    if (hashmap_has_key(config, "p"))
    {
        ret.print_op_data = atol((char*)hashmap_get(config, "p"));
    }

    return ret;
}

void print_client_config(ClientConfig to_print)
{
    printf("Configurazione del client:\n");

    printf("Nome del socket: %s\n", to_print.socket_name);

    if (to_print.expelled_dir != NULL)
        printf("Path di scritura file espulsi: %s\n", to_print.expelled_dir);
    if (to_print.read_dir != NULL)
        printf("Path di lettura file: %s\n", to_print.read_dir);
    
    printf("Intervallo delle richieste: %d\n", to_print.request_rate);
    printf("Stampa dei dati delle operazioni: %d\n", to_print.print_op_data);
   
}

int parse_options(Hashmap* config, List* requests, int n_args, char** args)
{
    int opt;
    unsigned short int used_name = FALSE;
    unsigned short int used_val = FALSE;
    
    char* opt_name = malloc(sizeof(char) * OPT_NAME_LENGTH);
    char* opt_value = malloc(sizeof(char) * OPT_VALUE_LENGTH);

    ArgLineRequest* curr_request = malloc(sizeof(ArgLineRequest));
    memset(curr_request, 0, sizeof(ArgLineRequest));

    sprintf(opt_value, "%d", 0);
    sprintf(opt_name, "p");
    hashmap_put(config, opt_value, opt_name);

    opt_value = malloc(sizeof(char) * OPT_VALUE_LENGTH);
    memset(opt_value, 0, OPT_NAME_LENGTH);

    opt_name = malloc(sizeof(char) * OPT_NAME_LENGTH);
    memset(opt_name, 0, OPT_NAME_LENGTH);

    while ((opt = getopt(n_args, args, ":phf:w:W:D:R::r:d:t:l:u:c:a:")) != -1)
    {
        switch (opt)
        {
            // Gestisco le opzioni inseribili una sola volta (opzioni di configurazione)
            case 'f':
            case 'D':
            case 'd':
            case 't':
                // Converto il nome dell'opzione a stringa, aggiungo i terminatori
                sprintf(opt_name, "%c", opt);
                sprintf(opt_value, "%s", optarg);
                // Uso tale stringa come chiave per il valore dell'argomento
                hashmap_put(config, (void*)opt_value, opt_name);

                used_name = TRUE;
                used_val = TRUE;
                break;
            case 'h':
                print_client_options();
                break;
            case 'p':
                sprintf(opt_name, "p");
                sprintf(opt_value, "1");
                // Uso tale stringa come chiave per il valore dell'argomento
                hashmap_put(config, (void*)opt_value, opt_name);

                used_name = TRUE;
                used_val = TRUE;
                break;
            // Gestisco le opzioni che richiedono una lista di argomenti separati da virgola
            case 'w':
            case 'W':
            case 'r':
            case 'l':
            case 'u':
            case 'c':
            case 'a':
                sprintf(opt_name, "%c", opt);
                // Salvo le informazioni passate da linea di comando
                curr_request->code = opt;
                curr_request->arguments = (char*)optarg;
                
                // Inserisco la richiesta nella coda delle richieste
                list_enqueue(requests, (void*)curr_request, opt_name);
                // Rialloco la struttura
                curr_request = malloc(sizeof(ArgLineRequest));
                memset(curr_request, 0, sizeof(ArgLineRequest));

                used_name = TRUE;
                break;
            // Gestisco le istruzioni con un parametro opzionale
            case 'R':
                // Controllo se R ha un parametro o no
                if (optarg != NULL)
                    sprintf(opt_value, "%s", optarg);
                else
                    sprintf(opt_value, "-1");
                
                curr_request->code = 'R';
                curr_request->arguments = opt_value;
                // Metto R in coda come operazione
                list_enqueue(requests, (void*)curr_request, NULL);
                curr_request = malloc(sizeof(ArgLineRequest));
                memset(curr_request, 0, sizeof(ArgLineRequest));

                used_val = TRUE;
                break;
            case ':':
                fprintf(stderr, "L'opzione %c necessita di un parametro.\n", optopt);
                break;
            case '?':
                fprintf(stderr, "L'opzione %c non e' riconosciuta.\n", optopt);
                break;
            default:
                break;
        }

        if (used_name)
        {
            // Rialloco la chiave così la prossima volta è in una locazione diversa
            opt_name = malloc(sizeof(char) * OPT_NAME_LENGTH);
            memset(opt_name, 0, OPT_NAME_LENGTH);
        }
        if (used_val)
        {
            // Rialloco il parametro così la prossima volta è in una locazione diversa
            opt_value = malloc(sizeof(char) * OPT_VALUE_LENGTH);
            memset(opt_value, 0, OPT_VALUE_LENGTH);
        }
        
        used_name = FALSE;
        used_val = FALSE;
    }

    free(opt_name);
    free(opt_value);
    free(curr_request);

    return validate_input(*config, *requests);
}

int validate_input(Hashmap config, List requests)
{
    // -D va usata con -W o -w
    if (hashmap_has_key(config, "D"))
    {
        if (!(list_contains_key(requests, "w") || list_contains_key(requests, "W")))
        {
            fprintf(stderr, "L'opzione -D non puo' essere usata senza -w o -W.\n");
            return INCONSISTENT_INPUT_ERROR;
        }
    }

    // -d va usata con -R o -r
    if (hashmap_has_key(config, "d"))
    {
        if (!(list_contains_string(requests, "r") || list_contains_string(requests, "R")))
        {
            fprintf(stderr, "L'opzione -d non puo' essere usata senza -r o -R.\n");
            return INCONSISTENT_INPUT_ERROR;
        }
    }

    // L'argomento di t dev'essere un numero e dev'essere un numero valido
    if (hashmap_has_key(config, "t"))
    {
        // Usato per gestire gli errori in strtol
        char* endptr = NULL;
        char* t_value = (char*)hashmap_get(config, "t");

        errno = 0;
        int t = strtol(t_value, &endptr, 10);

        if ((t == 0 && errno != 0) || (t == 0 && endptr == t_value))
        {
            fprintf(stderr, "Il parametro di -t non è valido.\n");
            errno = 0;
            return NAN_INPUT_ERROR;
        }
        else
        {
            if (t < 0)
            {
                fprintf(stderr, "Il parametro di -t non puo' essere un numero negativo.\n");
                return INVALID_NUMBER_INPUT_ERROR;
            }
        }
    }

    // L'argomento di R, se esiste, dev'essere un numero
    int r_index = list_contains_string(requests, "R");
    if (r_index >= 0)
    {
        // Usato per gestire gli errori in strtol
        char* endptr = NULL;
        ArgLineRequest* R_value = (ArgLineRequest*)list_get(requests, r_index);

        errno = 0;
        int R = strtol(R_value->arguments, &endptr, 10);

        if ((R == 0 && errno != 0) || (R == 0 && endptr == R_value->arguments))
        {
            fprintf(stderr, "Il parametro di -R non è valido.\n");
            errno = 0;
            return NAN_INPUT_ERROR;
        }
    }

    return 0;
}

void print_client_options()
{

}

void print_node_request(Node* node)
{
    ArgLineRequest* to_print = (ArgLineRequest*)node->data;

    if (to_print->code != 'p')
        printf("Op: %c, args: %s\n", to_print->code, to_print->arguments);
}

void print_node_string(Node* node)
{
    char* string = (char*) (node->data);

    printf("Key: %s, value: %s\n", node->key, string);
}

void clean_request_node(Node* node)
{
    ArgLineRequest* data = (ArgLineRequest*) (node->data);

    free((void*)data->arguments);
}