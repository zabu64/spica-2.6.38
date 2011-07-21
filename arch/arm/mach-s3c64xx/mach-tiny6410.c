/* linux/arch/arm/mach-s3c64xx/mach-tiny6410.c
 *
 * Copyright 2011 Tomasz Figa <tomasz.figa at gmail.com>
 *
 * Based on linux/arch/arm/mach-s3c64xx/mach-mini6410.c.
 *
 * Copyright 2010 Darius Augulis <augulis.darius@gmail.com>
 * Copyright 2008 Openmoko, Inc.
 * Copyright 2008 Simtec Electronics
 *	Ben Dooks <ben@simtec.co.uk>
 *	http://armlinux.simtec.co.uk/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/fb.h>
#include <linux/gpio.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/dm9000.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <linux/serial_core.h>
#include <linux/types.h>
#include <linux/gpio_keys.h>
#include <linux/tiny6410_1wire.h>
#include <linux/android_pmem.h>
#include <linux/input.h>
#include <linux/leds.h>

#include <linux/usb/android_composite.h>
#include <linux/usb/ch9.h>
#include <linux/usb/gadget.h>

#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/setup.h>

#include <mach/map.h>
#include <mach/regs-fb.h>
#include <mach/regs-gpio.h>
#include <mach/regs-modem.h>
#include <mach/regs-srom.h>
#include <mach/s3c6410.h>

#include <plat/adc.h>
#include <plat/cpu.h>
#include <plat/devs.h>
#include <plat/fb.h>
#include <plat/nand.h>
#include <plat/regs-serial.h>
#include <plat/ts.h>
#include <plat/gpio-cfg.h>
#include <plat/sdhci.h>
#include <plat/pm.h>

#include <video/platform_lcd.h>

#define UCON	(S3C2410_UCON_DEFAULT)
#define ULCON	(S3C2410_LCON_CS8 | S3C2410_LCON_PNONE | S3C2410_LCON_STOPB)
#define UFCON 	(S3C2410_UFCON_RXTRIG8 | S3C2410_UFCON_FIFOMODE)

static struct s3c2410_uartcfg tiny6410_uartcfgs[] __initdata = {
	[0] = {
		.hwport	= 0,
		.flags	= 0,
		.ucon	= UCON,
		.ulcon	= ULCON,
		.ufcon	= UFCON,
	},
	[1] = {
		.hwport	= 1,
		.flags	= 0,
		.ucon	= UCON,
		.ulcon	= ULCON,
		.ufcon	= UFCON,
	},
	[2] = {
		.hwport	= 2,
		.flags	= 0,
		.ucon	= UCON,
		.ulcon	= ULCON,
		.ufcon	= UFCON,
	},
	[3] = {
		.hwport	= 3,
		.flags	= 0,
		.ucon	= UCON,
		.ulcon	= ULCON,
		.ufcon	= UFCON,
	},
};

/* DM9000AEP 10/100 ethernet controller */

static struct resource tiny6410_dm9k_resource[] = {
	[0] = {
		.start	= S3C64XX_PA_XM0CSN1,
		.end	= S3C64XX_PA_XM0CSN1 + 1,
		.flags	= IORESOURCE_MEM
	},
	[1] = {
		.start	= S3C64XX_PA_XM0CSN1 + 4,
		.end	= S3C64XX_PA_XM0CSN1 + 5,
		.flags	= IORESOURCE_MEM
	},
	[2] = {
		.start	= S3C_EINT(7),
		.end	= S3C_EINT(7),
		.flags	= IORESOURCE_IRQ | IORESOURCE_IRQ_HIGHLEVEL
	}
};

static struct dm9000_plat_data tiny6410_dm9k_pdata = {
	.flags		= (DM9000_PLATF_16BITONLY | DM9000_PLATF_NO_EEPROM),
};

static struct platform_device tiny6410_device_eth = {
	.name		= "dm9000",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(tiny6410_dm9k_resource),
	.resource	= tiny6410_dm9k_resource,
	.dev		= {
		.platform_data	= &tiny6410_dm9k_pdata,
	},
};

static struct mtd_partition tiny6410_nand_part[] = {
	[0] = {
		.name		= "Bootloader",
		.size		= SZ_512K,
		.offset		= 0,
		.mask_flags	= MTD_CAP_NANDFLASH,
	},
	[1] = {
		.name		= "Kernel",
		.size		= SZ_4M + SZ_1M,
		.offset		= MTDPART_OFS_APPEND,
		.mask_flags	= MTD_CAP_NANDFLASH,
	},
	[2] = {
		.name		= "File System",
		.size		= MTDPART_SIZ_FULL,
		.offset		= MTDPART_OFS_APPEND,
	},
};

static struct s3c2410_nand_set tiny6410_nand_sets[] = {
	[0] = {
		.name		= "nand",
		.nr_chips	= 1,
		.nr_partitions	= ARRAY_SIZE(tiny6410_nand_part),
		.partitions	= tiny6410_nand_part,
	},
};

static struct s3c2410_platform_nand tiny6410_nand_info = {
	.tacls		= 25,
	.twrph0		= 55,
	.twrph1		= 40,
	.nr_sets	= ARRAY_SIZE(tiny6410_nand_sets),
	.sets		= tiny6410_nand_sets,
};

/* LEDS */
static struct gpio_led tiny6410_led_list[] = {
	{
		.name = "led1",
		.default_trigger = "heartbeat",
		.gpio = S3C64XX_GPK(4),
		.active_low = 1,
		.retain_state_suspended = 0,
		.default_state = LEDS_GPIO_DEFSTATE_OFF,
	}, {
		.name = "led2",
		.default_trigger = "nand-disk",
		.gpio = S3C64XX_GPK(5),
		.active_low = 1,
		.retain_state_suspended = 0,
		.default_state = LEDS_GPIO_DEFSTATE_OFF,
	}, {
		.name = "led3",
		.default_trigger = "mmc0",
		.gpio = S3C64XX_GPK(6),
		.active_low = 1,
		.retain_state_suspended = 0,
		.default_state = LEDS_GPIO_DEFSTATE_OFF,
	}, {
		.name = "led4",
		.default_trigger = NULL,
		.gpio = S3C64XX_GPK(7),
		.active_low = 1,
		.retain_state_suspended = 0,
		.default_state = LEDS_GPIO_DEFSTATE_OFF,
	},
};

static struct gpio_led_platform_data tiny6410_led_pdata = {
	.num_leds = ARRAY_SIZE(tiny6410_led_list),
	.leds = tiny6410_led_list,
};

static struct platform_device tiny6410_leds = {
	.name = "leds-gpio",
	.id = -1,
	.dev = {
		.platform_data	= &tiny6410_led_pdata,
	},
};

static struct s3c_fb_pd_win tiny6410_fb_win[] = {
	{
		.win_mode	= {	/* 4.3" 480x272 */
			.left_margin	= 45,
			.right_margin	= 4,
			.upper_margin	= 3,
			.lower_margin	= 2,
			.hsync_len	= 40,
			.vsync_len	= 6,
			.xres		= 480,
			.yres		= 272,
		},
		.max_bpp	= 32,
		.default_bpp	= 16,
		.virtual_y	= 544,
	}, {
		.win_mode	= {	/* 7.0" 800x480 */
			.left_margin	= 8,
			.right_margin	= 13,
			.upper_margin	= 7,
			.lower_margin	= 5,
			.hsync_len	= 3,
			.vsync_len	= 1,
			.xres		= 800,
			.yres		= 480,
		},
		.max_bpp	= 32,
		.default_bpp	= 16,
		.virtual_y	= 960,
	},
};

static struct s3c_fb_platdata tiny6410_lcd_pdata __initdata = {
	.setup_gpio	= s3c64xx_fb_gpio_setup_24bpp,
	.win[0]		= &tiny6410_fb_win[0],
	.vidcon0	= VIDCON0_VIDOUT_RGB | VIDCON0_PNRMODE_RGB,
	.vidcon1	= VIDCON1_INV_VCLK,
};

static void tiny6410_lcd_power_set(struct plat_lcd_data *pd,
				   unsigned int power)
{
	if (power)
		gpio_direction_output(S3C64XX_GPE(0), 1);
	else
		gpio_direction_output(S3C64XX_GPE(0), 0);
}

static struct plat_lcd_data tiny6410_lcd_power_data = {
	.set_power	= tiny6410_lcd_power_set,
};

static struct platform_device tiny6410_lcd_powerdev = {
	.name	= "platform-lcd",
	.dev	= {
		.parent		= &s3c_device_fb.dev,
		.platform_data	= &tiny6410_lcd_power_data,
	},
};

static void tiny6410_1wire_pullup(int enable)
{
	if (enable)
		s3c_gpio_setpull(S3C64XX_GPF(15), S3C_GPIO_PULL_UP);
	else
		s3c_gpio_setpull(S3C64XX_GPF(15), S3C_GPIO_PULL_NONE);
}

static struct tiny6410_1wire_platform_data tiny6410_1wire_data = {
	.gpio_pin	= S3C64XX_GPF(15),
	.set_pullup	= tiny6410_1wire_pullup,
};

static struct platform_device tiny6410_1wire = {
	.name	= "tiny6410-1wire",
	.dev	= {
		.platform_data	= &tiny6410_1wire_data,
	},
};

static struct s3c2410_ts_mach_info s3c_ts_platform __initdata = {
	.delay			= 10000,
	.presc			= 49,
	.oversampling_shift	= 2,
};

static struct gpio_keys_button gpio_buttons[] = {
	{
		.gpio			= S3C64XX_GPN(0),
		.code			= 158,
		.desc			= "HOME",
		.active_low		= 1,
		.wakeup			= 1,
		.debounce_interval	= 5,
		.type                   = EV_KEY,
	}, {
		.gpio			= S3C64XX_GPN(1),
		.code			= 139,
		.desc			= "MENU",
		.active_low		= 1,
		.wakeup			= 0,
		.debounce_interval	= 5,
		.type                   = EV_KEY,
	}, {
		.gpio			= S3C64XX_GPN(3),
		.code			= 105,
		.desc			= "DPAD_LEFT",
		.active_low		= 1,
		.wakeup			= 0,
		.debounce_interval	= 5,
		.type                   = EV_KEY,
	}, {
		.gpio			= S3C64XX_GPN(4),
		.code			= 108,
		.desc			= "DPAD_DOWN",
		.active_low		= 1,
		.wakeup			= 0,
		.debounce_interval	= 5,
		.type                   = EV_KEY,
	}, {
		.gpio			= S3C64XX_GPN(5),
		.code			= 103,
		.desc			= "DPAD_UP",
		.active_low		= 1,
		.wakeup			= 0,
		.debounce_interval	= 5,
		.type                   = EV_KEY,
	}, {
		.gpio			= S3C64XX_GPN(2),
		.code			= 106,
		.desc			= "DPAD_RIGHT",
		.active_low		= 1,
		.wakeup			= 0,
		.debounce_interval	= 5,
		.type                   = EV_KEY,
	}, {
		.gpio			= S3C64XX_GPL(11),
		.code			= 102,
		.desc			= "BACK",
		.active_low		= 1,
		.wakeup			= 0,
		.debounce_interval	= 5,
		.type                   = EV_KEY,
	}, {
		.gpio			= S3C64XX_GPL(12),
		.code			= 232,
		.desc			= "DPAD_CENTER",
		.active_low		= 1,
		.wakeup			= 1,
		.debounce_interval	= 5,
		.type                   = EV_KEY,
	}
};

static struct gpio_keys_platform_data gpio_button_data = {
	.buttons	= gpio_buttons,
	.nbuttons	= ARRAY_SIZE(gpio_buttons),
};

static struct platform_device s3c_device_gpio_btns = {
	.name		= "gpio-keys",
	.id		= -1,
	.num_resources	= 0,
	.dev		= {
		.platform_data	= &gpio_button_data,
	}
};

static struct s3c_sdhci_platdata tiny6410_hsmmc0_pdata = {
    .max_width      = 4,
    .cd_type        = S3C_SDHCI_CD_INTERNAL,
};

static struct s3c_sdhci_platdata tiny6410_hsmmc1_pdata = {
    .max_width      = 4,
    .cd_type        = S3C_SDHCI_CD_PERMANENT,
};

/*
 * USB gadget
 */

#include <linux/usb/android_composite.h>
#define S3C_VENDOR_ID			0x18d1
#define S3C_UMS_PRODUCT_ID		0x4E21
#define S3C_UMS_ADB_PRODUCT_ID		0x4E22
#define S3C_RNDIS_PRODUCT_ID		0x4E23
#define S3C_RNDIS_ADB_PRODUCT_ID	0x4E24
#define MAX_USB_SERIAL_NUM	17

static char *usb_functions_ums[] = {
	"usb_mass_storage",
};

static char *usb_functions_rndis[] = {
	"rndis",
};

static char *usb_functions_rndis_adb[] = {
	"rndis",
	"adb",
};
static char *usb_functions_ums_adb[] = {
	"usb_mass_storage",
	"adb",
};

static char *usb_functions_all[] = {
	"rndis",
	"usb_mass_storage",
	"adb",
};

static struct android_usb_product usb_products[] = {
	{
		.product_id	= S3C_UMS_PRODUCT_ID,
		.num_functions	= ARRAY_SIZE(usb_functions_ums),
		.functions	= usb_functions_ums,
	},
	{
		.product_id	= S3C_UMS_ADB_PRODUCT_ID,
		.num_functions	= ARRAY_SIZE(usb_functions_ums_adb),
		.functions	= usb_functions_ums_adb,
	},
	{
		.product_id	= S3C_RNDIS_PRODUCT_ID,
		.num_functions	= ARRAY_SIZE(usb_functions_rndis),
		.functions	= usb_functions_rndis,
	},
	{
		.product_id	= S3C_RNDIS_ADB_PRODUCT_ID,
		.num_functions	= ARRAY_SIZE(usb_functions_rndis_adb),
		.functions	= usb_functions_rndis_adb,
	},
};

static char device_serial[MAX_USB_SERIAL_NUM] = "0123456789ABCDEF";
/* standard android USB platform data */

/* Information should be changed as real product for commercial release */
static struct android_usb_platform_data android_usb_pdata = {
	.vendor_id		= S3C_VENDOR_ID,
	.product_id		= S3C_UMS_PRODUCT_ID,
	.manufacturer_name	= "Samsung",
	.product_name		= "Galaxy GT-I5700",
	.serial_number		= device_serial,
	.num_products		= ARRAY_SIZE(usb_products),
	.products		= usb_products,
	.num_functions		= ARRAY_SIZE(usb_functions_all),
	.functions		= usb_functions_all,
};

struct platform_device spica_android_usb = {
	.name	= "android_usb",
	.id	= -1,
	.dev	= {
		.platform_data	= &android_usb_pdata,
	},
};

static struct usb_mass_storage_platform_data ums_pdata = {
	.vendor			= "Android",
	.product		= "UMS Composite",
	.release		= 1,
	.nluns			= 1,
};

struct platform_device spica_usb_mass_storage = {
	.name	= "usb_mass_storage",
	.id	= -1,
	.dev	= {
		.platform_data = &ums_pdata,
	},
};

static struct usb_ether_platform_data rndis_pdata = {
	/* ethaddr is filled by board_serialno_setup */
	.vendorID	= 0x18d1,
	.vendorDescr	= "Samsung",
};

struct platform_device spica_usb_rndis = {
	.name	= "rndis",
	.id	= -1,
	.dev	= {
		.platform_data = &rndis_pdata,
	},
};

static struct platform_device *tiny6410_devices[] __initdata = {
	&tiny6410_device_eth,
	&s3c_device_hsmmc0,
	&s3c_device_hsmmc1,
#if 0
	&s3c_device_ohci,
#endif
	&s3c_device_nand,
	&s3c_device_fb,
	&s3c_device_rtc,
#if 0
	&s3c_device_usb_hsotg,
	&spica_android_usb,
	&spica_usb_mass_storage,
	&spica_usb_rndis,
#endif
	&tiny6410_lcd_powerdev,
	&s3c_device_adc,
	&s3c_device_ts,
	&s3c_device_gpio_btns,
	&tiny6410_1wire,
	&tiny6410_leds,
};

/*
 * Platform devices for Samsung modules
 */

static struct resource s3c_g3d_resource[] = {
	[0] = {
		.start = S3C64XX_PA_G3D,
		.end   = S3C64XX_PA_G3D + S3C64XX_SZ_G3D - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_S3C6410_G3D,
		.end   = IRQ_S3C6410_G3D,
		.flags = IORESOURCE_IRQ,
	}
};

static struct resource s3c_mfc_resource[] = {
	[0] = {
		.start = S3C64XX_PA_MFC,
		.end   = S3C64XX_PA_MFC + S3C64XX_SZ_MFC - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_MFC,
		.end   = IRQ_MFC,
		.flags = IORESOURCE_IRQ,
	}
};

static struct resource s3c_jpeg_resource[] = {
	[0] = {
		.start = S3C64XX_PA_JPEG,
		.end   = S3C64XX_PA_JPEG + S3C64XX_SZ_JPEG - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_JPEG,
		.end   = IRQ_JPEG,
		.flags = IORESOURCE_IRQ,
	}
};

static struct resource s3c_camif_resource[] = {
	[0] = {
		.start = S3C64XX_PA_FIMC,
		.end   = S3C64XX_PA_FIMC + S3C64XX_SZ_FIMC - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_CAMIF_C,
		.end   = IRQ_CAMIF_C,
		.flags = IORESOURCE_IRQ,
	},
	[2] = {
		.start = IRQ_CAMIF_P,
		.end   = IRQ_CAMIF_P,
		.flags = IORESOURCE_IRQ,
	}
};

static struct resource s3c_g2d_resource[] = {
	[0] = {
		.start = S3C64XX_PA_G2D,
		.end   = S3C64XX_PA_G2D + S3C64XX_SZ_G2D - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_2D,
		.end   = IRQ_2D,
		.flags = IORESOURCE_IRQ,
	}
};

static struct resource s3c_rotator_resource[] = {
	[0] = {
		.start = S3C64XX_PA_ROTATOR,
		.end   = S3C64XX_PA_ROTATOR + S3C64XX_SZ_ROTATOR - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_ROTATOR,
		.end   = IRQ_ROTATOR,
		.flags = IORESOURCE_IRQ,
	}
};

static struct resource s3c_pp_resource[] = {
	[0] = {
		.start = S3C64XX_PA_PP,
		.end   = S3C64XX_PA_PP + S3C64XX_SZ_PP - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_POST0,
		.end   = IRQ_POST0,
		.flags = IORESOURCE_IRQ,
	}
};

static struct platform_device *tiny6410_mod_devices[] __initdata = {
	&(struct platform_device){
		.name		= "s3c-g3d",
		.id		= -1,
		.num_resources	= ARRAY_SIZE(s3c_g3d_resource),
		.resource	= s3c_g3d_resource,
	}, &(struct platform_device){
		.name		= "s3c-mfc",
		.id		= -1,
		.num_resources	= ARRAY_SIZE(s3c_mfc_resource),
		.resource	= s3c_mfc_resource,
	}, &(struct platform_device){
		.name		= "s3c-jpeg",
		.id		= -1,
		.num_resources	= ARRAY_SIZE(s3c_jpeg_resource),
		.resource	= s3c_jpeg_resource,
	}, &(struct platform_device){
		.name		= "s3c-fimc",
		.id		= -1,
		.num_resources	= ARRAY_SIZE(s3c_camif_resource),
		.resource	= s3c_camif_resource,
	}, &(struct platform_device){
		.name		= "s3c-g2d",
		.id		= -1,
		.num_resources	= ARRAY_SIZE(s3c_g2d_resource),
		.resource	= s3c_g2d_resource,
	}, &(struct platform_device){
		.name		= "s3c-rotator",
		.id		= -1,
		.num_resources	= ARRAY_SIZE(s3c_rotator_resource),
		.resource	= s3c_rotator_resource,
	}, &(struct platform_device){
		.name		= "s3c-pp",
		.id		= -1,
		.num_resources	= ARRAY_SIZE(s3c_pp_resource),
		.resource	= s3c_pp_resource,
	}
};

/*
 * Memory Configuration (Reserved Memory Setting)
 */

#define	PHYS_SIZE			(208 * SZ_1M)

#define DRAM_END_ADDR 			(PHYS_OFFSET + PHYS_SIZE)
#define RESERVED_PMEM_END_ADDR 		(DRAM_END_ADDR)

#define RESERVED_MEM_CMM		(3 * SZ_1M)
#define RESERVED_MEM_MFC		(6 * SZ_1M)
/* PMEM_PIC and MFC use share area */
#define RESERVED_PMEM_PICTURE		(6 * SZ_1M)
#define RESERVED_PMEM_JPEG		(3 * SZ_1M)
#define RESERVED_PMEM_PREVIEW		(2 * SZ_1M)
#define RESERVED_PMEM_RENDER	  	(2 * SZ_1M)
#define RESERVED_PMEM_STREAM	  	(2 * SZ_1M)
/* G3D is shared with uppper memory areas */
#define RESERVED_G3D			(32 * SZ_1M)
#define RESERVED_PMEM_GPU1		(RESERVED_G3D)
#define RESERVED_PMEM			(8 * SZ_1M)
#define RESERVED_PMEM_SKIA		(0 * SZ_1M)
#define RESERVED_G3D_UI			(6 * SZ_1M)
#define RESERVED_G3D_SHARED		(RESERVED_MEM_CMM \
					+ RESERVED_MEM_MFC \
					+ RESERVED_PMEM_STREAM \
					+ RESERVED_PMEM_JPEG \
					+ RESERVED_PMEM_PREVIEW \
					+ RESERVED_PMEM_RENDER)
#define RESERVED_G3D_APP		(RESERVED_G3D \
					- RESERVED_G3D_UI \
					- RESERVED_G3D_SHARED)

#define CMM_RESERVED_MEM_START		(RESERVED_PMEM_END_ADDR \
					- RESERVED_MEM_CMM)
#define PICTURE_RESERVED_PMEM_START	(CMM_RESERVED_MEM_START \
					- RESERVED_MEM_MFC)
#define JPEG_RESERVED_PMEM_START	(PICTURE_RESERVED_PMEM_START \
					- RESERVED_PMEM_JPEG)
#define PREVIEW_RESERVED_PMEM_START	(JPEG_RESERVED_PMEM_START \
					- RESERVED_PMEM_PREVIEW)
#define RENDER_RESERVED_PMEM_START	(PREVIEW_RESERVED_PMEM_START \
					- RESERVED_PMEM_RENDER)
#define STREAM_RESERVED_PMEM_START	(RENDER_RESERVED_PMEM_START \
					- RESERVED_PMEM_STREAM)
/* G3D is shared with uppper memory areas */
#define G3D_RESERVED_START		(RESERVED_PMEM_END_ADDR \
					- RESERVED_G3D)
#define GPU1_RESERVED_PMEM_START	(RESERVED_PMEM_END_ADDR \
					- RESERVED_PMEM_GPU1)
#define RESERVED_PMEM_START		(GPU1_RESERVED_PMEM_START \
					- RESERVED_PMEM)
#define PHYS_UNRESERVED_SIZE		(RESERVED_PMEM_START \
					- PHYS_OFFSET)
#define SKIA_RESERVED_PMEM_START	(0)

static void __init tiny6410_fixup(struct machine_desc *desc,
		struct tag *tags, char **cmdline, struct meminfo *mi)
{
	mi->nr_banks = 2;

	mi->bank[0].start = PHYS_OFFSET;
	mi->bank[0].size = SZ_128M;

	mi->bank[1].start = PHYS_OFFSET + SZ_128M;
	mi->bank[1].size = PHYS_UNRESERVED_SIZE - SZ_128M;
}

static void __init tiny6410_map_io(void)
{
	u32 tmp;

	s3c64xx_init_io(NULL, 0);
	s3c24xx_init_clocks(12000000);
	s3c24xx_init_uarts(tiny6410_uartcfgs, ARRAY_SIZE(tiny6410_uartcfgs));

	/* set the LCD type */
	tmp = __raw_readl(S3C64XX_SPCON);
	tmp &= ~S3C64XX_SPCON_LCD_SEL_MASK;
	tmp |= S3C64XX_SPCON_LCD_SEL_RGB;
	__raw_writel(tmp, S3C64XX_SPCON);

	/* remove the LCD bypass */
	tmp = __raw_readl(S3C64XX_MODEM_MIFPCON);
	tmp &= ~MIFPCON_LCD_BYPASS;
	__raw_writel(tmp, S3C64XX_MODEM_MIFPCON);
}

/*
 * tiny6410_features string
 *
 * 0-9 LCD configuration
 *
 */
static char tiny6410_features_str[12] __initdata = "0";

static int __init tiny6410_features_setup(char *str)
{
	if (str)
		strlcpy(tiny6410_features_str, str,
			sizeof(tiny6410_features_str));
	return 1;
}

__setup("tiny6410=", tiny6410_features_setup);

#define FEATURE_SCREEN (1 << 0)

struct tiny6410_features_t {
	int done;
	int lcd_index;
};

static void tiny6410_parse_features(
		struct tiny6410_features_t *features,
		const char *features_str)
{
	const char *fp = features_str;

	features->done = 0;
	features->lcd_index = 0;

	while (*fp) {
		char f = *fp++;

		switch (f) {
		case '0'...'9':	/* tft screen */
			if (features->done & FEATURE_SCREEN) {
				printk(KERN_INFO "TINY6410: '%c' ignored, "
					"screen type already set\n", f);
			} else {
				int li = f - '0';
				if (li >= ARRAY_SIZE(tiny6410_fb_win))
					printk(KERN_INFO "TINY6410: '%c' out "
						"of range LCD mode\n", f);
				else {
					features->lcd_index = li;
				}
			}
			features->done |= FEATURE_SCREEN;
			break;
		}
	}
}

/*
 * Android PMEM
 */

#ifdef CONFIG_ANDROID_PMEM
static struct android_pmem_platform_data pmem_pdata = {
	.name		= "pmem",
	.no_allocator	= 1,
	.cached		= 1,
	.buffered	= 1,
	.start		= RESERVED_PMEM_START,
	.size		= RESERVED_PMEM,
};

static struct android_pmem_platform_data pmem_gpu1_pdata = {
	.name		= "pmem_gpu1",
	.no_allocator	= 0,
	.cached		= 1,
	.buffered	= 1,
	.start		= GPU1_RESERVED_PMEM_START,
	.size		= RESERVED_PMEM_GPU1,
};

static struct android_pmem_platform_data pmem_render_pdata = {
	.name		= "pmem_render",
	.no_allocator	= 1,
	.cached		= 0,
	.start		= RENDER_RESERVED_PMEM_START,
	.size		= RESERVED_PMEM_RENDER,
};

static struct android_pmem_platform_data pmem_stream_pdata = {
	.name		= "pmem_stream",
	.no_allocator	= 1,
	.cached		= 0,
	.start		= STREAM_RESERVED_PMEM_START,
	.size		= RESERVED_PMEM_STREAM,
};

static struct android_pmem_platform_data pmem_preview_pdata = {
	.name		= "pmem_preview",
	.no_allocator	= 1,
	.cached		= 0,
        .start		= PREVIEW_RESERVED_PMEM_START,
        .size		= RESERVED_PMEM_PREVIEW,
};

static struct android_pmem_platform_data pmem_picture_pdata = {
	.name		= "pmem_picture",
	.no_allocator	= 1,
	.cached		= 0,
        .start		= PICTURE_RESERVED_PMEM_START,
        .size		= RESERVED_PMEM_PICTURE,
};

static struct android_pmem_platform_data pmem_jpeg_pdata = {
	.name		= "pmem_jpeg",
	.no_allocator	= 1,
	.cached		= 0,
        .start		= JPEG_RESERVED_PMEM_START,
        .size		= RESERVED_PMEM_JPEG,
};

static struct android_pmem_platform_data pmem_skia_pdata = {
	.name		= "pmem_skia",
	.no_allocator	= 1,
	.cached		= 0,
        .start		= SKIA_RESERVED_PMEM_START,
        .size		= RESERVED_PMEM_SKIA,
};

static struct platform_device pmem_device = {
	.name		= "android_pmem",
	.id		= 0,
	.dev		= { .platform_data = &pmem_pdata },
};

static struct platform_device pmem_gpu1_device = {
	.name		= "android_pmem",
	.id		= 1,
	.dev		= { .platform_data = &pmem_gpu1_pdata },
};

static struct platform_device pmem_render_device = {
	.name		= "android_pmem",
	.id		= 2,
	.dev		= { .platform_data = &pmem_render_pdata },
};

static struct platform_device pmem_stream_device = {
	.name		= "android_pmem",
	.id		= 3,
	.dev		= { .platform_data = &pmem_stream_pdata },
};

static struct platform_device pmem_preview_device = {
	.name		= "android_pmem",
	.id		= 5,
	.dev		= { .platform_data = &pmem_preview_pdata },
};

static struct platform_device pmem_picture_device = {
	.name		= "android_pmem",
	.id		= 6,
	.dev		= { .platform_data = &pmem_picture_pdata },
};

static struct platform_device pmem_jpeg_device = {
	.name		= "android_pmem",
	.id		= 7,
	.dev		= { .platform_data = &pmem_jpeg_pdata },
};

static struct platform_device pmem_skia_device = {
	.name		= "android_pmem",
	.id		= 8,
	.dev		= { .platform_data = &pmem_skia_pdata },
};

static struct platform_device *pmem_devices[] = {
	&pmem_device,
	&pmem_gpu1_device,
	&pmem_render_device,
	&pmem_stream_device,
	&pmem_preview_device,
	&pmem_picture_device,
	&pmem_jpeg_device,
	&pmem_skia_device
};

static void __init tiny6410_add_mem_devices(void)
{
	unsigned i;
	for (i = 0; i < ARRAY_SIZE(pmem_devices); ++i)
		if (pmem_devices[i]->dev.platform_data) {
			struct android_pmem_platform_data *pmem =
					pmem_devices[i]->dev.platform_data;

			if (pmem->size)
				platform_device_register(pmem_devices[i]);
		}
}
#endif

static void __init tiny6410_machine_init(void)
{
	u32 cs1;
	struct tiny6410_features_t features = { 0 };

	s3c_pm_init();

	printk(KERN_INFO "TINY6410: Option string tiny6410=%s\n",
			tiny6410_features_str);

	/* Parse the feature string */
	tiny6410_parse_features(&features, tiny6410_features_str);

	tiny6410_lcd_pdata.win[0] = &tiny6410_fb_win[features.lcd_index];

	printk(KERN_INFO "TINY6410: selected LCD display is %dx%d\n",
		tiny6410_lcd_pdata.win[0]->win_mode.xres,
		tiny6410_lcd_pdata.win[0]->win_mode.yres);

	s3c_nand_set_platdata(&tiny6410_nand_info);
	s3c_fb_set_platdata(&tiny6410_lcd_pdata);
	s3c24xx_ts_set_platdata(&s3c_ts_platform);
	s3c_sdhci0_set_platdata(&tiny6410_hsmmc0_pdata);
	s3c_sdhci1_set_platdata(&tiny6410_hsmmc1_pdata);

	/* configure nCS1 width to 16 bits */

	cs1 = __raw_readl(S3C64XX_SROM_BW) &
		~(S3C64XX_SROM_BW__CS_MASK << S3C64XX_SROM_BW__NCS1__SHIFT);
	cs1 |= ((1 << S3C64XX_SROM_BW__DATAWIDTH__SHIFT) |
		(1 << S3C64XX_SROM_BW__WAITENABLE__SHIFT) |
		(1 << S3C64XX_SROM_BW__BYTEENABLE__SHIFT)) <<
			S3C64XX_SROM_BW__NCS1__SHIFT;
	__raw_writel(cs1, S3C64XX_SROM_BW);

	/* set timing for nCS1 suitable for ethernet chip */

	__raw_writel((0 << S3C64XX_SROM_BCX__PMC__SHIFT) |
		(6 << S3C64XX_SROM_BCX__TACP__SHIFT) |
		(4 << S3C64XX_SROM_BCX__TCAH__SHIFT) |
		(1 << S3C64XX_SROM_BCX__TCOH__SHIFT) |
		(13 << S3C64XX_SROM_BCX__TACC__SHIFT) |
		(4 << S3C64XX_SROM_BCX__TCOS__SHIFT) |
		(0 << S3C64XX_SROM_BCX__TACS__SHIFT), S3C64XX_SROM_BC1);

	gpio_request(S3C64XX_GPE(0), "LCD power");

	platform_add_devices(tiny6410_devices, ARRAY_SIZE(tiny6410_devices));

#ifdef CONFIG_ANDROID_PMEM
	/* Register PMEM devices */
	tiny6410_add_mem_devices();
#endif

	platform_add_devices(tiny6410_mod_devices, ARRAY_SIZE(tiny6410_mod_devices));
}

MACHINE_START(TINY6410, "TINY6410")
	.boot_params	= S3C64XX_PA_SDRAM + 0x100,
	.init_irq	= s3c6410_init_irq,
	.fixup		= tiny6410_fixup,
	.map_io		= tiny6410_map_io,
	.init_machine	= tiny6410_machine_init,
	.timer		= &s3c64xx_timer,
MACHINE_END
