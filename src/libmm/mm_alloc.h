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

#ifndef __MM_ALLOC_H
#define __MM_ALLOC_H

#include "types.h"

/* prototypes */
void *mm_alloc0(uint32_t);
void mm_free0(void **);

#endif
