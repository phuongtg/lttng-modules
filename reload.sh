#!/bin/sh

sudo make KERNELDIR=../linux modules_install

sudo rmmod lttng-probe-addons
sudo rmmod lttng-addons
sudo rmmod lttng-backtrace-mod
sudo rmmod lttng-probe-backtrace

sudo modprobe lttng-probe-addons
sudo modprobe lttng-addons
sudo modprobe lttng-probe-backtrace
sudo modprobe lttng-backtrace-mod

sudo dmesg -c
