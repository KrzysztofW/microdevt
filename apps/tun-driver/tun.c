/*
 * microdevt - Microcontroller Development Toolkit
 *
 * Copyright (c) 2017, Krzysztof Witek
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * The full GNU General Public License is included in this distribution in
 * the file called "LICENSE".
 *
*/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <assert.h>

#include <sys/socket.h>
#include <linux/if.h>
#include <linux/if_tun.h>

#include "tun.h"

void tun_alloc(char *dev, struct pollfd *tun_fds)
{
	struct ifreq ifr;

	assert(dev != NULL);
	if ((tun_fds[0].fd = open("/dev/net/tun", O_RDWR)) < 0) {
		fprintf(stderr, "%s: cannot open tun device (%m)\n", __func__);
		exit(EXIT_FAILURE);
	}

	memset(&ifr, 0, sizeof(ifr));
	ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
	strncpy(ifr.ifr_name, dev, IFNAMSIZ);
	if (ioctl(tun_fds[0].fd, TUNSETIFF, (void *) &ifr) < 0) {
		fprintf(stderr, "%s: cannot ioctl tun device (%m)\n", __func__);
		exit(EXIT_FAILURE);
	}
	strncpy(dev, ifr.ifr_name, IFNAMSIZ);
	tun_fds[0].events = POLLIN;
}
