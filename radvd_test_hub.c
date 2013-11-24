
#include "config.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <net/if_arp.h>
#include <sys/select.h>

#ifdef HAVE_GETOPT_H
# include <getopt.h>
#endif

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
	fprintf(out, "     MaxRtrAdvInterval 600;\n");
	fprintf(out, "     AdvLinkMTU 1472;\n");
	fprintf(out, "     prefix 1234:5678:9abc::/64 {\n");
	fprintf(out, "             AdvOnLink off;\n");
	fprintf(out, "             AdvAutonomous on;\n");
	fprintf(out, "             AdvRouterAddr on; \n");
	fprintf(out, "             AdvPreferredLifetime 900;\n");
	fprintf(out, "             AdvValidLifetime 1200;\n");
	fprintf(out, "     };\n");
	fprintf(out, "};\n");

	fclose(out);

}

#ifdef HAVE_GETOPT_LONG

static char usage_str[] = {
"\n"
"  -C, --config=PATH       Sets the config file.  Default is /etc/radvd.conf.\n"
"  -h, --help              Print this help message then quit.\n"
"  -n, --number=NUM        Sets the number of interfaces to create.\n"
"  -t, --type=string       Sets the type of interfaces to create.  Valid values are [tun|tap].\n"
};

static struct option prog_opt[] = {
	{"config", 1, 0, 'C'},
	{"help", 0, 0, 'h'},
	{"number", 1, 0, 'n'},
	{"type", 1, 0, 't'},
	{NULL, 0, 0, 0}
};

#else

static char usage_str[] = {
	"[-C config_file] [-h] [-n number] [-t type]\n"
};

#endif

static char *pname;
void usage(void);
void usage(void)
{
	fprintf(stderr, "usage: %s %s\n", pname, usage_str);
	exit(1);
}

int main(int argc, char * argv[])
{
	int sock;
	int type = IFF_TAP;
	struct ifreq	ifr;
	int fds[1000];
	int tunnels = 1;
	int i;
	int c;
	fd_set readset;
	FD_ZERO(&readset);
	char const * conf_file = "radvd.conf";
#ifdef HAVE_GETOPT_LONG
	int opt_idx;
#endif

	pname = ((pname=strrchr(argv[0],'/')) != NULL)?pname+1:argv[0];
	/* parse args */
#define OPTIONS_STR "n:t:h"
#ifdef HAVE_GETOPT_LONG
	while ((c = getopt_long(argc, argv, OPTIONS_STR, prog_opt, &opt_idx)) > 0)

#else
	while ((c = getopt(argc, argv, OPTIONS_STR)) > 0)
#endif
	{
		switch (c) {
		case 'C':
			conf_file = optarg;
			break;
		case 't':
			if (strcmp(optarg, "tun"))
				type = IFF_TUN;
			else if(strcmp(optarg, "tap"))
				type = IFF_TAP;
			else
				usage();
			break;
		case 'n':
			tunnels = atoi(optarg);
			if (tunnels > (sizeof(fds)/sizeof(fds[0]))) {
				fprintf(stderr, "number of tunnels is capped at %lu", sizeof(fds)/sizeof(fds[0]));
				tunnels = sizeof(fds)/sizeof(fds[0]);
			}
			break;
		case 'h':
			usage();
#ifdef HAVE_GETOPT_LONG
		case ':':
			fprintf(stderr, "%s: option %s: parameter expected\n", pname,
				prog_opt[opt_idx].name);
			exit(1);
#endif
		case '?':
			exit(1);
		}
	}


	sock = open_icmpv6_socket();
	if (sock < 0) {
		perror("open_icmpv6_socket failed");
		exit(1);
	}

	unlink(conf_file);

	for (i = 0; i < tunnels; ++i) {
		char dev[IFNAMSIZ] = {""};

		fds[i] = tun_alloc(dev, type);
		if (fds[i] < 0) {
			perror("tun_alloc failed");
			exit(1);
		}
		FD_SET(fds[i], &readset);
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

		printf("interface %s ready...\n", dev);
	}

	printf("%s written with %d interface definitions.\n", conf_file, tunnels);

	while (1) {
		int rc = select(tunnels, &readset, 0, 0, 0);
		if (rc > 0) {
			for (i = 0; i < tunnels; ++i) {
				if (FD_ISSET(fds[i], &readset)) {
					char buffer[1500];
					int count = read(fds[i], buffer, sizeof(buffer));
					printf("read %d bytes from fds[%d]\n", count, i);
				}
			}
		}
		
	}

	return 0;
}

