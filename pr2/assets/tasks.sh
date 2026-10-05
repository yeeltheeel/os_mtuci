#!/bin/bash +x

trap 'printf "\e[32m%s@%s\e[0m:~$ %s\n" "$USER" "$HOSTNAME" "$BASH_COMMAND"' DEBUG

file /sbin/init # обычно ссылка на systemd
systemctl --version # версия systemd
ps -p 1 -o comm= # имя процесса с PID 1:
cat /proc/cmdline #  параметры командной строки, переданные ядру
systemd-analyze time # или просто systemd-analyze
systemd-analyze plot > ~/boot.svg # визуализация графика загрузки
ls -l ~/boot.svg # проверка создания файла
lsmod | head -10 # список загруженных модулей ядра (первые 10)

ls /boot/vmlinuz* | sort -V # список доступных ядер в системе
sudo head -30 /boot/grub/grub.cfg # структура файла конфигурации GRUB
sudo grep "menuentry" /boot/grub/grub.cfg | grep -v "if" | grep -v "export" # список всех пунктов меню GRUB
sudo grep "menuentry" /boot/grub/grub.cfg | grep -v "if" | grep -v "export" | cut -d "'" -f2 # только названия пунктов меню
grub-install --version # версия GRUB
cat /etc/default/grub # основные настройки загрузчика
grep GRUB_TIMEOUT /etc/default/grub # время показа меню
grep GRUB_DEFAULT /etc/default/grub # пункт по умолчанию

systemd-analyze # общее время загрузки (ядро + userspace
systemd-analyze blame | head -10 # время инициализации каждого сервиса
systemd-analyze critical-chain # критическую цепочку загрузки
systemctl list-units --type=service | head -15 # список всех сервисов и их состояние

sudo ls -lh /boot/initrd.img-$(uname -r) # информацию о файле initramfs
sudo lsinitramfs /boot/initrd.img-$(uname -r) | head -20 # содержимое initramfs (первые 20 строк)
sudo lsinitramfs /boot/initrd.img-$(uname -r) | grep -E "\.ko$" | head -10 # драйверы внутри initramfs
sudo lsinitramfs /boot/initrd.img-$(uname -r) | grep "scripts" # скрипты инициализации внутри initramfs

uptime -s # время начала текущей загрузки
who -b
journalctl -b | less # все логи текущей загрузки
# только сообщения ядра
dmesg | head -30 # первые 30 строк
dmesg | grep -i "error\|warn" # поиск ошибок/предупреждений
journalctl -b -p err # только логи с ошибками (приоритет err и выше)
journalctl -b -u systemd-networkd # логи конкретного сервиса (например, network)
journalctl -b | grep "Mount.*root" # момент монтирования корневой файловой системы в логах

lsmod | wc -l # количество загруженных модулей
mount | grep "^/dev" # смонтированных файловых системах (только блочные устройства)
df -h # использовании дискового пространства
ip addr show # сетевых интерфейсах
ps -e --no-headers | wc -l # количество запущенных процессов

systemctl list-unit-files --type=target # список всех доступных targets
systemctl get-default # текущий target по умолчанию
systemctl list-dependencies multi-user.target | head -20 # сервисы, необходимые для multi-user.target
systemctl list-dependencies graphical.target | head -20 # сервисы, необходимые для graphical.target
systemctl list-units --type=target --state=active # текущий активный target 