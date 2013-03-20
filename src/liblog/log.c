/*
 *
 * Author: Sylvain Afchain <safchain@gmail.com>, (C) 2008 - 2013
 *
 * Copyright: See COPYING file that comes with this distribution
 *
 */

#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <string.h>

#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <unistd.h>
#include <sys/inotify.h>
#ifndef	_LINUX_FCNTL_H
#include <fcntl.h>
#endif
#include <types.h>

#include "log.h"

#define EVENT_SIZE  	( sizeof (struct inotify_event) )
#define EVENT_BUF_LEN	( 1024 * ( EVENT_SIZE + 16 ) )

static char *level2str[] = { "", "error", "info", "warn", "debug" };

int _log_open_file(LOG *logg, char *filename) {
	int rtv;

	if ((logg->fp = fopen(filename, "a")) == NULL) {
		log_printf(logg, LOG_LEVEL_ERR, "unable to open log file: %s", filename);
		return -1;
	}

	/*creating the INOTIFY instance*/
	if ((rtv = inotify_init()) < 0) {
		log_printf(logg, LOG_LEVEL_ERR, "unable to monitor(inotify) log file: %s", filename);
		return -1;
	}
	logg->inotify_fd = rtv;

	fcntl(logg->inotify_fd, F_SETFL, O_NONBLOCK);

	if ((rtv = inotify_add_watch(logg->inotify_fd, filename, IN_ATTRIB | IN_DELETE_SELF | IN_MOVE_SELF)) == -1) {
		log_printf(logg, LOG_LEVEL_ERR, "unable to monitor(inotify) log file: %s", filename);
		return -1;
	}
	logg->wd = rtv;

	return 0;
}

void _log_close_file(LOG *logg) {
	if (logg->fp) {
		inotify_rm_watch(logg->inotify_fd, logg->wd);
		close(logg->inotify_fd);
		fclose(logg->fp);
	}
}

LOG *log_open(char *filename, uint32_t level, uint32_t maxsize) {
	LOG *logg = NULL;

	if ((logg = (LOG *) malloc(sizeof(LOG))) == NULL
		)
		return NULL;

	if ((_log_open_file(logg, filename)) == -1) {
		free(logg);

		return NULL;
	}

	logg->filename = filename;
	logg->maxsize = maxsize;
	logg->level = level;

	return logg;
}

void log_printf(LOG *logg, uint32_t level, char *fmt, ...) {
	FILE *fp = stderr;
	char buffer[EVENT_BUF_LEN], bufftime[BUFSIZ];
	va_list ap;
	time_t curtime;
	struct tm *loctime;

	if (logg && logg->fp) {
		if (read(logg->inotify_fd, buffer, EVENT_BUF_LEN ) > 0) {
			_log_close_file(logg);

			if (_log_open_file(logg, logg->filename) == 0)
			fp = logg->fp;
		}
		else
		fp = logg->fp;
	}

	if (logg && logg->level < level)
	return;

	curtime = time (NULL);

	loctime = localtime (&curtime);

	strftime (bufftime, BUFSIZ, "%Y-%m-%d %H:%M:%S", loctime);

	fprintf (fp, "%s\t%s\t", bufftime, level2str[level]);

	va_start(ap, fmt);
	vfprintf (fp, fmt, ap);
	va_end(ap);

	fputs ("\n", fp);
	fflush(fp);

	fflush (fp);
}

void log_close(LOG *logg) {
	_log_close_file(logg);

	free(logg);
}
