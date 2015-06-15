#ifndef _ADS1015_ADC_
#define _ADS1015_ADC_

#include <linux/types.h>


struct ads1015_adc_sample {
	__s64 dt;
	__u16 value;
};



















#endif
