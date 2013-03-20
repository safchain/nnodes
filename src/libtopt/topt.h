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

#ifndef __TOPT_H
#define __TOPT_H

typedef enum {
        STR2AV_OK = 0,
        STR2AV_UNBALANCED_QUOTE,
        STR2AV_MEMORY_ERROR,
        STR2AV_FATAL_ERROR
} str_to_argv_rtv_t;


/* prototypes */
str_to_argv_rtv_t topt_str_to_argv(const char *, int *, char ***);
void topt_free_argv(char **);

#endif
