/*
 *
 * Author: Sylvain Afchain <safchain@gmail.com>, (C) 2008 - 2013
 *
 * Copyright: See COPYING file that comes with this distribution
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>

#include <types.h>

#include <mm.h>
#include <hl.h>
#include <misc.h>
#include <dns.h>
#include <mon.h>
#include <monitors.h>
#include <rdb.h>
#include <log.h>
#include <ast.h>

#include "read.h"
#include "write.h"
#include "lookup.h"

/* prototypes */
static int _search_rrs(LOOKUP *, LIST *, char *, uint16_t, uint16_t);

static inline uint16_t _get_rnd_weight(uint16_t max) {
	return rand() % max;
}

uint32_t _rrs_all_unprotected(LIST *rrs) {
	struct dns_rr_s **rr;
	LIST_ITERATOR iterator;

	hl_list_init_iterator(rrs, &iterator);

	while ((rr = hl_list_iterate(&iterator)) != NULL) {
		pthread_rwlock_rdlock(&((*rr)->extra.rwlock_unavailable));

		/* one of the records is not protected */
		if ((*rr)->extra.unavailable != 2) {
			pthread_rwlock_unlock(&((*rr)->extra.rwlock_unavailable));
			return 0;
		}
		pthread_rwlock_unlock(&((*rr)->extra.rwlock_unavailable));
	}

	return 1;
}

static int _search_rrs_for(LOOKUP *lookup, LIST *rrs, char *dn, uint16_t class, uint16_t type) {
	struct dns_rr_s **rr;
	LIST_ITERATOR iterator;
	LIST **list, *pretend = NULL;
	HNODE *hnode;
	HCODE *zones = lookup->rdb->zones;
	char *key = NULL, datadn[MAX_DN_SIZE + 1];
	int32_t found = 0, rtv = 0;
	uint32_t weight, weigth_max;
	char *str = NULL;
	int32_t check_avalaible = 1;

	if ((key = dns_compute_rr_key(dn, class, type)) == NULL)
		goto clean;

	/* prevent multiple response with the same entry */
	if (hl_hash_get(lookup->h_dn_done, key)) {
		found = 1;
		goto clean;
	}
	if ((hnode = hl_hash_put(lookup->h_dn_done, key, &rtv, sizeof(rtv))) == NULL)
		goto clean;

	/* use a list for fast reset, less list nodes than hcode nodes */
	if (hl_list_push(lookup->l_dn_done, &hnode, sizeof(HNODE *)) == -1)
		goto clean;

	/* search the key domain in database (hcode) */
	if ((list = hl_hash_get(zones, key)) != NULL) {

		hl_list_round_robin(*list);
		if ((pretend = hl_list_alloc()) == NULL)
			goto clean;

		/* disable monitors if all records are unavailables */
		if (_rrs_all_unprotected(*list))
			check_avalaible = 0;

		/* initialize the enumeration */
		hl_list_init_iterator(*list, &iterator);

		/* current max weight */
		weigth_max = 0;

		/* search the correct asection, which as the same type and class */
		while ((rr = hl_list_iterate(&iterator)) != NULL) {

			/* see before */
			if (check_avalaible) {
				pthread_rwlock_rdlock(&((*rr)->extra.rwlock_unavailable));

				/* check status, monitoring */
				if ((*rr)->extra.unavailable) {
					pthread_rwlock_unlock(&((*rr)->extra.rwlock_unavailable));
	#ifdef DEBUG
					str = dns_get_data_str(*rr);
					LOG_DEBUG("record [%s %s %s %s] is unavailable",
							(*rr)->name,
							dns_int_to_class((*rr)->class),
							dns_int_to_type((*rr)->type),
							str);
					free (str);
	#endif
					continue;
				}

				pthread_rwlock_unlock(&((*rr)->extra.rwlock_unavailable));
			}

			if ((*rr)->extra.ast_acl) {
				if (! ast_asktree((*rr)->extra.ast_acl, lookup)) {
					str = dns_get_data_str(*rr);
					LOG_INFO("acl [%s] set record [%s %s %s %s] unavailable",
							(*rr)->extra.ast_acl->name,
							(*rr)->name,
							dns_int_to_class((*rr)->class),
							dns_int_to_type((*rr)->type),
							str);
					free (str);

					continue;
				}
			}

			if ((*rr)->extra.weight == 0) {
				if (hl_list_push(rrs, rr, sizeof(struct dns_rr_s *)) == -1)
					goto clean;
				found++;

				/* if cname type search related A type */
				if ((*rr)->type == CNAME_TYPE) {
					if ((rtv = dns_get_cname_datadn(*rr, datadn, MAX_DN_SIZE + 1)) == -1) {
						LOG_ERR("fatal error in resource record %s", dn);
						goto clean;
					}
					if ((rtv = _search_rrs(lookup, rrs, datadn, (*rr)->class, A_TYPE)) > 0)
						found += rtv;
				}

			} else {
				if (hl_list_push(pretend, rr, sizeof(struct dns_rr_s *)) == -1)
					goto clean;
				weigth_max += (*rr)->extra.weight;
				(*rr)->extra.weight_max = weigth_max;
			}
		}

		/* get weighted asection */
		if (weigth_max) {
			/* initialize the enumeration */
			hl_list_init_iterator(pretend, &iterator);

			weight = _get_rnd_weight(weigth_max);

			/* catch a random asection for the working list */
			while ((rr = hl_list_iterate(&iterator)) != NULL) {
				if ((*rr)->extra.weight_max > weight) {
					if (hl_list_push(rrs, rr, sizeof(struct dns_rr_s *)) == -1)
						goto clean;
					found++;

					/* if cname type search related A type */
					if ((*rr)->type == CNAME_TYPE) {
						if ((rtv = dns_get_cname_datadn(*rr, datadn, MAX_DN_SIZE + 1)) == -1) {
							LOG_ERR("fatal error in resource record %s", dn);
							goto clean;
						}
						if ((rtv = _search_rrs(lookup, rrs, datadn, (*rr)->class, A_TYPE)) > 0)
							found += rtv;
					}

					break;
				}
			}
		}
	}

clean:
	if (pretend)
		hl_list_free(pretend);

	if (key)
		free(key);

	return found;
}

static int _search_wcard_rrs(LOOKUP *lookup, LIST *rrs, char *dn, uint16_t class, uint16_t type) {
	char stardn[MAX_DN_SIZE], *subdn, *dup;
	int depth;
	int found = 0;

	dup = strdup(dn);

	/* get depth of the domain for search in sub domain with a star */
	depth = dns_get_depth(dup);

	/* search sub domain with star in database */
	while (depth-- > 1) {

		/* get the sub dn */
		subdn = dns_get_subdn(dup, depth);

		/* star the subdomain */
		stardn[0] = '*';
		stardn[1] = '.';
		stardn[2] = '\0';
		strcat(stardn + 2, subdn);

		if ((found = _search_rrs_for(lookup, rrs, stardn, class, type)) > 0)
			goto clean;
	}

clean:
	free(dup);

	return found;
}

static int _search_rrs(LOOKUP *lookup, LIST *rrs, char *dn, uint16_t class, uint16_t type) {
	int found;
	int matchA = (class == IN_CLASS && type == A_TYPE);

	if ((found = _search_rrs_for(lookup, rrs, dn, class, type)) > 0)
		return found;

	/* no wildcard for ptr */
	if (class == IN_CLASS && type == PTR_TYPE)
		return found;

	/* no A founded, so search with CNAME */
	if (matchA) {
		if ((found = _search_rrs_for(lookup, rrs, dn, class, CNAME_TYPE)) > 0)
			return found;
	}

	if ((found = _search_wcard_rrs(lookup, rrs, dn, class, type)) > 0)
		return found;

	/* no A founded, then search with CNAME */
	if (matchA) {
		if ((found = _search_wcard_rrs(lookup, rrs, dn, class, CNAME_TYPE)) > 0)
			return found;
	}

	return 0;
}

static int _wr_rlsections(LOOKUP *lookup, char *raw, uint32_t left, struct dns_answers_s *answers, char *dn) {
	struct dns_rr_s **rr;
	LIST *rlsections = lookup->rlsections;
	LIST_ITERATOR iterator;
	char *r_ptr;
	int rtv = 0;

	r_ptr = raw;

	hl_list_init_iterator(rlsections, &iterator);
	while ((rr = hl_list_iterate(&iterator)) != NULL) {
		if ((rtv = nnodes_wr_rr(r_ptr, left, *rr, dn)) == -1)
			return -1;
		r_ptr += rtv;
		left -= rtv;

		answers->header.arcount++;
	}

	return r_ptr - raw;
}

static int _get_datadn(struct dns_rr_s *rr, char *datadn, int size) {
	if (rr->type == NS_TYPE)
		return dns_get_ns_datadn(rr, datadn, size);
	else if (rr->type == MX_TYPE)
		return dns_get_mx_datadn(rr, datadn, size);
	else if (rr->type == CNAME_TYPE)
		return dns_get_cname_datadn(rr, datadn, size);

	return -1;
}

static int _findnwr_rlsections(LOOKUP *lookup, char *raw, uint32_t left, struct dns_answers_s *answers) {
	struct dns_rr_s **rr;
	LIST *rlsections = lookup->rlsections;
	LIST_ITERATOR iterator;
	char *r_ptr, datadn[MAX_DN_SIZE + 1];
	int rtv;

	memset(datadn, 0, MAX_DN_SIZE + 1);

	r_ptr = raw;

	hl_list_init_iterator(answers->ansections, &iterator);
	while ((rr = hl_list_iterate(&iterator)) != NULL) {
		if ((*rr)->type == A_TYPE || (*rr)->type == CNAME_TYPE)
			continue;

		if ((rtv = _get_datadn(*rr, datadn, MAX_DN_SIZE + 1)) == -1)
			continue;

		hl_list_reset(rlsections);
		if ((rtv = _search_rrs(lookup, rlsections, datadn, (*rr)->class, A_TYPE)) > 0) {
			if ((rtv = _wr_rlsections(lookup, r_ptr, left, answers, datadn)) == -1)
				return -1;
			r_ptr += rtv;
			left -= rtv;
		}
	}

	return r_ptr - raw;
}

static int _wr_ansections(LOOKUP *lookup, char *raw, uint32_t left, struct dns_answers_s *answers, char *dn) {
	struct dns_rr_s **rr;
	LIST *ansections = lookup->ansections;
	LIST_ITERATOR iterator;
	char *r_ptr = raw, *wdn;
	int rtv = 0;

	hl_list_init_iterator(ansections, &iterator);
	while ((rr = hl_list_iterate(&iterator)) != NULL) {
		wdn = NULL;

		if (dns_is_wcard_name((*rr)->name))
			wdn = dn;

		if ((rtv = nnodes_wr_rr(r_ptr, left, *rr, wdn)) == -1)
			return -1;
		r_ptr += rtv;
		left -= rtv;

		answers->header.ancount++;
	}

	return r_ptr - raw;
}

static int _findnwr_ansections(LOOKUP *lookup, char *raw, uint32_t left, struct dns_answers_s *answers) {
	struct dns_qsection_s *qdsection;
	LIST *ansections = lookup->ansections;
	LIST_ITERATOR iterator;
	char *r_ptr = raw;
	int rtv;

	hl_list_init_iterator(answers->qdsections, &iterator);
	while ((qdsection = hl_list_iterate(&iterator)) != NULL) {
		LOG_INFO("client %s, lookup for record [%s %s %s]",
				inet_ntoa(((struct sockaddr_in *)lookup->client)->sin_addr),
				qdsection->name,
				dns_int_to_class(qdsection->class),
				dns_int_to_type(qdsection->type));

		hl_list_reset(ansections);
		if (_search_rrs(lookup, ansections, qdsection->name, qdsection->class, qdsection->type) > 0) {
			if ((rtv = _wr_ansections(lookup, r_ptr, left, answers, qdsection->name)) == -1)
				return -1;
			r_ptr += rtv;
			left -= rtv;
		}
	}

	return r_ptr - raw;
}

static int _findnwr(LOOKUP *lookup, char *raw, uint32_t left, struct dns_questions_s *questions) {
	struct dns_answers_s answers;
	char *r_ptr;
	int rtv;

	/* init and fill the header */
	memset(&(answers.header), 0, sizeof(struct dns_header_s));
	answers.header.id = questions->header.id;
	answers.header.aa = 1;
	answers.header.qr = 1;
	answers.header.rd = questions->header.rd;

	answers.header.qdcount = questions->header.qdcount;

	/* init a pointer to raw data */
	r_ptr = raw + sizeof(struct dns_header_s);
	left -= sizeof(struct dns_header_s);

	/* fill the asections for the answers */
	answers.qdsections = questions->qdsections;
	answers.ansections = lookup->ansections;
	answers.rlsections = lookup->rlsections;
	/*answers.nssections = lookup->nssections;
	answers.arsections = lookup->arsections;*/

	/* write qdsections */
	if ((rtv = nnodes_wr_qdsections(r_ptr, left, answers.qdsections)) == -1)
		return -1;
	r_ptr += rtv;
	left -= rtv;

	/* find and write */
	if ((rtv = _findnwr_ansections(lookup, r_ptr, left, &answers)) == -1)
		return -1;
	r_ptr += rtv;
	left -= rtv;

	if ((rtv = _findnwr_rlsections(lookup, r_ptr, left, &answers)) == -1)
		return -1;
	r_ptr += rtv;
	left -= rtv;

	if (nnodes_wr_header(raw, left, &(answers.header)) == -1)
		return -1;

	rtv = r_ptr - raw;

	return rtv;
}

LOOKUP *nnodes_lookup_init(RDB *rdb) {
	LOOKUP *lookup;

	if ((lookup = calloc(1, sizeof (LOOKUP))) == NULL)
		return NULL;

	lookup->rdb = rdb;

	if ((lookup->qdsections = hl_list_alloc()) == NULL)
		goto clean;
	if ((lookup->ansections = hl_list_alloc()) == NULL)
		goto clean;
	if ((lookup->rlsections = hl_list_alloc()) == NULL)
		goto clean;
	/*if ((lookup->nssections = hl_list_alloc()) == NULL)
		goto clean;
	if ((lookup->arsections = hl_list_alloc()) == NULL)
		goto clean;*/
	if ((lookup->l_dn_done = hl_list_alloc()) == NULL)
		goto clean;
	if ((lookup->h_dn_done = hl_hash_alloc(0)) == NULL)
		goto clean;

	return lookup;

clean:
	nnodes_lookup_free (lookup);

	return NULL;
}

void nnodes_lookup_free(LOOKUP *lookup) {
	if (lookup->qdsections)
		hl_list_free(lookup->qdsections);
	if (lookup->ansections)
		hl_list_free(lookup->ansections);
	if (lookup->rlsections)
		hl_list_free(lookup->rlsections);
	/*if (lookup->nssections)
		hl_list_free(lookup->nssections);
	if (lookup->arsections)
		hl_list_free(lookup->arsections);*/
	if (lookup->l_dn_done)
		hl_list_free(lookup->l_dn_done);
	if (lookup->h_dn_done)
		hl_hash_free(lookup->h_dn_done);
}

int nnodes_lookup(LOOKUP *lookup, struct sockaddr *client, char *rin, char *rout, uint32_t size) {
	LIST_ITERATOR iterator;
	HNODE **hnode;
	struct dns_questions_s *questions;
	int rtv;

	if ((questions = (struct dns_questions_s *) malloc(sizeof(struct dns_questions_s))) == NULL)
		return -1;

	hl_list_reset(lookup->qdsections);
	/*hl_list_reset(lookup->ansections);
	hl_list_reset(lookup->rlsections);
	hl_list_reset(lookup->nssections);
	hl_list_reset(lookup->arsections);*/

	lookup->client = client;

	hl_list_init_iterator(lookup->l_dn_done, &iterator);
	while ((hnode = hl_list_iterate(&iterator)) != NULL) {
		hl_hash_free_node(*hnode);
	}
	hl_list_reset(lookup->l_dn_done);

	questions->qdsections = lookup->qdsections;

	/* read query */
	nnodes_rd_questions(rin, questions);

	rtv = _findnwr(lookup, rout, size, questions);

	free(questions);

	return rtv;
}
