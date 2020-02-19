/*
 * Copyright (C) 2017 Freescale Semiconductor, Inc. All Rights Reserved.
 */
/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

#ifndef CLK_TEST_H_
#define CLK_TEST_H_

#include <media/v4l2-device.h>
#include <media/v4l2-ctrls.h>
#include <media/videobuf2-core.h>
#include <media/videobuf2-dma-contig.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/bug.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/list.h>
#include <linux/mfd/syscon.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <media/media-device.h>
#include <media/media-entity.h>
#include <media/v4l2-subdev.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-mem2mem.h>

#define MXC_ISI_DRIVER_NAME	"xd-isi"
#define MXC_ISI_MAX_DEVS	8
#define ISI_OF_NODE_NAME	"isi"
#define ASSIGNED_CLK_COUNT_MAX 8

struct mxc_isi_dev {
	struct platform_device		*pdev;
	int						id;
	struct clk		*clk;
};

struct clk_test {
	struct platform_device		*pdev;
	unsigned int isi_num;
	unsigned int isi_mask;
	struct mxc_isi_dev *mxc_isi[MXC_ISI_MAX_DEVS];
};

#endif
