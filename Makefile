# Phony targets dei test
.PHONY : test1 test2

# Opzioni di compilazione
CC = gcc
GENERIC_FLAGS = -Wall -pedantic -I/headers/
THREAD_FLAGS = -lpthread
LIB_FLAGS = -L ./libs/ -ldata-structures

# -------------------Cartelle utilizzate frequentemente---------------
# Cartella delle strutture dati
DS_FOLDER = src/data-structures

# Dipendenze di client e server
client_deps = depc_a.o depc_b.o libs/libdata-structures.so

# Make delle librerie, che sono indipendenti
libs/libdata-structures.so: build/data-structures.o
	$(CC) -shared -o build/libdata-structures.so build/data-structures.o
build/data-structures.o:
	$(CC) -std=c99 -Wall DS_FOLDER/node.c DS_FOLDER/queue.c DS_FOLDER/list.c DS_FOLDER/hashmap.c -c -fPIC -o build/$^

# Make dei test, che hanno bisogno sia del client che del server
test1: client server
	echo 'Eseguo il test 1'
test2: client server
	echo 'Eseguo il test 2'

# Compilazione del client: ha bisogno di tutte le sue dipendenze
client: client_deps
	$(CC) $(client_deps) -o client -L . -l/libs/data-structures.so

#Compilazione delle dipendenze del client: hanno bisogno dei sorgenti
depc_a.o: depc_a.c
	$(CC) $(CFLAGS) -c $^	
depc_b.o: depc_b.c
	$(CC) $(CFLAGS) -c $^

