SHELL := /bin/bash

# Phony targets dei test
.PHONY : all client server clean test1 test2 test3

# Opzioni di compilazione
CC = gcc
GENERIC_FLAGS = -Wall -pedantic -lpthread -g 
THREAD_FLAGS = -lpthread
INCLUDES = -I./headers

# -------------------Cartelle utilizzate frequentemente---------------
# Cartella dei file oggetto
O_FOLDER = build/obj
# Cartella delle strutture dati
DS_FOLDER = sources/data-structures
# Cartella delle api
API_FOLDER = sources/api

# Dipendenze di client e server
client_deps = sources/client.c sources/utility/utility.c libs/libdata-structures.so libs/libapi.so
server_deps = sources/server.c sources/utility/utility.c libs/libdata-structures.so libs/libapi.so libs/libz.so

clean:

	rm -f -rf build
	rm -f -rf libs
	rm -f *.sk
	rm -f server
	rm -f client
	
	mkdir libs
	mkdir build
	mkdir build/obj

all: client server

# Compilazione del server
server: $(server_deps)
	$(CC) $(INCLUDES) $(GENERIC_FLAGS) sources/server.c sources/utility/utility.c -o server -Wl,-rpath,./libs -L ./libs -ldata-structures -lapi -lz
# Compilazione del client
client: $(client_deps)
	$(CC) $(INCLUDES) $(GENERIC_FLAGS) sources/client.c sources/utility/utility.c -o client -Wl,-rpath,./libs -L ./libs -ldata-structures -lapi
	
# Make di zlib
libs/libz.so:
	cd deps/zlib-1.2.11/; \
	./configure;make test; \
	cd ../..; \
	cp deps/zlib-1.2.11/libz.so libs

# Make della libreria delle strutture dati
libs/libdata-structures.so: $(O_FOLDER)/nodes.o $(O_FOLDER)/list.o $(O_FOLDER)/hashmap.o
	$(CC) -shared -o libs/libdata-structures.so $^
$(O_FOLDER)/nodes.o:
	$(CC) $(INCLUDES) $(GENERIC_FLAGS) $(DS_FOLDER)/nodes.c -c -fPIC -o $@
$(O_FOLDER)/list.o:
	$(CC) $(INCLUDES) $(GENERIC_FLAGS) $(DS_FOLDER)/list.c -c -fPIC -o $@
$(O_FOLDER)/hashmap.o:
	$(CC) $(INCLUDES) $(GENERIC_FLAGS) $(DS_FOLDER)/hashmap.c -c -fPIC -o $@

# Make della libreria delle api
libs/libapi.so: $(O_FOLDER)/api.o $(O_FOLDER)/utility.o
	$(CC) -shared -o libs/libapi.so $^
$(O_FOLDER)/api.o:
	$(CC) $(INCLUDES) $(GENERIC_FLAGS) $(API_FOLDER)/api.c -c -fPIC -o $@
$(O_FOLDER)/utility.o:
	$(CC) $(INCLUDES) sources/utility/utility.c -c -fPIC -o $@

# Make dei test, che hanno bisogno sia del client che del server
#test1: client server
#	echo 'Eseguo il test 1'
#test2: client server
#	echo 'Eseguo il test 2'
#Compilazione delle dipendenze del client: hanno bisogno dei sorgenti
#depc_a.o: depc_a.c
#	$(CC) $(CFLAGS) -c $^	
#depc_b.o: depc_b.c
#	$(CC) $(CFLAGS) -c $^

