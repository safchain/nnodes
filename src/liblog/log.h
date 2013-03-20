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

#ifndef __LOG_H
#define __LOG_H

#include <stdio.h>

#include <types.h>

#define LOG_LEVEL_ERR		1
#define	LOG_LEVEL_INFO		2
#define LOG_LEVEL_WARNING	3
#define LOG_LEVEL_DEBUG		4

typedef struct log_s LOG;

extern LOG *logg;

#define LOG_ERR(...) log_printf(logg, LOG_LEVEL_ERR, __VA_ARGS__)
#define LOG_INFO(...) log_printf(logg, LOG_LEVEL_INFO, __VA_ARGS__)
#define LOG_DEBUG(...) log_printf(logg, LOG_LEVEL_DEBUG, __VA_ARGS__)
#define LOG_WARN(...) log_printf(logg, LOG_LEVEL_WARNING, __VA_ARGS__)

struct log_s {
	char *filename;
	FILE *fp;

    uint32_t inotify_fd;
    uint32_t wd;

    uint32_t maxsize;
    uint32_t level;
};

/* prototypes */
LOG *log_open(char *, uint32_t, uint32_t);
void log_printf(LOG *, uint32_t, char *, ...);
void log_close (LOG *);

#endif
