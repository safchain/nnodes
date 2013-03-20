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
#include <ctype.h>
#include <arpa/inet.h>

#include <types.h>

#include <mm.h>

#include "dns_base.h"

/* type in char format */
static char *dns_type[] = {
		"ERR", /* 0 error */
		"A", /* 1  a host address */
		"NS", /* 2  authoritative name server */
		"MD", /* 3  mail destination (obsolete) */
		"MF", /* 4  mail forwarder (obsolete) */
		"CNAME", /* 5  canonical name for alias */
		"SOA", /* 6  start of a zone of authority */
		"MB", /* 7  mailbox domain name (experimental) */
		"MG", /* 8  mail group member (experimental) */
		"MR", /* 9  mail rename domain name (experimental) */
		"RR", /* 10 null RR (experimental) */
		"WKS", /* 11 well known service description */
		"PTR", /* 12 domain name pointer */
		"HINFO", /* 13 host info */
		"MINFO", /* 14 mailbox or mail list information */
		"MX", /* 15 mail exchange */
		"TXT" /* 16 text string */
};

/* class in char format */
static char *dns_class[] = {
		"ERR", /* 0  error */
		"IN", /* 1  the internet */
		"CS", /* 2  the csnet class (obsolete) */
		"CH", /* 3  the choas class */
		"HS" /* 4  Hesiod (Dyer 87) */
};

inline char *dns_int_to_type(int type) {
	return dns_type[type];
}

inline char *dns_int_to_class(int class) {
	return dns_class[class];
}

/* TODO: use function below */
char *dns_get_data_str(struct dns_rr_s *rr) {
	char *str = NULL;
	int len;

	if (rr->class == IN_CLASS) {
		switch (rr->type) {
		case A_TYPE:
			len = snprintf(NULL, 0, "%u.%u.%u.%u", ((unsigned char *) rr->data)[0], ((unsigned char *) rr->data)[1], ((unsigned char *) rr->data)[2], ((unsigned char *) rr->data)[3]);
			len++;
			if ((str = malloc(len)) == NULL)
				return NULL;
			snprintf(str, len, "%u.%u.%u.%u", ((unsigned char *) rr->data)[0], ((unsigned char *) rr->data)[1], ((unsigned char *) rr->data)[2], ((unsigned char *) rr->data)[3]);

			break;
		case SOA_TYPE:
		case CNAME_TYPE:
		case NS_TYPE:
		case PTR_TYPE:
			len = dns_rawtostr_size(rr->data);
			if ((str = malloc(len + 1)) == NULL)
				return NULL;
			memset(str, 0, len + 1);
			dns_rawtostr(rr->data, str, len);

			break;
		case TXT_TYPE:
			len = rr->dlength;
			if ((str = malloc(len)) == NULL)
				return NULL;
			memset(str, 0, len);
			memcpy(str, ((char *) rr->data) + 1, len - 1);

			break;
		case MX_TYPE:
			len = dns_rawtostr_size(rr->data + sizeof(uint16_t));

			if ((str = malloc(len + 1)) == NULL)
				return NULL;
			memset(str, 0, len + 1);

			dns_rawtostr(rr->data + sizeof(uint16_t), str, len);
			break;
		default:
			return strdup("");
		}
	}

	return str;
}

inline char *dns_compute_rr_key(char *dn, uint16_t type, uint16_t class) {
	char *key, *ptr;
	uint32_t length = strlen(dn);
	uint32_t size = length + 3;

	if ((key = malloc(size)) == NULL
		)
		return NULL;
	ptr = key;

	memcpy(ptr, dn, length);
	ptr += length;

	*ptr++ = class;
	*ptr++ = type;
	*ptr = '\0';

	return key;
}

/* PTR->12 */
inline char dns_type_to_int(char *type) {
	int i;

	for (i = 1; i != MAX_TYPE; i++)
		if (dns_type[i] != NULL
			)
			if (strcmp(type, dns_type[i]) == 0)
				return i;

	return -1;
}

/* IN->1 */
inline char dns_class_to_int(char *class) {
	uint8_t i;

	for (i = 1; i != MAX_CLASS; i++)
		if (dns_class[i] != NULL
			)
			if (strcmp(class, dns_class[i]) == 0)
				return i;

	return -1;
}

/* return a sub domain of a domain in function of depth,
 ex: www.google.com., 1 -> com.

 str: string of a domain
 depth: depth which want to get

 return a pointer to the sub domain
 */
inline char *dns_get_subdn(char *str, uint32_t depth) {
	int n = strlen(str);

	while (n--) {
		if (*(str + n) == '.') {
			if (depth == 0)
				return (str + n + 1);
			depth--;
		}
	}

	return str;
}

/* return depth of a domain: www.yahoo.fr. -> 3

 str: string of a domain

 return depth a the domain
 */
inline int dns_get_depth(char *str) {
	int n = 0;

	while (*str != '\0') {
		if (*str++ == '.')
			n++;
	}

	return n;
}

/* conv: 4toto3com0 -> toto.com\0 

 raw: data from the packet

 return the number of readed octects from raw buffer
 */
int dns_rawtostr_size(char *raw) {
	char *r_ptr = raw;
	int nc;

	while (*r_ptr != 0) {
		nc = *r_ptr++;

		/* check the correct size of the label (63 max) */
		if (nc > 63)
			return -1;

		while (nc--)
			r_ptr++;
	}
	r_ptr++;

	/* check the correct size of the domain (255 max) */
	if (r_ptr - raw > 255)
		return -1;

	return (r_ptr - raw);
}

/* conv: 4toto3com0 -> toto.com\0 

 raw: data from the packet
 str: pointer to buffer which recv the string

 return the number of readed octects from raw buffer
 */
int dns_rawtostr(char *raw, char *str, int sz) {
	char *r_ptr = raw, *s_str = str;
	int nc;

	while (*r_ptr != 0) {
		nc = *r_ptr++;

		/* check if the correct size of the label (63 max) */
		if (nc > 63)
			return -1;

		while (nc--) {
			if (s_str - str >= sz)
				return -1;
			*s_str++ = tolower(*r_ptr++);
		}
		if (s_str - str >= sz)
			return -1;
		*s_str++ = '.';
	}
	/* check the correct size of the domain (255 max) */
	if (s_str - str > 255)
		return -1;

	if (s_str - str >= sz)
		return -1;
	*s_str = *r_ptr++;

	return (r_ptr - raw);
}

/* conv: toto.com\0 -> 4toto3com0

 str: pointer to buffer which recv the string

 return the number of writed octects from raw buffer
 */
inline int dns_strtoraw_size(char *str) {
	if (strlen(str) > 255)
		return -1;

	return strlen(str) + 1;
}

/* conv: toto.com\0 -> 4toto3com0

 str: pointer to buffer which recv the string
 raw: data from the packet

 return the number of writed octects from raw buffer
 */
int dns_strtoraw(char *str, char *raw, uint32_t left) {
	char *r_ptr = raw;
	uint32_t to_w, len;

	len = strlen(str);
	if (len > left - 1 || len > 255)
		return -1;

	while (*str != '\0') {
		to_w = 0;

		/* number of c before a point */
		while (*(str + to_w) != '.' && *(str + to_w) != '\0')
			to_w++;

		/* check if the correct size of the label (63 max) */
		if (to_w > 63)
			return -1;

		/* write number of c */
		*r_ptr++ = to_w;

		/* write c */
		while (to_w--)
			*r_ptr++ = *str++;

		/* skip point */
		str++;
	}

	/* end of data */
	*r_ptr++ = 0;

	return (r_ptr - raw);
}

inline int dns_is_wcard_name(char *name) {
	if (*name == '*')
		return 1;

	return 0;
}

inline int dns_is_name_closed(char *name) {
	int len = strlen(name);

	if (*(name + len - 1) == '.')
		return 1;

	return 0;
}

int dns_mk_a_data(char *ip, struct dns_rr_s *rr) {
	char *d_curr = ip;
	char *d_next, *data_ptr;

	if (!ip)
		return -1;

	rr->dlength = 4;
	if ((rr->data = malloc(rr->dlength)) == NULL
		)
		return -1;

	data_ptr = (char *) rr->data;

	*data_ptr++ = (char) strtol(d_curr, &d_next, 10);
	d_curr = d_next + 1;

	*data_ptr++ = (char) strtol(d_curr, &d_next, 10);
	d_curr = d_next + 1;

	*data_ptr++ = (char) strtol(d_curr, &d_next, 10);
	d_curr = d_next + 1;

	*data_ptr++ = (char) strtol(d_curr, &d_next, 10);

	return 0;
}

int dns_mk_ptr_data(char *dn, struct dns_rr_s *rr) {
	int length;

	if (!dn)
		return -1;

	if ((length = dns_strtoraw_size(dn)) == -1)
		return -1;

	rr->dlength = length;
	if ((rr->data = calloc(rr->dlength, sizeof(char))) == NULL
		)
		return -1;

	if (dns_strtoraw(dn, rr->data, rr->dlength) == -1)
		return -1;

	return 0;
}

int dns_mk_ns_data(char *dn, struct dns_rr_s *rr) {
	int length;

	if (!dn)
		return -1;

	if ((length = dns_strtoraw_size(dn)) == -1)
		return -1;

	rr->dlength = length;
	if ((rr->data = calloc(rr->dlength, sizeof(char))) == NULL
		)
		return -1;

	if (dns_strtoraw(dn, rr->data, rr->dlength) == -1)
		return -1;

	return 0;
}

int dns_mk_cname_data(char *dn, struct dns_rr_s *rr) {
	int length;

	if (!dn)
		return -1;

	if ((length = dns_strtoraw_size(dn)) == -1)
		return -1;

	rr->dlength = length;
	if ((rr->data = calloc(rr->dlength, sizeof(char))) == NULL
		)
		return -1;

	if (dns_strtoraw(dn, rr->data, rr->dlength) == -1)
		return -1;

	return 0;
}

int dns_mk_txt_data(char *txt, struct dns_rr_s *rr) {
	uint8_t length;

	if (!txt)
		return -1;

	if ((length = (uint8_t) strlen(txt)) == 0)
		return -1;

	rr->dlength = length + 1;
	if ((rr->data = calloc(rr->dlength, sizeof(char))) == NULL
		)
		return -1;

	*((char *) rr->data) = length;
	memcpy(((char *) rr->data + 1), txt, length);

	return 0;
}

int dns_mk_soa_data(char *master, char *mail, uint32_t serial, uint32_t refresh,
		uint32_t retry, uint32_t expire, uint32_t minimum, struct dns_rr_s *rr) {
	int n, length = sizeof(uint32_t) * 5;
	char *ptr;

	if (!master || !mail)
		return -1;

	if ((n = dns_strtoraw_size(master)) == -1)
		return -1;
	length += n;

	if ((n = dns_strtoraw_size(mail)) == -1)
		return -1;
	length += n;

	rr->dlength = length;
	if ((rr->data = calloc(rr->dlength, sizeof(char))) == NULL)
		return -1;
	ptr = rr->data;

	if ((n = dns_strtoraw(master, ptr, rr->dlength)) == -1)
		return -1;
	ptr += n;

	if ((n = dns_strtoraw(mail, ptr, rr->dlength - n)) == -1)
		return -1;
	ptr += n;

	*((uint32_t *) ptr) = htonl(serial);
	ptr += sizeof(uint32_t);
	*((uint32_t *) ptr) = htonl(refresh);
	ptr += sizeof(uint32_t);
	*((uint32_t *) ptr) = htonl(retry);
	ptr += sizeof(uint32_t);
	*((uint32_t *) ptr) = htonl(expire);
	ptr += sizeof(uint32_t);
	*((uint32_t *) ptr) = htonl(minimum);

	return 0;
}

int dns_mk_mx_data(uint16_t preference, char *exchange,
		struct dns_rr_s *rr) {
	int n, length = sizeof(uint16_t);
	char *ptr;

	if (!exchange)
		return -1;

	if ((n = dns_strtoraw_size(exchange)) == -1)
		return -1;
	length += n;

	rr->dlength = length;
	if ((rr->data = calloc(rr->dlength, sizeof(char))) == NULL
		)
		return -1;
	ptr = rr->data;

	*((uint16_t *) ptr) = htons(preference);
	ptr += sizeof(uint16_t);

	if ((n = dns_strtoraw(exchange, ptr, rr->dlength - sizeof(uint16_t)))
			== -1)
		return -1;

	return 0;
}

int dns_get_cname_datadn(struct dns_rr_s *rr, char *dn, int size) {
	int rtv;

	if ((rtv = dns_rawtostr_size(rr->data)) == -1)
		return -1;

	if (rtv >= size)
		return -1;

	if ((rtv = dns_rawtostr(rr->data, dn, size)) == -1)
		return -1;

	return 0;
}

int dns_get_ns_datadn(struct dns_rr_s *rr, char *dn, int size) {
	int rtv;

	if ((rtv = dns_rawtostr_size(rr->data)) == -1)
		return -1;

	if (rtv >= size)
		return -1;

	if ((rtv = dns_rawtostr(rr->data, dn, size)) == -1)
		return -1;

	return 0;
}

int dns_get_mx_datadn(struct dns_rr_s *rr, char *dn, int size) {
	int rtv;

	if ((rtv = dns_rawtostr_size(rr->data + sizeof(uint16_t))) == -1)
		return -1;

	if (rtv >= size)
		return -1;

	if ((rtv = dns_rawtostr(rr->data + sizeof(uint16_t), dn, size)) == -1)
		return -1;

	return 0;
}
