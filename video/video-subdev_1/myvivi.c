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
#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/ctype.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/of_device.h>
#include <linux/i2c.h>
#include <linux/v4l2-mediabus.h>
#include <linux/of_gpio.h>
#include <linux/pinctrl/consumer.h>
#include <linux/regulator/consumer.h>
#include <media/v4l2-device.h>
#include <linux/videodev2.h>
#include <media/v4l2-ioctl.h>
#include <media/videobuf2-core.h>
#include <media/videobuf2-dma-contig.h>

static unsigned debug;
module_param(debug, uint, 0644);
MODULE_PARM_DESC(debug, "activates debug info");

int i2c_bus = 0;
int i2c_addr = 0x40;

#define MYVIVI_NAME		"myvivi"
struct myvivi_dev *mydev;
static struct i2c_client *myvivi_client = NULL;

struct myvivi_dev {
	struct v4l2_subdev		subdev;
	struct v4l2_device	v4l2_dev;
	struct i2c_client *client;
	struct device *dev;

	atomic_t		num_inst;
	struct mutex		dev_mutex;
	spinlock_t		irqlock;

	struct list_head myvivi_vb_local_queue;
	struct v4l2_format myvivi_format;

	struct timer_list	timer;
};

/**
 * 视频设备通常需要实现core和video成员，这两个OPS中的操作都是可选的
 * 
 * @author zpq (18-5-30)
 */
static struct v4l2_subdev_ops myvivi_subdev_ops = {
	//.core	= &ov5642_subdev_core_ops,
	//.video	= &ov5642_subdev_video_ops,
	//.pad	= &ov5642_subdev_pad_ops,
};


/*!
 * myvivi I2C probe function
 *
 * @param adapter            struct i2c_adapter *
 * @return  Error code indicating success or failure
 */
static int myvivi_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	struct device *dev = &client->dev;

	int retval = 0;
	dev_info(dev, "%s: In %s()\n", __FILE__, __func__);


	mydev = devm_kzalloc(&client->dev, sizeof(struct myvivi_dev), GFP_KERNEL);
	if (!mydev)
		return -ENOMEM;



	mydev->client = client;
	mydev->dev = dev;
	mydev->subdev.v4l2_dev = &mydev->v4l2_dev;
	sprintf(mydev->subdev.name, "%s" , "myvivid-subdev");


	/* register v4l2 device */
	v4l2_subdev_init(&mydev->subdev, &myvivi_subdev_ops);
	mydev->subdev.flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;
	mydev->subdev.owner = THIS_MODULE;
	strlcpy(mydev->v4l2_dev.name, "mx8-img-md", sizeof(mydev->v4l2_dev.name));
	/* 注册v4l2设备 */
	retval = v4l2_device_register(dev, &mydev->v4l2_dev);
	if (retval)
	{
		v4l2_err(&mydev->v4l2_dev, "Failed to register ret=(%d)\n",  retval);
		goto v4l2_err;
	}

	retval = v4l2_device_register_subdev(&mydev->v4l2_dev, &mydev->subdev);
	if (retval < 0)
	{
		v4l2_err(&mydev->v4l2_dev, "Failed to register ret=(%d)\n", retval);
		return retval;
	}

	retval = v4l2_device_register_subdev_nodes(&mydev->v4l2_dev);
	if (retval < 0) {
		v4l2_err(&mydev->v4l2_dev, "%s v4l2_device_register_subdev_nodes err\n", __func__);
		return retval;
	}
	
	return retval;
v4l2_err:
	devm_kfree(&client->dev, mydev);
	return retval;
}


/*!
 * max9286 I2C detach function
 *
 * @param client            struct i2c_client *
 * @return  Error code indicating success or failure
 */
static int myvivi_remove(struct i2c_client *client)
{
	dev_info(mydev->dev, "In %s() Ok\n",  __func__);

	v4l2_device_unregister_subdev(&mydev->subdev);
	if (mydev)
		devm_kfree(&client->dev, mydev);
	return 0;
}


static const struct i2c_device_id myvivi_id[] = {
	{"myvivi", 0},
	{ },
};
MODULE_DEVICE_TABLE(i2c, myvivi_id);

static struct i2c_driver myvivi_i2c_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name  = "myvivi",
	},
	.probe  = myvivi_probe,
	.remove = myvivi_remove,
	.id_table = myvivi_id,
};

static int myvivi_i2c_driver_init(void)
{
	u8 err;

	err = i2c_add_driver(&myvivi_i2c_driver);
	if (err != 0)
		pr_err("%s:driver registration failed, error=%d\n",	__func__, err);
	return err;
}

static void myvivi_i2c_driver_exit(void)
{
	i2c_del_driver(&myvivi_i2c_driver);
}

static struct i2c_board_info myvivi_info = {
	I2C_BOARD_INFO("myvivi", 0x40),
};


/**
 * i2c设备注册函数 
 * 在模块加载的时候,直接注册i2c设备,不使用设备树 
 * 
 * @author zpq (18-5-21)
 * 
 * @return int 0:Ok -1:Err
 */
static int myvivi_i2c_device_init(void)
{
	struct i2c_adapter *adapter;
	adapter = i2c_get_adapter(i2c_bus);
	if(!adapter)
		return -ENODEV;

	myvivi_info.addr = i2c_addr;
	myvivi_client = i2c_new_device(adapter, &myvivi_info);
	if(!myvivi_client) {
		i2c_put_adapter(adapter);
		return -EBUSY;
	}
	i2c_put_adapter(adapter);
	return 0;
}


/**
 * i2c卸载函数, 卸载i2c的device
 * 
 * @author zpq (18-5-21)
 * 
 * @param void 
 */
static void myvivi_i2c_device_exit(void)
{
	if(myvivi_client) i2c_unregister_device(myvivi_client);
}


/**
 * 模块初始化函数 
 * 在模块加载时,内核会运行此函数,在函数中注册i2c的driver驱动,并且注册device设备 
 * 
 * @author zpq (18-5-20)
 * 
 * @return int __init 0:OK -1:Err
 */
static __init int myvivi_init(void)
{
	u8 err = 0;

	err = myvivi_i2c_device_init();
	if(err != 0) return err;

	err = myvivi_i2c_driver_init();
	if(err) goto err1;

	pr_debug("myvivi_init: module init\n");
	return err;

err1:
	myvivi_i2c_device_exit();
	return err;
}

/**
 * 模块卸载函数 
 * 在卸载模块的时候,内核会运行此函数,卸载i2c的driver和i2c的设备
 * 
 * @author zpq (18-5-20)
 * 
 * @param void 
 * 
 * @return void __exit 
 */
static void __exit myvivi_exit(void)
{
	myvivi_i2c_device_exit();
	myvivi_i2c_driver_exit();
	pr_debug("myvivi_exit: module exit\n");
}

/* 模块加载时运行 */
module_init(myvivi_init);
/* 模块卸载时运行 */
module_exit(myvivi_exit);

MODULE_AUTHOR("ZhengPengqiao");
MODULE_DESCRIPTION("MYVIVID Driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.1");


