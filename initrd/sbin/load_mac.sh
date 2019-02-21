#!/bin/sh

# WDMC Mirror Gen2 / Ex2 Ultra u-boot vars fs on internal nand
mkdir -p /mnt/wd_vars
ubiattach /dev/ubi_ctrl -m 7
wait
mount -t ubifs ubi0:reserve2 /mnt/wd_vars
ifconfig eth0 hw ether $(cat /mnt/wd_vars/mac_addr)
umount /mnt/wd_vars

