echo "Esecuzione del test 3"
echo -e "8\n32000000\n100\nLSOfilestorage.sk\nLogs" > config.txt



pids=()
# Creo una subshell
{ ./server config.txt; } & 
# Ottengo il pid della shell corrente
server_pid=$!
# Ottengo il pid della subshell creata, cioè il pid il cui parent è server_pid (questa shell)
server_pid=$(ps -ax -o ppid,pid --no-headers | sed -r 's/^ +//g;s/ +/ /g' |
                           grep "^$server_pid " | cut -f 2 -d " ")

current_date=$(date +%s)
stop_date=$(echo "$current_date + 30" | bc)

file_index=1
folder_index=1

while [ $(date +%s) -lt $stop_date ]; do
    for i in {0..9}; do
        if ! ps -p ${pids[$i]} > "/dev/null" 2>&1; then
            { ./scripts/launch_client.sh "-f LSOfilestorage.sk -W TestDir/BigTest$folder_index/lorem$file_index.txt -D ToWriteIn\
            -W TestDir/BigTest$folder_index/lorem$(($file_index + 1)).txt\
            -W TestDir/BigTest$folder_index/lorem$(($file_index + 2)).txt\
            -W TestDir/BigTest$folder_index/lorem$(($file_index + 3)).txt\
            -W TestDir/BigTest$folder_index/lorem$(($file_index + 4)).txt"; } &
            pids=( "${pids[@]:0:$i}" $! "${pids[@]:$i}" )

            file_index=$(($file_index + 5))

            if (($file_index > 15)); then 
                file_index=1;
                folder_index=$(( ($folder_index + 1) % 15 ))

                if (($folder_index == 0)); then
                    folder_index=1
                fi
            fi
        fi
    done
done

for i in {0..9}; do 
    wait ${pids[$i]}
done

kill -s SIGINT $server_pid
echo 'finished'