#ifndef _ADS1015_ADC_
#define _ADS1015_ADC_

#include <linux/types.h>
#include <linux/major.h>
#include <linux/ioctl.h>


struct ads1015_adc_frame {
	uint64_t start_timestamp;
	uint64_t duration;
	uint32_t count;
	uint32_t overflow;
};

#define ADS1015_IOC_MAGIC		's'
// ADS1015_IOC_MAGIC or MISC_MAJOR ?
#define ADS1015_IOCTL_SWITCH	_IOWR(ADS1015_IOC_MAGIC, 13, struct ads1015_adc_frame *)
















#endif
