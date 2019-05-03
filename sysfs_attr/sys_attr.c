/**
 * @brief 虚拟内存驱动
 * 
 * @file sys_attr.c
 * @author ZhengPengqiao
 * @date 2019-06-21
 */

#include <linux/module.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/timer.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/errno.h>
#include <linux/platform_device.h>
#include <linux/uaccess.h>//copy_from_user

static unsigned debug;
module_param(debug, uint, 0644);
MODULE_PARM_DESC(debug, "activates debug info");

/**
 * 内部的全局变量
 * 
 * @author zpq (18-6-21)
 */
#define MYDEV_NAME		"sys_attr"

int num1 = 0;
int num2 = 0;


static ssize_t sys_attr_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "num1=%d num2=%d\n", num1, num2);
}

static ssize_t sys_attr_store(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t count)
{

    pr_info("-->%s\n", buf);
	sscanf(buf, "%x%x" , &num1, &num2);
    pr_info("--> num1=%d num2=%d\n", num1, num2);
	return count;
}

static DEVICE_ATTR(sys_attr, 0664, sys_attr_show, sys_attr_store);

static struct attribute *sys_attr_attributes[] = {
	&dev_attr_sys_attr.attr,
	NULL
};

static const struct attribute_group sys_attr_group = {
	.name     = "sys_attr",
	.attrs = sys_attr_attributes,
};


/**
 * 匹配上平台设备的时候,运行此函数
 * 
 * @author zpq (18-5-20)
 * 
 * @param pdev 平台设备
 * 
 * @return int 0:Ok, -1:Err
 */
static int sys_attr_probe(struct platform_device *pdev)
{
    int ret;
    ret = sysfs_create_group(&pdev->dev.kobj, &sys_attr_group);
    if (ret) {
        return -ret;
    }

	return 0;
}

/**
 * 移除平台设备的时候, 会运行此函数
 * 
 * @author zpq (18-5-20)
 * 
 * @param pdev :平台设备
 * 
 * @return int 0:Ok, -1:Err
 */
static int sys_attr_remove(struct platform_device *pdev)
{
	dev_info(&pdev->dev, "%s Ok \n", __func__);
	return 0;
}

static void sys_attr_dev_release(struct device *dev)
{
}

static struct platform_device sys_attr_pdev = {
	.name		= MYDEV_NAME,
	.dev.release	= sys_attr_dev_release,
};

/**
 * 平台设备结构体
 * 
 * @author zpq (18-5-20)
 */
static struct platform_driver sys_attr_pdrv = {
	.probe		=  sys_attr_probe,
	.remove		=  sys_attr_remove,
	.driver		= {
		.name	= MYDEV_NAME,
	},
};


/**
 * 模块卸载函数 
 * 在卸载模块的时候,内核会运行此函数 
 * 
 * @author zpq (18-5-20)
 * 
 * @param void 
 * 
 * @return void __exit 
 */
static void __exit sys_attr_exit(void)
{
	printk("run %s\n", __func__);
	platform_driver_unregister(&sys_attr_pdrv);
	platform_device_unregister(&sys_attr_pdev);

	printk("%s OK\n", __func__);
}

/**
 * 模块初始化函数 
 * 在模块加载时,内核会运行此函数 
 * 
 * @author zpq (18-5-20)
 * 
 * @return int __init 0:OK -1:Err
 */
static int __init sys_attr_init(void)
{
	int ret;
	printk("run %s\n", __func__);
	ret = platform_device_register(&sys_attr_pdev);
	if (ret) return ret;

	ret = platform_driver_register(&sys_attr_pdrv);
	if (ret) 
	{
		platform_device_unregister(&sys_attr_pdev);
	}

	printk("%s OK\n", __func__);
	return 0;
}

/* 模块加载时运行 */
module_init(sys_attr_init);
/* 模块卸载时运行 */
module_exit(sys_attr_exit);

MODULE_DESCRIPTION("My SYSFS Attr");
MODULE_AUTHOR("ZhengPengqiao, <157510312@qq.com>");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.1.1");
