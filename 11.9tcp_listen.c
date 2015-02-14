#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>

#define LISTENQ 3
#define MAXLINE 128

int tcp_listen(const char *host, const char *serv, socklen_t *addrlenp)
{
	int listenfd, n;
	const int on = 1;
	struct addrinfo hints, *res, *ressave;
	
	bzero(&hints, sizeof(hints));
	hints.ai_flags = AI_PASSIVE;	
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;	
	
	if ((n = getaddrinfo(host, serv, &hints, &res)) != 0) {
		printf("tcp_listen error for %s, %s: %s\n", host, serv, gai_strerror(n));
		return -1;
	}

	ressave = res;
	do {
		listenfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
		if (listenfd < 0) continue;
		if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0) {
			close(listenfd);
			printf("setsockopt error:%s\n", strerror(errno));
			continue;
		}
		if (bind(listenfd, res->ai_addr, res->ai_addrlen) == 0) break;
		close(listenfd);
	} while ((res = res->ai_next) != NULL);

	if (res == NULL) {
		printf("tcp_listen error for %s, %s\n", host, serv);
		return -1;
	}

	if (listen(listenfd, LISTENQ) < 0) {
		printf("listen error:%s\n", strerror(errno));
		close(listenfd);
		return -1;
	}

	if (addrlenp) *addrlenp =  res->ai_addrlen;
	
	freeaddrinfo(ressave);
	return listenfd;
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
			}
			return str;
		}
	}

	return NULL;
}

int main(int argc, char **argv)
{
	int listenfd, connfd;
	socklen_t addrlen, len;
	char buff[MAXLINE + 1];
	time_t ticks;
	struct sockaddr *cliaddr;
	
	if (argc != 2) {
		printf("usage: exename <service or port>\n");
		return -1;
	}	

	listenfd = tcp_listen(NULL, argv[1], &addrlen);
	if (listenfd < 0) return -1;
	cliaddr = malloc(addrlen);
	
	
	for (;;) {
		len = addrlen;
		connfd = accept(listenfd, cliaddr, &len);
		printf("connection from :%s\n", sock_ntop(cliaddr, len));
		ticks = time(NULL);
		snprintf(buff, sizeof(buff), "%.24s\r\n", ctime(&ticks));
		write(connfd, buff, strlen(buff));	
		close(connfd);
	}
	return 0;
}
