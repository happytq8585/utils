#include <netmodel.h>
#include <errno.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static int del_fd(int fd, fdparam_t*p);
static int add_fd(int fd, uint32_t event, fdparam_t*p);
static int mod_fd(int fd, uint32_t event, callback cb, fdparam_t* p);

static netmodel_t netmodel;

static init_model(int size)
{
    int epfd = epoll_create(size);
    if (epfd < 0) {
        perror("epoll_create()");
        goto err2;
    }
    struct epoll_event* p = malloc(size*sizeof(struct epoll_event));
    if (p == NULL) {
        fprintf(stderr, "malloc() at %s failed", __func__);
        goto err1;
    }
    netmodel.epfd = epfd;
    netmodel.evs  = p;
    netmodel.evs_n= size;
    return 0;
err1:
    close(epfd);
err2:
    return -1;
}

static int client_write(fdparam_t *p)
{
    int fd = p->fd, ret;
    char *buf = "HTTP/1.1 200 OK\r\nContent-type:text/html\r\nContent-length:12\r\nHello World!";
    ret = write(fd, buf, strlen(buf));
    del_fd(fd, p);
    free(p);
    close(fd);
}

static int client_read(fdparam_t *p)
{
    int fd = p->fd, ret;
    char buf[1024];
    while (1) {
        ret = read(fd, buf, sizeof(buf));
        if (ret < 0) {
            if (errno != EAGAIN && errno != EINPROGRESS) {
                close(fd);
                return -1;
            }
            break;
        }
        write(1, buf, ret);
    }
    ret = mod_fd(fd, WRITE, client_write, p);
    if (ret < 0) {
        fprintf(stderr, "add_fd() failed!\n");
        del_fd(fd, p);
        close(fd);
        return -1;
    }
    return 0;
}
static int del_fd(int fd, fdparam_t* p)
{
    int epfd = netmodel.epfd;
    int ret = epoll_ctl(epfd, EPOLL_CTL_DEL, fd, &p->ev);
    if (ret < 0) {
        perror("epoll_ctl()");
        return -1;
    }
    free(p);
    return 0;
}
static int mod_fd(int fd, uint32_t event, callback cb, fdparam_t* p)
{
    int epfd = netmodel.epfd;
    p->ev.events  = 0;
    p->cb = cb;
    if (p->ev.events&READ) {
        p->ev.events |= EPOLLIN;
    }
    if (p->ev.events&WRITE) {
        p->ev.events |= EPOLLOUT;
    }
    int ret = epoll_ctl(epfd, EPOLL_CTL_MOD, p->fd, &p->ev);
    if (ret < 0) {
        perror("epoll_ctl()");
        return -1;
    }
    return 0;
}
static int accept_cb(struct fdparam* p)
{
    int fd = accept(p->fd, NULL, NULL);
    if (fd < 0) {
        return -1;
    }
    int ret = setnonblock(fd);
    if (ret < 0) {
        close(fd);
        return -1;
    }
    fdparam_t* ptr = malloc(sizeof(fdparam_t));
    if (ptr == NULL) {
        fprintf(stderr, "malloc() failed at %s\n", __func__);
        return -1;
    }
    ptr->fd = fd;
    ptr->cb = client_read;
    ptr->ev.events = EPOLLIN;
    ret = add_fd(fd, READ, ptr);
    if (ret < 0) {
        free(ptr);
        return -1;
    }
    return 0;
}
static int add_fd(int fd, uint32_t event, fdparam_t* p)
{
    int epfd = netmodel.epfd;
    p->fd = fd;
    p->ev.data.ptr = p;
    p->ev.events = 0;
    if (event&READ) {
        p->ev.events |= EPOLLIN;
    }
    if (event&WRITE) {
        p->ev.events |= EPOLLOUT;
    }
    int ret = epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &p->ev);
    if (ret < 0) {
        perror("epoll_ctl()");
        return -1;
    }
}
//sd: socket fd
int start(int sd)
{
    if (init_model(4096) < 0) {
        return -1;
    }
    fdparam_t sockparam;
    sockparam.cb = accept_cb;
    int ret = add_fd(sd, READ, &sockparam);
    if (ret < 0) {
        fprintf(stderr, "add_accept() failed\n");
        return -1;
    }
    int epfd = netmodel.epfd;
    struct epoll_event *evs = netmodel.evs;
    int evs_n = netmodel.evs_n;
    while (1) {
        int n = epoll_wait(epfd, evs, evs_n, 500);
        int i;
        for (i = 0; i < n; ++i) {
            fdparam_t* p = evs[i].data.ptr;
            p->cb(p);
        }
    }
}
