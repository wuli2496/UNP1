#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define MAXLINE 128

int udp_connect(const char *host, const char *serv)
{
	int sockfd, n;
	struct addrinfo hints, *res, *ressave;

	bzero(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;	
	hints.ai_socktype = SOCK_DGRAM;	

	if ((n = getaddrinfo(host, serv, &hints, &res)) != 0) {
		printf("udp_connect error for %s, %s:%s\n", host, serv, gai_strerror(errno));
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
		printf("udp_connect error for %s, %s\n", host, serv);
		return -1;
	}

	freeaddrinfo(ressave);
	
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
	int sockfd, n;
	char recvline[MAXLINE + 1];
	socklen_t salen;
	struct sockaddr *sa;
	
	if (argc != 3) {
		printf("usage:exename <hostname/ipaddress> <sevice/port>\n");
		return -1;
	}

	sockfd = udp_connect(argv[1], argv[2]);
	//printf("sending to %s\n", sock_ntop(sa, salen));
	
	send(sockfd, " ", 1, 0);
	n = recv(sockfd, recvline, MAXLINE, 0);
	recvline[n] = 0;
	fputs(recvline, stdout);
	return 0;
}
