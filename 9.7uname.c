#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/utsname.h>


#define BUFLEN 128

char ** my_addrs(int *addrtype)
{
	struct hostent *hptr;
	struct utsname myname;

	if (uname(&myname) < 0) return NULL;
	
	if ((hptr = gethostbyname(myname.nodename)) == NULL) return NULL;
	
	*addrtype = hptr->h_addrtype;
	return hptr->h_addr_list;
}

int main(int argc, char **argv)
{
	int type;
	char *paddr = *my_addrs(&type);
	char buf[BUFLEN];
	
	printf("type=%d, haddr=%s\n", type,inet_ntop(AF_INET, paddr, buf, sizeof(buf)));
	return 0;
}
