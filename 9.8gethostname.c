#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#define BUFLEN 128


int main(int argc, char **argv)
{
	char buf[BUFLEN];
	
	if (gethostname(buf, sizeof(buf)) < 0) {
		printf("gethostname error:%s\n", strerror(errno));
		return -1;
	}

	printf("hostname=%s\n", buf);
	return 0;
}
