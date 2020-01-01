#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>//memset()
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "mem_test.h"

#define CMD_WRITR       1
#define CMD_READ        2
#define CMD_GETSIZE     3
#define CMD_SETSIZE     4
#define CMD_CYCLETEST     5

int cmd = 0;
int size = 1024;
int times = 1024;

void showHelp()
{
	printf("./programe options \n");	
	printf("USED:");
	printf("	-help: show help info\n");
	printf("	-cmd val: 1:write 2:read 3:get size 4:resize default 5:mem_help(%d)\n", cmd);
	printf("	-size val: resize size value default(%d)\n", size);
	printf("	-times val: cycle test times default(%d)\n", times);
}

int checkParam(int argc,char **argv)
{
	int i = 0;
	for(i = 1;i < argc;i++)
	{
		if( strcmp("-help", argv[i]) == 0 )
		{
			showHelp();
			return -1;
		}
		else if( strcmp("-size", argv[i]) == 0)
		{
			size = atoi(argv[i+1]);
			i++;
		}
        else if( strcmp("-times", argv[i]) == 0)
		{
			times = atoi(argv[i+1]);
			i++;
		}
        else if( strcmp("-cmd", argv[i]) == 0)
		{
			cmd = atoi(argv[i+1]);
			i++;
		}
		else
		{
			printf("param %s is not support \n\n", argv[i]);
			showHelp();
			return -1;
		}
	}
	return 0;
}


int cmd_write(int fd)
{
    int cnt;
    char buf[256];
    printf("input the data for kernel:");
    scanf("%s",buf);
    cnt = write(fd, buf, 256);
    if(cnt == 0)
    {
        printf("write error\n");
    }
    return 0;
}

int cmd_read(int fd)
{
    char buf[256];
    int i;
    int cnt;

    printf("clear buf,and will read from kernel...\n");
    for(i=0;i<256;i++)
        buf[i] = 32;

    cnt = read(fd, buf, 256);
    if(cnt > 0)
        printf("read data from kernel is:%s\n",buf);
    else
        printf("read data error\n");
    
    return 0;
}


int cmd_set_size(int fd)
{
    int ioctl_cmd = 0;

    printf("<--- Call MEMDEV_IOCSETSIZE --->\n");
    ioctl_cmd = MEMDEV_IOCSETSIZE;
    if (ioctl(fd, ioctl_cmd, &size) < 0)
    {
        printf("Call cmd MEMDEV_IOCSETSIZE fail\n");
        return -1;
    }
    return 0;
}


int cmd_get_size(int fd)
{
    int ioctl_cmd = 0;

    printf("<--- Call MEMDEV_IOCGETSIZE --->\n");
    ioctl_cmd = MEMDEV_IOCGETSIZE;
    if (ioctl(fd, ioctl_cmd, &size) < 0)
        {
            printf("Call cmd MEMDEV_IOCGETSIZE fail\n");
            return -1;
    }
    printf("<--- In User Space MEMDEV_IOCGETSIZE Get Size is %d --->\n\n", size);  
    return 0;
}



int cmd_cycle_test(int fd)
{
    int i = 0;
    int ioctl_cmd = 0;
    for( i = 0 ; i < times; i++ )
    {
        printf("<--- Call MEMDEV_IOCSETSIZE --->\n");
        ioctl_cmd = MEMDEV_IOCSETSIZE;
        if (ioctl(fd, ioctl_cmd, &size) < 0)
        {
                printf("Call cmd MEMDEV_IOCSETSIZE fail\n");
                return -1;
        }
        usleep(10*1000);
        printf("<--- Call MEMDEV_IOCGETSIZE --->\n");
        ioctl_cmd = MEMDEV_IOCGETSIZE;
        if (ioctl(fd, ioctl_cmd, &size) < 0)
            {
                printf("Call cmd MEMDEV_IOCGETSIZE fail\n");
                return -1;
        }
        printf("<--- In User Space MEMDEV_IOCGETSIZE Get Size is %d --->\n\n", size); 
    }
}


int main(int argc,char **argv)
{
    int i;
    int fd;
    int ioctl_cmd = 0;

	if( checkParam(argc, argv) )
	{
		return -1;
	}

    // 打开设备节点
    printf("char device testing..\n");
    fd = open("/dev/mem_test",O_RDWR);
    if(fd == 0)
    {
        printf("open failed.\n");
        return 1;
    }

    switch(cmd)
    {
        case CMD_WRITR:
            cmd_write(fd);
        break;
        case CMD_READ:
            cmd_read(fd);
        break;
        case CMD_GETSIZE: /* 调用命令CMD_GETSIZE */  
            cmd_get_size(fd);
        break;
        case CMD_SETSIZE: /* 调用命令CMD_SETSIZE*/
            cmd_set_size(fd);
        break;
        case CMD_CYCLETEST:
            cmd_cycle_test(fd);
        break;
        default:
            cmd_write(fd);
            cmd_read(fd);
        break;
    }

    close(fd);
    printf("close app..\n");
    return 0;
}