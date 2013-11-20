
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include "tuntap.h"


void write_config(char const * dev);
void write_config(char const * dev)
{

	FILE * out = fopen("/tmp/radvd.conf", "w");

	if (!out) {
		perror("fopen failed");
		exit(1);
	}

	fprintf(out, "interface %s {\n", dev);
	fprintf(out, "     AdvSendAdvert on;\n");
	fprintf(out, "     MinRtrAdvInterval 20;\n");
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
	int fd;
	char dev[IFNAMSIZ] = {""};
	pid_t pid;

	fd = tun_alloc(dev, IFF_TUN);
	if (fd < 0) {
		perror("tun_alloc failed");
		exit(1);
	}

	write_config(dev);

	pid = fork();

	if (pid < 0) {
		perror("fork failed");
		exit(1);
	}
	else if (pid == 0) {
		char * args[] = {
			"./radvd",
			"-m", "stderr",
			"-d", "5",
			"--config", "/tmp/radvd.conf",
			"-n",
			0,
		};
		execvp(args[0], args);
	}
	else {
		sleep(3);
		kill(pid, SIGINT);
	}
	return 0;
}

