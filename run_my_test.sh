#!/bin/bash

rm -rf logs/log1.txt
rm -rf logs/log2.txt
rm -rf logs/log3.txt

if [ -d "pidfiles/server1.txt"]; then
    echo "Kill server1"
    pid1=$(cat pidfiles/server1.txt)
    kill -9 $pid1
fi
if [ -d "pidfiles/server2.txt"]; then
    echo "Kill server2"
    pid2=$(cat pidfiles/server2.txt)
    kill -9 $pid2
fi
if [ -d "pidfiles/server3.txt"]; then
    echo "Kill server3"
    pid3=$(cat pidfiles/server3.txt)
    kill -9 $pid3
fi

rm -rf storage

ALLSERVERS=10.64.101.39:5254,10.64.101.39:5255,10.64.101.39:5256

build/LogCabin --config configs/logcabin-1.conf --bootstrap
build/LogCabin --config configs/logcabin-1.conf -l logs/log1.txt -p pidfiles/server1.txt &
build/LogCabin --config configs/logcabin-2.conf -l logs/log2.txt -p pidfiles/server2.txt &
build/LogCabin --config configs/logcabin-3.conf -l logs/log3.txt -p pidfiles/server3.txt &

build/Examples/Reconfigure --cluster=$ALLSERVERS set 10.64.101.39:5254 10.64.101.39:5255 10.64.101.39:5256