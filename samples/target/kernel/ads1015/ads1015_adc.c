

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

struct ads1015_adc_data {

	// i2c device configuration
	u8 channel;
	u8 gain;
	u8 rate;
	u8 comp_mode:1;
	u8 comp_pol:1;
	u8 comp_lat:1;
	u8 comp_que;

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

static ssize_t ads1015_adc_attr_show(struct device *dev,
			struct device_attribute *attr, char *buf);

static ssize_t ads1015_adc_attr_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count);

static DEVICE_ATTR(channel, S_IRWXUGO, ads1015_adc_attr_show, ads1015_adc_attr_store);
static DEVICE_ATTR(gain, S_IRWXUGO, ads1015_adc_attr_show, ads1015_adc_attr_store);
static DEVICE_ATTR(rate, S_IRWXUGO, ads1015_adc_attr_show, ads1015_adc_attr_store);

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

	char* tail = 0;
	int value = simple_strtol(buf, &tail, 0);

	if (!tail) {
		if (attr == &dev_attr_channel) {
			data->channel = value;
		} else if (attr == &dev_attr_gain) {
			data->gain = value;
		} else if (attr == &dev_attr_rate) {
			data->rate = value;
		} else {
			return -EINVAL;
		}
	} else {
		return -EINVAL;
	}

	return count;
}

int ads1015_adc_open (struct inode *inode, struct file *filp)
{
    printk("filp->private_data == 0x%p\n", filp->private_data);



	return 0;
}

int ads1015_adc_release (struct inode *inode, struct file *filp)
{
	printk("ads1015_adc_release\n");


	return 0;
}

long ads1015_adc_ioctl (struct file* filp ,unsigned int ioctl_num , unsigned long  ioctl_parm)
{
	printk("ads1015_adc_ioctl\n");


	return 0;
}

struct file_operations misc_fops = {
	.owner = THIS_MODULE,
	.open = ads1015_adc_open,
	.release = ads1015_adc_release,
	.unlocked_ioctl = ads1015_adc_ioctl,
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

	data->misc_dev.minor = MISC_DYNAMIC_MINOR;
	data->misc_dev.name = "ADS1015_ADC";
	data->misc_dev.fops = &misc_fops;

	err = misc_register(&data->misc_dev);
    if (err) {
    	printk(KERN_ERR "Unable to register \"ADS1015_ADC\" misc device\n");
    	goto exit_remove;
    }

    printk("&data->misc_dev == 0x%p\n", &data->misc_dev);

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

