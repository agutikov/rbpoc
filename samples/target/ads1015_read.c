
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <stdint.h>
#include <sys/ioctl.h>

#include <kernel/ads1015/ads1015_adc.h>

uint16_t buffer[10000];

struct ads1015_adc_frame frame;


int main (int argc, const char* argv[])
{
	
	int fd = open("/dev/ADS1015_ADC", O_RDONLY);
	
	if (fd < 0) {
		printf("Error opening file\n");
		return -1;
	} else {
		printf("open returns\n");
	}
	
	printf("ADS1015_IOCTL_SWITCH == 0x%08X\n", ADS1015_IOCTL_SWITCH);
	
	struct timespec dt = {.tv_sec = 0, .tv_nsec = 500*1000*1000};
	
	while (1) {
		nanosleep(&dt, 0);
		
		int result = ioctl(fd, ADS1015_IOCTL_SWITCH, &frame);
		printf("ioctl: %d\n", result);
		
		int length = read(fd, buffer, sizeof(buffer));
		printf("readed: %d %d\n", length, length / sizeof(buffer[0]));
		
		for (int i = 0; i < frame.count; i++) {
//			printf("%d\n", buffer[i]);
		}
		
		if (!result) {
			printf("start: %lld\n", frame.start_timestamp);
			printf("duration: %lld\n", frame.duration);
			printf("count: %d\n", frame.count);
			printf("overflow: %d\n", frame.overflow);
		}
		
		printf("\n");
	}
		
	
	return 0;
}

