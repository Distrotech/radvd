
#include <stdio.h>
#include <stdlib.h>
#include <net/if_arp.h>
#include <sys/types.h>
#include <linux/ipv6.h>

#include "tuntap.h"

int main(int argc, char * argv[])
{
	int fd;
	int sock;
	char dev[IFNAMSIZ] = {""};
	struct ifreq	ifr;
	struct in6_ifreq ifr6;
	struct sockaddr_in6 sai;

	fd = tun_alloc(dev, IFF_TAP);
	if (fd < 0) {
		perror("tun_alloc failed");
		exit(1);
	}

	sock = open_icmpv6_socket();
	if (sock < 0) {
		perror("open_icmpv6_socket failed");
		exit(1);
	}

	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, dev, IFNAMSIZ-1);
#if 1
	ifr.ifr_hwaddr.sa_family = ARPHRD_ETHER;
	ifr.ifr_hwaddr.sa_data[0] = 0;
	ifr.ifr_hwaddr.sa_data[1] = 1;
	ifr.ifr_hwaddr.sa_data[2] = 2;
	ifr.ifr_hwaddr.sa_data[3] = 3;
	ifr.ifr_hwaddr.sa_data[4] = 4;
	ifr.ifr_hwaddr.sa_data[5] = 5;

	if (ioctl(sock, SIOCSIFHWADDR, &ifr) < 0) {
		perror("ioctl(SIOCSIFHWADDR) failed");
		exit(1);
	}
#endif

#if 1
	if (ioctl(sock, SIOCGIFFLAGS, &ifr) < 0) {
		perror("ioctl(SIOCGIFFLAGS) failed");
		exit(1);
	}
	ifr.ifr_flags |= IFF_UP | IFF_RUNNING;
	if (ioctl(sock, SIOCSIFFLAGS, &ifr) < 0) {
		perror("ioctl(SIOCGIFFLAGS) failed");
		exit(1);
	}
#endif


	return 0;
}

