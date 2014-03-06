
#include "../radvd.h"
#include <string.h>
#include <stdio.h>

int main(int argc, char * argv[])
{
	struct Interface iface;
	unsigned char buff[100];
	size_t len = 0;

	memset(&iface, 0, sizeof(iface));
	len = add_sllao(buff, len, iface);
	return 1;
}
