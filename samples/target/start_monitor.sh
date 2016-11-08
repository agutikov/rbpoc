#!/bin/sh


# HOST=192.168.128.190
HOST=192.168.1.241
PORT=9999

insmod ./ads1015_adc.ko
echo ads1015_adc 0x48 > /sys/bus/i2c/devices/i2c-1/new_device

echo 6 > /sys/bus/i2c/drivers/ads1015_adc/1-0048/ads1015_adc_config/channel
echo 1 > /sys/bus/i2c/drivers/ads1015_adc/1-0048/ads1015_adc_config/gain
echo 3 > /sys/bus/i2c/drivers/ads1015_adc/1-0048/ads1015_adc_config/rate



./monitor_ads1015_adc.py $HOST $PORT






