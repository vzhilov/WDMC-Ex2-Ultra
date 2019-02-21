#!/bin/sh

if [ -e /decrypt_key ]; then
	echo "Please, input decrypt key:"
	read -sp KEY
	echo -n $KEY > /decrypt_key
fi

CRYPTDEV=$(blkid -l -o device -t TYPE=crypto_LUKS)
echo "export CRYPTDEV=$CRYPTDEV" >> /etc/profile
cryptsetup luksOpen $CRYPTDEV cryptroot --key-file /decrypt_key

lvm vgscan --mknodes
lvm vgchange -a ly
lvm vgscan --mknodes
wait

if ! [ -e /dev/mapper/rootfs ]; then
	echo "Something wrong! /dev/mapper/rootfs not exist!"
	exit 1
fi
