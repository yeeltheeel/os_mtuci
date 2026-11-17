#!/bin/bash

sudo apt update
sudo apt upgrade
sudo apt install memtester

# virtual memory map
pmap -x $(pgrep gnome-shell)

# swapfile 
free -h
sudo fallocate -l 1G /swapfile # or sudo dd if=/dev/zero of=/swapfile bs=1M count=1024
sudo chmod 600 /swapfile
sudo mkswap /swapfile
sudo swapon /swapfile
swapon --show # free -h
sudo subl /etc/fstab
# add /swapfile none swap sw 0 0
sudo reboot -i # system reboot
free -h -t 
swapon –-show # -s is deprecated
sudo swapoff /swapfile
sudo rm /swapfile
sudo subl /etc/fstab
# comment out last line
free -h

# memtester
sudo memtester 1024 2 2>&1 | tee memtester_results.log
cat memtester_results.log

# swapiness
cat /proc/sys/vm/swappiness
sudo sysctl vm.swappiness=10
cat /proc/sys/vm/swappiness
sudo sysctl -w vm.swappiness=60 # default
