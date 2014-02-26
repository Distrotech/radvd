
#include "config.h"
#include "radvd.h"

#include <sys/types.h> 
#include <sys/socket.h>

static int socks[2];

int radvd_socket(int domain, int type, int protocol)
{
	/* type can be SOCK_RAW SOCK_STREAM */
	if (0 != socketpair(AF_LOCAL, SOCK_STREAM, 0, socks)) {
		perror("socketpair failed");
		exit(1);
	}
	return socks[0];
}

ssize_t radvd_sendmsg(int sockfd, const struct msghdr *msg, int flags)
{
	return 0;//sendmsg(sockfd, msg, flags);
}

ssize_t radvd_recvmsg(int sockfd, struct msghdr *msg, int flags)
{
	return 0;//recvmsg(sockfd, msg, flags);
}

int radvd_setsockopt(int sockfd, int level, int optname,
                      const void *optval, socklen_t optlen)
{
	return 0;//setsockopt(sockfd, level, optname, optval, optlen);
}


/*
*/
int radvd_ioctl(int d, int request, void *p)
{
	switch (request) {
	case SIOCGIFMTU:{
		struct ifreq *ifr = (struct ifreq*)p;
		int * mtu = (int*)&ifr->ifr_mtu;
		*mtu = 1500;
		return 0;
	}

	case SIOCGIFFLAGS:{
		struct ifreq *ifr = (struct ifreq*)p;
		int * flags = (int*)&ifr->ifr_flags;
		*flags = IFF_UP | IFF_RUNNING | IFF_MULTICAST;
		return 0;
	}

	case SIOCGIFADDR:{
		struct ifreq *ifr = (struct ifreq*)p;
		struct sockaddr_in * ipaddr = (struct sockaddr_in*)&ifr->ifr_addr;
		inet_pton(AF_INET6, "2002::", ipaddr);
		return 0;
	}

	case SIOCGIFHWADDR:{
		return 0;
	}

	default:
		break;
	}
	return -1;
}

int radvd_if_nametoindex(char const * name)
{
	return 1;
}
