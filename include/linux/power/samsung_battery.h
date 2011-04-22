#ifndef _SAMSUNG_BATTERY_H
#define _SAMSUNG_BATTERY_H

struct samsung_battery_threshold {
	/* Start of voltage range */
	unsigned int volts;
	/* Start of percentage range */
	unsigned int percents;
};

struct samsung_battery_pdata {
	/* Optional: Platform callbacks */
	int (*init)(void);
	void (*exit)(void);

	/* Voltage lookup table */
	const struct samsung_battery_threshold *lut;
	unsigned int lut_cnt;

	/* ADC channels */
	const unsigned int volt_channel;
	const unsigned int temp_channel;

	/* Charger power supply name */
	char *charger;

	/* Battery can be used for APM emulation */
	int use_for_apm;
};

#endif
