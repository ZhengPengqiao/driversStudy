/**
 * @brief 虚拟摄像头驱动
 * 
 * @file myvivi.c
 * @author ZhengPengqiao
 * @date 2018-05-07
 */

#include <linux/module.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/ioport.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/pci.h>
#include <linux/random.h>
#include <linux/version.h>
#include <linux/mutex.h>
#include <linux/videodev2.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/kthread.h>
#include <linux/highmem.h>
#include <linux/freezer.h>
#include <media/videobuf-vmalloc.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>


/**
 * @brief 驱动模块加载函数
 * 注册Video设备
 * 
 * @return 0:Ok  其他:Err 
 */
static int __init myvivi_init(void)
{
    int err = 0;

    printk("myvivi module is init Ok !\n");
    return err;
}

static void __exit myvivi_uninit(void)
{
    printk("myvivi module is uninit Ok !\n");
}

module_init(myvivi_init);
module_exit(myvivi_uninit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ZhengPengqiao");


