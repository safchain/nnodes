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
#include <arpa/inet.h>

#include <types.h>
#include <mm.h>

#include <dns.h>

int nnodes_wr_header(char *raw, uint32_t left, struct dns_header_s *header) {
	struct dns_header_s *h_ptr;

	if (left < sizeof(struct dns_header_s))
		return -1;

	memcpy(raw, header, sizeof(struct dns_header_s));

	h_ptr = (struct dns_header_s *) raw;
	h_ptr->qdcount = htons(h_ptr->qdcount);
	h_ptr->ancount = htons(h_ptr->ancount);
	h_ptr->nscount = htons(h_ptr->nscount);
	h_ptr->arcount = htons(h_ptr->arcount);

	return (sizeof(struct dns_header_s));
}

int nnodes_wr_qsection(char *raw, uint32_t left, struct dns_qsection_s *qsection) {
	char *r_ptr = raw;
	int rtv;

	if ((rtv = dns_strtoraw(qsection->name, raw, left)) == -1)
		return -1;
	r_ptr += rtv;
	left -= rtv;

	if (left < 2 * sizeof(uint16_t))
		return -1;

	*((uint16_t *) r_ptr) = htons(qsection->type);
	r_ptr += sizeof(uint16_t);
	*((uint16_t *) r_ptr) = htons(qsection->class);
	r_ptr += sizeof(uint16_t);

	return (r_ptr - raw);
}

int nnodes_wr_qdsections(char *raw, uint32_t left, LIST *qdsections) {
	struct dns_qsection_s *qdsection;
	LIST_ITERATOR iterator;
	char *r_ptr = raw;
	int rtv;

	hl_list_init_iterator(qdsections, &iterator);
	while ((qdsection = (struct dns_qsection_s *) hl_list_iterate(&iterator))
			!= NULL) {
		if ((rtv = nnodes_wr_qsection(r_ptr, left, qdsection)) == -1)
			goto clean;
		r_ptr += rtv;
		left -= rtv;
	}

	return r_ptr - raw;

clean:
	return rtv;
}

int nnodes_wr_rr(char *raw, uint32_t left, struct dns_rr_s *rr, char *dn) {
	char *r_ptr = raw;
	int rtv;

	if (dn) {
		if ((rtv = dns_strtoraw(dn, raw, left)) == -1)
			return -1;
		r_ptr += rtv;
	} else {
		rtv = strlen(rr->raw) + 1;
		if (left < (uint32_t) rtv)
			return -1;
		memcpy(r_ptr, rr->raw, rtv);
		r_ptr += rtv;
	}
	left -= rtv;
	if (left < 3 * sizeof(uint16_t) + sizeof(uint32_t) + rr->dlength)
		return -1;

	*((uint16_t *) r_ptr) = htons(rr->type);
	r_ptr += sizeof(uint16_t);
	*((uint16_t *) r_ptr) = htons(rr->class);
	r_ptr += sizeof(uint16_t);

	*((uint32_t *) r_ptr) = (int) htonl(rr->ttl);
	r_ptr += sizeof(uint32_t);

	*((uint16_t *) r_ptr) = htons(rr->dlength);
	r_ptr += sizeof(uint16_t);

	memcpy(r_ptr, rr->data, rr->dlength);
	r_ptr += rr->dlength;

	return (r_ptr - raw);
}
