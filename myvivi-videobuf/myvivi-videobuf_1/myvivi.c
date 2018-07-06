/**
 * @brief 虚拟摄像头驱动
 * 
 * @file myvivi.c
 * @author ZhengPengqiao
 * @date 2018-05-07
 */

#include <linux/module.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/timer.h>
#include <linux/sched.h>
#include <linux/slab.h>

#include <linux/platform_device.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-event.h>
#include <media/videobuf-vmalloc.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>

static unsigned debug;
module_param(debug, uint, 0644);
MODULE_PARM_DESC(debug, "activates debug info");


#define MYVIVI_NAME		"myvivi"
struct myvivi_dev *mydev;
static struct v4l2_format myvivi_format;
static struct videobuf_queue myvivi_vb_vidqueue;
static struct list_head myvivi_vb_local_queue;

struct myvivi_dev {
	struct v4l2_device	v4l2_dev;
	struct video_device	vfd;

	atomic_t		num_inst;
	struct mutex		dev_mutex;
	spinlock_t		irqlock;

	struct timer_list	timer;
};

#include "fillbuf.h"

/* ------------------------------------------------------------------
	Videobuf operations
   ------------------------------------------------------------------*/
/* APP调用ioctl VIDIOC_REQBUFS时会导致此函数被调用,
 * 它重新调整count和size
 */
static int myvivi_buffer_setup(struct videobuf_queue *vq, unsigned int *count, unsigned int *size)
{

	*size = myvivi_format.fmt.pix.sizeimage;

	if (0 == *count)
		*count = 32;

	return 0;
}

/* APP调用ioctlVIDIOC_QBUF时导致此函数被调用,
 * 它会填充video_buffer结构体并调用videobuf_iolock来分配内存
 * 
 */
static int myvivi_buffer_prepare(struct videobuf_queue *vq, struct videobuf_buffer *vb,
						enum v4l2_field field)
{
	/* 0. 设置videobuf */
	vb->size = myvivi_format.fmt.pix.sizeimage;
    vb->bytesperline = myvivi_format.fmt.pix.bytesperline;
	vb->width  = myvivi_format.fmt.pix.width;
	vb->height = myvivi_format.fmt.pix.height;
	vb->field  = field;
    
    /* 1. 做些准备工作 */
    myvivi_precalculate_bars(0, myvivi_format);
#if 0
    /* 2. 调用videobuf_iolock为类型为V4L2_MEMORY_USERPTR的videobuf分配内存 */
	if (VIDEOBUF_NEEDS_INIT == buf->vb.state) {
		rc = videobuf_iolock(vq, &buf->vb, NULL);
		if (rc < 0)
			goto fail;
	}
#endif
    /* 3. 设置状态 */
	vb->state = VIDEOBUF_PREPARED;

	return 0;
}



/* APP调用ioctl VIDIOC_QBUF时:
 * 1. 先调用buf_prepare进行一些准备工作
 * 2. 把buf放入stream队列
 * 3. 调用buf_queue(起通知、记录作用)
 */
static void myvivi_buffer_queue(struct videobuf_queue *vq, struct videobuf_buffer *vb)
{
	vb->state = VIDEOBUF_QUEUED;

    /* 把videobuf放入本地一个队列尾部
     * 定时器处理函数就可以从本地队列取出videobuf
     */
    list_add_tail(&vb->queue, &myvivi_vb_local_queue);
}


/* APP不再使用队列时, 用它来释放内存 */
static void myvivi_buffer_release(struct videobuf_queue *vq,
			   struct videobuf_buffer *vb)
{
	videobuf_vmalloc_free(vb);
	vb->state = VIDEOBUF_NEEDS_INIT;
}

static struct videobuf_queue_ops myvivi_video_qops = {
	.buf_setup      = myvivi_buffer_setup, /* 计算大小以免浪费 */
	.buf_prepare    = myvivi_buffer_prepare,
	.buf_queue      = myvivi_buffer_queue,
	.buf_release    = myvivi_buffer_release,
};


static int myvivi_open(struct file *file)
{
	    /* 队列操作2: 初始化 */
	videobuf_queue_vmalloc_init(&myvivi_vb_vidqueue, &myvivi_video_qops,
			NULL, &mydev->irqlock, V4L2_BUF_TYPE_VIDEO_CAPTURE, V4L2_FIELD_INTERLACED,
			sizeof(struct videobuf_buffer), NULL, NULL); /* 倒数第2个参数是buffer的头部大小 */

    mydev->timer.expires = jiffies + 1;
    add_timer(&mydev->timer);
	return 0;
}


static int myvivi_release(struct file *file)
{

    del_timer(&mydev->timer);
	videobuf_stop(&myvivi_vb_vidqueue);
	videobuf_mmap_free(&myvivi_vb_vidqueue);

	return 0;
}


static int myvivi_mmap(struct file *file, struct vm_area_struct *vma)
{
	return videobuf_mmap_mapper(&myvivi_vb_vidqueue, vma);
}


/**
 * 查询video设备的功能
 * 
 * @author zpq (18-5-20)
 * 
 * @param file video文件结构
 * @param priv 私有成员
 * @param cap video设备的功能
 * 
 * @return int 0:Ok -1:Err
 */
static int vidioc_querycap(struct file *file, void *priv,
			   struct v4l2_capability *cap)
{
	strncpy(cap->driver, MYVIVI_NAME, sizeof(cap->driver) - 1);
	strncpy(cap->card, MYVIVI_NAME, sizeof(cap->card) - 1);
	snprintf(cap->bus_info, sizeof(cap->bus_info), "platform:%s", MYVIVI_NAME);
	cap->device_caps = V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_STREAMING;
	cap->capabilities = cap->device_caps | V4L2_CAP_DEVICE_CAPS;
	return 0;
}

static int myvivi_vidioc_enum_fmt_vid_cap(struct file *file, void *priv,
				   struct v4l2_fmtdesc *f)
{

	if (f->index >= 1)
		return -EINVAL;

	strcpy(f->description, "4:2:2, packed, YUYV");
	f->pixelformat = V4L2_PIX_FMT_YUYV;
	return 0;
}

static int myvivi_vidioc_g_fmt_vid_cap(struct file *file, void *priv,
				struct v4l2_format *f)
{
    memcpy(f, &myvivi_format, sizeof(myvivi_format));
	return 0;
}


static int myvivi_vidioc_try_fmt_vid_cap(struct file *file, void *priv,
				  struct v4l2_format *f)
{
	unsigned int maxw, maxh;
	enum v4l2_field field;

	if (f->fmt.pix.pixelformat != V4L2_PIX_FMT_YUYV)
		return -EINVAL;

	field = f->fmt.pix.field;

	if (field == V4L2_FIELD_ANY) 
	{
		field = V4L2_FIELD_INTERLACED;
	}
	else if (V4L2_FIELD_INTERLACED != field) 
	{
		return -EINVAL;
	}

	maxw  = 1024;
	maxh  = 768;

	/**
	 * 调整format的width, height, 
	 * 计算bytesperline, sizeimage 
	 */
	v4l_bound_align_image(&f->fmt.pix.width, 48, maxw, 2, &f->fmt.pix.height, 32, maxh, 0, 0);
	f->fmt.pix.bytesperline = (f->fmt.pix.width * 16) >> 3;
	f->fmt.pix.sizeimage = f->fmt.pix.height * f->fmt.pix.bytesperline;

	return 0;
}


static int myvivi_vidioc_s_fmt_vid_cap(struct file *file, void *priv,
				struct v4l2_format *f)
{
	int ret = myvivi_vidioc_try_fmt_vid_cap(file, NULL, f);
	if (ret < 0)
		return ret;

	memcpy(&myvivi_format, f, sizeof(myvivi_format));

	return ret;
}


int myvivi_vidioc_g_input(struct file *file, void *fh, unsigned int *i)
{

	return 0;
}


int myvivi_vidioc_s_input(struct file *file, void *fh, unsigned int i)
{

	return 0;
}


static int myvivi_vidioc_reqbufs(struct file *file, void *priv,
			  struct v4l2_requestbuffers *p)
{
	return (videobuf_reqbufs(&myvivi_vb_vidqueue, p));
}

static int myvivi_vidioc_querybuf(struct file *file, void *priv, struct v4l2_buffer *p)
{
	return (videobuf_querybuf(&myvivi_vb_vidqueue, p));
}

static int myvivi_vidioc_qbuf(struct file *file, void *priv, struct v4l2_buffer *p)
{
	return (videobuf_qbuf(&myvivi_vb_vidqueue, p));
}

static int myvivi_vidioc_dqbuf(struct file *file, void *priv, struct v4l2_buffer *p)
{
	return (videobuf_dqbuf(&myvivi_vb_vidqueue, p, file->f_flags & O_NONBLOCK));
}

static int myvivi_vidioc_streamon(struct file *file, void *priv, enum v4l2_buf_type i)
{
	return videobuf_streamon(&myvivi_vb_vidqueue);
}

static int myvivi_vidioc_streamoff(struct file *file, void *priv, enum v4l2_buf_type i)
{
	videobuf_streamoff(&myvivi_vb_vidqueue);
    return 0;
}

/**
 * v4l2文件的ioctl函数
 * 
 * @author zpq (18-5-20)
 */
static const struct v4l2_ioctl_ops myvivi_ioctl_ops = {
	.vidioc_querycap	= vidioc_querycap,

	/* 用于列举、获得、测试、设置摄像头的数据的格式 */ 
	.vidioc_enum_fmt_vid_cap  = myvivi_vidioc_enum_fmt_vid_cap,
	.vidioc_g_fmt_vid_cap     = myvivi_vidioc_g_fmt_vid_cap,
	.vidioc_try_fmt_vid_cap   = myvivi_vidioc_try_fmt_vid_cap,
	.vidioc_s_fmt_vid_cap     = myvivi_vidioc_s_fmt_vid_cap,
	.vidioc_g_input     = myvivi_vidioc_g_input,
	.vidioc_s_input     = myvivi_vidioc_s_input,

	/* 缓冲区操作: 申请/查询/放入队列/取出队列 */
	.vidioc_reqbufs       = myvivi_vidioc_reqbufs,
	.vidioc_querybuf      = myvivi_vidioc_querybuf,
	.vidioc_qbuf          = myvivi_vidioc_qbuf,
	.vidioc_dqbuf         = myvivi_vidioc_dqbuf,

	// 启动/停止
	.vidioc_streamon      = myvivi_vidioc_streamon,
	.vidioc_streamoff     = myvivi_vidioc_streamoff,
};

/**
 * v4l2文件的操作函数
 * 
 * @author zpq (18-5-20)
 */
static const struct v4l2_file_operations myvivi_fops = {
	.owner		= THIS_MODULE,
	.open		= myvivi_open,
	.release	= myvivi_release,
    .mmap       = myvivi_mmap,
	.unlocked_ioctl	= video_ioctl2,
};


/**
 * video设备
 * 
 * @author zpq (18-5-20)
 */
static struct video_device myvivi_videodev = {
	.name		= MYVIVI_NAME,
	.fops		= &myvivi_fops,
	.ioctl_ops	= &myvivi_ioctl_ops,
	.minor		= -1,
	.release	= video_device_release_empty,
};



static void myvivi_timer_function(unsigned long data)
{
    struct videobuf_buffer *vb;
	void *vbuf;
	struct timeval ts;
    
    /* 1. 构造数据: 从队列头部取出第1个videobuf, 填充数据
     */

    /* 1.1 从本地队列取出第1个videobuf */
    if (list_empty(&myvivi_vb_local_queue)) {
        goto out;
    }
    
    vb = list_entry(myvivi_vb_local_queue.next,
             struct videobuf_buffer, queue);
    
    /* Nobody is waiting on this buffer, return */
    if (!waitqueue_active(&vb->done))
        goto out;
    


	/* 1.2 填充数据 */
	vbuf = videobuf_to_vmalloc(vb);
	//memset(vbuf, 0xff, vb->size);
	myvivi_fillbuff(vb);

	vb->field_count++;
    do_gettimeofday(&ts);
    vb->ts = ts;
    vb->state = VIDEOBUF_DONE;

    /* 1.3 把videobuf从本地队列中删除 */
    list_del(&vb->queue);

    /* 2. 唤醒进程: 唤醒videobuf->done上的进程 */
    wake_up(&vb->done);
    
out:
    /* 3. 修改timer的超时时间 : 30fps, 1秒里有30帧数据
     *    每1/30 秒产生一帧数据
     */
	mod_timer(&mydev->timer, jiffies + HZ / 30);
}


/**
 * 匹配上平台设备的时候,运行此函数
 * 
 * @author zpq (18-5-20)
 * 
 * @param pdev 平台设备
 * 
 * @return int 0:Ok, -1:Err
 */
static int myvivi_probe(struct platform_device *pdev)
{
	struct video_device *vfd;
	int ret;

	v4l2_info(pdev, "%s \n", __func__);

	/* 分配驱动整体的结构体 */
	mydev = devm_kzalloc(&pdev->dev, sizeof(*mydev), GFP_KERNEL);
	if (!mydev) return -ENOMEM;

    /* 定义/初始化一个队列(会用到一个spinlock) */
	spin_lock_init(&mydev->irqlock);

	/* 注册v4l2设备 */
	ret = v4l2_device_register(&pdev->dev, &mydev->v4l2_dev);
	if (ret) return ret;

	atomic_set(&mydev->num_inst, 0);
	mutex_init(&mydev->dev_mutex);

	/* 分配一个video_device结构体 */
	mydev->vfd = myvivi_videodev;
	vfd = &mydev->vfd;
	vfd->lock = &mydev->dev_mutex;
	vfd->v4l2_dev = &mydev->v4l2_dev;

	/* 注册 */
	ret = video_register_device(vfd, VFL_TYPE_GRABBER, 5);
	if (ret)
	{
		v4l2_err(&mydev->v4l2_dev, "Failed to register video device\n");
		goto unreg_dev;
	}

	video_set_drvdata(vfd, mydev);
	snprintf(vfd->name, sizeof(vfd->name), "%s", myvivi_videodev.name);
	v4l2_info(&mydev->v4l2_dev, "Device registered as /dev/video%d\n", vfd->num);

	platform_set_drvdata(pdev, mydev);

    /* 用定时器产生数据并唤醒进程 */
	init_timer(&mydev->timer);
    mydev->timer.function  = myvivi_timer_function;

    INIT_LIST_HEAD(&myvivi_vb_local_queue);

	return 0;

unreg_dev:
	v4l2_device_unregister(&mydev->v4l2_dev);

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
static int myvivi_remove(struct platform_device *pdev)
{
	struct myvivi_dev *mydev = platform_get_drvdata(pdev);

	v4l2_info(&mydev->v4l2_dev, "%s \n", __func__);
	video_unregister_device(&mydev->vfd);
	v4l2_device_unregister(&mydev->v4l2_dev);

	return 0;
}

static void myvivi_dev_release(struct device *dev)
{
}

static struct platform_device myvivi_pdev = {
	.name		= MYVIVI_NAME,
	.dev.release	= myvivi_dev_release,
};

/**
 * 平台设备结构体
 * 
 * @author zpq (18-5-20)
 */
static struct platform_driver myvivi_pdrv = {
	.probe		= myvivi_probe,
	.remove		= myvivi_remove,
	.driver		= {
		.name	= MYVIVI_NAME,
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
static void __exit myvivi_exit(void)
{
	printk("run %s\n", __func__);
	platform_driver_unregister(&myvivi_pdrv);
	platform_device_unregister(&myvivi_pdev);

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
static int __init myvivi_init(void)
{
	int ret;
	printk("run %s\n", __func__);
	ret = platform_device_register(&myvivi_pdev);
	if (ret) return ret;

	ret = platform_driver_register(&myvivi_pdrv);
	if (ret) platform_device_unregister(&myvivi_pdev);

	printk("%s OK\n", __func__);
	return 0;
}

/* 模块加载时运行 */
module_init(myvivi_init);
/* 模块卸载时运行 */
module_exit(myvivi_exit);

MODULE_DESCRIPTION("My vivi camera");
MODULE_AUTHOR("ZhengPengqiao, <157510312@qq.com>");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.1.1");
