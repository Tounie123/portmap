/**
 * portmap  net part
 *
 * author:liyangguang (liyangguang@software.ict.ac.cn)
 * date: 2011-01-14 15:11:01
 *
 * changes
 */

#ifndef _PORTMAP_NET_H
#define _PORTMAP_NET_H

#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "list.h"
#define MAX_PORT_NUM 65536
#define MAX_REQ_SIZE 1024
#define MAX_DATA_LEN 65536
struct local_server 
{
    int sockfd; 
    unsigned int listen_port;
    unsigned int accepted_num;

    /** port range to map */
    unsigned int lower_port;
    unsigned int high_port;
    unsigned int used_ports[MAX_PORT_NUM];
};


struct mapped_server
{
    int sockfd;
    unsigned int listen_port;
    unsigned int accepted_num;
    /** src address to map */
    char src_addr[INET_ADDRSTRLEN];
    unsigned int src_port;
    /** mapped address */
    char dest_addr[INET_ADDRSTRLEN];
    struct local_server *ls;
    struct list_head ms_list;

};

struct mapped_pair
{
    int remote_sockfd; // connect to src address
    int local_sockfd;  // client connect to dest address
    unsigned int send_bytes;
    unsigned int recv_bytes;
    int ref;  //ref = 2
    struct mapped_server *ms;
    pthread_mutex_t mutex;
};


int init_local_server(unsigned int port, unsigned int lower_port, unsigned int high_port);

#endif /*_PORTMAP_NET_H */
