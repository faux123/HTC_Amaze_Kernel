/* arch/arm/mach-msm/include/mach/htc_fast_clk.c
 *
 * Copyright (C) 2011 HTC, Inc.
 * Author: assd bt <htc_ssdbt@htc.com>
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
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/device.h>
#include <asm/mach-types.h>

#include <mach/htc_fast_clk.h>
#include <linux/mfd/pmic8058.h>
#include <linux/gpio.h>
#include <linux/err.h>
#include <linux/regulator/consumer.h>

static int htc_fast_clk_pin;
static int htc_fast_clk_state_wifi;
static int htc_fast_clk_state_bt;
static DEFINE_MUTEX(htc_fast_clk_mutex);

static struct regulator *vreg_osc;

/* pm8058 config */
static struct pm_gpio pmic_gpio_fast_clk_output = {
	.direction      = PM_GPIO_DIR_OUT,
	.output_buffer  = PM_GPIO_OUT_BUF_CMOS,
	.output_value   = 0,
	.pull           = PM_GPIO_PULL_NO,
	.vin_sel        = PM8058_GPIO_VIN_L5,      /* L5 2.85 V */
	.out_strength   = PM_GPIO_STRENGTH_HIGH,
	.function       = PM_GPIO_FUNC_NORMAL,
};

static int set_wifi_bt_fast_clk(int on)
{
	int err = 0;

	printk(KERN_DEBUG "%s pin=%d\n", __func__, htc_fast_clk_pin);

	if (on) {
		if (vreg_osc) {
			err = regulator_enable(vreg_osc);
			if (err)
				pr_err("%s: regulator_enable error: %d", __func__, err);
		}

		printk(KERN_DEBUG "EN FAST CLK\n");
		pmic_gpio_fast_clk_output.output_value = 1;
	} else {
		if (vreg_osc) {
			err = regulator_disable(vreg_osc);
			if (err)
				pr_err("%s: regulator_disable error: %d", __func__, err);
		}

		printk(KERN_DEBUG "DIS FAST CLK\n");
		pmic_gpio_fast_clk_output.output_value = 0;
	}

	err = pm8xxx_gpio_config(htc_fast_clk_pin,
					&pmic_gpio_fast_clk_output);


	if (err) {
		if (on)
			printk(KERN_DEBUG "ERR EN FAST CLK, ERR=%d\n", err);
		else
			printk(KERN_DEBUG "ERR DIS FAST CLK, ERR=%d\n", err);
	}

	return err;
}

int htc_wifi_bt_fast_clk_ctl(int on, int id)
{
	int err = 0;

	printk(KERN_DEBUG "%s ON=%d, ID=%d\n", __func__, on, id);

	if (htc_fast_clk_pin < 0) {
		printk(KERN_DEBUG "== ERR FAST CLK PIN=%d ==\n",
			htc_fast_clk_pin);
		return htc_fast_clk_pin;
	}

	mutex_lock(&htc_fast_clk_mutex);
	if (on) {
		if ((CLK_OFF == htc_fast_clk_state_wifi)
			&& (CLK_OFF == htc_fast_clk_state_bt)) {

			err = set_wifi_bt_fast_clk(CLK_ON);

			if (err) {
				mutex_unlock(&htc_fast_clk_mutex);
				return err;
			}
		}

		if (id == ID_BT)
			htc_fast_clk_state_bt = CLK_ON;
		else
			htc_fast_clk_state_wifi = CLK_ON;
	} else {
		if (((id == ID_BT) && (CLK_OFF == htc_fast_clk_state_wifi))
			|| ((id == ID_WIFI)
			&& (CLK_OFF == htc_fast_clk_state_bt))) {

			/* Only disable the clock and vreg when one of bt/wifi is enable
			 * To avoid unbalanced vreg enable/disable
			 */
			if (CLK_ON == htc_fast_clk_state_bt  || CLK_ON == htc_fast_clk_state_wifi ) {
				err = set_wifi_bt_fast_clk(CLK_OFF);

				if (err) {
					mutex_unlock(&htc_fast_clk_mutex);
					return err;
				}
			}

		} else {
			printk(KERN_DEBUG "KEEP FAST CLK ALIVE\n");
		}

		if (id)
			htc_fast_clk_state_bt = CLK_OFF;
		else
			htc_fast_clk_state_wifi = CLK_OFF;
	}
	mutex_unlock(&htc_fast_clk_mutex);

	printk(KERN_DEBUG "%s ON=%d, ID=%d DONE\n", __func__, on, id);

	return 0;
}


static int htc_fast_clk_probe(struct platform_device *pdev)
{
	int rc = 0;
	struct htc_fast_clk_platform_data *pdata = pdev->dev.platform_data;

	if (pdata == NULL) {
		pr_err("err: null fast clk pdata\n");
		return -EINVAL;
	} else {
		htc_fast_clk_pin = pdata->fast_clk_pin;

		if (pdata->id) {
			printk(KERN_DEBUG "%s id=%s\n", __func__, pdata->id);
			vreg_osc = regulator_get(NULL, pdata->id);
			if (IS_ERR(vreg_osc)) {
				pr_err("%s: regulator_get(%s) failed (%ld) ===\n",
				__func__, pdata->id, PTR_ERR(vreg_osc));
				return PTR_ERR(vreg_osc);
			}

			rc = regulator_set_voltage(vreg_osc,
					2850000, 2850000);
			if (rc) {
				pr_err("unable to set the voltage "
						"for bt/wifi fastclock\n");
				vreg_osc = NULL;
				regulator_put(vreg_osc);
				return rc;
			}
		}

		/* Set initial state */
		htc_fast_clk_state_wifi = CLK_OFF;
		htc_fast_clk_state_bt = CLK_OFF;

		printk(KERN_DEBUG "%s pin=%d\n", __func__, htc_fast_clk_pin);
	}

	return rc;
}

static int htc_fast_clk_remove(struct platform_device *dev)
{
	pr_info("%s", __func__);
	if (vreg_osc && !IS_ERR(vreg_osc)) {
		if (CLK_ON == htc_fast_clk_state_bt  || CLK_ON == htc_fast_clk_state_wifi)
			regulator_disable(vreg_osc);
		regulator_put(vreg_osc);
	}

	return 0;
}

static struct platform_driver htc_fast_clk_driver = {
	.probe = htc_fast_clk_probe,
	.remove = htc_fast_clk_remove,
	.driver = {
		.name = "htc_fast_clk",
		.owner = THIS_MODULE,
	},
};

static int __init htc_fast_clk_init(void)
{
	return platform_driver_register(&htc_fast_clk_driver);
}

static void __exit htc_fast_clk_exit(void)
{
	platform_driver_unregister(&htc_fast_clk_driver);
}

module_init(htc_fast_clk_init);
module_exit(htc_fast_clk_exit);
MODULE_DESCRIPTION("htc wifi/bt fast clock config");
MODULE_AUTHOR("htc_ssdbt<htc_ssdbt@htc.com>");
MODULE_LICENSE("GPL");

