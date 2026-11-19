#!/bin/bash
set -e

LOOP_DIR=/opt/loopdisks
sudo mkdir -p "$LOOP_DIR"

create_loop() {
    local name=$1 size=$2
    local img="$LOOP_DIR/$name.img"
    if [ ! -f "$img" ]; then
        sudo dd if=/dev/zero of="$img" bs=1M count="$size" status=none
    fi
    sudo losetup -fP "$img"
}

if [ ! -b /dev/sdb ]; then
    create_loop sdb 2048
    create_loop sdc 3072 
    create_loop sdd 1024 
    create_loop sde 1024 
    create_loop sdf 1024 
    create_loop sdg 1024 
    echo "loop disks created:"
    losetup -l
    echo "warning: loop-devices (loop0, loop1...) may not match sdb..sdg."
fi

# new disk
sudo wipefs -a /dev/sdb
sudo parted /dev/sdb --script mklabel msdos # mbr
sudo parted /dev/sdb --script mkpart primary 1MiB 50%
sudo parted /dev/sdb --script mkpart primary 50% 60%
sudo parted /dev/sdb --script mkpart primary 60% 70%
sudo parted /dev/sdb --script mkpart extended 70% 100%
sudo parted /dev/sdb --script mkpart logical 70% 100%
sudo partprobe /dev/sdb

sudo wipefs -a /dev/sdc
sudo parted /dev/sdc --script mklabel gpt # gpt
sudo parted /dev/sdc --script mkpart primary xfs 1MiB 2GiB
sudo mkfs.xfs /dev/sdc1

sudo mkfs.ext4 /dev/sdb1
sudo mkdir -p /mnt/disk1
sudo mount /dev/sdb1 /mnt/disk1
sudo mkdir -p /mnt/disk2
sudo mount /dev/sdc1 /mnt/disk2
sudo touch /mnt/disk2/testfile
df -hT

sudo mkfs.ext4 /dev/sdb1
sudo mkdir -p /mnt/disk1
sudo mount /dev/sdb1 /mnt/disk1
sudo umount /mnt/disk2
sudo parted /dev/sdc --script resizepart 1 3GiB
sudo mount /dev/sdc1 /mnt/disk2
sudo xfs_growfs /mnt/disk2
ls -l /mnt/disk2/testfile
df -hT

lsblk -a 
sudo fdisk -l

# raid-array
sudo apt-get install -y mdadm
sudo wipefs -a /dev/sdd /dev/sde
sudo mdadm --create /dev/md0 -l 1 -n 2 /dev/sd{d,e} 
sudo mdadm -D /dev/md0
#cat /proc/mdstat
#subl /etc/mdadm.conf

# fs info
#cat /proc/filesystems
df -hT
df -i

# extra
sudo pvcreate /dev/sdf
sudo vgcreate myvg /dev/sdf
sudo lvcreate -l 50%FREE -n lv1 myvg
sudo lvcreate -l 100%FREE -n lv2 myvg
sudo mkfs.xfs /dev/myvg/lv1
sudo mkfs.xfs /dev/myvg/lv2
sudo mkdir -p /mnt/lv1 /mnt/lv2
sudo mount /dev/myvg/lv1 /mnt/lv1
sudo mount /dev/myvg/lv2 /mnt/lv2
df -h; pvs; vgs; lvs

sudo pvcreate /dev/sdg
sudo vgextend myvg /dev/sdg
sudo lvextend -l +100%FREE /dev/myvg/lv1
sudo xfs_growfs /mnt/lv1

df -h
pvs
lvs
vgs
