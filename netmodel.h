#ifndef _NET_MODEL_H_
#define _NET_MODEL_H_

#define ACCEPT        1
#define CLIENT        2

#define READ          1
#define WRITE         2

#define ADD           1
#define MOD           2
#define DEL           3

#include <sys/epoll.h>
#include <unistd.h>

/**
 *@brief:   epoll network model data structer
 */
typedef struct netmodel
{
    int epfd;   //epoll hanlder
    struct epoll_event *evs;  //epoll_event array
    int evs_n; //size of epoll_event array
} netmodel_t;

//when comes up a file descriptor, define a handler how to handle it
typedef int (*fd_op)(int fd);

/**
 *@brief:  the file descriptor handlers
 */
struct fd_operations
{
    fd_op     rd_op;   //when fd becomes read already
    fd_op     wr_op;   //when fd becomes write already
    fd_op     err_op;  //when fd becomes error
    fd_op     hub_op;  //when fd becomes hangup
    fd_op     pri_op;  //when fd has priority data 
};

/**
 *@brief:  a file descriptor context
 */
typedef struct fd_ctx
{
    int fd;  //a file descriptor
    struct fd_operations* fops;  //fd operations
    struct epoll_event ev;       
    void (*handle)(void* arg, int sd); //handle a fd
} fd_ctx_t;

int start(int fd);
#endif
