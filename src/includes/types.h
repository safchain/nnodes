/*
*
* Author: Sylvain Afchain <safchain@gmail.com>, (C) 2008 - 2013
*
* Copyright: See COPYING file that comes with this distribution
*
*/

#ifndef __TYPES_H
#define __TYPES_H

#ifdef HAVE_STDINT_H
#include <stdint.h>     /* For uint32_t */
#elif HAVE_INTTYPE_H
#include <inttypes.h>
#else
typedef unsigned char uint8_t;
typedef unsigned int uint32_t;
typedef unsigned short uint16_t;
#endif

#endif
