#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <time.h>
#include <sys/types.h>

#define SERV_PORT 9999
#define MAXLINE 128
#define MAX(a, b) ((a) < (b) ? (b) : (a))


char *gf_time()
{
	struct timeval tv;
	static char str[30];
	char *ptr;

	if (gettimeofday(&tv, NULL) < 0) {
		fprintf(stderr, "gettimeofday error:%s\n", strerror(errno));
		return NULL;
	} 

	ptr = ctime(&tv.tv_sec);
	strcpy(str, &ptr[11]);
	snprintf(str + 8, sizeof(str) - 8, ".%06ld", tv.tv_usec);

	return str;
}

void str_cli(FILE *fp, int sockfd)
{
	int maxfd1, val, stdineof;
	ssize_t n, nwritten;
	fd_set rset, wset;
	char to[MAXLINE], from[MAXLINE];
	char *toiptr, *tooptr, *friptr, *froptr;
	val = fcntl(sockfd, F_GETFL, 0);
	if (fcntl(sockfd, F_SETFL, val | O_NONBLOCK) < 0) {
		printf("fcntl error:%s\n", strerror(errno));
		close(sockfd);
		return;
	}
	
	val = fcntl(STDIN_FILENO, F_GETFL, 0);
	if (fcntl(STDIN_FILENO, F_SETFL, val | O_NONBLOCK) < 0) {
		printf("fcntl stdin error:%s\n", strerror(errno));
		close(sockfd);
		return;
	}	

	val = fcntl(STDOUT_FILENO, F_GETFL, 0);
	if (fcntl(STDOUT_FILENO, F_SETFL, val | O_NONBLOCK) < 0) {
		printf("fcntl stdout error:%s\n", strerror(errno));
		close(sockfd);
		return ;
	}

	toiptr = tooptr = to;
	friptr = froptr = from;
	stdineof = 0;

	maxfd1 = MAX(sockfd, MAX(STDIN_FILENO, STDOUT_FILENO)) + 1;
	
	for (;;) {
		FD_ZERO(&rset);
		FD_ZERO(&wset);
		
		if (stdineof == 0 && toiptr < &to[MAXLINE]) {
			FD_SET(STDIN_FILENO, &rset);
		}

		if (friptr < &from[MAXLINE]) {
			FD_SET(sockfd, &rset);
		}

		if (tooptr != toiptr) {
			FD_SET(sockfd, &wset);
		}

		if (froptr != friptr) {
			FD_SET(STDOUT_FILENO, &wset);
		}
		
		int res;

		if ((res = select(maxfd1, &rset, &wset, NULL, NULL)) < 0) {
			if (errno != EINTR) {
				close(sockfd);
				return;
			}
		}

		if (FD_ISSET(STDIN_FILENO, &rset)) {
			if ((n = read(STDIN_FILENO, toiptr, &to[MAXLINE] - toiptr)) < 0) {
				if (errno != EWOULDBLOCK) {
					printf("read error from stdin:%s\n", strerror(errno));
					close(sockfd);
					return;
				}
			} else if (n == 0) {
				fprintf(stderr, "%s:EOF on stdin\n", gf_time());
				stdineof = 1;
				if (toiptr == tooptr) shutdown(sockfd, SHUT_WR);
			} else {
				fprintf(stderr, "%s:read %d bytes from stdin\n", gf_time(), n);
				toiptr += n;
				FD_SET(sockfd, &wset);
			}
		}

		if (FD_ISSET(sockfd, &rset)) {
			if ((n = read(sockfd, friptr, &from[MAXLINE] - friptr)) < 0) {
				if (errno != EWOULDBLOCK) {
					printf("read error on socket:%s\n", strerror(errno));
					close(sockfd);
					return;
				}
			} else if (n == 0) {
				fprintf(stderr, "%s :EOF on socket\n", gf_time());
				if (stdineof) return;
			} else {
				fprintf(stderr, "%s:read %d bytes from socket\n", gf_time(), n);
				friptr += n;
				FD_SET(STDOUT_FILENO, &wset);
			}
		}

		if (FD_ISSET(STDOUT_FILENO, &wset) && ((n = friptr - froptr) > 0)) {
			if ((nwritten = write(STDOUT_FILENO, froptr, n)) < 0) {
				if (errno != EWOULDBLOCK) {
					printf("write stdout error:%s\n", strerror(errno));
					close(sockfd);
					return;
				}
			} else {
				fprintf(stderr, "%s:write %d bytes to stdout\n", gf_time(), nwritten);
				froptr += nwritten;
				if (froptr == friptr) friptr = froptr = from;
			}
		}

		if (FD_ISSET(sockfd, &wset) && ((n = toiptr - tooptr) > 0)) {
			if ((nwritten = write(sockfd, tooptr, n)) < 0) {
				if (errno != EWOULDBLOCK) {
					fprintf(stderr, "write on socket error:%s\n", strerror(errno));
					close(sockfd);
					return;
				}
			} else {
				fprintf(stderr, "%s:write %d bytes to socket\n", gf_time(), nwritten);	
				tooptr += nwritten;
				if (tooptr == toiptr) {
					toiptr = tooptr = to;
					if (stdineof) shutdown(sockfd, SHUT_WR);
				}
			}
		}

		
	}

	
}

int main(int argc, char **argv)
{
	struct sockaddr_in servaddr, cliaddr;
	socklen_t len;
	int sockfd;

	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(SERV_PORT);
	if (inet_pton(AF_INET, argv[1], &servaddr.sin_addr) < 0) {
		printf("inet_pton error:%s\n", strerror(errno));
		return -1;
	}

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		printf("socket error:%s\n", strerror(errno));
		return -1;
	}

	if (connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
		printf("connect error:%s\n", strerror(errno));	
		close(sockfd);
		return -1;
	}

	str_cli(stdin, sockfd);
	return 0;
}
