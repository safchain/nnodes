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

#ifndef __NNODES_WRITE_H
#define __NNODES_WRITE_H

#include <types.h>
#include <hl.h>

/* prototype */
int nnodes_wr_header (char *, uint32_t, struct dns_header_s *);
int nnodes_wr_qdsections (char *, uint32_t, LIST *);
int nnodes_wr_rr (char *, uint32_t, struct dns_rr_s *, char *);

#endif
