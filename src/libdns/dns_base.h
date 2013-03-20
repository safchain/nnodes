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

#ifndef __DNS_BASE_H
#define __DNS_BASE_H

#include <stdlib.h>
#include <stdio.h>
#include <endian.h>

#include <types.h>
#include <hl.h>
#include <mon.h>
#include <ast.h>

#define MAX_CLASS	5
#define MAX_TYPE	17
#define MAX_DN_SIZE	255

/* type define */
#define A_TYPE		1	/* 1  a host address */
#define NS_TYPE		2	/* 2  authoritative name server */
#define MD_TYPE		3	/* 3  mail destination (obsolete) */
#define MF_TYPE		4	/* 4  mail forwarder (obsolete) */
#define CNAME_TYPE	5	/* 5  canonical name for alias */
#define SOA_TYPE	6	/* 6  start of a zone of authority */
#define MB_TYPE		7	/* 7  mailbox domain name (experimental) */
#define MG_TYPE		8	/* 8  mail group member (experimental) */ 
#define MR_TYPE		9	/* 9  mail rename domain name (experimental) */
#define WKS_TYPE	11	/* 11 well known service description */
#define PTR_TYPE	12	/* 12 domain name pointer */
#define HINFO_TYPE	13	/* 13 host info */
#define MINFO_TYPE	14	/* 14 mailbox or mail list information */
#define MX_TYPE		15	/* 15 mail exchange */
#define TXT_TYPE	16	/* 16 text string */

/* class define */
#define IN_CLASS	1	/* 1  the internet */
#define CS_CLASS	2	/* 2  the csnet class (obsolete) */ 
#define CH_CLASS	3	/* 3  the choas class */
#define HS_CLASS	4	/* 4  Hesiod (Dyer 87) */

/* default values */
#define DEFAULT_TTL	3600	/* 1 hour */

/* the header structure */
struct dns_header_s {
    /* id of transaction */
    unsigned id : 16;

    /* the flags */
#if __BYTE_ORDER == __BIG_ENDIAN
    /* fields in the 3rd byte */
    unsigned qr : 1; /* 1 bit specifies whether this message is a question or a answer */
    unsigned opcode : 4; /* 4 bit field specifies kind of query in this message */
    unsigned aa : 1; /* 1 bit specifies that the responding server is an authority */
    unsigned tc : 1; /* 1 bit message was truncated due to length greater than udp size */
    unsigned rd : 1; /* 1 bit specifies that the recursion is desire */
    /* fields in the 4th byte */
    unsigned ra : 1; /* 1 bit specifies that the recursion is available */
    unsigned z : 3; /* 3 bit reserved */
    unsigned rcode : 4; /* 4 bit specifies response code */
#elif __BYTE_ORDER == __LITTLE_ENDIAN		/* Intel x86 */
    /* fields in the 3rd byte */
    unsigned rd : 1;
    unsigned tc : 1;
    unsigned aa : 1;
    unsigned opcode : 4;
    unsigned qr : 1;
    /* fields in the 4th byte */
    unsigned rcode : 4;
    unsigned z : 3;
    unsigned ra : 1;
#else
#error "Please fix <bits/endian.h>"
#endif

    /* number of records blocks */
    unsigned qdcount : 16; /* specifying the number of entries in question section */
    unsigned ancount : 16; /* specifying the number of entries in answer section */
    unsigned nscount : 16; /* specifying the number of entries in authority section */
    unsigned arcount : 16; /* specifying the number of entries in additional section */
};

/* structure of a question packet */
struct dns_questions_s {
    struct dns_header_s header; /* begin with one header */
    LIST *qdsections; /* followed by some questions */
};

/* structure of one question */
struct dns_qsection_s {
    char name[255]; /* domain name to which resource record pertains */
    uint16_t type; /* the meaning of the data */
    uint16_t class; /* the class of the data */
};

/* reserved for Next */
struct dns_rr_extra_s {
    uint32_t errors;
    time_t last_mon_check;

    char unavailable;
    pthread_rwlock_t rwlock_unavailable;
    
    uint32_t weight;
    uint32_t weight_max;

    AST_TREE *ast_acl;
};

/* structure of one answer */
struct dns_rr_s {
    char name[255]; /* domain name to which resource record pertains */
    char raw[255]; /* raw form of domain */
    uint16_t type; /* the meaning of the data */
    uint16_t class; /* the class of the data */
    uint32_t ttl; /* the time interval (sec) before record should be discard */
    uint16_t dlength; /* length in octects of the data */
    void *data; /* octects that describes the resource */
    struct dns_rr_extra_s extra; /* reserved for Next */
};

/* structure of a answer packet */
struct dns_answers_s {
    struct dns_header_s header; /* begin with one header */
    LIST *qdsections; /* follow by some questions */
    LIST *ansections; /* follow by some answers */
    LIST *rlsections; /* related answers */
    LIST *nssections; /* end with some authorities */
    LIST *arsections; /* end with some additional */
};

/* prototypes */
int dns_mk_a_data(char *, struct dns_rr_s *);
int dns_mk_ptr_data(char *, struct dns_rr_s *);
int dns_mk_ns_data(char *, struct dns_rr_s *);
int dns_mk_cname_data(char *, struct dns_rr_s *);
int dns_mk_txt_data(char *, struct dns_rr_s *);
int dns_mk_soa_data(char *, char *, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, struct dns_rr_s *);
int dns_mk_mx_data(uint16_t, char *, struct dns_rr_s *);
int dns_get_mx_datadn(struct dns_rr_s *, char *, int);
int dns_get_cname_datadn(struct dns_rr_s *, char *, int);
int dns_get_ns_datadn(struct dns_rr_s *, char *, int);
char dns_type_to_int(char *);
char dns_class_to_int(char *);
char *dns_int_to_type(int);
char *dns_int_to_class(int);
char *dns_compute_rr_key(char *, uint16_t, uint16_t);
char *dns_get_subdn(char *, uint32_t);
int dns_rawtostr_size(char *);
int dns_rawtostr(char *, char *, int);
int dns_strtoraw_size(char *);
int dns_strtoraw(char *, char *, uint32_t);
int dns_get_depth(char *);
int dns_is_wcard_name(char *);
int dns_is_name_closed(char *);
char *dns_get_data_str(struct dns_rr_s *);

#endif
