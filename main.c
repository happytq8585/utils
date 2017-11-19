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
#include <sys/epoll.h>


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

    ret = listen(sd, LISTENSIZE);
    if (ret < 0) {
        perror("listen()");
        return -1;
    }
    return sd;
}

int myread(int fd)
{
    char buf[1024];
    int ret, n=0;
    while (1) {
        ret = read(fd, buf, sizeof(buf));
        if (ret < 0) {
            if (errno == EINTR) {
                continue;
            }
            break;
        }
        if (ret == 0) {
            break;
        }
        buf[ret] = 0;
        printf("%s\n", buf);
    }
    return 0;
}
int mywrite(int fd)
{
    char buf[1024] = "good good study, day day up!";
    int ret, offset=0, n = strlen(buf);
    while (offset < n) {
        ret = write(fd, buf+offset, n-offset);
        if (ret < 0) {
            if (errno == EINTR) {
                continue;
            }
            break;
        }
        offset += ret;
    }
    return n;
}

int epfd = -1;
struct epoll_event* ep = NULL;
int epoll_size = LISTENSIZE;
static int init_epfd() {
    int epfd = epoll_create(epoll_size);
    if (epfd < 0) {
        fprintf("epoll_create failed!\n");
        return -1;
    }
    struct epoll_event* p = malloc(epoll_size*sizeof(struct epoll_event));
    if (p == NULL) {
        close(epfd);
        fprintf("malloc failed!\n");
        return -1;
    }
    return 0;
}

static int start(int sd) {
    struct epoll_event ev = {EPOLLET | EPOLLIN | EPOLLRDHUP | EPOLLPRI, (void*)sd};
    int ret = epoll_ctl(epfd, EPOLL_CTL_ADD, sd, &ev);
    if (ret < 0) {
        fprintf("epoll_ctl add socket fd failed!\n");
        close(epfd);
        close(sd);
        return -1;
    }
    int n = 0, i;
    for (;;) {
        n = epoll_wait(epfd, ep, epoll_size, 500);
        for (i = 0; i < n; ++i) {
            int fd = (int)ep[i].data.ptr;
            handle(fd);
        }
    }
    return 0;
}
int main(int argc, char* argv[])
{
    int ret, sd;
    struct sockaddr_in server_addr;
    sd = ret = init(argc, argv, &server_addr);
    if (ret < 0) {
        return -1;
    }
    ret = start(sd);
    return 0;
}
