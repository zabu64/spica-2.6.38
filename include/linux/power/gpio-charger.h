/*
 *  Copyright (C) 2010, Lars-Peter Clausen <lars@metafoo.de>
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under  the terms of the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the License, or (at your
 *  option) any later version.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#ifndef __LINUX_POWER_GPIO_CHARGER_H__
#define __LINUX_POWER_GPIO_CHARGER_H__

#include <linux/power_supply.h>
#include <linux/types.h>

/**
 * struct gpio_charger_platform_data - platform_data for gpio_charger devices
 * @name:		Name for the chargers power_supply device
 * @type:		Type of the charger
 * @gpio:		GPIO which is used to indicate the chargers status
 * @gpio_active_low:	Should be set to 1 if the GPIO is active low
 * 			otherwise 0
 * @gpio_chg:		GPIO which is used to indicate that the charging is
 * 			in progress
 * @gpio_chg_active_low:Should be set to 1 if the charging GPIO is active low
 * 			otherwise 0
 * @gpio_en:		GPIO which is used to control the charging process
 * @gpio_en_active_low:	Should be set to 1 if the charging control GPIO
 * 			is active low otherwise 0
 * @supplied_to:	Array of battery names to which this chargers
 * 			supplies power
 * @num_supplicants:	Number of entries in the supplied_to array
 */
struct gpio_charger_platform_data {
	const char *name;
	enum power_supply_type type;

	int gpio;
	int gpio_active_low;

	int gpio_chg;
	int gpio_chg_active_low;

	int gpio_en;
	int gpio_en_active_low;

	char **supplied_to;
	size_t num_supplicants;
};

#endif
