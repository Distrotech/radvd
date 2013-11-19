
#include <stdio.h>
#include <stdlib.h>

#include "tuntap.h"

int main(int argc, char * argv[])
{
	int fd;
	char dev[IFNAMSIZ] = {""};

	fd = tun_alloc(dev, IFF_TUN);
	if (fd >= 0) {
		if (ioctl(fd, TUNSETPERSIST, 1) < 0) {
			perror("enabling TUNSETPERSIST");
			exit(1);
		}
		printf("%d %s\n", fd, dev);
	}
	else {
		perror("tun_alloc failed");
		exit(1);
	}
	return 0;
}

