#include<linux/acpi.h>
#include<linux/bcd.h>
#include<linux/i2c.h>
#include<linux/init.h>
#include<linux/module.h>
#include<linux/of_device.h>
#include<linux/rtc/ds3231.h>
#include<linux/rtc.h>
#include<linux/slab.h>
#include<linux/string.h>
#include<linux/hwmon.h>
#include<linux/hwmon-sysfs.h>
#include<linux/clk-provider.h>
#include<linux/regmap.h>
#include<linux/watchdog.h>


int i2c_master_send(struct i2c_client *client, const char *buf,int count);

int i2c_master_recv(struct i2c_client *client, char *buf,int count);

struct ds3231{
	struct device *dev;
	struct regmap *regmap;
	const char *name;
	struct rtc_device *rtc;
};


static int ds3231_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	
}
static int ds3231_remove(struct i2c_client *client);
{

}

static const struct i2c_device_id ds3231_idtable[]={
	{"ds3231",0},
	{}
};
MODULE_DEVICE_TABLE(i2c,ds3231_idtable);
static struct i2c_driver ds3231_driver={
	.driver={
		.name="rtc-ds3231",
		
		

	},
	.id_table=ds3231_idtable,
	.probe	=ds3231_probe,
	.remove= ds3231_probe,
	
};

module_i2c_driver(ds3231_driver);

MODULE_AUTHOR("Ompolu Sai Naga Pothachari <saiompolu@gmail.com>");
MODULE_DESCRIPTION("DS3231 RTC Driver");
MODULE_LICENSE("GPL");

