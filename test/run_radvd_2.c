
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <net/if_arp.h>

#include "tuntap.h"


void write_config(char const * dev, char const * mode);
void write_config(char const * dev, char const * mode)
{

	FILE * out = fopen("radvd.conf", mode);

	if (!out) {
		perror("fopen failed");
		exit(1);
	}

	fprintf(out, "interface %s {\n", dev);
	fprintf(out, "     AdvSendAdvert on;\n");
	fprintf(out, "     MinRtrAdvInterval 10;\n");
	fprintf(out, "     MaxRtrAdvInterval 60;\n");
	fprintf(out, "     AdvLinkMTU 1472;\n");
	fprintf(out, "     prefix 1234:5678:9abc::/64 {\n");
	fprintf(out, "             AdvOnLink off;\n");
	fprintf(out, "             AdvAutonomous on;\n");
	fprintf(out, "             AdvRouterAddr on; \n");
	fprintf(out, "             AdvPreferredLifetime 90;\n");
	fprintf(out, "             AdvValidLifetime 120;\n");
	fprintf(out, "     };\n");
	fprintf(out, "     RDNSS 2001:470:20::2\n");
	fprintf(out, "     {\n");
	fprintf(out, "             AdvRDNSSLifetime 60;\n");
	fprintf(out, "     };\n");
	fprintf(out, "};\n");

	fclose(out);

}

int main(int argc, char * argv[])
{
	int sock;
	pid_t pid;
	struct ifreq	ifr;
	int tunnels = 8;

	sock = open_icmpv6_socket();
	if (sock < 0) {
		perror("open_icmpv6_socket failed");
		exit(1);
	}

	unlink("radvd.conf");

	int fd;
	while (tunnels--) {
		char dev[IFNAMSIZ] = {""};

		fd = tun_alloc(dev, IFF_TAP);
		if (fd < 0) {
			perror("tun_alloc failed");
			exit(1);
		}
#if 0
		if(ioctl(fd, TUNSETPERSIST, 1) < 0){
			perror("enabling TUNSETPERSIST");
			exit(1);
		}
#endif

		write_config(dev, "a");

		memset(&ifr, 0, sizeof(ifr));
		strncpy(ifr.ifr_name, dev, IFNAMSIZ-1);

#if 0
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

	}

	while (0) {
		int status = -1;
		pid_t rc;
		int count = 0;

		usleep(10000);
	}

	return 0;
}

