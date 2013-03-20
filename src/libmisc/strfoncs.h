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

#ifndef __STRFONCS_H
#define __STRFONCS_H

#include <types.h>
#include <mm.h>

/* prototypes */
char *str_dup_pool_alloc(MM_POOL *, const char *);
uint32_t str_lcpy(char *, const char *, uint32_t);
uint32_t str_lcat(char *, const char *, uint32_t);

#endif
