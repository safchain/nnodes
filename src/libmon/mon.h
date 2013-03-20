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

#ifndef __MON_H
#define __MON_H

#include <stdio.h>
#include <types.h>
#include <hl.h>

#define MON_TYPE_PIPE 1

#define MON_STATUS_OK 0
#define MON_STATUS_KO 1

typedef struct mon_s MON;

struct pipe_s {
    char *cmd;

    int pid;
    int ppfd[2];
    int pcfd[2];

    char buffer[255];
};

struct mon_s {
	char *name;

	int efd;
    uint32_t refresh;
    uint32_t type;
    uint32_t status;
    uint32_t pending;

    uint32_t try;
    uint32_t protection;

    union mon_u {
        struct pipe_s pipe;
    } mon;

    LIST *rrs;
};

/* prototypes */
int mon_initialize(struct mon_s *);
int mon_prepare(struct mon_s *, int);
int mon_fetch_result(struct mon_s *);
int mon_execute(struct mon_s *);
int mon_terminate (struct mon_s *);

#endif
