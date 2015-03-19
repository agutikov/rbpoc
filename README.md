
Intel Edison HOWTO
==================


### Resources
[Intel: Edison docs and downloads](http://www.intel.com/support/maker/edison.htm#documents)


### Login
Empty password
	edison login: root


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

#### uImage

#### modify kernel config and save changes



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












### Recovery mode
No relevant info.
[Intel: community forum: How to recover Edison firmware](https://communities.intel.com/thread/55187)















