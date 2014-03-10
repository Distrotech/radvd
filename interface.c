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

void iface_init_defaults(struct Interface *iface)
{
	memset(iface, 0, sizeof(struct Interface));

	iface->cease_adv = 0;

	iface->ready = 0;
	iface->IgnoreIfMissing = DFLT_IgnoreIfMissing;
	iface->AdvSendAdvert = DFLT_AdvSendAdv;
	iface->MaxRtrAdvInterval = DFLT_MaxRtrAdvInterval;
	iface->AdvSourceLLAddress = DFLT_AdvSourceLLAddress;
	iface->AdvReachableTime = DFLT_AdvReachableTime;
	iface->AdvRetransTimer = DFLT_AdvRetransTimer;
	iface->AdvLinkMTU = DFLT_AdvLinkMTU;
	iface->AdvCurHopLimit = DFLT_AdvCurHopLimit;
	iface->AdvIntervalOpt = DFLT_AdvIntervalOpt;
	iface->AdvHomeAgentInfo = DFLT_AdvHomeAgentInfo;
	iface->AdvHomeAgentFlag = DFLT_AdvHomeAgentFlag;
	iface->HomeAgentPreference = DFLT_HomeAgentPreference;
	iface->MinDelayBetweenRAs = DFLT_MinDelayBetweenRAs;
	iface->AdvMobRtrSupportFlag = DFLT_AdvMobRtrSupportFlag;

	iface->MinRtrAdvInterval = -1;
	iface->AdvDefaultLifetime = -1;
	iface->AdvDefaultPreference = DFLT_AdvDefaultPreference;
	iface->HomeAgentLifetime = -1;
}

void prefix_init_defaults(struct AdvPrefix *prefix)
{
	memset(prefix, 0, sizeof(struct AdvPrefix));

	prefix->AdvOnLinkFlag = DFLT_AdvOnLinkFlag;
	prefix->AdvAutonomousFlag = DFLT_AdvAutonomousFlag;
	prefix->AdvRouterAddr = DFLT_AdvRouterAddr;
	prefix->AdvValidLifetime = DFLT_AdvValidLifetime;
	prefix->AdvPreferredLifetime = DFLT_AdvPreferredLifetime;
	prefix->DeprecatePrefixFlag = DFLT_DeprecatePrefixFlag;
	prefix->DecrementLifetimesFlag = DFLT_DecrementLifetimesFlag;
	prefix->if6to4[0] = 0;
	prefix->enabled = 1;

	prefix->curr_validlft = prefix->AdvValidLifetime;
	prefix->curr_preferredlft = prefix->AdvPreferredLifetime;
}

void route_init_defaults(struct AdvRoute *route, struct Interface *iface)
{
	memset(route, 0, sizeof(struct AdvRoute));

	route->AdvRouteLifetime = DFLT_AdvRouteLifetime(iface);
	route->AdvRoutePreference = DFLT_AdvRoutePreference;
	route->RemoveRouteFlag = DFLT_RemoveRouteFlag;
}

void rdnss_init_defaults(struct AdvRDNSS *rdnss, struct Interface *iface)
{
	memset(rdnss, 0, sizeof(struct AdvRDNSS));

	rdnss->AdvRDNSSLifetime = DFLT_AdvRDNSSLifetime(iface);
	rdnss->AdvRDNSSNumber = 0;
	rdnss->FlushRDNSSFlag = DFLT_FlushRDNSSFlag;
}

void dnssl_init_defaults(struct AdvDNSSL *dnssl, struct Interface *iface)
{
	memset(dnssl, 0, sizeof(struct AdvDNSSL));

	dnssl->AdvDNSSLLifetime = DFLT_AdvDNSSLLifetime(iface);
	dnssl->FlushDNSSLFlag = DFLT_FlushDNSSLFlag;
}

int check_iface(struct Interface *iface)
{
	struct AdvPrefix *prefix;
	struct AdvRoute *route;
	int res = 0;
	int MIPv6 = 0;

	/* Check if we use Mobile IPv6 extensions */
	if (iface->AdvHomeAgentFlag || iface->AdvHomeAgentInfo || iface->AdvIntervalOpt) {
		MIPv6 = 1;
		flog(LOG_INFO, "using Mobile IPv6 extensions");
	}

	prefix = iface->AdvPrefixList;
	while (!MIPv6 && prefix) {
		if (prefix->AdvRouterAddr) {
			MIPv6 = 1;
		}
		prefix = prefix->next;
	}

	if (iface->MinRtrAdvInterval < 0)
		iface->MinRtrAdvInterval = DFLT_MinRtrAdvInterval(iface);

	if ((iface->MinRtrAdvInterval < (MIPv6 ? MIN_MinRtrAdvInterval_MIPv6 : MIN_MinRtrAdvInterval)) || (iface->MinRtrAdvInterval > MAX_MinRtrAdvInterval(iface))) {
		flog(LOG_ERR,
		     "MinRtrAdvInterval for %s (%.2f) must be at least %.2f but no more than 3/4 of MaxRtrAdvInterval (%.2f)",
		     iface->Name, iface->MinRtrAdvInterval, MIPv6 ? MIN_MinRtrAdvInterval_MIPv6 : (int)MIN_MinRtrAdvInterval, MAX_MinRtrAdvInterval(iface));
		res = -1;
	}

	if ((iface->MaxRtrAdvInterval < (MIPv6 ? MIN_MaxRtrAdvInterval_MIPv6 : MIN_MaxRtrAdvInterval))
	    || (iface->MaxRtrAdvInterval > MAX_MaxRtrAdvInterval)) {
		flog(LOG_ERR,
		     "MaxRtrAdvInterval for %s (%.2f) must be between %.2f and %d",
		     iface->Name, iface->MaxRtrAdvInterval, MIPv6 ? MIN_MaxRtrAdvInterval_MIPv6 : (int)MIN_MaxRtrAdvInterval, MAX_MaxRtrAdvInterval);
		res = -1;
	}

	if (iface->MinDelayBetweenRAs < (MIPv6 ? MIN_DELAY_BETWEEN_RAS_MIPv6 : MIN_DELAY_BETWEEN_RAS)) {
		flog(LOG_ERR,
		     "MinDelayBetweenRAs for %s (%.2f) must be at least %.2f", iface->Name, iface->MinDelayBetweenRAs, MIPv6 ? MIN_DELAY_BETWEEN_RAS_MIPv6 : MIN_DELAY_BETWEEN_RAS);
		res = -1;
	}

	if ((iface->AdvLinkMTU != 0) && ((iface->AdvLinkMTU < MIN_AdvLinkMTU) || (iface->if_maxmtu != -1 && (iface->AdvLinkMTU > iface->if_maxmtu)))) {
		flog(LOG_ERR, "AdvLinkMTU for %s (%u) must be zero or between %u and %u", iface->Name, iface->AdvLinkMTU, MIN_AdvLinkMTU, iface->if_maxmtu);
		res = -1;
	}

	if (iface->AdvReachableTime > MAX_AdvReachableTime) {
		flog(LOG_ERR, "AdvReachableTime for %s (%u) must not be greater than %u", iface->Name, iface->AdvReachableTime, MAX_AdvReachableTime);
		res = -1;
	}

	if (iface->AdvDefaultLifetime < 0)
		iface->AdvDefaultLifetime = DFLT_AdvDefaultLifetime(iface);

	if ((iface->AdvDefaultLifetime != 0) && ((iface->AdvDefaultLifetime > MAX_AdvDefaultLifetime) || (iface->AdvDefaultLifetime < MIN_AdvDefaultLifetime(iface)))) {
		flog(LOG_ERR,
		     "AdvDefaultLifetime for %s (%u) must be zero or between %u and %u",
		     iface->Name, iface->AdvDefaultLifetime, (int)MIN_AdvDefaultLifetime(iface), MAX_AdvDefaultLifetime);
		res = -1;
	}

	/* Mobile IPv6 ext */
	if (iface->HomeAgentLifetime < 0)
		iface->HomeAgentLifetime = DFLT_HomeAgentLifetime(iface);

	/* Mobile IPv6 ext */
	if (iface->AdvHomeAgentInfo) {
		if ((iface->HomeAgentLifetime > MAX_HomeAgentLifetime) || (iface->HomeAgentLifetime < MIN_HomeAgentLifetime)) {
			flog(LOG_ERR,
			     "HomeAgentLifetime for %s (%u) must be between %u and %u", iface->Name, iface->HomeAgentLifetime, MIN_HomeAgentLifetime, MAX_HomeAgentLifetime);
			res = -1;
		}
	}

	/* Mobile IPv6 ext */
	if (iface->AdvHomeAgentInfo && !(iface->AdvHomeAgentFlag)) {
		flog(LOG_ERR, "AdvHomeAgentFlag for %s must be set with HomeAgentInfo", iface->Name);
		res = -1;
	}
	if (iface->AdvMobRtrSupportFlag && !(iface->AdvHomeAgentInfo)) {
		flog(LOG_ERR, "AdvHomeAgentInfo for %s must be set with AdvMobRtrSupportFlag", iface->Name);
		res = -1;
	}

	/* XXX: need this? prefix = iface->AdvPrefixList; */

	while (prefix) {
		if (prefix->PrefixLen > MAX_PrefixLen) {
			flog(LOG_ERR, "invalid prefix length (%u) for %s", prefix->PrefixLen, iface->Name);
			res = -1;
		}

		if (prefix->AdvPreferredLifetime > prefix->AdvValidLifetime) {
			flog(LOG_ERR, "AdvValidLifetime for %s (%u) must be " "greater than AdvPreferredLifetime for", iface->Name, prefix->AdvValidLifetime);
			res = -1;
		}

		prefix = prefix->next;
	}

	route = iface->AdvRouteList;

	while (route) {
		if (route->PrefixLen > MAX_PrefixLen) {
			flog(LOG_ERR, "invalid route prefix length (%u) for %s", route->PrefixLen, iface->Name);
			res = -1;
		}

		route = route->next;
	}

	return res;
}

/*
 * iface_index_changed will let find_iface_by_index know that
 * the by_index array may be out of order and need a resort.
 */
void iface_index_changed(struct Interface *iface)
{
	iface->interfaces->flags = 1;
}

static int compare_by_index(void const *a, void const *b)
{
	return (*(struct Interface const *const *)b)->if_index - (*(struct Interface const *const *)a)->if_index;
}

struct Interface *find_iface_by_index(void *interfaces, int index)
{
	struct interfaces *ifaces = (struct interfaces *)interfaces;
	if (ifaces->flags) {
		flog(LOG_INFO, "by_index array is dirty, resorting %d interfaces", ifaces->count);
		qsort(ifaces->by_index, ifaces->count, sizeof(struct Interface *), compare_by_index);
		ifaces->flags &= ~0x1;
	}
	flog(LOG_INFO, "bsearching %d Interfaces", ifaces->count);
	struct Interface iface;
	memset(&iface, 0, sizeof(iface));
	iface.if_index = index;
	struct Interface *key = &iface;
	struct Interface **item = bsearch(&key, ifaces->by_index, ifaces->count, sizeof(struct Interface *), compare_by_index);
	if (!item) {
		return 0;
	}
	return *item;
}

void reschedule_iface(struct Interface * iface);
void reschedule_iface(struct Interface * iface)
{
	double next = rand_between(iface->MinRtrAdvInterval, iface->MaxRtrAdvInterval);
	rb_erase(&iface->rb_node, &iface->interfaces->iface_tree);
	iface->next_multicast = next_timeval(next);
	iface_tree_insert(&iface->interfaces->iface_tree, iface);
}

struct Interface *find_iface_by_time(void *interfaces)
{
	struct interfaces *ifaces = (struct interfaces *)interfaces;
	struct Interface *IfaceList = ifaces->IfaceList;
	struct Interface *needle = NULL;
	/* TODO: This is a great place to use a min heap. */
	if (IfaceList) {
		struct Interface *iface;
		int timeout = next_time_msec(IfaceList);
		needle = IfaceList;
		for (iface = IfaceList->next; iface; iface = iface->next) {
			int t;
			t = next_time_msec(iface);
			if (timeout > t) {
				timeout = t;
				needle = iface;
			}
		}
	}

	struct rb_node *node;
	for (node = rb_first(&ifaces->iface_tree); node; node = rb_next(node))
		printf("key=%d,name=%s\n", 
			(int)rb_entry(node, struct Interface, rb_node)->next_multicast.tv_sec,
			(char*)rb_entry(node, struct Interface, rb_node)->Name);

	return needle;
}

void for_each_iface(void *interfaces, void (*foo) (struct Interface *, void *), void *data)
{
	struct interfaces *ifaces = (struct interfaces *)interfaces;
	struct Interface *iface = ifaces->IfaceList;

	for (; iface; iface = iface->next) {
		foo(iface, data);
	}
}

void free_iface_list(struct Interface *iface)
{
	while (iface) {
		struct Interface *next_iface = iface->next;
		struct AdvPrefix *prefix;
		struct AdvRoute *route;
		struct AdvRDNSS *rdnss;
		struct AdvDNSSL *dnssl;

		dlog(LOG_DEBUG, 4, "freeing interface %s", iface->Name);

		prefix = iface->AdvPrefixList;
		while (prefix) {
			struct AdvPrefix *next_prefix = prefix->next;

			free(prefix);
			prefix = next_prefix;
		}

		route = iface->AdvRouteList;
		while (route) {
			struct AdvRoute *next_route = route->next;

			free(route);
			route = next_route;
		}

		rdnss = iface->AdvRDNSSList;
		while (rdnss) {
			struct AdvRDNSS *next_rdnss = rdnss->next;

			free(rdnss);
			rdnss = next_rdnss;
		}

		dnssl = iface->AdvDNSSLList;
		while (dnssl) {
			struct AdvDNSSL *next_dnssl = dnssl->next;
			int i;

			for (i = 0; i < dnssl->AdvDNSSLNumber; i++)
				free(dnssl->AdvDNSSLSuffixes[i]);
			free(dnssl->AdvDNSSLSuffixes);
			free(dnssl);

			dnssl = next_dnssl;
		}

		free(iface);
		iface = next_iface;
	}
}

void free_ifaces(void *interfaces)
{
	flog(LOG_INFO, "Freeing Interfaces");

	struct interfaces *ifaces = (struct interfaces *)interfaces;

	free_iface_list(ifaces->IfaceList);

	free(interfaces);
}
