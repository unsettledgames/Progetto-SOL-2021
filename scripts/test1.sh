#!/bin/bash

echo "Esecuzione del test 1"; 
echo -e "1\n128000000\n10000\nLSOfilestorage.sk\nLogs" > config.txt

valgrind --leak-check=full ./server config.txt &
pid=$!
sleep 1s

./client -f LSOfilestorage.sk -W TestDir/img0.jpg -w TestDir/1.0/ -t 200 -p
./client -f LSOfilestorage.sk -r TestDir/1.0/file0.txt -R2 -d ReadFolder -t 200 -p
./client -f LSOfilestorage.sk -R -d AllRead -p
./client -h

sleep 1s

kill -s SIGHUP $pid
./scripts/stats.sh
kill -s SIGKILL $pid
