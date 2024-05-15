#!/bin/bash

ports="5254 5255 5256"

for port in $ports; do
    lsof -i:$port | grep LogCabin | while read line; do
        array=($line)
        pid=${array[1]}
        kill $pid
    done
done

echo "clear successfully"