#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char **argv)
{
	int sockfd;
	socklen_t len;
	struct sockaddr_un addr1, addr2;
	
	if (argc != 2) {
		printf("usage: unixbind <pathname>\n");
		return -1;
	}

	sockfd = socket(AF_LOCAL, SOCK_STREAM, 0);	
	unlink(argv[1]);
	bzero(&addr1, sizeof(addr1));
	addr1.sun_family = AF_LOCAL;
	strncpy(addr1.sun_path, argv[1], sizeof(addr1.sun_path) - 1);
	if (bind(sockfd, (struct sockaddr*)&addr1, SUN_LEN(&addr1)) < 0) {
		printf("bind error:%s\n", strerror(errno));
		return -1;
	}	

	len = sizeof(addr2);
	if (getsockname(sockfd, (struct sockaddr*)&addr2, &len) < 0) {
		printf("getsockname error:%s\n", strerror(errno));
		close(sockfd);
		return -1;
	}

	printf("bound name=%s, returned len=%d\n", addr2.sun_path, len);
	exit(0);
	return 0;
}
