
#include "config.h"
#include "radvd.h"

#include <sys/types.h>
#include <sys/socket.h>

int radvd_socket(int domain, int type, int protocol)
{
	return socket(domain, type, protocol);
}

ssize_t radvd_sendmsg(int sockfd, const struct msghdr * msg, int flags)
{
	return sendmsg(sockfd, msg, flags);
}

ssize_t radvd_recvmsg(int sockfd, struct msghdr * msg, int flags)
{
	return recvmsg(sockfd, msg, flags);
}

int radvd_setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen)
{
	return setsockopt(sockfd, level, optname, optval, optlen);
}

int radvd_ioctl(int d, int request, void *p)
{
	return ioctl(d, request, p);
}

char *radvd_if_indextoname(int index, char *name)
{
	return if_indextoname(index, name);
}

int radvd_if_nametoindex(char const *name)
{
	return if_nametoindex(name);
}

int radvd_getifaddrs(struct ifaddrs **addresses)
{
	return getifaddrs(addresses);
}

void radvd_freeifaddrs(struct ifaddrs *ifa)
{
	freeifaddrs(ifa);
}

int radvd_bind(int sock, struct sockaddr *snl, size_t size)
{
	return bind(sock, snl, size);
}
