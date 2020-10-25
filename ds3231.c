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
#include<linux/delay.h>

#define DS1307_REG_SECS 	0x00
#define DS1307_BIT_CH 		0x80
#define DS1307_BIT_nEOSC 	0x80
#define DS1307_REG_MIN 	0x01
#define DS1307_REG_HOUR 	0x02
#define DS1307_BIT_12HR 	0x40
#define DS1307_BIT_PM 		0x20
#define DS1307_REG_WDAY 	0x03
#define DS1307_REG_MDAY 	0x04
#define DS1307_REG_MONTH 	0x05
#define DS1307_REG_YEAR 	0x06
#define DS3231_BIT_CENTURY	0x80

/**control registers*/

#define DS3231_REG_CONTROL 	0x0e
#define DS3231_BIT_nEOSC	0x80
#define DS3231_BIT_BBSQW	0x40

/**status registers**/

#define DS3231_REG_STATUS	0x0f
#define DS3231_BIT_OSF		0x80



struct ds3231{
	struct device *dev;
	struct regmap *regmap;
	const char *name;
	struct rtc_device *rtc;
};

struct chip_desc{
	u8 offset;
	u8 century_reg;
	u8 century_enable_bit;
	u8 century_bit;
	u8 bbsqi_bit;
	const struct rtc_class_ops *rtc_ops;
};

static const struct chip_desc chips[last_ds_type];

static int ds3231_get_time(struct device *dev,struct rtc_time *t)
{
	struct ds3231 *ds3231=dev_get_drvdata(dev);
	int tmp,ret;
	const struct chip_desc *chip=&chips[ds3231->type];
	u8 regs[7];
	/**read the RTC date and time registers all at once**/
	ret=regmap_bulk_read(ds3231->regmap, chip->offset, regs, sizeof(regs));
	if (ret)
	{
		dev_err(dev,"%s error %d \n","read",ret);
		return ret;
	}
	dev_dbg(dev,"%s: %7ph\n","read",regs);
	
	tmp=regs[DS3231_REG_SECS];
	t->tm_sec=bcd2bin(regs[DS3231_REG_SECS]&0x7f);
	t->tm_min=bcd2bin(regs[DS3231_REG_MIN]&0x7f);
	tmp=regs[DS3231_REG_HOUR]&0x3f;
	t->tm_hour=bcd2bin(tmp);
	t->tm_wday=bcd2bin(regs[DS3231_REG_WDAY]&0x07)-1;
	t->tm_mday=bcd2bin(regs[DS3231_REG_MDAY]&0x3f);
	tmp=regs[DS3231_REG_MONTH]&0x1f;
	t->tm_mon=bcd2bin(tmp)-1;
	t->tm_year=bcd2bin(regs[DS3231_REG_YEAR])+100;
	
	if(regs[chip->century_reg]&chip->century_bit && 					IS_ENABLED(CONFIG_RTC_DRV_DS1307_CENTURY))
		t->tm_year += 100;
	dev_dbg(dev,"%s secs=%d,mins=%d,""hours=%d, mday=%d, year=%d, wday=%d\n","read",t->tm_sec,t->tm_min,t->tm_hour,t->tm_mday,t->tm_mon,t->tm_year,t->tm_wday);
	return 0;
}

static int ds3231_set_time(struct device *dev,struct rtc_time *t)
{
	struct ds3231 *ds3231 = dev_get_drvdata(dev);
	const struct chip_desc *chip=&chips[ds3231->type];
	int result;
	int tmp;
	u8 regs[7];
	
	dev_dbg(dev,"%s secs=%d,mins=%d,""hours=%d,mday=%d,year=%d,wday=%d\n"."write",t->tm_sec,t->tm_min,t->tm_hour,t->tm_mday,t->tm_mon,t->tm_year,t->tm_wday);
	if (t->tm_year < 100)
		return -EINVAL;
	regs[DS3231_REG_SECS]=bin2bcd(t->tm_sec);
	regs[DS3231_REG_MIN]=bin2bcd(t->tm_min);
	regs[DS3231_REG_HOUR]=bin2bcd(t->tm_hour);
	regs[DS3231_REG_WDAY]=bin2bcd(t->tm_wday+1);
	regs[DS3231_REG_MONTH]=bin2bcd(t->tm_mon+1);
	
	/** assume 20yy not 19yy **/
	tmp=t->tm_year - 100;
	regs[DS3231_REG_YEAR]=bin2bcd(tmp);
	
	dev_dbg(dev,"%s:%7ph\n","write",regs);
	
	result=regmap_bulk_write(ds3231->regmap, chip->offset,regs,sizeof(regs));
	if(result)
	{
		dev_err(dev,"%s error %d\n","write",result);
		return result;
	}
	return 0;
}

static const struct chip_desc chips[last_ds_type]={
	[ds_3231]={
	.century_reg=DS3231_REG_MONTH,
	.century_bit=DS3231_BIT_CENTURY,
	.bbsqi_bit=DS3231_BIT_BBSQW,
	},
};

static const struct rtc_class_ops ds3231_rtc_ops={
	.get_time = ds3231_get_time;
	.set_time = ds3231_set_time;
};


static const struct i2c_device_id ds3231_idtable[]={
	{"ds3231",0},
	{}
};
MODULE_DEVICE_TABLE(i2c,ds3231_idtable);

static const struct of_device_id ds3231_of_match[]={
{
	.compatible="maxim,ds3231",
	.data=(void *)ds_3231
}
};
MODULE_DEVICE_TABLE(of, ds3231_of_match);

static const struct regmap_config regmap_config={
	.reg_bits=8,
	.val_bits=8,
};

static int ds3231_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct ds3231	*ds3231;
	int err=-ENODEV;
	int tmp;
	const struct chip_desc *chip;
	unsigned char regs[8];
	ds3231=devm_kzalloc(&client->dev, sizeof(struct ds3231),GFP_KERNEL);
	
	if(!ds3231)
		return -ENOMEM;
	
	dev_set_drvdata(&client->dev,ds1307);
	ds3231->dev=&clent->dev;
	ds3231->name=client->name;
	ds3231->regmap=devm_regmap_init_i2c(client, &regmap_config);
	
	if(IS_ERR(ds3231->regmap))
	{
		dev_err(ds3231->dev,"regmap allocation failed\n");
		return PTR_ERR(ds3231->regmap);
	}
	
	i2c_set_clientdata(client,ds3231);
	
	err=regmap_bulk_read(ds3231->regmap,ds3231_REG_CONTROL,regs,2);
	if(err)
	{
		dev_dbg(ds3231->dev,"read error %d\n",err);
		return err;
	}
	if(regs[0] & DS3231_BIT_nEOSC)
		regs[0]&=~DS3231_BIT_nEOSC;
	regmap_write(ds1307->regmap, DS3231_REG_CONTROL,regs[0]);
	
	/***oscillator fault***/
	if(regs[1] & DS3231_BIT_OSF)
	{
		regmap_write(ds3231->regmap,DS3231_REG_STATUS,regs[1] & ~DS3231_BIT_OSF);
		dev_warn(ds3231->dev,"SET TIME!\n");
	}
	
	/***read RTC registers***/
	err=regmap_bulk_read(ds3231->regmap,chip->offset,regs,sizeof(regs));
	if(err)
	{
		dev_dbg(ds3231->dev,"read erro %d\n",err);
		return err;
	}
	
	tmp=regs[DS3231_REG_HOUR];
	if(!(tmp & DS3231_BIT_12HR))
		break;
	tmp=bcd2bin(tmp & 0x1f);
	if(tmp==12)
		tmp=0;
	if(regs[DS3231_REG_HOUR]&DS3231_BIT_PM)
		tmp+=12;
	regmap_write(ds3231->regmap, chip->offset + DS3231_REG_HOUR, bin2bcd(tmp));
	
	ds3231->rtc=devm_rtc_allocate_device(ds3231->dev);
	//
	ds3231->rtc->ops=chip->rtc_ops ?: &ds3231_rtc_ops;
	err=rtc_register_device(ds3231->rtc);
	if(err)
		return err;	
}


static struct i2c_driver ds3231_driver={
	.driver={
		.name="rtc-ds3231",
		.of_match_table=of_match_ptr(ds3231_of_match);

	},
	.probe	=ds3231_probe,
	.id_table=ds3231_idtable,
};

module_i2c_driver(ds3231_driver);

MODULE_AUTHOR("Ompolu Sai Naga Pothachari <saiompolu@gmail.com>");
MODULE_DESCRIPTION("DS3231 RTC Driver");
MODULE_LICENSE("GPL");

