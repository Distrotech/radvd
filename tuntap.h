
#pragma once
#include <string.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/if.h>
#include <linux/if_tun.h> /* TUNSETIFF */
#include <fcntl.h> /* O_RDWR */
#include <unistd.h>

int tun_alloc(char *dev, int flags);

