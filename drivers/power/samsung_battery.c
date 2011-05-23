/*
 *	ADC-based battery driver for boards with Samsung SoCs.
 *	Copyright (c) Tomasz Figa
 *
 *	Based on s3c_adc_battery.c by Vasily Khoruzhick
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive for
 * more details.
 *
 */

#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/power_supply.h>
#include <linux/leds.h>
#include <linux/gpio.h>
#include <linux/err.h>
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/slab.h>

#include <linux/power/samsung_battery.h>

#include <plat/adc.h>

/* Time between samples (in milliseconds) */
#define BAT_POLL_INTERVAL	10000
#define BAT_POLL_INTERVAL_CHG	2000

/* Number of samples for averaging (a power of two!) */
#define NUM_SAMPLES		4

/* Voltage lookup table entry */
struct lookup_entry {
	unsigned int start;
	unsigned int end;
	unsigned int base;
	unsigned int delta;
};

/* Driver data */
struct samsung_battery {
	struct power_supply		psy;
	struct s3c_adc_client		*client;
	struct samsung_battery_pdata	*pdata;
	int				volt_value;
	int				temp_value;
	unsigned int			timestamp;
	int				level;
	int				status;
	int				cable_plugged:1;
	struct work_struct		work;
	struct delayed_work		poll_work;

	/* Voltage lookup data */
	struct lookup_entry		*lookup_table;
	unsigned int			lookup_size;

	/* Polling data */
	unsigned int			interval;

	/* Averaging algorithm data */
	int				samples[NUM_SAMPLES];
	unsigned int			cur_sample;
	int				sum;
};

/* Here the assumption that NUM_SAMPLES is a power of two shows its importance,
 * as both the modulo and division operations will be translated to simple
 * bitwise AND and shift operations. */
static int put_sample_get_avg(struct samsung_battery *bat, int sample)
{
	bat->sum -= bat->samples[bat->cur_sample];
	bat->samples[bat->cur_sample] = sample;
	bat->sum += sample;
	bat->cur_sample = (bat->cur_sample + 1) % NUM_SAMPLES;

	return bat->sum / NUM_SAMPLES;
}

/* Gets called when external power supply reports state change */
static void samsung_battery_ext_power_changed(struct power_supply *psy)
{
	struct samsung_battery *bat =
				container_of(psy, struct samsung_battery, psy);

	schedule_work(&bat->work);
}

/* Properties which we support */
static enum power_supply_property samsung_battery_props[] = {
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_CHARGE_FULL_DESIGN,
	POWER_SUPPLY_PROP_CHARGE_EMPTY_DESIGN,
	POWER_SUPPLY_PROP_CHARGE_NOW,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_TEMP,
};

/* Calculates percentage value based on battery voltage */
static int lookup_percentage(struct samsung_battery *bat, int voltage)
{
	struct lookup_entry *table = bat->lookup_table;
	unsigned int min = 0, max = bat->lookup_size - 1;

	/* Do a binary search in the lookup table */
	while (min != max) {
		if (voltage < table[(min + max) / 2].start) {
			max = -1 + (min + max) / 2;
			continue;
		}
		if (voltage >= table[(min + max) / 2].end) {
			min = 1 + (min + max) / 2;
			continue;
		}
		min = (min + max) / 2;
		break;
	}

	/* We should have the correct table entry pointer here */
	return table[min].base + (voltage - table[min].start)*table[min].delta;
}

/* Calculates temperature value based on ADC sample */
static int lookup_temperature(struct samsung_battery *bat, int temperature)
{
	return 22000;
}

/* Gets a property */
static int samsung_battery_get_property(struct power_supply *psy,
				    enum power_supply_property psp,
				    union power_supply_propval *val)
{
	struct samsung_battery *bat =
			container_of(psy, struct samsung_battery, psy);
	unsigned long flags;
	int ret = 0;
	int temp, volts;

	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		local_irq_save(flags);
		val->intval = bat->status;
		local_irq_restore(flags);
		break;
	case POWER_SUPPLY_PROP_CHARGE_FULL_DESIGN:
		val->intval = 100000;
		break;
	case POWER_SUPPLY_PROP_CHARGE_EMPTY_DESIGN:
		val->intval = 0;
		break;
	case POWER_SUPPLY_PROP_CHARGE_NOW:
		local_irq_save(flags);
		volts = bat->volt_value;
		local_irq_restore(flags);
		/* Now things get tricky... We have to use a lookup table
		 * to linearize the voltage into percents. We assume that
		 * values are linear inside each lookup range. */
		val->intval = lookup_percentage(bat, volts);
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		local_irq_save(flags);
		val->intval = bat->volt_value;
		local_irq_restore(flags);
		break;
	case POWER_SUPPLY_PROP_TEMP:
		local_irq_save(flags);
		temp = bat->temp_value;
		local_irq_restore(flags);
		/* Now the temperature value must be translated from voltage
		 * into tenths of degree Celsius. Using a lookup table again. */
		val->intval = lookup_temperature(bat, temp);
		break;
	default:
		ret = -EINVAL;
	}

	return ret;
}

/* Polling function */
static void samsung_battery_poll(struct work_struct *work)
{
	struct delayed_work *dwrk =
			container_of(work, struct delayed_work, work);
	struct samsung_battery *bat =
			container_of(dwrk, struct samsung_battery, poll_work);
	int sample;

	/* Get a voltage sample from the ADC */
	sample = s3c_adc_read(bat->client, bat->pdata->volt_channel);

	/* Put the sample and get the new average */
	bat->volt_value = put_sample_get_avg(bat, sample);

	/* Get a temperature sample from the ADC */
	bat->temp_value = s3c_adc_read(bat->client, bat->pdata->temp_channel);

	schedule_delayed_work(&bat->poll_work, msecs_to_jiffies(bat->interval));
}

/* Checks whether charging finished */
static int charge_finished(struct samsung_battery *bat,
						struct power_supply *charger)
{
	union power_supply_propval val;

	if (!charger)
		return 1;

	val.intval = POWER_SUPPLY_CHARGE_TYPE_UNKNOWN;

	if (charger->get_property)
		charger->get_property(charger,
					POWER_SUPPLY_PROP_CHARGE_TYPE, &val);

	return val.intval <= POWER_SUPPLY_CHARGE_TYPE_NONE;
}

// TODO: Distinguish between trickle and fast charging
static void samsung_battery_work(struct work_struct *work)
{
	struct samsung_battery *bat =
			container_of(work, struct samsung_battery, work);
	int is_charged;
	int is_plugged;
	static int was_plugged;
	union power_supply_propval val;
	struct power_supply *charger;

	/* Cancel any pending works */
	cancel_delayed_work(&bat->poll_work);

	charger = power_supply_get_by_name(bat->pdata->charger);

	// TODO: Consider setting lookup tables dependent on charge status
	/* Update charging status and polling interval */
	is_plugged = power_supply_am_i_supplied(&bat->psy);
	bat->cable_plugged = is_plugged;
	if (is_plugged != was_plugged) {
		was_plugged = is_plugged;
		if (is_plugged) {
			if (charger && charger->set_property) {
				val.intval = POWER_SUPPLY_CHARGE_TYPE_FAST;
				charger->set_property(charger,
					POWER_SUPPLY_PROP_CHARGE_TYPE, &val);
			}
			bat->status = POWER_SUPPLY_STATUS_CHARGING;
			bat->interval = BAT_POLL_INTERVAL_CHG;
		} else {
			if (charger && charger->set_property) {
				val.intval = POWER_SUPPLY_CHARGE_TYPE_NONE;
				charger->set_property(charger,
					POWER_SUPPLY_PROP_CHARGE_TYPE, &val);
			}
			bat->status = POWER_SUPPLY_STATUS_DISCHARGING;
			bat->interval = BAT_POLL_INTERVAL;
		}
	} else if (is_plugged) {
		is_charged = charge_finished(bat, charger);
		if (is_charged) {
			if (charger && charger->set_property) {
				val.intval = POWER_SUPPLY_CHARGE_TYPE_NONE;
				charger->set_property(charger,
					POWER_SUPPLY_PROP_CHARGE_TYPE, &val);
			}
			bat->status = POWER_SUPPLY_STATUS_FULL;
			bat->interval = BAT_POLL_INTERVAL;
		} else {
			if (charger && charger->set_property) {
				val.intval = POWER_SUPPLY_CHARGE_TYPE_FAST;
				charger->set_property(charger,
					POWER_SUPPLY_PROP_CHARGE_TYPE, &val);
			}
			bat->status = POWER_SUPPLY_STATUS_CHARGING;
			bat->interval = BAT_POLL_INTERVAL_CHG;
		}
	}

	/* Update the values and spin the polling loop */
	samsung_battery_poll(&bat->poll_work.work);

	/* Notify anyone interested */
	power_supply_changed(&bat->psy);
}

static const struct power_supply psy_template = {
	.type			= POWER_SUPPLY_TYPE_BATTERY,
	.properties		= samsung_battery_props,
	.num_properties		= ARRAY_SIZE(samsung_battery_props),
	.get_property		= samsung_battery_get_property,
	.external_power_changed = samsung_battery_ext_power_changed,
};

static int create_lookup_table(struct samsung_battery *bat)
{
	struct samsung_battery_pdata *pdata = bat->pdata;
	const struct samsung_battery_threshold *thresholds = pdata->lut;
	unsigned int threshold_count = pdata->lut_cnt;
	struct lookup_entry *entry;

	if (!thresholds || !threshold_count)
		return -ENODEV;

	bat->lookup_size = threshold_count + 1;

	entry = kmalloc(bat->lookup_size * sizeof(*entry), GFP_KERNEL);
	if (!entry)
		return -ENOMEM;

	bat->lookup_table = entry;

	/* Don't account the last entry */
	--threshold_count;

	/* Add the bottom sentinel */
	entry->start = 0;
	entry->end = thresholds->volts;
	entry->base = 0;
	entry->delta = 0;

	do {
		unsigned int volt_range, percent_range;
		volt_range = (thresholds+1)->volts - thresholds->volts;
		percent_range = (thresholds+1)->percents - thresholds->percents;
		entry->start = thresholds->volts;
		entry->end = (thresholds+1)->volts;
		entry->delta = 1000*percent_range / volt_range;
		entry->base = 1000*thresholds->percents;
		++thresholds;
		++entry;
	} while (--threshold_count);

	/* Add the top sentinel */
	entry->start = thresholds->volts;
	entry->end = 0xffffffff;
	entry->base = 100000;
	entry->delta = 0;

	return 0;
}

static int samsung_battery_probe(struct platform_device *pdev)
{
	struct s3c_adc_client	*client;
	struct samsung_battery_pdata *pdata = pdev->dev.platform_data;
	struct samsung_battery *bat;
	int ret, i;

	if (!pdata) {
		dev_err(&pdev->dev, "no platform data supplied\n");
		return -ENODEV;
	}

	client = s3c_adc_register(pdev, NULL, NULL, 0);
	if (IS_ERR(client)) {
		dev_err(&pdev->dev, "could not register adc\n");
		return PTR_ERR(client);
	}

	bat = kzalloc(sizeof(struct samsung_battery), GFP_KERNEL);
	if (!bat) {
		dev_err(&pdev->dev, "could not allocate driver data\n");
		ret = -ENOMEM;
		goto err_alloc;
	}

	platform_set_drvdata(pdev, bat);

	bat->psy = psy_template;
	bat->psy.name = dev_name(&pdev->dev);
	bat->psy.use_for_apm = pdata->use_for_apm;
	bat->client = client;
	bat->pdata = pdata;
	bat->volt_value = -1;
	bat->temp_value = -1;
	bat->cable_plugged = 0;
	bat->status = POWER_SUPPLY_STATUS_DISCHARGING;

	ret = create_lookup_table(bat);
	if (ret) {
		dev_err(&pdev->dev, "could not get create lookup table");
		goto err_lookup;
	}

	INIT_WORK(&bat->work, samsung_battery_work);
	INIT_DELAYED_WORK(&bat->poll_work, samsung_battery_poll);

	/* Get some initial data for averaging */
	for (i = 0; i < NUM_SAMPLES; ++i) {
		int sample;
		/* Get a voltage sample from the ADC */
		sample = s3c_adc_read(bat->client, bat->pdata->volt_channel);
		/* Put the sample and get the new average */
		bat->volt_value = put_sample_get_avg(bat, sample);
	}

	/* Register the power supply */
	ret = power_supply_register(&pdev->dev, &bat->psy);
	if (ret) {
		dev_err(&pdev->dev, "could not register power supply");
		goto err_reg_main;
	}

	/* Invoke platform callback if supplied */
	if (pdata->init) {
		ret = pdata->init();
		if (ret) {
			dev_err(&pdev->dev, "platform init failed");
			goto err_platform;
		}
	}

	dev_info(&pdev->dev, "successfully loaded\n");
	device_init_wakeup(&pdev->dev, 1);

	/* Schedule work to check current status */
	schedule_work(&bat->work);

	return 0;

err_platform:
	power_supply_unregister(&bat->psy);
err_reg_main:
	kfree(bat->lookup_table);
err_lookup:
	kfree(bat);
err_alloc:
	s3c_adc_release(client);
	return ret;
}

static int samsung_battery_remove(struct platform_device *pdev)
{
	struct samsung_battery *bat = platform_get_drvdata(pdev);
	struct samsung_battery_pdata *pdata = pdev->dev.platform_data;

	power_supply_unregister(&bat->psy);

	s3c_adc_release(bat->client);

	cancel_delayed_work(&bat->poll_work);

	if (pdata->exit)
		pdata->exit();

	return 0;
}

#ifdef CONFIG_PM
static int samsung_battery_suspend(struct platform_device *pdev,
	pm_message_t state)
{
	struct samsung_battery *bat = platform_get_drvdata(pdev);

	/* Cancel the polling */
	cancel_delayed_work(&bat->poll_work);

	return 0;
}

static int samsung_battery_resume(struct platform_device *pdev)
{
	struct samsung_battery *bat = platform_get_drvdata(pdev);

	/* Schedule timer to check current status */
	schedule_work(&bat->work);

	return 0;
}
#else
#define samsung_battery_suspend NULL
#define samsung_battery_resume NULL
#endif

static struct platform_driver samsung_battery_driver = {
	.driver		= {
		.name	= "samsung-battery",
	},
	.probe		= samsung_battery_probe,
	.remove		= samsung_battery_remove,
	.suspend	= samsung_battery_suspend,
	.resume		= samsung_battery_resume,
};

static int __init samsung_battery_init(void)
{
	return platform_driver_register(&samsung_battery_driver);
}
module_init(samsung_battery_init);

static void __exit samsung_battery_exit(void)
{
	platform_driver_unregister(&samsung_battery_driver);
}
module_exit(samsung_battery_exit);

MODULE_AUTHOR("Tomasz Figa <tomasz.figa at gmail.com>");
MODULE_DESCRIPTION("Samsung SoC ADC-based battery driver");
MODULE_LICENSE("GPL");
