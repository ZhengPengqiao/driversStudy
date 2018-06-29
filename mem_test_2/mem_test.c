/**
 * @brief 虚拟内存驱动
 * 
 * @file mem_test.c
 * @author ZhengPengqiao
 * @date 2018-05-07
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
int mem_major = 0; //主设备号  0：自动分配，  其它：直接注册
int mem_minor = 0; //次设备号

#define MEM_SIZE		(1024*1024*2)
#define MYDEV_NAME		"mem_test"
struct MyDev *mydev;


struct MyDev {
	struct platform_device *pdev;
	struct class *mem_test_class; 
	dev_t devnum; 
	void *vaddr;
	long long int paddr;
	size_t mem_size;
};


/**
 * 打开文件设备时，将会调用此函数
 * 
 * @author zpq (18-6-22)
 * 
 * @param inode 设备文件节点
 * @param f 文件描述符
 * 
 * @return int 0：Ok -1:Err
 */
int mem_test_open (struct inode *inode, struct file *f)
{
	f->private_data = mydev;
	dev_info(&mydev->pdev->dev, "%s\n", __func__);
	return 0;
}


/**
 * 关闭文件设备时，将会调用此函数
 * 
 * @author zpq (18-6-22)
 * 
 * @param inode 设备文件节点
 * @param f 文件描述符
 * 
 * @return int 0：Ok -1:Err 
 */
int mem_test_release(struct inode *inode, struct file *filp)  
{  
	dev_info(&mydev->pdev->dev, "%s\n", __func__);
	return 0;
}


/**
 * 读文件时，将会调用驱动中的这个函数来完成实际工作
 * 
 * @author zpq (18-6-22)
 * 
 * @param filp 文件设备
 * @param buf 用户空间的buf
 * @param count 用户空间buf大小
 * @param f_pos 
 * 
 * @return ssize_t 从驱动中读取的大小
 */
ssize_t mem_test_read(struct file *filp, char __user *buf, size_t count,loff_t *f_pos)  
{  
	int ret = -1;

	dev_info(&mydev->pdev->dev, "%s: copy data to the user space\n", __func__);
    ret = copy_to_user(buf, mydev->vaddr, count);
    if(ret == 0)
    {
		dev_info(&mydev->pdev->dev, "%s : read copy data success.\n", __func__);
        return count;
    }
    else
    {
		dev_info(&mydev->pdev->dev, "%s : read copy data error.\n", __func__);
        return 0;
    }
    return count;
}


/**
 * 写文件时，将会调用驱动中的这个函数来完成实际工作
 * 
 * @author zpq (18-6-22)
 * 
 * @param filp 文件设备
 * @param buf 用户空间的buf
 * @param count 用户空间buf大小
 * @param f_pos 
 * 
 * @return ssize_t 写入的大小
 */
ssize_t mem_test_write(struct file *filp, const char __user *buf, size_t count,loff_t *f_pos)  
{  
	int ret = -1;
	dev_info(&mydev->pdev->dev, "%s : copy data from the user space\n", __func__);

    ret = copy_from_user(mydev->vaddr, buf, count);
    if(ret == 0)
    {
		dev_info(&mydev->pdev->dev, "%s : write copy data success.\n", __func__);
        return count;
    }
    else
    {
		dev_info(&mydev->pdev->dev, "%s : write copy data error.\n", __func__);
        return 0;    
    }
    return count;
}  

/**
 * 文件操作方法集合
 * 
 * @author zpq (18-6-22)
 */
static struct file_operations fops =
{
	.owner = THIS_MODULE,
	.open	=	mem_test_open,
    .read 	=	mem_test_read,  
    .write 	=	mem_test_write,  
    .release =	mem_test_release
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
static int mem_test_probe(struct platform_device *pdev)
{
	int ret = 0;

	/* 分配驱动整体的结构体 */
	mydev = devm_kzalloc(&pdev->dev, sizeof(*mydev), GFP_KERNEL);
	if (!mydev) return -ENOMEM;
	mydev->pdev = pdev;


	mem_major = register_chrdev(mem_major, MYDEV_NAME, &fops);
	if( mem_major < 0 )
	{
		return mem_major;
	}
    mydev->devnum = MKDEV(mem_major, mem_minor);

	mydev->mem_test_class = class_create(THIS_MODULE, MYDEV_NAME);
	if(IS_ERR(mydev->mem_test_class))
	{
		goto ERR_STEP1;
	}
	device_create(mydev->mem_test_class, NULL, mydev->devnum, 0, MYDEV_NAME);
	dev_info(&pdev->dev, "%s  Ok\n", __func__);

	mydev->mem_size = MEM_SIZE;
	mydev->vaddr = NULL;
	mydev->vaddr = kmalloc(mydev->mem_size, GFP_DMA | GFP_KERNEL);
	if( mydev->vaddr == NULL )
	{
		dev_info(&pdev->dev,"kmalloc err, vaddr=%p mem_size=%lu\n", mydev->vaddr, mydev->mem_size);
	}
	else
	{
		mydev->paddr = virt_to_phys(mydev->vaddr);
		dev_info(&pdev->dev,"kmalloc ok, vaddr=%p paddr=%lld mem_size=%lu\n", mydev->vaddr, mydev->paddr, mydev->mem_size);
	}
	return 0;

ERR_STEP1:
	dev_info(&pdev->dev, "%s  ERR_STEP1\n", __func__);
	unregister_chrdev(mem_major, MYDEV_NAME);

	return ret;
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
static int mem_test_remove(struct platform_device *pdev)
{
	if( mydev->vaddr == NULL )
	{
		kfree(mydev->vaddr);

		dev_info(&pdev->dev,"kfree ok\n");
	}

	unregister_chrdev(mem_major, MYDEV_NAME);

    device_destroy(mydev->mem_test_class, mydev->devnum);
    class_destroy(mydev->mem_test_class);

	dev_info(&pdev->dev, "%s Ok \n", __func__);
	return 0;
}

static void mem_test_dev_release(struct device *dev)
{
}

static struct platform_device mem_test_pdev = {
	.name		= MYDEV_NAME,
	.dev.release	= mem_test_dev_release,
};

/**
 * 平台设备结构体
 * 
 * @author zpq (18-5-20)
 */
static struct platform_driver mem_test_pdrv = {
	.probe		=  mem_test_probe,
	.remove		=  mem_test_remove,
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
static void __exit mem_test_exit(void)
{
	printk("run %s\n", __func__);
	platform_driver_unregister(&mem_test_pdrv);
	platform_device_unregister(&mem_test_pdev);

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
static int __init mem_test_init(void)
{
	int ret;
	printk("run %s\n", __func__);
	ret = platform_device_register(&mem_test_pdev);
	if (ret) return ret;

	ret = platform_driver_register(&mem_test_pdrv);
	if (ret) 
	{
		platform_device_unregister(&mem_test_pdev);
	}

	printk("%s OK\n", __func__);
	return 0;
}

/* 模块加载时运行 */
module_init(mem_test_init);
/* 模块卸载时运行 */
module_exit(mem_test_exit);

MODULE_DESCRIPTION("My MEM Test");
MODULE_AUTHOR("ZhengPengqiao, <157510312@qq.com>");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.1.1");
