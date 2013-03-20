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

#include <mm.h>
#include "topt.h"

static str_to_argv_rtv_t _copy_raw_string(char **dest, char **src) {
	while (1) {
		char ch = *((*src)++);

		switch (ch) {
		case '\0':
			return STR2AV_UNBALANCED_QUOTE;
		case '\'':
			*(*dest) = '\0';

			return STR2AV_OK;
		case '\\':
			ch = *((*src)++);
			switch (ch) {
			case '\0':
				return STR2AV_UNBALANCED_QUOTE;
			default:
				/*
				 * unknown/invalid escape.  Copy escape character.
				 */
				*((*dest)++) = '\\';
				break;
			case '\\':
			case '\'':
				break;
			} /* FALLTHROUGH */

		default:
			*((*dest)++) = ch;
			break;
		}
	}

	return STR2AV_FATAL_ERROR;
}

static char _escape_convt(char **src) {
	char ch = *((*src)++);

	/*
	 *  Escape character is always eaten.  The next character is sometimes
	 *  treated specially.
	 */
	switch (ch) {
	case 'a':
		ch = '\a';
		break;
	case 'b':
		ch = '\b';
		break;
	case 't':
		ch = '\t';
		break;
	case 'n':
		ch = '\n';
		break;
	case 'v':
		ch = '\v';
		break;
	case 'f':
		ch = '\f';
		break;
	case 'r':
		ch = '\r';
		break;
	}

	return ch;
}

static str_to_argv_rtv_t _copy_cooked_string(char **dest, char **src) {
	while (1) {
		char ch = *((*src)++);
		switch (ch) {
		case '\0':
			return STR2AV_UNBALANCED_QUOTE;
		case '"':
			*(*dest) = '\0';

			return STR2AV_OK;
		case '\\':
			ch = _escape_convt(src);
			if (ch == '\0')
				return STR2AV_UNBALANCED_QUOTE;
			/* FALLTHROUGH */
		default:
			*((*dest)++) = ch;
			break;
		}
	}

	return STR2AV_FATAL_ERROR;
}

str_to_argv_rtv_t topt_str_to_argv(const char *str, int *argc_p, char ***argv_p) {
	int argc = 0, act = 10;
	char **res, **argv, **tmp;
	char *ptr, *scan, *dest;
	str_to_argv_rtv_t rtv = STR2AV_OK;

	if ((res = calloc(act, sizeof(char *) * act)) == NULL)
		return STR2AV_MEMORY_ERROR;
	argv = res;

	while (isspace((unsigned char) * str))
		str++;
	if ((ptr = strdup(str)) == NULL)
		return STR2AV_MEMORY_ERROR;
	scan = ptr;

	while (1) {
		while (isspace((unsigned char) *scan))
			scan++;
		if (*scan == '\0')
			break;

		if (++argc >= act - 1) {
			if ((tmp = calloc((act + act / 2), sizeof(char *))) == NULL) {
				rtv = STR2AV_MEMORY_ERROR;
				goto clean;
			}
			memcpy(tmp, res, sizeof(char *) * act);
			free(res);
			res = tmp;

			act += act / 2;
			argv = res + (argc - 1);
		}
		dest = scan;

		*(argv++) = dest;

		while (1) {
			char ch = *(scan++);
			switch (ch) {
			case '\0':
				goto done;
			case '\\':
				if ((*(dest++) = *(scan++)) == '\0')
					goto done;
				break;
			case '\'':
				rtv = _copy_raw_string(&dest, &scan);
				if (rtv != STR2AV_OK)
					goto clean;
				break;
			case '"':
				rtv = _copy_cooked_string(&dest, &scan);
				if (rtv != STR2AV_OK)
					goto clean;
				break;
			case ' ':
			case '\t':
			case '\n':
			case '\f':
			case '\r':
			case '\v':
			case '\b':
				goto token_done;
			default:
				*(dest++) = ch;
			}
		}

		token_done: *dest = '\0';
	}

	done: *argv_p = res;
	*argc_p = argc;
	*argv = NULL;

	return STR2AV_OK;

	clean: free(res);
	free((void *) ptr);

	return rtv;
}

void topt_free_argv(char **argv) {
	free(argv[0]);
	free(argv);
}
