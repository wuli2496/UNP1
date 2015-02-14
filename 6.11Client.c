#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#define SIN_PORT 9999
#define BUF_LEN 256
#define max(a, b) ((a) > (b) ? (a) : (b))

size_t readline(int fd, void *ptr, int maxsize)
{
    char *p = ptr;
    char c;
    int rc;
    int n;
    
    for (n = 1; n < maxsize; n++) {
    again:
        if ((rc = read(fd, &c, 1)) == 1) {
              *p++ = c;
              if (c == '\n') break; 
        } else if (rc == 0) {
                    if (n == 1) return 0;
                       else break;
                       }else {
            if (errno == EINTR) goto again;
            else return -1;
        }
    }
    
    *p = 0;
    return n;
}


size_t readn(int fd, void *ptr, size_t n)
{
    char *p = ptr;
    int nleft = n;
    int nread;
    
    while (nleft > 0) {
          if ((nread = read(fd, p, nleft)) < 0) {
                if (errno == EINTR) nread = 0;
                else return -1;
          } else if (nread == 0) break;
          nleft -= nread;
          p += nread;
    }
    
    return n - nleft;
}

size_t writen(int fd, void *ptr, size_t n)
{
    size_t nwriten;
    size_t nleft = n;
    char *p = ptr;
    
    while (nleft > 0) {
          if ((nwriten = write(fd, p, nleft)) <= 0) {
                if (errno == EINTR) nwriten = 0;
                else return -1;
          }
          
          nleft -= nwriten;
          p += nwriten;
    }
    
    return n;
}

void str_cli(FILE *file, int fd)
{
    char recvline[BUF_LEN], sendline[BUF_LEN];
    int maxfdp1;
    fd_set fdset;
    int ret;

    int stdineof = 0;

    FD_ZERO(&fdset);

    for(;;) {
        if (stdineof == 0)
            FD_SET(fileno(file), &fdset);
        FD_SET(fd, &fdset);
        maxfdp1 = max(fileno(file), fd) + 1;
        ret = select(maxfdp1, &fdset, NULL, NULL, NULL);   
        if (ret <= 0) {
             continue;
        }
        //printf("ret=%d\n", ret);
        if (FD_ISSET(fd, &fdset)) {
            if (readline(fd, recvline, BUF_LEN) == 0) {
                if (stdineof == 1)
                    return;
                else {
                    printf("readline error:%s\n", strerror(errno));
                    return;
                }
            }
            fputs(recvline, stdout);
        } 
    
        if (FD_ISSET(fileno(file), &fdset)) {
            memset(sendline, 0x00, sizeof(sendline));
            if (fgets(sendline, BUF_LEN, file) == NULL) {
                //printf("fgets error:%s\n", strerror(errno));
                stdineof = 1;
                shutdown(fd, SHUT_WR);
                FD_CLR(fileno(file), &fdset);
                continue;
            }
            
            //printf("len=%d\n", strlen(sendline));
            writen(fd, sendline, strlen(sendline));
        }   
    }
}

int main(int argc, char** argv)
{
    struct sockaddr_in serveraddr;
    int sockfd;
    
#ifndef ONLINE_JUDGE
    //freopen("6.4Client.c", "r", stdin);
#endif

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        printf("socket error:%s\n", strerror(errno));
        return -1;
    }   
    
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(SIN_PORT);
    if (inet_pton(AF_INET, argv[1], &serveraddr.sin_addr) != 1) {
          printf("inet_pton error:%s\n", strerror(errno));
          close(sockfd);
          return -1;
    }
    
    if (connect(sockfd, (struct sockaddr*)&serveraddr, sizeof(serveraddr)) < 0) {
        printf("connect error:%s\n", strerror(errno));
        close(sockfd);
        return -1;
    }
    
    str_cli(stdin, sockfd);
    
    exit(0);
    
}
