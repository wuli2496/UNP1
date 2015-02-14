#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netdb.h>
#include <signal.h>
#include <sys/time.h>
#include <stdio.h>

#define BUFSIZE 1500

char recvbuf[BUFSIZE];
char sendbuf[BUFSIZE];
//int datalen;
char *host;
int nsent;
pid_t pid;
int sockfd;
int verbose;

void proc_v4(char *, ssize_t, struct timeval *);
//void proc_v6(char *, ssize_t, struct timeval *);
void send_v4();
//void send_v6();
void readloop();
void sig_alrm(int);
void tv_sub(struct timeval*, struct timeval*);

struct proto
{
	void (*fproc)(char *, ssize_t, struct timeval*);
	void (*fsend)(void);
	struct sockaddr *sasend;
	struct sockaddr *sarecv;
	socklen_t salen;
	int icmpproto;
}*pr;

char *sock_ntop(struct sockaddr *sa, socklen_t len)
{
	char portstr[7];	
	static char str[128];
	
	switch (sa->sa_family) {
	case AF_INET:
	{
		struct sockaddr_in *sin = (struct sockaddr_in*)sa;
		if (inet_ntop(AF_INET, &sin->sin_addr, str, sizeof(str)) == NULL) return NULL;
		if (ntohs(sin->sin_port) != 0) {
			snprintf(portstr, sizeof(portstr), "port=%d", ntohs(sin->sin_port));	
			strcat(str, portstr);
		}

		return str;
	}
	}	
}

struct addrinfo *host_serv(const char *host, const char *serv, int family, int socktype)
{
	struct addrinfo hints, *res;
	int n;
		
	bzero(&hints, sizeof(hints));
	hints.ai_flags = AI_CANONNAME;
	hints.ai_family = family;
	hints.ai_socktype = socktype;


	if ((n = getaddrinfo(host, serv, &hints, &res)) != 0) return NULL;

	return res;
}

struct proto proto_v4 = {proc_v4, send_v4, NULL, NULL, 0, IPPROTO_ICMP};
int datalen = 56;

int main(int argc, char **argv)
{
	int c;
	struct addrinfo *ai;
	
	pid = getpid();
	signal(SIGALRM, sig_alrm);

	ai = host_serv(argv[1], NULL, 0, 0);
	pr = &proto_v4;

	printf("ICMP_ECHO=%d\n", ICMP_ECHO);
	pr->sasend = ai->ai_addr;
	pr->sarecv = calloc(1, ai->ai_addrlen);
	pr->salen = ai->ai_addrlen;

	readloop();	
	exit(0);
}

void readloop(void)
{
	int size;
	char recvbuf[BUFSIZE];
	socklen_t len;
	ssize_t n;
	struct timeval tval;
	
	sockfd = socket(pr->sasend->sa_family, SOCK_RAW, pr->icmpproto);
	setuid(getuid());
	size = 60 * 1024;
	setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size));
	sig_alrm(SIGALRM);

	for (;;)
 	{
		len = pr->salen;	
		n = recvfrom(sockfd, recvbuf, sizeof(recvbuf), 0, pr->sarecv, &len);
		if (n < 0) {
			if (errno == EINTR) continue;
			else {
				printf("recvfrom error:%s\n", strerror(errno));
				return;
			}
		}

		gettimeofday(&tval, NULL);
		(*pr->fproc)(recvbuf, n, &tval);
	}		
}


void tv_sub(struct timeval* out, struct timeval *in)
{
	if ((out->tv_usec -= in->tv_usec) < 0) {
		--out->tv_sec;
		out->tv_usec += 1000000;
	}

	out->tv_sec -= in->tv_sec;
}

void proc_v4(char *ptr, ssize_t len, struct timeval *tvrecv)
{
	int hlen1, icmplen;
	double rtt;
	struct ip *ip;
	struct icmp *icmp;
	struct timeval *tvsend;

	ip = (struct ip*)ptr;
	hlen1 = ip->ip_hl << 2;
	icmp = (struct icmp*)(ptr + hlen1);
	if ((icmplen = len - hlen1) < 8) {
		fprintf(stderr, "icmp len error\n");
		return;
	}

	if (icmp->icmp_type == ICMP_ECHOREPLY) {
		if (icmp->icmp_id != pid) return;

		if (icmplen < 16) {
			fprintf(stderr, "icmplen (%d) < 16\n", icmplen);
			return;
		}
		
		tvsend = (struct timeval *)icmp->icmp_data;
		tv_sub(tvrecv, tvsend);
		rtt = tvrecv->tv_sec * 1000 + tvrecv->tv_usec / 1000;
		printf("%d bytes from %s:seq=%u, ttl=%d, rtt=%.3f ms\n", 
			icmplen, sock_ntop(pr->sarecv, pr->salen), icmp->icmp_seq, ip->ip_ttl, rtt); 
	} else if (verbose) {
		printf("%d bytes from %s:type=%d, code=%d\n", icmplen,
		sock_ntop(pr->sarecv, pr->salen), icmp->icmp_type, icmp->icmp_code);
	}
}

void sig_alrm(int signo)
{
	(*pr->fsend)();
	alarm(1);
	return;
}

unsigned short in_cksum(unsigned short *addr, int len)
{
	int nleft = len;
	int sum = 0;
	unsigned short *w = addr;
	unsigned short answer = 0;

	while (nleft > 1)
	{
		sum += *w++;
		nleft -= 2;
	}

	if (nleft == 1) 
	{
		*(unsigned char *)(&answer) = *(unsigned char *)w;
		sum += answer;
	}

	sum = (sum >> 16) + (sum & 0xffff);
	sum += (sum >> 16);
	answer = ~sum;
	return answer;
}

void send_v4()
{
	int len;
	struct icmp *icmp;
	icmp = (struct icmp *)sendbuf;
	icmp->icmp_type = ICMP_ECHO;
	icmp->icmp_code = 0;
	icmp->icmp_id = pid;
	icmp->icmp_seq = nsent++;
	gettimeofday((struct timeval*)icmp->icmp_data, NULL);
	len = 8 + datalen;
	icmp->icmp_cksum = 0;
	icmp->icmp_cksum = in_cksum((unsigned short*)icmp, len);
	sendto(sockfd, sendbuf, len, 0, pr->sasend, pr->salen);
}
