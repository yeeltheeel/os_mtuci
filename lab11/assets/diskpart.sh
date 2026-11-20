#!/bin/bash
set -e

LOOP_DIR=/opt/loopdisks
sudo mkdir -p "$LOOP_DIR"

get_loop() {
    losetup -j "$LOOP_DIR/$1.img" | cut -d: -f1
}

create_disks() {
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
    else
        echo "/dev/sdb already exist"
    fi
}

main_task() {
    SDB=$(get_loop sdb)
    SDC=$(get_loop sdc)
    SDD=$(get_loop sdd)
    SDE=$(get_loop sde)
    SDF=$(get_loop sdf)
    SDG=$(get_loop sdg)

    [ -n "$SDB" ] || SDB=/dev/sdb
    [ -n "$SDC" ] || SDC=/dev/sdc
    [ -n "$SDD" ] || SDD=/dev/sdd
    [ -n "$SDE" ] || SDE=/dev/sde
    [ -n "$SDF" ] || SDF=/dev/sdf
    [ -n "$SDG" ] || SDG=/dev/sdg

    # mbr
    sudo wipefs -a "$SDB"
    sudo parted "$SDB" --script mklabel msdos
    sudo parted "$SDB" --script mkpart primary 1MiB 50%
    sudo parted "$SDB" --script mkpart primary 50% 60%
    sudo parted "$SDB" --script mkpart primary 60% 70%
    sudo parted "$SDB" --script mkpart extended 70% 100%
    sudo parted "$SDB" --script mkpart logical 70% 100%
    sudo partprobe "$SDB"

    sudo mkfs.ext4 "${SDB}p1"
    sudo mkdir -p /mnt/disk1
    sudo mount "${SDB}p1" /mnt/disk1

    # gpt
    sudo wipefs -a "$SDC"
    sudo parted "$SDC" --script mklabel gpt
    sudo parted "$SDC" --script mkpart primary xfs 1MiB 2GiB
    sudo mkfs.xfs "${SDC}p1"
    sudo mkdir -p /mnt/disk2
    sudo mount "${SDC}p1" /mnt/disk2
    sudo touch /mnt/disk2/testfile
    df -hT

    sudo umount /mnt/disk2
    sudo parted "$SDC" --script resizepart 1 3GiB
    sudo partprobe "$SDC"
    sudo mount "${SDC}p1" /mnt/disk2
    sudo xfs_growfs /mnt/disk2
    ls -l /mnt/disk2/testfile
    df -hT

    lsblk -a
    sudo fdisk -l

    # raid-array
    sudo apt-get install -y mdadm
    sudo wipefs -a "$SDD" "$SDE"
    sudo mdadm --create /dev/md0 -l 1 -n 2 "$SDD" "$SDE"
    sudo mdadm -D /dev/md0
    cat /proc/mdstat
    sudo mdadm --detail --scan | sudo tee -a /etc/mdadm.conf

    # fs info
    df -hT
    df -i

    # extra
    sudo pvcreate "$SDF"
    sudo vgcreate myvg "$SDF"
    sudo lvcreate -l 50%FREE -n lv1 myvg
    sudo lvcreate -l 100%FREE -n lv2 myvg
    sudo mkfs.xfs /dev/myvg/lv1
    sudo mkfs.xfs /dev/myvg/lv2
    sudo mkdir -p /mnt/lv1 /mnt/lv2
    sudo mount /dev/myvg/lv1 /mnt/lv1
    sudo mount /dev/myvg/lv2 /mnt/lv2
    df -h; pvs; vgs; lvs

    sudo pvcreate "$SDG"
    sudo vgextend myvg "$SDG"
    sudo lvextend -l +100%FREE /dev/myvg/lv1
    sudo xfs_growfs /mnt/lv1

    df -h
    pvs
    lvs
    vgs
}

clean_all() {
    sudo umount /mnt/disk1 /mnt/disk2 /mnt/lv1 /mnt/lv2 2>/dev/null || true

    sudo mdadm --stop /dev/md0 2>/dev/null || true
    sudo mdadm --remove /dev/md0 2>/dev/null || true

    sudo vgchange -an myvg 2>/dev/null || true
    sudo vgremove -f myvg 2>/dev/null || true

    for img in "$LOOP_DIR"/sdb.img "$LOOP_DIR"/sdc.img "$LOOP_DIR"/sdd.img "$LOOP_DIR"/sde.img "$LOOP_DIR"/sdf.img "$LOOP_DIR"/sdg.img; do
        loop=$(losetup -j "$img" | cut -d: -f1)
        [ -n "$loop" ] && sudo losetup -d "$loop" || true
    done

    sudo rm -f /opt/loopdisks/{sdb,sdc,sdd,sde,sdf,sdg}.img

    echo "--- loop-devices left: ---"
    losetup -l
}

case "$1" in
    create) create_disks ;;
    main)   main_task ;;
    clean)  clean_all ;;
    *)
        echo "usage: $0 {create|main|clean}"
        exit 1
        ;;
esac
