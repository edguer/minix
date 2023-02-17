#!/bin/bash
qemu-system-i386 --enable-kvm -m 1024 -hda /home/edguer/Projects/minix/minix_x86.img &
sleep 1
wmctrl -r :ACTIVE: -e 0,2100,1,620,480
