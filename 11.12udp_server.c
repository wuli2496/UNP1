#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MAXLINE 128

int udp_server(const char *host, const char *serv, socklen_t *addrlenp)
{
	int sockfd, n;
	struct addrinfo hints, *res, *ressave;

	bzero(&hints, sizeof(hints));	
	hints.ai_flags = AI_PASSIVE;	
	hints.ai_family = AF_UNSPEC;	
	hints.ai_socktype = SOCK_DGRAM;	

	if ((n = getaddrinfo(host, serv, &hints, &res)) != 0) {
		printf("udp_server error for %s,%s:%s\n", host, serv, gai_strerror(n));
		return -1;
	}

	ressave = res;

	do {
		sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
		if (sockfd < 0) continue;

		if (bind(sockfd, res->ai_addr, res->ai_addrlen) == 0) break;

		close(sockfd);
	} while ((res = res->ai_next) != NULL);

	if (res == NULL) {
		printf("udp_server error for %s,%s\n", host, serv);
		return -1;
	}

	freeaddrinfo(ressave);
	if (addrlenp) *addrlenp = res->ai_addrlen;
	return sockfd;
	
}

char *sock_ntop(const struct sockaddr* sa, socklen_t len)
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
	int sockfd;
	ssize_t n;
	char buff[MAXLINE];
	time_t ticks;
	socklen_t addrlen, len;
	struct sockaddr *cliaddr;

	if (argc == 2) sockfd = udp_server(NULL, argv[1], &addrlen);
	else if (argc == 3) sockfd = udp_server(argv[1], argv[2], &addrlen);
	else {
		printf("usage:exename [<host>] <service or port>\n");
		return -1;
	}

	cliaddr = malloc(addrlen);
	for (;;) {
		len = addrlen;
		n = recvfrom(sockfd, buff, MAXLINE, 0, cliaddr, &len);
		printf("datagram form %s\n", sock_ntop(cliaddr, len));
		ticks = time(NULL);
		snprintf(buff, sizeof(buff), "%.24s\r\n", ctime(&ticks));
		sendto(sockfd, buff, strlen(buff), 0, cliaddr, len);
	}
}
