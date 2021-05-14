# Progetto-SOL-2021

## TODO LIST

- Esecuzione richieste client
- Invio richieste al server
- Attesa della risposta
- Chiusura e pulizia

### SHOULD HAVE

- Implementare anche un comparatore nei nodi così da evitare problemi di confusione tra chiavi e valori
  nelle code, che tecnicamente hanno solo valori

### COSE PREOCCUPANTI
- A un certo punto abstime andrà settato in maniera corretta (parsing di date da linea di comando?)

### ERROR CHECKING
- Error checking nelle strutture dati
- Error checking nelle chiamate alle funzioni dei socket e nell'allocazione della memoria
- Controllare che file e directory di configurazione esistano
- Magari utilizzare le procedure viste a lezione, sono piuttosto generiche e potrebbero rendere il tutto un po' più ordinato

#### DOCUMENTATION:
- Commenti alle funzioni nelle strutture dati

#### POLISH:
- Limiti alle strutture dati o differenziazione: se creo una lista per farci una coda non devo   poter
  usare le operazioni dello stack. Basterebbe un enum che dice di che tipo specifico è la lista, oppure
  si potrebbero suddividere le strutture, ma mi sembra fuori dallo scopo del progetto
- Le liste ammettono copie? Dovrei implementare una contains ed evitare copie probabilmente
- Dividere funzioni che svolgono compiti relativi a domini simili in librerie
- Sarebbe probabilmente più pulito avere una libreria dedicata alla gestione degli errori
- Se gli errori vanno gestiti a cascata e basta una exit(EXIT_FAILURE) per gestirli, allora potrebbe 
  essere utile mettere certe funzioni in wrappers che gestiscano gli errori in modo autonomo per rendere
  più pulito il codice.
- Usare dup2 nel server per redirezionare l'output al file di log se necessario