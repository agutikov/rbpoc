

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

#define ADS1015_ADC_DEFAULT_CHANNEL 0
#define ADS1015_ADC_DEFAULT_GAIN 2
#define ADS1015_ADC_DEFAULT_RATE 4
#define ADS1015_ADC_DEFAULT_COMP_MODE 0
#define ADS1015_ADC_DEFAULT_COMP_POL 0
#define ADS1015_ADC_DEFAULT_COMP_LAT 0
#define ADS1015_ADC_DEFAULT_COMP_QUE 3

struct ads1015_adc_buffer {
	struct ads1015_adc_sample *buffer;
	size_t buffer_size;
	size_t buffer_count;
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

	// adc samples queue
	struct ads1015_adc_buffer *current_buffer;
	struct ads1015_adc_buffer buffer_1;
	struct ads1015_adc_buffer buffer_2;
	u8 switch_flag;
	u8 overflow;

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

int thread_convertion (void *arg)
{
	int res;
	ktime_t now;
	struct ads1015_adc_data *data = (struct ads1015_adc_data *) arg;
	ktime_t prev = ktime_get();
	struct ads1015_adc_sample sample = {0, 0};
	u32 delay_us;

	printk("thread_convertion start\n");

	while (data->run) {
		now = ktime_get();
		sample.dt = now.tv64 - prev.tv64;
		prev = now;

		res = i2c_smbus_read_word_swapped(data->client, ADS1015_CONVERSION);
		if (res < 0) {
			printk("read conv error\n");
			break;
		} else {
			sample.value = (u16)res;
		}

		if (data->current_buffer->buffer_count == data->current_buffer->buffer_size) {
			data->overflow++;
		} else {
			data->current_buffer->buffer[data->current_buffer->buffer_count] = sample;
			data->current_buffer->buffer_count++;
		}

		if (data->switch_flag) {
			if (data->current_buffer == &data->buffer_1) {
				data->current_buffer = &data->buffer_2;
			} else {
				data->current_buffer = &data->buffer_1;
			}
			data->switch_flag = 0;
		}

		delay_us = 1000000 / data_rate_table[data->rate];
		udelay(delay_us - 190);
	}

	printk("thread_convertion exit\n");

	return 0;
}

int ads1015_adc_start_conversion (struct ads1015_adc_data *data)
{
	int res;
	u16 config;
	printk("ads1015_adc_start_conversion\n");

	// alloc new buffers
	data->buffer_1.buffer_size = data_rate_table[data->rate];
	data->buffer_2.buffer_size = data_rate_table[data->rate];
	data->buffer_1.buffer = kzalloc(data->buffer_1.buffer_size * sizeof(*data->buffer_1.buffer),
			GFP_KERNEL);
	data->buffer_2.buffer = kzalloc(data->buffer_2.buffer_size * sizeof(*data->buffer_2.buffer),
			GFP_KERNEL);
	data->current_buffer = &data->buffer_1;
	data->switch_flag = 0;

	// write config to ads1015
	config = ads1015_adc_data_to_config_regval(data);
	res = i2c_smbus_write_word_swapped(data->client, ADS1015_CONFIG, config);
	if (res < 0) {
		printk(KERN_ERR "ads1015_adc: error i2c write\n");
		return -1;
	}

	// start conversion thread
	data->run = 1;
	data->task = kthread_run(&thread_convertion, (void *)data, "ads1015");
	if (data->task) {
		wake_up_process(data->task);
	}

	return 0;
}


void ads1015_adc_stop_conversion (struct ads1015_adc_data *data)
{
	printk("ads1015_adc_stop_conversion\n");

	// stop conversion thread if running
	if (data->run) {
		data->run = 0;
		kthread_stop(data->task);
		data->task = 0;
	}

	// cleanup
	if (data->current_buffer) {
		kfree(data->buffer_1.buffer);
		kfree(data->buffer_2.buffer);
		data->buffer_1.buffer_count = 0;
		data->buffer_2.buffer_count = 0;
		data->buffer_1.buffer_size = 0;
		data->buffer_2.buffer_size = 0;
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
	int i;
	struct ads1015_adc_buffer *buffer;
	size_t length_adc_data;
	int sleep_us = 1000000 / data->rate;

	if (!data->run) {
		printk("ads1015 adc conversion not running\n");
		return -EINVAL;
	}

	i = 3;
	data->switch_flag = 1;
	while (data->switch_flag) {
		usleep_range(sleep_us, sleep_us*2);
		i--;
		if (!i) {
			data->switch_flag = 0;
			printk("ads1015_adc_read: switch wait timeout\n");
			return 0;
		}
	}

	if (data->current_buffer == &data->buffer_1) {
		buffer = &data->buffer_2;
	} else {
		buffer = &data->buffer_1;
	}

	length_adc_data = sizeof(buffer->buffer[0]) * buffer->buffer_count;

	if (length_adc_data > length) {
		buffer->buffer_count = length / sizeof(buffer->buffer[0]);
		length_adc_data = sizeof(buffer->buffer[0]) * buffer->buffer_count;
	}

	i = copy_to_user(user_ptr, buffer->buffer, length_adc_data);
	if (i > 0) {
		printk("ads1015_adc_read: copy_to_user error\n");
		return length_adc_data - i;
	}

#if 0
	for (i = 0; i < buffer->buffer_count; i++) {
		printk("%12lld %6d\n", buffer->buffer[i].dt, buffer->buffer[i].value);
	}
	printk("count: %d\n", buffer->buffer_count);
	printk("overflow: %d\n", data->overflow);
#endif

	data->overflow = 0;
	buffer->buffer_count = 0;

	return length_adc_data;
}

long ads1015_adc_ioctl (struct file* filp, unsigned int ioctl_num, unsigned long  ioctl_parm)
{
//	struct miscdevice *misc_dev = filp->private_data;
//	struct ads1015_adc_data *data = container_of(misc_dev, struct ads1015_adc_data, misc_dev);


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
