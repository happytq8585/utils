#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

int setnonblock(int fd) 
{
    int ret;
    ret = fcntl(fd, F_GETFL);
    if (ret < 0) {
        return -1; 
    }   
    ret = fcntl(fd, F_SETFL, ret|O_NONBLOCK);
    if (ret < 0) {
        return -1; 
    }   
    return 0;
}

int main(int argc, char* argv[])
{
    int sd = socket(AF_INET, SOCK_STREAM, 0);
    if (sd < 0) {
        perror("socket()");
        return -1;
    }
    if (setnonblock(sd) < 0) {
        fprintf(stderr, "setnonblock() failed!\n");
        return -1;
    }
    struct sockaddr_in server_addr;
    server_addr.sin_family     = AF_INET;
    server_addr.sin_port       = htons(8080);
    if (inet_pton(AF_INET, "127.0.0.1", (void*)&server_addr.sin_addr) != 1) {
        perror("inet_pton()");
        return -1;
    }
    int ret = connect(sd, (void*)&server_addr, sizeof(server_addr));
    if (ret < 0 && errno != EINPROGRESS && errno != EALREADY) {
        perror("connect()");
        return -1;
    }
    char rbuf[1024], sbuf[1024] = "hello world";
    while (1) {
        int ret = write(sd, sbuf, 11);
        printf("ret=%d\n", ret);
        ret = read(sd, rbuf, sizeof(rbuf));
        rbuf[ret] = 0;
        printf("ret=%d %s\n", ret, rbuf);
        sleep(2);
    }
    return 0;
}
