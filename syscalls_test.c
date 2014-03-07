
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

ssize_t radvd_sendmsg(int sockfd, const struct msghdr * msg, int flags)
{
	size_t i;
	printf("%d (0x%x):\n", sockfd, flags);
	printf("\tnamelen:    %lu\n", (size_t) msg->msg_namelen);
	printf("\tiovlen:     %lu\n", msg->msg_iovlen);
	for (i = 0; i < msg->msg_iovlen; ++i) {
		printf("\t\tiov[%lu].iov_len: %lu\n", i, msg->msg_iov[i].iov_len);
	}
	return 0;		//sendmsg(sockfd, msg, flags);
}

ssize_t radvd_recvmsg(int sockfd, struct msghdr * msg, int flags)
{
	return 0;		//recvmsg(sockfd, msg, flags);
}

int radvd_setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen)
{
	return 0;		//setsockopt(sockfd, level, optname, optval, optlen);
}

/*
*/
int radvd_ioctl(int d, int request, void *p)
{
	struct ifreq *ifr = (struct ifreq *)p;

	switch (request) {
	case SIOCGIFMTU:{
			int *mtu = (int *)&ifr->ifr_mtu;
			*mtu = 1500;
			return 0;
		}

	case SIOCGIFFLAGS:{
			int *flags = (int *)&ifr->ifr_flags;
			*flags = IFF_UP | IFF_RUNNING | IFF_MULTICAST;
			return 0;
		}

	case SIOCGIFADDR:{
			struct sockaddr_in *ipaddr = (struct sockaddr_in *)&ifr->ifr_addr;
			inet_pton(AF_INET6, "2002::", ipaddr);
			return 0;
		}

	case SIOCGIFHWADDR:{
			ifr->ifr_hwaddr.sa_family = ARPHRD_ETHER;

			ifr->ifr_hwaddr.sa_data[0] = 0;
			ifr->ifr_hwaddr.sa_data[1] = 0;
			ifr->ifr_hwaddr.sa_data[2] = 0;
			ifr->ifr_hwaddr.sa_data[3] = 0;
			ifr->ifr_hwaddr.sa_data[4] = 0;
			ifr->ifr_hwaddr.sa_data[5] = 1;

			return 0;
		}

	default:
		break;
	}
	return -1;
}

char *radvd_if_indextoname(int index, char *name)
{
	strcpy("test1", name);
	return "test1";
}

int radvd_if_nametoindex(char const *name)
{
	return 1;
}

struct test_iface {
	char ifa_name[IFNAMSIZ];
	int ifa_index;
	sa_family_t sa_family;
	char addr[INET6_ADDRSTRLEN];
};

struct test_iface test_ifaces[] = {
	{{"test0"}, 1, AF_INET, {"192.168.1.1"}},
	{{"test1"}, 2, AF_INET, {"192.168.1.3"}},
	{{"test2"}, 3, AF_INET, {"192.168.1.5"}},
	{{"test3"}, 4, AF_INET, {"192.168.1.7"}},
	{{"test4"}, 5, AF_INET, {"192.168.1.8"}},
	{{"test5"}, 6, AF_INET, {"192.168.1.12"}},
	{{"test6"}, 7, AF_INET, {"192.168.3.1"}},
	{{"test7"}, 8, AF_INET, {"192.168.3.4"}},
	{{"test8"}, 9, AF_INET6, {"fe80::1234"}},
};

int radvd_getifaddrs(struct ifaddrs **addresses)
{
	struct ifaddrs *ifa = 0;
	struct sockaddr_in6 *a6;
	int i;

	for (i = 0; i < sizeof(test_ifaces)/sizeof(test_ifaces[0]); ++i) {
		struct ifaddrs *ifa_prev = ifa;
		ifa = malloc(sizeof(struct ifaddrs));
		memset(ifa, 0, sizeof(struct ifaddrs));

		ifa->ifa_name = strdup("test1");
		ifa->ifa_addr = malloc(sizeof(struct sockaddr));
		memset(ifa->ifa_addr, 0, sizeof(struct sockaddr));
		a6 = (struct sockaddr_in6 *)ifa->ifa_addr;
		inet_pton(AF_INET6, "fe80::1234", &a6->sin6_addr);
		ifa->ifa_addr->sa_family = AF_INET6;
		if (ifa_prev) {
			ifa_prev->ifa_next = ifa;
		}
		if (i == 0) {
			*addresses = ifa;
		}
	}

	return 0;
}

void radvd_freeifaddrs(struct ifaddrs *ifa)
{
	while (ifa) {
		struct ifaddrs *ifa_next = ifa->ifa_next;
		free(ifa->ifa_name);
		free(ifa->ifa_addr);
		free(ifa);
		ifa = ifa_next;
	}
}

int radvd_bind(int sock, struct sockaddr *snl, size_t size)
{
	return 0;
}
