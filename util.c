#include <util.h>
#include <sys/types.h>
#include <fcntl.h>

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
