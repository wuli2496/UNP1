#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/select.h>
#include <sys/time.h>

#define SERV_PORT 9999
#define BUF_LEN 128


void dg_echo(int sockfd, struct sockaddr_in *pcliaddr, socklen_t clilen)
{
	char buf[BUF_LEN];	
	socklen_t len;
	int n;
	
	for (;;) {
		len = clilen;
		
		if ((n = recvfrom(sockfd, buf, BUF_LEN, 0, (struct sockaddr*)pcliaddr, &clilen)) < 0) {
			if (errno == EWOULDBLOCK)  printf("alarm timeout\n");
			printf("recvfrom error:%s\n", strerror(errno));
			continue;
		} 
		sendto(sockfd, buf, n, 0, (struct sockaddr*)pcliaddr, len);
		
	}	
}

int main(int argc, char **argv)
{
	struct sockaddr_in servaddr, clientaddr;
	int sockfd;
	struct timeval tv;

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);	
	if (sockfd < 0) {
		printf("socket error:%s\n", strerror(errno));
		return -1;
	}

	memset(&servaddr, 0x00, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(SERV_PORT);

	tv.tv_sec = 5;
	tv.tv_usec = 0;	
	if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
		printf("setsockopt error:%s\n", strerror(errno));
	}		

	if (bind(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
		printf("bind error:%s\n", strerror(errno));
		close(sockfd);	
		return -1;
	}
		
	dg_echo(sockfd, &clientaddr, sizeof(clientaddr));
	return 0;
}
