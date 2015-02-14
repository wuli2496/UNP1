#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define SIN_PORT 9999
#define BUFLEN 256

int main(int argc, char **argv)
{
    int listenfd, connfd;
    struct sockaddr_in servaddr;
    char buf[BUFLEN];
    time_t ticks;

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0) {
        printf("socket error :%s\n", strerror(errno));        
        return -1;
    }

    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(SIN_PORT);
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    
    
    if (bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        printf("bind error:%s\n", strerror(errno));
        close(listenfd);
        return -1;
    }
    
    
    if (listen(listenfd, 3) < 0) {
        printf("listen error:%s\n", strerror(errno));
        close(listenfd);
        return -1;
    }

    
    

    struct sockaddr_in clientaddr;
    int len;

    for (;;) {
        len = sizeof(clientaddr);
        connfd = accept(listenfd, (struct sockaddr*)&clientaddr, &len);
        if (connfd < 0) {
            printf("accept error:%s\n", strerror(errno));
            close(listenfd);
            return -1;
        }
        printf("connection from %s, port:%d\n", inet_ntop(AF_INET, &clientaddr.sin_addr, buf, sizeof(buf)), htons(clientaddr.sin_port));
        ticks = time(NULL);
        snprintf(buf, sizeof(buf), "%.24s\r\n", ctime(&ticks));
        write(connfd, buf, strlen(buf));
        close(connfd);
    }

    return 0;
}
