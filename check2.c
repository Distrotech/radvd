
#include <stdio.h>
#include <stdlib.h>

#include "tuntap.h"

int main(int argc, char * argv[])
{
	int count = 100;
	int allocated = 0;

	while (count--) {	
		int fd;
		char dev[IFNAMSIZ] = {""};
		fd = tun_alloc(dev, IFF_TUN);
		if (fd >= 0) {
			++allocated;
		}
	}

	printf("allocated %d tunnels\n", allocated);

	return 0;
}

