#include <netinet/in.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/types.h>

#define RECVLINE 128
#define LISTENQ 3

int main(int argc, char** argv)
{
    int sockfd, n;
    char buf[RECVLINE];
    struct sockaddr_in servaddr;

    
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("error socket : %s\n", strerror(errno));
        return -1;
    }
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(9999);
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	/*
   if (inet_aton(argv[1], &servaddr.sin_addr) <= 0) {
	fprintf(stderr, "inet_aton error:%s\n", strerror(errno));
	return -1;
   }
    */
   int on = 1;
   if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on))) {
	fprintf(stderr, "setsockopt error:%s\n", strerror(errno));
	return -1;
   }

   if (bind(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        printf("bind error: %s\n", strerror(errno));
        close(sockfd);
        return -1;
   }

   struct sockaddr_in addr;
   socklen_t addrlen = sizeof(addr);
   if (getsockname(sockfd, (struct sockaddr*)&addr, &addrlen) == -1) {
	fprintf(stderr, "err:%s\n", strerror(errno));

   }   else {
	printf("addr=%s, port=%d\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
   }
   if (listen(sockfd, LISTENQ) < 0) {
        printf("listen error;%s\n", strerror(errno));
        close(sockfd);
        return -1;
   }

   for (;;) {
        int connfd = accept(sockfd, (struct sockaddr*)NULL, NULL);
        if (connfd < 0) {
            printf("accept error:%s\n", strerror(errno));
            close(sockfd);
            return -1;
        }
	
	struct sockaddr_in connaddr;
	socklen_t connlen;
	
	if (getsockname(connfd, (struct sockaddr*)&connaddr, &connlen) < 0) {
		fprintf(stderr, "getsockaddr error:%s\n", strerror(errno));
		return -1;
	}
	char tmp[128];
	printf("addr:%s\n", inet_ntop(AF_INET, &connaddr.sin_addr, tmp, sizeof(tmp)));
        time_t ticks = time(NULL);
        snprintf(buf, RECVLINE, "%.24s\n", ctime(&ticks));
        write(connfd, buf, strlen(buf));
        close(connfd);
   }
    return 0;
}
