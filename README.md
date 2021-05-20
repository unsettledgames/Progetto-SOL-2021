# Progetto-SOL-2021

## TODO LIST
- Flag nella openFile (media)
- Controllo errori (delicata)
- Funzioni opzionali
  - Logging (facile)
  - Compressione (da valutare)
  - test3 (da valutare)
- Polish se resta tempo (delicato)
- Controllo finale leak e accessi in memoria (media)

### COSE PREOCCUPANTI
- Come distinguo tra un path assoluto e uno relativo? E' sufficiente supporre che tutti i path passati da linea di comando siano assoluti?
  - Idea interessante: quando apro un file che non esiste, creo una versione temporanea nel path specificato dall'utente, così poi può riaccederci quando vuole usando il path relativo. 
  - Altra idea interessante per i path: se realpath ritorna NULL, provo comunque l'accesso usando il path locale
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