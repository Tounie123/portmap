/**
 * portmap net part
 *
 * author: liyangguang  (liyangguang@software.ict.ac.cn)
 *
 * date 2011-01-14 16:52:01
 *
 * changes
 */
#include "net.h"
#include "event.h"
#include "log.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/epoll.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

static LIST_HEAD(s_ms_list);

static inline int set_nonblock(int fd)
{
    int ret = fcntl(fd, F_GETFL);

    if (ret < 0) {
    
        LOG_ERR("fcntl F_GETFL error [error:%s]", strerror(errno));

        return -1;
    }

    ret = fcntl(fd, F_SETFL, ret | O_NONBLOCK);
    if (ret < 0) {
    
        LOG_ERR("fcntl F_SETFL error [error:%s]", strerror(errno));
        return -1;
    }

    return 0;

}

static int get_local_ip(char *res_ip);
static int create_server(unsigned int port);

static int connect_to(const char *addr, unsigned int port);

static struct mapped_server *find_mapped_server(const char *src_addr, unsigned int port);
static int
create_mapped_server(struct local_server *pls, const char *src_addr, 
        unsigned int port, char *mapped_res);
static int destroy_mapped_pair(int fd, struct mapped_pair *pmp);

int local_server_client_handler(int fd, int event, void *opaque);

int local_server_handler(int fd, int event, void *opaque);

int mapped_server_handler(int fd, int event, void *opaque);

int mapped_pair_handler(int fd, int event, void *opaque);

static int
get_local_ip(char *res_ip)
{
    assert(res_ip != NULL);

    char hostname[40];
    int ret;
    struct addrinfo *res, *res0;

    ret = gethostname(hostname, 40);

    if (ret == -1) {
        
        LOG_ERR("gethostname error [error:%s].", strerror(errno));
        return -1;
    }

    LOG_INFO("[hostname:%s]", hostname);

    ret = getaddrinfo(hostname, NULL, NULL, &res);

    if (ret == -1) {
    
        LOG_ERR("getaddrinfo error [error:%s]", strerror(errno));
        return -1;
    }

    for (res0 = res; res0 != NULL; res0=res0->ai_next) {
    
        struct sockaddr_in *sa = (struct sockaddr_in *)(res0->ai_addr);
        inet_ntop(AF_INET, &(sa->sin_addr), res_ip, INET_ADDRSTRLEN);
        break;
    }
    LOG_INFO("local ip:%s", res_ip);
    return 0;

}

static int create_server(unsigned int port)
{
    char service_name[60]; 
    int ret, sockfd, opt, success = 0;
    struct addrinfo hints, *res, *res0;

    memset(service_name, 0, sizeof(service_name));
    snprintf(service_name, sizeof(service_name), "%u", port);
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    
    ret = getaddrinfo(NULL, service_name, &hints, &res);
    
    if (ret < 0) {
        LOG_ERR("unable to getaddrinfo.[error:%s]", strerror(errno));
        return -1;
    }

    for (res0 = res; res0 != NULL; res0 = res0->ai_next) {
    
        sockfd = socket(res0->ai_family, res0->ai_socktype, res0->ai_protocol);

        if (sockfd < 0)
            continue;

        opt = 1;
        ret = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        if (ret != 0) {
            LOG_ERR("set SO_REUSEADDR error.[error:%s]", strerror(errno));
        }

        //bind
        ret = bind(sockfd, res0->ai_addr, res0->ai_addrlen);

        if (ret != 0) {
        
            LOG_ERR("bind error.[error:%s]", strerror(errno));
            close(sockfd);
            continue;
        }
        
        //listen
        ret = listen(sockfd, SOMAXCONN);

        if (ret != 0) {
        
            LOG_ERR("listen error.[error:%s]", strerror(errno));
            close(sockfd);
            continue;
        }
        //set NONBLOCK
        ret = set_nonblock(sockfd);

        if (ret < 0) {
            close(sockfd);
            continue;
        }
        ++success;
        break;
    
    }

    if (success == 0) {
    
        LOG_ERR("create server fd error");
        return -1;
    }

    LOG_INFO("create server sockfd success");
    return sockfd;
}


static int
connect_to(const char *addr, unsigned int port)
{
    assert(addr != NULL);
    char service_name[60];
    int sockfd, ret, success;
    struct addrinfo hints, *res, *res0;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    memset(service_name, 0, sizeof(service_name));
    snprintf(service_name, 60, "%u", port);
     
    ret = getaddrinfo(addr, service_name, &hints, &res);

    if (ret != 0) {
    
        LOG_ERR("getaddrinfo error.[error:%s]", strerror(errno));

        return -1;
    }
    success = 0; 
    LOG_INFO("connecting server.[addr:%s] [port:%u]", addr, port);
    for (res0 = res; res0 != NULL; res0 = res0->ai_next) {
    
        sockfd = socket(res0->ai_family, res0->ai_socktype, res0->ai_protocol);

        if (sockfd < 0) {
        
            LOG_ERR("create socket error.[error:%s]", strerror(errno));
            continue;
        }

        ret = connect(sockfd, res0->ai_addr, res0->ai_addrlen);

        if (ret < 0) {
        
            LOG_ERR("connect error. [error:%s]", strerror(errno));
            close(sockfd);
            continue;
        }
        LOG_INFO("connect success.");
        success = 1;
        break;
    }
    freeaddrinfo(res);
    return success == 1 ? sockfd : -1;
}


static struct mapped_server *
find_mapped_server(const char *src_addr, unsigned int port)
{
    struct mapped_server *pms; 

    list_for_each_entry(pms, &s_ms_list, ms_list) {
        if (strncmp(src_addr, pms->src_addr, INET_ADDRSTRLEN) == 0 
                && port == pms->src_port) {
        
            return pms;
        }
    }
    return NULL;

}


static int
create_mapped_server(struct local_server *pls, const char *src_addr, 
        unsigned int port, char *mapped_res)
{
    struct mapped_server *pms;
    int ret;
    unsigned int listen_port;


    //first find src_addr+port is exists?
    pms = find_mapped_server(src_addr, port);
    if (pms != NULL) {
    
        LOG_INFO("find_mapped_server = true [src_addr:%s] [port:%u].", src_addr, port); 
        snprintf(mapped_res, INET_ADDRSTRLEN+10, "%s:%u", pms->dest_addr, pms->listen_port);
        return 0;
    }
    LOG_INFO("find_mapped_server = false [src_addr:%s] [port:%u].", src_addr, port); 

    //others create new mapped_server
    for (listen_port = pls->lower_port; listen_port <= pls->high_port; ++listen_port) {
    
        if (pls->used_ports[listen_port] == 0) {
        
            ret = create_server(listen_port);
            if (ret < 0)
                continue;
            pls->used_ports[listen_port] = 1;
            break;
        }
    }
    if (ret < 0) {
    
        LOG_ERR("Can't create mapped_server");
        return -1;
    }
    pms = (struct mapped_server *)malloc(sizeof(struct mapped_server));
    pls->accepted_num += 1;
    pms->sockfd = ret;
    pms->listen_port = listen_port;
    pms->accepted_num = 0;
    strncpy(pms->src_addr, src_addr, INET_ADDRSTRLEN);
    pms->src_port = port;
    pms->ls = pls;
    get_local_ip(pms->dest_addr);
    snprintf(mapped_res, INET_ADDRSTRLEN+10, "%s:%u", pms->dest_addr, listen_port);
    ret = register_event(ret, mapped_server_handler, pms);

    if (ret < 0) {
        close(pms->sockfd);
        free(pms);
        return -1;
    }
    
    //add to the s_ms_list
    list_add(&pms->ms_list, &s_ms_list);
    return 0;

}



static int
destroy_mapped_pair(int fd, struct mapped_pair *pmp)
{
    assert(pmp != NULL);

    unregister_event(fd);
    //unregister_event(pmp->local_sockfd);
    //close(pmp->remote_sockfd);
    //close(pmp->local_sockfd);
    close(fd);
    pmp->ref -= 1;

    if (pmp->ref == 0) {
    
        free(pmp);
    }
    return 0;
}


int
local_server_client_handler(int fd, int event, void *opaque)
{
    struct local_server *pls = (struct local_server *)opaque;
    char req_buf[MAX_REQ_SIZE], src_addr[INET_ADDRSTRLEN], response_buf[MAX_REQ_SIZE];
    int nread = 0, ret = 0, nlen = 0, nwrite = 0;
    unsigned int src_port;
    char *colon_pos, *p;
    memset(req_buf, 0, sizeof(req_buf));
    memset(src_addr, 0, sizeof(src_addr));
    memset(response_buf, 0, sizeof(response_buf));
    response_buf[0] = '0';
    response_buf[1] = '|';
    if (event & EPOLLIN) {
    
        nread = recv(fd, req_buf, MAX_REQ_SIZE, 0);

        if (nread == 0 || nread < 0) {
        
            if (nread != EAGAIN) {
            
                unregister_event(fd);
                close(fd);
                LOG_INFO("close clientfd.[clientfd:%d]", fd);
                return 0;
            }
        } 
    }
    else {
        LOG_DEBUG("event != EPOLLIN");
        return -1;
    }
    colon_pos = strchr(req_buf, ':');
    if (colon_pos == NULL) {
    
        response_buf[0] = '1';
        LOG_ERR("req_buf error.colon not found [req_buf:%s]", req_buf); 
        goto send_response;
    }
    strncpy(src_addr, req_buf, (int)(colon_pos - req_buf));
    int tmp_port = atoi(colon_pos + 1);

    if (tmp_port <= 0) {
        
        response_buf[0] = '1';
        LOG_ERR("req_buf error.port < 0 [req_buf:%s]", req_buf); 
        goto send_response;
    
    }
    src_port = (unsigned int)tmp_port;
    ret = create_mapped_server(pls, src_addr, src_port, response_buf + 2);
    if (ret != 0) {
    
        response_buf[0] = '2';
        goto send_response;
    }
    
send_response:
    nlen = strlen(response_buf);
    LOG_INFO("send response to clientfd.[clientfd:%d] [response_buf:%s]", fd, response_buf);
    p = response_buf;
    do {
        nwrite = send(fd, p, nlen, 0);
        if (nwrite > 0) {
            nlen -= nwrite; 
            p = p + nwrite;
        }
        else {
        
            if (nwrite != EAGAIN) {
                LOG_ERR("send error. close fd [clientfd:%d]", fd);

                unregister_event(fd);
                close(fd);
                return -1;
            }
        }
    }while(nlen != 0);
    return 0;
}


//accept client
int
local_server_handler(int fd, int event, void *opaque)
{
    struct local_server *pls = (struct local_server *)opaque;
    struct sockaddr_in from_addr;
    socklen_t from_addr_len;
    int clientfd, ret;
    char str_ip[INET_ADDRSTRLEN];

    if (!(event & EPOLLIN)) return -1;
    from_addr_len = sizeof(from_addr);
    clientfd = accept(pls->sockfd, (struct sockaddr *)&from_addr, &from_addr_len);

    if (clientfd < 0) {
    
        LOG_ERR("accept error.[error:%s]", strerror(errno));

        return -1;
    }
    inet_ntop(AF_INET, &(from_addr.sin_addr), str_ip, INET_ADDRSTRLEN);
    LOG_INFO("client %s connect to local_server [clientfd:%d]", str_ip, clientfd);

    //from_addr_len = sizeof(from_addr);
    //getpeername(clientfd, &from_addr, &from_addr_len);
    //printf("ip:%s\n", inet_ntoa(from_addr.sin_addr));
    //inet_ntop(AF_INET, &(from_addr.sin_addr), str_ip, INET_ADDRSTRLEN);
    //LOG_INFO("client %s connect to local_server [clientfd:%d]", str_ip, clientfd);
    
    //set NONBLOCK
    set_nonblock(clientfd);
    ret = register_event(clientfd, local_server_client_handler, pls);

    if (ret < 0) {
        close(clientfd);
        return -1;
    }
    return 0;
}


int
mapped_server_handler(int fd, int event, void *opaque)
{

    struct mapped_server *pms = (struct mapped_server *)opaque;
    struct sockaddr_storage from_addr;
    socklen_t from_addr_len;
    int local_fd, remote_fd, ret;
    char str_from_addr[INET_ADDRSTRLEN];

    struct mapped_pair *pmp;
    //int sendbuffsize = MAX_DATA_LEN, optlen;

    if (!(event & EPOLLIN)) return -1;

    from_addr_len = sizeof(from_addr);
    local_fd = accept(pms->sockfd, (struct sockaddr *)&from_addr, &from_addr_len);

    if (local_fd < 0) {
        
        LOG_ERR("mapped_server accept error.[error:%s]", strerror(errno));
        return -1;
    }
    struct sockaddr_in * sa = (struct sockaddr_in *)&from_addr; 
    inet_ntop(AF_INET, &(sa->sin_addr), str_from_addr, INET_ADDRSTRLEN);

    LOG_INFO("client %s connect mapped_server [local_fd:%d]", str_from_addr, local_fd); 


    remote_fd = connect_to(pms->src_addr, pms->src_port);
    
    if (remote_fd < 0) {
        LOG_ERR("connect remote server error [remote_addr:%s] [remote_port:%u]", pms->src_addr, pms->src_port);
        close(local_fd);
        //remote server disconnected. so free the mapped_server struct
        unregister_event(pms->sockfd);
        close(pms->sockfd);
        pms->ls->used_ports[pms->listen_port] = 0;

        //delete from the s_sm_list
        list_del(&pms->ms_list);
        free(pms);
        return -1;
    }
    pmp = (struct mapped_pair *)malloc(sizeof(struct mapped_pair));

    pmp->remote_sockfd = remote_fd; 
    pmp->local_sockfd  = local_fd;
    pmp->send_bytes    = 0;
    pmp->recv_bytes    = 0;
    pmp->ref           = 2;
    pmp->ms            = pms;

    set_nonblock(remote_fd);
    set_nonblock(local_fd);
    ret = register_event(remote_fd, mapped_pair_handler, pmp);

    if (ret < 0) {
    
        close(remote_fd); 
        close(local_fd);
        free(pmp);
        return -1;
    }
    ret = register_event(local_fd, mapped_pair_handler, pmp);

    if (ret < 0) {
        unregister_event(remote_fd);
        close(remote_fd); 
        close(local_fd);
        free(pmp);
        return -1;
    }
    //increase send buffer
    //optlen = sizeof(sendbuffsize);
    //ret = setsockopt(local_fd, SOL_SOCKET, SO_SNDBUF, &sendbuffsize, optlen);
    //ret = setsockopt(remote_fd, SOL_SOCKET, SO_SNDBUF, &sendbuffsize, optlen);

    return 0;
}


int
mapped_pair_handler(int fd, int event, void *opaque)
{
    struct mapped_pair *pmp = (struct mapped_pair *)opaque;
    int read_fd, write_fd; 
    int nrecv, nlen, nsend;
    char buffer[MAX_DATA_LEN] = {0}, *p;
    
    if (pmp->ref == 1) {
    
        destroy_mapped_pair(fd, pmp);
        return -1;
    }
    read_fd = fd;
    if (pmp->remote_sockfd == fd) {
    
        write_fd = pmp->local_sockfd; 
    }
    else {
    
        write_fd = pmp->remote_sockfd;
    }
    nlen = 0; 
    do {
        nrecv = recv(read_fd, buffer, MAX_DATA_LEN, 0);
        //LOG_INFO("mapped_pair_handler recv [read_fd:%d] [nrecv:%d]", read_fd, nrecv);
        if (nrecv == -1) {
        
            if (errno == EAGAIN){
                break;
            }
            else {
                LOG_ERR("recv error.[errno:%d] [error:%s]", errno, strerror(errno));
                destroy_mapped_pair(read_fd, pmp);
                return -1;
            }
        }
        else if (nrecv == 0) {
            
            LOG_INFO("sockfd closed.[sockfd:%d] [nrecv:%d] ", read_fd, nrecv);
            destroy_mapped_pair(read_fd, pmp);
            return -1;
        }

        //send
        nlen = nrecv;
        p = buffer;
        while (nlen > 0) {
        
            nsend = write(write_fd, p, nlen); 
            //LOG_INFO("mapped_pair_handler send [write_fd:%d] [nsend:%d]", write_fd, nsend);

            if (nsend == -1) {
                
                if (errno == EAGAIN) {
                    //LOG_ERR("send error.[errno:%d] [error:%s]", errno, strerror(errno));
                    continue;
                }
                LOG_ERR("send error.[errno:%d] [error:%s]", errno, strerror(errno));
                destroy_mapped_pair(write_fd, pmp);
                return -1; 
            }
            p = p + nsend;
            nlen -= nsend;
        }
    }while(nrecv > 0);

    return 0;
}


int
init_local_server(unsigned int port, unsigned int lower_port, unsigned int high_port)
{

    int sockfd;
    sockfd = create_server(port);
    struct local_server *pls;

    if (sockfd < 0)
        return -1;
    pls = malloc(sizeof (struct local_server));
    pls->sockfd = sockfd;
    pls->listen_port = port;
    pls->accepted_num = 0;
    pls->lower_port = lower_port;
    pls->high_port = high_port;

    memset(pls->used_ports, 0, sizeof(pls->used_ports));
    return register_event(sockfd, local_server_handler, pls);
}
