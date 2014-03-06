
#include "../radvd.h"
#include <string.h>
#include <stdio.h>

int main(int argc, char * argv[])
{
	struct Interface iface;
	unsigned char buff[100];
	unsigned char const result1[] = {ND_OPT_SOURCE_LINKADDR, 1, 1, 2, 3, 4, 5, 6, 0, 0, 0, 0, 0, 0, 0, 0};
	unsigned char const result2[] = {ND_OPT_SOURCE_LINKADDR, 2, 1, 2, 3, 4, 5, 6, 7, 8, 0, 0, 0, 0, 0, 0};
	size_t len = 0;

	memset(&iface, 0, sizeof(iface));
	memset(buff, 0, sizeof(buff));
	iface.if_hwaddr[0] = 1;
	iface.if_hwaddr[1] = 2;
	iface.if_hwaddr[2] = 3;
	iface.if_hwaddr[3] = 4;
	iface.if_hwaddr[4] = 5;
	iface.if_hwaddr[5] = 6;
	iface.if_hwaddr[6] = 7;
	iface.if_hwaddr[7] = 8;
	iface.if_hwaddr[8] = 9;
	iface.if_hwaddr[9] = 10;
	iface.if_hwaddr[10] = 11;
	iface.if_hwaddr[11] = 12;
	iface.if_hwaddr[12] = 13;
	iface.if_hwaddr[13] = 14;
	iface.if_hwaddr[14] = 15;
	iface.if_hwaddr[15] = 16;

	iface.if_hwaddr_len = 48;
	add_sllao(buff, &len, &iface);
	if (len != 8) {
		fprintf(stderr, "len != 8\n");
		fprintf(stderr, "len: %u\n", (unsigned)len);
		return 1;
	}
	if (0 != memcmp(buff, result1, sizeof(result1))) {
		int i;
		fprintf(stderr, "0 != memcmp(buff, result1, sizeof(result1))\n");
		for (i = 0; i < sizeof(result1); ++i) {
			printf("%d\n", buff[i]);
		}	
		return 1;
	}

	iface.if_hwaddr_len = 64;
	add_sllao(buff, &len, &iface);
	if (len != 24) {
		fprintf(stderr, "len != 24\n");
		fprintf(stderr, "len: %u\n", (unsigned)len);
		return 1;
	}
	if (0 != memcmp(buff+8, result2, sizeof(result2))) {
		fprintf(stderr, "0 != memcmp(buff+8, result2, sizeof(result2))\n");
		int i;
		fprintf(stderr, "0 != memcmp(buff, result1, sizeof(result1))\n");
		for (i = 0; i < sizeof(result2); ++i) {
			printf("%d\n", buff[i+8]);
		}	
		return 1;
	}

	return 0;
}
