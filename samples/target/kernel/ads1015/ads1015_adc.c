

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/ktime.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/kthread.h>
#include <linux/timer.h>
#include <linux/sched.h>
#include <asm/uaccess.h>

#include <ads1015_adc.h>

/* ADS1015 registers */
enum {
	ADS1015_CONVERSION = 0,
	ADS1015_CONFIG = 1,
};

/* PGA fullscale voltages in mV */
static const unsigned int fullscale_table[8] = {
	6144, 4096, 2048, 1024, 512, 256, 256, 256 };

/* Data rates in samples per second */
static const unsigned int data_rate_table[8] = {
	128, 250, 490, 920, 1600, 2400, 3300, 3300 };

#define ADS1015_ADC_DEFAULT_CHANNEL 4
#define ADS1015_ADC_DEFAULT_GAIN 0
#define ADS1015_ADC_DEFAULT_RATE 6
#define ADS1015_ADC_DEFAULT_COMP_MODE 0
#define ADS1015_ADC_DEFAULT_COMP_POL 0
#define ADS1015_ADC_DEFAULT_COMP_LAT 0
#define ADS1015_ADC_DEFAULT_COMP_QUE 3

struct ads1015_adc_buffer {
	__u16 *buffer;
	size_t size;
	size_t count;
	size_t read_count;
	ktime_t first;
	ktime_t last;
};

struct ads1015_adc_data {
	// i2c slave device
	struct i2c_client *client;

	// i2c device configuration
	u8 channel;
	u8 gain;
	u8 rate;
	u8 comp_mode:1;
	u8 comp_pol:1;
	u8 comp_lat:1;
	u8 comp_que;

	// conversion thread
	struct task_struct *task;
	u8 run;
	u8 stopped;
//	struct timer_list timer;

	// adc samples queue
	struct ads1015_adc_buffer *current_buffer;
	struct ads1015_adc_buffer buffers[2];
	u8 switch_flag;
	uint32_t overflow;

	// interface - misc device
	struct miscdevice misc_dev;
};

u16 ads1015_adc_data_to_config_regval (struct ads1015_adc_data *data)
{
	u16 config = 0;
	config |= (data->channel & 0x0007) << 12;
	config |= (data->gain & 0x0007) << 9;
	config |= (data->rate & 0x0007) << 5;
	config |= (data->comp_mode & 0x01) << 4;
	config |= (data->comp_pol & 0x01) << 3;
	config |= (data->comp_lat & 0x01) << 2;
	config |= data->comp_que & 0x03;

	return config;
}

#if 1
int thread_convertion (void *arg)
{
	int res;
	struct ads1015_adc_data *data = (struct ads1015_adc_data *) arg;
	uint16_t sample;
	u32 delay_us;

	data->current_buffer->first = ktime_get();

	printk("thread_convertion start\n");

	while (data->run) {
		res = i2c_smbus_read_word_swapped(data->client, ADS1015_CONVERSION);
		if (res < 0) {
			printk("read conv error\n");
			break;
		} else {
			sample = (u16)res;
		}
		data->current_buffer->last = ktime_get();

		if (data->current_buffer->count == data->current_buffer->size) {
			data->overflow++;
		} else {
			data->current_buffer->buffer[data->current_buffer->count] = sample;
			data->current_buffer->count++;
		}

		if (data->switch_flag) {
			if (data->current_buffer == &data->buffers[0]) {
				data->current_buffer = &data->buffers[1];
			} else {
				data->current_buffer = &data->buffers[0];
			}
			data->switch_flag = 0;
			data->current_buffer->first = ktime_get();
			data->current_buffer->count = 0;
			data->current_buffer->read_count = 0;
		}

		delay_us = 1000000 / data_rate_table[data->rate];
		udelay(delay_us - 190);
	}
	data->stopped = 1;
	printk("thread_convertion exit\n");

	return 0;
}
#endif

#if 0
void conversion_timer_call (unsigned long arg)
{
	int res;
	struct ads1015_adc_data *data = (struct ads1015_adc_data *) arg;
	uint16_t sample;
	u32 delay_us;

	printk("conversion_timer_call\n");

	res = i2c_smbus_read_word_swapped(data->client, ADS1015_CONVERSION);
	if (res < 0) {
		printk("read conv error\n");
	} else {
		sample = (u16)res;

		data->current_buffer->last = ktime_get();

		if (data->current_buffer->count == data->current_buffer->size) {
			data->overflow++;
		} else {
			data->current_buffer->buffer[data->current_buffer->count] = sample;
			data->current_buffer->count++;
		}
	}

	if (data->switch_flag) {
		if (data->current_buffer == &data->buffers[0]) {
			data->current_buffer = &data->buffers[1];
		} else {
			data->current_buffer = &data->buffers[0];
		}
		data->switch_flag = 0;
		data->current_buffer->first = ktime_get();
		data->current_buffer->count = 0;
		data->current_buffer->read_count = 0;
	}

	delay_us = 1000000 / data_rate_table[data->rate];

	if (data->run) {
		res = mod_timer(&data->timer, jiffies + usecs_to_jiffies(delay_us - 190));
		if (res)
			printk("mod_timer error %d\n", res);
	} else {
		data->stopped = 1;
	}
}
#endif

int ads1015_adc_start_conversion (struct ads1015_adc_data *data)
{
	int res;
	u16 config;
//	u32 delay_us;
	u32 buf_size;

	printk("ads1015_adc_start_conversion\n");

	// alloc new buffers
	buf_size = data_rate_table[data->rate] * 10;
	printk("buffer max length: %d\n", buf_size);

	data->buffers[0].size = buf_size;
	data->buffers[1].size = buf_size;
	data->buffers[0].buffer = kzalloc(data->buffers[0].size * sizeof(data->buffers[0].buffer[0]),
			GFP_KERNEL);
	data->buffers[1].buffer = kzalloc(data->buffers[1].size * sizeof(data->buffers[1].buffer[0]),
			GFP_KERNEL);
	data->buffers[0].count = 0;
	data->buffers[1].count = 0;
	data->buffers[0].read_count = 0;
	data->buffers[1].read_count = 0;
	data->current_buffer = &data->buffers[0];
	data->switch_flag = 0;

	// write config to ads1015
	config = ads1015_adc_data_to_config_regval(data);
	res = i2c_smbus_write_word_swapped(data->client, ADS1015_CONFIG, config);
	if (res < 0) {
		printk(KERN_ERR "ads1015_adc: error i2c write\n");
		return -1;
	}

	// start conversion thread
	data->stopped = 0;
	data->run = 1;
#if 1
	data->task = kthread_run(&thread_convertion, (void *)data, "ads1015");
	if (data->task) {
		wake_up_process(data->task);
	}
#endif
#if 0
	setup_timer(&data->timer, conversion_timer_call, (long unsigned int)data);

	// call timer first time
	delay_us = 1000000 / data_rate_table[data->rate];
	usleep_range(delay_us, delay_us*2);
	data->current_buffer->first = ktime_get();
	conversion_timer_call((long unsigned int)data);
#endif
	return 0;
}


void ads1015_adc_stop_conversion (struct ads1015_adc_data *data)
{
	int sleep_us = 1000000 / data_rate_table[data->rate];
	int i = 3;

	printk("ads1015_adc_stop_conversion\n");

	// stop conversion thread if running
#if 1
	if (data->run) {
		data->run = 0;
		while (!data->stopped && i--) {
			wake_up_process(data->task);
			usleep_range(sleep_us, sleep_us*2);
		}
		if (!i) {
			printk("ads1015_adc_stop_conversion: stop wait timeout\n");
		}
		kthread_stop(data->task);
		data->task = 0;
	}
#endif
#if 0
	if (data->run) {
		data->run = 0;
		while (!data->stopped && i--) {
			usleep_range(sleep_us, sleep_us*2);
		}
		if (!i) {
			printk("ads1015_adc_stop_conversion: stop wait timeout\n");
		}
		del_timer(&data->timer);
	}
#endif
	// cleanup
	if (data->current_buffer) {
		kfree(data->buffers[0].buffer);
		kfree(data->buffers[1].buffer);
		data->buffers[0].count = 0;
		data->buffers[1].count = 0;
		data->buffers[0].read_count = 0;
		data->buffers[1].read_count = 0;
		data->buffers[0].size = 0;
		data->buffers[1].size = 0;
		data->current_buffer = 0;
	}
}


static ssize_t ads1015_adc_attr_show(struct device *dev,
			struct device_attribute *attr, char *buf);

static ssize_t ads1015_adc_attr_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count);

static DEVICE_ATTR(channel, S_IRUGO | S_IWUGO, ads1015_adc_attr_show, ads1015_adc_attr_store);
static DEVICE_ATTR(gain, S_IRUGO | S_IWUGO, ads1015_adc_attr_show, ads1015_adc_attr_store);
static DEVICE_ATTR(rate, S_IRUGO | S_IWUGO, ads1015_adc_attr_show, ads1015_adc_attr_store);

static struct attribute *ads1015_adc_attrs[] = {
	&dev_attr_channel.attr,
	&dev_attr_gain.attr,
	&dev_attr_rate.attr,
	NULL
};

static const struct attribute_group ads1015_adc_gr = {
	.name = "ads1015_adc_config",
	.attrs = ads1015_adc_attrs
};

static ssize_t ads1015_adc_attr_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct ads1015_adc_data *data = i2c_get_clientdata(client);

	if (attr == &dev_attr_channel) {
		return sprintf(buf, "%d\n", data->channel);
	} else if (attr == &dev_attr_gain) {
		return sprintf(buf, "%d\n", data->gain);
	} else if (attr == &dev_attr_rate) {
		return sprintf(buf, "%d\n", data->rate);
	} else {
		return -EINVAL;
	}
}

static ssize_t ads1015_adc_attr_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct ads1015_adc_data *data = i2c_get_clientdata(client);

	long value = 0;
	int res = kstrtol(buf, 0, &value);

	if (!res) {
		if (attr == &dev_attr_channel) {
			data->channel = value & 0x07;
		} else if (attr == &dev_attr_gain) {
			data->gain = value & 0x07;
		} else if (attr == &dev_attr_rate) {
			data->rate = value & 0x07;
		} else {
			return -EINVAL;
		}
	} else {
		return res;
	}

	return count;
}

int ads1015_adc_open (struct inode *inode, struct file *filp)
{
	struct miscdevice *misc_dev = filp->private_data;
	struct ads1015_adc_data *data = container_of(misc_dev, struct ads1015_adc_data, misc_dev);

	int res = ads1015_adc_start_conversion(data);

	return res;
}

int ads1015_adc_release (struct inode *inode, struct file *filp)
{
	struct miscdevice *misc_dev = filp->private_data;
	struct ads1015_adc_data *data = container_of(misc_dev, struct ads1015_adc_data, misc_dev);

	ads1015_adc_stop_conversion(data);

	return 0;
}

ssize_t ads1015_adc_read (struct file *filp, char __user * user_ptr, size_t length, loff_t * offset)
{
	struct miscdevice *misc_dev = filp->private_data;
	struct ads1015_adc_data *data = container_of(misc_dev, struct ads1015_adc_data, misc_dev);
	struct ads1015_adc_buffer *buffer;
	size_t left_adc_data_length;
	size_t count;
	long res;

	if (!data->run) {
		printk("ads1015 adc conversion not running\n");
		return -EINVAL;
	}

	if (data->current_buffer == &data->buffers[0]) {
		buffer = &data->buffers[1];
	} else {
		buffer = &data->buffers[0];
	}

	count = buffer->count - buffer->read_count;
	left_adc_data_length = sizeof(buffer->buffer[0]) * count;

	if (left_adc_data_length > length) {
		count = length / sizeof(buffer->buffer[0]);
		left_adc_data_length = sizeof(buffer->buffer[0]) * count;
	}

	if (count) {
		res = copy_to_user(user_ptr, buffer->buffer, left_adc_data_length);
		if (res > 0) {
			printk("ads1015_adc_read: copy_to_user error\n");
			return left_adc_data_length - res;
		}
	}

#if 0
	for (i = 0; i < buffer->buffer_count; i++) {
		printk("%12lld %6d\n", buffer->buffer[i].dt, buffer->buffer[i].value);
	}
	printk("count: %d\n", buffer->buffer_count);
	printk("overflow: %d\n", data->overflow);
#endif

	buffer->read_count += count;

	return left_adc_data_length;
}

long ads1015_adc_ioctl (struct file* filp, unsigned int ioctl_num, unsigned long ioctl_parm)
{
	struct miscdevice *misc_dev = filp->private_data;
	struct ads1015_adc_data *data = container_of(misc_dev, struct ads1015_adc_data, misc_dev);
	struct ads1015_adc_buffer *buffer;
	int i = 3;
	int sleep_us = 1000000 / data_rate_table[data->rate];
	struct ads1015_adc_frame frame;
	void __user *argp = (void __user *)ioctl_parm;

	switch (ioctl_num) {

	case ADS1015_IOCTL_SWITCH:

		data->switch_flag = 1;
		while (data->switch_flag) {
			usleep_range(sleep_us, sleep_us*2);
			i--;
			if (!i) {
				printk("ads1015_adc_ioctl: switch wait timeout\n");
				return -EAGAIN;
			}
		}

		if (data->current_buffer == &data->buffers[0]) {
			buffer = &data->buffers[1];
		} else {
			buffer = &data->buffers[0];
		}

		frame.count = buffer->count;
		frame.overflow = data->overflow;
		frame.start_timestamp = buffer->first.tv64;
		frame.duration = buffer->last.tv64 - buffer->first.tv64;

		data->overflow = 0;

		if (copy_to_user(argp, &frame, sizeof(frame)))
			return -EFAULT;

		break;

	default:
		return -EINVAL;
	}

	return 0;
}

struct file_operations misc_fops = {
	.owner = THIS_MODULE,
	.open = ads1015_adc_open,
	.release = ads1015_adc_release,
	.unlocked_ioctl = ads1015_adc_ioctl,
	.read = ads1015_adc_read,
};

int ads1015_adc_probe( struct i2c_client *client, const struct i2c_device_id *id)
{
	struct ads1015_adc_data *data;
	int err;

	data = devm_kzalloc(&client->dev, sizeof(struct ads1015_adc_data), GFP_KERNEL);
	if (!data) {
		return -ENOMEM;
	}

	i2c_set_clientdata(client, data);
	data->client = client;

	data->channel = ADS1015_ADC_DEFAULT_CHANNEL;
	data->gain = ADS1015_ADC_DEFAULT_GAIN;
	data->rate = ADS1015_ADC_DEFAULT_RATE;
	data->comp_mode = ADS1015_ADC_DEFAULT_COMP_MODE;
	data->comp_pol = ADS1015_ADC_DEFAULT_COMP_POL;
	data->comp_lat = ADS1015_ADC_DEFAULT_COMP_LAT;
	data->comp_que = ADS1015_ADC_DEFAULT_COMP_QUE;

	data->misc_dev.minor = MISC_DYNAMIC_MINOR;
	data->misc_dev.name = "ADS1015_ADC";
	data->misc_dev.fops = &misc_fops;

	err = misc_register(&data->misc_dev);
	if (err) {
		printk(KERN_ERR "Unable to register \"ADS1015_ADC\" misc device\n");
		goto exit_remove;
	}

	err = sysfs_create_group(&client->dev.kobj, &ads1015_adc_gr);
	if (err) {
		dev_err(&client->dev, "sysfs_create_group failed\n");
		goto exit_deregister;
	}

	return 0;

exit_deregister:
	misc_deregister(&data->misc_dev);
exit_remove:
	return err;
}

int ads1015_adc_remove (struct i2c_client *client)
{
	struct ads1015_adc_data *data = i2c_get_clientdata(client);

	sysfs_remove_group(&client->dev.kobj, &ads1015_adc_gr);

	misc_deregister(&data->misc_dev);

	return 0;
}

static const struct i2c_device_id ads1015_adc_id[] = {
	{ "ads1015_adc", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, ads1015_adc_id);

static struct i2c_driver ads1015_adc_driver = {
	.driver = {
		.name = "ads1015_adc",
	},
	.probe = ads1015_adc_probe,
	.remove = ads1015_adc_remove,
	.id_table = ads1015_adc_id,
};

module_i2c_driver(ads1015_adc_driver);

MODULE_AUTHOR("Alex Gutikov <gutikoff@gmail.com>");
MODULE_DESCRIPTION("ADS1015 ADC driver");
MODULE_LICENSE("GPL");
