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

#ifndef __RDB_ZONE_H
#define __RDB_ZONE_H

#include <types.h>
#include <hl.h>

struct rdb_zone_s {
    char *name;
    uint16_t class;
    uint32_t ttl;
};
extern struct rdb_zone_s rdb_zone;

/* prototypes */
void rdb_zone_add_xc(HCODE *);

#endif
