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

#include <types.h>
#include <mm.h>

char *str_dup_pool_alloc(MM_POOL *pool, const char *str) {
        char *dup;
        uint32_t len;

        len = strlen(str) + 1;

        if ((dup = mm_pool_alloc0(pool, len)) == NULL)
                return NULL;

        memcpy(dup, str, len);

        return dup;	
}

uint32_t str_lcpy(char *dst, const char *src, uint32_t size) {
        char *d = dst;
        const char *s = src;
        uint32_t n = size;

        if (n != 0 && --n != 0) {
                do {
                        if ((*d++ = *s++) == 0)
                                break;
                } while (--n != 0);
        }

        if (n == 0) {
                if (size != 0)
                        *d = '\0';
                while (*s++);
        }

        return (s - src - 1);
}

uint32_t str_lcat(char *dst, const char *src, uint32_t size) {
        char *d = dst;
        const char *s = src;
        uint32_t n = size, dlen;

        while (n-- != 0 && *d != '\0')
                d++;
        dlen = d - dst;
        n = size - dlen;

        if (n == 0)
                return (dlen + strlen(s));
        while (*s != '\0') {
                if (n != 1) {
                        *d++ = *s;
                        n--;
                }
                s++;
        }
        *d = '\0';

        return (dlen + (s - src));
}

