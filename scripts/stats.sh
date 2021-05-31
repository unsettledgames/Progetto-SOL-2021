#!/bin/bash

tot_read=0
tot_write=0
tot_open=0
tot_close=0
tot_LRU=0

read_bytes=0
written_bytes=0

max_size=0
max_files=0
max_conn=0

if [ -d "Logs" ]; then
    cd Logs
    log_file=$(ls -Ar | head -n1)
    echo "Log filename: ${log_file}"

    if [ log_file != '' ]; then
        # Conto le occorrenze di [RD] per contare le letture
        tot_read=$(grep -o -i "\[RD\]" "${log_file}" | wc -l)
        # Conto le occorrenze di [WT] per contare le scritture
        tot_write=$(grep -o -i "\[WT\]" "${log_file}" | wc -l)
        # Conto le occorrenze di [OP] per contare le aperture
        tot_open=$(grep -o -i "\[OP\]" "${log_file}" | wc -l)
        # Conto le occorrenze di [CL] per contare le chiusure
        tot_close=$(grep -o -i "\[CL\]" "${log_file}" | wc -l)
        # Conto le occorrenze di [LRU] per contare i rimpiazzi
        tot_LRU=$(grep -o -i "\[LRU\]" "${log_file}" | wc -l)

        # Ciclo tra le linee che contengono [WT], tenendo solo i numeri che seguono [WT]
        for i in $(grep -e '\[WT\]' "${log_file}" | cut -c 6- ); do
            written_bytes=$written_bytes+$i;
        done
        # Passo la stringa risultante a bc per ottenere la somma
        written_bytes=$(bc <<< ${written_bytes})

        # Stesso processo per i byte letti
        for i in $(grep -e '\[RD\]' "${log_file}" | cut -c 6- ); do
            read_bytes=$read_bytes+$i;
        done
        echo "Read bytes: $written_bytes"
        read_bytes=$(bc <<< ${read_bytes})

        # Prendo i dati che cominciano con [SIZE], li metto in ordine decrescente, prendo il primo ed è il massimo
        max_size=$(grep -e '\[SIZE\]' "${log_file}" | cut -c 8- | sort -r | head -1)
        # Stesso concetto per il numero massimo di files
        max_files=$(grep -e '\[NFILES\]' "${log_file}" | cut -c 10- | sort -r | head -1)
        # Stesso concetto per il numero massimo di connessioni
        max_conn=$(grep -e '\[CN\]' "${log_file}" | cut -c 6- | sort -r | head -1)

        # Ora, prendo il numero di workers
        n_threads=$(grep -e '\[NTH\]' <<< "${log_file}" | cut -c 7- )
        n_threads=$(bc <<< "${n_threads}-1")

        # Per ogni numero di thread, conto il numero di richieste (occorrenze di "[RQ] tid")
        thread_rq=()
        for i in $( eval echo {0..${n_threads}} ); do
            thread_rq[$i]=$(grep -o -i "\[RQ\] $i" "${log_file}" | wc -l)
        done
        
        # Ora posso stampare correttamente le statistiche
        echo "Statistiche relative all'ultima esecuzione del server:"
        echo "Numero di open: ${tot_open}"
        echo "Numero di close: ${tot_close}"
        echo "Numero di read: ${tot_read}"
        echo "Numero di write: ${tot_write}"
        echo "Invocazioni dell'algoritmo di rimpiazzamento: ${tot_LRU}"
        echo "Massimo spazio occupato da file: ${max_size}"
        echo "Massimo numero di file: ${max_files}"
        echo "Massimo numero di connessioni contemporanee: ${max_conn}"
        echo "Numero di richieste soddisfatte per thread:"
        for i in $( eval echo {0..${n_threads}} ); do
            echo "Worker $i: ${thread_rq[$i]}"
        done

        if [ ${tot_read} != 0 ]; then
            echo "Bytes letti in media:"
            echo "scale=2; ${read_bytes} / ${tot_read}" | bc -l
        fi
        
        if [ ${tot_write} != 0 ]; then
            echo "Bytes scritti in media:"
            echo "scale=2; ${written_bytes} / ${tot_write}" | bc -l
        fi

    else
        echo "Non e' stato ancora eseguito il programma"
    fi
else
    echo "Impossibile trovare directory dei log"
fi


