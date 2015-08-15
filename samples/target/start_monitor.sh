#!/bin/sh


HOST=192.168.1.241
PORT=9999

insmod ./ads1015_adc.ko
echo ads1015_adc 0x48 > /sys/bus/i2c/devices/i2c-1/new_device

./monitor_ads1015_adc.py $HOST $PORT






