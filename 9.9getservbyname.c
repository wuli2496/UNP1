#include <netdb.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAXLINE 128


char *sock_ntop(const struct sockaddr* sa, socklen_t salen)
{
	char portstr[7];
	static char str[MAXLINE];

	switch (sa->sa_family) {
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
}

int main(int argc, char **argv)
{
	int sockfd, n;
	char recvline[MAXLINE + 1];
	struct sockaddr_in servaddr;
	struct in_addr **pptr;
	struct hostent *hp;
	struct servent *sp;
	
	if (argc != 3) {
		printf("usage: execname <hostname> <service> \n");
		return -1;
	}
	
	if ((hp = gethostbyname(argv[1])) == NULL) {
		printf("hostname error for %s:%s\n", argv[1],  hstrerror(h_errno));
		return -1;
	}

	if ((sp = getservbyname(argv[2], "tcp")) == NULL) {
		printf("getservbyname error for %s:%s\n", argv[2], hstrerror(h_errno));
		return -1;
	}

	pptr = (struct in_addr**)hp->h_addr_list;
	for (; *pptr != NULL; pptr++) {
		sockfd = socket(AF_INET, SOCK_STREAM, 0);
		if (sockfd < 0) {
			printf("socket error:%s\n", strerror(errno));
			return -1;
		}

		bzero(&servaddr, sizeof(servaddr));
		servaddr.sin_family = AF_INET;
		servaddr.sin_port = sp->s_port;
		memcpy(&servaddr.sin_addr, *pptr, sizeof(struct in_addr));
		printf("addr=%s, port=%d\n", inet_ntoa(**pptr), ntohs(sp->s_port));
		printf("trying %s\n", sock_ntop((struct sockaddr*)&servaddr, sizeof(servaddr)));
		if (connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) == 0) break;
		printf("connect error:%s\n", strerror(errno));
		close(sockfd);
	}	

	if (*pptr == NULL) {
		printf("unable to connect\n");
		return -1;
	}

	while ((n = read(sockfd, recvline, MAXLINE)) > 0) {
		recvline[n] = 0;
		fputs(recvline, stdout);
	}
	return 0;
}
