/**
 * portmap worker thread 
 *
 * worker.h
 *
 * author: liyangguang (liyangguang@software.ict.ac.cn)
 *
 * date: 2011-01-19 17:28:01
 *
 * changes
 */

#ifndef _PORTMAP_WORKER_H
#define _PORTMAP_WORKER_H

#include <sys/epoll.h>


int init_worker(int thread_num);

int add_tasks(struct epoll_event tasks[], int task_num);

int wait_worker();

#endif /* _PORTMAP_WORKER_H */
