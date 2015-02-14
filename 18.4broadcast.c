#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <netinet/in.h>
#include <signal.h>

#define SERV_PORT 9999
#define BUF_LEN 128

static void recvfrom_alarm(int signo)
{

}


char *sock_ntop(struct sockaddr*sa, socklen_t len)
{
	char portstr[7];
	static char str[128];

	switch(sa->sa_family) {
		case AF_INET: 
		{
			struct sockaddr_in *sin = (struct sockaddr_in*)sa;
			if (inet_ntop(AF_INET, &sin->sin_addr, str, sizeof(str)) == NULL) return NULL;
			if (ntohs(sin->sin_port) != 0) {
				snprintf(portstr, sizeof(portstr), ".%d", ntohs(sin->sin_port));
				strcat(str, portstr);
			}

			return str;
		}
	}

	return NULL;
}

void dg_cli(FILE *fp, int sockfd, const struct sockaddr_in * pservaddr, socklen_t servlen)
{
	int n;
	char sendline[BUF_LEN], recvline[BUF_LEN];
	const int on = 1;
	socklen_t len;
	struct sockaddr *preply_addr;
	
	preply_addr = malloc(servlen);
	if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on)) < 0) {
		fprintf(stderr, "setsockopt broadcast error:%s\n", strerror(errno));
	}	

	signal(SIGALRM, recvfrom_alarm);
		
	while (fgets(sendline, BUF_LEN, fp) != NULL) {
		sendto(sockfd, sendline, strlen(sendline), 0, (struct sockaddr*)pservaddr, servlen);	
		alarm(5);
		for (;;) {
			len = servlen;
			n = recvfrom(sockfd, recvline, BUF_LEN, 0, preply_addr, &len);
			if (n < 0) {
				if (errno == EINTR) break;
				else fprintf(stderr, "recvfrom error:%s\n", strerror(errno));
			} else {
				recvline[n] = 0;
			}
		}
		//n = recvfrom(sockfd, recvline, BUF_LEN, 0, NULL, NULL);
		//recvline[n] = 0;
		//fputs(recvline, stdout);
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
