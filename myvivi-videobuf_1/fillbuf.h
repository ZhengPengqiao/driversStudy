/* 
* @file fillbuf.c
* @author ZhengPengqiao
* @date 2018-05-21
*/

#ifndef __FILLBUF_H_
#define __FILLBUF_H_

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

extern void myvivi_precalculate_bars(int input, struct v4l2_format informat);
extern void myvivi_fillbuff(struct videobuf_buffer *vb);

#endif //__FILLBUF_H_
