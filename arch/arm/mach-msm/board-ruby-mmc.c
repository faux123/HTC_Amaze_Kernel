/* linux/arch/arm/mach-msm/board-ruby-mmc.c
 *
 * Copyright (C) 2008 HTC Corporation.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/mmc/host.h>
#include <linux/mmc/sdio_ids.h>
#include <linux/err.h>
#include <linux/debugfs.h>
#include <linux/gpio.h>

#include <asm/gpio.h>
#include <asm/io.h>

#include <mach/vreg.h>
#include <mach/htc_pwrsink.h>

#include <asm/mach/mmc.h>

#include "devices.h"
#include "board-ruby.h"
#include "proc_comm.h"
#include <mach/msm_iomap.h>
#include <linux/mfd/pmic8058.h>
#include <mach/htc_sleep_clk.h>
#include <mach/htc_fast_clk.h>

#if 0
static int msm_sdcc_cfg_mpm_sdiowakeup(struct device *dev, unsigned mode)
{
	struct platform_device *pdev;
	enum msm_mpm_pin pin;
	int ret = 0;

	pdev = container_of(dev, struct platform_device, dev);

	/* Only SDCC4 slot connected to WLAN chip has wakeup capability */
	if (pdev->id == 4)
		pin = MSM_MPM_PIN_SDC4_DAT1;
	else
		return -EINVAL;

	switch (mode) {
	case SDC_DAT1_DISABLE:
		ret = msm_mpm_enable_pin(pin, 0);
		break;
	case SDC_DAT1_ENABLE:
		ret = msm_mpm_set_pin_type(pin, IRQ_TYPE_LEVEL_LOW);
		ret = msm_mpm_enable_pin(pin, 1);
		break;
	case SDC_DAT1_ENWAKE:
		ret = msm_mpm_set_pin_wake(pin, 1);
		break;
	case SDC_DAT1_DISWAKE:
		ret = msm_mpm_set_pin_wake(pin, 0);
		break;
	default:
		ret = -EINVAL;
		break;
	}
	return ret;
}
#endif

/* #include <linux/module.h> */
int msm_proc_comm(unsigned cmd, unsigned *data1, unsigned *data2);

extern int msm_add_sdcc(unsigned int controller, struct mmc_platform_data *plat);

/* ---- SDCARD ---- */
/* ---- WIFI ---- */


static uint32_t wifi_on_gpio_table[] = {
	/* GPIO_CFG(116, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_4MA), */ /* DAT3 */
	/* GPIO_CFG(117, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_4MA), */ /* DAT2 */
	/* GPIO_CFG(118, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_4MA), */ /* DAT1 */
	/* GPIO_CFG(119, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_4MA), */ /* DAT0 */
	/* GPIO_CFG(111, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA), */ /* CMD */
	/* GPIO_CFG(110, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA), */ /* CLK */
	GPIO_CFG(RUBY_GPIO_WIFI_IRQ, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_4MA), /* WLAN IRQ */
};

static uint32_t wifi_off_gpio_table[] = {
	/* GPIO_CFG(116, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_4MA), */ /* DAT3 */
	/* GPIO_CFG(117, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_4MA), */ /* DAT2 */
	/* GPIO_CFG(118, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_4MA), */ /* DAT1 */
	/* GPIO_CFG(119, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_4MA), */ /* DAT0 */
	/* GPIO_CFG(111, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_4MA), */ /* CMD */
	/* GPIO_CFG(110, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_4MA), */ /* CLK */
	GPIO_CFG(RUBY_GPIO_WIFI_IRQ, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_4MA), /* WLAN IRQ */
};



static void config_gpio_table(uint32_t *table, int len)
{
	int n, rc;
	for (n = 0; n < len; n++) {
		rc = gpio_tlmm_config(table[n], GPIO_CFG_ENABLE);
		if (rc) {
			pr_err("%s: gpio_tlmm_config(%#x)=%d\n",
				__func__, table[n], rc);
			break;
		}
	}
}



#ifdef CONFIG_TIWLAN_SDIO
static struct sdio_embedded_func wifi_func_array[] = {
	{
		.f_class        = SDIO_CLASS_NONE,
		.f_maxblksize   = 512,
	},
	{
		.f_class        = SDIO_CLASS_WLAN,
		.f_maxblksize   = 512,
	},
};

static struct embedded_sdio_data ruby_wifi_emb_data = {
	.cis    = {
		.vendor         = SDIO_VENDOR_ID_TI,
		.device         = SDIO_DEVICE_ID_TI_WL12xx,
		.blksize        = 512,
		.max_dtr        = 25000000,
	},
	.cccr   = {
		.multi_block	= 1,
		.low_speed	= 0,
		.wide_bus	= 1,
		.high_power	= 0,
		.high_speed	= 0,
		.disable_cd	= 1,
	},
	.funcs  = wifi_func_array,
	.num_funcs = 2,
};

static void (*wifi_status_cb)(int card_present, void *dev_id);
static void *wifi_status_cb_devid;

static int
ruby_wifi_status_register(void (*callback)(int card_present, void *dev_id),
				void *dev_id)
{
	if (wifi_status_cb)
		return -EAGAIN;

	wifi_status_cb = callback;
	wifi_status_cb_devid = dev_id;
	return 0;
}

static int ruby_wifi_cd;	/* WiFi virtual 'card detect' status */

static unsigned int ruby_wifi_status(struct device *dev)
{
	return ruby_wifi_cd;
}
#endif

static unsigned int ruby_sdc_slot_type = MMC_TYPE_SDIO_WIFI;
static struct mmc_platform_data ruby_wifi_data = {
//#ifdef CONFIG_TIWLAN_SDIO
	.ocr_mask		= MMC_VDD_165_195,
//#else
//	.ocr_mask		= MMC_VDD_28_29,
//#endif
#ifdef CONFIG_TIWLAN_SDIO
	.status			= ruby_wifi_status,
	.register_status_notify	= ruby_wifi_status_register,
	.embedded_sdio		= &ruby_wifi_emb_data,
#endif
	.mmc_bus_width		= MMC_CAP_4_BIT_DATA,
	.msmsdcc_fmin		= 400000,
	.msmsdcc_fmid		= 24000000,
	.msmsdcc_fmax		= 48000000,
	.slot_type		= &ruby_sdc_slot_type,
	.nonremovable		= 1,
	.pclk_src_dfab		= 1,
};

#ifdef CONFIG_TIWLAN_SDIO
int ruby_wifi_set_carddetect(int val)
{
	printk(KERN_INFO "%s: %d\n", __func__, val);
	ruby_wifi_cd = val;
	if (wifi_status_cb)
		wifi_status_cb(val, wifi_status_cb_devid);
	else
		printk(KERN_WARNING "%s: Nobody to notify\n", __func__);
	return 0;
}
EXPORT_SYMBOL(ruby_wifi_set_carddetect);
#endif

int ti_wifi_power(int on)
{

	const unsigned SDC4_HDRV_PULL_CTL_ADDR = (unsigned) MSM_TLMM_BASE + 0x20A0;

	printk(KERN_INFO "%s: %d\n", __func__, on);

	if (on) {
		/*enable osc clk*/
		htc_wifi_bt_fast_clk_ctl(on, ID_WIFI);
		mdelay(100);
		/*enable 32k sleep clk*/
		htc_wifi_bt_sleep_clk_ctl(on, ID_WIFI);
		mdelay(100);
		/*config wifi shundown pin*/
		gpio_set_value(RUBY_GPIO_WIFI_SHUTDOWN_N, 1); /* WIFI_SHUTDOWN */
		msleep(15);
		gpio_set_value(RUBY_GPIO_WIFI_SHUTDOWN_N, 0);
		msleep(1);
		gpio_set_value(RUBY_GPIO_WIFI_SHUTDOWN_N, 1);
		msleep(70);
		/*config wifi gpio*/
		config_gpio_table(wifi_on_gpio_table,
				  ARRAY_SIZE(wifi_on_gpio_table));
		mdelay(200);
		/*wifi data pin enable,SDC4_CMD_PULL = Pull Up, SDC4_DATA_PULL = Pull up*/
		writel(0x1FDB, SDC4_HDRV_PULL_CTL_ADDR);
	} else {
		/*wifi data pin enable,SDC4_CMD_PULL = Pull Up, SDC4_DATA_PULL = Pull up*/
		writel(0x1FDB, SDC4_HDRV_PULL_CTL_ADDR);
		config_gpio_table(wifi_off_gpio_table,
				  ARRAY_SIZE(wifi_off_gpio_table));
		/*config wifi shundown pin*/
		gpio_set_value(RUBY_GPIO_WIFI_SHUTDOWN_N, on); /* WIFI_SHUTDOWN */
		mdelay(1);
		/*disable 32k clk*/
		htc_wifi_bt_sleep_clk_ctl(on, ID_WIFI);
		mdelay(1);
		/*disable osc clk*/
		htc_wifi_bt_fast_clk_ctl(on, ID_WIFI);
	}


	mdelay(120);

	return 0;
}
EXPORT_SYMBOL(ti_wifi_power);


static int ruby_wifi_reset_state;
int ruby_wifi_reset(int on)
{
	printk(KERN_WARNING"%s: %d\n", __func__, on);
	ruby_wifi_reset_state = on;
	return 0;
}

int __init ruby_init_mmc()
{
	uint32_t id;
	const unsigned SDC4_HDRV_PULL_CTL_ADDR = (unsigned)(MSM_TLMM_BASE + 0x20A0);
#ifdef CONFIG_TIWLAN_SDIO
	wifi_status_cb = NULL;
#endif

	printk(KERN_INFO "ruby: %s\n", __func__);

	/* initial WIFI_SHUTDOWN# */
	id = GPIO_CFG(RUBY_GPIO_WIFI_SHUTDOWN_N, 0, GPIO_CFG_OUTPUT,
		GPIO_CFG_NO_PULL, GPIO_CFG_2MA);

	/*WIFI:Per HW request,we need pull up wlan SDCC4 DATA/CMD pin when initial to prevent power leakage issue*/
	writel(0x1FDB, SDC4_HDRV_PULL_CTL_ADDR);

	gpio_tlmm_config(id, 0);
	gpio_set_value(RUBY_GPIO_WIFI_SHUTDOWN_N, 0);

	msm_add_sdcc(4, &ruby_wifi_data);

	return 0;
}
