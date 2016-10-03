#ifndef _NET_MODEL_H_
#define _NET_MODEL_H_

#define EPOLL_MODEL   1
#define POLL_MODEL    2
#define SELECT_MODEL  3

#define READ          1
#define WRITE         2

#include <sys/epoll.h>

typedef struct netmodel
{
    int epfd;   //epoll hanlder
    struct epoll_event *evs;  //epoll_event array
    int evs_n; //size of epoll_event array
} netmodel_t;

struct fdparam;
typedef int (*callback)(struct fdparam*);
typedef struct fdparam
{
    int fd;
    callback cb;
    struct epoll_event ev;
} fdparam_t;

int start(int fd);
#endif
