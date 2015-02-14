#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <sys/time.h>
#include <signal.h>

#define SER_PORT 9999
#define BUFLEN 128
#define max(a, b) ((a) < (b) ? (b):(a))


void str_echo(int);

void signal_chld(int signo)
{
	pid_t pid;
	int status;

	while ((pid = waitpid(-1, &status, WNOHANG)) < 0) {
		printf("child pid:%d terminated\n", pid);
	}

	return;
}

ssize_t readline(int fd, void *ptr, int maxsize)
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


ssize_t readn(int fd, void *ptr, size_t n)
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

ssize_t writen(int fd, void *ptr, size_t n)
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

int main(int argc, char **argv)
{
	struct sockaddr_in servaddr, clientaddr;
	int listenfd, connfd, udpfd;
	int on = 1;
	pid_t child;
	fd_set rset;
	int maxfd, nready;
	int len;
	int n;
	char recvline[BUFLEN];
	
	memset(&servaddr, 0x00, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(SER_PORT);
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	if (listenfd < 0) {
		printf("socket error:%s\n", strerror(errno));
		return -1;
	}
	
	if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0) {
		printf("setsockopt error:%s\n", strerror(errno));
		close(listenfd);
		return -1;
	}

	if (bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr))) {
		printf("bind error:%s\n", strerror(errno));
		close(listenfd);
		return -1;
	}
	
	if (listen(listenfd, 100) < 0) {
		printf("listen error:%s\n", strerror(errno));
		close(listenfd);
		return -1;
	}

	
	udpfd = socket(AF_INET, SOCK_DGRAM, 0);	
	if (udpfd < 0) {
		printf("socket error:%s\n", strerror(errno));
		close(listenfd);	
		return -1;
	}
	
	memset(&servaddr, 0x00, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(SER_PORT);	
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);	
	if (bind(udpfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
		printf("bind updfd error:%s\n", strerror(errno));
		close(listenfd);	
		close(udpfd);	
		return -1;
	}	
	
	
	signal(SIGCHLD, signal_chld);
	
	maxfd = max(listenfd, udpfd) + 1;

	FD_ZERO(&rset);	
	for (;;) {
		FD_SET(listenfd, &rset);	
		FD_SET(udpfd, &rset);	
		nready = select(maxfd, &rset, NULL, NULL, NULL);
		if (nready <= 0) {
			if (errno == EINTR) continue;
			else {
				printf("select error:%s\n", strerror(errno));
				break;
			}
		}		

		if (FD_ISSET(udpfd, &rset)) {
			len = sizeof(clientaddr);
			n = recvfrom(udpfd, recvline, BUFLEN, 0, (struct sockaddr*)&clientaddr, &len);
			
			recvline[n] = 0;
			printf("recvline=%s\n", recvline);
			sendto(udpfd, recvline, strlen(recvline), 0, (struct sockaddr*)&clientaddr, len);			
		}
			
		if (FD_ISSET(listenfd, &rset)) {
			connfd = accept(listenfd, (struct sockaddr*)&clientaddr, &len);
			if ((child = fork()) == 0) {
				close(listenfd);
				str_echo(connfd);
				exit(0);
			} 

			close(connfd);
		}
		
		
			
	}	


	
}

void str_echo(int fd)
{
	int n;
	char recvline[BUFLEN];
	
	for (;;) {
		 n = readline(fd, recvline, BUFLEN);
		//printf("n=%d\n", n);
		if (n ==  0) return;
		if (n > 0)writen(fd, recvline, n);
	}	
	
