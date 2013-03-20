/*
 *
 * Author: Sylvain Afchain <safchain@gmail.com>, (C) 2008
 *
 * Copyright: See COPYING file that comes with this distribution
 *
 */

#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <error.h>

#include <mm.h>
#include <misc.h>
#include <rdb.h>
#include <nnodes.h>
#include <log.h>

static char *config = NULL;
static RDB *rdb = NULL;
LOG *logg = NULL;

static int _usage(char *bin) {
	fprintf(stderr, "Usage: %s --cf config\n", bin);

	return -1;
}

int main(int argc, char **argv) {
	struct option long_options[] = { { "cf", 1, 0, 0 }, { 0, 0, 0, 0 } };
	int idx;
	char c;

	while (1) {
		c = getopt_long(argc, argv, "", long_options, &idx);
		if (c == -1)
			break;

		switch (c) {
		case 0:
			if (strcmp(long_options[idx].name, "cf") == 0)
				config = strdup(optarg);
			break;
		default:
			return _usage(argv[0]);

		}
	}

	if (!config)
		return _usage(argv[0]);

	for (idx = 0; idx != 1; idx++) {
		rdb = rdb_read(config);
		rdb_free(rdb);
	}

	return 0;
}
