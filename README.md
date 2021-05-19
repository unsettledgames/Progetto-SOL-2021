# Progetto-SOL-2021

## TODO LIST

- Aggiunta delle funzioni di caching nelle API (restituzione e salvatagio file espulsi)
  - readFile
- Segnali
- Flag nella openFile
- Conversione da file testuali a binari
- Chiusura e pulizia
- Controllo leak e accessi in memoria
  - Flag rb nell'apertura e nella scrittura dei file
  - Verifica che le funzioni sulle stringhe continuino a funzionare
  - Passaggio della dimensione dei file da parte delle api
- Controllo errori
- Funzioni opzionali
  - Logging
  - Compressione
  - test3
  - -D
- Polish se resta tempo

### COSE PREOCCUPANTI
- A un certo punto abstime andrà settato in maniera corretta (parsing di date da linea di comando?)
- Come distinguo tra un path assoluto e uno relativo? E' sufficiente supporre che tutti i path passati da linea di comando siano assoluti?
- - Idea interessante: quando apro un file che non esiste, creo una versione temporanea nel path specificato dall'utente, così poi può riaccederci quando vuole usando il path relativo. 

### ERROR CHECKING
- Error checking nelle strutture dati
- Error checking nelle chiamate alle funzioni dei socket e nell'allocazione della memoria
- Controllare che file e directory di configurazione esistano
- Magari utilizzare le procedure viste a lezione, sono piuttosto generiche e potrebbero rendere il tutto un po' più ordinato

#### DOCUMENTATION:
- Commenti alle funzioni nelle strutture dati

#### POLISH:
- Dividere funzioni che svolgono compiti relativi a domini simili in librerie
- Se gli errori vanno gestiti a cascata e basta una exit(EXIT_FAILURE) per gestirli, allora potrebbe 
  essere utile mettere certe funzioni in wrappers che gestiscano gli errori in modo autonomo per rendere
  più pulito il codice.
- Usare dup2 nel server per redirezionare l'output al file di log se necessario