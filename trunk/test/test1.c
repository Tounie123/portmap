#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <unistd.h>

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
        return -1;
    }
    success = 0; 
    for (res0 = res; res0 != NULL; res0 = res0->ai_next) {
    
        sockfd = socket(res0->ai_family, res0->ai_socktype, res0->ai_protocol);

        if (sockfd < 0) {
        
            continue;
        }

        ret = connect(sockfd, res0->ai_addr, res0->ai_addrlen);

        if (ret < 0) {
        
            close(sockfd);
            continue;
        }
        success = 1;
        break;
    }
    freeaddrinfo(res);
    return success == 1 ? sockfd : -1;
}


int
main(int argc, char **argv)
{

    const char *addr = "10.60.1.124";
    unsigned int port = 5903;

    const char *request = "10.60.1.124:5905";
    char response[100];

    int sockfd ,nwrite, nread, nlen;
    sockfd = connect_to(addr, port);

    if (sockfd == -1) {
    
        fprintf(stderr, "connect to %s:%u failed.\n", addr, port);
        return -1;
    }
    
    nlen = strlen(request);
    nwrite = write(sockfd, request, nlen); 

    if (nwrite == -1) {
    
        fprintf(stderr, "write error [error:%s].\n", strerror(errno));
        return -1;
    }

    fprintf(stdout, "write success [request:%s] [nlen:%d] [nwrite:%d].\n", request, nlen, nwrite);

    nread = read(sockfd, response, 100);

    if (nread == -1) {
    
        fprintf(stdout, "read error [error:%s].\n", strerror(errno));
        return -1;
    }

    fprintf(stdout, "read success [response:%s] [nread:%d].\n", response, nread);

    return 0;


}
