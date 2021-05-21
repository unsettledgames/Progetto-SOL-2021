# Progetto-SOL-2021

## TODO LIST
- Logging (in corso)
  - Apertura / chiusura connessione
  - Dettagli operazioni di lettura (nome file, dimensione file restituito)
  - Dettagli operazioni di scrittura (dettagli sulla procedura di caching, nome file espulsi, numero byte scritti)
  - Esito operazioni
  - Formato: HH:mm:ss:ms -> dettagli
  
- Controllo errori (delicata)
- Funzioni opzionali
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