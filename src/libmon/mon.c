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
#include <time.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>

#include <types.h>
#include <errno.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>

#include <mm.h>
#include <topt.h>
#include <dns.h>
#include <log.h>

#include "mon.h"

static char *status2str[] = { "avalaible", "unavailable", "unavailable, protection=true" };

int mon_initialize(struct mon_s *mon) {
	int argc;
	char **argv;

	if (mon->type == MON_TYPE_PIPE) {
		memset(mon->mon.pipe.buffer, 0, sizeof(mon->mon.pipe.buffer));
		if (pipe(mon->mon.pipe.ppfd) == -1) {
			LOG_ERR("pipe (%s), %s", mon->mon.pipe.cmd, strerror(
					errno));
			return -1;
		}
		if (pipe(mon->mon.pipe.pcfd) == -1) {
			LOG_ERR("pipe (%s), %s", mon->mon.pipe.cmd, strerror(
					errno));
			return -1;
		}

		if (topt_str_to_argv(mon->mon.pipe.cmd, &argc, &argv) != STR2AV_OK) {
			LOG_ERR("pipe (%s), unable to parse command line",
					mon->mon.pipe.cmd);
			return -1;
		}

		mon->mon.pipe.pid = fork();
		if (mon->mon.pipe.pid == -1) {
			LOG_ERR("fork (%s), %s", mon->mon.pipe.cmd, strerror(
					errno));
			return -1;
		}

		if (mon->mon.pipe.pid == 0) { /* Child reads from pipe */
			dup2(mon->mon.pipe.ppfd[0], 0);
			dup2(mon->mon.pipe.pcfd[1], 1);

			close(mon->mon.pipe.ppfd[0]);
			close(mon->mon.pipe.ppfd[1]);
			close(mon->mon.pipe.pcfd[0]);
			close(mon->mon.pipe.pcfd[1]);

			execvp(argv[0], argv);
			LOG_ERR("execv (%s), %s", mon->mon.pipe.cmd, strerror(errno));

			exit(-1);
		}
		close(mon->mon.pipe.ppfd[0]);
		close(mon->mon.pipe.pcfd[1]);

		topt_free_argv(argv);
	}

	return 0;
}

int mon_terminate(struct mon_s *mon) {
	int status;

	close(mon->mon.pipe.ppfd[0]);
	close(mon->mon.pipe.ppfd[1]);
	close(mon->mon.pipe.pcfd[0]);
	close(mon->mon.pipe.pcfd[1]);

	kill(mon->mon.pipe.pid, SIGTERM);
	wait(&status);

	return status;
}

int mon_prepare(struct mon_s *mon, int efd) {
	struct epoll_event ev;
	int flags;

	mon->efd = efd;

	LOG_INFO("prepare monitoring [%s]", mon->name);

	if (mon->type == MON_TYPE_PIPE) {

		/* non blocking pipe */
		flags = fcntl(mon->mon.pipe.pcfd[0], F_GETFL, 0);
		fcntl(mon->mon.pipe.pcfd[0], F_SETFL, flags | O_NONBLOCK);

		flags = fcntl(mon->mon.pipe.ppfd[1], F_GETFL, 0);
		fcntl(mon->mon.pipe.ppfd[1], F_SETFL, flags | O_NONBLOCK);

		memset(&ev, 0, sizeof(struct epoll_event));
		ev.events = EPOLLIN;
		ev.data.fd = mon->mon.pipe.pcfd[0];
		ev.data.ptr = mon;
		if (epoll_ctl(mon->efd, EPOLL_CTL_ADD, mon->mon.pipe.pcfd[0], &ev) < 0) {
			LOG_ERR("epoll set insertion error: fd=%d",
					mon->mon.pipe.pcfd[0]);
			return -1;
		}
	} else
		return 0;

	return 1;
}

int mon_execute(struct mon_s *mon) {
	LIST_ITERATOR iterator;
	struct dns_rr_s **rr;
	char *cmd, *str = NULL;
	uint32_t t, i = 0, len;

	if (mon->type == MON_TYPE_PIPE) {
		if (mon->pending)
			return 0;

		t = time(NULL);

		hl_list_init_iterator(mon->rrs, &iterator);
		while ((rr = hl_list_iterate(&iterator)) != NULL) {
			if (! (*rr)->extra.last_mon_check) {
				(*rr)->extra.last_mon_check = t + rand() % mon->refresh;

				i++;

				continue;
			}

			if ((*rr)->extra.last_mon_check + mon->refresh > t) {
				i++;

				continue;
			}

			str = dns_get_data_str(*rr);
			if (!str) {
				LOG_ERR("memory alloction error during monitor %s execution", mon->name);

				i++;
		
				continue;
			}
			len = snprintf(NULL, 0, "check %d %s %s %s %s\n", i,
					dns_int_to_class((*rr)->class),
					dns_int_to_type((*rr)->type),
					(*rr)->name, str);

			if ((cmd = malloc(len + 1)) == NULL)
				goto clean;

			snprintf(cmd, len + 1, "check %d %s %s %s %s\n", i,
					dns_int_to_class((*rr)->class),
					dns_int_to_type((*rr)->type),
					(*rr)->name, str);

			write(mon->mon.pipe.ppfd[1], cmd, len);
			mon->pending++;
			i++;

			LOG_INFO("execute monitor [%s] for record [%s %s %s]", mon->name,
					(*rr)->name,
					dns_int_to_class((*rr)->class),
					dns_int_to_type((*rr)->type),
					str);

			free(cmd);
			mm_free0((void *) &str);
		}
	}

clean:
	if (str)
		free(str);

	return 0;
}

int mon_fetch_result(struct mon_s *mon) {
	struct dns_rr_s **rr;
	char buffer[sizeof(mon->mon.pipe.buffer)], *ptr, *left, *save = NULL;
	uint32_t r, offset, size, i;
	char *str, unavailable = 0;

	if (mon->type == MON_TYPE_PIPE) {
		offset = strlen(mon->mon.pipe.buffer);
		size = sizeof(mon->mon.pipe.buffer) - offset - 1;

		if (size <= 0) {
			LOG_ERR("error response is too long");
			return -1;
		}

		r = read(mon->mon.pipe.pcfd[0], mon->mon.pipe.buffer + offset, size);
		if (!r) {
			LOG_ERR("error while reading pipe");
			return -1;
		}
		offset += r;
		mon->mon.pipe.buffer[offset] = '\0';

		ptr = strtok_r(mon->mon.pipe.buffer, "\n", &save);
		while (ptr) {
			if (sscanf(ptr, "%s %d", buffer, &i) != 2)
				break;

			mon->pending--;

			ptr = strtok_r(NULL, "\n", &save);

			if (strcasecmp(buffer, "ok") == 0)
				unavailable = 0;
			else if (strcasecmp(buffer, "ko") == 0)
				unavailable = 1 + mon->protection;
			else {
				LOG_ERR("monitor %s return error: %s", mon->name, buffer);
				continue;
			}

			rr = hl_list_get(mon->rrs, i);
			if (!rr) {
				LOG_ERR("no records for this index, see previous check request");
				continue;
			}
			(*rr)->extra.last_mon_check = time(NULL);

			if ((str = dns_get_data_str(*rr)) == NULL
			)
				LOG_ERR("memory alloction error during monitor %s execution", mon->name);

			if ((*rr)->extra.unavailable != unavailable) {

				if (unavailable) {
					if ((*rr)->extra.errors < mon->try) {
						(*rr)->extra.errors++;

						if (str) {
							LOG_INFO("monitor [%s] increase errors[%d/%d] of record [%s %s %s %s]",
									mon->name,
									(*rr)->extra.errors,
									mon->try,
									(*rr)->name,
									dns_int_to_class((*rr)->class),
									dns_int_to_type((*rr)->type),
									str);
						}
					}
				}
				else
					(*rr)->extra.errors = 0;

				if (!unavailable ||
						(unavailable && (*rr)->extra.errors >= mon->try)) {
					pthread_rwlock_wrlock(&((*rr)->extra.rwlock_unavailable));
					(*rr)->extra.unavailable = unavailable;
					pthread_rwlock_unlock(&((*rr)->extra.rwlock_unavailable));

					if (str) {
						LOG_INFO("monitor [%s] set record [%s %s %s %s] %s",
								mon->name,
								(*rr)->name,
								dns_int_to_class((*rr)->class),
								dns_int_to_type((*rr)->type),
								str, status2str[(int) unavailable]);
					}
				}
			}
			if (str)
				free(str);
		}
		if (ptr)
			strcpy(mon->mon.pipe.buffer, ptr);
		else
			memset(mon->mon.pipe.buffer, 0, sizeof(mon->mon.pipe.buffer));
	}

	return 0;
}
