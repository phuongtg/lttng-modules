#!/bin/sh

sudo make KERNELDIR=../linux module_install

sudo rmmod lttng-probe-addons
sudo rmmod lttng-addons
sudo rmmod lttng-wakeup
sudo rmmod lttng-probe-wakeup
sudo modprobe lttng-addons
sudo modprobe lttng-probe-addons
sudo modprobe lttng-wakeup
sudo modprobe lttng-probe-wakeup
sudo dmesg -c
