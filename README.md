# Progetto-SOL-2021

## TODO LIST
- Controllo leak e accessi in memoria (impegnativa, manca il server)
- Aggiungere controllo della modifica alla LRU
- Timer nelle richieste (facile)
- Flag nella openFile (media)
- Conversione da file testuali a binari (delicata)
  - Flag rb nell'apertura e nella scrittura dei file (facile)
  - Verifica che le funzioni sulle stringhe continuino a funzionare (media)
  - Passaggio della dimensione dei file da parte delle api (facile)
- Controllo errori (delicata)
- Funzioni opzionali
  - Logging (facile)
  - Compressione (da valutare)
  - test3 (da valutare)
- Passaggio nome del file di configurazione da linea di comando al server
- Polish se resta tempo (delicato)
- Controllo finale leak e accessi in memoria (media)

### COSE PREOCCUPANTI
- A un certo punto abstime andrà settato in maniera corretta (parsing di date da linea di comando?)
- Se un client si disconnette prima del dovuto, il server va in loop per richieste non supportate
  - Per difendersi potrebbe chiudere la connessione con un client che invia richieste non supportate e proseguire
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