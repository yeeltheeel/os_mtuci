#!/bin/bash 
set -uo pipefail 

MAP_DIR="/var/log/my_mem_maps"
mkdir -p "$MAP_DIR"

gcc -O2 -Wall -Wextra -o memtest program.c

./memtest p1 & PID1=$!
./memtest p2 & PID2=$!

sleep 5

NOW=$(date +"%Y-%m-%d_%-H:%-M:%-S")
pmap -x "$PID1" > "$MAP_DIR/map_${PID1}_${NOW}_p1.txt"
pmap -x "$PID2" > "$MAP_DIR/map_${PID2}_${NOW}_p2.txt"
echo "saved logs for $NOW"

kill -15 $PID1 $PID2 # or kill -TERM $PID1 $PID2
wait
