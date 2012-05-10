#!/bin/bash 

rmmod lttng-user-event
modprobe lttng-user-event
lttng create test
lttng enable-event lttng_uevent_memcpy -k
lttng enable-event lttng_uevent_cfu -k
lttng enable-event lttng_uevent -k
lttng start
echo -n "bidon" > /proc/lttng_user_event
echo -n "bidon" > /proc/lttng_user_event_cfu
echo -n "bidon" > /proc/lttng_user_event_memcpy
lttng stop
lttng view
lttng destroy
