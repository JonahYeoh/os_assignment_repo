#!/bin/bash
clear
sudo insmod main.ko
echo 'MODULE INSERTED'
dmesg -c
echo 'MODULE REMOVED'
sudo rmmod main.ko
dmesg -c
