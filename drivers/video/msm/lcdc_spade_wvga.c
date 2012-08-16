/* adapted from linux/arch/arm/mach-msm/panel-sonywvga-s6d16a0x21-7x30.c
 *
 * Copyright (c) 2009 Google Inc.
 * Copyright (c) 2009 HTC.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/delay.h>
#include <linux/err.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/wakelock.h>
#include <linux/leds.h>
#include <asm/mach-types.h>
#include <mach/panel_id.h>
#include <linux/gpio.h>
#include <linux/spi/spi.h>
#include <mach/atmega_microp.h>
#include "msm_fb.h"

#define DEBUG_LCM

#ifdef DEBUG_LCM
#define LCMDBG(fmt, arg...)     printk("[lcm]%s"fmt, __func__, ##arg)
#else
#define LCMDBG(fmt, arg...)     {}
#endif

#define BRIGHTNESS_DEFAULT_LEVEL        102

enum {
	PANEL_AUO,
	PANEL_SHARP,
	PANEL_UNKNOW
};

static int color_enhancement = 0;
int qspi_send_16bit(unsigned char id, unsigned data);
int qspi_send_9bit(struct spi_msg *msg);

static int spade_adjust_backlight(enum led_brightness val);

extern int panel_type;
static DEFINE_MUTEX(panel_lock);
static void (*panel_power_gpio)(int on);
static struct wake_lock panel_idle_lock;

static atomic_t lcm_init_done = ATOMIC_INIT(1);
static uint8_t last_val = BRIGHTNESS_DEFAULT_LEVEL;
static bool screen_on = true;


static struct msm_fb_panel_data spadewvga_panel_data;

struct lcm_cmd {
        uint8_t		cmd;
        uint8_t		data;
};

#define LCM_MDELAY	0x03

static void spadewvga_panel_power(int on)
{
  if (panel_power_gpio)
    (*panel_power_gpio)(on);
  if (on == 1 && (panel_type == PANEL_SHARP || panel_type == PANEL_ID_SPADE_AUO_N90))
    screen_on = true;
}

/* AUO */
static struct lcm_cmd auo_init_seq[] = {
        {0x1, 0xc0}, {0x0, 0x00}, {0x2, 0x86},
        {0x1, 0xc0}, {0x0, 0x01}, {0x2, 0x00},
        {0x1, 0xc0}, {0x0, 0x02}, {0x2, 0x86},
        {0x1, 0xc0}, {0x0, 0x03}, {0x2, 0x00},
        {0x1, 0xc1}, {0x0, 0x00}, {0x2, 0x40},
        {0x1, 0xc2}, {0x0, 0x00}, {0x2, 0x02},
	{LCM_MDELAY, 1},
        {0x1, 0xc2}, {0x0, 0x02}, {0x2, 0x32},

        /* Gamma setting: start */
        {0x1, 0xe0}, {0x0, 0x00}, {0x2, 0x0e },
        {0x1, 0xe0}, {0x0, 0x01}, {0x2, 0x34},
        {0x1, 0xe0}, {0x0, 0x02}, {0x2, 0x3f},
        {0x1, 0xe0}, {0x0, 0x03}, {0x2, 0x49},
        {0x1, 0xe0}, {0x0, 0x04}, {0x2, 0x1d},
        {0x1, 0xe0}, {0x0, 0x05}, {0x2, 0x2c},
        {0x1, 0xe0}, {0x0, 0x06}, {0x2, 0x5f},
        {0x1, 0xe0}, {0x0, 0x07}, {0x2, 0x3a},
        {0x1, 0xe0}, {0x0, 0x08}, {0x2, 0x20},
        {0x1, 0xe0}, {0x0, 0x09}, {0x2, 0x28},
        {0x1, 0xe0}, {0x0, 0x0a}, {0x2, 0x80},
        {0x1, 0xe0}, {0x0, 0x0b}, {0x2, 0x13},
        {0x1, 0xe0}, {0x0, 0x0c}, {0x2, 0x32},
        {0x1, 0xe0}, {0x0, 0x0d}, {0x2, 0x56},
        {0x1, 0xe0}, {0x0, 0x0e}, {0x2, 0x79},
        {0x1, 0xe0}, {0x0, 0x0f}, {0x2, 0xb8},
        {0x1, 0xe0}, {0x0, 0x10}, {0x2, 0x55},
        {0x1, 0xe0}, {0x0, 0x11}, {0x2, 0x57},

        {0x1, 0xe1}, {0x0, 0x00}, {0x2, 0x0e},
        {0x1, 0xe1}, {0x0, 0x01}, {0x2, 0x34},
        {0x1, 0xe1}, {0x0, 0x02}, {0x2, 0x3f},
        {0x1, 0xe1}, {0x0, 0x03}, {0x2, 0x49},
        {0x1, 0xe1}, {0x0, 0x04}, {0x2, 0x1d},
        {0x1, 0xe1}, {0x0, 0x05}, {0x2, 0x2c},
        {0x1, 0xe1}, {0x0, 0x06}, {0x2, 0x5f},
        {0x1, 0xe1}, {0x0, 0x07}, {0x2, 0x3a},
        {0x1, 0xe1}, {0x0, 0x08}, {0x2, 0x20},
        {0x1, 0xe1}, {0x0, 0x09}, {0x2, 0x28},
        {0x1, 0xe1}, {0x0, 0x0a}, {0x2, 0x80},
        {0x1, 0xe1}, {0x0, 0x0b}, {0x2, 0x13},
        {0x1, 0xe1}, {0x0, 0x0c}, {0x2, 0x32},
        {0x1, 0xe1}, {0x0, 0x0d}, {0x2, 0x56},
        {0x1, 0xe1}, {0x0, 0x0e}, {0x2, 0x79},
        {0x1, 0xe1}, {0x0, 0x0f}, {0x2, 0xb8},
        {0x1, 0xe1}, {0x0, 0x10}, {0x2, 0x55},
        {0x1, 0xe1}, {0x0, 0x11}, {0x2, 0x57},

        {0x1, 0xe2}, {0x0, 0x00}, {0x2, 0x0e},
        {0x1, 0xe2}, {0x0, 0x01}, {0x2, 0x34},
        {0x1, 0xe2}, {0x0, 0x02}, {0x2, 0x3f},
        {0x1, 0xe2}, {0x0, 0x03}, {0x2, 0x49},
        {0x1, 0xe2}, {0x0, 0x04}, {0x2, 0x1d},
        {0x1, 0xe2}, {0x0, 0x05}, {0x2, 0x2c},
        {0x1, 0xe2}, {0x0, 0x06}, {0x2, 0x5d},
        {0x1, 0xe2}, {0x0, 0x07}, {0x2, 0x3a},
        {0x1, 0xe2}, {0x0, 0x08}, {0x2, 0x20},
        {0x1, 0xe2}, {0x0, 0x09}, {0x2, 0x28},
        {0x1, 0xe2}, {0x0, 0x0a}, {0x2, 0x80},
        {0x1, 0xe2}, {0x0, 0x0b}, {0x2, 0x13},
        {0x1, 0xe2}, {0x0, 0x0c}, {0x2, 0x32},
        {0x1, 0xe2}, {0x0, 0x0d}, {0x2, 0x56},
        {0x1, 0xe2}, {0x0, 0x0e}, {0x2, 0x79},
        {0x1, 0xe2}, {0x0, 0x0f}, {0x2, 0xb8},
        {0x1, 0xe2}, {0x0, 0x10}, {0x2, 0x55},
        {0x1, 0xe2}, {0x0, 0x11}, {0x2, 0x57},

        {0x1, 0xe3}, {0x0, 0x00}, {0x2, 0x0e},
        {0x1, 0xe3}, {0x0, 0x01}, {0x2, 0x34},
        {0x1, 0xe3}, {0x0, 0x02}, {0x2, 0x3f},
        {0x1, 0xe3}, {0x0, 0x03}, {0x2, 0x49},
        {0x1, 0xe3}, {0x0, 0x04}, {0x2, 0x1d},
        {0x1, 0xe3}, {0x0, 0x05}, {0x2, 0x2c},
        {0x1, 0xe3}, {0x0, 0x06}, {0x2, 0x5f},
        {0x1, 0xe3}, {0x0, 0x07}, {0x2, 0x3a},
        {0x1, 0xe3}, {0x0, 0x08}, {0x2, 0x20},
        {0x1, 0xe3}, {0x0, 0x09}, {0x2, 0x28},
        {0x1, 0xe3}, {0x0, 0x0a}, {0x2, 0x80},
        {0x1, 0xe3}, {0x0, 0x0b}, {0x2, 0x13},
        {0x1, 0xe3}, {0x0, 0x0c}, {0x2, 0x32},
        {0x1, 0xe3}, {0x0, 0x0d}, {0x2, 0x56},
        {0x1, 0xe3}, {0x0, 0x0e}, {0x2, 0x79},
        {0x1, 0xe3}, {0x0, 0x0f}, {0x2, 0xb8},
        {0x1, 0xe3}, {0x0, 0x10}, {0x2, 0x55},
        {0x1, 0xe3}, {0x0, 0x11}, {0x2, 0x57},

        {0x1, 0xe4}, {0x0, 0x00}, {0x2, 0x0e},
        {0x1, 0xe4}, {0x0, 0x01}, {0x2, 0x34},
        {0x1, 0xe4}, {0x0, 0x02}, {0x2, 0x3f},
        {0x1, 0xe4}, {0x0, 0x03}, {0x2, 0x49},
        {0x1, 0xe4}, {0x0, 0x04}, {0x2, 0x1d},
        {0x1, 0xe4}, {0x0, 0x05}, {0x2, 0x2c},
        {0x1, 0xe4}, {0x0, 0x06}, {0x2, 0x5f},
        {0x1, 0xe4}, {0x0, 0x07}, {0x2, 0x3a},
        {0x1, 0xe4}, {0x0, 0x08}, {0x2, 0x20},
        {0x1, 0xe4}, {0x0, 0x09}, {0x2, 0x28},
        {0x1, 0xe4}, {0x0, 0x0a}, {0x2, 0x80},
        {0x1, 0xe4}, {0x0, 0x0b}, {0x2, 0x13},
        {0x1, 0xe4}, {0x0, 0x0c}, {0x2, 0x32},
        {0x1, 0xe4}, {0x0, 0x0d}, {0x2, 0x56},
        {0x1, 0xe4}, {0x0, 0x0e}, {0x2, 0x79},
        {0x1, 0xe4}, {0x0, 0x0f}, {0x2, 0xb8},
        {0x1, 0xe4}, {0x0, 0x10}, {0x2, 0x55},
        {0x1, 0xe4}, {0x0, 0x11}, {0x2, 0x57},

        {0x1, 0xe5}, {0x0, 0x00}, {0x2, 0x0e},
        {0x1, 0xe5}, {0x0, 0x01}, {0x2, 0x34},
        {0x1, 0xe5}, {0x0, 0x02}, {0x2, 0x3f},
        {0x1, 0xe5}, {0x0, 0x03}, {0x2, 0x49},
        {0x1, 0xe5}, {0x0, 0x04}, {0x2, 0x1d},
        {0x1, 0xe5}, {0x0, 0x05}, {0x2, 0x2c},
        {0x1, 0xe5}, {0x0, 0x06}, {0x2, 0x5f},
        {0x1, 0xe5}, {0x0, 0x07}, {0x2, 0x3a},
        {0x1, 0xe5}, {0x0, 0x08}, {0x2, 0x20},
        {0x1, 0xe5}, {0x0, 0x09}, {0x2, 0x28},
        {0x1, 0xe5}, {0x0, 0x0a}, {0x2, 0x80},
        {0x1, 0xe5}, {0x0, 0x0b}, {0x2, 0x13},
        {0x1, 0xe5}, {0x0, 0x0c}, {0x2, 0x32},
        {0x1, 0xe5}, {0x0, 0x0d}, {0x2, 0x56},
        {0x1, 0xe5}, {0x0, 0x0e}, {0x2, 0x79},
        {0x1, 0xe5}, {0x0, 0x0f}, {0x2, 0xb8},
        {0x1, 0xe5}, {0x0, 0x10}, {0x2, 0x55},
        {0x1, 0xe5}, {0x0, 0x11}, {0x2, 0x57},
        /* Gamma setting: done */

	/* Set RGB-888 */
        {0x1, 0x3a}, {0x0, 0x00}, {0x2, 0x70},
        {0x1, 0x11}, {0x0, 0x00}, {0x2, 0x00},
        {LCM_MDELAY, 120},
        {0x1, 0x29}, {0x0, 0x0 }, {0x2, 0x0 },
};

static struct lcm_cmd auo_uninit_seq[] = {
	{0x1, 0x28}, {0x0, 0x00}, {0x2, 0x00},
	{LCM_MDELAY, 34},
	{0x1, 0x10}, {0x0, 0x00}, {0x2, 0x00},
	{LCM_MDELAY, 100},
};

static struct lcm_cmd auo_sleep_in_seq[] = {
	{0x1, 0x28}, {0x0, 0x00}, {0x2, 0x00},
	{0x1, 0x11}, {0x0, 0x00}, {0x2, 0x00},
	{LCM_MDELAY, 5},
	{0x1, 0x4f}, {0x0, 0x00}, {0x2, 0x01},
};

static int lcm_auo_write_seq(struct lcm_cmd *cmd_table, unsigned size)
{
        int i;

        for (i = 0; i < size; i++) {
		if (cmd_table[i].cmd == LCM_MDELAY) {
			hr_msleep(cmd_table[i].data);
			continue;
		}
		qspi_send_16bit(cmd_table[i].cmd, cmd_table[i].data);
	}
        return 0;
}

static int spade_auo_panel_init(void)
{
	LCMDBG("\n");

	mutex_lock(&panel_lock);
	spadewvga_panel_power(1);
	lcm_auo_write_seq(auo_init_seq, ARRAY_SIZE(auo_init_seq));
	mutex_unlock(&panel_lock);
	return 0;
}

static int spade_auo_panel_unblank(struct platform_device *pdev)
{
	LCMDBG("\n");

	if (spade_auo_panel_init())
		printk(KERN_ERR "spade_auo_panel_init failed\n");

#if 0
	if (color_enhancement == 0) {
		spade_mdp_color_enhancement(mdp_pdata.mdp_dev);
		color_enhancement = 1;
	}
#endif
	atomic_set(&lcm_init_done, 1);
	spade_adjust_backlight(last_val);

	return 0;
}

static int spade_auo_panel_blank(struct platform_device *pdev)
{
	LCMDBG("\n");
	spade_adjust_backlight(0);
	atomic_set(&lcm_init_done, 0);
	mutex_lock(&panel_lock);
	lcm_auo_write_seq(auo_uninit_seq, ARRAY_SIZE(auo_uninit_seq));
	mutex_unlock(&panel_lock);
        spadewvga_panel_power(0);

	return 0;
}

/* AUO N90 */
static struct lcm_cmd auo_n90_init_seq[] = {
/*Enable page 1*/
{0x1, 0xF0}, {0x0, 0x00}, {0x2, 0x55},
{0x1, 0xF0}, {0x0, 0x01}, {0x2, 0xAA},
{0x1, 0xF0}, {0x0, 0x02}, {0x2, 0x52},
{0x1, 0xF0}, {0x0, 0x03}, {0x2, 0x08},
{0x1, 0xF0}, {0x0, 0x04}, {0x2, 0x01},

/* Gamma setting: start */
{0x1, 0xD1}, {0x0, 0x00}, {0x2, 0x00},
{0x1, 0xD1}, {0x0, 0x01}, {0x2, 0x20},
{0x1, 0xD1}, {0x0, 0x02}, {0x2, 0x00},
{0x1, 0xD1}, {0x0, 0x03}, {0x2, 0x2B},
{0x1, 0xD1}, {0x0, 0x04}, {0x2, 0x00},
{0x1, 0xD1}, {0x0, 0x05}, {0x2, 0x3C},
{0x1, 0xD1}, {0x0, 0x06}, {0x2, 0x00},
{0x1, 0xD1}, {0x0, 0x07}, {0x2, 0x56},
{0x1, 0xD1}, {0x0, 0x08}, {0x2, 0x00},
{0x1, 0xD1}, {0x0, 0x09}, {0x2, 0x68},
{0x1, 0xD1}, {0x0, 0x0A}, {0x2, 0x00},
{0x1, 0xD1}, {0x0, 0x0B}, {0x2, 0x87},
{0x1, 0xD1}, {0x0, 0x0C}, {0x2, 0x00},
{0x1, 0xD1}, {0x0, 0x0D}, {0x2, 0x9E},
{0x1, 0xD1}, {0x0, 0x0E}, {0x2, 0x00},
{0x1, 0xD1}, {0x0, 0x0F}, {0x2, 0xC6},
{0x1, 0xD1}, {0x0, 0x10}, {0x2, 0x00},
{0x1, 0xD1}, {0x0, 0x11}, {0x2, 0xE4},
{0x1, 0xD1}, {0x0, 0x12}, {0x2, 0x01},
{0x1, 0xD1}, {0x0, 0x13}, {0x2, 0x12},
{0x1, 0xD1}, {0x0, 0x14}, {0x2, 0x01},
{0x1, 0xD1}, {0x0, 0x15}, {0x2, 0x37},
{0x1, 0xD1}, {0x0, 0x16}, {0x2, 0x01},
{0x1, 0xD1}, {0x0, 0x17}, {0x2, 0x75},
{0x1, 0xD1}, {0x0, 0x18}, {0x2, 0x01},
{0x1, 0xD1}, {0x0, 0x19}, {0x2, 0xA5},
{0x1, 0xD1}, {0x0, 0x1A}, {0x2, 0x01},
{0x1, 0xD1}, {0x0, 0x1B}, {0x2, 0xA6},
{0x1, 0xD1}, {0x0, 0x1C}, {0x2, 0x01},
{0x1, 0xD1}, {0x0, 0x1D}, {0x2, 0xD0},
{0x1, 0xD1}, {0x0, 0x1E}, {0x2, 0x01},
{0x1, 0xD1}, {0x0, 0x1F}, {0x2, 0xF5},
{0x1, 0xD1}, {0x0, 0x20}, {0x2, 0x02},
{0x1, 0xD1}, {0x0, 0x21}, {0x2, 0x0A},
{0x1, 0xD1}, {0x0, 0x22}, {0x2, 0x02},
{0x1, 0xD1}, {0x0, 0x23}, {0x2, 0x26},
{0x1, 0xD1}, {0x0, 0x24}, {0x2, 0x02},
{0x1, 0xD1}, {0x0, 0x25}, {0x2, 0x3B},
{0x1, 0xD1}, {0x0, 0x26}, {0x2, 0x02},
{0x1, 0xD1}, {0x0, 0x27}, {0x2, 0x6B},
{0x1, 0xD1}, {0x0, 0x28}, {0x2, 0x02},
{0x1, 0xD1}, {0x0, 0x29}, {0x2, 0x99},
{0x1, 0xD1}, {0x0, 0x2A}, {0x2, 0x02},
{0x1, 0xD1}, {0x0, 0x2B}, {0x2, 0xDD},
{0x1, 0xD1}, {0x0, 0x2C}, {0x2, 0x03},
{0x1, 0xD1}, {0x0, 0x2D}, {0x2, 0x10},
{0x1, 0xD1}, {0x0, 0x2E}, {0x2, 0x03},
{0x1, 0xD1}, {0x0, 0x2F}, {0x2, 0x26},
{0x1, 0xD1}, {0x0, 0x30}, {0x2, 0x03},
{0x1, 0xD1}, {0x0, 0x31}, {0x2, 0x32},
{0x1, 0xD1}, {0x0, 0x32}, {0x2, 0x03},
{0x1, 0xD1}, {0x0, 0x33}, {0x2, 0x9A},

{0x1, 0xD2}, {0x0, 0x00}, {0x2, 0x00},
{0x1, 0xD2}, {0x0, 0x01}, {0x2, 0xA0},
{0x1, 0xD2}, {0x0, 0x02}, {0x2, 0x00},
{0x1, 0xD2}, {0x0, 0x03}, {0x2, 0xA9},
{0x1, 0xD2}, {0x0, 0x04}, {0x2, 0x00},
{0x1, 0xD2}, {0x0, 0x05}, {0x2, 0xB5},
{0x1, 0xD2}, {0x0, 0x06}, {0x2, 0x00},
{0x1, 0xD2}, {0x0, 0x07}, {0x2, 0xBF},
{0x1, 0xD2}, {0x0, 0x08}, {0x2, 0x00},
{0x1, 0xD2}, {0x0, 0x09}, {0x2, 0xC9},
{0x1, 0xD2}, {0x0, 0x0A}, {0x2, 0x00},
{0x1, 0xD2}, {0x0, 0x0B}, {0x2, 0xDC},
{0x1, 0xD2}, {0x0, 0x0C}, {0x2, 0x00},
{0x1, 0xD2}, {0x0, 0x0D}, {0x2, 0xEE},
{0x1, 0xD2}, {0x0, 0x0E}, {0x2, 0x01},
{0x1, 0xD2}, {0x0, 0x0F}, {0x2, 0x0A},
{0x1, 0xD2}, {0x0, 0x10}, {0x2, 0x01},
{0x1, 0xD2}, {0x0, 0x11}, {0x2, 0x21},
{0x1, 0xD2}, {0x0, 0x12}, {0x2, 0x01},
{0x1, 0xD2}, {0x0, 0x13}, {0x2, 0x48},
{0x1, 0xD2}, {0x0, 0x14}, {0x2, 0x01},
{0x1, 0xD2}, {0x0, 0x15}, {0x2, 0x67},
{0x1, 0xD2}, {0x0, 0x16}, {0x2, 0x01},
{0x1, 0xD2}, {0x0, 0x17}, {0x2, 0x97},
{0x1, 0xD2}, {0x0, 0x18}, {0x2, 0x01},
{0x1, 0xD2}, {0x0, 0x19}, {0x2, 0xBE},
{0x1, 0xD2}, {0x0, 0x1A}, {0x2, 0x01},
{0x1, 0xD2}, {0x0, 0x1B}, {0x2, 0xC0},
{0x1, 0xD2}, {0x0, 0x1C}, {0x2, 0x01},
{0x1, 0xD2}, {0x0, 0x1D}, {0x2, 0xE1},
{0x1, 0xD2}, {0x0, 0x1E}, {0x2, 0x02},
{0x1, 0xD2}, {0x0, 0x1F}, {0x2, 0x04},
{0x1, 0xD2}, {0x0, 0x20}, {0x2, 0x02},
{0x1, 0xD2}, {0x0, 0x21}, {0x2, 0x17},
{0x1, 0xD2}, {0x0, 0x22}, {0x2, 0x02},
{0x1, 0xD2}, {0x0, 0x23}, {0x2, 0x36},
{0x1, 0xD2}, {0x0, 0x24}, {0x2, 0x02},
{0x1, 0xD2}, {0x0, 0x25}, {0x2, 0x50},
{0x1, 0xD2}, {0x0, 0x26}, {0x2, 0x02},
{0x1, 0xD2}, {0x0, 0x27}, {0x2, 0x7E},
{0x1, 0xD2}, {0x0, 0x28}, {0x2, 0x02},
{0x1, 0xD2}, {0x0, 0x29}, {0x2, 0xAC},
{0x1, 0xD2}, {0x0, 0x2A}, {0x2, 0x02},
{0x1, 0xD2}, {0x0, 0x2B}, {0x2, 0xF1},
{0x1, 0xD2}, {0x0, 0x2C}, {0x2, 0x03},
{0x1, 0xD2}, {0x0, 0x2D}, {0x2, 0x20},
{0x1, 0xD2}, {0x0, 0x2E}, {0x2, 0x03},
{0x1, 0xD2}, {0x0, 0x2F}, {0x2, 0x38},
{0x1, 0xD2}, {0x0, 0x30}, {0x2, 0x03},
{0x1, 0xD2}, {0x0, 0x31}, {0x2, 0x43},
{0x1, 0xD2}, {0x0, 0x32}, {0x2, 0x03},
{0x1, 0xD2}, {0x0, 0x33}, {0x2, 0x9A},

{0x1, 0xD3}, {0x0, 0x00}, {0x2, 0x00},
{0x1, 0xD3}, {0x0, 0x01}, {0x2, 0x50},
{0x1, 0xD3}, {0x0, 0x02}, {0x2, 0x00},
{0x1, 0xD3}, {0x0, 0x03}, {0x2, 0x53},
{0x1, 0xD3}, {0x0, 0x04}, {0x2, 0x00},
{0x1, 0xD3}, {0x0, 0x05}, {0x2, 0x73},
{0x1, 0xD3}, {0x0, 0x06}, {0x2, 0x00},
{0x1, 0xD3}, {0x0, 0x07}, {0x2, 0x89},
{0x1, 0xD3}, {0x0, 0x08}, {0x2, 0x00},
{0x1, 0xD3}, {0x0, 0x09}, {0x2, 0x9F},
{0x1, 0xD3}, {0x0, 0x0A}, {0x2, 0x00},
{0x1, 0xD3}, {0x0, 0x0B}, {0x2, 0xC1},
{0x1, 0xD3}, {0x0, 0x0C}, {0x2, 0x00},
{0x1, 0xD3}, {0x0, 0x0D}, {0x2, 0xDA},
{0x1, 0xD3}, {0x0, 0x0E}, {0x2, 0x01},
{0x1, 0xD3}, {0x0, 0x0F}, {0x2, 0x02},
{0x1, 0xD3}, {0x0, 0x10}, {0x2, 0x01},
{0x1, 0xD3}, {0x0, 0x11}, {0x2, 0x23},
{0x1, 0xD3}, {0x0, 0x12}, {0x2, 0x01},
{0x1, 0xD3}, {0x0, 0x13}, {0x2, 0x50},
{0x1, 0xD3}, {0x0, 0x14}, {0x2, 0x01},
{0x1, 0xD3}, {0x0, 0x15}, {0x2, 0x6F},
{0x1, 0xD3}, {0x0, 0x16}, {0x2, 0x01},
{0x1, 0xD3}, {0x0, 0x17}, {0x2, 0x9F},
{0x1, 0xD3}, {0x0, 0x18}, {0x2, 0x01},
{0x1, 0xD3}, {0x0, 0x19}, {0x2, 0xC5},
{0x1, 0xD3}, {0x0, 0x1A}, {0x2, 0x01},
{0x1, 0xD3}, {0x0, 0x1B}, {0x2, 0xC6},
{0x1, 0xD3}, {0x0, 0x1C}, {0x2, 0x01},
{0x1, 0xD3}, {0x0, 0x1D}, {0x2, 0xE3},
{0x1, 0xD3}, {0x0, 0x1E}, {0x2, 0x02},
{0x1, 0xD3}, {0x0, 0x1F}, {0x2, 0x08},
{0x1, 0xD3}, {0x0, 0x20}, {0x2, 0x02},
{0x1, 0xD3}, {0x0, 0x21}, {0x2, 0x16},
{0x1, 0xD3}, {0x0, 0x22}, {0x2, 0x02},
{0x1, 0xD3}, {0x0, 0x23}, {0x2, 0x2B},
{0x1, 0xD3}, {0x0, 0x24}, {0x2, 0x02},
{0x1, 0xD3}, {0x0, 0x25}, {0x2, 0x4D},
{0x1, 0xD3}, {0x0, 0x26}, {0x2, 0x02},
{0x1, 0xD3}, {0x0, 0x27}, {0x2, 0x6F},
{0x1, 0xD3}, {0x0, 0x28}, {0x2, 0x02},
{0x1, 0xD3}, {0x0, 0x29}, {0x2, 0x8C},
{0x1, 0xD3}, {0x0, 0x2A}, {0x2, 0x02},
{0x1, 0xD3}, {0x0, 0x2B}, {0x2, 0xD6},
{0x1, 0xD3}, {0x0, 0x2C}, {0x2, 0x03},
{0x1, 0xD3}, {0x0, 0x2D}, {0x2, 0x12},
{0x1, 0xD3}, {0x0, 0x2E}, {0x2, 0x03},
{0x1, 0xD3}, {0x0, 0x2F}, {0x2, 0x28},
{0x1, 0xD3}, {0x0, 0x30}, {0x2, 0x03},
{0x1, 0xD3}, {0x0, 0x31}, {0x2, 0x3E},
{0x1, 0xD3}, {0x0, 0x32}, {0x2, 0x03},
{0x1, 0xD3}, {0x0, 0x33}, {0x2, 0x9A},

{0x1, 0xD4}, {0x0, 0x00}, {0x2, 0x00},
{0x1, 0xD4}, {0x0, 0x01}, {0x2, 0x20},
{0x1, 0xD4}, {0x0, 0x02}, {0x2, 0x00},
{0x1, 0xD4}, {0x0, 0x03}, {0x2, 0x2B},
{0x1, 0xD4}, {0x0, 0x04}, {0x2, 0x00},
{0x1, 0xD4}, {0x0, 0x05}, {0x2, 0x3C},
{0x1, 0xD4}, {0x0, 0x06}, {0x2, 0x00},
{0x1, 0xD4}, {0x0, 0x07}, {0x2, 0x56},
{0x1, 0xD4}, {0x0, 0x08}, {0x2, 0x00},
{0x1, 0xD4}, {0x0, 0x09}, {0x2, 0x68},
{0x1, 0xD4}, {0x0, 0x0A}, {0x2, 0x00},
{0x1, 0xD4}, {0x0, 0x0B}, {0x2, 0x87},
{0x1, 0xD4}, {0x0, 0x0C}, {0x2, 0x00},
{0x1, 0xD4}, {0x0, 0x0D}, {0x2, 0x9E},
{0x1, 0xD4}, {0x0, 0x0E}, {0x2, 0x00},
{0x1, 0xD4}, {0x0, 0x0F}, {0x2, 0xC6},
{0x1, 0xD4}, {0x0, 0x10}, {0x2, 0x00},
{0x1, 0xD4}, {0x0, 0x11}, {0x2, 0xE4},
{0x1, 0xD4}, {0x0, 0x12}, {0x2, 0x01},
{0x1, 0xD4}, {0x0, 0x13}, {0x2, 0x12},
{0x1, 0xD4}, {0x0, 0x14}, {0x2, 0x01},
{0x1, 0xD4}, {0x0, 0x15}, {0x2, 0x37},
{0x1, 0xD4}, {0x0, 0x16}, {0x2, 0x01},
{0x1, 0xD4}, {0x0, 0x17}, {0x2, 0x75},
{0x1, 0xD4}, {0x0, 0x18}, {0x2, 0x01},
{0x1, 0xD4}, {0x0, 0x19}, {0x2, 0xA5},
{0x1, 0xD4}, {0x0, 0x1A}, {0x2, 0x01},
{0x1, 0xD4}, {0x0, 0x1B}, {0x2, 0xA6},
{0x1, 0xD4}, {0x0, 0x1C}, {0x2, 0x01},
{0x1, 0xD4}, {0x0, 0x1D}, {0x2, 0xD0},
{0x1, 0xD4}, {0x0, 0x1E}, {0x2, 0x01},
{0x1, 0xD4}, {0x0, 0x1F}, {0x2, 0xF5},
{0x1, 0xD4}, {0x0, 0x20}, {0x2, 0x02},
{0x1, 0xD4}, {0x0, 0x21}, {0x2, 0x0A},
{0x1, 0xD4}, {0x0, 0x22}, {0x2, 0x02},
{0x1, 0xD4}, {0x0, 0x23}, {0x2, 0x26},
{0x1, 0xD4}, {0x0, 0x24}, {0x2, 0x02},
{0x1, 0xD4}, {0x0, 0x25}, {0x2, 0x3B},
{0x1, 0xD4}, {0x0, 0x26}, {0x2, 0x02},
{0x1, 0xD4}, {0x0, 0x27}, {0x2, 0x6B},
{0x1, 0xD4}, {0x0, 0x28}, {0x2, 0x02},
{0x1, 0xD4}, {0x0, 0x29}, {0x2, 0x99},
{0x1, 0xD4}, {0x0, 0x2A}, {0x2, 0x02},
{0x1, 0xD4}, {0x0, 0x2B}, {0x2, 0xDD},
{0x1, 0xD4}, {0x0, 0x2C}, {0x2, 0x03},
{0x1, 0xD4}, {0x0, 0x2D}, {0x2, 0x10},
{0x1, 0xD4}, {0x0, 0x2E}, {0x2, 0x03},
{0x1, 0xD4}, {0x0, 0x2F}, {0x2, 0x26},
{0x1, 0xD4}, {0x0, 0x30}, {0x2, 0x03},
{0x1, 0xD4}, {0x0, 0x31}, {0x2, 0x32},
{0x1, 0xD4}, {0x0, 0x32}, {0x2, 0x03},
{0x1, 0xD4}, {0x0, 0x33}, {0x2, 0x9A},

{0x1, 0xD5}, {0x0, 0x00}, {0x2, 0x00},
{0x1, 0xD5}, {0x0, 0x01}, {0x2, 0xA0},
{0x1, 0xD5}, {0x0, 0x02}, {0x2, 0x00},
{0x1, 0xD5}, {0x0, 0x03}, {0x2, 0xA9},
{0x1, 0xD5}, {0x0, 0x04}, {0x2, 0x00},
{0x1, 0xD5}, {0x0, 0x05}, {0x2, 0xB5},
{0x1, 0xD5}, {0x0, 0x06}, {0x2, 0x00},
{0x1, 0xD5}, {0x0, 0x07}, {0x2, 0xBF},
{0x1, 0xD5}, {0x0, 0x08}, {0x2, 0x00},
{0x1, 0xD5}, {0x0, 0x09}, {0x2, 0xC9},
{0x1, 0xD5}, {0x0, 0x0A}, {0x2, 0x00},
{0x1, 0xD5}, {0x0, 0x0B}, {0x2, 0xDC},
{0x1, 0xD5}, {0x0, 0x0C}, {0x2, 0x00},
{0x1, 0xD5}, {0x0, 0x0D}, {0x2, 0xEE},
{0x1, 0xD5}, {0x0, 0x0E}, {0x2, 0x01},
{0x1, 0xD5}, {0x0, 0x0F}, {0x2, 0x0A},
{0x1, 0xD5}, {0x0, 0x10}, {0x2, 0x01},
{0x1, 0xD5}, {0x0, 0x11}, {0x2, 0x21},
{0x1, 0xD5}, {0x0, 0x12}, {0x2, 0x01},
{0x1, 0xD5}, {0x0, 0x13}, {0x2, 0x48},
{0x1, 0xD5}, {0x0, 0x14}, {0x2, 0x01},
{0x1, 0xD5}, {0x0, 0x15}, {0x2, 0x67},
{0x1, 0xD5}, {0x0, 0x16}, {0x2, 0x01},
{0x1, 0xD5}, {0x0, 0x17}, {0x2, 0x97},
{0x1, 0xD5}, {0x0, 0x18}, {0x2, 0x01},
{0x1, 0xD5}, {0x0, 0x19}, {0x2, 0xBE},
{0x1, 0xD5}, {0x0, 0x1A}, {0x2, 0x01},
{0x1, 0xD5}, {0x0, 0x1B}, {0x2, 0xC0},
{0x1, 0xD5}, {0x0, 0x1C}, {0x2, 0x01},
{0x1, 0xD5}, {0x0, 0x1D}, {0x2, 0xE1},
{0x1, 0xD5}, {0x0, 0x1E}, {0x2, 0x02},
{0x1, 0xD5}, {0x0, 0x1F}, {0x2, 0x04},
{0x1, 0xD5}, {0x0, 0x20}, {0x2, 0x02},
{0x1, 0xD5}, {0x0, 0x21}, {0x2, 0x17},
{0x1, 0xD5}, {0x0, 0x22}, {0x2, 0x02},
{0x1, 0xD5}, {0x0, 0x23}, {0x2, 0x36},
{0x1, 0xD5}, {0x0, 0x24}, {0x2, 0x02},
{0x1, 0xD5}, {0x0, 0x25}, {0x2, 0x50},
{0x1, 0xD5}, {0x0, 0x26}, {0x2, 0x02},
{0x1, 0xD5}, {0x0, 0x27}, {0x2, 0x7E},
{0x1, 0xD5}, {0x0, 0x28}, {0x2, 0x02},
{0x1, 0xD5}, {0x0, 0x29}, {0x2, 0xAC},
{0x1, 0xD5}, {0x0, 0x2A}, {0x2, 0x02},
{0x1, 0xD5}, {0x0, 0x2B}, {0x2, 0xF1},
{0x1, 0xD5}, {0x0, 0x2C}, {0x2, 0x03},
{0x1, 0xD5}, {0x0, 0x2D}, {0x2, 0x20},
{0x1, 0xD5}, {0x0, 0x2E}, {0x2, 0x03},
{0x1, 0xD5}, {0x0, 0x2F}, {0x2, 0x38},
{0x1, 0xD5}, {0x0, 0x30}, {0x2, 0x03},
{0x1, 0xD5}, {0x0, 0x31}, {0x2, 0x43},
{0x1, 0xD5}, {0x0, 0x32}, {0x2, 0x03},
{0x1, 0xD5}, {0x0, 0x33}, {0x2, 0x9A},

{0x1, 0xD6}, {0x0, 0x00}, {0x2, 0x00},
{0x1, 0xD6}, {0x0, 0x01}, {0x2, 0x50},
{0x1, 0xD6}, {0x0, 0x02}, {0x2, 0x00},
{0x1, 0xD6}, {0x0, 0x03}, {0x2, 0x53},
{0x1, 0xD6}, {0x0, 0x04}, {0x2, 0x00},
{0x1, 0xD6}, {0x0, 0x05}, {0x2, 0x73},
{0x1, 0xD6}, {0x0, 0x06}, {0x2, 0x00},
{0x1, 0xD6}, {0x0, 0x07}, {0x2, 0x89},
{0x1, 0xD6}, {0x0, 0x08}, {0x2, 0x00},
{0x1, 0xD6}, {0x0, 0x09}, {0x2, 0x9F},
{0x1, 0xD6}, {0x0, 0x0A}, {0x2, 0x00},
{0x1, 0xD6}, {0x0, 0x0B}, {0x2, 0xC1},
{0x1, 0xD6}, {0x0, 0x0C}, {0x2, 0x00},
{0x1, 0xD6}, {0x0, 0x0D}, {0x2, 0xDA},
{0x1, 0xD6}, {0x0, 0x0E}, {0x2, 0x01},
{0x1, 0xD6}, {0x0, 0x0F}, {0x2, 0x02},
{0x1, 0xD6}, {0x0, 0x10}, {0x2, 0x01},
{0x1, 0xD6}, {0x0, 0x11}, {0x2, 0x23},
{0x1, 0xD6}, {0x0, 0x12}, {0x2, 0x01},
{0x1, 0xD6}, {0x0, 0x13}, {0x2, 0x50},
{0x1, 0xD6}, {0x0, 0x14}, {0x2, 0x01},
{0x1, 0xD6}, {0x0, 0x15}, {0x2, 0x6F},
{0x1, 0xD6}, {0x0, 0x16}, {0x2, 0x01},
{0x1, 0xD6}, {0x0, 0x17}, {0x2, 0x9F},
{0x1, 0xD6}, {0x0, 0x18}, {0x2, 0x01},
{0x1, 0xD6}, {0x0, 0x19}, {0x2, 0xC5},
{0x1, 0xD6}, {0x0, 0x1A}, {0x2, 0x01},
{0x1, 0xD6}, {0x0, 0x1B}, {0x2, 0xC6},
{0x1, 0xD6}, {0x0, 0x1C}, {0x2, 0x01},
{0x1, 0xD6}, {0x0, 0x1D}, {0x2, 0xE3},
{0x1, 0xD6}, {0x0, 0x1E}, {0x2, 0x02},
{0x1, 0xD6}, {0x0, 0x1F}, {0x2, 0x08},
{0x1, 0xD6}, {0x0, 0x20}, {0x2, 0x02},
{0x1, 0xD6}, {0x0, 0x21}, {0x2, 0x16},
{0x1, 0xD6}, {0x0, 0x22}, {0x2, 0x02},
{0x1, 0xD6}, {0x0, 0x23}, {0x2, 0x2B},
{0x1, 0xD6}, {0x0, 0x24}, {0x2, 0x02},
{0x1, 0xD6}, {0x0, 0x25}, {0x2, 0x4D},
{0x1, 0xD6}, {0x0, 0x26}, {0x2, 0x02},
{0x1, 0xD6}, {0x0, 0x27}, {0x2, 0x6F},
{0x1, 0xD6}, {0x0, 0x28}, {0x2, 0x02},
{0x1, 0xD6}, {0x0, 0x29}, {0x2, 0x8C},
{0x1, 0xD6}, {0x0, 0x2A}, {0x2, 0x02},
{0x1, 0xD6}, {0x0, 0x2B}, {0x2, 0xD6},
{0x1, 0xD6}, {0x0, 0x2C}, {0x2, 0x03},
{0x1, 0xD6}, {0x0, 0x2D}, {0x2, 0x12},
{0x1, 0xD6}, {0x0, 0x2E}, {0x2, 0x03},
{0x1, 0xD6}, {0x0, 0x2F}, {0x2, 0x28},
{0x1, 0xD6}, {0x0, 0x30}, {0x2, 0x03},
{0x1, 0xD6}, {0x0, 0x31}, {0x2, 0x3E},
{0x1, 0xD6}, {0x0, 0x32}, {0x2, 0x03},
{0x1, 0xD6}, {0x0, 0x33}, {0x2, 0x9A},
/* Gamma setting: done */

/* VGLX  Voltage Setting */
{0x1, 0xBA}, {0x0, 0x00}, {0x2, 0x14},
{0x1, 0xBA}, {0x0, 0x01}, {0x2, 0x14},
{0x1, 0xBA}, {0x0, 0x02}, {0x2, 0x14},

/* VGH	Voltage Setting */
{0x1, 0xBF}, {0x0, 0x00}, {0x2, 0x01},
{0x1, 0xB3}, {0x0, 0x00}, {0x2, 0x07},
{0x1, 0xB3}, {0x0, 0x01}, {0x2, 0x07},
{0x1, 0xB3}, {0x0, 0x02}, {0x2, 0x07},
{0x1, 0xB9}, {0x0, 0x00}, {0x2, 0x25},
{0x1, 0xB9}, {0x0, 0x01}, {0x2, 0x25},
{0x1, 0xB9}, {0x0, 0x02}, {0x2, 0x25},

/* Gamma Voltage */
{0x1, 0xBC}, {0x0, 0x01}, {0x2, 0xA0},
{0x1, 0xBC}, {0x0, 0x02}, {0x2, 0x00},
{0x1, 0xBD}, {0x0, 0x01}, {0x2, 0xA0},
{0x1, 0xBD}, {0x0, 0x02}, {0x2, 0x00},

/* Enable Page 0 */
{0x1, 0xF0}, {0x0, 0x00}, {0x2, 0x55},
{0x1, 0xF0}, {0x0, 0x01}, {0x2, 0xAA},
{0x1, 0xF0}, {0x0, 0x02}, {0x2, 0x52},
{0x1, 0xF0}, {0x0, 0x03}, {0x2, 0x08},
{0x1, 0xF0}, {0x0, 0x04}, {0x2, 0x00},

/* RAM Keep */
{0x1, 0xB1}, {0x0, 0x00}, {0x2, 0xCC},

/* Z-Inversion */
{0x1, 0xBC}, {0x0, 0x00}, {0x2, 0x05},
{0x1, 0xBC}, {0x0, 0x01}, {0x2, 0x05},
{0x1, 0xBC}, {0x0, 0x02}, {0x2, 0x05},

/* Proch Lines */
{0x1, 0xBD}, {0x0, 0x02}, {0x2, 0x07},
{0x1, 0xBD}, {0x0, 0x03}, {0x2, 0x31},
{0x1, 0xBE}, {0x0, 0x02}, {0x2, 0x07},
{0x1, 0xBE}, {0x0, 0x03}, {0x2, 0x31},
{0x1, 0xBF}, {0x0, 0x02}, {0x2, 0x07},
{0x1, 0xBF}, {0x0, 0x03}, {0x2, 0x31},

/* Enable LV3 */
{0x1, 0xFF}, {0x0, 0x00}, {0x2, 0xAA},
{0x1, 0xFF}, {0x0, 0x01}, {0x2, 0x55},
{0x1, 0xFF}, {0x0, 0x02}, {0x2, 0x25},
{0x1, 0xFF}, {0x0, 0x03}, {0x2, 0x01},

/* TE On */
{0x1, 0x35}, {0x0, 0x00}, {0x2, 0x00},

/* Sleep Out */
{0x1, 0x11}, {0x0, 0x00}, {0x2, 0x00},

/* Delay 120 */
{0x3, 120},

/* Display on */
{0x1, 0x29}, {0x0, 0x00}, {0x2, 0x00},

/* Delay 100 */
{0x3, 100},
};

static struct lcm_cmd auo_n90_uninit_seq[] = {
	{0x1, 0x28}, {0x0, 0x00}, {0x2, 0x00},
	{LCM_MDELAY, 34},
	{0x1, 0x10}, {0x0, 0x00}, {0x2, 0x00},
	{LCM_MDELAY, 100},
};

static struct lcm_cmd auo_n90_sleep_in_seq[] = {
	{0x1, 0x28}, {0x0, 0x00}, {0x2, 0x00},
	{LCM_MDELAY, 100},
	{0x1, 0x11}, {0x0, 0x00}, {0x2, 0x00},
	{LCM_MDELAY, 5},
	{0x1, 0x4f}, {0x0, 0x00}, {0x2, 0x01},
};

static int spade_auo_n90_panel_init(void)
{
	LCMDBG("\n");

	mutex_lock(&panel_lock);
	spadewvga_panel_power(1);
	lcm_auo_write_seq(auo_n90_init_seq, ARRAY_SIZE(auo_n90_init_seq));
	mutex_unlock(&panel_lock);
	return 0;
}

static int spade_auo_n90_panel_unblank(struct platform_device *pdev)
{
	LCMDBG("\n");
	if (spade_auo_n90_panel_init())
		printk(KERN_ERR "spade_auo_panel_init failed\n");

#if 0
	if (color_enhancement == 0) {
		spade_mdp_color_enhancement(mdp_pdata.mdp_dev);
		color_enhancement = 1;
	}
#endif
	atomic_set(&lcm_init_done, 1);
	spade_adjust_backlight(last_val);

	return 0;
}

static int spade_auo_n90_panel_blank(struct platform_device *pdev)
{
	LCMDBG("\n");
	spade_adjust_backlight(0);
	atomic_set(&lcm_init_done, 0);
	mutex_lock(&panel_lock);
	lcm_auo_write_seq(auo_n90_uninit_seq, ARRAY_SIZE(auo_n90_uninit_seq));
	mutex_unlock(&panel_lock);
        spadewvga_panel_power(0);

	return 0;
}

/* SHARP */
#define LCM_CMD(_cmd, ...)                                      \
{                                                               \
	.cmd = _cmd,                                            \
	.data = (u8 []){__VA_ARGS__},                           \
	.len = sizeof((u8 []){__VA_ARGS__}) / sizeof(u8)        \
}

static struct spi_msg sharp_init_seq[] = {
        LCM_CMD(0x11), LCM_CMD(LCM_MDELAY, 110), LCM_CMD(0x29),
};

static struct spi_msg sharp_uninit_seq[] = {
        LCM_CMD(0x28), LCM_CMD(0x10),
};

static int lcm_sharp_write_seq(struct spi_msg *cmd_table, unsigned size)
{
	int i;

	for (i = 0; i < size; i++) {
		if (cmd_table[i].cmd == LCM_MDELAY) {
			hr_msleep(cmd_table[i].data[0]);
			continue;
		}
		qspi_send_9bit(cmd_table + i);
	}
	return 0;
}

static int spade_sharp_panel_init(void)
{
        LCMDBG("\n");

        mutex_lock(&panel_lock);
        spadewvga_panel_power(1);
        lcm_sharp_write_seq(sharp_init_seq, ARRAY_SIZE(sharp_init_seq));
        mutex_unlock(&panel_lock);
        return 0;
}

static int spade_sharp_panel_unblank(struct platform_device *pdev)
{
	LCMDBG("\n");

	if (spade_sharp_panel_init())
		printk(KERN_ERR "spade_auo_panel_init failed\n");

#if 0
	if (color_enhancement == 0) {
		spade_mdp_color_enhancement(mdp_pdata.mdp_dev);
		color_enhancement = 1;
	}
#endif
	atomic_set(&lcm_init_done, 1);
	spade_adjust_backlight(last_val);

        return 0;
}

static int spade_sharp_panel_blank(struct platform_device *pdev)
{
	LCMDBG("\n");
	spade_adjust_backlight(0);
	atomic_set(&lcm_init_done, 0);

        mutex_lock(&panel_lock);
        lcm_sharp_write_seq(sharp_uninit_seq, ARRAY_SIZE(sharp_uninit_seq));
	screen_on = false;
        mutex_unlock(&panel_lock);
        spadewvga_panel_power(0);

        return 0;
}

static struct platform_device this_device = {
	.name	= "lcdc_panel",
	.id	= 1,
	.dev	= {
		.platform_data = &spadewvga_panel_data,
	},
};

/*----------------------------------------------------------------------------*/

static int spade_adjust_backlight(enum led_brightness val)
{
	uint32_t def_bl;
        uint8_t data[4] = {     /* PWM setting of microp, see p.8 */
                0x05,           /* Fading time; suggested: 5/10/15/20/25 */
                val,            /* Duty Cycle */
                0x00,           /* Channel H byte */
                0x20,           /* Channel L byte */
                };
	uint8_t shrink_br;


	if (panel_type == PANEL_ID_SPADE_SHA_N90)
		def_bl = 101;
	else
		def_bl = 91;

        mutex_lock(&panel_lock);
        if (val == 0)
                shrink_br = 0;
        else if (val <= 30)
                shrink_br = 7;
        else if ((val > 30) && (val <= 143))
                shrink_br = (def_bl - 7) * (val - 30) / (143 - 30) + 7;
        else
                shrink_br = (217 - def_bl) * (val - 143) / (255 - 143) + def_bl;

	if (shrink_br == 0)
		 data[0] = 0;
        data[1] = shrink_br;

        microp_i2c_write(0x25, data, sizeof(data));
        last_val = shrink_br ? shrink_br: last_val;
        mutex_unlock(&panel_lock);

	return shrink_br;
}

static void spade_brightness_set(struct led_classdev *led_cdev,
		enum led_brightness val)
{
	if (atomic_read(&lcm_init_done) == 0) {
		last_val = val ? val : last_val;
		LCMDBG(":lcm not ready, val=%d\n", val);
		return;
	}
	led_cdev->brightness = spade_adjust_backlight(val);
}

static struct led_classdev spade_backlight_led = {
	.name = "lcd-backlight",
	.brightness = LED_FULL,
	.brightness_set = spade_brightness_set,
};

static int spade_backlight_probe(struct platform_device *pdev)
{
	int rc;

	rc = led_classdev_register(&pdev->dev, &spade_backlight_led);
	if (rc)
		LCMDBG("backlight: failure on register led_classdev\n");
	return 0;
}

static struct platform_device spade_backlight = {
	.name = "spade-backlight",
};

static struct platform_driver spade_backlight_driver = {
	.probe          = spade_backlight_probe,
	.driver         = {
		.name   = "spade-backlight",
		.owner  = THIS_MODULE,
	},
};

static int __init spadewvga_init_panel(void)
{
	int ret;

	/* set gpio to proper state in the beginning */
	if (panel_power_gpio)
		(*panel_power_gpio)(1);

	wake_lock_init(&panel_idle_lock, WAKE_LOCK_SUSPEND,
			"backlight_present");

	ret = platform_device_register(&spade_backlight);
	if (ret)
		return ret;

	return 0;
}

static int spadewvga_probe(struct platform_device *pdev)
{
	int rc = -EIO;
	struct msm_panel_common_pdata *lcdc_spadewvga_pdata;

	pr_info("%s: id=%d\n", __func__, pdev->id);

	/* power control */
	lcdc_spadewvga_pdata = pdev->dev.platform_data;
	panel_power_gpio = lcdc_spadewvga_pdata->panel_config_gpio;

	rc = spadewvga_init_panel();
	if (rc)
		printk(KERN_ERR "%s fail %d\n", __func__, rc);

	return rc;
}

static struct platform_driver this_driver = {
	.probe = spadewvga_probe,
	.driver = {
		.name = "lcdc_spade_wvga"
	},
};

int spade_panel_sleep_in(void)
{
	int ret;

	mutex_lock(&panel_lock);
	LCMDBG(", screen=%s\n", screen_on ? "on" : "off");
	if (screen_on) {
		mutex_unlock(&panel_lock);
		return 0;
	}

        spadewvga_panel_power(1);
	switch (panel_type) {
		case PANEL_AUO:
			lcm_auo_write_seq(auo_sleep_in_seq,
				ARRAY_SIZE(auo_sleep_in_seq));
			ret = 0;
			break;
		case PANEL_ID_SPADE_AUO_N90:
			lcm_auo_write_seq(auo_n90_sleep_in_seq,
				ARRAY_SIZE(auo_n90_sleep_in_seq));
			ret = 0;
			break;
		case PANEL_SHARP:
		case PANEL_ID_SPADE_SHA_N90:
			lcm_sharp_write_seq(sharp_uninit_seq,
				ARRAY_SIZE(sharp_uninit_seq));
			ret = 0;
			break;
		default:
			ret = -EINVAL;
			break;
	}
	mutex_unlock(&panel_lock);
	return ret;
}

static int __init spadewvga_init(void)
{
	int ret;
	struct msm_panel_info *pinfo;

#ifdef CONFIG_FB_MSM_MDDI_AUTO_DETECT
	if (msm_fb_detect_client("lcdc_s6d16a0x21_wvga")) {
		pr_err("%s: detect failed\n", __func__);
		return 0;
	}
#endif

	ret = platform_driver_register(&this_driver);
	if (ret) {
		pr_err("%s: driver register failed, rc=%d\n", __func__, ret);
		return ret;
	}

	pinfo = &spadewvga_panel_data.panel_info;
	pinfo->xres = 480;
	pinfo->yres = 800;
	pinfo->type = LCDC_PANEL;
	pinfo->pdest = DISPLAY_1;
	pinfo->wait_cycle = 0;
	pinfo->bpp = 18;
	pinfo->fb_num = 2;
	pinfo->bl_max = 255;
	pinfo->bl_min = 1;
        
        pinfo->clk_rate = 24576000;
        pinfo->lcdc.border_clr = 0;
        pinfo->lcdc.underflow_clr = 0xff;
        pinfo->lcdc.hsync_skew = 0;
        
        switch (panel_type)
          {
          case PANEL_AUO:
            pinfo->lcdc.h_back_porch = 30;
            pinfo->lcdc.h_front_porch = 2;
            pinfo->lcdc.h_pulse_width = 2;
            pinfo->lcdc.v_back_porch = 5;
            pinfo->lcdc.v_front_porch = 2;
            pinfo->lcdc.v_pulse_width = 2;
            spadewvga_panel_data.on = spade_auo_panel_unblank;
            spadewvga_panel_data.off = spade_auo_panel_blank;
            break;
          case PANEL_ID_SPADE_AUO_N90:
            pinfo->lcdc.h_back_porch = 8;
            pinfo->lcdc.h_front_porch = 8;
            pinfo->lcdc.h_pulse_width = 4;
            pinfo->lcdc.v_back_porch = 8;
            pinfo->lcdc.v_front_porch = 8;
            pinfo->lcdc.v_pulse_width = 4;
            spadewvga_panel_data.on = spade_auo_n90_panel_unblank;
            spadewvga_panel_data.off = spade_auo_n90_panel_blank;
            break;
          case PANEL_SHARP:
          case PANEL_ID_SPADE_SHA_N90:
            pinfo->clk_rate = 24576000;
            pinfo->lcdc.h_back_porch = 21;
            pinfo->lcdc.h_front_porch = 6;
            pinfo->lcdc.h_pulse_width = 6;
            pinfo->lcdc.v_back_porch = 6;
            pinfo->lcdc.v_front_porch = 3;
            pinfo->lcdc.v_pulse_width = 3;
            spadewvga_panel_data.on = spade_sharp_panel_unblank;
            spadewvga_panel_data.off = spade_sharp_panel_blank;
            break;
          default:
            return -EINVAL;
            break;
          }

	ret = platform_device_register(&this_device);
	if (ret) {
		printk(KERN_ERR "%s not able to register the device\n",
			__func__);
		platform_driver_unregister(&this_driver);
	}

	return ret;
}

static int __init spade_backlight_init(void)
{
	return platform_driver_register(&spade_backlight_driver);
}

device_initcall(spadewvga_init);
module_init(spade_backlight_init);

