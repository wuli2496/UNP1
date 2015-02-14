#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#define SERV_PORT 9999
#define BUF_LEN 128

void dg_echo(int sockfd, struct sockaddr_in *pcliaddr, socklen_t clilen)
{
	char buf[BUF_LEN];	
	socklen_t len;
	int n;
	int cnt = 0;

	for (;;) {
		len = clilen;
		if ((n = recvfrom(sockfd, buf, BUF_LEN, 0, (struct sockaddr*)pcliaddr, &clilen)) < 0) {
			printf("recvfrom error:%s\n", strerror(errno));
			//printf("cnt=%d\n", cnt);
			continue;
		} else if (n == 0) {
			printf("cnt=%d\n", cnt);
		}
		cnt++;
		printf("cnt=%d\n", cnt);
		sendto(sockfd, buf, n, 0, (struct sockaddr*)pcliaddr, len);
		
	}	

//	printf("cnt=%d\n", cnt);
}

int main(int argc, char **argv)
{
	struct sockaddr_in servaddr, clientaddr;
	int sockfd;

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);	
	if (sockfd < 0) {
		printf("socket error:%s\n", strerror(errno));
		return -1;
	}

	memset(&servaddr, 0x00, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(SERV_PORT);
	
	 int n = 6;
        socklen_t len = sizeof(n);
        getsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &n, &len);
        printf("n=%d\n", n);
        n = 1;
        setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &n, len);
        getsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &n, &len);
        printf("n2=%d\n", n);
	
	if (bind(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
		printf("bind error:%s\n", strerror(errno));
		close(sockfd);	
		return -1;
	}
	
	dg_echo(sockfd, &clientaddr, sizeof(clientaddr));
	return 0;
}
