/*
 *
 *   Authors:
 *    Lars Fenneberg		<lf@elemental.net>
 *
 *   This software is Copyright 1996,1997 by the above mentioned author(s),
 *   All Rights Reserved.
 *
 *   The license which is distributed with this software in the file COPYRIGHT
 *   applies to this software. If your distribution is missing this file, you
 *   may request it from <reubenhwk@gmail.com>.
 *
 */

#include "config.h"
#include "includes.h"
#include "radvd.h"
#include "defaults.h"
#include "pathnames.h"

int check_device(int sock, struct Interface *iface)
{
	struct ifreq ifr;

	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, iface->Name, IFNAMSIZ - 1);

	if (radvd_ioctl(sock, SIOCGIFFLAGS, &ifr) < 0) {
		flog(LOG_ERR, "radvd_ioctl(SIOCGIFFLAGS) failed for %s: %s", iface->Name, strerror(errno));
		return -1;
	} else {
		dlog(LOG_ERR, 5, "radvd_ioctl(SIOCGIFFLAGS) succeeded for %s", iface->Name);
	}

	if (!(ifr.ifr_flags & IFF_UP)) {
		flog(LOG_ERR, "interface %s is not up", iface->Name);
		return -1;
	} else {
		dlog(LOG_ERR, 3, "interface %s is up", iface->Name);
	}

	if (!(ifr.ifr_flags & IFF_RUNNING)) {
		flog(LOG_ERR, "interface %s is not running", iface->Name);
		return -1;
	} else {
		dlog(LOG_ERR, 3, "interface %s is running", iface->Name);
	}

	if (!iface->UnicastOnly && !(ifr.ifr_flags & IFF_MULTICAST)) {
		flog(LOG_INFO, "interface %s does not support multicast, forcing UnicastOnly", iface->Name);
		iface->UnicastOnly = 1;
	} else {
		dlog(LOG_ERR, 3, "interface %s supports multicast", iface->Name);
	}

	return 0;
}

int get_v4addr(const char *ifn, unsigned int *dst)
{
	struct ifreq ifr;
	struct sockaddr_in *addr;
	int fd;

	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		flog(LOG_ERR, "create socket for IPv4 radvd_ioctl failed for %s: %s", ifn, strerror(errno));
		return -1;
	}

	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, ifn, IFNAMSIZ - 1);
	ifr.ifr_name[IFNAMSIZ - 1] = '\0';
	ifr.ifr_addr.sa_family = AF_INET;

	if (radvd_ioctl(fd, SIOCGIFADDR, &ifr) < 0) {
		flog(LOG_ERR, "radvd_ioctl(SIOCGIFADDR) failed for %s: %s", ifn, strerror(errno));
		close(fd);
		return -1;
	}

	addr = (struct sockaddr_in *)(&ifr.ifr_addr);

	dlog(LOG_DEBUG, 3, "IPv4 address for %s is %s", ifn, inet_ntoa(addr->sin_addr));

	*dst = addr->sin_addr.s_addr;

	close(fd);

	return 0;
}

/*
 * Saves the first link local address seen on the specified interface to iface->if_addr
 *
 */
int setup_linklocal_addr(struct Interface *iface)
{
	struct ifaddrs *addresses = 0;

	if (radvd_getifaddrs(&addresses) != 0) {
		flog(LOG_ERR, "getifaddrs failed: %s(%d)", strerror(errno), errno);
	} else {
		char addr_str[INET6_ADDRSTRLEN];
		uint8_t const ll_prefix[] = { 0xfe, 0x80, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 };
		struct ifaddrs *ifa;
		for (ifa = addresses; ifa != NULL; ifa = ifa->ifa_next) {

			if (!ifa->ifa_addr)
				continue;

			if (ifa->ifa_addr->sa_family != AF_INET6)
				continue;

			struct sockaddr_in6 *a6 = (struct sockaddr_in6 *)ifa->ifa_addr;

			/* Skip if it is not a linklocal address */
			if (memcmp(&(a6->sin6_addr), ll_prefix, sizeof(ll_prefix)) != 0)
				continue;

			/* Skip if it is not the interface we're looking for. */
			if (strcmp(ifa->ifa_name, iface->Name) != 0)
				continue;

			memcpy(&iface->if_addr, &(a6->sin6_addr), sizeof(struct in6_addr));

			radvd_freeifaddrs(addresses);

			addrtostr(&iface->if_addr, addr_str, sizeof(addr_str));
			dlog(LOG_DEBUG, 4, "linklocal address for %s is %s", iface->Name, addr_str);

			return 0;
		}
	}

	if (addresses)
		radvd_freeifaddrs(addresses);

	if (iface->IgnoreIfMissing)
		dlog(LOG_DEBUG, 4, "no linklocal address configured for %s", iface->Name);
	else
		flog(LOG_ERR, "no linklocal address configured for %s", iface->Name);

	return -1;
}

/*
 * This function updates the if_index of an interface.  If the
 * if index changes, then return 1 to let the calling code know
 * to let the interfaces api know so the by_index array can be
 * marked dirty and later resorted.
 */
int update_device_index(struct Interface *iface)
{
	int retval = 0;
	int index = radvd_if_nametoindex(iface->Name);
	if (0 == index) {
		/* Yes, if_nametoindex returns zero on failure.  2014/01/16 */
		flog(LOG_ERR, "%s not found: %s", iface->Name, strerror(errno));
		retval |= 0x2;
	}
	if (iface->if_index != index) {
		iface->if_index = index;
		retval |= 0x1;
	}
	return retval;
}

int check_ip6_forwarding(void)
{
#ifdef HAVE_SYS_SYSCTL_H
	int forw_sysctl[] = { SYSCTL_IP6_FORWARDING };
#endif
	int value;
	size_t size = sizeof(value);
	FILE *fp = NULL;
	static int warned = 0;

#ifdef __linux__
	fp = fopen(PROC_SYS_IP6_FORWARDING, "r");
	if (fp) {
		int rc = fscanf(fp, "%d", &value);
		if (rc != 1) {
			flog(LOG_ERR, "cannot read value from %s: %s", PROC_SYS_IP6_FORWARDING, strerror(errno));
			exit(1);
		}
		fclose(fp);
	} else {
		flog(LOG_DEBUG, "Correct IPv6 forwarding procfs entry not found, " "perhaps the procfs is disabled, " "or the kernel interface has changed?");
		value = -1;
	}
#endif				/* __linux__ */

#ifdef HAVE_SYS_SYSCTL_H
	if (!fp && sysctl(forw_sysctl, sizeof(forw_sysctl) / sizeof(forw_sysctl[0]), &value, &size, NULL, 0) < 0) {
		flog(LOG_DEBUG, "Correct IPv6 forwarding sysctl branch not found, " "perhaps the kernel interface has changed?");
		return (0);	/* this is of advisory value only */
	}
#endif

#ifdef __linux__
	/* Linux allows the forwarding value to be either 1 or 2.
	 * https://git.kernel.org/cgit/linux/kernel/git/torvalds/linux.git/tree/Documentation/networking/ip-sysctl.txt?id=ae8abfa00efb8ec550f772cbd1e1854977d06212#n1078
	 *
	 * The value 2 indicates forwarding is enabled and that *AS* *WELL* router solicitions are being done.
	 *
	 * Which is sometimes used on routers performing RS on their WAN (ppp, etc.) links
	 */
	if (!warned && value != 1 && value != 2) {
		warned = 1;
		flog(LOG_DEBUG, "IPv6 forwarding setting is: %u, should be 1 or 2", value);
		return -1;
	}
#else
	if (!warned && value != 1) {
		warned = 1;
		flog(LOG_DEBUG, "IPv6 forwarding setting is: %u, should be 1", value);
		return -1;
	}
#endif				/* __linux__ */

	return (0);
}
