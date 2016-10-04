#include <netmodel.h>
#include <errno.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <util.h>

static netmodel_t netmodel;
static char *buf = "HTTP/1.1 200 OK\r\nContent-type:text/html\r\nContent-length:12\r\n\r\nHello World!";
static void handle(void* arg);
static int accept_read(int sd);
static int default_read(int fd);
static int default_write(int fd);
static struct fd_operations accept_op = {.rd_op = accept_read};
static struct fd_operations client_op = {.rd_op = default_read, .wr_op = default_write};

int regist(int how, fd_op cb)
{
    static int arr[] = {0, READ, WRITE, ERR, HUP, PRI};
    if (how < 1 || how > arrlen(arr)-1) {
        return -1;
    }
    typedef struct fd_operations op;
    static int off[] = {0, offsetof(struct fd_operations, rd_op), offsetof(struct fd_operations, wr_op), offsetof(struct fd_operations, err_op), offsetof(struct fd_operations, hup_op), offsetof(struct fd_operations, pri_op)};
    *(fd_op*)((char*)&client_op + off[how]) = cb;
    return 0;
}

static int hangup(int fd)
{
    int ret = 0;
    do {
        ret = close(fd);
    } while (ret < 0 && errno == EINTR);
    return ret;
}
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
            return -1;
        }
        if (ret == 0) {
            return 0;
        }
        buf[ret] = 0;
        printf("%s\n",  buf);
    }
    return 0;
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
    p->handle = handle;
    p->ptr = p;
}
static int init_ctx(fd_ctx_t *ctx)
{
    int ret, fd = ctx->fd;
    ctx->iev = EPOLLET | EPOLLIN | EPOLLRDHUP | EPOLLPRI;
    ctx->oev = 0;
    struct epoll_event ev = {ctx->iev, ctx};
    ret = epoll_ctl(netmodel.epfd, EPOLL_CTL_ADD, fd, &ev);
    if (ret < 0) {
        close(fd);
        free(ctx);
        return -1;
    }
    return 0;
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
        close(fd);
        return -1;
    }
    fd_ctx_t *ctx = create_ctx(fd, CLIENT);
    if (ctx == NULL) {
        close(fd);
        return -1;
    }
    ret = init_ctx(ctx);
    if (ret < 0) {
        return -1;
    }
    return 0;
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

static void handle(void* arg)
{
    int ret;
    fd_ctx_t *ctx = arg;
    int fd = ctx->fd;
    int epfd = netmodel.epfd;
    uint32_t events = ctx->oev;
    if (events & (EPOLLERR | EPOLLHUP)) {
        goto err;
    }
    if (events & EPOLLRDHUP) {
        goto err;
    }
    if (events & EPOLLPRI) {
        goto err;
    }
    if ((events & EPOLLIN) && ctx->fops->rd_op) {
        ret = ctx->fops->rd_op(fd);
        if (ret < 0) {
            goto err;
        }
        if (ctx->fops->wr_op) {
            ctx->iev |= EPOLLOUT;
            struct epoll_event ev = {ctx->iev, ctx};
            ret = epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev);
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
        ctx->iev &= ~EPOLLOUT;
        struct epoll_event ev = {ctx->iev, ctx};
        ret = epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev);
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
    int epfd = netmodel.epfd, i, n, evs_n, ret;
    ret = init_ctx(accept_ctx);
    if (ret < 0) {
        return -1;
    }
    struct epoll_event *evs = netmodel.evs;
    evs_n = netmodel.evs_n;
    while (1) {
        n = epoll_wait(epfd, evs, evs_n, 500);
        for (i = 0; i < n; ++i) {
            fd_ctx_t* p = evs[i].data.ptr;
            p->oev = evs[i].events;
            p->handle(p);
        }
    }
}
