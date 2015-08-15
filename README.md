
Intel Edison HOWTO
==================


### Resources
[Intel: Edison docs and downloads](http://www.intel.com/support/maker/edison.htm#documents)


### Power on and Login
Empty password
	edison login: root

[Intel: Arduino board USB and power](https://software.intel.com/en-us/articles/intel-edison-arduino-expansion-board-assembly)

### WiFi

	vi /etc/wpa_supplicant/wpa_supplicant.conf  
	systemctl restart wpa_supplicant

Или:

	systemctl stop hostapd  
	systemctl disable hostapd  
	systemctl enable wpa_supplicant  
	systemctl start wpa_supplicant  
	wpa_cli reconfigure  
	wpa_cli select_network wlan0  
	udhcpc -i wlan0  

From [fab-lab.eu/edison](http://fab-lab.eu/edison/)


### TFTP

Download  
	tftp -g -r <filename> <ip-addr>

Upload  
	tftp -p -r <filename> <ip-addr>

From: [tftp download and upload command](https://rathodpratik.wordpress.com/2012/11/15/usage-of-tftp-server-to-transfer-files/)


### Arduino board buttons and jumpers
[Intel: community article: Buttons and Switch](https://communities.intel.com/docs/DOC-23454)


### Build Yocto

[working git repo](https://github.com/agutikov/edison-src)

* [Intel: official manual](http://www.intel.com/support/edison/sb/CS-035278.htm)
* [ShawnHymel: Creating a Custom Linux Kernel for the Edison](http://shawnhymel.com/585/creating-a-custom-linux-kernel-for-the-edison/)

#### Image
	./device-software/setup.sh --dl_dir=/home/user/work/edison/bb_downloads/ \
	--sstate_dir=/home/user/work/edison/bb_sstate/ --bb_number_thread=4 --parallel_make=4

	source poky/oe-init-build-env

	bitbake edison-image

	../device-software/utils/flash/postBuild.sh

	ls build/toFlash

#### SDK
	source poky/oe-init-build-env  
	bitbake edison-image -c populate_sdk  
	ls tmp/deploy/sdk/  

Populate SDK script:

	poky/meta/classes/populate_sdk_base.bbclass

Environment settings:

	source /opt/poky-edison/1.6.1/environment-setup-core2-32-poky-linux

Or just PATH, CROSS_COMPILE, ARCH and SYSROOT:

	export ARCH=x68
	export PATH=$PATH:/opt/poky-edison/1.6.1/sysroots/x86_64-pokysdk-linux/usr/bin/i586-poky-linux
	export CROSS_COMPILE=i586-poky-linux-
	export SYSROOT=/opt/poky-edison/1.6.1/sysroots/core2-32-poky-linux

	${CROSS_COMPILE}gcc --sysroot=${SYSROOT}

#### Linux kernel

poky/meta/recipes-kernel/linux
poky/meta-skeleton/recipes-kernel/linux
poky/meta-yocto-bsp/recipes-kernel/linux

device-software/meta-edison/recipes-kernel/linux/

build/tmp/work/edison-poky-linux/linux-yocto/


	bitbake virtual/kernel -c menuconfig
or
	bitbake linux-yocto -c menuconfig
and then
	cp ./tmp/work/edison-poky-linux/linux-yocto/3.10.17+gitAUTOINC+6ad20f049a_c03195ed6e-r0/linux-edison-standard-build/.config \
	../device-software/meta-edison/recipes-kernel/linux/files/defconfig

bitbake linux-yocto -c deploy
build/tmp/deploy/images/edison/bzImage
build/tmp/deploy/images/edison/modules-edison.tgz

https://software.intel.com/en-us/blogs/2015/02/27/intel-edison-adding-kernel-modules-to-yocto-example-batman

../device-software/meta-edison/recipes-kernel/linux/files/defconfig

./tmp/work/edison-poky-linux/linux-yocto/defconfig
./tmp/work/edison-poky-linux/linux-yocto/3.10.17+gitAUTOINC+6ad20f049a_c03195ed6e-r0/linux/arch/x86/configs/i386_edison_defconfig

Нафиг дублировать defconfig в upstream_to_edison.patch ?


./tmp/work/edison-poky-linux/linux-yocto/3.10.17+gitAUTOINC+6ad20f049a_c03195ed6e-r0/linux-edison-standard-build/.config

do_kernel_configme
do_configure
CONFIG_LOCALVERSION
poky/meta/classes/kernel-yocto.bbclass
device-software/meta-edison/recipes-kernel/linux/linux-yocto_3.10.bbappend

do_menuconfig
poky/meta/classes/cml1.bbclass

export LINUX_EDISON_BUILD_DIR="/home/dtest/edison/tmp/edison_manual_build/build/tmp/work/edison-poky-linux/linux-yocto/3.10.17-r0/linux-edison-standard-build/"
make -C $LINUX_EDISON_BUILD_DIR  M=`pwd`

#### U-boot

poky/meta/recipes-bsp/u-boot

device-software/meta-edison/recipes-bsp/u-boot

build/tmp/work/edison-poky-linux/u-boot

	edit device-software/meta-edison/recipes-bsp/u-boot/u-boot-internal.inc
	add u-boot_edison_config.patch

	bitbake u-boot -c clean  
	bitbake u-boot -c configure  
	cd tmp/work/edison-poky-linux/u-boot/2014.04-1-r0/git  
	git add .  
	git commit -am 'upstream_to_edison.patch'  

	edit include/configs/edison.h

	git diff > ../../../../../../../device-software/meta-edison/recipes-bsp/u-boot/files/u-boot_edison_config.patch



#### uImage




### Reflash, upgrade

Install dfu-util.

Run flashall.sh as root.


### NFS

	mount -t nfs -o nolock <host_ip>:<host_path>



#### add nfs-utils packet to image

device-software/meta-edison-distro/recipes-core/images/edison-image.bb  
	IMAGE_INSTALL += "nfs-utils"

#### nfsrootfs

TODO: u-boot usb-ethernet

	sudo mount -t ext4 -o loop edison-src_15/build/toFlash/edison-image-edison.ext4 ./nfs/rootfs/



#### Arduino board ADC

[ads7951](http://www.ti.com/product/ads7951)
Connected to spi0, _CS is gpio110.

Start with libmraa.



#### GPIO

70-pin connector:
[Intel: Edison Compute Module Hardware Guide](http://www.intel.com/support/edison/sb/CS-035274.htm)

Arduino board connector:
[Emutex Lab: Edison GPIO Pin Multiplexing Guide](http://www.emutexlabs.com/project/215-intel-edison-gpio-pin-multiplexing-guide)



Also start with libmraa.


#### libmraa

[Intel: MRAA library doxygen](http://iotdk.intel.com/docs/master/mraa/pages.html)

[Intel mraa official github](https://github.com/intel-iot-devkit/mraa)



### Recovery mode

https://communities.intel.com/docs/DOC-23454

https://communities.intel.com/message/257632

[Intel: community forum: How to recover Edison firmware](https://communities.intel.com/thread/55187)

https://github.com/pokey9000/edbian/wiki

https://communities.intel.com/message/257193






### I2C and Sparkfun ADC shield

http://elinux.org/Interfacing_with_I2C_Devices

https://github.com/modmaker/BeBoPr-plus-plus/wiki/ADS1015-ADC-Access

drivers/hwmon/ads1015.c



insmod ./ads1015_adc.ko
echo ads1015_adc 0x48 > /sys/bus/i2c/devices/i2c-1/new_device

echo 0x48 > /sys/bus/i2c/devices/i2c-1/delete_device
rmmod ads1015_adc






