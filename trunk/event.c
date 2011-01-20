/**
 * portmap event part 
 * 
 * author: liyangguang (liyangguang@software.ict.ac.cn)
 *
 * date: 2011-01-17 15:12:00
 *
 * changes
 * 1, worker thread support 2011-01-20 09:39:01
 */

#include "event.h"
#include "log.h"
#include "list.h"
#include "worker.h"
#include <sys/epoll.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

//epoll fd
static int s_efd;
static LIST_HEAD(s_events_list);

extern int g_started;

static struct event_info *lookup_event(int fd)
{
    struct event_info *ei;

	list_for_each_entry(ei, &s_events_list, ei_list) {
		if (ei->fd == fd)
			return ei;
	}
	return NULL;
}

int
init_event()
{
    s_efd = epoll_create(EPOLL_SIZE); 

    if (s_efd == -1) {
    
        LOG_ERR("epoll_create error.[error:%s]", strerror(errno));
        return -1;
    }
    
    LOG_INFO("epoll_create success.[s_efd:%d]", s_efd);
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
    
    ev.events = EPOLLIN | EPOLLET;

    ev.data.ptr = ei;

    ret = epoll_ctl(s_efd, EPOLL_CTL_ADD, fd, &ev);

    if (ret == -1) {
    
        LOG_ERR("epoll_ctl EPOLL_CTL_ADD error.[fd:%d] [errno:%d] [error:%s]", fd, errno, strerror(errno));
        return -1;
    }
    list_add(&ei->ei_list, &s_events_list);

    LOG_INFO("epoll_ctl EPOLL_CTL_ADD success.[s_efd:%d] [fd:%d]", s_efd, fd);

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

    ret = epoll_ctl(s_efd, EPOLL_CTL_DEL, fd, NULL);

    if (ret == -1) {
    
        LOG_ERR("epoll_ctl EPOLL_CTL_DEL failed.[fd:%d]",fd);
        return -1;
    }

    list_del(&ei->ei_list);

    free(ei);
    LOG_INFO("epoll_ctl EPOLL_CTL_DEL success.[fd:%d]", fd);
    return 0;
}



void
loop_event(int timeout)
{
    struct epoll_event events[EPOLL_SIZE];    
    int size;
    struct event_info *ei;
    LOG_INFO("enter loop_event success....");
    while (g_started) {
        size = epoll_wait(s_efd, events, EPOLL_SIZE, timeout);

        if (size == -1) {

            LOG_ERR("epoll_wait failed. [error:%s]", strerror(errno));
            continue;
        }

        if (size) {

            add_tasks(events, size);
#if 0
            for (i = 0; i < size; ++i) {

                ei = (struct event_info *)events[i].data.ptr;
                ei->handler(ei->fd, events[i].events, ei->data);
            }
#endif
        }
    }
    //free
    close(s_efd);
    list_for_each_entry(ei, &s_events_list, ei_list) {
        if (ei != NULL) {
            //free(ei->data); 
            free(ei);
        }
    }
}
