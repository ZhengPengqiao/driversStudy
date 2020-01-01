#ifndef __MEM_TEST_H__
#define __MEM_TEST_H__

/* 定义幻数 */
#define MEMDEV_IOC_MAGIC  'k'

//nr为序号，datatype为数据类型,如int
//_IO(type,  nr ) //没有参数的命令
//_IOR(type, nr, datatype) //从驱动中读数据
//_IOW(type, nr, datatype) //写数据到驱动
//_IOWR(type,nr, datatype) //双向传送

/* 定义命令 */
#define MEMDEV_IOCHELP   _IO(MEMDEV_IOC_MAGIC, 0)
#define MEMDEV_IOCSETSIZE _IOW(MEMDEV_IOC_MAGIC, 1, int)
#define MEMDEV_IOCGETSIZE _IOR(MEMDEV_IOC_MAGIC, 2, int)

#define MEMDEV_IOC_MAXNR 3

#define MEM_SIZE		(1024*1024*2)
#define MYDEV_NAME		"mem_test"

struct MyDev {
	struct platform_device *pdev;
	struct class *mem_test_class; 
	dev_t devnum; 
	void *vaddr;
	long long int paddr;
	size_t mem_size;
};

#endif //__MEM_TEST_H__
