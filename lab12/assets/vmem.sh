#!/bin/bash 
set -uo pipefail 

MAP_DIR="/var/log/my_mem_maps"
mkdir -p "$MAP_DIR"

TARGET="memtest"
SRC="program.c"

if [[ ! -x "$TARGET" ]]; then
    echo ">>> $TARGET not found, building from $SRC"
    gcc -O2 -Wall -Wextra -o "$TARGET" "$SRC" || { echo "build failed" >&2; exit 1; }
elif [[ "$SRC" -nt "$TARGET" ]]; then
    echo ">>> $SRC is newer than $TARGET, rebuilding"
    gcc -O2 -Wall -Wextra -o "$TARGET" "$SRC" || { echo "rebuild failed" >&2; exit 1; }
else
    echo ">>> $TARGET is up to date, skipping build"
fi

./"$TARGET" p1 & PID1=$!
./"$TARGET" p2 & PID2=$!

sleep 5

NOW=$(date +"%Y-%m-%d_%-H:%-M:%-S")
pmap -x "$PID1" > "$MAP_DIR/map_${PID1}_${NOW}_p1.txt"
pmap -x "$PID2" > "$MAP_DIR/map_${PID2}_${NOW}_p2.txt"
echo "saved logs for $NOW"

kill -15 $PID1 $PID2 # or kill -TERM $PID1 $PID2
wait
