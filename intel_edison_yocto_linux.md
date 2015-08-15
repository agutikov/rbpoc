
================================================================================
GNU/LINUX ROOTFS IMAGE MANUAL BUILD, ON LINUX HOST, FOR INTEL EDISON, WITH YOCTO
================================================================================
UDERSTANDABLE MANUAL
====================


PREREQUISITES
-------------

*** Overview

yocto
bitbake (poratge, bsd ports => yocto is successor of bsd - be proud, you are compiling bsd :)
poky
intel edison layers

*** Documentation

yocto, bitbake, poky

*** Requirements

git
dfu-util


GETTING BUILD RECEPIES, SCRIPTS AND BUILD CONFIGS
-------------------------------------------------

*** Layers

Unfortunately, still each layer is not in separate git repo - so 
we have to combine layers from different sources.
Good news are that for bitbake there is no difference where layer is
placed in filesystem, main is it should be correctly added 
by absolute path in config.

git://git.yoctoproject.org/poky
  meta
  meta-yocto
  meta-yocto-bsp

git://git.yoctoproject.org/meta-intel-edison
  meta-intel-edison-bsp
  meta-intel-edison-distro
  meta-intel-arduino

git://git.yoctoproject.org/meta-intel-iot-middleware

https://github.com/01org/meta-arduino.git

**** Optional layers

git://git.yoctoproject.org/meta-mingw
git://git.yoctoproject.org/meta-darwin

*** Clone repos with layers

	mkdir git
	cd git
	git clone git://git.yoctoproject.org/poky
	git clone git://git.yoctoproject.org/meta-intel-edison
	git clone git://git.yoctoproject.org/meta-intel-iot-middleware
	git clone https://github.com/01org/meta-arduino.git
	cd ../


WHAT IS WHAT HERE
-----------------

`meta-intel-edison` packed into edison-src-ww18-15.tgz
So download `edison-src-ww18-15.tgz` and clone `meta-intel-edison`
are practically the same, except cloning bring you tha latest version,
and possibility to checkout any other.
While donwloading left you only one single version.
So IMHO git is always better.

`meta-intel-edison` is a single entry point for beginners 
with scripts that makes first build pretty easy.
To start compile ASAP, please, read `meta-intel-edison/README`.
But I think that detailed practical manual (like gentoo handbook)
is much better, because let you understand 
how build works on allmost all levels.
After you satisfy your curiosuty scripts
like `meta-intel-edison/setup.sh` and
`meta-intel-edison/utils/Makefile.mk` became very usefull.

*** poky

*** meta-intel-iot-middleware

*** meta-intel-edison

*** meta-arduino


CONFIGURE BUILD ENVIRONMENT
---------------------------

*** Checkout specific branches

meta-intel-edison have no branches, master is already checked out

	cd poky
	git checkout yocto-1.7.2

	cd ../meta-intel-iot-middleware
	git checkout c6d681475e76107e6c04c5f7a06034dc9e772d1e

	cd ../meta-arduino
	git checkout 1.6.x

	cd ../

*** Find layers

Now we have next 8 layers:
./poky/meta
./poky/meta-yocto
./poky/meta-yocto-bsp
./meta-intel-edison/meta-intel-edison-bsp
./meta-intel-edison/meta-intel-edison-distro
./meta-intel-edison/meta-intel-arduino
./meta-intel-iot-middleware
./meta-arduino

*** Cretae bitbake cache dirs

	mkdir -p bbcache/downloads
	mkdir bbcache/sstate-cache

*** Create build configs

	mkdir -p build/conf
	touch build/conf/local.conf
	touch build/conf/bblayers.conf
	touch build/conf/templateconf.cfg

Links to configs documentation:
* local.conf
* bblayers.conf
* templateconf.cfg

1. create default configs and then update configs ...

2. Copy configs from here


BUILD YOCTO IMAGE
-----------------

	source ./poky/oe-init-build-env ./build

	bitbake edison-image

What is where now?


PREPARE FLAHSABLE IMAGES
------------------------

	../meta-intel-edison/utils/flash/postBuild.sh

	find ./toFlash/

FLASH EDISON
------------

Install dfu-util.

	cd ./toFlash/

	sudo ./flashall.sh






























