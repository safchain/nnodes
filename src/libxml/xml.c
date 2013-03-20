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
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <types.h>
#include <mm.h>
#include <misc.h>

#include "xml.h"

enum {
	XML_TAG_OPEN,
	XML_TAG_CLOSE,
	XML_TAG_SINGLE,
	XML_TAG_META,
	XML_TAG_COMMENT,
	XML_TAG_SPEC,
	XML_TAG_CDATA
};

#define XML_XPATH_SIZE	128

/* protection */
static int _xml_increase_buffer(char **ptr, uint32_t *sz, uint32_t inc) {
	if ((*ptr = realloc(*ptr, *sz + inc)) == NULL)
		return -1;

	*sz += inc;

	return 0;
}

static int _xml_push_cf_xpath(const char *tagname, XML_CONFIG *config) {
	uint32_t tlen, xlen;

	tlen = strlen(tagname) + 1; /* / */
	xlen = strlen(config->xpath) + 1; /* \0 */

	if (tlen + xlen > config->xpath_size) {
		if (_xml_increase_buffer(&config->xpath, &config->xpath_size, (tlen
				- xlen) - config->xpath_size) == -1)
			return -1;
	}

	strcat(config->xpath, "/");
	strcat(config->xpath, tagname);

	return 0;
}

static void _xml_pop_xpath(char *xpath) {
	char *last = NULL;

	while (*xpath != '\0') {
		if (*xpath == '/')
			last = xpath;
		xpath++;
	}
	if (last)
		*last = '\0';
}

static void _xml_pop_cf_xpath(XML_CONFIG *config) {
	_xml_pop_xpath(config->xpath);
}

static char *_xml_get_attr_name(const char *start, char **name,
		XML_CONFIG *config) {
	char *end;
	uint32_t size;

	while (*start != '\0') {
		if (*start != ' ' && *start != '\t' && *start != '/' && *start != '\r'
				&& *start != '\n')
			break;
		start++;
	}

	end = (char *) start;
	while (*end != '\0') {
		if (*end == ' ' || *end == '\t' || *end == '/' || *end == '=' || *end
				== '\r' || *end == '\n')
			break;
		end++;
	}

	if (end == start) {
		*name = NULL;
		return (char *) start;
	}

	size = (end - start) + 1;
	if ((*name = calloc(size, sizeof(char))) == NULL) {
		snprintf(config->error, sizeof(config->error),
				"memory allocation error, line %d", config->line);
		return NULL;
	}

	memcpy(*name, start, end - start);

	return end;
}

static char *_xml_get_attr_value(const char *start, char **value,
		XML_CONFIG *config) {
	char *end;
	uint32_t size, str = 0;

	while (*start != '\0') {
		if (*start != ' ' && *start != '\t' && *start != '/')
			break;
		start++;
	}

	if (*start == '"') {
		str = 1;
		start++;
	}

	end = (char *) start;
	while (*end != '\0') {
		if (str) {
			if (*end == '"')
				break;
		} else {
			if (*end == ' ' || *end == '\t' || *end == '/')
				break;
		}
		end++;
	}

	size = (end - start) + 1;
	if ((*value = calloc(size, sizeof(char))) == NULL) {
		snprintf(config->error, sizeof(config->error),
				"memory allocation error, line %d", config->line);
		return NULL;
	}

	memcpy(*value, start, end - start);

	if (str)
		end++;

	return end;
}

static char *_xml_get_next_attr(const char *start, char **name, char **value,
		XML_CONFIG *config) {
	if ((start = _xml_get_attr_name(start, name, config)) == NULL)
		return NULL;

	if (*start == '=') {
		if ((start = _xml_get_attr_value(start + 1, value, config)) == NULL)
			return NULL;
	} else
		*value = NULL;

	return (char *) start;
}

static char *_xml_get_tag_name(const char *tag, char **name, XML_CONFIG *config) {
	char *start, *end;
	uint32_t size;

	start = (char *) tag;
	while (*start != '\0') {
		if (*start != ' ' && *start != '\t')
			break;
		start++;
	}

	end = start;
	while (*end != '\0') {
		if (*end == ' ' || *end == '\t')
			break;
		end++;
	}

	if (end == start) {
		*name = NULL;
		return start;
	}

	size = (end - start) + 1;
	if ((*name = calloc(size, sizeof(char))) == NULL) {
		snprintf(config->error, sizeof(config->error),
				"memory allocation error, line %d", config->line);
		return NULL;
	}
	memcpy(*name, start, end - start);

	return end;
}

static char *_xml_read_cdata(const char *buff, XML_CONFIG *config) {
	if (strncasecmp(buff, "[CDATA[", 7) != 0)
		return NULL;
	buff += 7;

	return (char *) buff;
}

static int _xml_read_tag(FILE *fp, char **ptr, uint32_t *sz, uint32_t *type,
		XML_CONFIG *config) {
	uint32_t offset = 0;
	int cdm = 0, c, b[2] = { 0, 0 };

	if ((c = getc(fp)) != '<')
		return -1;

	*type = XML_TAG_OPEN;

	while ((c = getc(fp)) != EOF) {
		if (offset > *sz - 1)
			if (_xml_increase_buffer(ptr, sz, 10) == -1) {
				snprintf(config->error, sizeof(config->error),
						"memory allocation error, line %d", config->line);
				return -1;
			}

		if (c == '\n')
			config->line++;

		if (*type != XML_TAG_CDATA && *type != XML_TAG_META) {
			if (c == '!') {
				*type = XML_TAG_SPEC;
				cdm = 1;
				continue;
			} else if (c == '?') {
				*type = XML_TAG_META;
				continue;
			} else if (c == '/' && b[1] == 0) {
				*type = XML_TAG_CLOSE;
				continue;
			}
		}

		if (cdm) {
			cdm = 0;
			if (c == '[')
				*type = XML_TAG_CDATA;
			if (c == '-') {
				*type = XML_TAG_COMMENT;
				continue;
			}
		}

		if (c == '>') {
			if (*type == XML_TAG_CDATA) {
				if (b[0] == ']' && b[1] == ']') {
					*(*ptr + offset - 2) = '\0';
					return 0;
				}
			} else if (*type == XML_TAG_COMMENT) {
				if (b[0] == '-' && b[1] == '-') {
					*(*ptr + offset - 2) = '\0';
					return 0;
				}
			} else if (*type == XML_TAG_META) {
				if (b[1] == '?') {
					*(*ptr + offset - 1) = '\0';
					return 0;
				}
			} else if (b[1] == '/') {
				*(*ptr + offset - 1) = '\0';
				*type = XML_TAG_SINGLE;
				return 0;
			} else {
				*(*ptr + offset) = '\0';
				return 0;
			}
		}

		b[0] = b[1];
		b[1] = c;

		*(*ptr + offset++) = c;
	}

	return -1;
}

static int _xml_skip_spaces(FILE *fp, XML_CONFIG *config) {
	int c, r = 0;

	while ((c = getc(fp)) != EOF) {
		if (c == '\n')
			config->line++;

		if (c != ' ' && c != '\t' && c != '\r' && c != '\n') {
			ungetc(c, fp);
			break;
		}

		r++;
	}

	return r;
}

static int _xml_read_text(FILE *fp, char **ptr, uint32_t *sz, XML_CONFIG *config) {
	uint32_t offset = 0;
	int c, r = 0, l = '\0';

	while ((c = getc(fp)) != EOF) {
		if (c == '\n')
			config->line++;

		if (c != ' ' && c != '\t' && c != '\r' && c != '\n')
			break;

		r++;
	}

	while (c >= 0) {
		r++;

		if (offset > *sz - 1)
			if (_xml_increase_buffer(ptr, sz, 10) == -1) {
				snprintf(config->error, sizeof(config->error),
						"memory allocation error, line %d", config->line);
				return -1;
			}

		if (c == '\n')
			config->line++;
		else if (c == '<') {
			ungetc(c, fp);
			*(*ptr + offset) = '\0';

			break;
		} else if (c == '\t')
			c = ' ';
		if (c != '\r' && c != '\n')
			if (!(c == ' ' && l == ' '))
				*(*ptr + offset++) = c;
		l = c;

		c = getc(fp);
	}

	if (l == ' ')
		*(*ptr + offset - 1) = '\0';

	return r;
}

XML_CALLBACKS *_xml_xpath_callbacks(XML_CONFIG *config) {
	XML_CALLBACKS *callbacks = NULL;
	char *xpath = NULL;

	if (!config->callbacks)
		return NULL;

	xpath = strdup(config->xpath);

	while (*xpath != '\0') {
		if ((callbacks = hl_hash_get(config->callbacks, xpath)) != NULL)
			goto clean;
		_xml_pop_xpath(xpath);
	}

	clean: if (xpath)
		free(xpath);

	return callbacks;
}

int xml_is_xpath_child(const char *xpath, XML_CONFIG *config) {
	uint32_t len = strlen(xpath);

	if (strncasecmp(xpath, config->xpath, len) == 0)
		return 1;

	return 0;
}

XML_CONFIG *xml_alloc_config() {
	XML_CONFIG *config;

	if ((config = calloc(1, sizeof(XML_CONFIG))) == NULL)
		return NULL;

	if ((config->callbacks = hl_hash_alloc(0)) == NULL)
		goto clean;

	return config;
	clean: free(config);

	return NULL;
}

void xml_free_config(XML_CONFIG *config) {
	if (config->xpath)
		free(config->xpath);

	free(config);
}

int xml_parse_file(const char *filename, XML_CONFIG *config) {
	FILE *fp = NULL;
	char *buff = NULL, *start, *next;
	char *tagname = NULL, *attrname = NULL, *attrvalue = NULL;
	uint32_t size = 128, type = 0;
	XML_CALLBACKS *callbacks;
	struct stat st_buf;
	int rtv = -1;

	if (!config) {
		fprintf(stderr, "no config struct given");
		return -1;
	}

	if (stat (filename, &st_buf) == -1) {
		snprintf(config->error, sizeof(config->error), "stat (%s), %s",
				filename, strerror(errno));
		goto clean;
	}

	if ((fp = fopen(filename, "r")) == NULL) {
		snprintf(config->error, sizeof(config->error), "fopen (%s), %s",
				filename, strerror(errno));
		goto clean;
	}

	if ((buff = calloc(size, sizeof(char))) == NULL)
		goto clean;

	if (!config->xpath) {
		if ((config->xpath = calloc(XML_XPATH_SIZE, sizeof(char))) == NULL)
			goto clean;
		config->xpath_size = XML_XPATH_SIZE;
	}

	config->line = 1;
	_xml_skip_spaces(fp, config);

	while (_xml_read_tag(fp, &buff, &size, &type, config) != -1) {
		switch (type) {
		case XML_TAG_SINGLE:
		case XML_TAG_OPEN:
			if ((start = _xml_get_tag_name(buff, &tagname, config)) == NULL)
				continue;

			if (tagname == NULL) {
				snprintf(config->error, sizeof(config->error),
						"unable to read name (%s), line %d", buff, config->line);
				goto clean;
			}

			if ((rtv = _xml_push_cf_xpath(tagname, config)) == -1)
				goto clean;

			if ((callbacks = _xml_xpath_callbacks(config)) != NULL)
				if (callbacks->on_tag_open)
					callbacks->on_tag_open(tagname, config);

			while ((next = _xml_get_next_attr(start, &attrname, &attrvalue,
					config)) != NULL) {
				if (attrname == NULL)
					break;

				if ((callbacks = _xml_xpath_callbacks(config)) != NULL)
					if (callbacks->on_tag_attr)
						callbacks->on_tag_attr(tagname, attrname, attrvalue,
								config);

				if (attrname)
					mm_free0((void*) &attrname);
				if (attrvalue)
					mm_free0((void*) &attrvalue);

				start = next;
			}
			if (tagname)
				mm_free0((void*) &tagname);

			if (type == XML_TAG_OPEN)
				break;
		case XML_TAG_CLOSE:
			if ((start = _xml_get_tag_name(buff, &tagname, config)) == NULL)
				continue;

			if (tagname == NULL) {
				snprintf(config->error, sizeof(config->error),
						"unable to read name (%s), line %d", buff, config->line);
				goto clean;
			}

			if ((callbacks = _xml_xpath_callbacks(config)) != NULL)
				if (callbacks->on_tag_close)
					callbacks->on_tag_close(tagname, config);

			_xml_pop_cf_xpath(config);

			if (tagname)
				mm_free0((void*) &tagname);

		case XML_TAG_META:
		case XML_TAG_SPEC:
		case XML_TAG_COMMENT:
			break;
		case XML_TAG_CDATA:
			if ((start = _xml_read_cdata(buff, config)) == NULL) {
				snprintf(config->error, sizeof(config->error),
						"unable to read cdata (%s), line %d", buff,
						config->line);
				goto clean;
			}

			if (strlen(start) > 0) {
				if ((callbacks = _xml_xpath_callbacks(config)) != NULL)
					if (callbacks->on_text)
						callbacks->on_text(start, config);
			}

			break;
		default:
			snprintf(config->error, sizeof(config->error),
					"unknow tag type, line %d", config->line);
			break;
		}

		if (_xml_read_text(fp, &buff, &size, config) <= 0)
			return -1;

		if (strlen(buff) > 0) {
			if ((callbacks = _xml_xpath_callbacks(config)) != NULL)
				if (callbacks->on_text)
					callbacks->on_text(buff, config);
		}
	}

clean:
	if (tagname)
		free(tagname);
	if (attrname)
		free(attrname);
	if (attrvalue)
		free(attrvalue);

	rtv = 0;

	if (fp)
		fclose(fp);

	if (buff)
		free(buff);

	return rtv;
}
