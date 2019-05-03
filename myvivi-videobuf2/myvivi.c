#include <linux/module.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/font.h>
#include <linux/mutex.h>
#include <linux/platform_device.h>
#include <linux/videodev2.h>
#include <linux/kthread.h>
#include <linux/freezer.h>
#include <media/videobuf2-vmalloc.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-fh.h>
#include <media/v4l2-event.h>
#include <media/v4l2-common.h>

#define VFL_TYPE_GRABBER    0

#define MAX_WIDTH 1920
#define MAX_HEIGHT 1200
static unsigned int vid_limit = 16;
struct mutex		dev_mutex;
struct v4l2_device	v4l2_dev;
struct video_device	video_dev;              // video_device 结构，用来描述一个 video 设备
static struct vb2_queue vivi_queue;         // vivi_queue 用来存放缓冲区信息，缓冲区链表等
struct task_struct         *kthread;        // 内核线程，用来向缓冲区中填充数据
DECLARE_WAIT_QUEUE_HEAD(wait_queue_head);   // 等待队列头
struct list_head        my_list;            // 链表头

// 用来存放应用程序设置的视频格式
static struct mformat {
    __u32           width;
    __u32           height;
    __u32           pixelsize;
    __u32           field;
    __u32           fourcc;
    __u32           depth;
}mformat;

static void mvideo_device_release(struct video_device *vdev)
{
    dev_info(&video_dev.dev, "in %s \n", __func__);
}


static int vivi_mmap(struct file *file, struct vm_area_struct *vma)
{   
    int ret;
    dev_info(&video_dev.dev, "in %s \n", __func__);

    ret = vb2_mmap(&vivi_queue, vma);
    if(ret == 0){
        dev_info(&video_dev.dev, "in %s mmap ok\n", __func__);
    }else{
        dev_info(&video_dev.dev, "in %s mmap error\n", __func__);
    }
    return ret;
}


// 查询设备能力
static int mvidioc_querycap(struct file *file, void  *priv, struct v4l2_capability *cap)
{
    dev_info(&video_dev.dev, "in %s\n", __func__);
    strcpy(cap->driver, "vivi");
    strcpy(cap->card, "vivi");
    strcpy(cap->bus_info, "mvivi");
	cap->device_caps = V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_STREAMING;
	cap->capabilities = cap->device_caps | V4L2_CAP_DEVICE_CAPS;

    return 0;
}

// 枚举视频支持的格式
static int vidioc_enum_fmt_vid_cap(struct file *file, void  *priv, struct v4l2_fmtdesc *f)
{
    dev_info(&video_dev.dev, "in %s\n", __func__);
    if (f->index >= 1)
        return -EINVAL;

    strcpy(f->description, "mvivi");
    f->pixelformat = mformat.fourcc;

    return 0;
}

// 修正应用层传入的视频格式
static int vidioc_try_fmt_vid_cap(struct file *file, void *priv, struct v4l2_format *f)
{
    dev_info(&video_dev.dev, "in %s\n", __func__);
    if (f->fmt.pix.pixelformat != V4L2_PIX_FMT_UYVY) {
        return -EINVAL;
    }

    f->fmt.pix.field = V4L2_FIELD_INTERLACED;
    v4l_bound_align_image(&f->fmt.pix.width, 48, MAX_WIDTH, 2,
                  &f->fmt.pix.height, 32, MAX_HEIGHT, 0, 0);
    f->fmt.pix.bytesperline = (f->fmt.pix.width * mformat.depth) / 8;
    f->fmt.pix.sizeimage    = f->fmt.pix.height * f->fmt.pix.bytesperline;
    if (mformat.fourcc == V4L2_PIX_FMT_UYVY)
        f->fmt.pix.colorspace = V4L2_COLORSPACE_SMPTE170M;
    else
        f->fmt.pix.colorspace = V4L2_COLORSPACE_SRGB;
    return 0;
}

// 获取支持的格式
static int vidioc_g_fmt_vid_cap(struct file *file, void *priv, struct v4l2_format *f)
{
    dev_info(&video_dev.dev, "in %s\n", __func__);
    // 将参数写回用户空间
    f->fmt.pix.width        = mformat.width;
    f->fmt.pix.height       = mformat.height;
    f->fmt.pix.field        = mformat.field;
    f->fmt.pix.pixelformat  = mformat.fourcc;
    f->fmt.pix.bytesperline = (f->fmt.pix.width * mformat.depth) / 8;
    f->fmt.pix.sizeimage    = f->fmt.pix.height * f->fmt.pix.bytesperline;
    if (mformat.fourcc == V4L2_PIX_FMT_UYVY)
        f->fmt.pix.colorspace = V4L2_COLORSPACE_SMPTE170M;
    else
        f->fmt.pix.colorspace = V4L2_COLORSPACE_SRGB;

    return 0;
}

// 设置视频格式
static int vidioc_s_fmt_vid_cap(struct file *file, void *priv, struct v4l2_format *f)
{
    int ret = 0;

    dev_info(&video_dev.dev, "in %s\n", __func__);
    ret = vidioc_try_fmt_vid_cap(file, priv, f);
    if (ret < 0)
        return ret;
        // 存储用户空间传入的参数设置
    mformat.fourcc      =  V4L2_PIX_FMT_UYVY;
    mformat.pixelsize   =  mformat.depth / 8;
    mformat.width       =  f->fmt.pix.width;
    mformat.height      =  f->fmt.pix.height;
    mformat.field       =  f->fmt.pix.field;
    printk("vidioc_s_fmt_vid_capp  \n");
    return 0;
}


int vidioc_g_input(struct file *file, void *fh, unsigned int *i)
{
    dev_info(&video_dev.dev, "in %s\n", __func__);

	return 0;
}


int vidioc_s_input(struct file *file, void *fh, unsigned int i)
{
    dev_info(&video_dev.dev, "in %s\n", __func__);

	return 0;
}


// vb2 核心层 vb2_reqbufs 中调用它，确定申请缓冲区的大小
static int queue_setup(struct vb2_queue *q, unsigned int *num_buffers, unsigned int *num_planes,
			   unsigned int sizes[], struct device *alloc_devs[])
{
    unsigned long size;
    dev_info(&video_dev.dev, "in %s\n", __func__);

    printk("mformat.width %d \n",mformat.width);
    printk("mformat.height %d \n",mformat.height);
    printk("mformat.pixelsize %d \n",mformat.pixelsize);

    size = mformat.width * mformat.height * mformat.pixelsize;

    if (0 == *num_buffers)
        *num_buffers = 32;

    while (size * *num_buffers > vid_limit * 1024 * 1024)
        (*num_buffers)--;

    *num_planes = 1;

    sizes[0] = size;
    return 0;
}

static int buffer_init(struct vb2_buffer *vb)
{
    dev_info(&video_dev.dev, "in %s\n", __func__);
    return 0;
}

static void buffer_finish(struct vb2_buffer *vb)
{
    dev_info(&video_dev.dev, "in %s\n", __func__);
}

static int buffer_prepare(struct vb2_buffer *vb)
{
    unsigned long size;

    dev_info(&video_dev.dev, "in %s\n", __func__);
    size = mformat.width * mformat.height * mformat.pixelsize;
    vb2_plane_size(vb, 0);
    //vb2_set_plane_payload(&buf->vb, 0, size);
    return 0;
}

static void buffer_queue(struct vb2_buffer *vb)
{
    dev_info(&video_dev.dev, "in %s\n", __func__);

}

// 内核线程中填充数据，效果是一个逐渐放大的圆形，视频大小为 640 * 480
static void vivi_fillbuff(struct vb2_buffer *vb)
{
    void *vbuf = NULL;   

    dev_info(&video_dev.dev, "in %s\n", __func__);
    unsigned char (*p)[mformat.width][mformat.pixelsize];
    static unsigned int t = 0;
    unsigned int i,j;
    vbuf = vb2_plane_vaddr(vb, 0);
    p = vbuf;

    for(j = 0; j < mformat.height; j++){
        for(i = 0; i < mformat.width; i++){
            if((j - 240)*(j - 240) + (i - 320)*(i - 320) < (t * t)){
                *(*(*(p+j)+i)+0) = (unsigned char)0xff;
                *(*(*(p+j)+i)+1) = (unsigned char)0xff;
            }else{
                *(*(*(p+j)+i)+0) = (unsigned char)0;
                *(*(*(p+j)+i)+1) = (unsigned char)0;
            }           
        }
    }
    t++;
    printk("%d\n",t);
    if( t >= mformat.height/2) t = 0;
}

// 内核线程每一次唤醒调用它
static void vivi_thread_tick(void)
{
    struct vb2_buffer *buf = NULL;

    dev_info(&video_dev.dev, "in %s\n", __func__);
    if (list_empty(&vivi_queue.queued_list)) {
        //printk("No active queue to serve\n");
        return;
    }
    
    // 注意我们这里取出之后就删除了，剩的重复工作，但是在 dqbuf 时，vb2_dqbuf 还会删除一次，我做的处理是在dqbuf之前将buf随便挂入一个链表
    buf = list_entry(vivi_queue.queued_list.next, struct vb2_buffer, queued_entry);
    list_del(&buf->queued_entry);

    /* 填充数据 */
    vivi_fillbuff(buf);
    printk("filled buffer %p\n", buf->planes[0].mem_priv);

    // 它干两个工作，把buffer 挂入done_list 另一个唤醒应用层序，让它dqbuf
    vb2_buffer_done(buf, VB2_BUF_STATE_DONE);
}

#define WAKE_NUMERATOR 30
#define WAKE_DENOMINATOR 1001
#define BUFFER_TIMEOUT     msecs_to_jiffies(500)  /* 0.5 seconds */
#define frames_to_ms(frames)                    \
    ((frames * WAKE_NUMERATOR * 1000) / WAKE_DENOMINATOR)

static void vivi_sleep(void)
{
    int timeout;
    DECLARE_WAITQUEUE(wait, current);

    dev_info(&video_dev.dev, "in %s\n", __func__);
    add_wait_queue(&wait_queue_head, &wait);
    if (kthread_should_stop())
        goto stop_task;

    /* Calculate time to wake up */
    timeout = msecs_to_jiffies(frames_to_ms(1));

    vivi_thread_tick();

    schedule_timeout_interruptible(timeout);

stop_task:
    remove_wait_queue(&wait_queue_head, &wait);
    try_to_freeze();
}

static int vivi_thread(void *data)
{
    dev_info(&video_dev.dev, "in %s\n", __func__);
    set_freezable();

    for (;;) {
        vivi_sleep();

        if (kthread_should_stop())
            break;
    }
    printk("thread: exit\n");
    return 0;
}

static int vivi_start_generating(void)
{   
    dev_info(&video_dev.dev, "in %s\n", __func__);
    kthread = kthread_run(vivi_thread, &video_dev, video_dev.name);

    if (IS_ERR(kthread)) {
        printk("kthread_run error\n");
        return PTR_ERR(kthread);
    }

    /* Wakes thread */
    wake_up_interruptible(&wait_queue_head);

    return 0;
}

static int start_streaming(struct vb2_queue *vq, unsigned int count)
{
    dev_info(&video_dev.dev, "in %s\n", __func__);
    vivi_start_generating();
    return 0;
}

static void stop_streaming(struct vb2_queue *q)
{
    dev_info(&video_dev.dev, "in %s\n", __func__);
    if (kthread) {
        kthread_stop(kthread);
        kthread = NULL;
    }
     /*

     while (!list_empty(&vivi_queue.queued_list)) {   
       struct vb2_buffer *buf; 
       buf = list_entry(vivi_queue.queued_list.next, struct vb2_buffer, queued_entry); 
       list_del(&buf->queued_entry); 
       vb2_buffer_done(buf, VB2_BUF_STATE_ERROR);
     }
     */
}
static struct vb2_ops vivi_video_qops = {
    .queue_setup        = queue_setup,
    .buf_init       = buffer_init,
    .buf_finish     = buffer_finish,
    .buf_prepare        = buffer_prepare,
    .buf_queue      = buffer_queue,
    .start_streaming    = start_streaming,
    .stop_streaming     = stop_streaming,
};

static int mvivi_open(struct file *filp)
{   
    dev_info(&video_dev.dev, "in %s\n", __func__);

    vivi_queue.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    vivi_queue.io_modes = VB2_MMAP;
    vivi_queue.drv_priv = &video_dev;
    //vivi_queue.buf_struct_size = sizeof(struct vivi_buffer);
    vivi_queue.ops      = &vivi_video_qops;
    vivi_queue.mem_ops  = &vb2_vmalloc_memops;
    vivi_queue.buf_struct_size = sizeof(struct vb2_buffer);
    INIT_LIST_HEAD(&vivi_queue.queued_list);
    INIT_LIST_HEAD(&vivi_queue.done_list);
    spin_lock_init(&vivi_queue.done_lock);
    init_waitqueue_head(&vivi_queue.done_wq);
    mutex_init(&vivi_queue.mmap_lock);
    mformat.fourcc = V4L2_PIX_FMT_UYVY;
    mformat.depth  = 16;
    INIT_LIST_HEAD(&my_list);
    return 0;
}


static int mvivi_release(struct file *file)
{
    dev_info(&video_dev.dev, "in %s\n", __func__);

	return 0;
}


static int vidioc_reqbufs(struct file *file, void *priv, struct v4l2_requestbuffers *p)
{
    dev_info(&video_dev.dev, "in %s\n", __func__);
    printk("count %d\n",p->count);
    printk("memory %d\n",p->memory);
    return vb2_reqbufs(&vivi_queue, p);
}

static int vidioc_querybuf(struct file *file, void *priv, struct v4l2_buffer *p)
{
    dev_info(&video_dev.dev, "in %s\n", __func__);
    return vb2_querybuf(&vivi_queue, p);
}

static int vidioc_qbuf(struct file *file, void *priv, struct v4l2_buffer *p)
{
    dev_info(&video_dev.dev, "in %s\n", __func__);
    return vb2_qbuf(&vivi_queue, p);
}

static int vidioc_dqbuf(struct file *file, void *priv, struct v4l2_buffer *p)
{
    struct vb2_buffer *vb;

    dev_info(&video_dev.dev, "in %s\n", __func__);
    vb = list_first_entry(&vivi_queue.done_list, struct vb2_buffer, done_entry);
    list_add_tail(&vb->queued_entry, &my_list);
    return vb2_dqbuf(&vivi_queue, p, file->f_flags & O_NONBLOCK);
}

static int vidioc_streamon(struct file *file, void *priv, enum v4l2_buf_type i)
{
    dev_info(&video_dev.dev, "in %s\n", __func__);

    return vb2_streamon(&vivi_queue, i);
}

static int vidioc_streamoff(struct file *file, void *priv, enum v4l2_buf_type i)
{
    dev_info(&video_dev.dev, "in %s\n", __func__);

    return vb2_streamoff(&vivi_queue, i);
}

static struct v4l2_ioctl_ops mvivi_ioctl_ops = {
    .vidioc_querycap            = mvidioc_querycap,
	/* 用于列举、获得、测试、设置摄像头的数据的格式 */ 
    .vidioc_enum_fmt_vid_cap    = vidioc_enum_fmt_vid_cap,
    .vidioc_g_fmt_vid_cap       = vidioc_g_fmt_vid_cap,
    .vidioc_try_fmt_vid_cap     = vidioc_try_fmt_vid_cap,
    .vidioc_s_fmt_vid_cap       = vidioc_s_fmt_vid_cap,
    
    .vidioc_g_input             = vidioc_g_input,
	.vidioc_s_input             = vidioc_s_input,

	/* 缓冲区操作: 申请/查询/放入队列/取出队列 */
    .vidioc_reqbufs             = vidioc_reqbufs,
    .vidioc_querybuf            = vidioc_querybuf,
    .vidioc_qbuf                = vidioc_qbuf,
    .vidioc_dqbuf               = vidioc_dqbuf,

	// 启动/停止
    .vidioc_streamon            = vidioc_streamon,
    .vidioc_streamoff           = vidioc_streamoff,
};


static unsigned int mvivi_poll(struct file *file, struct poll_table_struct *wait)
{
    struct vb2_buffer *vb = NULL;
    int res = 0;
    dev_info(&video_dev.dev, "in %s\n", __func__);

    poll_wait(file, &vivi_queue.done_wq, wait);

    if (!list_empty(&vivi_queue.done_list))
        vb = list_first_entry(&vivi_queue.done_list, struct vb2_buffer, done_entry);

    if (vb && (vb->state == VB2_BUF_STATE_DONE || vb->state == VB2_BUF_STATE_ERROR)) {
        return (V4L2_TYPE_IS_OUTPUT(vivi_queue.type)) ?
                res | POLLOUT | POLLWRNORM :
                res | POLLIN | POLLRDNORM;  
    }
    return 0;
}


/**
 * v4l2文件的操作函数
 * 
 * @author zpq (18-5-20)
 */
static const struct v4l2_file_operations mvivi_fops = {
	.owner		= THIS_MODULE,
	.open		= mvivi_open,
	.unlocked_ioctl	= video_ioctl2,
	.release	= mvivi_release,
    .poll       = mvivi_poll,
    .mmap       = vivi_mmap,
};


/**
 * video设备
 * 
 * @author zpq (18-5-20)
 */
static struct video_device vivi_template = {
	.name		= "mvivi",
	.fops		= &mvivi_fops,
	.ioctl_ops	= &mvivi_ioctl_ops,
	.minor		= -1,
	.release	= mvideo_device_release,
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
static int myvivi_probe(struct platform_device *pdev)
{
	int ret;

	v4l2_info(pdev, "%s \n", __func__);

	/* 注册v4l2设备 */
	ret = v4l2_device_register(&pdev->dev, &v4l2_dev);
	if (ret) return ret;

	mutex_init(&dev_mutex);

	/* 分配一个video_device结构体 */
	video_dev = vivi_template;
	video_dev.lock = &dev_mutex;
	video_dev.v4l2_dev = &v4l2_dev;

	/* 注册 */
	ret = video_register_device(&video_dev, VFL_TYPE_GRABBER, 5);
	if (ret)
	{
		v4l2_err(&v4l2_dev, "Failed to register video device\n");
		goto unreg_dev;
	}

	snprintf(video_dev.name, sizeof(video_dev.name), "%s", vivi_template.name);
	v4l2_info(&v4l2_dev, "Device registered as /dev/video%d\n", video_dev.num);


 
	return 0;

unreg_dev:
	v4l2_device_unregister(&v4l2_dev);

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
	v4l2_info(&v4l2_dev, "%s \n", __func__);
	video_unregister_device(&video_dev);
	v4l2_device_unregister(&v4l2_dev);

	return 0;
}

static void myvivi_dev_release(struct device *dev)
{
}

static struct platform_device myvivi_pdev = {
	.name		= "mvivi",
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
		.name	= "mvivi",
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
	if (ret)
    {
        return ret;
    }

	ret = platform_driver_register(&myvivi_pdrv);
	if (ret)
    {
        platform_device_unregister(&myvivi_pdev);
    }
	
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
