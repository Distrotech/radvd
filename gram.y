/*
 *
 *   Authors:
 *    Pedro Roque		<roque@di.fc.ul.pt>
 *    Lars Fenneberg		<lf@elemental.net>
 *
 *   This software is Copyright 1996-2000 by the above mentioned author(s),
 *   All Rights Reserved.
 *
 *   The license which is distributed with this software in the file COPYRIGHT
 *   applies to this software. If your distribution is missing this file, you
 *   may request it from <reubenhwk@gmail.com>.
 *
 */

%define api.pure
%parse-param {struct yydata * yydata}
%locations
%defines

%code requires {
struct yydata;
}

%{
#define YYERROR_VERBOSE
static void yyerror(void const * loc, void * vp, char const * s);
#include "config.h"
#include "includes.h"
#include "radvd.h"
#include "defaults.h"
#include "rbtree.h"
#include "stddef.h"

static int countbits(int b);
static int count_mask(struct sockaddr_in6 *m);
static struct in6_addr get_prefix6(struct in6_addr const *addr, struct in6_addr const *mask);

#if 0 /* no longer necessary? */
#ifndef HAVE_IN6_ADDR_S6_ADDR
# ifdef __FreeBSD__
#  define s6_addr32 __u6_addr.__u6_addr32
#  define s6_addr16 __u6_addr.__u6_addr16
# endif
#endif
#endif

#define ADD_TO_LL(type, list, value) \
	do { \
		if (yydata->iface->list == NULL) \
			yydata->iface->list = value; \
		else { \
			type *current = yydata->iface->list; \
			while (current->next != NULL) \
				current = current->next; \
			current->next = value; \
		} \
	} while (0)

%}

%token		T_INTERFACE
%token		T_PREFIX
%token		T_ROUTE
%token		T_RDNSS
%token		T_DNSSL
%token		T_CLIENTS
%token		T_LOWPANCO
%token		T_ABRO

%token	<str>	STRING
%token	<num>	NUMBER
%token	<snum>	SIGNEDNUMBER
%token	<dec>	DECIMAL
%token	<num>	SWITCH
%token	<addr>	IPV6ADDR
%token 		INFINITY

%token		T_IgnoreIfMissing
%token		T_AdvSendAdvert
%token		T_MaxRtrAdvInterval
%token		T_MinRtrAdvInterval
%token		T_MinDelayBetweenRAs
%token		T_AdvManagedFlag
%token		T_AdvOtherConfigFlag
%token		T_AdvLinkMTU
%token		T_AdvReachableTime
%token		T_AdvRetransTimer
%token		T_AdvCurHopLimit
%token		T_AdvDefaultLifetime
%token		T_AdvDefaultPreference
%token		T_AdvSourceLLAddress

%token		T_AdvOnLink
%token		T_AdvAutonomous
%token		T_AdvValidLifetime
%token		T_AdvPreferredLifetime
%token		T_DeprecatePrefix
%token		T_DecrementLifetimes

%token		T_AdvRouterAddr
%token		T_AdvHomeAgentFlag
%token		T_AdvIntervalOpt
%token		T_AdvHomeAgentInfo

%token		T_Base6Interface
%token		T_Base6to4Interface
%token		T_UnicastOnly

%token		T_HomeAgentPreference
%token		T_HomeAgentLifetime

%token		T_AdvRoutePreference
%token		T_AdvRouteLifetime
%token		T_RemoveRoute

%token		T_AdvRDNSSPreference
%token		T_AdvRDNSSOpenFlag
%token		T_AdvRDNSSLifetime
%token		T_FlushRDNSS

%token		T_AdvDNSSLLifetime
%token		T_FlushDNSSL

%token		T_AdvMobRtrSupportFlag

%token		T_AdvContextLength
%token		T_AdvContextCompressionFlag
%token		T_AdvContextID
%token		T_AdvLifeTime
%token		T_AdvContextPrefix

%token		T_AdvVersionLow
%token		T_AdvVersionHigh
%token		T_AdvValidLifeTime
%token		T_Adv6LBRaddress

%token		T_BAD_TOKEN

%type	<str>	name
%type	<pinfo> prefixdef
%type	<ainfo> clientslist v6addrlist
%type	<rinfo>	routedef
%type	<rdnssinfo> rdnssdef
%type	<dnsslinfo> dnssldef
%type   <lowpancoinfo> lowpancodef
%type   <abroinfo> abrodef
%type   <num>	number_or_infinity

%union {
	unsigned int		num;
	int			snum;
	double			dec;
	struct in6_addr		*addr;
	char			*str;
	struct AdvPrefix	*pinfo;
	struct AdvRoute		*rinfo;
	struct AdvRDNSS		*rdnssinfo;
	struct AdvDNSSL		*dnsslinfo;
	struct Clients		*ainfo;
	struct AdvLowpanCo	*lowpancoinfo;
	struct AdvAbro		*abroinfo;
};

%{
#include "scanner.h"
struct yydata
{
	yyscan_t scaninfo;
	char const * filename;
	int interface_count;
	struct Interface *IfaceList;
	struct Interface *iface;
	struct AdvPrefix *prefix;
	struct AdvRoute *route;
	struct AdvRDNSS *rdnss;
	struct AdvDNSSL *dnssl;
	struct AdvLowpanCo *lowpanco;
	struct AdvAbro  *abro;
};
static void cleanup(struct yydata * yydata);
#define ABORT	do { cleanup(yydata); YYABORT; } while (0);
#define YYLEX_PARAM yydata->scaninfo
%}

%%

grammar		: grammar ifacedef
		| ifacedef
		;

ifacedef	: ifacehead '{' ifaceparams  '}' ';'
		{
			struct Interface *iface2;

			iface2 = yydata->IfaceList;
			while (iface2)
			{
				if (!strcmp(iface2->Name, yydata->iface->Name))
				{
					/* TODO: print the locations of the duplicates. */
					flog(LOG_ERR, "duplicate interface "
						"definition for %s", yydata->iface->Name);
					ABORT;
				}
				iface2 = iface2->next;
			}

			yydata->iface->next = yydata->IfaceList;
			yydata->IfaceList = yydata->iface;
			yydata->iface = NULL;
			++yydata->interface_count;
		};

ifacehead	: T_INTERFACE name
		{
			yydata->iface = malloc(sizeof(struct Interface));

			if (yydata->iface == NULL) {
				flog(LOG_CRIT, "malloc failed: %s", strerror(errno));
				ABORT;
			}

			iface_init_defaults(yydata->iface);
			strncpy(yydata->iface->Name, $2, IFNAMSIZ-1);
			yydata->iface->Name[IFNAMSIZ-1] = '\0';
		}
		;

name		: STRING
		{
			/* check vality */
			$$ = $1;
		}
		;

ifaceparams :
		/* empty */
		| ifaceparam ifaceparams
		;

ifaceparam 	: ifaceval
		| prefixdef 	{ ADD_TO_LL(struct AdvPrefix, AdvPrefixList, $1); }
		| clientslist 	{ ADD_TO_LL(struct Clients, ClientList, $1); }
		| routedef 	{ ADD_TO_LL(struct AdvRoute, AdvRouteList, $1); }
		| rdnssdef 	{ ADD_TO_LL(struct AdvRDNSS, AdvRDNSSList, $1); }
		| dnssldef 	{ ADD_TO_LL(struct AdvDNSSL, AdvDNSSLList, $1); }
		| lowpancodef   { ADD_TO_LL(struct AdvLowpanCo, AdvLowpanCoList, $1); }
		| abrodef       { ADD_TO_LL(struct AdvAbro, AdvAbroList, $1); }
		;

ifaceval	: T_MinRtrAdvInterval NUMBER ';'
		{
			yydata->iface->MinRtrAdvInterval = $2;
		}
		| T_MaxRtrAdvInterval NUMBER ';'
		{
			yydata->iface->MaxRtrAdvInterval = $2;
		}
		| T_MinDelayBetweenRAs NUMBER ';'
		{
			yydata->iface->MinDelayBetweenRAs = $2;
		}
		| T_MinRtrAdvInterval DECIMAL ';'
		{
			yydata->iface->MinRtrAdvInterval = $2;
		}
		| T_MaxRtrAdvInterval DECIMAL ';'
		{
			yydata->iface->MaxRtrAdvInterval = $2;
		}
		| T_MinDelayBetweenRAs DECIMAL ';'
		{
			yydata->iface->MinDelayBetweenRAs = $2;
		}
		| T_IgnoreIfMissing SWITCH ';'
		{
			yydata->iface->IgnoreIfMissing = $2;
		}
		| T_AdvSendAdvert SWITCH ';'
		{
			yydata->iface->AdvSendAdvert = $2;
		}
		| T_AdvManagedFlag SWITCH ';'
		{
			yydata->iface->AdvManagedFlag = $2;
		}
		| T_AdvOtherConfigFlag SWITCH ';'
		{
			yydata->iface->AdvOtherConfigFlag = $2;
		}
		| T_AdvLinkMTU NUMBER ';'
		{
			yydata->iface->AdvLinkMTU = $2;
		}
		| T_AdvReachableTime NUMBER ';'
		{
			yydata->iface->AdvReachableTime = $2;
		}
		| T_AdvRetransTimer NUMBER ';'
		{
			yydata->iface->AdvRetransTimer = $2;
		}
		| T_AdvDefaultLifetime NUMBER ';'
		{
			yydata->iface->AdvDefaultLifetime = $2;
		}
		| T_AdvDefaultPreference SIGNEDNUMBER ';'
		{
			yydata->iface->AdvDefaultPreference = $2;
		}
		| T_AdvCurHopLimit NUMBER ';'
		{
			yydata->iface->AdvCurHopLimit = $2;
		}
		| T_AdvSourceLLAddress SWITCH ';'
		{
			yydata->iface->AdvSourceLLAddress = $2;
		}
		| T_AdvIntervalOpt SWITCH ';'
		{
			yydata->iface->AdvIntervalOpt = $2;
		}
		| T_AdvHomeAgentInfo SWITCH ';'
		{
			yydata->iface->AdvHomeAgentInfo = $2;
		}
		| T_AdvHomeAgentFlag SWITCH ';'
		{
			yydata->iface->AdvHomeAgentFlag = $2;
		}
		| T_HomeAgentPreference NUMBER ';'
		{
			yydata->iface->HomeAgentPreference = $2;
		}
		| T_HomeAgentLifetime NUMBER ';'
		{
			yydata->iface->HomeAgentLifetime = $2;
		}
		| T_UnicastOnly SWITCH ';'
		{
			yydata->iface->UnicastOnly = $2;
		}
		| T_AdvMobRtrSupportFlag SWITCH ';'
		{
			yydata->iface->AdvMobRtrSupportFlag = $2;
		}
		;

clientslist	: T_CLIENTS '{' v6addrlist '}' ';'
		{
			$$ = $3;
		}
		;

v6addrlist	: IPV6ADDR ';'
		{
			struct Clients *new = calloc(1, sizeof(struct Clients));
			if (new == NULL) {
				flog(LOG_CRIT, "calloc failed: %s", strerror(errno));
				ABORT;
			}

			memcpy(&(new->Address), $1, sizeof(struct in6_addr));
			$$ = new;
		}
		| v6addrlist IPV6ADDR ';'
		{
			struct Clients *new = calloc(1, sizeof(struct Clients));
			if (new == NULL) {
				flog(LOG_CRIT, "calloc failed: %s", strerror(errno));
				ABORT;
			}

			memcpy(&(new->Address), $2, sizeof(struct in6_addr));
			new->next = $1;
			$$ = new;
		}
		;


prefixdef	: prefixhead optional_prefixplist ';'
		{
			if (yydata->prefix) {
				unsigned int dst;

				if (yydata->prefix->AdvPreferredLifetime > yydata->prefix->AdvValidLifetime)
				{
					flog(LOG_ERR, "AdvValidLifeTime must be "
						"greater than AdvPreferredLifetime in %s, line %d",
						yydata->filename, @1.first_line);
					ABORT;
				}

				if ( yydata->prefix->if6[0] && yydata->prefix->if6to4[0]) {
					flog(LOG_ERR, "Base6Interface and Base6to4Interface are mutually exclusive at this time.");
					ABORT;
				}

				if ( yydata->prefix->if6to4[0] )
				{
					if (get_v4addr(yydata->prefix->if6to4, &dst) < 0)
					{
						flog(LOG_ERR, "interface %s has no IPv4 addresses, disabling 6to4 prefix", yydata->prefix->if6to4 );
						yydata->prefix->enabled = 0;
					}
					else
					{
						*((uint16_t *)(yydata->prefix->Prefix.s6_addr)) = htons(0x2002);
						memcpy( yydata->prefix->Prefix.s6_addr + 2, &dst, sizeof( dst ) );
					}
				}

				if ( yydata->prefix->if6[0] )
				{
#ifndef HAVE_IFADDRS_H
					flog(LOG_ERR, "Base6Interface not supported in %s, line %d", yydata->filename, @1.first_line);
					ABORT;
#else
					struct ifaddrs *ifap = 0, *ifa = 0;
					struct AdvPrefix *next = yydata->prefix->next;

					if (yydata->prefix->PrefixLen != 64) {
						flog(LOG_ERR, "Only /64 is allowed with Base6Interface.  %s:%d", yydata->filename, @1.first_line);
						ABORT;
					}

					if (getifaddrs(&ifap) != 0)
						flog(LOG_ERR, "getifaddrs failed: %s", strerror(errno));

					for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
						struct sockaddr_in6 *s6 = 0;
						struct sockaddr_in6 *mask = (struct sockaddr_in6 *)ifa->ifa_netmask;
						struct in6_addr base6prefix;
						char buf[INET6_ADDRSTRLEN];
						int i;

						if (strncmp(ifa->ifa_name, yydata->prefix->if6, IFNAMSIZ))
							continue;

						if (ifa->ifa_addr->sa_family != AF_INET6)
							continue;

						s6 = (struct sockaddr_in6 *)(ifa->ifa_addr);

						if (IN6_IS_ADDR_LINKLOCAL(&s6->sin6_addr))
							continue;

						base6prefix = get_prefix6(&s6->sin6_addr, &mask->sin6_addr);
						for (i = 0; i < 8; ++i) {
							yydata->prefix->Prefix.s6_addr[i] &= ~mask->sin6_addr.s6_addr[i];
							yydata->prefix->Prefix.s6_addr[i] |= base6prefix.s6_addr[i];
						}
						memset(&yydata->prefix->Prefix.s6_addr[8], 0, 8);
						yydata->prefix->AdvRouterAddr = 1;
						yydata->prefix->AutoSelected = 1;
						yydata->prefix->next = next;

						if (inet_ntop(ifa->ifa_addr->sa_family, (void *)&(yydata->prefix->Prefix), buf, sizeof(buf)) == NULL)
							flog(LOG_ERR, "%s: inet_ntop failed in %s, line %d!", ifa->ifa_name, yydata->filename, @1.first_line);
						else
							dlog(LOG_DEBUG, 3, "auto-selected prefix %s/%d on interface %s from interface %s",
								buf, yydata->prefix->PrefixLen, yydata->iface->Name, ifa->ifa_name);

						/* Taking only one prefix from the Base6Interface.  Taking more than one would require allocating new
						   prefixes and building a list.  I'm not sure how to do that from here. So for now, break. */
						break;
					}

					if (ifap)
						freeifaddrs(ifap);
#endif /* ifndef HAVE_IFADDRS_H */
				}
			}
			$$ = yydata->prefix;
			yydata->prefix = NULL;
		}
		;

prefixhead	: T_PREFIX IPV6ADDR '/' NUMBER
		{
			struct in6_addr zeroaddr;
			memset(&zeroaddr, 0, sizeof(zeroaddr));

			if (!memcmp($2, &zeroaddr, sizeof(struct in6_addr))) {
#ifndef HAVE_IFADDRS_H
				flog(LOG_ERR, "invalid all-zeros prefix in %s, line %d", yydata->filename, @1.first_line);
				ABORT;
#else
				struct ifaddrs *ifap = 0, *ifa = 0;
				struct AdvPrefix *next = yydata->iface->AdvPrefixList;

				while (next) {
					if (next->AutoSelected) {
						flog(LOG_ERR, "auto selecting prefixes works only once per interface.  See %s, line %d", yydata->filename, @1.first_line);
						ABORT;
					}
					next = next->next;
				}
				next = 0;

				dlog(LOG_DEBUG, 5, "all-zeros prefix in %s, line %d, parsing..", yydata->filename, @1.first_line);

				if (getifaddrs(&ifap) != 0)
					flog(LOG_ERR, "getifaddrs failed: %s", strerror(errno));

				for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
					struct sockaddr_in6 *s6 = (struct sockaddr_in6 *)ifa->ifa_addr;
					struct sockaddr_in6 *mask = (struct sockaddr_in6 *)ifa->ifa_netmask;
					char buf[INET6_ADDRSTRLEN];

					if (strncmp(ifa->ifa_name, yydata->iface->Name, IFNAMSIZ))
						continue;

					if (ifa->ifa_addr->sa_family != AF_INET6)
						continue;

					s6 = (struct sockaddr_in6 *)(ifa->ifa_addr);

					if (IN6_IS_ADDR_LINKLOCAL(&s6->sin6_addr))
						continue;

					yydata->prefix = malloc(sizeof(struct AdvPrefix));

					if (yydata->prefix == NULL) {
						flog(LOG_CRIT, "malloc failed: %s", strerror(errno));
						ABORT;
					}

					prefix_init_defaults(yydata->prefix);
					yydata->prefix->Prefix = get_prefix6(&s6->sin6_addr, &mask->sin6_addr);
					yydata->prefix->AdvRouterAddr = 1;
					yydata->prefix->AutoSelected = 1;
					yydata->prefix->next = next;
					next = yydata->prefix;

					if (yydata->prefix->PrefixLen == 0)
						yydata->prefix->PrefixLen = count_mask(mask);

					if (inet_ntop(ifa->ifa_addr->sa_family, (void *)&(yydata->prefix->Prefix), buf, sizeof(buf)) == NULL)
						flog(LOG_ERR, "%s: inet_ntop failed in %s, line %d!", ifa->ifa_name, yydata->filename, @1.first_line);
					else
						dlog(LOG_DEBUG, 3, "auto-selected prefix %s/%d on interface %s", buf, yydata->prefix->PrefixLen, ifa->ifa_name);
				}

				if (!yydata->prefix) {
					flog(LOG_WARNING, "no auto-selected prefix on interface %s, disabling advertisements",  yydata->iface->Name);
				}

				if (ifap)
					freeifaddrs(ifap);
#endif /* ifndef HAVE_IFADDRS_H */
			}
			else {
				yydata->prefix = malloc(sizeof(struct AdvPrefix));

				if (yydata->prefix == NULL) {
					flog(LOG_CRIT, "malloc failed: %s", strerror(errno));
					ABORT;
				}

				prefix_init_defaults(yydata->prefix);

				if ($4 > MAX_PrefixLen)
				{
					flog(LOG_ERR, "invalid prefix length in %s, line %d", yydata->filename, @1.first_line);
					ABORT;
				}

				yydata->prefix->PrefixLen = $4;

				memcpy(&yydata->prefix->Prefix, $2, sizeof(struct in6_addr));
			}
		}
		;

optional_prefixplist: /* empty */
		| '{' /* somewhat empty */ '}'
		| '{' prefixplist '}'
		;

prefixplist	: prefixplist prefixparms
		| prefixparms
		;

prefixparms	: T_AdvOnLink SWITCH ';'
		{
			if (yydata->prefix) {
				if (yydata->prefix->AutoSelected) {
					struct AdvPrefix *p = yydata->prefix;
					do {
						p->AdvOnLinkFlag = $2;
						p = p->next;
					} while (p && p->AutoSelected);
				}
				else
					yydata->prefix->AdvOnLinkFlag = $2;
			}
		}
		| T_AdvAutonomous SWITCH ';'
		{
			if (yydata->prefix) {
				if (yydata->prefix->AutoSelected) {
					struct AdvPrefix *p = yydata->prefix;
					do {
						p->AdvAutonomousFlag = $2;
						p = p->next;
					} while (p && p->AutoSelected);
				}
				else
					yydata->prefix->AdvAutonomousFlag = $2;
			}
		}
		| T_AdvRouterAddr SWITCH ';'
		{
			if (yydata->prefix) {
				if (yydata->prefix->AutoSelected && $2 == 0)
					flog(LOG_WARNING, "prefix automatically selected, AdvRouterAddr always enabled, ignoring config line %d", @1.first_line);
				else
					yydata->prefix->AdvRouterAddr = $2;
			}
		}
		| T_AdvValidLifetime number_or_infinity ';'
		{
			if (yydata->prefix) {
				if (yydata->prefix->AutoSelected) {
					struct AdvPrefix *p = yydata->prefix;
					do {
						p->AdvValidLifetime = $2;
						p->curr_validlft = $2;
						p = p->next;
					} while (p && p->AutoSelected);
				}
				else
					yydata->prefix->AdvValidLifetime = $2;
					yydata->prefix->curr_validlft = $2;
			}
		}
		| T_AdvPreferredLifetime number_or_infinity ';'
		{
			if (yydata->prefix) {
				if (yydata->prefix->AutoSelected) {
					struct AdvPrefix *p = yydata->prefix;
					do {
						p->AdvPreferredLifetime = $2;
						p->curr_preferredlft = $2;
						p = p->next;
					} while (p && p->AutoSelected);
				}
				else
					yydata->prefix->AdvPreferredLifetime = $2;
					yydata->prefix->curr_preferredlft = $2;
			}
		}
		| T_DeprecatePrefix SWITCH ';'
		{
			yydata->prefix->DeprecatePrefixFlag = $2;
		}
		| T_DecrementLifetimes SWITCH ';'
		{
			yydata->prefix->DecrementLifetimesFlag = $2;
		}
		| T_Base6Interface name ';'
		{
			if (yydata->prefix) {
				if (yydata->prefix->AutoSelected) {
					flog(LOG_ERR, "automatically selecting the prefix and Base6Interface are mutually exclusive");
					ABORT;
				} /* fallthrough */
				dlog(LOG_DEBUG, 4, "using prefixes on interface %s for prefixes on interface %s", $2, yydata->iface->Name);
				strncpy(yydata->prefix->if6, $2, IFNAMSIZ-1);
				yydata->prefix->if6[IFNAMSIZ-1] = '\0';
			}
		}

		| T_Base6to4Interface name ';'
		{
			if (yydata->prefix) {
				if (yydata->prefix->AutoSelected) {
					flog(LOG_ERR, "automatically selecting the prefix and Base6to4Interface are mutually exclusive");
					ABORT;
				} /* fallthrough */
				dlog(LOG_DEBUG, 4, "using interface %s for 6to4 prefixes on interface %s", $2, yydata->iface->Name);
				strncpy(yydata->prefix->if6to4, $2, IFNAMSIZ-1);
				yydata->prefix->if6to4[IFNAMSIZ-1] = '\0';
			}
		}
		;

routedef	: routehead '{' optional_routeplist '}' ';'
		{
			$$ = yydata->route;
			yydata->route = NULL;
		}
		;


routehead	: T_ROUTE IPV6ADDR '/' NUMBER
		{
			yydata->route = malloc(sizeof(struct AdvRoute));

			if (yydata->route == NULL) {
				flog(LOG_CRIT, "malloc failed: %s", strerror(errno));
				ABORT;
			}

			route_init_defaults(yydata->route, yydata->iface);

			if ($4 > MAX_PrefixLen)
			{
				flog(LOG_ERR, "invalid route prefix length in %s, line %d", yydata->filename, @1.first_line);
				ABORT;
			}

			yydata->route->PrefixLen = $4;

			memcpy(&yydata->route->Prefix, $2, sizeof(struct in6_addr));
		}
		;


optional_routeplist: /* empty */
		| routeplist
		;

routeplist	: routeplist routeparms
		| routeparms
		;


routeparms	: T_AdvRoutePreference SIGNEDNUMBER ';'
		{
			yydata->route->AdvRoutePreference = $2;
		}
		| T_AdvRouteLifetime number_or_infinity ';'
		{
			yydata->route->AdvRouteLifetime = $2;
		}
		| T_RemoveRoute SWITCH ';'
		{
			yydata->route->RemoveRouteFlag = $2;
		}
		;

rdnssdef	: rdnsshead '{' optional_rdnssplist '}' ';'
		{
			$$ = yydata->rdnss;
			yydata->rdnss = NULL;
		}
		;

rdnssaddrs	: rdnssaddrs rdnssaddr
		| rdnssaddr
		;

rdnssaddr	: IPV6ADDR
		{
			if (!yydata->rdnss) {
				/* first IP found */
				yydata->rdnss = malloc(sizeof(struct AdvRDNSS));

				if (yydata->rdnss == NULL) {
					flog(LOG_CRIT, "malloc failed: %s", strerror(errno));
					ABORT;
				}

				rdnss_init_defaults(yydata->rdnss, yydata->iface);
			}

			switch (yydata->rdnss->AdvRDNSSNumber) {
				case 0:
					memcpy(&yydata->rdnss->AdvRDNSSAddr1, $1, sizeof(struct in6_addr));
					yydata->rdnss->AdvRDNSSNumber++;
					break;
				case 1:
					memcpy(&yydata->rdnss->AdvRDNSSAddr2, $1, sizeof(struct in6_addr));
					yydata->rdnss->AdvRDNSSNumber++;
					break;
				case 2:
					memcpy(&yydata->rdnss->AdvRDNSSAddr3, $1, sizeof(struct in6_addr));
					yydata->rdnss->AdvRDNSSNumber++;
					break;
				default:
					flog(LOG_CRIT, "Too many addresses in RDNSS section");
					ABORT;
			}

		}
		;

rdnsshead	: T_RDNSS rdnssaddrs
		{
			if (!yydata->rdnss) {
				flog(LOG_CRIT, "No address specified in RDNSS section");
				ABORT;
			}
		}
		;

optional_rdnssplist: /* empty */
		| rdnssplist
		;

rdnssplist	: rdnssplist rdnssparms
		| rdnssparms
		;


rdnssparms	: T_AdvRDNSSPreference NUMBER ';'
		{
			flog(LOG_WARNING, "Ignoring deprecated RDNSS preference.");
		}
		| T_AdvRDNSSOpenFlag SWITCH ';'
		{
			flog(LOG_WARNING, "Ignoring deprecated RDNSS open flag.");
		}
		| T_AdvRDNSSLifetime number_or_infinity ';'
		{
			if ($2 < yydata->iface->MaxRtrAdvInterval && $2 != 0) {
				flog(LOG_ERR, "AdvRDNSSLifetime must be at least MaxRtrAdvInterval");
				ABORT;
			}
			if ($2 > 2*(yydata->iface->MaxRtrAdvInterval))
				flog(LOG_WARNING, "Warning: AdvRDNSSLifetime <= 2*MaxRtrAdvInterval would allow stale DNS servers to be deleted faster");

			yydata->rdnss->AdvRDNSSLifetime = $2;
		}
		| T_FlushRDNSS SWITCH ';'
		{
			yydata->rdnss->FlushRDNSSFlag = $2;
		}
		;

dnssldef	: dnsslhead '{' optional_dnsslplist '}' ';'
		{
			$$ = yydata->dnssl;
			yydata->dnssl = NULL;
		}
		;

dnsslsuffixes	: dnsslsuffixes dnsslsuffix
		| dnsslsuffix
		;

dnsslsuffix	: STRING
		{
			char *ch;
			for (ch = $1;*ch != '\0';ch++) {
				if (*ch >= 'A' && *ch <= 'Z')
					continue;
				if (*ch >= 'a' && *ch <= 'z')
					continue;
				if (*ch >= '0' && *ch <= '9')
					continue;
				if (*ch == '-' || *ch == '.')
					continue;

				flog(LOG_CRIT, "Invalid domain suffix specified");
				ABORT;
			}

			if (!yydata->dnssl) {
				/* first domain found */
				yydata->dnssl = malloc(sizeof(struct AdvDNSSL));

				if (yydata->dnssl == NULL) {
					flog(LOG_CRIT, "malloc failed: %s", strerror(errno));
					ABORT;
				}

				dnssl_init_defaults(yydata->dnssl, yydata->iface);
			}

			yydata->dnssl->AdvDNSSLNumber++;
			yydata->dnssl->AdvDNSSLSuffixes =
				realloc(yydata->dnssl->AdvDNSSLSuffixes,
					yydata->dnssl->AdvDNSSLNumber * sizeof(char*));
			if (yydata->dnssl->AdvDNSSLSuffixes == NULL) {
				flog(LOG_CRIT, "realloc failed: %s", strerror(errno));
				ABORT;
			}

			yydata->dnssl->AdvDNSSLSuffixes[yydata->dnssl->AdvDNSSLNumber - 1] = strdup($1);
		}
		;

dnsslhead	: T_DNSSL dnsslsuffixes
		{
			if (!yydata->dnssl) {
				flog(LOG_CRIT, "No domain specified in DNSSL section");
				ABORT;
			}
		}
		;

optional_dnsslplist: /* empty */
		| dnsslplist
		;

dnsslplist	: dnsslplist dnsslparms
		| dnsslparms
		;


dnsslparms	: T_AdvDNSSLLifetime number_or_infinity ';'
		{
			if ($2 < yydata->iface->MaxRtrAdvInterval && $2 != 0) {
				flog(LOG_ERR, "AdvDNSSLLifetime must be at least MaxRtrAdvInterval");
				ABORT;
			}
			if ($2 > 2*(yydata->iface->MaxRtrAdvInterval))
				flog(LOG_WARNING, "Warning: AdvDNSSLLifetime <= 2*MaxRtrAdvInterval would allow stale DNS suffixes to be deleted faster");

			yydata->dnssl->AdvDNSSLLifetime = $2;
		}
		| T_FlushDNSSL SWITCH ';'
		{
			yydata->dnssl->FlushDNSSLFlag = $2;
		}
		;

lowpancodef 	: lowpancohead  '{' optional_lowpancoplist '}' ';'
		{
			$$ = yydata->lowpanco;
			yydata->lowpanco = NULL;
		}
		;

lowpancohead	: T_LOWPANCO
		{
			yydata->lowpanco = malloc(sizeof(struct AdvLowpanCo));

			if (yydata->lowpanco == NULL) {
				flog(LOG_CRIT, "malloc failed: %s", strerror(errno));
				ABORT;
			}

			memset(yydata->lowpanco, 0, sizeof(struct AdvLowpanCo));
		}
		;

optional_lowpancoplist:
		| lowpancoplist
		;

lowpancoplist	: lowpancoplist lowpancoparms
		| lowpancoparms
		;

lowpancoparms 	: T_AdvContextLength NUMBER ';'
		{
			yydata->lowpanco->ContextLength = $2;
		}
		| T_AdvContextCompressionFlag SWITCH ';'
		{
			yydata->lowpanco->ContextCompressionFlag = $2;
		}
		| T_AdvContextID NUMBER ';'
		{
			yydata->lowpanco->AdvContextID = $2;
		}
		| T_AdvLifeTime NUMBER ';'
		{
			yydata->lowpanco->AdvLifeTime = $2;
		}
		;

abrodef		: abrohead  '{' optional_abroplist '}' ';'
		{
			$$ = yydata->abro;
			yydata->abro = NULL;
		}
		;

abrohead	: T_ABRO IPV6ADDR '/' NUMBER
		{
			if ($4 > MAX_PrefixLen)
			{
				/* TODO: print the locations. */
				flog(LOG_ERR, "invalid abro prefix length in %s");
				ABORT;
			}

			yydata->abro = malloc(sizeof(struct AdvAbro));

			if (yydata->abro == NULL) {
				flog(LOG_CRIT, "malloc failed: %s", strerror(errno));
				ABORT;
			}

			memset(yydata->abro, 0, sizeof(struct AdvAbro));
			memcpy(&yydata->abro->LBRaddress, $2, sizeof(struct in6_addr));
		}
		;

optional_abroplist:
		| abroplist
		;

abroplist	: abroplist abroparms
		| abroparms
		;

abroparms	: T_AdvVersionLow NUMBER ';'
		{
			yydata->abro->Version[1] = $2;
		}
		| T_AdvVersionHigh NUMBER ';'
		{
			yydata->abro->Version[0] = $2;
		}
		| T_AdvValidLifeTime NUMBER ';'
		{
			yydata->abro->ValidLifeTime = $2;
		}
		;


number_or_infinity	: NUMBER
			{
				$$ = $1;
			}
			| INFINITY
			{
				$$ = (uint32_t)~0;
			}
			;

%%

static
int countbits(int b)
{
	int count;

	for (count = 0; b != 0; count++) {
		b &= b - 1; // this clears the LSB-most set bit
	}

	return (count);
}

static
int count_mask(struct sockaddr_in6 *m)
{
	struct in6_addr *in6 = &m->sin6_addr;
	int i;
	int count = 0;

	for (i = 0; i < 16; ++i) {
		count += countbits(in6->s6_addr[i]);
	}
	return count;
}

static
struct in6_addr get_prefix6(struct in6_addr const *addr, struct in6_addr const *mask)
{
	struct in6_addr prefix = *addr;
	int i = 0;

	for (; i < 16; ++i) {
		prefix.s6_addr[i] &= mask->s6_addr[i];
	}

	return prefix;
}

static void cleanup(struct yydata * yydata)
{
	if (yydata->iface)
		free(yydata->iface);

	if (yydata->prefix)
		free(yydata->prefix);

	if (yydata->route)
		free(yydata->route);

	if (yydata->rdnss)
		free(yydata->rdnss);

	if (yydata->dnssl) {
		int i;
		for (i = 0;i < yydata->dnssl->AdvDNSSLNumber;i++)
			free(yydata->dnssl->AdvDNSSLSuffixes[i]);
		free(yydata->dnssl->AdvDNSSLSuffixes);
		free(yydata->dnssl);
	}

	if (yydata->lowpanco)
		free(yydata->lowpanco);

	if (yydata->abro)
		free(yydata->abro);

	if (yydata->IfaceList) {
		free_iface_list(yydata->IfaceList);
		yydata->IfaceList = 0;
	}
}

static
void yyerror(void const * loc, void * vp, char const * msg)
{
	char * str1 = 0;
	char * str2 = 0;
	char * str3 = 0;
	int rc = 0;
	YYLTYPE const * t = (YYLTYPE const*)loc;
	struct yydata * yydata = (struct yydata *)vp;

	cleanup(yydata);

	rc = asprintf(&str1, "%s", msg);
	if (rc == -1) {
		flog (LOG_ERR, "asprintf failed in yyerror");
	}

	rc = asprintf(&str2, "location %d.%d-%d.%d: %s",
		t->first_line, t->first_column,
		t->last_line,  t->last_column,
		yyget_text(yydata->scaninfo));
	if (rc == -1) {
		flog (LOG_ERR, "asprintf failed in yyerror");
	}

	rc = asprintf(&str3, "%s in %s, %s", str1, yydata->filename, str2);
	if (rc == -1) {
		flog (LOG_ERR, "asprintf failed in yyerror");
	}

	flog (LOG_ERR, "%s", str3);

	if (str1) {
		free(str1);
	}

	if (str2) {
		free(str2);
	}

	if (str3) {
		free(str3);
	}
}

struct fill_by_indexer {
	struct Interface ** array;
	int index;
	unsigned int * flags;
};

void fill_by_index(struct Interface * iface, void * data);
void fill_by_index(struct Interface * iface, void * data)
{
	struct fill_by_indexer * fbi = (struct fill_by_indexer *)data;
	fbi->array[fbi->index++] = iface;
	iface->flags = fbi->flags;
}

#define container_of(ptr, type, member) ({                      \
        const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
        (type *)( (char *)__mptr - offsetof(type,member) );})
int iface_tree_insert(struct rb_root *root, struct Interface *data);
int iface_tree_insert(struct rb_root *root, struct Interface *data)
{
  	struct rb_node **new = &(root->rb_node), *parent = NULL;

  	/* Figure out where to put new node */
  	while (*new) {
  		struct Interface *this = container_of(*new, struct Interface, rb_node);
  		int result = timevaldiff(&data->next_multicast, &this->next_multicast);

		parent = *new;
  		if (result <= 0)
  			new = &((*new)->rb_left);
  		else
  			new = &((*new)->rb_right);
  	}

  	/* Add new node and rebalance tree. */
  	rb_link_node(&data->rb_node, parent, new);
  	rb_insert_color(&data->rb_node, root);

	return 0;
}


struct Interface *my_search(struct rb_root *root, struct timeval *tv);
struct Interface *my_search(struct rb_root *root, struct timeval *tv)
{
  	struct rb_node *node = root->rb_node;

  	while (node) {
  		struct Interface *data = container_of(node, struct Interface, rb_node);
  		int result = timevaldiff(&data->next_multicast, tv);

		if (result < 0)
  			node = node->rb_left;
		else if (result > 0)
  			node = node->rb_right;
		else
  			return data;
	}
	return NULL;
}


void build_tree(struct Interface * iface, void * data);
void build_tree(struct Interface * iface, void * data)
{
	struct rb_root * iface_tree = (struct rb_root*)data;
	iface_tree_insert(iface_tree, iface);
}

struct interfaces * readin_config(char const *fname)
{
	struct interfaces * interfaces = 0;
	struct yydata yydata;
	FILE * in;

	in = fopen(fname, "r");

	if (!in)
	{
		flog(LOG_ERR, "can't open %s: %s", fname, strerror(errno));
		return 0;
	}

	memset(&yydata, 0, sizeof(yydata));
	yydata.filename = fname;
	yylex_init(&yydata.scaninfo);
	yyset_in(in, yydata.scaninfo);

	if (yyparse(&yydata) != 0) {
		flog(LOG_ERR, "error parsing or activating the config file: %s", fname);
	}
	else {
		dlog(LOG_DEBUG, 1, "config file syntax ok.");

		if (yydata.IfaceList) {
			interfaces = malloc(sizeof(struct interfaces));
			if (!interfaces) {
				flog(LOG_ERR, "Unable to allocate memory for %d interfaces", yydata.interface_count);
				exit(1);
			}
			memset(interfaces, 0, sizeof(struct interfaces));
			interfaces->iface_tree = RB_ROOT;
			interfaces->IfaceList = yydata.IfaceList;
			interfaces->by_index = malloc(yydata.interface_count * sizeof(struct Interface*));
			if (!interfaces->by_index) {
				flog(LOG_ERR, "Unable to allocate memory for %d interfaces", yydata.interface_count);
				exit(1);
			}
			struct fill_by_indexer fbi = {interfaces->by_index, 0, &interfaces->flags};
			for_each_iface(interfaces, fill_by_index, &fbi);
			for_each_iface(interfaces, build_tree, &interfaces->iface_tree);
			interfaces->count = yydata.interface_count;
			interfaces->flags = 1;
		}
		
		dlog(LOG_INFO, 3, "Loaded %d Interfaces", interfaces->count);
	}

	yylex_destroy(yydata.scaninfo);

	fclose(in);

	return interfaces;
}

