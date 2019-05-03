#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

int main(int argc, char *argv[])
{
    int fd,cnt;
    char tx[2];
    char val[2];
    int i = 0;

    fd = open("/dev/hello_gpio", O_RDWR);
    if(fd == 0)
    {
        printf("open failed.\n");
        return 1;
    }
    
    for( i = 0; i < 100; i++ )
    {
        tx[0] = !tx[0];
        tx[1] = !tx[1];
        cnt = write(fd, tx, 2);
        if(cnt == 0)
        {
            printf("write error\n");
        }

        printf("clear buf,and will read from kernel...\n");
        cnt = read(fd, val, 2);
        if(cnt > 0)
            printf("read data from kernel is: val[0]=%d val[1]=%d \n",val[0], val[1]);
        else
            printf("read data error\n");
        sleep(1);
    }

    close(fd);
    printf("close app..\n");
    return 0;
}