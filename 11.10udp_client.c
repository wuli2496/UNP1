#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>


#define MAXLINE 128

int udp_client(const char *host, const char *serv, void **saptr, socklen_t *lenp)
{
	int sockfd;
	int n;
	struct addrinfo hints, *res, *ressave;
	
	bzero(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;	
	hints.ai_socktype = SOCK_DGRAM;	
	
	if ((n = getaddrinfo(host, serv, &hints, &res)) != 0) {
		printf("udp_client error for %s, %s:%s\n", host, serv, gai_strerror(n));
		return -1;
	}

	ressave = res;
	do {
		sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
		if (sockfd >= 0) break;
	} while ((res = res->ai_next) != NULL);

	if (res == NULL) {
		printf("udp_client error for %s, %s\n", host, serv);
		return -1;
	}

	*saptr = malloc(res->ai_addrlen);
	memcpy(*saptr, res->ai_addr, sizeof(res->ai_addrlen));
	*lenp = res->ai_addrlen;

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

	sockfd = udp_client(argv[1], argv[2], (void **)&sa, &salen);
	printf("sending to %s\n", sock_ntop(sa, salen));
	
	sendto(sockfd, " ", 1, 0, sa, salen);
	n = recvfrom(sockfd, recvline, MAXLINE, 0, NULL, NULL);
	recvline[n] = 0;
	fputs(recvline, stdout);
	return 0;
}
