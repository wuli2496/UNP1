#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <net/if.h>
#include <net/route.h>
#include <stdio.h>
//#include <net/if_dl.h>

#define BUFLEN (sizeof(struct rt_msghdr) + 512)
#define SEQ 9999
#define SA struct sockaddr
#define ROUNDUP(a,size) (((a) & ((size) - 1)) ? (1 + ((a) | ((size) - 1))): (a))
#define NEXT_SA(ap) ap = (SA*)((caddr_t)ap + (ap->sa_len ? ROUNDUP(ap->sa_len, sizeof(unsigned long)): sizeof(unsigned long)))
#define RTAX_MAX 255

void get_rtaddrs(int addrs, SA * sa, struct sockaddr **rti_info);
char *sock_masktop(SA *sa, socklen_t salen);

char *sock_ntop(struct sockaddr* sa, socklen_t len)
{
	char portstr[7];
	static char str[128];

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

	return NULL;
}

int main(int argc, char **argv)
{
	int sockfd;
	char *buf;
	pid_t pid;
	ssize_t n;
	struct rt_msghdr *rtm;
	struct sockaddr *sa, *rtl_info[RTAX_MAX];
	struct sockaddr_in *sin;
	
	if (argc != 2) {
		fprintf(stderr, "usage:getrt<IPaddress>\n");
		return -1;
	}

	sockfd = socket(AF_ROUTE, SOCK_RAW, 0);
	if (sockfd < 0) {
		fprintf(stderr, "socket error:%s\n", strerror(errno));
		return -1;
	}

	buf = calloc(1, BUFLEN);
	rtm = (struct rt_msghdr*)buf;
	rtm->rtm_msglen = sizeof(struct rt_msghdr) + sizeof(struct sockaddr_in);
	rtm->rtm_version = RTM_VERSION;
	rtm->rtm_type = RTM_GET;	
	rtm->rtm_addrs = RTA_DST;
	rtm->rtm_pid = pid = getpid();
	rtm->rtm_seq = SEQ;

	sin = (struct sockaddr_in*)(rtm + 1);
	sin->sin_family = AF_INET;	
	inet_pton(AF_INET, argv[1], &sin->sin_addr);
	write(sockfd, rtm, rtm->rtm_msglen);	

	do {	
		n = read(sockfd, rtm, BUFLEN);
	} while (rtm->rtm_type != RTM_GET || rtm->rtm_seq != SEQ || rtm->rtm_pid != pid);

	rtm = (struct rt_msghdr*)buf;
	sa = (struct sockaddr*)(rtm + 1);
	get_rtaddrs(rtm->rtm_addrs, sa, rtl_info);
	if ((sa = rtl_info[RTAX_DST]) != NULL) printf("dest:%s\n", sock_ntop(sa, sa->sa_len));	
	
	if ((sa = rtl_info[RTAX_GATEWAY]) != NULL) printf("gateway:%s\n", sock_ntop(sa, sa->sa_len));
	
	if ((sa = rtl_info[RTAX_NETMASK]) != NULL) printf("netmask:%s\n", sock_masktop(sa, sa->sa_len));
	
	if ((sa = rtl_info[RTAX_GENMASK]) != NULL) printf("genmask:%s\n", sock_masktop(sa, sa->sa_len));


	return 0;
}

void get_rtaddrs(int addrs, SA *sa, struct sockaddr **rti_info)
{
	int i;
	for (i = 0; i < RTAX_MAX; i++) {
		if (addrs & (1 << i)) {
			rti_info[i] = sa;
			NEXT_SA(sa);
		} else rti_info[i] = NULL;
	}
}

char *sock_masktop(SA *sa, socklen_t salen)
{
	static char str[INET6_ADDRSTRLEN];
	unsigned char *ptr = &sa->sa_data[2];

	if (sa->sa_len == 0) return ("0.0.0.0");
	else if (sa->sa_len == 5) snprintf(str, sizeof(str), "%d.0.0.0", *ptr);
	else if (sa->sa_len == 6) snprintf(str, sizeof(str), "%d.%d.0.0", *ptr, *(ptr + 1));
	else if (sa->sa_len == 7) snprintf(str, sizeof(str), "%d.%d.%d.0", *ptr, *(ptr + 1), *(ptr + 2));
	else if (sa->sa_len == 8) snprintf(str, sizeof(str), "%d.%d.%d.%d", *ptr, *(ptr + 1), *(prt + 2), *(ptr + 3));
	else snprintf(str, sizeof(str), "(unknown mask, len=%d, family=%d)", sa->sa_len, sa->sa_family);
	return str;
}
