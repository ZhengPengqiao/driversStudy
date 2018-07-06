#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/ctype.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/sched.h>
#include <linux/kthread.h>

#include <linux/pagemap.h>
#include <linux/vmalloc.h>
#include <linux/dma-mapping.h>

#include <linux/videodev2.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-ctrls.h>

#include <linux/i2c.h>
#include <linux/fsl_devices.h>
#include <linux/platform_device.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 10, 0)
#include <mach/mipi_csi2.h>
#include <mach/iomux-mx6q.h>
#include <mach/ipu-v3.h>
#else
#include <linux/platform_data/dma-imx.h>
#include <linux/regmap.h>
#include <linux/mfd/syscon.h>
#include <linux/mfd/syscon/imx6q-iomuxc-gpr.h>
#include <linux/pinctrl/consumer.h>
#include <linux/regulator/consumer.h>
#endif


struct camera_test {
	struct i2c_client*	client;

	/* v4l2 data member */
	struct v4l2_device	v4l2_dev;
	struct video_device	*vdev;
	struct mutex mutex;

};

static struct camera_test xdm;

static int video_nr = 5;
static int i2c_bus = 0;
static int i2c_addr = 0x60;


module_param(video_nr, int, 0444);
module_param(i2c_bus, int, 0444);
module_param(i2c_addr, int, 0444);

MODULE_PARM_DESC(video_nr, "  Set video node number '/dev/videoX'(default is 5).");
MODULE_PARM_DESC(i2c_bus, "   Set deserializer I2C bus number(default is 1).");
MODULE_PARM_DESC(i2c_addr, "  Set deserializer client addr(default is 0x6A).");


static struct v4l2_file_operations cameratest_fops = {
	.owner = THIS_MODULE,
};

void cameratest_release_empty(struct video_device *vdev)
{

	pr_info("In cameratest_release_empty\n");
}

static const struct video_device vdev_template = {
	.name		= "cameratest",
	.fops		= &cameratest_fops,
	.release	=  cameratest_release_empty,
};


static int cameratest_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct device *dev = &client->dev;
	struct video_device *vfd;
	int retval;


	dev_info(dev, "cameratest_probe Deserialize probe.\n");

	/* Check if the adapter supports the needed features */
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
		return -ENODEV;
	}

	memset(&xdm, 0, sizeof(xdm));
	xdm.client = client;

	retval = v4l2_device_register(dev, &xdm.v4l2_dev);
	if (retval) {
		goto err1;
	}

	dev_set_drvdata(dev, &xdm);

	xdm.vdev = video_device_alloc();
	if (xdm.vdev == NULL) {
		retval = -ENODEV;
		goto err2;
	}

	vfd = xdm.vdev;
	*vfd = vdev_template;

	mutex_init(&xdm.mutex);
	vfd->lock = &xdm.mutex;
	vfd->v4l2_dev = &xdm.v4l2_dev;
	vfd->minor = -1;

	video_set_drvdata(vfd, &xdm);

	if ( !xdm.vdev->release  ) 
	{
		pr_info(" xdm.vdev->release is NULL %p   %p \n",  xdm.vdev->release, vfd->release);
	}
	else
	{
		pr_info(" xdm.vdev->release is  %p Ok\n", xdm.vdev->release);
	}

	if ( !xdm.vdev->v4l2_dev  ) 
	{
		pr_info(" xdm.vdev->v4l2_dev is NULL %p   %p \n",  xdm.vdev->v4l2_dev, vfd->v4l2_dev);
	}
	else
	{
		pr_info(" xdm.vdev->v4l2_dev is  %p Ok\n", xdm.vdev->v4l2_dev);
	}

	retval = video_register_device(xdm.vdev, VFL_TYPE_GRABBER, video_nr);
	if (retval) {
		goto err3;
	}

	pr_debug("   Video device registered: %s #%d\n", xdm.vdev->name,
		xdm.vdev->minor);
       pr_info("cameratest probe done\n");
	return retval;

err3:
	video_device_release(xdm.vdev);
err2:
	v4l2_device_unregister(&xdm.v4l2_dev);
err1:
	return retval;
}

static int cameratest_remove(struct i2c_client *client)
{
	video_unregister_device(xdm.vdev);
	video_device_release(xdm.vdev);
	v4l2_device_unregister(&xdm.v4l2_dev);

	pr_info("cameratest_remove done\n");
	return 0;
}

static const struct i2c_device_id cameratest_id[] = {
	{"xd_mipi", 0},
	{ },
};
MODULE_DEVICE_TABLE(i2c, cameratest_id);

static struct i2c_driver cameratest_i2c_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name  = "xd_mipi",
	},
	.probe  = cameratest_probe,
	.remove = cameratest_remove,
	.id_table = cameratest_id,
};

static int cameratest_i2c_driver_init(void)
{
	u8 err;

	err = i2c_add_driver(&cameratest_i2c_driver);
	if (err != 0)
		pr_err("%s:driver registration failed, error=%d\n",	__func__, err);
	return err;
}

static void cameratest_i2c_driver_exit(void)
{
	i2c_del_driver(&cameratest_i2c_driver);
}

static struct i2c_board_info cameratest_info = {
	I2C_BOARD_INFO("xd_mipi", 0x6A),
};

static struct i2c_client *cameratest_client = NULL;

static int cameratest_i2c_device_init(void)
{
	struct i2c_adapter *adapter;
	adapter = i2c_get_adapter(i2c_bus);
	if(!adapter)
		return -ENODEV;

	cameratest_info.addr = i2c_addr;
	cameratest_client = i2c_new_device(adapter, &cameratest_info);
	if(!cameratest_client) {
		i2c_put_adapter(adapter);
		return -EBUSY;
	}
	i2c_put_adapter(adapter);
	return 0;
}

static void cameratest_i2c_device_exit(void)
{
	if(cameratest_client)
		i2c_unregister_device(cameratest_client);
}

static __init int xd_mipi_init(void)
{
	u8 err = 0;

	err = cameratest_i2c_device_init();
	if(err != 0)
		return err;

	err = cameratest_i2c_driver_init();
	if(err)
		goto err1;

	pr_debug("sd_mipi_init: module init\n");
	return err;

err1:
	cameratest_i2c_device_exit();
	return err;
}

static void __exit xd_mipi_exit(void)
{
	cameratest_i2c_device_exit();
	cameratest_i2c_driver_exit();
	pr_debug("xd_mipi_exit: module exit\n");
}

module_init(xd_mipi_init);
module_exit(xd_mipi_exit);

MODULE_AUTHOR("Tamsong, Tec. Inc.");
MODULE_DESCRIPTION("GMSL Deserializer Driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.1");
MODULE_ALIAS("CSI");
MODULE_SUPPORTED_DEVICE("video");
