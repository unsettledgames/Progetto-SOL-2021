# Progetto-SOL-2021

Difetti del progetto:

- Dimensione minima della richiesta di 900KB, si può migliorare allocando dinamicamente il buffer (va anche deallocato però)
- server.c:711, alloco un intero dinamicamente quando basterebbe castarlo a void*
- server.c:635, per com'è fatta la list_get ho un while dentro il for totalmente inutile. Risolvibile o con iteratori sulla lista oppure facendo un while direttamente sulla lista (togliendo il livello di astrazione della get)
- Divisione tra connection_handler e dispatcher, non una critica vera e propria, quanto un appunto riguardo al fatto che non c'è un vantaggio concreto nel separarle, a parte la leggibilità e la debuggabilità del progetto
- Warning in readn e writen per aritmetica su puntatori void: visto che void* non ha tecnicamente una dimensione, andrebbero castati a char* per svolgere le addizioni

Per compilare ed eseguire:

    1. Decomprimere l’archivio e posizionarsi dentro la cartella del progetto.
    2. Eseguire chmod 777 scripts/*.sh
    3. Eseguire chmod 777 deps/zlib-1.2.11/configure
    4. Eseguire make clean
    5. Eseguire make all
    6. Eventualmente, eseguire make test1 per eseguire il test1, make test2 per eseguire il test2 e make test3 per eseguire il test3 (è anche possibile eseguirli senza bisogno di make eseguendo ./scripts/test1.sh, ./scripts/test2.sh o ./scripts/test3.sh)

Se si hanno problemi, aprire un issue e usare il commit precedente nel frattempo.
