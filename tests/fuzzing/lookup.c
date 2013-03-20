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
	fprintf(stderr, "Usage: %s --cf config --rf raw\n", bin);

	return -1;
}

int main(int argc, char **argv) {
	struct option long_options[] = { { "cf", 1, 0, 0 }, { "rf", 1, 0, 0 }, { 0, 0, 0, 0 } };
	char req_data[BUFSIZ], buffer[BUFSIZ], *rf = NULL;
	LOOKUP *lookup;
	struct sockaddr client;
	int idx, fd;
	char c;

	while (1) {
		c = getopt_long(argc, argv, "", long_options, &idx);
		if (c == -1)
			break;

		switch (c) {
		case 0:
			if (strcmp(long_options[idx].name, "cf") == 0)
				config = strdup(optarg);
			if (strcmp(long_options[idx].name, "rf") == 0)
				rf = strdup(optarg);
			break;
		default:
			return _usage(argv[0]);

		}
	}

	if (!config || !rf)
		return _usage(argv[0]);

	if ((rdb = rdb_read(config)) == NULL)
		return -1;

	lookup = nnodes_lookup_init(rdb);

	if ((fd = open (rf, O_RDONLY)) == -1) {
		return -1;
	}
	read(fd, req_data, sizeof(req_data));

	for (idx = 0; idx != 100000; idx++) {
		memset(buffer, 0, BUFSIZ);
		memcpy(buffer, req_data, sizeof(req_data));

		nnodes_lookup(lookup, &client, buffer, buffer, BUFSIZ);
	}

	return 0;
}
