#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/gpio.h>
#include <mach/mfp.h>
#include <linux/interrupt.h>
#include <linux/irq.h>

#define GPIO_VIBRATOR	9

#ifdef 	CONFIG_SYSFS 

#define	VIBON 	 -1
#define	VIBOFF 	 0

struct device *vibrator_dev;
unsigned long vib_gpio;

#define BACKLIGHT_GPIO 103
unsigned long bl_gpio;

#include <linux/delay.h>

static unsigned spi_cs1_gpio;
static inline void vibrator_set_status(long value)
{
	if (value == VIBOFF) {
		//gpio_set_value(vib_gpio, 0);
		pr_info("clear spi_cs1");
		gpio_set_value(spi_cs1_gpio, 0);
	}
	else if (value == VIBON) {
		//gpio_set_value(vib_gpio, 1);
		pr_info("set spi_cs1");
		gpio_set_value(spi_cs1_gpio, 1);
	}
	else 
	      printk("Error getting data");
}

static ssize_t vibrator_status_store(struct device *dev,
			struct device_attribute *attr, 
			const char *buf, size_t size)
{
	int value;
	sscanf(buf, "%d", &value);
	vibrator_set_status(value);
	return size;
}

static DEVICE_ATTR(enable, 0644, NULL, vibrator_status_store);

struct class output_class = {
	.name		= "timed_output",
};

#if 0
//static unsigned long vib_gpio_cfg = MFP_CFG_X(SD1_CLK, GPIO, DS0, PULL_NONE, IO_NONE);

//static unsigned long bl_gpio_cfg	= MFP_CFG_X(LCD_EN, GPIO, DS0, PULL_NONE, IO_OE);



static int bl_gpio_test(void)
{
	int err;
	
	sprd_mfp_config(&bl_gpio_cfg, 1);
	bl_gpio = mfp_to_gpio(MFP_CFG_TO_PIN(bl_gpio_cfg));
	pr_info("backlight gpio is:%d\r\n", bl_gpio);
	
	err = gpio_request(bl_gpio, "backlight");
	if (err) {
		pr_warning("cannot alloc gpio for backlight\r\n");
		return err;
	}
	gpio_direction_output(bl_gpio, 0);
	return 0;
}
#endif
static unsigned long spi_cs1_gpio_cfg = MFP_CFG_X(SPI_CSN0, GPIO, DS1, F_PULL_NONE, S_PULL_DOWN, IO_NONE);
static int spics1_gpio_test(void)
{
	int err;
	
	sprd_mfp_config(&spi_cs1_gpio_cfg, 1);
	
	spi_cs1_gpio = 32;
	pr_info("spi_cs1_gpio gpio is:%d\r\n", spi_cs1_gpio);
	
	err = gpio_request(spi_cs1_gpio, "spi_cs1");
	if (err) {
		pr_warning("cannot alloc gpio for spi cs1\r\n");
		return err;
	}
	gpio_direction_output(spi_cs1_gpio, 0);
	return 0;
}

#if 0
static unsigned long hs_gpio_cfg = MFP_CFG_X(CLK_LCD, GPIO, DS1, PULL_NONE, IO_IE);

#define IRQ_HEADSET 32

static irqreturn_t
headset_handler(int irq, void *dev)
{
//	sprd_ack_gpio_irq(irq);
//	sprd_mask_gpio_irq(irq);
	
	disable_irq_nosync(irq);
        msleep(1000);
	enable_irq(irq);
	pr_info("headset irq \n");
	return IRQ_HANDLED;
}

static int headset_gpio_int_test(void)
{
	int hs_gpio;
	int err;
	
	sprd_mfp_config(&hs_gpio_cfg, 1);
	hs_gpio = 115;//mfp_to_gpio(MFP_CFG_TO_PIN(bl_gpio_cfg));
	pr_info("headset gpio is:%d\r\n", hs_gpio);
	
	err = gpio_request(hs_gpio, "backlight");
	if (err) {
		pr_warning("cannot alloc gpio for backlight\r\n");
		return err;
	}
	gpio_direction_input(hs_gpio);

	sprd_gpio_irq_register(hs_gpio, IRQ_HEADSET);

	err = request_threaded_irq(IRQ_HEADSET,NULL, headset_handler, IRQF_TRIGGER_LOW
		 | IRQF_ONESHOT, "headset irq", NULL);
		//| IRQF_NOAUTOEN | IRQF_ONESHOT, "headset irq", NULL);
	if (err) {
		pr_warning("cannot alloc irq for headset, err %d\r\n", err);
		return err;
	}
	#if 0
	msleep(5000);
	pr_info("re-enable headset irq again\n");
	enable_irq(IRQ_HEADSET);
	#endif
	return 0;
}

#endif
static int creat_vibrator_sysfs_file(void)
{
	int err;
	
	
	err = class_register(&output_class);
	if (err) 
	{
		printk(KERN_ERR "timed_output: unable to register timed_output class\n");
		return err;
	}

	vibrator_dev = device_create(&output_class, NULL, 0, NULL, "%s", "vibrator");
	err = device_create_file(vibrator_dev, &dev_attr_enable);
	if (err) {
		device_unregister(vibrator_dev);
		return err;
	}
#if 0
	sprd_mfp_config(&vib_gpio_cfg, 1);
	vib_gpio = mfp_to_gpio(MFP_CFG_TO_PIN(vib_gpio_cfg));
	pr_info("vibrator gpio is:%d\r\n", vib_gpio);
	
	err = gpio_request(vib_gpio, "vibrator");
	if (err) {
		pr_warning("cannot alloc gpio for vibrator\r\n");
		return err;
	}
	gpio_direction_output(vib_gpio, 0);
	bl_gpio_test();
	headset_gpio_int_test();
#endif
	spics1_gpio_test();
	return 0;
}

static void remove_vibrator_sysfs_file(void)
{
	device_remove_file(vibrator_dev, &dev_attr_enable);
	device_unregister(vibrator_dev);
	class_unregister(&output_class);
}

#endif

static int __init vibrator_sprd_init(void)
{
	return creat_vibrator_sysfs_file();
}

static void __exit vibrator_sprd_exit(void)
{
	remove_vibrator_sysfs_file();
}

module_init(vibrator_sprd_init);
module_exit(vibrator_sprd_exit);