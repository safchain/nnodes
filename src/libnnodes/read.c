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
#include <hl.h>
#include <dns.h>

int nnodes_rd_header(char *raw, struct dns_header_s *header) {
	memcpy(header, raw, sizeof(struct dns_header_s));

	header->qdcount = htons(header->qdcount);
	header->ancount = htons(header->ancount);
	header->nscount = htons(header->nscount);
	header->arcount = htons(header->arcount);

	return (sizeof(struct dns_header_s));
}

int nnodes_rd_qsection(char *raw, struct dns_qsection_s *qsection) {
	char *r_ptr = raw;
	int r;

	// TEST HERE
	if ((r = dns_rawtostr(r_ptr, qsection->name, sizeof(qsection->name) - 1)) == -1)
		return -1;
	r_ptr += r;

	qsection->type = htons(*((uint16_t *) r_ptr));
	if (! qsection->type || qsection->type > 16)
		return -1;

	r_ptr += sizeof(uint16_t);
	qsection->class = htons(*((uint16_t *) r_ptr));
	if (! qsection->class || qsection->class > 4)
		return -1;

	r_ptr += sizeof(uint16_t);

	return (r_ptr - raw);
}

int nnodes_rd_questions(char *raw, struct dns_questions_s *questions) {
	struct dns_qsection_s qdsection;
	uint16_t qdc;
	char *r_ptr = raw;
	int rtv;

	r_ptr += nnodes_rd_header(r_ptr, &(questions->header));

	for (qdc = 0; qdc != questions->header.qdcount; qdc++) {
		if ((rtv = nnodes_rd_qsection(r_ptr, &qdsection)) == -1)
			return rtv;

		if (hl_list_push(questions->qdsections, &qdsection,
				sizeof(struct dns_qsection_s)) != -1)
			r_ptr += rtv;
	}

	return r_ptr - raw;
}
