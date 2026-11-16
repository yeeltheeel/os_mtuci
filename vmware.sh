sudo apt update
sudo apt install open-vm-tools open-vm-tools-desktop

sudo mkdir -p /mnt/hgfs
sudo vmhgfs-fuse .host:/ /mnt/hgfs -o allow_other
sudo nano /etc/fstab
# .host:/ /mnt/hgfs fuse.vmhgfs-fuse auto,allow_other 0 0
# ctrl+o, enter, ctrl+x

sudo mount -a
ls -la /mnt/hgfs

ln -s /mnt/hgfs/my_ubuntu ~/Desktop


