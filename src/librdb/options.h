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

#ifndef __RDB_OPTIONS_H
#define __RDB_OPTIONS_H

#include <types.h>
#include <hl.h>

#ifdef HAVE_GEOIP
#include <GeoIP.h>
#endif

struct rdb_addr_s {
	char *addr;
	uint16_t port;
};

struct rdb_options_s {
    LIST *addrs;

    uint16_t uid;
    uint16_t gid;
    char *pid;

    char *log_file;
    uint32_t log_maxfile;
    uint32_t log_level;

    uint32_t nthreads;

    uint16_t websrv_port;
    char *websrv_xslt;

#ifdef HAVE_GEOIP
    GeoIP * gi;
#endif
};

/* prototypes */
void rdb_options_add_xc(HCODE *);

#endif
