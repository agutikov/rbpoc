


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


### Build yocto image
[Intel: official manual](http://www.intel.com/support/edison/sb/CS-035278.htm)
[ShawnHymel: Creating a Custom Linux Kernel for the Edison](http://shawnhymel.com/585/creating-a-custom-linux-kernel-for-the-edison/)

### Reflash, upgrade
Simply run flashall.sh as root.


### Recovery mode
No relevant info.
[Intel: community forum: How to recover Edison firmware](https://communities.intel.com/thread/55187)















