/*
 *   $Id: socket.c,v 1.1.1.1 1997/10/14 17:17:40 lf Exp $
 *
 *   Authors:
 *    Pedro Roque		<roque@di.fc.ul.pt>
 *    Lars Fenneberg		<lf@elemental.net>
 *
 *   This software is Copyright 1996,1997 by the above mentioned author(s), 
 *   All Rights Reserved.
 *
 *   The license which is distributed with this software in the file COPYRIGHT
 *   applies to this software. If your distribution is missing this file, you
 *   may request it from <lf@elemental.net>.
 *
 */

#include <config.h>
#include <includes.h>
#include <radvd.h>

int
open_icmpv6_socket(void)
{
	int sock;
	struct icmpv6_filter filter;
	int err, val;

        sock = socket(AF_INET6, SOCK_RAW, IPPROTO_ICMPV6);
	if (sock < 0)
	{
		log(LOG_ERR, "can't create socket(AF_INET6): %s", strerror(errno));
		return (-1);
	}

	val = 1;
	err = setsockopt(sock, IPPROTO_IPV6, IPV6_PKTINFO, &val, sizeof(int));
	if (err < 0)
	{
		log(LOG_ERR, "setsockopt(IPV6_PKTINFO): %s", strerror(errno));
		return (-1);
	}

	val = 2;
#ifdef __linux__
	err = setsockopt(sock, IPPROTO_RAW, IPV6_CHECKSUM, &val, sizeof(int));
#else
	err = setsockopt(sock, IPPROTO_IPV6, IPV6_CHECKSUM, &val, sizeof(int));
#endif
	if (err < 0)
	{
		log(LOG_ERR, "setsockopt(IPV6_CHECKSUM): %s", strerror(errno));
		return (-1);
	}

	val = 255;
	err = setsockopt(sock, IPPROTO_IPV6, IPV6_UNICAST_HOPS, &val, sizeof(int));
	if (err < 0)
	{
		log(LOG_ERR, "setsockopt(IPV6_UNICAST_HOPS): %s", strerror(errno));
		return (-1);
	}

	val = 255;
	err = setsockopt(sock, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, &val, sizeof(int));
	if (err < 0)
	{
		log(LOG_ERR, "setsockopt(IPV6_MULTICAST_HOPS): %s", strerror(errno));
		return (-1);
	}

#ifdef IPV6_HOPLIMIT
	val = 1;
	err = setsockopt(sock, IPPROTO_IPV6, IPV6_HOPLIMIT, &val, sizeof(int));
	if (err < 0)
	{
		log(LOG_ERR, "setsockopt(IPV6_HOPLIMIT): %s", strerror(errno));
		return (-1);
	}
#endif

	/*
	 * setup ICMP filter
	 */
	
	ICMPV6_FILTER_SETBLOCKALL(&filter);
	ICMPV6_FILTER_SETPASS(ND6_ROUTER_SOLICITATION, &filter);
	ICMPV6_FILTER_SETPASS(ND6_ROUTER_ADVERTISEMENT, &filter);

	err = setsockopt(sock, IPPROTO_ICMPV6, ICMPV6_FILTER, &filter,
			 sizeof(struct icmpv6_filter));
	if (err < 0)
	{
		log(LOG_ERR, "setsockopt(ICMPV6_FILTER): %s", strerror(errno));
		return (-1);
	}

	return sock;
}
