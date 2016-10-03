#include <netmodel.h>
#include <errno.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <util.h>

static netmodel_t netmodel;
static char *buf = "HTTP/1.1 200 OK\r\nContent-type:text/html\r\nContent-length:12\r\n\r\nHello World!";
static void handle(void* arg, int sd);
static int accept_read(int sd);
static int default_read(int fd)
{
    char buf[2048] = {0};
    int offset = 0;
    while (1) {
        int ret = read(fd, buf, sizeof(buf));
        if (ret < 0) {
            if (errno == EINTR) {
                continue;
            }
            if (errno == EAGAIN) {
                printf("\n");
                return 0;
            }
            printf("\n");
            return -1;
        }
        buf[ret] = 0;
        printf("%s\n",  buf);
    }
}
static int default_write(int fd)
{
    int n = strlen(buf);
    int offset = 0;
    int ret;
    while (offset < n) {
        ret = write(fd, buf+offset, n-offset);
        if (ret < 0) {
            if (errno == EINTR) {
                continue;
            }
            if (errno == EAGAIN) {
                return 0;
            }
            return -1;
        }
        offset += ret;
    }
    return 0;
}

struct fd_operations accept_op = {.rd_op = accept_read};
struct fd_operations client_op = {.rd_op = default_read, .wr_op = default_write};
static fd_ctx_t* create_ctx(int fd, int who)
{
    fd_ctx_t * p = malloc(sizeof(fd_ctx_t));
    if (p == NULL) {
        return p;
    }
    p->fd = fd;
    if (who == ACCEPT) {
        p->fops = &accept_op;
    }
    else
    {
        p->fops = &client_op;
    }
    p->ev.events = 0;
    p->handle = handle;
    p->ev.data.ptr = p;
}

static int accept_read(int sd)
{
    int fd = accept(sd, NULL, NULL);
    if (fd < 0) {
        return -1;
    }
    int ret = setnonblock(fd);
    if (ret < 0) {
        printf("setnonblock() falield!\n");
        goto err1;
    }
    fd_ctx_t *ctx = create_ctx(fd, CLIENT);
    if (ctx == NULL) {
        goto err1;
    }
    ctx->ev.events = EPOLLET | EPOLLIN;
    ret = epoll_ctl(netmodel.epfd, EPOLL_CTL_ADD, fd, &ctx->ev);
    if (ret < 0) {
        close(fd);
        free(ctx);
    }
    return 0;
err1:
    close(fd);
    return -1;
}
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

static void handle(void* arg, int sd)
{
    int ret;
    fd_ctx_t *ctx = arg;
    int fd = ctx->fd;
    int epfd = netmodel.epfd;
    uint32_t events = ctx->ev.events;
    if (events & (EPOLLERR | EPOLLHUP)) {
        goto err;
    }
    if ((events & EPOLLIN) && ctx->fops->rd_op) {
        ret = ctx->fops->rd_op(fd);
        if (ret < 0) {
            goto err;
        }
        if (ctx->fops->wr_op) {
            ctx->ev.events |= EPOLLOUT;
            ret = epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ctx->ev);
            if (ret < 0) {
                goto err;
            }
        }
    }
    if ((events & EPOLLOUT) && ctx->fops->wr_op) {
        ret = ctx->fops->wr_op(fd);
        if (ret < 0) {
            goto err;
        }
        ctx->ev.events &= ~EPOLLOUT;
        ret = epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ctx->ev);
        if (ret < 0) {
            goto err;
        }
    }
    return;
err:
    close(fd);
    free(ctx);
}

//sd: socket fd
int start(int sd)
{
    if (init_model(4096) < 0) {
        return -1;
    }
    fd_ctx_t *accept_ctx = create_ctx(sd, ACCEPT);
    if (accept_ctx == NULL) {
        return -1;
    }
    int epfd = netmodel.epfd;
    accept_ctx->ev.events = EPOLLET | EPOLLIN;
    int ret = epoll_ctl(epfd, EPOLL_CTL_ADD, sd, &accept_ctx->ev);
    if (ret < 0) {
        close(accept_ctx->fd);
        free(accept_ctx);
        return -1;
    }
    struct epoll_event *evs = netmodel.evs;
    int evs_n = netmodel.evs_n;
    while (1) {
        int n = epoll_wait(epfd, evs, evs_n, 500);
        int i;
        for (i = 0; i < n; ++i) {
            fd_ctx_t* p = evs[i].data.ptr;
            p->handle(p, sd);
        }
    }
}
