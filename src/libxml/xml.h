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

#ifndef __XML_H
#define __XML_H

#include <types.h>

#include <hl.h>

typedef struct xml_config_s XML_CONFIG;
typedef struct xml_callbacks_s XML_CALLBACKS;

struct xml_config_s {
	HCODE 	*callbacks;

	char 	*xpath;
	uint32_t 	xpath_size;
	
	uint32_t 	line;
	char 	error[255];
	
	void	*backptr;
};

struct xml_callbacks_s {
	int (*on_text) (char *text, XML_CONFIG *xml_config);
	int (*on_tag_open) (char *tag, XML_CONFIG *xml_config);
	int (*on_tag_attr) (char *tag, char *name, char *value, XML_CONFIG *xml_config);
	int (*on_tag_close) (char *tag, XML_CONFIG *xml_config);
	int (*on_error) (char *text, XML_CONFIG *xml_config);
};

/* prototypes */
int xml_is_xpath_child (const char *, XML_CONFIG *);
int xml_parse_file (const char *, XML_CONFIG *);
XML_CONFIG *xml_alloc_config ();
void xml_free_config (XML_CONFIG *);

#endif
