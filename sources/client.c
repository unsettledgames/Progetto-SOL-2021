#include "client.h"

ClientConfig client_configuration;

int main(int argc, char** argv)
{
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
            // Esecuzione delle richieste
            execute_requests(&requests);
        }
        else
        {
            clean_client(config, requests);
            return errno;
        }
    }

    clean_client(config, requests);
    return 0;
}

void clean_client(Hashmap config, List requests)
{
    // Pulisco la memoria delle strutture dati
    hashmap_clean(config, NULL);
    list_clean(requests, NULL);
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

int execute_requests(List* requests)
{
    return 0;
}

int parse_options(Hashmap* config, List* requests, int n_args, char** args)
{
    int opt;
    unsigned short int used_name = FALSE;
    unsigned short int used_val = FALSE;
    
    char* opt_name = malloc(sizeof(char) * OPT_NAME_LENGTH);
    char* opt_value = malloc(sizeof(char) * OPT_VALUE_LENGTH);

    Request* curr_request = malloc(sizeof(Request));

    sprintf(opt_value, "%d", 0);
    sprintf(opt_name, "p");
    hashmap_put(config, opt_value, opt_name);

    opt_value = malloc(sizeof(char) * OPT_VALUE_LENGTH);
    opt_name = malloc(sizeof(char) * OPT_NAME_LENGTH);

    while ((opt = getopt(n_args, args, ":phf:w:W:D:R::r:d:t:l:u:c:")) != -1)
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
                sprintf(opt_name, "%c", opt);
                // Salvo le informazioni passate da linea di comando
                curr_request->code = opt;
                curr_request->arguments = (char*)optarg;
                
                // Inserisco la richiesta nella coda delle richieste
                list_enqueue(requests, (void*)curr_request, opt_name);
                // Rialloco la struttura
                curr_request = malloc(sizeof(Request));

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
                curr_request = malloc(sizeof(Request));

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
            // Rialloco la chiave così la prossima volta è in una locazione diversa
            opt_name = malloc(sizeof(char) * OPT_NAME_LENGTH);
        if (used_val)
            // Rialloco il parametro così la prossima volta è in una locazione diversa
            opt_value = malloc(sizeof(char) * OPT_VALUE_LENGTH);
        
        used_name = FALSE;
        used_val = FALSE;
    }

    free(opt_name);
    free(opt_value);
    free(curr_request);

    print_list(*requests, "Requests");

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
        Request* R_value = (Request*)list_get(requests, r_index);

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
    Request* to_print = (Request*)node->data;

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
    Request* data = (Request*) (node->data);

    free((void*)data->arguments);
}