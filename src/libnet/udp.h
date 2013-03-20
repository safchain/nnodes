/*
 *
 * Author: Sylvain Afchain <safchain@gmail.com>, (C) 2008 - 2013
 *
 * Copyright: See COPYING file that comes with this distribution
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifndef __UDP_H
#define __UDP_H

#include <types.h>

#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define SOCKUTP	struct sockutp_s

struct sockutp_s {
    int udp_sock;
    int tcp_sock;
};

/* prototypes */
SOCKUTP *net_bind(const char *, uint16_t);
void net_unbind(SOCKUTP *);

#endif
