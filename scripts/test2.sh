#!/bin/bash

echo "Esecuzione del test 2"
echo -e "4\n1000000\n10\nLSOfilestorage.sk\nLogs" > config.txt

./server config.txt &
pid=$$
for i in {1..11}; do
    ./client -f LSOfilestorage.sk -W TestDir/lorem$i.txt -p -D ToWriteIn &
done
sleep 2s
kill -SIGHUP $pid &
./scripts/stats.sh