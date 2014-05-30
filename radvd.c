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

#include "config.h"
#include "includes.h"
#include "radvd.h"
#include "pathnames.h"

#ifdef HAVE_NETLINK
#include "netlink.h"
#endif

#include <poll.h>
#include <libdaemon/dfork.h>
#include <libdaemon/dpid.h>

#ifdef HAVE_GETOPT_LONG

/* *INDENT-OFF* */
static char usage_str[] = {
"\n"
"  -c, --configtest        Parse the config file and exit.\n"
"  -C, --config=PATH       Sets the config file.  Default is /etc/radvd.d.\n"
"  -d, --debug=NUM         Sets the debug level.  Values can be 1, 2, 3, 4 or 5.\n"
"  -f, --facility=NUM      Sets the logging facility.\n"
"  -h, --help              Show this help screen.\n"
"  -l, --logfile=PATH      Sets the log file.\n"
"  -m, --logmethod=X       Sets the log method to one of: syslog, stderr, stderr_syslog, logfile, or none.\n"
"  -p, --pidfile=PATH      Sets the pid file.\n"
"  -t, --chrootdir=PATH    Chroot to the specified path.\n"
"  -u, --username=USER     Switch to the specified user.\n"
"  -n, --nodaemon          Prevent the daemonizing.\n"
"  -v, --version           Print the version and quit.\n"
};

static struct option prog_opt[] = {
	{"debug", 1, 0, 'd'},
	{"configtest", 0, 0, 'c'},
	{"config", 1, 0, 'C'},
	{"pidfile", 1, 0, 'p'},
	{"logfile", 1, 0, 'l'},
	{"logmethod", 1, 0, 'm'},
	{"facility", 1, 0, 'f'},
	{"username", 1, 0, 'u'},
	{"chrootdir", 1, 0, 't'},
	{"version", 0, 0, 'v'},
	{"help", 0, 0, 'h'},
	{"nodaemon", 0, 0, 'n'},
	{NULL, 0, 0, 0}
};

#else

static char usage_str[] = {
"[-hsvcn] [-d level] [-C config_path] [-m log_method] [-l log_file]\n"
"\t[-f facility] [-p pid_file] [-u username] [-t chrootdir]"

};

/* *INDENT-ON* */
#endif

static volatile int sighup_received = 0;
static volatile int sigterm_received = 0;
static volatile int sigint_received = 0;
static volatile int sigusr1_received = 0;

void sighup_handler(int sig);
void sigterm_handler(int sig);
void sigint_handler(int sig);
void sigusr1_handler(int sig);
void timer_handler(int sock, struct Interface *iface);
void config_interface(struct Interface *iface);
void config_ifaces(struct Interface *ifaces);
void kickoff_adverts(int sock, struct Interface *iface);
void stop_advert_foo(struct Interface *iface, void *data);
void stop_adverts(int sock, struct Interface *ifaces);
void version(void);
void usage(char const *pname);
int drop_root_privileges(const char *);
int check_conffile_perm(const char *, const char *);
int check_confpath_perm(const char *, const char *);
const char *radvd_get_pidfile(void);
void setup_iface_foo(struct Interface *iface, void *data);
void setup_ifaces(int sock, struct Interface *ifaces);
void main_loop(int sock, struct Interface *ifaces, char const *conf_path);
void reset_prefix_lifetimes_foo(struct Interface *iface, void *data);
void reset_prefix_lifetimes(struct Interface *ifaces);
struct Interface *reload_config(int sock, struct Interface *ifaces, char const *conf_path);

int main(int argc, char *argv[])
{
	struct Interface *ifaces = NULL;
	int sock = -1;
	int c, log_method;
	char *logfile;
	int facility;
	char *username = NULL;
	char *chrootdir = NULL;
	int configtest = 0;
	int daemonize = 1;
	int force_pid_file = 0;
#ifdef HAVE_GETOPT_LONG
	int opt_idx;
#endif

	char const *pname = ((pname = strrchr(argv[0], '/')) != NULL) ? pname + 1 : argv[0];

	srand((unsigned int)time(NULL));

	log_method = L_STDERR_SYSLOG;
	logfile = PATH_RADVD_LOG;
	char const *conf_path = PATH_RADVD_CONF;
	facility = LOG_FACILITY;
	daemon_pid_file_ident = PATH_RADVD_PID;	/* libdaemon defines daemon_pid_file_ident */

	/* parse args */
#define OPTIONS_STR "d:C:l:m:p:t:u:vhcn"
#ifdef HAVE_GETOPT_LONG
	while ((c = getopt_long(argc, argv, OPTIONS_STR, prog_opt, &opt_idx)) > 0)
#else
	while ((c = getopt(argc, argv, OPTIONS_STR)) > 0)
#endif
	{
		switch (c) {
		case 'C':
			conf_path = optarg;
			break;
		case 'd':
			set_debuglevel(atoi(optarg));
			break;
		case 'f':
			facility = atoi(optarg);
			break;
		case 'l':
			logfile = optarg;
			break;
		case 'p':
			daemon_pid_file_ident = optarg;
			force_pid_file = 1;
			break;
		case 'm':
			if (!strcmp(optarg, "syslog")) {
				log_method = L_SYSLOG;
			} else if (!strcmp(optarg, "stderr_syslog")) {
				log_method = L_STDERR_SYSLOG;
			} else if (!strcmp(optarg, "stderr")) {
				log_method = L_STDERR;
			} else if (!strcmp(optarg, "logfile")) {
				log_method = L_LOGFILE;
			} else if (!strcmp(optarg, "none")) {
				log_method = L_NONE;
			} else {
				fprintf(stderr, "%s: unknown log method: %s\n", pname, optarg);
				exit(1);
			}
			break;
		case 't':
			chrootdir = strdup(optarg);
			break;
		case 'u':
			username = strdup(optarg);
			break;
		case 'v':
			version();
			break;
		case 'c':
			configtest = 1;
			break;
		case 'n':
			daemonize = 0;
			break;
		case 'h':
			usage(pname);
#ifdef HAVE_GETOPT_LONG
		case ':':
			fprintf(stderr, "%s: option %s: parameter expected\n", pname, prog_opt[opt_idx].name);
			exit(1);
#endif
		case '?':
			exit(1);
		}
	}

	if (chrootdir) {
		if (!username) {
			fprintf(stderr, "Chroot as root is not safe, exiting\n");
			exit(1);
		}

		if (chroot(chrootdir) == -1) {
			perror("chroot");
			exit(1);
		}

		if (chdir("/") == -1) {
			perror("chdir");
			exit(1);
		}
		/* username will be switched later */
	}

	if (configtest) {
		set_debuglevel(1);
		log_method = L_STDERR;
	}

	if (log_open(log_method, pname, logfile, facility) < 0) {
		perror("log_open");
		exit(1);
	}

	if (!configtest) {
		flog(LOG_INFO, "version %s started", VERSION);

		/* Calling privsep here, before opening the socket and reading the config
		 * file, ensures we're not going to be wasting resources in the privsep
		 * process. */
		dlog(LOG_DEBUG, 3, "Initializing privsep");
		if (privsep_init() < 0) {
			flog(LOG_INFO, "Failed to initialize privsep.");
			exit(1);
		}
	}

	/* check that 'other' cannot write the file
	 * for non-root, also that self/own group can't either
	 */
	if (check_confpath_perm(username, conf_path) < 0) {
		if (get_debuglevel() == 0) {
			flog(LOG_ERR, "Exiting, permissions on conf_file invalid.");
			exit(1);
		} else
			flog(LOG_WARNING, "Insecure file permissions, but continuing anyway");
	}

	/* parse config file */
	if ((ifaces = readin_config(conf_path)) == 0) {
		flog(LOG_ERR, "Exiting, failed to read config file.");
		exit(1);
	}

	if (configtest) {
		free_ifaces(ifaces);
		exit(0);
	}

	/* get a raw socket for sending and receiving ICMPv6 messages */
	sock = open_icmpv6_socket();
	if (sock < 0) {
		perror("open_icmpv6_socket");
		exit(1);
	}

	/* if we know how to do it, check whether forwarding is enabled */
	if (check_ip6_forwarding()) {
		flog(LOG_WARNING, "IPv6 forwarding seems to be disabled, but continuing anyway.");
	}

	daemon_pid_file_proc = radvd_get_pidfile;

	/*
	 * okay, config file is read in, socket and stuff is setup, so
	 * lets fork now...
	 */
	dlog(LOG_DEBUG, 3, "radvd startup PID is %d", getpid());
	if (daemonize) {
		pid_t pid;

		if (daemon_retval_init()) {
			flog(LOG_ERR, "Could not initialize daemon IPC.");
			exit(1);
		}

		/* TODO: research daemon_log (in libdaemon) and have it log the same as radvd. */
		pid = daemon_fork();

		if (-1 == pid) {
			flog(LOG_ERR, "Could not fork: %s", strerror(errno));
			daemon_retval_done();
			exit(1);
		}

		if (0 < pid) {
			switch (daemon_retval_wait(1)) {
			case 0:
				dlog(LOG_DEBUG, 3, "radvd PID is %d", pid);
				exit(0);
				break;

			case 1:
				flog(LOG_ERR, "radvd already running, terminating.");
				exit(1);
				break;

			case 2:
				flog(LOG_ERR, "Cannot create radvd PID file, terminating: %s", strerror(errno));
				exit(2);
				break;

			default:
				flog(LOG_ERR, "Could not daemonize.");
				exit(-1);
				break;
			}
		}

		if (daemon_pid_file_is_running() >= 0) {
			daemon_retval_send(1);
			exit(1);
		}

		if (daemon_pid_file_create()) {
			daemon_retval_send(2);
			exit(2);
		}

		daemon_retval_send(0);
	} else {
		if (force_pid_file) {
			if (daemon_pid_file_is_running() >= 0) {
				flog(LOG_ERR, "radvd already running, terminating.");
				exit(1);
			}

			if (daemon_pid_file_create()) {
				flog(LOG_ERR, "Cannot create radvd PID file, terminating: %s", strerror(errno));
				exit(2);
			}
		}

		dlog(LOG_DEBUG, 3, "radvd PID is %d", getpid());
	}

	if (username) {
		if (drop_root_privileges(username) < 0) {
			perror("drop_root_privileges");
			flog(LOG_ERR, "unable to drop root privileges");
			exit(1);
		}
	}

	setup_ifaces(sock, ifaces);
	main_loop(sock, ifaces, conf_path);
	flog(LOG_INFO, "sending stop adverts");
	stop_adverts(sock, ifaces);
	if (daemonize) {
		flog(LOG_INFO, "removing %s", daemon_pid_file_ident);
		daemon_pid_file_remove();
	} else if (force_pid_file) {
		flog(LOG_INFO, "removing %s", radvd_get_pidfile());
		daemon_pid_file_remove();
	}

	flog(LOG_INFO, "returning from radvd main");
	log_close();
	return 0;
}

/* This function is copied from dpid.c (in libdaemon) and renamed. */
const char *radvd_get_pidfile(void)
{
#ifdef HAVE_ASPRINTF
	static char *fn = NULL;
	free(fn);
	asprintf(&fn, "%s", daemon_pid_file_ident ? daemon_pid_file_ident : "unknown");
#else
	static char fn[PATH_MAX];
	snprintf(fn, sizeof(fn), "%s", daemon_pid_file_ident ? daemon_pid_file_ident : "unknown");
#endif

	return fn;
}

void main_loop(int sock, struct Interface *ifaces, char const *conf_path)
{
	struct pollfd fds[2];
	sigset_t sigmask;
	sigset_t sigempty;
	struct sigaction sa;

	sigemptyset(&sigempty);

	sigemptyset(&sigmask);
	sigaddset(&sigmask, SIGHUP);
	sigaddset(&sigmask, SIGTERM);
	sigaddset(&sigmask, SIGINT);
	sigaddset(&sigmask, SIGUSR1);
	sigprocmask(SIG_BLOCK, &sigmask, NULL);

	sa.sa_handler = sighup_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sigaction(SIGHUP, &sa, 0);

	sa.sa_handler = sigterm_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sigaction(SIGTERM, &sa, 0);

	sa.sa_handler = sigint_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sigaction(SIGINT, &sa, 0);

	sa.sa_handler = sigusr1_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sigaction(SIGUSR1, &sa, 0);

	memset(fds, 0, sizeof(fds));

	fds[0].fd = sock;
	fds[0].events = POLLIN;

#if HAVE_NETLINK
	fds[1].fd = netlink_socket();
	fds[1].events = POLLIN;
#else
	fds[1].fd = -1;
#endif

	for (;;) {
		struct timespec *tsp = 0;

		struct Interface * next_iface_to_expire = find_iface_by_time(ifaces);
		if (next_iface_to_expire) {
			static struct timespec ts;
			int timeout = next_time_msec(next_iface_to_expire);
			ts.tv_sec = timeout / 1000;
			ts.tv_nsec = (timeout - 1000 * ts.tv_sec) * 1000000;
			tsp = &ts;
			dlog(LOG_DEBUG, 1, "polling for %g seconds. Next iface is %s.", timeout / 1000.0,
			     next_iface_to_expire->Name);
		} else {
			dlog(LOG_DEBUG, 1, "No iface is next. Polling indefinitely.");
		}

		int rc = ppoll(fds, sizeof(fds) / sizeof(fds[0]), tsp, &sigempty);

		if (rc > 0) {
#ifdef HAVE_NETLINK
			if (fds[1].revents & (POLLERR | POLLHUP | POLLNVAL)) {
				flog(LOG_WARNING, "socket error on fds[1].fd");
			} else if (fds[1].revents & POLLIN) {
				process_netlink_msg(fds[1].fd, ifaces);
			}
#endif

			if (fds[0].revents & (POLLERR | POLLHUP | POLLNVAL)) {
				flog(LOG_WARNING, "socket error on fds[0].fd");
			} else if (fds[0].revents & POLLIN) {
				int len, hoplimit;
				struct sockaddr_in6 rcv_addr;
				struct in6_pktinfo *pkt_info = NULL;
				unsigned char msg[MSG_SIZE_RECV];

				len = recv_rs_ra(sock, msg, &rcv_addr, &pkt_info, &hoplimit);
				if (len > 0 && pkt_info) {
					process(sock, ifaces, msg, len, &rcv_addr, pkt_info, hoplimit);
				} else if (!pkt_info) {
					dlog(LOG_INFO, 4, "recv_rs_ra returned null pkt_info.");
				} else if (len <= 0) {
					dlog(LOG_INFO, 4, "recv_rs_ra returned len <= 0: %d", len);
				}
			}
		} else if (rc == 0) {
			if (next_iface_to_expire)
				timer_handler(sock, next_iface_to_expire);
		} else if (rc == -1) {
			dlog(LOG_INFO, 3, "poll returned early: %s", strerror(errno));
		}

		if (sigint_received) {
			flog(LOG_WARNING, "Exiting, %d sigint(s) received.", sigint_received);
			break;
		}

		if (sigterm_received) {
			flog(LOG_WARNING, "Exiting, %d sigterm(s) received.", sigterm_received);
			break;
		}

		if (sighup_received) {
			dlog(LOG_INFO, 3, "sig hup received.");
			ifaces = reload_config(sock, ifaces, conf_path);
			sighup_received = 0;
		}

		if (sigusr1_received) {
			dlog(LOG_INFO, 3, "sig usr1 received.");
			reset_prefix_lifetimes(ifaces);
			sigusr1_received = 0;
		}

	}
}

void timer_handler(int sock, struct Interface *iface)
{
	double next;

	dlog(LOG_DEBUG, 1, "timer_handler called for %s", iface->Name);

	if (send_ra_forall(sock, iface, NULL) != 0) {
		dlog(LOG_DEBUG, 4, "send_ra_forall failed on interface %s", iface->Name);
	}

	next = rand_between(iface->MinRtrAdvInterval, iface->MaxRtrAdvInterval);

	reschedule_iface(iface, next);
}

void config_interface(struct Interface *iface)
{
	if (iface->AdvLinkMTU)
		set_interface_linkmtu(iface->Name, iface->AdvLinkMTU);
	if (iface->AdvCurHopLimit)
		set_interface_curhlim(iface->Name, iface->AdvCurHopLimit);
	if (iface->AdvReachableTime)
		set_interface_reachtime(iface->Name, iface->AdvReachableTime);
	if (iface->AdvRetransTimer)
		set_interface_retranstimer(iface->Name, iface->AdvRetransTimer);
}

void kickoff_adverts(int sock, struct Interface *iface)
{
	double next;		/* TODO: double? */

	/*
	 *      send initial advertisement and set timers
	 */

	gettimeofday(&iface->last_ra_time, NULL);

	if (iface->UnicastOnly)
		return;

	gettimeofday(&iface->last_multicast, NULL);

	/* send an initial advertisement */
	if (send_ra_forall(sock, iface, NULL) == 0) {
		dlog(LOG_DEBUG, 4, "send_ra_forall failed on interface %s", iface->Name);
	}

	next = min(MAX_INITIAL_RTR_ADVERT_INTERVAL, iface->MaxRtrAdvInterval);
	reschedule_iface(iface, next);
}

void stop_advert_foo(struct Interface *iface, void *data)
{
	int sock = *(int *)data;

	if (!iface->UnicastOnly) {
		/* send a final advertisement with zero Router Lifetime */
		dlog(LOG_DEBUG, 4, "stopping all adverts on %s.", iface->Name);
		iface->cease_adv = 1;
		send_ra_forall(sock, iface, NULL);
	}
}

void stop_adverts(int sock, struct Interface *ifaces)
{
	/*
	 *      send final RA (a SHOULD in RFC4861 section 6.2.5)
	 */
	for_each_iface(ifaces, stop_advert_foo, &sock);
}

int setup_iface(int sock, struct Interface *iface)
{
	iface->ready = 0;

	/* Check IFF_UP, IFF_RUNNING and IFF_MULTICAST */
	if (check_device(sock, iface) < 0) {
		return -1;
	}

	if (update_device_index(iface) < 0) {
		return -1;
	}

	/* Set iface->if_index, iface->max_mtu and iface hardware address */
	if (update_device_info(sock, iface) < 0) {
		return -1;
	}

	/* Make sure the settings in the config file for this interface are ok (this depends
	 * on iface->max_mtu already being set). */
	if (check_iface(iface) < 0) {
		return -1;
	}

	/* Make sure this is diabled.  We don't want this interface to autoconfig using its
	 * own advert messages. */
	if (disable_ipv6_autoconfig(iface->Name)) {
		return -1;
	}

	/* Save the first link local address seen on the specified interface to iface->if_addr */
	if (setup_linklocal_addr(iface) < 0) {
		return -1;
	}

	/* join the allrouters multicast group so we get the solicitations */
	if (setup_allrouters_membership(sock, iface) < 0) {
		return -1;
	}

	iface->ready = 1;

	dlog(LOG_DEBUG, 4, "interface definition for %s is ok", iface->Name);

	return 0;
}

void setup_iface_foo(struct Interface *iface, void *data)
{
	int sock = *(int *)data;

	if (setup_iface(sock, iface) < 0) {
		if (iface->IgnoreIfMissing) {
			dlog(LOG_DEBUG, 4, "interface %s does not exist or is not set up properly, ignoring the interface",
			     iface->Name);
		} else {
			flog(LOG_ERR, "interface %s does not exist or is not set up properly", iface->Name);
			exit(1);
		}
	}

	/* TODO: call these for changed ifaces only */
	config_interface(iface);
	kickoff_adverts(sock, iface);
}

void setup_ifaces(int sock, struct Interface *ifaces)
{
	for_each_iface(ifaces, setup_iface_foo, &sock);
}

struct Interface *reload_config(int sock, struct Interface *ifaces, char const *conf_path)
{
	free_ifaces(ifaces);

	flog(LOG_INFO, "attempting to reread config file");

	ifaces = NULL;

	/* reread config file */
	if ((ifaces = readin_config(conf_path)) == 0) {
		flog(LOG_ERR, "Exiting, failed to read config file.");
		exit(1);
	}
	setup_ifaces(sock, ifaces);

	flog(LOG_INFO, "resuming normal operation");

	return ifaces;
}

void sighup_handler(int sig)
{
	sighup_received = 1;
}

void sigterm_handler(int sig)
{
	++sigterm_received;

	if (sigterm_received > 2) {
		abort();
	}
}

void sigint_handler(int sig)
{
	++sigint_received;

	if (sigint_received > 2) {
		abort();
	}
}

void sigusr1_handler(int sig)
{
	sigusr1_received = 1;
}

void reset_prefix_lifetimes_foo(struct Interface *iface, void *data)
{
	struct AdvPrefix *prefix;
	char pfx_str[INET6_ADDRSTRLEN];

	flog(LOG_INFO, "Resetting prefix lifetimes on %s", iface->Name);

	for (prefix = iface->AdvPrefixList; prefix; prefix = prefix->next) {
		if (prefix->DecrementLifetimesFlag) {
			addrtostr(&prefix->Prefix, pfx_str, sizeof(pfx_str));
			dlog(LOG_DEBUG, 4, "%s/%u%%%s plft reset from %u to %u secs", pfx_str, prefix->PrefixLen,
			     iface->Name, prefix->curr_preferredlft, prefix->AdvPreferredLifetime);
			dlog(LOG_DEBUG, 4, "%s/%u%%%s vlft reset from %u to %u secs", pfx_str, prefix->PrefixLen,
			     iface->Name, prefix->curr_validlft, prefix->AdvValidLifetime);
			prefix->curr_validlft = prefix->AdvValidLifetime;
			prefix->curr_preferredlft = prefix->AdvPreferredLifetime;
		}
	}
}

void reset_prefix_lifetimes(struct Interface *ifaces)
{
	for_each_iface(ifaces, reset_prefix_lifetimes_foo, 0);
}

int drop_root_privileges(const char *username)
{
	struct passwd *pw = NULL;
	pw = getpwnam(username);
	if (pw) {
		if (initgroups(username, pw->pw_gid) != 0 || setgid(pw->pw_gid) != 0 || setuid(pw->pw_uid) != 0) {
			flog(LOG_ERR, "Couldn't change to '%.32s' uid=%d gid=%d", username, pw->pw_uid, pw->pw_gid);
			return -1;
		}
	} else {
		flog(LOG_ERR, "Couldn't find user '%.32s'", username);
		return -1;
	}
	return 0;
}

int check_conffile_perm(const char *username, const char *conf_file)
{
	struct stat stbuf;
	struct passwd *pw = NULL;
	FILE *fp = fopen(conf_file, "r");

	if (fp == NULL) {
		flog(LOG_ERR, "can't open %s: %s", conf_file, strerror(errno));
		return -1;
	}
	fclose(fp);

	if (!username)
		username = "root";

	pw = getpwnam(username);

	if (stat(conf_file, &stbuf) || pw == NULL)
		return -1;

	if (stbuf.st_mode & S_IWOTH) {
		flog(LOG_ERR, "Insecure file permissions (writable by others): %s", conf_file);
		return -1;
	}

	/* for non-root: must not be writable by self/own group */
	if (strncmp(username, "root", 5) != 0
	    && ((stbuf.st_mode & S_IWGRP && pw->pw_gid == stbuf.st_gid)
		|| (stbuf.st_mode & S_IWUSR && pw->pw_uid == stbuf.st_uid))) {
		flog(LOG_ERR, "Insecure file permissions (writable by self/group): %s", conf_file);
		return -1;
	}

	return 0;
}

int check_confpath_perm(const char *username, const char *conf_path)
{
	/* TODO: call check_conffile_parm for each file in conf_path */
	return 0;
}

void version(void)
{
	fprintf(stderr, "Version: %s\n\n", VERSION);
	fprintf(stderr, "Compiled in settings:\n");
	fprintf(stderr, "  default config path		\"%s\"\n", PATH_RADVD_CONF);
	fprintf(stderr, "  default pidfile		\"%s\"\n", PATH_RADVD_PID);
	fprintf(stderr, "  default logfile		\"%s\"\n", PATH_RADVD_LOG);
	fprintf(stderr, "  default syslog facility	%d\n", LOG_FACILITY);
	fprintf(stderr, "Please send bug reports or suggestions to %s.\n", CONTACT_EMAIL);

	exit(1);
}

void usage(char const *pname)
{
	fprintf(stderr, "usage: %s %s\n", pname, usage_str);
	exit(1);
}
