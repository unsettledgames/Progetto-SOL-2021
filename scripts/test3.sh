echo "Esecuzione del test 3"
echo -e "8\n32000000\n100\nLSOfilestorage.sk\nLogs" > config.txt

current_date=$(date +%s)
stop_date=$(echo "$current_date + 60" | bc)

pids=()
./server config.txt &
server_pid=$!

file_index=1
folder_index=1

while [ $(date +%s) -lt $stop_date ]; do
    for i in {1..10}; do
        if [ ! -z "$pids[$i]" ]; then
            ./client -f LSOfilestorage.sk -W TestDir/BigTest$folder_index/lorem$file_index.txt -p -D ToWriteIn \
            -W TestDir/BigTest$folder_index/lorem$(($file_index + 1)).txt -p -D ToWriteIn \
            -W TestDir/BigTest$folder_index/lorem$(($file_index + 2)).txt -p -D ToWriteIn \
            -W TestDir/BigTest$folder_index/lorem$(($file_index + 3)).txt -p -D ToWriteIn \
            -W TestDir/BigTest$folder_index/lorem$(($file_index + 4)).txt -p -D ToWriteIn
            pids=( "${pids[@]:0:$i}" $! "${pids[@]:$i}" )

            file_index=$(($file_index + 5))

            if (($file_index > 15)); then 
                echo "Resetto"
                file_index=1;
                folder_index=$(( ($folder_index + 1) % 15 ))

                if (($folder_index == 0)); then
                    $folder_index=1
                fi
            fi

            echo "File index: $file_index"
            echo "Folder index: $folder_index"

        fi
    done
done

kill -s SIGHUP $server_pid
echo 'finished'