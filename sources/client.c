#include "client.h"

int main(int argc, char** argv)
{
    Hashmap config;
    List requests;

    hashmap_initialize(&config, 1021, print_node_string);;
    list_initialize(&requests, print_node_request);

    parse_options(&config, &requests, argc, argv);

    print_hashmap(config, "Config data");
    print_list(requests, "Requests");

    hashmap_clean(config);
    
    return 0;
}


void parse_options(Hashmap* config, List* requests, int n_args, char** args)
{
    int opt;
    
    char* opt_name = malloc(sizeof(char) * OPT_NAME_LENGTH);
    char* opt_value = malloc(sizeof(char) * OPT_VALUE_LENGTH);

    Request* curr_request = malloc(sizeof(Request));

    sprintf(opt_value, "%d%c", 0, '\0');
    hashmap_put(config, opt_value, "p");

    opt_value = malloc(sizeof(char) * OPT_VALUE_LENGTH);

    while ((opt = getopt(n_args, args, "hf:w:W:D:R:r:d:t:l:u:c:p")) != -1)
    {
        switch (opt)
        {
            // Gestisco le opzioni inseribili una sola volta (opzioni di configurazione)
            case 'f':
            case 'D':
            case 'd':
            case 't':
                // Converto il nome dell'opzione a stringa, aggiungo i terminatori
                sprintf(opt_name, "%c%c", opt, '\0');
                sprintf(opt_value, "%s%c", optarg, '\0');
                // Uso tale stringa come chiave per il valore dell'argomento
                hashmap_put(config, (void*)opt_value, opt_name);

                // Rialloco la chiave così la prossima volta è in una locazione diversa
                opt_name = malloc(sizeof(char) * 5);
                // Rialloco il parametro così la prossima volta è in una locazione diversa
                opt_value = malloc(sizeof(char) * 100);
                break;
            case 'h':
                print_client_options();
                break;
            case 'p':
                break;
            // Gestisco le opzioni che richiedono una lista di argomenti separati da virgola
            case 'w':
            case 'W':
            case 'r':
            case 'l':
            case 'u':
            case 'c':
                // Salvo le informazioni passate da linea di comando
                curr_request->code = opt;
                curr_request->arguments = (char*)optarg;

                printf("Parsing Op: %c, args: %s\n", curr_request->code, curr_request->arguments);
                // Inserisco la richiesta nella coda delle richieste
                list_enqueue(requests, (void*)curr_request, NULL);

                curr_request = malloc(sizeof(Request));
                break;
            case 'R':
                break;
            // Gestisco le istruzioni con un parametro opzionale
            case ':':
                switch (optopt)
                {
                    case 'R':
                        // Stampo il valore di default a stringa
                        sprintf(opt_value, "%d", 0);
                        // Metto quella stringa nella tabella
                        //hashmap_put(ret, (void*)opt_value, "R");
                        break;
                    default:
                        fprintf(stderr, "L'opzione %c non ammette argomenti opzionali.\n", optopt);
                }
            case '?':
                fprintf(stderr, "L'opzione %c non e' stata immessa correttamente.\n", optopt);
                break;
            default:
                break;
        }
    }

    free(opt_name);
    free(opt_value);
    free(curr_request);
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