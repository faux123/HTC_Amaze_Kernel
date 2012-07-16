/* linux/arch/arm/mach-msm/board-ruby-wifi.c
*/
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <asm/mach-types.h>
#include <asm/gpio.h>
#include <asm/io.h>
#include <linux/skbuff.h>
#include <linux/wifi_tiwlan.h>

#include "board-ruby.h"

// HTC_WIFI_START
#include <linux/wl12xx.h>

static struct wl12xx_platform_data ruby_wlan_data __initdata = {
       .irq = MSM_GPIO_TO_INT(RUBY_GPIO_WIFI_IRQ),
       .board_ref_clock = WL12XX_REFCLOCK_26,
       .board_tcxo_clock = 1,
};
// HTC_WIFI_END

extern int ti_wifi_power(int on);
extern int ruby_wifi_reset(int on);
/*extern int ruby_wifi_set_carddetect(int on);*/
extern int ruby_wifi_get_mac_addr(unsigned char *buf);

#define PREALLOC_WLAN_NUMBER_OF_SECTIONS	4
#define PREALLOC_WLAN_NUMBER_OF_BUFFERS		160
#define PREALLOC_WLAN_SECTION_HEADER		24

#define WLAN_SECTION_SIZE_0	(PREALLOC_WLAN_NUMBER_OF_BUFFERS * 128)
#define WLAN_SECTION_SIZE_1	(PREALLOC_WLAN_NUMBER_OF_BUFFERS * 128)
#define WLAN_SECTION_SIZE_2	(PREALLOC_WLAN_NUMBER_OF_BUFFERS * 512)
#define WLAN_SECTION_SIZE_3	(PREALLOC_WLAN_NUMBER_OF_BUFFERS * 1024)

#define WLAN_SKB_BUF_NUM	16

static struct sk_buff *wlan_static_skb[WLAN_SKB_BUF_NUM];

typedef struct wifi_mem_prealloc_struct {
	void *mem_ptr;
	unsigned long size;
} wifi_mem_prealloc_t;

static wifi_mem_prealloc_t wifi_mem_array[PREALLOC_WLAN_NUMBER_OF_SECTIONS] = {
	{ NULL, (WLAN_SECTION_SIZE_0 + PREALLOC_WLAN_SECTION_HEADER) },
	{ NULL, (WLAN_SECTION_SIZE_1 + PREALLOC_WLAN_SECTION_HEADER) },
	{ NULL, (WLAN_SECTION_SIZE_2 + PREALLOC_WLAN_SECTION_HEADER) },
	{ NULL, (WLAN_SECTION_SIZE_3 + PREALLOC_WLAN_SECTION_HEADER) }
};

static void *ruby_wifi_mem_prealloc(int section, unsigned long size)
{
	if (section == PREALLOC_WLAN_NUMBER_OF_SECTIONS)
		return wlan_static_skb;
	if ((section < 0) || (section > PREALLOC_WLAN_NUMBER_OF_SECTIONS))
		return NULL;
	if (wifi_mem_array[section].size < size)
		return NULL;
	return wifi_mem_array[section].mem_ptr;
}

int __init ruby_init_wifi_mem(void)
{
	int i;

	for (i = 0; (i < WLAN_SKB_BUF_NUM); i++) {
		if (i < (WLAN_SKB_BUF_NUM/2))
			wlan_static_skb[i] = dev_alloc_skb(4096);
		else
			wlan_static_skb[i] = dev_alloc_skb(8192);
	}
	for (i = 0; (i < PREALLOC_WLAN_NUMBER_OF_SECTIONS); i++) {
		wifi_mem_array[i].mem_ptr = kmalloc(wifi_mem_array[i].size,
							GFP_KERNEL);
		if (wifi_mem_array[i].mem_ptr == NULL)
			return -ENOMEM;
	}
	return 0;
}

static struct resource ruby_wifi_resources[] = {
	[0] = {
		.name		= "device_wifi_irq",
		.start		= MSM_GPIO_TO_INT(RUBY_GPIO_WIFI_IRQ),
		.end		= MSM_GPIO_TO_INT(RUBY_GPIO_WIFI_IRQ),
		.flags      = IORESOURCE_IRQ | IORESOURCE_IRQ_LOWEDGE,
	},
};

static struct wifi_platform_data ruby_wifi_control = {
	.set_power      = ti_wifi_power,
	.set_reset      = ruby_wifi_reset,
#ifdef CONFIG_TIWLAN_SDIO
	.set_carddetect = ruby_wifi_set_carddetect,
#endif
	.mem_prealloc   = ruby_wifi_mem_prealloc,
};

static struct platform_device ruby_wifi_device = {
	.name           = "device_wifi",
	.id             = 1,
	.num_resources  = ARRAY_SIZE(ruby_wifi_resources),
	.resource       = ruby_wifi_resources,
	.dev            = {
		.platform_data = &ruby_wifi_control,
	},
};

int __init ruby_wifi_init(void)
{
	int ret;

	printk(KERN_INFO "%s: start\n", __func__);
	ruby_init_wifi_mem();
	ret = platform_device_register(&ruby_wifi_device);
// HTC_WIFI_START
       if(wl12xx_set_platform_data(&ruby_wlan_data))
               pr_err("Error setting wl12xx_data\n");
// HTC_WIFI_END

	return ret;
}

