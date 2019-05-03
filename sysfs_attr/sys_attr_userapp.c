#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>//memset()

int main(int argc, char *argv[])
{
    int fd,cnt;
    int num1, num2;
    char buf[256];
    int len = 0;
    int i;
    printf("sys attr device testing..\n");
    fd = open("/sys/devices/platform/sys_attr.0/sys_attr/sys_attr",O_RDWR);
    if(fd == 0)
    {
        printf("open failed.\n");
        return 1;
    }
    
    printf("input the data for kernel:");
    scanf("%x%x", &num1, &num2);
    len = sprintf(buf, "%x %x\n", num1, num2);
    cnt = write(fd, buf, len);
    if(cnt == 0)
        printf("write error\n");
    
    printf("clear buf,and will read from kernel...\n");
    memset(buf, 0, sizeof(buf));

    cnt = read(fd, buf, 256);
    if(cnt > 0)
        printf("read data from kernel is:%s\n",buf);
    else
        printf("read data error\n");
    close(fd);
    printf("close app..\n");
    return 0;
}