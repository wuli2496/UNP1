#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define QSIZE 8
#define MAXDG 4096

static int sockfd;
typedef struct {
	void *dg_data;
	size_t dg_len;
	struct sockaddr *dg_sa;
	socklen_t da_salen;
}DG;


static DG dg[QSIZE];
static long cntread[QSIZE + 1];
static int iget;
static int iput;
static int nqueue;
static socklen_t clilen;

static void sig_io(int sig_no)
{
	ssize_t len;
	int nread;
	DG *ptr;
	
	for (nread = 0; ;) {
		if (nqueue >= QSIZE) break;
		ptr = &dg[iput];
		ptr->dg_salen = clilen;
		len = recvfrom(sockfd, ptr->dg_data, MAXDG, 0, ptr->dg_sa, &ptr->dg_salen);
		if (len < 0) {
			if (errno == EWOULDBLOCK) break;
			else fprintf(stderr, "err:%s\n", strerror(errno));
		}

		ptr->dg_len = len;
		nread++;
		nqueue++;
		if (++iput > QSIZE) iput = 0;
	}
	cntread[nread]++;
}

static void sig_hup(int signo)
{
	int i;
	for (i = 0; i <= QSIZE; i++) {
		printf("cntread[%d]=%d\n", i, cntread[i]);
	}
}

void dg_echo(int sockfd_arg, struct sockaddr* pcliaddr, socklen_t clienlen)
{
	const int on = 1;
	sigset_t zeromask, newmask, oldmask;
	sockfd = sockfd_arg;
	clilen = clienlen;
	int i;

	for (i = 0; i < QSIZE; i++) {
		dg[i].dg_data = malloc(MAXDG);
		dg[i].dg_sa = malloc(clilen);
		dg[i].dg_salen = clilen;
	}

	iget = iput = nqueue = 0;

	signal(SIGIO, sig_io);
	signal(SIGHUP, sig_hup);
	fcntl(sockfd, F_SETOWN, getpid());
	ioctl(sockfd, FIOASYNC, &on);
	ioctl(sockfd, FIONBIO, &on);

	sigemptyset(&zeromask);
	sigemptyset(&newmask);
	sigemptyset(&oldmask);
	sigaddset(&newmask, SIGIO);
	
	sigprocmask(SIG_BLOCK, &newmask, &oldmask);
	
	while (1) {
		while (nqueue == 0) sigsuspend(&zeromask);
		sigprocmask(SIG_SETMASK, &oldmask, NULL);
		sendto(sockfd, dg[iget].dg_data, dg[iget].dg_len, 0, dg[iget].dg_sa, dg[iget].dg_salen);
		if (++iget > QSIZE) iget = 0;
		sigprocmask(SIG_BLOCK, &newmask, &oldmask);
		nqueue--;
	}
}	

int main(int argc, char **argv)
{
	
	eixt(0);
}

