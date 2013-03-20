/*
 *
 * Author: Sylvain Afchain <safchain@gmail.com>, (C) 2008 - 2013
 *
 * Copyright: See COPYING file that comes with this distribution
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include <types.h>
#include <error.h>

inline void mm_free0(void **ptr) {
        free(*ptr);
        *ptr = NULL;
}
