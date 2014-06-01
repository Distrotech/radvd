
#include "../radvd.h"
#include <string.h>
#include <stdio.h>

int main(int argc, char * argv[])
{
	struct in6_addr pfx = {
		{ {0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10, 0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef}}
	};
	struct in6_addr pfx2 = {
		{ {0xfe, 0x80, 0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef}}
	};
	struct in6_addr pfx3 = {
		{ {0xfe, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef}}
	};

	char buffer4[4];
	char pfx_str[INET6_ADDRSTRLEN];
	char pfx_str2[INET6_ADDRSTRLEN];
	char pfx_str3[INET6_ADDRSTRLEN];
	char buffer100[100];

	addrtostr(&pfx, buffer4, sizeof(buffer4));
	addrtostr(&pfx, pfx_str, sizeof(pfx_str));
	addrtostr(&pfx2, pfx_str2, sizeof(pfx_str2));
	addrtostr(&pfx3, pfx_str3, sizeof(pfx_str3));
	addrtostr(&pfx, buffer100, sizeof(buffer100));

	char const * const result1 = "[in";
	char const * const result2 = "fedc:ba98:7654:3210:123:4567:89ab:cdef";
	char const * const result3 = "fe80:123:4567:89ab:cdef::";
	char const * const result4 = "fe80::123:4567:89ab:cdef";
	char const * const result5 = "fedc:ba98:7654:3210:123:4567:89ab:cdef";

	if (0 != strcmp(result1, buffer4)) {
		fprintf(stderr, "Expected: %s\nBut got : %s\n", result1, buffer4);
		exit(1);
	}

	if (0 != strcmp(result2, pfx_str)) {
		fprintf(stderr, "Expected: %s\nBut got : %s\n", result2, pfx_str);
		exit(1);
	}

	if (0 != strcmp(result3, pfx_str2)) {
		fprintf(stderr, "Expected: %s\nBut got : %s\n", result3, pfx_str2);
		exit(1);
	}

	if (0 != strcmp(result4, pfx_str3)) {
		fprintf(stderr, "Expected: %s\nBut got : %s\n", result4, pfx_str3);
		exit(1);
	}

	if (0 != strcmp(result5, buffer100)) {
		fprintf(stderr, "Expected: %s\nBut got : %s\n", result5, buffer100);
		exit(1);
	}


	return 0;
}
