
#include "../radvd.h"
#include <string.h>
#include <stdio.h>

static int buffer[1024*1024*4];

int main(int argc, char * argv[])
{
	int i;
	char template[] = {"/tmp/radvd-tmp-XXXXXXXX"};
	int fd = mkstemp(template);

	if (fd < 0)
		return -1;

	unlink(template);

	memset(buffer, 1, sizeof(buffer));

	if (0 != writen(fd, 0, 0))
		return -1;
	for (i = 0; i < 10; ++i) {
		if (sizeof(buffer) != writen(fd, buffer, sizeof(buffer)))
			return -1;
	}

	lseek(fd, 0, SEEK_SET);

	if (0 != readn(fd, 0, 0))
		return -1;
	for (i = 0; i < 10; ++i) {
		if (sizeof(buffer) != readn(fd, buffer, sizeof(buffer)))
			return -1;
	}

	return 0;
}
