# Progetto-SOL-2021

## TODO LIST

- Configurazione client
- Esecuzione richieste client

### SHOULD HAVE

- Implementare anche un comparatore nei nodi così da evitare problemi di confusione tra chiavi e valori
  nelle code, che tecnicamente hanno solo valori

### ERROR CHECKING
- Error checking nelle strutture dati
- Controllare che file e directory di configurazione esistano

#### DOCUMENTATION:
- Commenti alle funzioni nelle strutture dati

#### POLISH:
- Limiti alle strutture dati o differenziazione: se creo una lista per farci una coda non devo   poter
  usare le operazioni dello stack. Basterebbe un enum che dice di che tipo specifico è la lista, oppure
  si potrebbero suddividere le strutture, ma mi sembra fuori dallo scopo del progetto
- Le liste ammettono copie? Dovrei implementare una contains ed evitare copie probabilmente
- Dividere funzioni che svolgono compiti relativi a domini simili in librerie
- Sarebbe probabilmente più pulito avere una libreria dedicata alla gestione degli errori