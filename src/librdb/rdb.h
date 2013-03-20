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

#ifndef __RDB_H
#define __RDB_H

#include <hl.h>
#include <dns.h>

#include "options.h"

#define RDB struct rdb_s

struct rdb_s {
    struct rdb_options_s *options;
    HCODE *acls;
    HCODE *monitors;
    HCODE *zones;
};

/* prototypes */
uint32_t rdb_rrs_available(LIST *);
RDB *rdb_read(char *);
int rdb_free(RDB *);

#endif
