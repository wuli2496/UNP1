#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>

#define SIN_PORT 9999
#define BUFLEN 256

void str_echo(int fd);

void sig_child(int signo)
{
    pid_t pid;
    int stat;

    while ((pid = waitpid(-1, &stat, WNOHANG)) > 0) {
        printf("pid %d terminated\n", pid);
    }

}

int main(int argc, char **argv)
{
    int listenfd, connfd;
    pid_t child;
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
    signal(SIGCHLD, sig_child);
    
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
        if ((child = fork()) == 0) {
            close(listenfd);
            str_echo(connfd);
              return 0;
        }
        
        close(connfd);
        
    }

    return 0;
}

size_t readn(int fd, void *ptr, size_t n)
{
    char *p = ptr;
    size_t nleft = n;
    size_t nread;

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

size_t readline(int fd, void *ptr, size_t maxsize)
{
    char *p = ptr;
    size_t rc, n;
    char c;

    for (n = 1; n < maxsize; n++) {
again:
        if ((rc = read(fd, &c, 1)) == 1) {
            *p++ = c;
            if (c == '\n') break;
            } else if (rc == 0) {
                if (n == 1) return 0;
                else break;
        } else {
            if (errno == EINTR) goto again;
            return -1;
        }
    }

    *p = 0;
    return n;
}

size_t writen(int fd, void *ptr, size_t n)
{
    char *p = ptr;
    size_t nleft = n,  nwriten;

    while (nleft > 0) {
        if ((nwriten = write(fd, p, nleft)) <= 0) {
            if (errno == EINTR) nwriten = 0;
            else return -1;
        }

        p += nwriten;
        nleft -= nwriten;
    }

    return n;
}

void str_echo(int fd)
{
    char recvline[BUFLEN];
    int n;
    
    for (;;) {
        if ((n = readline(fd, recvline, BUFLEN)) == 0) return;
         printf("received buf=%s", recvline);
        writen(fd, recvline, n);

    }
}
