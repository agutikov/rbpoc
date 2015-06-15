
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>

#include <kernel/ads1015/ads1015_adc.h>


struct ads1015_adc_sample buffer[10000];


int main (int argc, const char* argv[])
{
	
	int fd = open("/dev/ADS1015_ADC", O_RDONLY);
	
	if (fd < 0) {
		printf("Error opening file\n");
		return -1;
	}
	
	struct timespec dt = {.tv_sec = 0, .tv_nsec = 500*1000*1000};
	
	while (1) {
		nanosleep(&dt, 0);
		
		int length = read(fd, buffer, sizeof(buffer));
		
		printf("readed: %d %d\n", length, length / sizeof(buffer[0]));
	}
		
	
	return 0;
}

