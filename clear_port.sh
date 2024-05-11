#!/bin/bash

output=$(lsof -i:5254 | grep LISTEN)
array=($output)
pid=${array[1]}

kill $pid
echo "clear successfully"