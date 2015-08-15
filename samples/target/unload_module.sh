#!/bin/sh


echo 0x48 > /sys/bus/i2c/devices/i2c-1/delete_device
rmmod ads1015_adc



