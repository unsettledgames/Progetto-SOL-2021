# Progetto-SOL-2021

## TODO LIST
- Gestione path
  - Dev'essere spostata nelle api
  - Se il client fornisce un file che non esiste fisicamente
    - Allora mantengo un path relativo nel server se O_CREATE è stata specificata
  - Se il client fornisce un file che esiste già
    - Nel server salvo il path assoluto
  - In ogni caso, il client continua a riferirsi a quel file con il path locale
- Flag nella openFile (media)
- Controllo errori (delicata)
- Funzioni opzionali
  - Logging (facile)
  - Compressione (da valutare)
  - test3 (da valutare)
- Polish se resta tempo (delicato)
- Controllo finale leak e accessi in memoria (media)

### COSE PREOCCUPANTI
- Penso di non aver idea del perché la list_contains_string funzioni

### ERROR CHECKING
- Error checking nelle strutture dati
- Error checking nelle chiamate alle funzioni dei socket e nell'allocazione della memoria
- Controllare che file e directory di configurazione esistano
- Magari utilizzare le procedure viste a lezione, sono piuttosto generiche e potrebbero rendere il tutto un po' più ordinato

#### DOCUMENTATION:
- Commenti alle funzioni nelle strutture dati

#### POLISH:
- Sostituire alle combinazioni di malloc e memset una calloc
- Dividere funzioni che svolgono compiti relativi a domini simili in librerie
- Se gli errori vanno gestiti a cascata e basta una exit(EXIT_FAILURE) per gestirli, allora potrebbe 
  essere utile mettere certe funzioni in wrappers che gestiscano gli errori in modo autonomo per rendere
  più pulito il codice.
- Usare dup2 nel server per redirezionare l'output al file di log se necessario