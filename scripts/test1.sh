#!/bin/bash

echo "Esecuzione del test 1"; 
echo -e "1\n128000000\n10000\nLSOfilestorage.sk\nLogs" > config.txt

valgrind --leak-check=full ./server config.txt &
pid=$!
sleep 1s

./client -f LSOfilestorage.sk -W TestDir/img0.jpg -p
./client -f LSOfilestorage.sk -w TestDir/1.0/ -p
./client -f LSOfilestorage.sk -r TestDir/1.0/file0.txt -d ReadFolder -p
./client -f LSOfilestorage.sk -R2 -d ReadFolder -p
./client -f LSOfilestorage.sk -R AllRead -p

sleep 1s

kill -s SIGHUP $pid
./scripts/stats.sh
kill -s SIGKILL $pid
