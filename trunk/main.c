/**
 * portmap main func 
 *
 * author liyangguang (liyangguang@software.ict.ac.cn)
 *
 * date: 2011-01-17 16:21:00
 *
 * changes
 */
#include "net.h"
#include "event.h"
#include "log.h"
#include "worker.h"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <signal.h>

#define DEFAULT_LISTEN_PORT 5900
#define DEFAULT_MAPPED_LOWER_PORT 5901
#define DEFAULT_MAPPED_HIGH_PORT 65535

#define DEFAULT_LOG_FILE "portmap.log"

#define DEFAULT_TIMEOUT  -1

#define DEFAULT_THREAD_NUM 1

int g_started = 0;

void sig_handler(int signo)
{
    g_started = 0;
    log_close();
}

int 
main(int argc, char **argv)
{
    int ret = 0;
    unsigned int port = DEFAULT_LISTEN_PORT; 
    const char *log_file = DEFAULT_LOG_FILE;
    int timeout = DEFAULT_TIMEOUT;

    if (argc == 2) {
        port = (unsigned int)atoi(argv[1]); 
        assert(port > 0); 
    }
    else if (argc >= 3) {
        port = (unsigned int)atoi(argv[1]); 
        assert(port > 0); 
        
        log_file = argv[2];
    }
    signal(SIGINT, sig_handler);

    ret = log_init(log_file);

    if (ret < 0) {
    
        fprintf(stderr, "init_log error.");
        return -1;
    }

    ret = init_event();

    if (ret < 0) {
        
        fprintf(stderr, "init_event error.");
        return -1;
    
    }

    g_started = 1;

    ret = init_local_server(port, DEFAULT_MAPPED_LOWER_PORT, DEFAULT_MAPPED_HIGH_PORT);

    if (ret < 0) {
    
        fprintf(stderr, "init_local_server error.");
        return -1;
    }
    
    
    ret = init_worker(DEFAULT_THREAD_NUM);
    if (ret < 0) {
        fprintf(stderr, "init_worker error.");
        return -1;
    }

    loop_event(timeout);
    
    return 0;
}
