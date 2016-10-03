#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <event.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>

#include <netmodel.h>

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

static int init(int argc, char* argv[], struct sockaddr_in* addr)
{
    if (argc != 3) {
        fprintf(stderr, "Usage:%s ip port\n", argv[0]);
        return -1;
    }
    int sd = socket(AF_INET, SOCK_STREAM, 0);
    if (sd < 0) {
        perror("socket()");
        return -1;
    }
    int ret, flag=1, len=sizeof(flag);
    ret = setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &flag, len);
    if (ret < 0) {
        perror("setsockopt()");
        return -1;
    }
    ret = setnonblock(sd);
    if (ret < 0) {
        fprintf(stderr, "setnonblock() failed!\n");
        return -1;
    }
    addr->sin_family    = AF_INET;
    ret = inet_pton(AF_INET, "0.0.0.0", (void*)&addr->sin_addr);
    if (ret != 1) {
        perror("inet_pton()");
        return -1;
    }
    addr->sin_port      = htons(atoi(argv[2]));

    ret = bind(sd, (void*)addr, sizeof(*addr));
    if (ret < 0) {
        perror("bind()");
        return -1;
    }

    ret = listen(sd, 4096);
    if (ret < 0) {
        perror("listen()");
        return -1;
    }
    return sd;
}

int main(int argc, char* argv[])
{
    int ret, sd;
    struct sockaddr_in server_addr;
    sd = ret = init(argc, argv, &server_addr);
    if (ret < 0) {
        return -1;
    }
    start(sd);
}
