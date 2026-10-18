#!/bin/bash

LOG_DIR="/var/log/my_stats"
mkdir -p "$LOG_DIR"

TS=$(date +"%Y-%m-%d_%H:%M:%S")
LOG_FILE="${LOG_DIR}/${TS}_syslog"

for cmd in mpstat pidstat; do
    command -v "$cmd" >/dev/null 2>&1 || { 
    	echo "$cmd not found (install sysstat)" >&2; 
    	exit 1; 
    }
done

{
	echo "stats for host=$(hostname) on $(date)"
	echo "kernel: $(uname -r), uptime: $(uptime -p)"
	echo

	echo "mpstat (processors usage):"
	mpstat -P ALL 1 3 # 3 reports with 1s interval between them
	echo

	echo "pidstat (cpu usage):"
	pidstat -u 1 3 # 3 reports with 1s interval between them
	echo

	echo "pidstat (ram usage):"
	pidstat -r 1 3 # 3 reports with 1s interval between them
	echo

	echo "done at $(date)"
} >> "$LOG_FILE" 2>&1

if [ -s "$LOG_FILE" ]; then
    echo "log saved as $LOG_FILE"
else
    echo "warning: $LOG_FILE is empty or missing" >&2
    exit 1
fi
