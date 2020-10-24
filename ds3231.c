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



static int ds3231_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	
}
static struct i2c_driver ds3231_driver={
	.driver={
		.name="rtc-ds3231".
		

	},
	.probe	=ds3231_probe,
	.id_table=ds3231_id,
};

}

module_i2c_driver(ds3231_driver);
MODULE_AUTHOR("Ompolu Sai Naga Pothachari <saiompolu@gmail.com>");
MODULE_DESCRIPTION("DS3231 RTC Driver");
MODULE_LICENSE("GPL");

