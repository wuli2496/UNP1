#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define SERV_PORT 9999
#define BUF_LEN 128

char *sock_ntop(const struct sockaddr* sockaddr, socklen_t len)
{
	static char str[BUF_LEN];
	char portstr[7];
	struct sockaddr_in *sin;
		
	switch (sockaddr->sa_family) {
	case AF_INET:
		sin = (struct sockaddr_in*)sockaddr;
		if (inet_ntop(AF_INET, &sin->sin_addr, str, sizeof(str)) == (const char *)NULL) {
			printf("inet_ntop error:%s\n", strerror(errno));
			return NULL;
		}	
		
	
		if (ntohs(sin->sin_port) != 0) {
			snprintf(portstr, sizeof(portstr), ":%d", ntohs(sin->sin_port));
			strcat(str, portstr);
		}
		return str;
	}	

	
	return NULL;	
}

void dg_cli(FILE *fp, int sockfd, const struct sockaddr_in * pservaddr, socklen_t servlen)
{
	int n;
	char sendline[BUF_LEN], recvline[BUF_LEN];
	struct sockaddr reply_addr;
	socklen_t len;

	if (connect(sockfd, (struct sockaddr*)pservaddr, servlen) < 0) {
		printf("connect error:%s\n", strerror(errno));
		return ;
	}

	while (fgets(sendline, BUF_LEN, fp) != NULL) {
		//sendto(sockfd, sendline, strlen(sendline), 0, (struct sockaddr*)pservaddr, servlen);	
		write(sockfd, sendline, strlen(sendline));
		len = servlen;
		//n = recvfrom(sockfd, recvline, BUF_LEN, 0, &reply_addr, &len);
		n = read(sockfd, recvline, BUF_LEN);
		
		/*
		if (len != servlen || memcmp(&reply_addr, pservaddr, len) != 0) {
			printf("receive (%s) ignored\n", sock_ntop(&reply_addr, len));
			continue;
		}
		*/
		
		recvline[n] = 0;
		fputs(recvline, stdout);
	}
}

int main(int argc, char **argv)
{
	struct sockaddr_in servaddr;
	int sock;

	memset(&servaddr, 0x00, sizeof(servaddr));	
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(SERV_PORT);	
	
	if (inet_pton(AF_INET, argv[1], &servaddr.sin_addr) < 0) {
		printf("inet_pton error:%s\n", strerror(errno));
		return -1;
	}

	sock = socket(AF_INET, SOCK_DGRAM, 0);	
	if (sock < 0) {
		printf("socket error:%s\n", strerror(errno));
		return -1;
	}

	dg_cli(stdin, sock, &servaddr, sizeof(servaddr));
	return 0;
}
