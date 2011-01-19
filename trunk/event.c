/**
 * portmap event part 
 * 
 * author: liyangguang (liyangguang@software.ict.ac.cn)
 *
 * date: 2011-01-17 15:12:00
 *
 * changes
 */

#include "event.h"
#include "log.h"
#include "list.h"
#include <sys/epoll.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

//epoll fd
static int g_efd;
static LIST_HEAD(events_list);

static struct event_info *lookup_event(int fd)
{
    struct event_info *ei;

	list_for_each_entry(ei, &events_list, ei_list) {
		if (ei->fd == fd)
			return ei;
	}
	return NULL;
}

int
init_event()
{
    g_efd = epoll_create(EPOLL_SIZE); 

    if (g_efd == -1) {
    
        LOG_ERR("epoll_create error.[error:%s]", strerror(errno));
        return -1;
    }
    
    LOG_INFO("epoll_create success.[g_efd:%d]", g_efd);
    return 0;
}

int
register_event(int fd, event_handler_t handler, void *opaque)
{
    int ret;
    struct epoll_event ev;
    struct event_info *ei;

    ei = (struct event_info *)malloc(sizeof(struct event_info));

    ei->fd = fd;
    ei->handler = handler;
    ei->data = opaque;
    
    ev.events = EPOLLIN;

    ev.data.ptr = ei;

    ret = epoll_ctl(g_efd, EPOLL_CTL_ADD, fd, &ev);

    if (ret == -1) {
    
        LOG_ERR("epoll_ctl EPOLL_CTL_ADD error.[fd:%d]", fd);
        return -1;
    }
    list_add(&ei->ei_list, &events_list);

    LOG_INFO("epoll_ctl EPOLL_CTL_ADD success.[g_efd:%d] [fd:%d]", g_efd, fd);

    return 0;
}



int
unregister_event(int fd)
{

    int ret ;    

    struct event_info *ei;

    ei = lookup_event(fd);

    if (ei == NULL) {
    
        LOG_ERR("lookup_event failed.[fd:%d]", fd);
        return -1;
    }

    ret = epoll_ctl(g_efd, EPOLL_CTL_DEL, fd, NULL);

    if (ret == -1) {
    
        LOG_ERR("epoll_ctl EPOLL_CTL_DEL failed.[fd:%d]",fd);
        return -1;
    }

    list_del(&ei->ei_list);

    free(ei);
    LOG_INFO("epoll_ctl EPOLL_CTL_DEL success.[fd:%d]", fd);
    return 0;
}


extern int g_started;
void
loop_event(int timeout)
{
    struct epoll_event events[EPOLL_SIZE];    
    int i, size;
    struct event_info *ei;
    LOG_INFO("enter loop_event success....");
    while (g_started) {
        size = epoll_wait(g_efd, events, EPOLL_SIZE, timeout);

        if (size == -1) {

            LOG_ERR("epoll_wait failed. [error:%s]", strerror(errno));
            break;
        }

        if (size) {

            for (i = 0; i < size; ++i) {

                ei = (struct event_info *)events[i].data.ptr;
                ei->handler(ei->fd, events[i].events, ei->data);
            }
        }
    }
    //free
}
