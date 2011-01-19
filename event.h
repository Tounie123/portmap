/**
 * portmap event part
 *
 * author: liyangguang (liyangguang@software.ict.ac.cn)
 *
 * date: 2011-01-17:15:02
 *
 * changes
 */
#ifndef _PORTMAP_EVENT_H
#define _PORTMAP_EVENT_H

#include "list.h"
#define EPOLL_SIZE 65535
typedef int (* event_handler_t)(int fd, int event, void *opaque);
struct event_info
{
    int fd; 
    event_handler_t handler;
    void *data;
	struct list_head ei_list;

};

int
init_event();

int register_event(int fd, event_handler_t handler, void *opaque);

int unregister_event(int fd);

void loop_event(int timeout);

#endif /* _PORTMAP_EVENT_H */
