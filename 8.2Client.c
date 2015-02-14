#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <netinet/in.h>

#define SERV_PORT 9999
#define BUF_LEN 128

void dg_cli(FILE *fp, int sockfd, const struct sockaddr_in * pservaddr, socklen_t servlen)
{
	int n;
	char sendline[BUF_LEN], recvline[BUF_LEN];
	
	while (fgets(sendline, BUF_LEN, fp) != NULL) {
		sendto(sockfd, sendline, 0/*strlen(sendline)*/, 0, (struct sockaddr*)pservaddr, servlen);	
		n = recvfrom(sockfd, recvline, BUF_LEN, 0, NULL, NULL);
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
