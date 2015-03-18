


### Login
	


### WiFi
	http://fab-lab.eu/edison/
`
	vi /etc/wpa_supplicant/wpa_supplicant.conf
	systemctl stop hostapd
	systemctl disable hostapd
	systemctl enable wpa_supplicant
	systemctl start wpa_supplicant
	wpa_cli reconfigure
	wpa_cli select_network wlan0
	udhcpc -i wlan0
`

### TFTP
	https://rathodpratik.wordpress.com/2012/11/15/usage-of-tftp-server-to-transfer-files/


### Arduino board buttons and jumpers
	https://communities.intel.com/docs/DOC-23454


















