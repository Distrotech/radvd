
#include <stdio.h>
#include <stdlib.h>

#include "tuntap.h"
int sock;

int main(int argc, char * argv[])
{
	int tunfd;
	struct ifreq ifr;
	int mtu;
	char dev[IFNAMSIZ] = {""};

	memset(&ifr, 0, sizeof(ifr));

	tunfd = tun_alloc(dev, IFF_TUN);
	if (tunfd < 0) {
		perror("tun_alloc failed");
		exit(1);
	}

	sock = open_icmpv6_socket();
	if (sock < 0) {
		perror("open_icmpv6_socket failed");
		exit(1);
	}

	strncpy(ifr.ifr_name, dev, sizeof(ifr.ifr_name));
	if (ioctl(sock, SIOCGIFMTU, (caddr_t)&ifr) < 0) {
		perror("ioctl(sock, SIOCGIFMTU, (caddr_t)&ifr)");
		exit(1);
	}

	mtu = ifr.ifr_mtu;
	++ifr.ifr_mtu;

	if (ioctl(sock, SIOCSIFMTU, (caddr_t)&ifr) < 0) {
		perror("ioctl(sock, SIOCSIFMTU, (caddr_t)&ifr)");
		exit(1);
	}

	if (ioctl(sock, SIOCGIFMTU, (caddr_t)&ifr) < 0) {
		perror("ioctl(sock, SIOCGIFMTU, (caddr_t)&ifr)");
		exit(1);
	}

	if (mtu != (ifr.ifr_mtu-1)) {
		fprintf(stderr, "failed to set mtu.");
		exit(1);
	}

	close(tunfd);
	close(sock);

	return 0;
}

