/**
 * @brief 使用vmalloc虚拟存储的驱动
 * 
 * @file mem_test.c
 * @author your name
 * @date 2018-07-05
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

/**
 * @brief 宏定义
 * 
 */

#define MEM_MALLOC_SIZE 4096 //缓冲区大小


/**
 * @brief 全局变量
 * 
 */
int mem_major = 0; //主设备号  0：自动分配，  其它：直接注册
int mem_minor = 0; //次设备号
int devno;  //记录设备号
char *mem_spvm = NULL; ////缓冲区指针，指向内存区
struct cdev *mem_cdev = NULL; //字符设备对象指针
struct class *mem_class = NULL; //设备类指针


/**
 * @brief 打开文件设备时，将会调用此函数
 * 
 * @param inode 设备文件节点
 * @param filp 文件描述符
 * @return int 0：Ok -1:Err
 */
static int mem_open(struct inode *inode,struct file *filp)
{
    printk("open vmalloc space..\n");
    try_module_get(THIS_MODULE);//模块引用计数器自加
    printk("open vamlloc space ok..\n");
    return 0;
}


/**
 * @brief 关闭文件设备时，将会调用此函数
 * 
 * @param inode 设备文件节点
 * @param filp 文件描述符
 * @return int 0：Ok -1:Err 
 */
static int mem_release(struct inode *inode, struct file *filp)
{
    printk("close vmalloc space..\n");
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
static ssize_t mem_read(struct file *filp,char __user *buf,size_t count,loff_t *fpos)
{
    int ret = -1;
    char *tmp;
    printk("copy data to the user space\n");
    tmp = mem_spvm;
    if(count > MEM_MALLOC_SIZE)
        count = MEM_MALLOC_SIZE;
    if(tmp != NULL)//将内核数据写入到用户空间
        ret = copy_to_user(buf,tmp,count);
    if(ret == 0)
    {
        printk("read copy data success\n");
        return count;
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
ssize_t mem_write(struct file *filp, const char __user *buf, size_t count,loff_t *f_pos)
{
    int ret = -1;
    char *tmp;
    printk("read data from the user space.\n");
    tmp = mem_spvm;
    if(count > MEM_MALLOC_SIZE)
        count = MEM_MALLOC_SIZE;
    if(tmp != NULL)
        ret = copy_from_user(tmp,buf,count);
    if(ret == 0)
    {
        printk("write copy data success.\n");
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
static const struct file_operations mem_fops={
    .owner = THIS_MODULE,
    .open = mem_open,
    .release = mem_release,
    .read = mem_read,
    .write = mem_write,
};


/**
 * @brief 模块初始化函数
 * 在模块加载时,内核会运行此函数 
 * 
 * @return int hello_chdev_init  0:Ok, -1:Err
 */
static int __init mem_init(void)
{
    int ret = 0;
    printk("mem_init initial...\n");

    //开辟内核内存缓冲区
    mem_spvm = (char *)vmalloc(MEM_MALLOC_SIZE);
    if(mem_spvm == NULL)
    {
        printk("vmalloc mem_spvm error\n");
        return -ENOMEM;//
    }
    
    mem_major = register_chrdev(mem_major, "mem_test", &mem_fops);
	if( mem_major < 0 )
	{
		return ret;
	}

    devno = MKDEV(mem_major, mem_minor);

    mem_class = class_create(THIS_MODULE,"mem_test");
    if(IS_ERR(mem_class))
    {
        printk("class_create error..\n");
        return -1;
    }
    device_create(mem_class,NULL, devno, NULL,"mem_test");
    
    printk("init finished..\n");
    return 0;
}


/**
 * @brief 模块卸载函数
 * 在卸载模块的时候,内核会运行此函数 
 * 
 */
static void __exit mem_exit(void)
{
    printk("mem_exit starting..\n");
	unregister_chrdev(mem_major, "mem_test");
    printk("cdev_del ok\n");

    device_destroy(mem_class, devno);
    class_destroy(mem_class);

    if(mem_spvm != NULL)
        vfree(mem_spvm);

    printk("vfree ok\n");
    printk("mem_exit finished..\n");
}


/* 模块加载时运行 */
module_init(mem_init);
/* 模块卸载时运行 */
module_exit(mem_exit);
MODULE_LICENSE("GPL");
