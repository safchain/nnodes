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

#ifndef __NNODES_READ_H
#define __NNODES_READ_H

#include <dns.h>

/* prototypes */
int nnodes_rd_questions (char *, struct dns_questions_s *);

#endif
