#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>//memset()

int main(int argc, char *argv[])
{
    int fd,cnt;
    char buf[256];
    int i;
    printf("char device testing..\n");
    fd = open("/dev/mem_test",O_RDWR);
    if(fd == 0)
    {
        printf("open failed.\n");
        return 1;
    }
    
    printf("input the data for kernel:");
    scanf("%s",buf);
    cnt = write(fd,buf,256);
    if(cnt == 0)
        printf("write error\n");
    
    printf("clear buf,and will read from kernel...\n");
    for(i=0;i<256;i++)
        buf[i] = 32;//32 =" "

    cnt = read(fd,buf,256);
    if(cnt > 0)
        printf("read data from kernel is:%s\n",buf);
    else
        printf("read data error\n");
    close(fd);
    printf("close app..\n");
    return 0;
}