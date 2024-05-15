#!/bin/bash

bash /home/xuxiangcan/link_test/config.sh

ALLSERVERS=10.64.100.39:5254,10.64.100.39:5255,10.64.100.39:5256
build/MyTestCode/my_test --cluster=$ALLSERVERS