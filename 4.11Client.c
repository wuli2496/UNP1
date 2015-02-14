#include <netinet/in.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>

#define RECVLINE 128

int main(int argc, char** argv)
{
    int sockfd, n;
    char buf[RECVLINE];
    struct sockaddr_in servaddr;

    if (argc != 2) {
        printf("usage: a.out <IPADDRESS>\n");
        return -1;
    }
    
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("error socket : %s\n", strerror(errno));
        return -1;
    }
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(13);
   if (inet_pton(AF_INET, argv[1], &(servaddr.sin_addr)) <= 0) {
       printf("error inet_pton: %s\n", strerror(errno));
       close(sockfd);
       return -1;
    }

   if (connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        printf("connect error: %s\n", strerror(errno));
        close(sockfd);
        return -1;
   }
    struct sockaddr_in tmp;
    int len;
    getsockname(sockfd, (struct sockaddr*)&tmp, &len);
    printf("addr=%s, port=%d\n", inet_ntoa(tmp.sin_addr), ntohs(tmp.sin_port)) ;    
   int cnt = 0;
    while ((n = read(sockfd, buf, RECVLINE)) > 0) {
        buf[n] = 0;
        cnt++;
        if (fputs(buf, stdout) == EOF) {
            printf("fputs error:%s\n", strerror(errno));
            close(sockfd);
            return -1;
        }
    }
    //printf("cnt=%d n=%d\n", cnt, n);
    if (n < 0) {
        printf("read error:%s\n", strerror(errno));
    }

    exit(0);
    return 0;
}
