echo "Esecuzione del test 3"
echo -e "8\n32000000\n100\nLSOfilestorage.sk\nLogs" > config.txt

current_date=$(date +%s)
stop_date=$(echo "$current_date + 3" | bc)

pids=()
./server config.txt &

while [ $(date +%s) -lt $stop_date ]; do
    for i in {1..10}; do
        if [ ! -z "$pids[$i]" ]; then
            ./client -f LSOfilestorage.sk -W TestDir/lorem$i.txt -p -D ToWriteIn
            pids=( "${pids[@]:0:$i}" $! "${pids[@]:$i}" )
        fi
    done
done

echo 'finished'