#include <sys/socket.h>
#include <sys/types.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <net/if_arp.h>
#include <sys/utsname.h>
#include <sys/ioctl.h>

char **my_addrs(int *addtype);

int main(int argc, char **argv)
{
	int family, sockfd;
	char str[INET6_ADDRSTRLEN];
	char **pptr;
	unsigned char *ptr;
	struct arpreq arpreq;
	struct sockaddr_in *sin;
	
	pptr = my_addrs(&family);
	for (; *pptr != NULL; pptr++) {
		printf("%s : ", inet_ntop(family, *pptr, str, sizeof(str)));
		switch (family) {
		case AF_INET:	
			sockfd = socket(AF_INET, SOCK_DGRAM, 0);
			sin = (struct sockaddr_in*)&arpreq.arp_pa;
			bzero(sin, sizeof(struct sockaddr_in));
			sin->sin_family = AF_INET;
			memcpy(&sin->sin_addr, *pptr, sizeof(struct in_addr));
			ioctl(sockfd, SIOCGARP, &arpreq);
			ptr = &arpreq.arp_ha.sa_data[0];
			printf("%x:%x:%x:%x:%x:%x\n", *ptr, *(ptr + 1), *(ptr + 2), *(ptr + 3), *(ptr + 4), *(ptr + 5));
			break;
		default:
			printf("unsupported address family:%d\n", family);
		}
	}

	return 0;
}


char **my_addrs(int *addrtype)
{
	struct hostent *hptr;
	struct utsname myname;

	if (uname(&myname) < 0) return NULL;
		
	if ((hptr = gethostbyname(myname.nodename)) == NULL) return NULL;

	*addrtype = hptr->h_addrtype;
	return hptr->h_addr_list;
}
