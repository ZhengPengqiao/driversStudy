/**
 * @brief 创建字符设备
 * 
 * @file hello_chdev.c
 * @author your name
 * @date 2018-07-05
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/ctype.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/spinlock.h>
#include <linux/workqueue.h>
#include <linux/of_gpio.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/pagemap.h>
#include <linux/vmalloc.h>
#include <linux/dma-mapping.h>
#include <linux/i2c.h>
#include <linux/fsl_devices.h>
#include <linux/platform_device.h>

#define IMX_GPIO_NR(bank, nr) 	(((bank) - 1) * 32 + (nr))

static int GPIO_7 = IMX_GPIO_NR(1, 7);
static int EIM_A16 = IMX_GPIO_NR(2, 22);


/**
 * 全局变量
 * 
 * @author zpq (18-7-5)
 */
//起始次设备号
#define FIRSTMINOR  0
//需要申请设备号的个数
#define DEVICE_NUM  1

dev_t devno;  /*< 记录设备号 */
struct cdev hello_chdev_cdev[DEVICE_NUM];   /*< 字符设备对象 */
struct class *hello_chdev_class = NULL; /*< 设备类指针 */
static struct device *classdev;

/**
 * 宏定义
 */
#define MYDEV_NAME "hello_gpio"


/**
 * @brief 打开文件设备时，将会调用此函数
 * 
 * @param inode 设备文件节点
 * @param filp 文件描述符
 * @return int 0：Ok -1:Err
 */
static int hello_chdev_open(struct inode *inode,struct file *filp)
{
    printk("%s ok..\n", __func__);
    try_module_get(THIS_MODULE);//模块引用计数器自加
    return 0;
}


/**
 * @brief 关闭文件设备时，将会调用此函数
 * 
 * @param inode 设备文件节点
 * @param filp 文件描述符
 * @return int 0：Ok -1:Err 
 */
static int hello_chdev_release(struct inode *inode, struct file *filp)
{
    printk("%s ok..\n", __func__);
    module_put(THIS_MODULE);//模块引用计数器自减
    return 0;
}


/**
 * @brief 读文件时，将会调用驱动中的这个函数来完成实际工作
 * 
 * @param filp 文件设备
 * @param __user 用户空间的buf
 * @param count 用户空间buf大小
 * @param fpos 
 * @return ssize_t 从驱动中读取的大小
 */
static ssize_t hello_chdev_read(struct file *filp,char __user *buf,size_t count,loff_t *fpos)
{
    int ret = -1;
    int len = 0;
    char val[2];
    printk("%s ok..\n", __func__);

    val[0]=gpio_get_value(GPIO_7);
    val[1]=gpio_get_value(EIM_A16);

    if( count > sizeof(val) )
    {
        len = sizeof(val);
    }
    else
    {
        len = count;
    }

    /* 将内核空间数据拷贝到用户空间 */
    ret = copy_to_user(buf,val,len);
    if(ret == 0)
    {
        printk("to user val[0]=%d val[1]=%d\n", val[0], val[1]);
        return len;
    }
    else
    {
        printk("read copy data error\n");
        return 0;
    }
}


/**
 * @brief 写文件时，将会调用驱动中的这个函数来完成实际工作
 * 
 * @param filp 文件设备
 * @param __user 用户空间的buf
 * @param count 用户空间buf大小
 * @param fops 
 * @return ssize_t 写入的大小
 */
static ssize_t hello_chdev_write(struct file *filp, const char __user *buf, size_t count, loff_t *fops)
{
    int len;
    int ret = -1;
    char val[2];
    printk("%s ok..\n", __func__);
    if( count > sizeof(val) )
    {
        len = sizeof(val);
    }
    else
    {
        len = count;
    }
    

    /* 将用户空间数据拷贝到内核空间 */
    ret = copy_from_user(val,buf,len);
    if(ret == 0)
    {
        printk("form user val[0]=%d val[1]=%d\n", val[0], val[1]);
		gpio_set_value(GPIO_7, val[0]);
		gpio_set_value(EIM_A16, val[1]);
        return count;
    }
    else
    {
        printk("write copy data error.\n");
        return 0;    
    }
}


/**
 * @brief 文件操作方法集合
 * 
 */
static const struct file_operations hello_chdev_fops={
    .owner = THIS_MODULE,
    .open = hello_chdev_open,
    .release = hello_chdev_release,
    .read = hello_chdev_read,
    .write = hello_chdev_write,
};


/**
 * @brief 模块初始化函数
 * 在模块加载时,内核会运行此函数 
 * 
 * @return int hello_chdev_init  0:Ok, -1:Err
 */
static int __init hello_chdev_init(void)
{
    int ret;
    int i = 0;
    int j = 0;
    int devtemp;
    printk("in %s ..\n", __func__);

    ret = alloc_chrdev_region(&devno, FIRSTMINOR, DEVICE_NUM , MYDEV_NAME);
	if(ret < 0)
	{
		printk("alloc_chrdev_region fail \n");
		return -1;
	}

	printk("hello_chdev_init devno=%d major=%d \n", devno, MAJOR(devno));

    hello_chdev_class = class_create(THIS_MODULE, MYDEV_NAME);
    if(IS_ERR(hello_chdev_class))
    {
        printk("class_create error..\n");
        goto TAGERR1;
    }

    for (i = 0; i < DEVICE_NUM; i++)
	{

        devtemp = MKDEV(MAJOR(devno), i);

        printk("dev=%d devtemp=%d %s%d\n", i, devtemp, MYDEV_NAME, i);
        classdev = device_create(hello_chdev_class, NULL, devtemp, NULL, MYDEV_NAME"%d", i);
        if (IS_ERR(hello_chdev_class))
        {
            ret = PTR_ERR(classdev);
            printk(KERN_ERR "device create error %d\n", ret);
		    goto TAGERR3;
        }

        cdev_init(&hello_chdev_cdev[i], &hello_chdev_fops);
        ret = cdev_add(&hello_chdev_cdev[i], devtemp, 1);
        if (ret)
        {
            printk("cdev_add fail \n");
		    goto TAGERR2;
        }
    }

    // TODO: 引脚工作的前提条件：请确认dts中配置引脚为GPIO功能，并确保引脚没有分配给别的模块使用
	if(gpio_is_valid(EIM_A16)) {
		pr_info("EIM_A16 is valid num=%d Ok\n", EIM_A16);

		gpio_request(EIM_A16, "EIM_A16");
		gpio_direction_output(EIM_A16, 1);
	}
	else
	{
		pr_info("EIM_A16 is invalid num=%d\n", EIM_A16 );
	}
    
	if(gpio_is_valid(GPIO_7)) {
		pr_info("GPIO_7 is valid num=%d Ok\n", GPIO_7);

		gpio_request(GPIO_7, "GPIO_7");
		gpio_direction_output(GPIO_7, 0);
	}
	else
	{
		pr_info("GPIO_7 is invalid num=%d\n", GPIO_7 );
	}

    printk("%s ok..\n", __func__);
    return 0;

TAGERR3:
    cdev_del(&hello_chdev_cdev[i]);
TAGERR2:
    class_destroy(hello_chdev_class);
TAGERR1:
    for (j = 0; j < i; j++)
	{
		devtemp = MKDEV(MAJOR(devno), j);
		device_destroy(hello_chdev_class, devtemp);
		cdev_del(&hello_chdev_cdev[j]);
    }
	unregister_chrdev_region(devno, DEVICE_NUM);

    return ret;
}


/**
 * @brief 模块卸载函数
 * 在卸载模块的时候,内核会运行此函数 
 * 
 */
static void __exit hello_chdev_exit(void)
{
    int devtemp;
    int i = 0;
    printk("in %s ..\n", __func__);
    printk("hello_exit \n");
	
	for (i = 0; i < DEVICE_NUM; i++)
	{
		devtemp = MKDEV(MAJOR(devno), i);
		device_destroy(hello_chdev_class, devtemp);
		cdev_del(&hello_chdev_cdev[i]);
    }

    class_destroy(hello_chdev_class);
    unregister_chrdev_region(devno, DEVICE_NUM);
    printk("in %s ok..\n", __func__);
}



/* 模块加载时运行 */
module_init(hello_chdev_init);
/* 模块卸载时运行 */
module_exit(hello_chdev_exit);

MODULE_LICENSE("GPL");