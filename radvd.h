/*
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
 *   may request it from <reubenhwk@gmail.com>.
 *
 */

#pragma once

#include "config.h"
#include "includes.h"
#include "defaults.h"
#include "log.h"

#define CONTACT_EMAIL	"Reuben Hawkins <reubenhwk@gmail.com>"

#define min(a,b)	(((a) < (b)) ? (a) : (b))

struct AdvPrefix;
struct Clients;

#define HWADDR_MAX 16
#define USER_HZ 100

struct Interface {
	char Name[IFNAMSIZ];	/* interface name */

	struct in6_addr if_addr;
	unsigned int if_index;
	unsigned int *flags;

	uint8_t racount;	/* Initial RAs */

	uint8_t if_hwaddr[HWADDR_MAX];
	int if_hwaddr_len;
	int if_prefix_len;
	int if_maxmtu;

	int cease_adv;

	struct timeval last_ra_time;

	int IgnoreIfMissing;
	int AdvSendAdvert;
	double MaxRtrAdvInterval;
	double MinRtrAdvInterval;
	double MinDelayBetweenRAs;
	int AdvManagedFlag;
	int AdvOtherConfigFlag;
	uint32_t AdvLinkMTU;
	uint32_t AdvReachableTime;
	uint32_t AdvRetransTimer;
	uint8_t AdvCurHopLimit;
	int32_t AdvDefaultLifetime;	/* XXX: really uint16_t but we need to use -1 */
	int AdvDefaultPreference;
	int AdvSourceLLAddress;
	int UnicastOnly;

	/* Mobile IPv6 extensions */
	int AdvIntervalOpt;
	int AdvHomeAgentInfo;
	int AdvHomeAgentFlag;
	uint16_t HomeAgentPreference;
	int32_t HomeAgentLifetime;	/* XXX: really uint16_t but we need to use -1 */

	/* NEMO extensions */
	int AdvMobRtrSupportFlag;

	/* 6lowpan extension */
	struct AdvLowpanCo *AdvLowpanCoList;
	struct AdvAbro *AdvAbroList;

	struct AdvPrefix *AdvPrefixList;
	struct AdvRoute *AdvRouteList;
	struct AdvRDNSS *AdvRDNSSList;
	struct AdvDNSSL *AdvDNSSLList;
	struct Clients *ClientList;
	struct timeval last_multicast;
	struct timeval _next_multicast;

	/* Info whether this interface has been initialized successfully */
	int ready;

	struct Interface *next;
};

struct Clients {
	struct in6_addr Address;
	struct Clients *next;
};

struct AdvPrefix {
	struct in6_addr Prefix;
	uint8_t PrefixLen;

	int AdvOnLinkFlag;
	int AdvAutonomousFlag;
	uint32_t AdvValidLifetime;
	uint32_t AdvPreferredLifetime;
	int DeprecatePrefixFlag;
	int DecrementLifetimesFlag;

	uint32_t curr_validlft;
	uint32_t curr_preferredlft;

	/* Mobile IPv6 extensions */
	int AdvRouterAddr;

	/* 6to4 etc. extensions */
	char if6to4[IFNAMSIZ];
	int enabled;
	int AutoSelected;

	/* Select prefixes from this interface. */
	char if6[IFNAMSIZ];

	struct AdvPrefix *next;
};

/* More-Specific Routes extensions */

struct AdvRoute {
	struct in6_addr Prefix;
	uint8_t PrefixLen;

	int AdvRoutePreference;
	uint32_t AdvRouteLifetime;
	int RemoveRouteFlag;

	struct AdvRoute *next;
};

/* Options for DNS configuration */

struct AdvRDNSS {
	int AdvRDNSSNumber;
	uint32_t AdvRDNSSLifetime;
	int FlushRDNSSFlag;
	struct in6_addr AdvRDNSSAddr1;
	struct in6_addr AdvRDNSSAddr2;
	struct in6_addr AdvRDNSSAddr3;

	struct AdvRDNSS *next;
};

struct AdvDNSSL {
	uint32_t AdvDNSSLLifetime;

	int AdvDNSSLNumber;
	int FlushDNSSLFlag;
	char **AdvDNSSLSuffixes;

	struct AdvDNSSL *next;
};

/* Options for 6lopan configuration */

struct AdvLowpanCo {
	uint8_t ContextLength;
	uint8_t ContextCompressionFlag;
	uint8_t AdvContextID;
	uint16_t AdvLifeTime;
	struct in6_addr AdvContextPrefix;

	struct AdvLowpanCo *next;
};

struct AdvAbro {
	uint16_t Version[2];
	uint16_t ValidLifeTime;
	struct in6_addr LBRaddress;

	struct AdvAbro *next;
};

/* Mobile IPv6 extensions */

struct AdvInterval {
	uint8_t type;
	uint8_t length;
	uint16_t reserved;
	uint32_t adv_ival;
};

struct HomeAgentInfo {
	uint8_t type;
	uint8_t length;
	uint16_t flags_reserved;
	uint16_t preference;
	uint16_t lifetime;
};

/* Uclibc : include/netinet/icmpv6.h - Added by Bhadram*/
#define ND_OPT_ARO	33
#define ND_OPT_6CO	34
#define ND_OPT_ABRO	35

struct nd_opt_abro {
	uint8_t nd_opt_abro_type;
	uint8_t nd_opt_abro_len;
	uint16_t nd_opt_abro_ver_low;
	uint16_t nd_opt_abro_ver_high;
	uint16_t nd_opt_abro_valid_lifetime;
	struct in6_addr nd_opt_abro_6lbr_address;
};

struct nd_opt_6co {
	uint8_t nd_opt_6co_type;
	uint8_t nd_opt_6co_len;
	uint8_t nd_opt_6co_context_len;
	uint8_t nd_opt_6co_res:3;
	uint8_t nd_opt_6co_c:1;
	uint8_t nd_opt_6co_cid:4;
	uint16_t nd_opt_6co_reserved;
	uint16_t nd_opt_6co_valid_lifetime;
	struct in6_addr nd_opt_6co_con_prefix;
};				/*Added by Bhadram */

/* gram.y */
struct Interface *readin_config(char const *fname);

/* radvd.c */
int disable_ipv6_autoconfig(char const *iface);
int check_ip6_forwarding(void);
int setup_iface(int sock, struct Interface *iface);

/* timer.c */
struct timeval next_timeval(double next);
int timevaldiff(struct timeval const *a, struct timeval const *b);
int next_time_msec(struct Interface const *iface);
int expired(struct Interface const *iface);

/* device.c */
int update_device_index(struct Interface *iface);
int update_device_info(int sock, struct Interface *);
int check_device(int sock, struct Interface *);
int setup_linklocal_addr(struct Interface *);
int setup_allrouters_membership(int sock, struct Interface *);
int get_v4addr(const char *, unsigned int *);
int set_interface_linkmtu(const char *, uint32_t);
int set_interface_curhlim(const char *, uint8_t);
int set_interface_reachtime(const char *, uint32_t);
int set_interface_retranstimer(const char *, uint32_t);
int check_ip6_forwarding(void);

/* interface.c */
void iface_init_defaults(struct Interface *);
void prefix_init_defaults(struct AdvPrefix *);
void route_init_defaults(struct AdvRoute *, struct Interface *);
void rdnss_init_defaults(struct AdvRDNSS *, struct Interface *);
void dnssl_init_defaults(struct AdvDNSSL *, struct Interface *);
int check_iface(struct Interface *);
void free_ifaces(struct Interface *ifaces);

struct Interface *find_iface_by_index(struct Interface *iface, int index);
struct Interface *find_iface_by_time(struct Interface *iface_list);
void for_each_iface(struct Interface *ifaces, void (*foo) (struct Interface * iface, void *), void *data);
void free_iface_list(struct Interface *iface_list);
void reschedule_iface(struct Interface *iface, double next);

/* socket.c */
int open_icmpv6_socket(void);

/* send.c */
int send_ra(int sock, struct Interface *iface, struct in6_addr const *dest);
int send_ra_forall(int sock, struct Interface *iface, struct in6_addr *dest);

/* syscalls.c */
int radvd_socket(int domain, int type, int protocol);
ssize_t radvd_sendmsg(int sockfd, const struct msghdr *msg, int flags);
ssize_t radvd_recvmsg(int sockfd, struct msghdr *msg, int flags);
int radvd_setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen);
int radvd_ioctl(int d, int request, void *p);
int radvd_if_nametoindex(char const *name);
char *radvd_if_indextoname(int index, char *name);
int radvd_getifaddrs(struct ifaddrs **addresses);
void radvd_freeifaddrs(struct ifaddrs *);
int radvd_bind(int sock, struct sockaddr *snl, size_t size);

/* process.c */
void process(int sock, struct Interface *, unsigned char *, int, struct sockaddr_in6 *, struct in6_pktinfo *, int);

/* recv.c */
int recv_rs_ra(int sock, unsigned char *, struct sockaddr_in6 *, struct in6_pktinfo **, int *);

/* util.c */
double rand_between(double, double);
void addrtostr(struct in6_addr *, char *, size_t);
int check_rdnss_presence(struct AdvRDNSS *, struct in6_addr *);
int check_dnssl_presence(struct AdvDNSSL *, const char *);
ssize_t readn(int fd, void *buf, size_t count);
ssize_t writen(int fd, const void *buf, size_t count);

/* privsep.c */
int privsep_init(void);
int privsep_enabled(void);
int privsep_interface_linkmtu(const char *iface, uint32_t mtu);
int privsep_interface_curhlim(const char *iface, uint32_t hlim);
int privsep_interface_reachtime(const char *iface, uint32_t rtime);
int privsep_interface_retranstimer(const char *iface, uint32_t rettimer);

/*
 * compat hacks in case libc and kernel get out of sync:
 *
 * glibc 2.4 and uClibc 0.9.29 introduce IPV6_RECVPKTINFO etc. and change IPV6_PKTINFO
 * This is only supported in Linux kernel >= 2.6.14
 *
 * This is only an approximation because the kernel version that libc was compiled against
 * could be older or newer than the one being run.  But this should not be a problem --
 * we just keep using the old kernel interface.
 *
 * these are placed here because they're needed in all of socket.c, recv.c and send.c
 */
#ifdef __linux__
#if defined IPV6_RECVHOPLIMIT || defined IPV6_RECVPKTINFO
#include <linux/version.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,14)
#if defined IPV6_RECVHOPLIMIT && defined IPV6_2292HOPLIMIT
#undef IPV6_RECVHOPLIMIT
#define IPV6_RECVHOPLIMIT IPV6_2292HOPLIMIT
#endif
#if defined IPV6_RECVPKTINFO && defined IPV6_2292PKTINFO
#undef IPV6_RECVPKTINFO
#undef IPV6_PKTINFO
#define IPV6_RECVPKTINFO IPV6_2292PKTINFO
#define IPV6_PKTINFO IPV6_2292PKTINFO
#endif
#endif
#endif
#endif
