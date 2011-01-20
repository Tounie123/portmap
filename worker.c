/**
 * portmap worker thread
 *
 * worker.c
 *
 * author: liyangguang (liyangguang@software.ict.ac.cn)
 *
 * date:2011-01-19 17:34:05
 *
 * changes
 */

#include "worker.h"
#include "log.h"
#include "event.h"
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>

#define TASK_QUEUE_SIZE 65535
#define ERR_BUF_SIZE 100
#define MAX_THREAD_NUM 100

static struct epoll_event s_task_queue[TASK_QUEUE_SIZE];
static int s_tq_start = 0;
static int s_tq_end = 0;
static int s_tq_len = 0;
static pthread_t s_threads[MAX_THREAD_NUM];
static int s_thread_num;

//cond & mutex
static pthread_mutex_t s_tq_mutex;

//task queue empty mutex & cond
static pthread_mutex_t  s_tq_emutex;
static pthread_cond_t   s_tq_econd;

//task queue full mutex & cond
static pthread_mutex_t s_tq_fmutex;
static pthread_cond_t   s_tq_fcond;

static void *main_worker(void *opaque);

int
add_tasks(struct epoll_event tasks[], int task_num)
{
    int start = 0, ret;
    char err_buf[ERR_BUF_SIZE] = {0};
    if (task_num == 0) {
    
        return 0;
    }

    while (TASK_QUEUE_SIZE - s_tq_len < task_num) {
        ret = pthread_cond_wait(&s_tq_fcond, &s_tq_fmutex); 
        if (ret < 0) {
            strerror_r(ret, err_buf, ERR_BUF_SIZE);
            LOG_ERR("pthread_cond_wait error.[errno:%d] [error:%s]", ret, err_buf);
            return -1;
        }
    }
    pthread_mutex_lock(&s_tq_mutex);
    for (start = 0; start < task_num; ++start) {
        s_task_queue[s_tq_end] = tasks[start];
        s_tq_end = (s_tq_end + 1) % TASK_QUEUE_SIZE;
        ++s_tq_len;
    }
    LOG_DEBUG("add tasks success.[task_num:%d] [s_tq_start:%d] [s_tq_end:%d] [s_tq_len:%d]", task_num, s_tq_start, s_tq_end, s_tq_len);
    pthread_cond_signal(&s_tq_econd);
    pthread_mutex_unlock(&s_tq_mutex);
    return 0;
}


int
init_worker(int thread_num)
{
    int i = 0, ret;
    char err_buf[ERR_BUF_SIZE] = {0};

    LOG_INFO("start init_worker.....");
    pthread_mutex_init(&s_tq_mutex, NULL);
    pthread_mutex_init(&s_tq_emutex, NULL);
    pthread_mutex_init(&s_tq_fmutex, NULL);
    pthread_cond_init(&s_tq_econd, NULL);
    pthread_cond_init(&s_tq_fcond, NULL);
    

    assert(thread_num > 0);
    s_thread_num = thread_num;
    for (i = 0; i < thread_num; ++i) {
        ret = pthread_create(&s_threads[i], NULL, main_worker, NULL);    
        if (ret < 0) {
            strerror_r(ret, err_buf, ERR_BUF_SIZE);
            LOG_ERR("phtread_create failed. [errno:%d] [err_buf:%s]", ret, err_buf); 
            --i;
            continue;
        } 
        LOG_INFO("pthread_create success.[idx:%d]", i);
    }
    return 0;
}

int
wait_worker()
{
    int i = 0, ret;
    LOG_INFO("start wait_worker.....");
    for (i = 0; i < s_thread_num; ++i) {
        ret = pthread_join(s_threads[i], NULL);
        LOG_INFO("pthread_join.[ret:%d] [thread_id:%lu]", ret, s_threads[i]);
    }
    pthread_mutex_destroy(&s_tq_mutex);
    pthread_mutex_destroy(&s_tq_emutex);
    pthread_mutex_destroy(&s_tq_fmutex);
    pthread_cond_destroy(&s_tq_econd);
    pthread_cond_destroy(&s_tq_fcond);
    return 0;

}
extern int g_started;
static void *main_worker(void *opaque)
{
    struct epoll_event ev; 
    struct event_info *ei;
    int ret;
    char err_buf[ERR_BUF_SIZE] = {0};
    pthread_t pid;
    pid = pthread_self();
    LOG_INFO("starting new thread.[thread_id:%lu]", pid);
    while(g_started) {
        
        pthread_mutex_lock(&s_tq_mutex);
        if (s_tq_len == 0) {
            pthread_mutex_unlock(&s_tq_mutex);
            ret = pthread_cond_wait(&s_tq_econd, &s_tq_emutex);
            if (ret < 0) {
                strerror_r(ret, err_buf, ERR_BUF_SIZE);
                LOG_ERR("pthread_cond_wait error.[errno:%d] [error:%s]", ret, err_buf);
                return NULL;
            }
            //LOG_INFO("pthread_cond_wait signal.[thread_id:%lu] [s_tq_start:%d] [s_tq_end:%d] [s_tq_len:%d]", 
            //       pid, s_tq_start, s_tq_end, s_tq_len);

            //must be checked again!!
            continue;
        }
        ev = s_task_queue[s_tq_start];
        LOG_DEBUG("execute thread_id:%lu s_tq_start:%d s_tq_end:%d s_tq_len:%d", pid, s_tq_start, s_tq_end, s_tq_len);
        s_tq_start = (s_tq_start + 1) % TASK_QUEUE_SIZE;
        --s_tq_len;
        pthread_mutex_unlock(&s_tq_mutex);
        pthread_cond_signal(&s_tq_fcond);

        ei = (struct event_info *)ev.data.ptr;
        //execute    
        ret = ei->handler(ei->fd, ev.events, ei->data);
        if (ret < 0) {
            LOG_ERR("finish execute handler error.[thread_id:%lu] [ret:%d] ", pid, ret); 
        }
    }

    LOG_INFO("thread exit.[thread_id:%lu]", pid);

    pthread_exit(NULL);
}
