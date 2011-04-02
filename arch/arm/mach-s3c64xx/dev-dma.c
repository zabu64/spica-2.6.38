/* linux/arch/arm/mach-s3c64xx/dev-dma.c
 *
 * Copyright 2011 Tomasz Figa <tomasz.figa at gmail.com>
 *
 * S3C series device definition for PL080S DMA engines
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>

#include <mach/irqs.h>
#include <mach/map.h>

#include <linux/amba/pl08x.h>

/*
 * PL080S DMA engines
 */

static struct pl08x_channel_data s3c64xx_dma0_slaves[] = {
	{
		.bus_id		= "uart0_0",
		.min_signal	= 0,
		.max_signal	= 0,
		.periph_buses	= PL08X_AHB2,
	}, {
		.bus_id		= "uart0_1",
		.min_signal	= 1,
		.max_signal	= 1,
		.periph_buses	= PL08X_AHB2,
	}, {
		.bus_id		= "uart1_0",
		.min_signal	= 2,
		.max_signal	= 2,
		.periph_buses	= PL08X_AHB2,
	}, {
		.bus_id		= "uart1_1",
		.min_signal	= 3,
		.max_signal	= 3,
		.periph_buses	= PL08X_AHB2,
	}, {
		.bus_id		= "uart2_0",
		.min_signal	= 4,
		.max_signal	= 4,
		.periph_buses	= PL08X_AHB2,
	}, {
		.bus_id		= "uart2_1",
		.min_signal	= 5,
		.max_signal	= 5,
		.periph_buses	= PL08X_AHB2,
	}, {
		.bus_id		= "uart3_0",
		.min_signal	= 6,
		.max_signal	= 6,
		.periph_buses	= PL08X_AHB2,
	}, {
		.bus_id		= "uart3_1",
		.min_signal	= 7,
		.max_signal	= 7,
		.periph_buses	= PL08X_AHB2,
	}, {
		.bus_id		= "pcm0_tx",
		.min_signal	= 8,
		.max_signal	= 8,
		.periph_buses	= PL08X_AHB2,
	}, {
		.bus_id		= "pcm0_rx",
		.min_signal	= 9,
		.max_signal	= 9,
		.periph_buses	= PL08X_AHB2,
	}, {
		.bus_id		= "i2s0_tx",
		.min_signal	= 10,
		.max_signal	= 10,
		.periph_buses	= PL08X_AHB2,
	}, {
		.bus_id		= "i2s0_rx",
		.min_signal	= 11,
		.max_signal	= 11,
		.periph_buses	= PL08X_AHB2,
	}, {
		.bus_id		= "spi0_tx",
		.min_signal	= 12,
		.max_signal	= 12,
		.periph_buses	= PL08X_AHB2,
	}, {
		.bus_id		= "spi0_rx",
		.min_signal	= 13,
		.max_signal	= 13,
		.periph_buses	= PL08X_AHB2,
	}, {
		.bus_id		= "i2sv40_tx",
		.min_signal	= 14,
		.max_signal	= 14,
		.periph_buses	= PL08X_AHB2,
	}, {
		.bus_id		= "i2sv40_rx",
		.min_signal	= 15,
		.max_signal	= 15,
		.periph_buses	= PL08X_AHB2,
	},
};

static struct pl08x_platform_data s3c64xx_dma0_pdata = {
	.slave_channels		= s3c64xx_dma0_slaves,
	.num_slave_channels	= ARRAY_SIZE(s3c64xx_dma0_slaves),
	.lli_buses		= PL08X_AHB1,
	.mem_buses		= PL08X_AHB1,
	.clock			= "dma0",
};

static struct resource s3c64xx_dma0_resources[] = {
	[0] = {
		.start	= S3C64XX_PA_DMA0,
		.end	= S3C64XX_PA_DMA0 + 0x200 - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= IRQ_DMA0,
		.end	= IRQ_DMA0,
		.flags	= IORESOURCE_IRQ,
	},
};

static u64 s3c64xx_dma0_dma_mask = 0xffffffff;

struct platform_device s3c64xx_device_dma0 = {
	.name		= "pl080s-dmac",
	.id		= 0,
	.resource	= s3c64xx_dma0_resources,
	.num_resources	= ARRAY_SIZE(s3c64xx_dma0_resources),
	.dev		= {
		.platform_data		= &s3c64xx_dma0_pdata,
		.dma_mask		= &s3c64xx_dma0_dma_mask,
		.coherent_dma_mask	= 0xffffffff,
	}
};
EXPORT_SYMBOL(s3c64xx_device_dma0);

static struct pl08x_channel_data s3c64xx_dma1_slaves[] = {
	{
		.bus_id		= "pcm1_tx",
		.min_signal	= 0,
		.max_signal	= 0,
		.periph_buses	= PL08X_AHB2,
	}, {
		.bus_id		= "pcm1_rx",
		.min_signal	= 1,
		.max_signal	= 1,
		.periph_buses	= PL08X_AHB2,
	}, {
		.bus_id		= "i2s1_tx",
		.min_signal	= 2,
		.max_signal	= 2,
		.periph_buses	= PL08X_AHB2,
	}, {
		.bus_id		= "i2s1_rx",
		.min_signal	= 3,
		.max_signal	= 3,
		.periph_buses	= PL08X_AHB2,
	}, {
		.bus_id		= "spi1_tx",
		.min_signal	= 4,
		.max_signal	= 4,
		.periph_buses	= PL08X_AHB2,
	}, {
		.bus_id		= "spi1_rx",
		.min_signal	= 5,
		.max_signal	= 5,
		.periph_buses	= PL08X_AHB2,
	}, {
		.bus_id		= "ac97_out",
		.min_signal	= 6,
		.max_signal	= 6,
		.periph_buses	= PL08X_AHB2,
	}, {
		.bus_id		= "ac97_in",
		.min_signal	= 7,
		.max_signal	= 7,
		.periph_buses	= PL08X_AHB2,
	}, {
		.bus_id		= "ac97_mic",
		.min_signal	= 8,
		.max_signal	= 8,
		.periph_buses	= PL08X_AHB2,
	}, {
		.bus_id		= "pwm",
		.min_signal	= 9,
		.max_signal	= 9,
		.periph_buses	= PL08X_AHB2,
	}, {
		.bus_id		= "irda",
		.min_signal	= 10,
		.max_signal	= 10,
		.periph_buses	= PL08X_AHB2,
	}, {
		.bus_id		= "external",
		.min_signal	= 11,
		.max_signal	= 11,
		.periph_buses	= PL08X_AHB2,
	},
};

static struct pl08x_platform_data s3c64xx_dma1_pdata = {
	.slave_channels		= s3c64xx_dma1_slaves,
	.num_slave_channels	= ARRAY_SIZE(s3c64xx_dma1_slaves),
	.lli_buses		= PL08X_AHB1,
	.mem_buses		= PL08X_AHB1,
	.clock			= "dma1",
};

static struct resource s3c64xx_dma1_resources[] = {
	[0] = {
		.start	= S3C64XX_PA_DMA1,
		.end	= S3C64XX_PA_DMA1 + 0x200 - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= IRQ_DMA1,
		.end	= IRQ_DMA1,
		.flags	= IORESOURCE_IRQ,
	},
};

static u64 s3c64xx_dma1_dma_mask = 0xffffffff;

struct platform_device s3c64xx_device_dma1 = {
	.name		= "pl080s-dmac",
	.id		= 1,
	.resource	= s3c64xx_dma1_resources,
	.num_resources	= ARRAY_SIZE(s3c64xx_dma1_resources),
	.dev		= {
		.platform_data	= &s3c64xx_dma1_pdata,
		.dma_mask		= &s3c64xx_dma1_dma_mask,
		.coherent_dma_mask	= 0xffffffff,
	}
};
EXPORT_SYMBOL(s3c64xx_device_dma1);
