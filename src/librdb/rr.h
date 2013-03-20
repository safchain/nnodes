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

#ifndef __RDB_ASECTION_H
#define __RDB_ASECTION_H

#include <types.h>
#include <hl.h>
#include <dns.h>

/* prototypes */
int rdb_add_rr(HCODE *, char *, struct dns_rr_s *);

#endif
