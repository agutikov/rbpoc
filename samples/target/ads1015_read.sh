#!/bin/sh

LOGFILE=${1}


# trap "echo SIGHUP >> ${LOGFILE};" SIGHUP 
# trap "echo SIGINT >> ${LOGFILE};" SIGINT
# trap "echo SIGTERM >> ${LOGFILE};" SIGTERM


while true; do

	date >> ${LOGFILE}

	for i in 0 1 2 3 ; do
        
		case "$i" in
	        "0") CFG="0x00c5" ;;
        	"1") CFG="0x00d5" ;;
	        "2") CFG="0x00e5" ;;
	        "3") CFG="0x00f5" ;;
        	esac
        
		i2cset -y 1 0x48 1 $CFG w
        
		RES=`i2cget -y 1 0x48 1 w`
        
		until [ "$RES" = "$CFG" ] ; do
                	RES=`i2cget -y 1 0x48 1 w`
        	done
        
		X=`i2cget -y 1 0x48 0 w`
        
		XX="0x0`echo $X | cut -c 5-6``echo $X | cut -c 3`"
	        printf "AIN$i = %5d mV; " $XX >> ${LOGFILE}
	done

printf "\n" >> ${LOGFILE}

done
