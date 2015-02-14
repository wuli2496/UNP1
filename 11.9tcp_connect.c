#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define MAXLINE 128
#define MAXSOCKADDR sizeof(struct sockaddr)

struct addrinfo *host_serv(char *host, const char *serv, int family, int socktype)
{
	int n;
	struct addrinfo hints, *res;
	
	bzero(&hints, sizeof(hints));
	hints.ai_flags = AI_CANONNAME;
	hints.ai_family = family;
	hints.ai_socktype = socktype;

	if ((n = getaddrinfo(host, serv, &hints, &res)) != 0) return NULL;

	return res;
}

int tcp_connect(const char *hostname, const char *serv)
{
	int sockfd, n;
	
	struct addrinfo hints, *res, *ressave;
	bzero(&hints, sizeof(hints));
	hints.ai_family = AF_INET;	
	hints.ai_socktype = SOCK_STREAM;

	if ((n = getaddrinfo(hostname, serv, &hints, &res)) != 0) {
		printf("tcp connect error for %s, %s:%s\n", hostname, serv, gai_strerror(n));
		return -1;
	}

	ressave = res;

	do {
		sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
		if (sockfd < 0) continue;
		
		if (connect(sockfd, res->ai_addr, res->ai_addrlen) == 0) break;
		close(sockfd);
	} while ((res = res->ai_next) != NULL);

	if (res == NULL) {
		printf("tcp_connect error for %s, %s\n", hostname, serv);
		return -1;
	}

	freeaddrinfo(ressave);
	
	return sockfd;
}

char *sock_ntop(struct sockaddr *sa, socklen_t len)
{
	char portstr[7];
	static char str[MAXLINE + 1];

	switch (sa->sa_family) {
		case AF_INET:
		{
			struct sockaddr_in *sin = (struct sockaddr_in*)sa;
			if (inet_ntop(AF_INET, &sin->sin_addr, str, sizeof(str)) == NULL) return NULL;
			
			if (ntohs(sin->sin_port) != 0) {
				snprintf(portstr, sizeof(portstr), ":%d", ntohs(sin->sin_port));
				strcat(str, portstr);
				return str;
			}
		}
	}
	
	return NULL;
}


int main(int argc, char **argv)
{
	int sockfd, n;
	char recvline[MAXLINE + 1];
	socklen_t len;
	struct sockaddr *sa;
	
	if (argc != 3) {
		printf("usage: exename <hostname/ipaddress> <service/port>\n");
		return -1;
	}

	sockfd = tcp_connect(argv[1], argv[2]);
	if (sockfd < 0) {
		//printf("tcp_connect error\n");
		return -1;
	}

	sa = malloc(MAXSOCKADDR);
	len = MAXSOCKADDR;
	
	if (getpeername(sockfd, sa, &len) < 0) {
		printf("getpeername error:%s\n", strerror(errno));
		close(sockfd);
		return -1;
	}	

	printf("connected to %s\n", sock_ntop(sa, len));
	while ((n = read(sockfd, recvline, MAXLINE)) > 0) {
		recvline[n] = 0;
		fputs(recvline, stdout);	
	}
	return 0;
}
