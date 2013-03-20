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

#ifndef __RDB_ZONES_H
#define __RDB_ZONES_H

#include <types.h>
#include <hl.h>

struct rdb_zones_s {
	uint32_t 	ttl;
};
extern struct rdb_zones_s rdb_zones;

/* prototypes */
void rdb_zones_add_xc(HCODE *);

#endif
